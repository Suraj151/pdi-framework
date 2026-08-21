#!/usr/bin/env python3

"""
A throwaway MQTT 3.1.1 broker, just enough to be talked to by one board.

Deliberately not a real broker and deliberately not somebody else's: a machine
doing embedded work often already runs a broker for something that matters, and
pointing tests at it puts test traffic on it and leaves retained messages
behind. This binds a port of its own, remembers what it was told, and goes away
with the test run.

Supports what a device client uses: CONNECT/CONNACK with credentials and a
will, SUBSCRIBE/SUBACK, PUBLISH in both directions at QoS 0 and 1, PINGREQ, and
DISCONNECT. It distinguishes a clean DISCONNECT from a dropped socket, which is
the whole point of testing a last will.
"""

import socket
import struct
import threading
import time

CONNECT = 1
CONNACK = 2
PUBLISH = 3
PUBACK = 4
SUBSCRIBE = 8
SUBACK = 9
UNSUBSCRIBE = 10
UNSUBACK = 11
PINGREQ = 12
PINGRESP = 13
DISCONNECT = 14


class Message(object):

    def __init__(self, topic, payload, qos, retain):
        self.topic = topic
        self.payload = payload
        self.qos = qos
        self.retain = retain
        self.at = time.time()

    @property
    def text(self):
        return self.payload.decode(errors="replace")

    def __repr__(self):
        return "<Message %s %r qos=%d retain=%s>" % (
            self.topic, self.payload[:40], self.qos, self.retain)


class Session(object):
    """What one connected client told us."""

    def __init__(self):
        self.client_id = ""
        self.username = ""
        self.password = ""
        self.keepalive = 0
        self.clean_session = None
        self.will = None            # Message, when the CONNECT carried one
        self.subscriptions = []
        self.connected_at = time.time()
        self.disconnected_cleanly = None


def _encode_remaining(length):
    out = bytearray()
    while True:
        byte = length % 128
        length //= 128
        if length:
            byte |= 0x80
        out.append(byte)
        if not length:
            return bytes(out)


def _encode_string(text):
    raw = text.encode() if isinstance(text, str) else text
    return struct.pack(">H", len(raw)) + raw


class MqttBroker(object):
    """
    Runs in a background thread. Everything it saw is readable from the test.

    One client at a time is all a single board needs, and keeping it to one
    keeps the state easy to reason about.
    """

    def __init__(self, host="0.0.0.0", port=0):
        self.host = host
        self.port = port            # 0 asks the kernel for a free one
        self.messages = []          # PUBLISH received from the client
        self.sessions = []          # one per CONNECT
        self.wills_delivered = []   # wills this broker published on a drop
        self.unsubscribed = []      # topics the client asked to leave
        self.pings = 0

        self._socket = None
        self._thread = None
        self._running = False
        self._client = None
        self._session = None
        self._lock = threading.Lock()

    # -- lifecycle ---------------------------------------------------------

    def start(self):
        self._socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self._socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        self._socket.bind((self.host, self.port))
        self.port = self._socket.getsockname()[1]
        self._socket.listen(4)
        self._socket.settimeout(0.5)

        self._running = True
        self._thread = threading.Thread(target=self._serve, daemon=True)
        self._thread.start()
        return self

    def stop(self):
        self._running = False
        if self._thread is not None:
            self._thread.join(timeout=5)
        for sock in (self._client, self._socket):
            try:
                if sock is not None:
                    sock.close()
            except Exception:
                pass

    def __enter__(self):
        return self.start()

    def __exit__(self, *exc):
        self.stop()
        return False

    # -- what the test asks --------------------------------------------------

    @property
    def session(self):
        with self._lock:
            return self.sessions[-1] if self.sessions else None

    def wait_for_connect(self, timeout=60.0):
        deadline = time.time() + timeout
        while time.time() < deadline:
            if self.session is not None:
                return self.session
            time.sleep(0.2)
        return None

    def wait_for_message(self, timeout=60.0, topic=None, since=0):
        """The next PUBLISH from the client, optionally on one topic."""
        deadline = time.time() + timeout
        while time.time() < deadline:
            with self._lock:
                found = [m for m in self.messages[since:]
                         if topic is None or m.topic == topic]
            if found:
                return found[0]
            time.sleep(0.2)
        return None

    def message_count(self):
        with self._lock:
            return len(self.messages)

    def connect_count(self):
        with self._lock:
            return len(self.sessions)

    def ping_count(self):
        with self._lock:
            return self.pings

    def wait_for_reconnect(self, since, timeout=90.0):
        """The next CONNECT after the one numbered `since`, or None."""
        deadline = time.time() + timeout
        while time.time() < deadline:
            if self.connect_count() > since:
                return self.session
            time.sleep(0.5)
        return None

    def wait_for_subscription(self, topic, timeout=60.0):
        """Wait until the client has subscribed to topic; returns its qos."""
        deadline = time.time() + timeout
        while time.time() < deadline:
            session = self.session
            if session is not None:
                for name, qos in list(session.subscriptions):
                    if name == topic:
                        return qos
            time.sleep(0.5)
        return None

    def wait_for_ping(self, timeout=60.0):
        """Wait for one more keepalive PINGREQ than we have already seen."""
        with self._lock:
            already = self.pings
        deadline = time.time() + timeout
        while time.time() < deadline:
            with self._lock:
                if self.pings > already:
                    return True
            time.sleep(0.5)
        return False

    def publish(self, topic, payload, qos=0):
        """Send a PUBLISH to the connected client."""
        if isinstance(payload, str):
            payload = payload.encode()

        header = _encode_string(topic)
        if qos:
            header += struct.pack(">H", 1)

        body = header + payload
        packet = bytes([(PUBLISH << 4) | (qos << 1)]) + _encode_remaining(len(body)) + body

        with self._lock:
            client = self._client
        if client is None:
            return False
        try:
            client.sendall(packet)
            return True
        except OSError:
            return False

    def drop_client(self):
        """Close the socket without a DISCONNECT, so the will must fire."""
        with self._lock:
            client = self._client
        if client is not None:
            try:
                client.shutdown(socket.SHUT_RDWR)
            except OSError:
                pass
            try:
                client.close()
            except OSError:
                pass

    # -- the server itself ---------------------------------------------------

    def _serve(self):
        while self._running:
            try:
                client, _ = self._socket.accept()
            except socket.timeout:
                continue
            except OSError:
                break

            client.settimeout(0.5)
            with self._lock:
                self._client = client
                self._session = None

            try:
                self._handle(client)
            except Exception:
                pass
            finally:
                self._finish_session()
                try:
                    client.close()
                except OSError:
                    pass
                with self._lock:
                    if self._client is client:
                        self._client = None

    def _finish_session(self):
        with self._lock:
            session = self._session
            self._session = None

        # a client that vanished without DISCONNECT gets its will published
        if session is not None and session.disconnected_cleanly is None:
            session.disconnected_cleanly = False
            if session.will is not None:
                self.wills_delivered.append(session.will)

    def _read_exactly(self, client, count):
        buf = b""
        while len(buf) < count:
            if not self._running:
                raise OSError("broker stopping")
            try:
                chunk = client.recv(count - len(buf))
            except socket.timeout:
                continue
            if not chunk:
                raise OSError("client closed")
            buf += chunk
        return buf

    def _read_packet(self, client):
        first = self._read_exactly(client, 1)[0]

        multiplier = 1
        length = 0
        while True:
            byte = self._read_exactly(client, 1)[0]
            length += (byte & 0x7F) * multiplier
            if not byte & 0x80:
                break
            multiplier *= 128

        body = self._read_exactly(client, length) if length else b""
        return first, body

    def _handle(self, client):
        while self._running:
            try:
                first, body = self._read_packet(client)
            except OSError:
                return

            kind = first >> 4

            if CONNECT == kind:
                self._on_connect(client, body)
            elif PUBLISH == kind:
                self._on_publish(client, first, body)
            elif SUBSCRIBE == kind:
                self._on_subscribe(client, body)
            elif UNSUBSCRIBE == kind:
                self._on_unsubscribe(client, body)
            elif PINGREQ == kind:
                with self._lock:
                    self.pings += 1
                client.sendall(bytes([PINGRESP << 4, 0]))
            elif DISCONNECT == kind:
                with self._lock:
                    if self._session is not None:
                        self._session.disconnected_cleanly = True
                return

    def _on_connect(self, client, body):
        offset = 0
        protocol_len = struct.unpack(">H", body[offset:offset + 2])[0]
        offset += 2 + protocol_len
        offset += 1                       # protocol level
        flags = body[offset]; offset += 1
        keepalive = struct.unpack(">H", body[offset:offset + 2])[0]; offset += 2

        def take_string():
            nonlocal offset
            size = struct.unpack(">H", body[offset:offset + 2])[0]
            offset += 2
            value = body[offset:offset + size]
            offset += size
            return value

        session = Session()
        session.keepalive = keepalive
        session.clean_session = bool(flags & 0x02)
        session.client_id = take_string().decode(errors="replace")

        if flags & 0x04:                  # will flag
            will_topic = take_string().decode(errors="replace")
            will_payload = take_string()
            session.will = Message(will_topic, will_payload,
                                   (flags & 0x18) >> 3, bool(flags & 0x20))

        if flags & 0x80:                  # username
            session.username = take_string().decode(errors="replace")
        if flags & 0x40:                  # password
            session.password = take_string().decode(errors="replace")

        with self._lock:
            self._session = session
            self.sessions.append(session)

        client.sendall(bytes([CONNACK << 4, 2, 0, 0]))   # accepted

    def _on_publish(self, client, first, body):
        qos = (first & 0x06) >> 1
        retain = bool(first & 0x01)

        topic_len = struct.unpack(">H", body[:2])[0]
        topic = body[2:2 + topic_len].decode(errors="replace")
        offset = 2 + topic_len

        packet_id = None
        if qos:
            packet_id = struct.unpack(">H", body[offset:offset + 2])[0]
            offset += 2

        with self._lock:
            self.messages.append(Message(topic, body[offset:], qos, retain))

        if 1 == qos and packet_id is not None:
            client.sendall(bytes([PUBACK << 4, 2]) + struct.pack(">H", packet_id))

    def _on_unsubscribe(self, client, body):
        packet_id = struct.unpack(">H", body[:2])[0]
        offset = 2

        while offset + 2 <= len(body):
            size = struct.unpack(">H", body[offset:offset + 2])[0]
            offset += 2
            topic = body[offset:offset + size].decode(errors="replace")
            offset += size
            with self._lock:
                self.unsubscribed.append(topic)
                if self._session is not None:
                    self._session.subscriptions = [
                        s for s in self._session.subscriptions if s[0] != topic]

        client.sendall(bytes([UNSUBACK << 4, 2]) + struct.pack(">H", packet_id))

    def _on_subscribe(self, client, body):
        packet_id = struct.unpack(">H", body[:2])[0]
        offset = 2

        granted = []
        while offset + 2 <= len(body):
            size = struct.unpack(">H", body[offset:offset + 2])[0]
            offset += 2
            topic = body[offset:offset + size].decode(errors="replace")
            offset += size
            qos = body[offset] if offset < len(body) else 0
            offset += 1
            granted.append(qos)
            with self._lock:
                if self._session is not None:
                    self._session.subscriptions.append((topic, qos))

        payload = bytes(granted)
        client.sendall(bytes([SUBACK << 4]) + _encode_remaining(2 + len(payload))
                       + struct.pack(">H", packet_id) + payload)
