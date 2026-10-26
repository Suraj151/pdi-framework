#!/usr/bin/env python3

"""
A TCP proxy that makes the network between a client and the target misbehave
on cue, the way real links and real peers do.

The framework's network stack is exercised by well behaved clients in every
other suite, so the paths a bad link takes are reached only by accident on a
board. This sits in the middle of one connection and changes how bytes travel,
never what they are:

    to_target / from_target   one Direction per way, each holding
      chunk      bytes forwarded per write; 1 sends every byte in its own segment
      delay      seconds slept before each forwarded chunk
      stall      seconds this way pauses once, after `stall_after` bytes
      reset_after  bytes after which the connection is torn down with a reset
      read_size  bytes taken from the source per read; with a small receive
                 buffer this makes a slow reader, whose window closes

Point a client at proxy.port on 127.0.0.1 and it reaches the target through the
proxy. A proxy serves any number of connections, one after another or at once,
each with the settings current when it was accepted.
"""

import socket
import struct
import threading
import time


class Direction(object):

    def __init__(self, chunk=0, delay=0.0, stall=0.0, stall_after=0, reset_after=0,
                 read_size=65536, rcvbuf=0):
        self.chunk = chunk
        self.delay = delay
        self.stall = stall
        self.stall_after = stall_after
        self.reset_after = reset_after
        self.read_size = read_size
        self.rcvbuf = rcvbuf

    def copy(self):
        return Direction(self.chunk, self.delay, self.stall, self.stall_after,
                         self.reset_after, self.read_size, self.rcvbuf)


def _reset(sock):
    """Close with a reset instead of a FIN, the way a peer that crashed does."""
    try:
        sock.setsockopt(socket.SOL_SOCKET, socket.SO_LINGER, struct.pack("ii", 1, 0))
    except OSError:
        pass
    try:
        sock.close()
    except OSError:
        pass


class ImpairProxy(object):

    def __init__(self, target_host, target_port, to_target=None, from_target=None,
                 listen_host="127.0.0.1"):
        self.target = (target_host, target_port)
        self.to_target = to_target or Direction()
        self.from_target = from_target or Direction()
        self._listener = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self._listener.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        self._listener.bind((listen_host, 0))
        self._listener.listen(16)
        self.port = self._listener.getsockname()[1]
        self.host = listen_host
        self._stop = threading.Event()
        self._threads = []
        self._lock = threading.Lock()
        self.connections = 0
        self.bytes_to_target = 0
        self.bytes_from_target = 0
        self.resets = 0
        accept = threading.Thread(target=self._accept, daemon=True)
        accept.start()
        self._threads.append(accept)

    def __enter__(self):
        return self

    def __exit__(self, *exc):
        self.close()

    def _accept(self):
        self._listener.settimeout(0.2)
        while not self._stop.is_set():
            try:
                client, _ = self._listener.accept()
            except socket.timeout:
                continue
            except OSError:
                return

            up = self.to_target.copy()
            down = self.from_target.copy()
            try:
                server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
                if down.rcvbuf:
                    server.setsockopt(socket.SOL_SOCKET, socket.SO_RCVBUF, down.rcvbuf)
                server.settimeout(15.0)
                server.connect(self.target)
                server.settimeout(None)
            except OSError:
                _reset(client)
                continue

            if up.rcvbuf:
                client.setsockopt(socket.SOL_SOCKET, socket.SO_RCVBUF, up.rcvbuf)
            for sock in (client, server):
                sock.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)

            with self._lock:
                self.connections += 1

            pair = {"client": client, "server": server, "dead": threading.Event(), "ended": 0}
            for src, dst, how, counter in ((client, server, up, "bytes_to_target"),
                                           (server, client, down, "bytes_from_target")):
                worker = threading.Thread(target=self._pump,
                                          args=(src, dst, how, counter, pair), daemon=True)
                worker.start()
                self._threads.append(worker)

    def _pump(self, src, dst, how, counter, pair):
        moved = 0
        stalled = False
        src.settimeout(0.2)
        try:
            while not self._stop.is_set() and not pair["dead"].is_set():
                try:
                    data = src.recv(how.read_size)
                except socket.timeout:
                    continue
                if not data:
                    try:
                        dst.shutdown(socket.SHUT_WR)
                    except OSError:
                        pass
                    with self._lock:
                        pair["ended"] += 1
                        both = 2 == pair["ended"]
                    if both:
                        pair["dead"].set()
                    return

                step = how.chunk if how.chunk > 0 else len(data)
                for at in range(0, len(data), step):
                    piece = data[at:at + step]

                    if how.reset_after and moved + len(piece) >= how.reset_after:
                        keep = how.reset_after - moved
                        if keep > 0:
                            dst.sendall(piece[:keep])
                        with self._lock:
                            self.resets += 1
                        pair["dead"].set()
                        _reset(pair["client"])
                        _reset(pair["server"])
                        return

                    if how.stall and not stalled and moved + len(piece) > how.stall_after:
                        head = max(0, how.stall_after - moved)
                        if head:
                            dst.sendall(piece[:head])
                            moved += head
                            piece = piece[head:]
                        stalled = True
                        time.sleep(how.stall)
                    if how.delay:
                        time.sleep(how.delay)

                    dst.sendall(piece)
                    moved += len(piece)
                    with self._lock:
                        setattr(self, counter, getattr(self, counter) + len(piece))
        except OSError:
            pair["dead"].set()
        finally:
            if pair["dead"].is_set():
                for sock in (pair["client"], pair["server"]):
                    try:
                        sock.close()
                    except OSError:
                        pass

    def close(self):
        self._stop.set()
        try:
            self._listener.close()
        except OSError:
            pass
        for worker in self._threads:
            worker.join(timeout=2.0)
