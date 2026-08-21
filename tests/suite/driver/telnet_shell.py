#!/usr/bin/env python3

"""
A board reached over its telnet service, driven as a shell.

The service speaks no telnet option negotiation — it hands the socket straight
to the command line service — so this is a plain tcp socket carrying the same
prompt protocol the serial and ssh transports carry. Nothing IAC is sent for
that reason: a server that does not strip option bytes would read them as if
they had been typed.

Worth having for what it costs: the whole feature suite runs over it unchanged,
and it is the only thing that exercises TelnetServiceProvider's own client
handling and its flush.
"""

import socket
import time

from .shell import Shell, ShellError

DEFAULT_PORT = 23
DEFAULT_USER = "pdiStack"
DEFAULT_PASSWORD = "pdiStack@123"

# a listener the host refuses at its real port moves here, see the mock
# TcpServerInterface
SHADOW_PORT_BASE = 10000

IAC = 0xFF


class TelnetShell(Shell):

    def __init__(self, host, port=DEFAULT_PORT, timeout=20.0, try_shadow=True):
        super().__init__()

        self._host = host
        self._port = port

        candidates = [port]
        if try_shadow and port < SHADOW_PORT_BASE:
            candidates.append(SHADOW_PORT_BASE + port)

        last = None
        self._socket = None
        for candidate in candidates:
            try:
                self._socket = socket.create_connection((host, candidate), timeout=timeout)
                self._port = candidate
                break
            except OSError as err:
                last = err

        if self._socket is None:
            raise ShellError("telnet to %s:%s failed: %s" % (host, candidates, last))

        self._socket.setblocking(False)

    @property
    def host(self):
        return self._host

    @property
    def port(self):
        return self._port

    def _send(self, data):
        try:
            self._socket.sendall(data.encode())
        except OSError as err:
            raise ShellError("the telnet connection closed before input was sent: %s" % err)

    def _recv(self, timeout):
        import select as _select

        ready, _, _ = _select.select([self._socket], [], [], timeout)
        if not ready:
            return ""

        try:
            chunk = self._socket.recv(4096)
        except BlockingIOError:
            return ""
        except OSError as err:
            raise ShellError("the telnet connection dropped: %s" % err)

        if not chunk:
            raise ShellError("the telnet connection was closed by the target")

        # nothing here negotiates, but a peer that does would otherwise leave
        # option bytes in the middle of the output
        if IAC in chunk:
            chunk = self._strip_iac(chunk)

        return chunk.decode(errors="replace")

    @staticmethod
    def _strip_iac(chunk):
        out = bytearray()
        index = 0
        while index < len(chunk):
            if chunk[index] == IAC and index + 1 < len(chunk):
                command = chunk[index + 1]
                if command == IAC:
                    out.append(IAC)
                    index += 2
                elif command in (251, 252, 253, 254):  # WILL WONT DO DONT
                    index += 3
                else:
                    index += 2
            else:
                out.append(chunk[index])
                index += 1

        return bytes(out)

    def login(self, username=DEFAULT_USER, password=DEFAULT_PASSWORD, timeout=30.0):
        return super().login(username, password, timeout)

    def is_listening(self, port, host=None):
        try:
            with socket.create_connection((host or self._host, port), timeout=2.0):
                return port
        except OSError:
            return 0

    def close(self):
        try:
            self._socket.close()
        except Exception:
            pass

    def __enter__(self):
        return self

    def __exit__(self, *exc):
        self.close()
        return False
