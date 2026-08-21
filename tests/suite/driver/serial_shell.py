#!/usr/bin/env python3

"""
A board on the other end of a serial cable, driven as a shell.

The transport is the only thing that differs from the host process: bytes go
out of a uart instead of a pipe, and the target cannot be restarted by killing
a process. Everything the feature suites assert is the same either way.
"""

import select
import termios
import time

from .shell import Shell, ShellError

try:
    import serial
except ImportError:
    serial = None

DEFAULT_BAUD = 115200
DEFAULT_USER = "pdiStack"
DEFAULT_PASSWORD = "pdiStack@123"


class SerialShell(Shell):

    # Kept as margin, not as a workaround: the target used to discard unread
    # input on every flush, and answering a prompt instantly lost the answer
    # about one time in four. That is fixed, and --input-delay 0 now runs the
    # whole suite, but a board on older firmware still needs the pause.
    input_delay = 0.25

    def __init__(self, port, baud=DEFAULT_BAUD, reset=True, timeout=0.2):
        super().__init__()

        if serial is None:
            raise ShellError("pyserial is not installed; pip install pyserial")

        self._port_name = port
        self._baud = baud

        try:
            self._port = serial.Serial()
            self._port.port = port
            self._port.baudrate = baud
            self._port.timeout = timeout
            self._port.open()
            self._keep_lines_on_close()
        except Exception as err:
            raise ShellError("cannot open %s at %d: %s" % (port, baud, err))

        if reset:
            self.reset()
        else:
            self._port.reset_input_buffer()

    @property
    def port(self):
        return self._port_name

    def _keep_lines_on_close(self):
        """
        Stop the kernel dropping the modem lines when the port is closed.

        dtr and rts are wired to EN and GPIO0 on the usual esp dev boards, so a
        hangup on close resets the target — and the next run then attaches to a
        board that rebooted seconds ago, with its wifi still re-associating.
        That looked like a crashing device and a flapping station for a while;
        it was this.
        """
        try:
            attributes = termios.tcgetattr(self._port.fileno())
            attributes[2] &= ~termios.HUPCL
            termios.tcsetattr(self._port.fileno(), termios.TCSANOW, attributes)
        except Exception:
            pass

    def reset(self, settle=0.4):
        """
        Pulse EN so the next thing read is a boot banner rather than whatever
        the board happened to be printing.
        """
        self._buffer = ""
        self._port.reset_input_buffer()

        self._port.dtr = False
        self._port.rts = True
        time.sleep(0.1)
        self._port.rts = False
        time.sleep(settle)

    def _send(self, data):
        self._port.write(data.encode())
        self._port.flush()

    def _recv(self, timeout):
        # the wait is done with select rather than by assigning the port's own
        # timeout: pyserial reapplies the whole termios configuration on every
        # such assignment, and that pulses the control lines wired to EN, which
        # resets the board mid-test
        ready, _, _ = select.select([self._port.fileno()], [], [], timeout)
        if not ready:
            return ""

        waiting = self._port.in_waiting
        chunk = self._port.read(waiting if waiting else 1)
        if not chunk:
            return ""

        return chunk.decode(errors="replace")

    def login(self, username=DEFAULT_USER, password=DEFAULT_PASSWORD, timeout=30.0):
        return super().login(username, password, timeout)

    def is_listening(self, port, host=None):
        """
        Whether the board accepts on a port. Needs its address, which the
        caller knows and this transport does not, so an unaddressed call
        reports nothing rather than guessing at localhost.
        """
        if not host:
            return 0

        import socket
        try:
            with socket.create_connection((host, port), timeout=2.0):
                return port
        except OSError:
            return 0

    def close(self):
        try:
            self._port.close()
        except Exception:
            pass

    def __enter__(self):
        return self

    def __exit__(self, *exc):
        self.close()
        return False
