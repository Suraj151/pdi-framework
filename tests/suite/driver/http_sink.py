#!/usr/bin/env python3

"""
A throwaway HTTP server that a board reports to: it keeps every request whole
and answers with whatever status the test has set.

The board is the client here — posting its pins to the gpio server, an alert
to the event url — so the assertions are about what arrived: the method, the
path, the headers and the body. A test sets the status to see a refusal
handled, or makes the server hold the connection open to see a stalled server
handled without the board stalling with it.
"""

import socket
import threading
import time


class Request(object):

    def __init__(self, method, path, headers, body, at):
        self.method = method
        self.path = path
        self.headers = headers
        self.body = body
        self.at = at

    def text(self):
        return self.body.decode(errors="replace")

    def __repr__(self):
        return "%s %s %r" % (self.method, self.path, self.body[:120])


class HttpSink(object):

    def __init__(self, host="0.0.0.0", port=0):
        self._host = host
        self._port = port
        self._sock = None
        self._thread = None
        self._stop = threading.Event()
        self._lock = threading.Lock()
        self._requests = []
        self.status = 200
        self.reply = b"{\"status\":true}"
        self.hang = False

    @property
    def port(self):
        return self._sock.getsockname()[1] if self._sock else self._port

    def start(self):
        self._sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self._sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        self._sock.bind((self._host, self._port))
        self._sock.listen(8)
        self._sock.settimeout(0.5)
        self._thread = threading.Thread(target=self._serve, daemon=True)
        self._thread.start()
        return self

    def stop(self):
        self._stop.set()
        if self._thread is not None:
            self._thread.join(timeout=5.0)
        if self._sock is not None:
            self._sock.close()
            self._sock = None

    def requests(self, path_part=""):
        """Every request so far whose path contains the given text, in order."""
        with self._lock:
            return [r for r in self._requests if path_part in r.path]

    def clear(self):
        with self._lock:
            self._requests = []

    def wait_for(self, path_part, timeout, after=0):
        """The first request past the given count whose path matches, or None."""
        deadline = time.time() + timeout
        while time.time() < deadline:
            found = self.requests(path_part)
            if len(found) > after:
                return found[after]
            time.sleep(0.1)
        return None

    def _serve(self):
        while not self._stop.is_set():
            try:
                conn, _ = self._sock.accept()
            except socket.timeout:
                continue
            except OSError:
                return
            threading.Thread(target=self._handle, args=(conn,), daemon=True).start()

    def _handle(self, conn):
        conn.settimeout(10.0)
        try:
            request = self._read(conn)
            if request is None:
                return
            with self._lock:
                self._requests.append(request)

            if self.hang:
                # held open with no answer until the server is stopped
                while not self._stop.is_set():
                    time.sleep(0.2)
                return

            reason = {200: "OK", 404: "Not Found", 500: "Internal Server Error"}.get(self.status, "Status")
            head = ("HTTP/1.1 %d %s\r\nContent-Type: application/json\r\nContent-Length: %d\r\n"
                    "Connection: close\r\n\r\n" % (self.status, reason, len(self.reply)))
            conn.sendall(head.encode() + self.reply)
        except (socket.timeout, OSError):
            pass
        finally:
            conn.close()

    def _read(self, conn):
        data = b""
        while b"\r\n\r\n" not in data:
            chunk = conn.recv(4096)
            if not chunk:
                return None
            data += chunk

        head, _, body = data.partition(b"\r\n\r\n")
        lines = head.decode(errors="replace").split("\r\n")
        parts = lines[0].split(" ")
        if len(parts) < 2:
            return None

        headers = {}
        for line in lines[1:]:
            key, _, value = line.partition(":")
            headers[key.strip().lower()] = value.strip()

        length = int(headers.get("content-length", "0") or 0)
        while len(body) < length:
            chunk = conn.recv(4096)
            if not chunk:
                break
            body += chunk

        return Request(parts[0], parts[1], headers, body, time.time())
