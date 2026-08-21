#!/usr/bin/env python3

"""
A board reached over its own ssh server, driven as a shell.

This transport is also a test of the thing it runs over: the framework's ssh
server does the key exchange, the mac negotiation and the auth, and paramiko is
a client that was written for none of it in particular. A suite that runs here
proves interoperability as much as it proves the commands.

Unlike the serial and host transports there is no boot banner and no login
prompt — the ssh layer has already authenticated by the time a channel opens,
so wait_for_boot and login are satisfied without sending anything.
"""

import time

from .shell import Shell, ShellError, PROMPT

try:
    import paramiko
except ImportError:
    paramiko = None

DEFAULT_PORT = 22
DEFAULT_USER = "pdiStack"
DEFAULT_PASSWORD = "pdiStack@123"

# The board offers curve25519-sha256, the name RFC 8731 gave the algorithm.
# Paramiko implements exactly that algorithm but only advertises it under the
# older curve25519-sha256@libssh.org name, so the two cannot agree on a key
# exchange despite both supporting the same one. OpenSSH carries both names,
# which is why a normal ssh client connects and this one does not. Teaching the
# client the second name is right here; making the board advertise a vendor
# alias to suit one library is not.
def _register_standard_curve25519():
    if paramiko is None:
        return

    table = paramiko.Transport._kex_info
    legacy = table.get("curve25519-sha256@libssh.org")
    if legacy is not None and "curve25519-sha256" not in table:
        table["curve25519-sha256"] = legacy
        paramiko.Transport._preferred_kex = (
            ("curve25519-sha256",) + tuple(paramiko.Transport._preferred_kex))


_register_standard_curve25519()


class SshShell(Shell):

    # A handshake here is curve25519 and ed25519 on a microcontroller: 3 to 6
    # seconds when the board is idle, and longer while it is also serving a
    # filesystem workload. Paramiko reports its own impatience as "No existing
    # session", which reads like a broken server rather than a slow one, so the
    # budget is generous. It bounds a genuinely dead target, nothing more.
    CONNECT_TIMEOUT = 45.0

    def __init__(self, host, port=DEFAULT_PORT, username=DEFAULT_USER,
                 password=DEFAULT_PASSWORD, key_filename=None, timeout=CONNECT_TIMEOUT,
                 term="xterm", width=120, height=40):
        super().__init__()

        if paramiko is None:
            raise ShellError("paramiko is not installed; pip install paramiko")

        self._host = host
        self._port = port
        self._username = username

        self._client = paramiko.SSHClient()
        self._client.set_missing_host_key_policy(paramiko.AutoAddPolicy())

        try:
            self._client.connect(
                hostname=host,
                port=port,
                username=username,
                password=password if not key_filename else None,
                key_filename=key_filename,
                timeout=timeout,
                banner_timeout=timeout,
                auth_timeout=timeout,
                allow_agent=False,
                look_for_keys=False,
            )
        except Exception as err:
            raise ShellError("ssh to %s@%s:%d failed: %s" % (username, host, port, err))

        try:
            self._channel = self._client.invoke_shell(term=term, width=width, height=height)
        except Exception as err:
            self._client.close()
            raise ShellError("no shell channel on %s: %s" % (host, err))

        self._channel.settimeout(0.0)

    @property
    def host(self):
        return self._host

    @property
    def username(self):
        return self._username

    def _send(self, data):
        if self._channel.closed:
            raise ShellError("the ssh channel closed before input was sent")
        self._channel.sendall(data.encode())

    def _recv(self, timeout):
        deadline = time.time() + timeout
        while True:
            if self._channel.recv_ready():
                return self._channel.recv(4096).decode(errors="replace")

            if self._channel.exit_status_ready() and not self._channel.recv_ready():
                return ""

            if time.time() >= deadline:
                return ""

            time.sleep(min(0.02, max(0.0, deadline - time.time())))

    def wait_for_boot(self, timeout=20.0):
        """Already past the banner — settle on the first prompt instead."""
        return self.expect(PROMPT, timeout, consume=False)

    def login(self, username=None, password=None, timeout=20.0):
        """Authentication happened in the ssh handshake; just take the prompt."""
        return self.expect(PROMPT, timeout)

    def is_listening(self, port, host=None):
        import socket
        try:
            with socket.create_connection((host or self._host, port), timeout=2.0):
                return port
        except OSError:
            return 0

    def close(self):
        try:
            self._channel.close()
        except Exception:
            pass
        try:
            self._client.close()
        except Exception:
            pass

    def __enter__(self):
        return self

    def __exit__(self, *exc):
        self.close()
        return False
