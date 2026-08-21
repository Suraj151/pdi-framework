#!/usr/bin/env python3

"""
A shell on a pdi target, whatever it is reached over.

Everything above this file is written against Shell alone, so the same
assertions run against the host process, a board on a serial cable or a board
over ssh. A transport supplies the bytes; this supplies the prompt handling.
"""

import re
import time

# csi sequences the line editor emits while redrawing
ANSI = re.compile(r"\x1b\[[0-9;?]*[ -/]*[@-~]")

# the stack keeps logging while a prompt sits there waiting, so a prompt is
# matched wherever it appears rather than only at the end of what has arrived
#
# user@deviceid:(/path): with storage, user@deviceid: without
PROMPT = re.compile(r"[^\s@]+@[^\s:]+:(\([^)]*\))?: ")

LOGIN_PROMPT = re.compile(r"login: ")
PASSWORD_PROMPT = re.compile(r"Pass : ")

CTRL_C = "\x03"

DEFAULT_TIMEOUT = 20.0


class ShellError(Exception):
    pass


class ShellTimeout(ShellError):
    def __init__(self, expected, seen):
        self.expected = expected
        self.seen = seen
        super().__init__("timed out waiting for %s\n--- seen ---\n%s" % (expected, seen))


def strip_ansi(text):
    return ANSI.sub("", text)


class Shell(object):
    """
    Transports implement _send, _recv and close. _recv returns whatever has
    arrived within its timeout and an empty string when nothing has.
    """

    # a transport that needs a pause between reading a prompt and answering it
    # raises this; see SerialShell
    input_delay = 0.0

    def __init__(self):
        self._buffer = ""
        self._log = ""

    def _send(self, data):
        raise NotImplementedError

    def _recv(self, timeout):
        raise NotImplementedError

    def close(self):
        raise NotImplementedError

    @property
    def transcript(self):
        """Everything read so far, for a failure message worth reading."""
        return self._log

    def expect(self, pattern, timeout=DEFAULT_TIMEOUT, consume=True):
        """
        Read until pattern matches, and return everything up to and including
        the match. What matched is consumed unless asked otherwise, so the next
        call starts after it.
        """
        if isinstance(pattern, str):
            pattern = re.compile(re.escape(pattern))

        deadline = time.time() + timeout
        while True:
            clean = strip_ansi(self._buffer)
            found = pattern.search(clean)
            if found:
                self._buffer = clean[found.end():] if consume else clean
                return clean[:found.end()]

            remaining = deadline - time.time()
            if remaining <= 0:
                raise ShellTimeout(pattern.pattern, strip_ansi(self._buffer))

            chunk = self._recv(min(remaining, 0.2))
            if chunk:
                self._buffer += chunk
                self._log += chunk

    def send_line(self, line):
        self.send_raw(line + "\n")

    def send_raw(self, data):
        """
        Put bytes on the wire with nothing added. The line editor is driven a
        key at a time this way — arrows, tab, ctrl-c — so what a real keyboard
        produces is what the target sees.

        The transport's input delay applies here rather than only to whole
        lines: a single control byte sent the instant a prompt appears is lost
        the same way a line is.
        """
        if self.input_delay:
            time.sleep(self.input_delay)

        self._send(data)

    def drain(self, settle=0.3, limit=2.0):
        """
        Read whatever is still coming until it stops for `settle` seconds, and
        return it. Used to reach a known-quiet point before typing, and to see
        what the editor echoed when there is no prompt to wait for.
        """
        deadline = time.time() + limit
        while time.time() < deadline:
            chunk = self._recv(settle)
            if not chunk:
                break
            self._buffer += chunk
            self._log += chunk

        seen = strip_ansi(self._buffer)
        self._buffer = ""
        return seen

    def answer(self, line, pattern, timeout=DEFAULT_TIMEOUT, retries=1):
        """
        Reply to a prompt and wait for what should follow.

        A line sent the moment a prompt appears is sometimes not seen by the
        target at all, and the command then waits for input that will never
        arrive. The echo tells the two cases apart: a line that was received is
        echoed straight back, so silence for a beat means the line was dropped
        and can be sent again. A line that was echoed is never resent, however
        slow the rest of the answer is.
        """
        for attempt in range(retries + 1):
            self.send_line(line)

            echoed = self._recv(1.5)
            if echoed:
                self._buffer += echoed
                self._log += echoed
                return self.expect(pattern, timeout)

            if attempt == retries:
                return self.expect(pattern, timeout)

        raise ShellError("unreachable")

    def login(self, username, password, timeout=DEFAULT_TIMEOUT, attempts=2):
        """
        Answer the three line interactive login and land on a prompt.

        A dropped line is retried by starting the whole login again rather than
        by resending: the password prompt is masked, and a password that was in
        fact received would be sent a second time into whatever came next.
        Interrupting and beginning again cannot do that.
        """
        for attempt in range(attempts):
            try:
                self.expect(LOGIN_PROMPT, timeout)
                self.answer(username, PASSWORD_PROMPT, timeout)
                self.send_line(password)
                return self.expect(PROMPT, timeout)
            except ShellTimeout:
                if attempt + 1 == attempts:
                    raise

                self.send_raw(CTRL_C)
                self.drain(0.4, 4.0)
                self.send_raw("\n")
                self.drain(0.4, 4.0)

    def run(self, command, timeout=DEFAULT_TIMEOUT):
        """
        Run one command and return its output, with the echoed command line and
        the following prompt taken off.
        """
        self.send_line(command)
        raw = self.expect(PROMPT, timeout)

        body = raw
        echo = body.find(command)
        if echo >= 0:
            body = body[echo + len(command):]

        lines = [line.rstrip("\r") for line in body.split("\n")]
        if lines:
            lines.pop()

        return "\n".join(lines).strip("\r\n")

    def wait_for_boot(self, timeout=DEFAULT_TIMEOUT):
        """
        Read the boot banner and stop at the login prompt, leaving it there for
        login to answer.
        """
        return self.expect(LOGIN_PROMPT, timeout, consume=False)

    def attach(self, username, password, timeout=DEFAULT_TIMEOUT):
        """
        Reach a usable prompt, whatever state the target is in.

        A target that has just been started is still printing its banner and
        will offer a login prompt. A board that has been running for hours
        prints nothing at all until it is spoken to, and may already be logged
        in — resetting it to get a predictable banner would also drop its
        network, which the tests that need one would then skip.
        """
        self.send_raw("\n")
        seen = self.drain(0.5, 6.0)

        if PROMPT.search(seen) and not LOGIN_PROMPT.search(seen):
            return seen

        if LOGIN_PROMPT.search(seen):
            # drain consumed the prompt login is about to wait for, so it goes
            # back before handing over
            self._buffer = seen + self._buffer
            return self.login(username, password, timeout)

        # nothing yet: a target still coming up, so wait out the banner
        self.expect(LOGIN_PROMPT, timeout, consume=False)
        return self.login(username, password, timeout)
