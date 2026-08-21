#!/usr/bin/env python3

"""
The pdi stack running as a host process, driven as a shell.

Each instance gets its own config store and filesystem image in a scratch
directory, so one test cannot see what another left behind and several can run
at once.
"""

import os
import select
import shutil
import signal
import socket
import subprocess
import tempfile

from .shell import Shell, ShellError

DEFAULT_USER = "pdiStack"
DEFAULT_PASSWORD = "pdiStack@123"

# a listener the host refuses at its real port moves here, see the mock
# TcpServerInterface
SHADOW_PORT_BASE = 10000


class HostShell(Shell):

    def __init__(self, binary, workdir=None, seed=1, epoch=0, args=None, quiet_leaks=True):
        super().__init__()

        # the process runs in its own scratch directory, so the path to it has
        # to stop being relative before that happens
        binary = os.path.abspath(binary)
        if not os.path.exists(binary):
            raise ShellError("pdid not built at %s" % binary)

        self._binary = binary
        self._owns_workdir = workdir is None
        self._workdir = workdir or tempfile.mkdtemp(prefix="pdid-")

        argv = [
            binary,
            "--nvm", os.path.join(self._workdir, "nvm.bin"),
            "--fs-image", os.path.join(self._workdir, "fs.img"),
            "--seed", str(seed),
        ]
        if epoch:
            argv += ["--epoch", str(epoch)]
        if args:
            argv += list(args)

        env = dict(os.environ)
        if quiet_leaks:
            # the command registry is a program lifetime static a device never
            # tears down, so it is still held at exit and is not a finding here
            env["ASAN_OPTIONS"] = "detect_leaks=0"

        self._proc = subprocess.Popen(
            argv,
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            env=env,
            cwd=self._workdir,
        )

    @property
    def host(self):
        return "127.0.0.1"

    @property
    def pid(self):
        return self._proc.pid

    @property
    def workdir(self):
        return self._workdir

    @property
    def binary(self):
        return self._binary

    def _send(self, data):
        if self._proc.poll() is not None:
            raise ShellError("pdid exited with %d before input was sent" % self._proc.returncode)
        self._proc.stdin.write(data.encode())
        self._proc.stdin.flush()

    def _recv(self, timeout):
        ready, _, _ = select.select([self._proc.stdout], [], [], timeout)
        if not ready:
            return ""

        chunk = os.read(self._proc.stdout.fileno(), 4096)
        if not chunk:
            return ""

        return chunk.decode(errors="replace")

    def login(self, username=DEFAULT_USER, password=DEFAULT_PASSWORD, timeout=20.0):
        return super().login(username, password, timeout)

    def is_listening(self, port):
        """
        The port pdid is accepting on, or 0.

        The shadow is tried first, and this matters: the developer machine may
        well be running its own ssh or web server on the real port, and asking
        there first reports someone else's listener as if it were the target's.
        A port pdid could not bind is exactly the one it moved to its shadow.
        """
        for candidate in (SHADOW_PORT_BASE + port, port):
            try:
                with socket.create_connection(("127.0.0.1", candidate), timeout=1.0):
                    return candidate
            except OSError:
                continue

        return 0

    def stop(self, timeout=10.0):
        """Signal the way a power down would, and report the exit status."""
        if self._proc.poll() is None:
            self._proc.send_signal(signal.SIGTERM)

        try:
            self._proc.wait(timeout=timeout)
        except subprocess.TimeoutExpired:
            self._proc.kill()
            self._proc.wait(timeout=timeout)
            raise ShellError("pdid ignored SIGTERM")

        # whatever the shutdown path wrote is still in the pipe, and it is the
        # part a caller checking an orderly stop cares about
        while True:
            tail = self._recv(0.2)
            if not tail:
                break
            self._buffer += tail
            self._log += tail

        return self._proc.returncode

    def close(self):
        try:
            if self._proc.poll() is None:
                self._proc.kill()
                self._proc.wait(timeout=5)
        finally:
            for stream in (self._proc.stdin, self._proc.stdout):
                try:
                    stream.close()
                except Exception:
                    pass
            if self._owns_workdir:
                shutil.rmtree(self._workdir, ignore_errors=True)

    def __enter__(self):
        return self

    def __exit__(self, *exc):
        self.close()
        return False
