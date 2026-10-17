#!/usr/bin/env python3

"""
The telnet server as a real telnet client meets it, over a raw socket.

A client usually opens by negotiating options and may report its window size
at any time. Those bytes are protocol, not typing: the server has to take them
out of the stream, answer each offer — refusing is always a valid answer — and
leave the login and the shell to see only what the user typed. The suite's own
shell driver sends no negotiation at all, which is why none of this was seen.
"""

import socket
import time

from .registry import test, Skip
from .test_portal import upload
from ..driver.host_shell import HostShell, SHADOW_PORT_BASE

IAC, DONT, DO, WONT, WILL, SB, SE, NOP = 255, 254, 253, 252, 251, 250, 240, 241
ECHO, SGA, TTYPE, NAWS, TSPEED, LINEMODE = 1, 3, 24, 31, 32, 34
PROMPT_MARK = b":("


class Raw(object):
    """A telnet connection with nothing between the test and the bytes."""

    def __init__(self, t):
        port = SHADOW_PORT_BASE + 23 if isinstance(t.shell, HostShell) else 23
        try:
            self.sock = socket.create_connection((t.address(), port), timeout=8)
        except OSError as error:
            raise Skip("no telnet server reachable on port %d: %s" % (port, error))
        self.t = t

    def send(self, data):
        self.sock.sendall(data)

    def read_for(self, seconds):
        self.sock.settimeout(0.3)
        out = b""
        end = time.time() + seconds
        while time.time() < end:
            try:
                chunk = self.sock.recv(4096)
            except socket.timeout:
                continue
            if not chunk:
                break
            out += chunk
        return out

    def read_until(self, marker, seconds=8.0):
        self.sock.settimeout(0.3)
        out = b""
        end = time.time() + seconds
        while time.time() < end and marker not in out:
            try:
                chunk = self.sock.recv(4096)
            except socket.timeout:
                continue
            if not chunk:
                break
            out += chunk
        return out

    def login(self):
        self.read_until(b"login:")
        self.send(self.t.username.encode() + b"\r\n")
        self.read_until(b"Pass")
        self.send(self.t.password.encode() + b"\r\n")
        seen = self.read_until(PROMPT_MARK)
        if PROMPT_MARK not in seen:
            raise AssertionError("the login did not reach a prompt: %r" % seen[-200:])

    def run(self, line):
        self.send(line + b"\r\n")
        return self.read_until(PROMPT_MARK)

    def close(self):
        try:
            self.send(b"logout\r\n")
        except OSError:
            pass
        self.sock.close()


def negotiation(*pairs):
    return b"".join(bytes([IAC, command, option]) for command, option in pairs)


@test("a client that opens by negotiating options logs in with the first name it types")
def negotiating_client_logs_in(t):
    raw = Raw(t)
    try:
        raw.send(negotiation((DO, SGA), (WILL, TTYPE), (WILL, NAWS), (WILL, TSPEED), (WILL, LINEMODE), (DO, ECHO)))
        raw.send(t.username.encode() + b"\r\n")
        seen = raw.read_until(b"Pass")
        if b"Pass" not in seen:
            raise AssertionError("the negotiation broke the login prompt: %r" % seen[-300:])
        if b"CmdErr" in seen or seen.count(b"login:") > 1:
            raise AssertionError("the negotiation was typed into the login: %r" % seen[-300:])
        raw.send(t.password.encode() + b"\r\n")
        if PROMPT_MARK not in raw.read_until(PROMPT_MARK):
            raise AssertionError("the first username after a negotiation did not log in")
    finally:
        raw.close()


@test("every option a client offers is answered with a refusal")
def offers_are_refused(t):
    raw = Raw(t)
    try:
        raw.read_until(b"login:")
        raw.send(negotiation((DO, SGA), (WILL, NAWS), (DO, 99)))
        reply = raw.read_for(1.5)
        for expected in (bytes([IAC, WONT, SGA]), bytes([IAC, DONT, NAWS]), bytes([IAC, WONT, 99])):
            if expected not in reply:
                raise AssertionError("no %r in the reply to the offers: %r" % (expected, reply))
        if bytes([WILL]) in reply.replace(bytes([IAC, WILL]), b""):
            raise AssertionError("the offer bytes were echoed back as text: %r" % reply)
    finally:
        raw.close()


@test("a window size report mid-session never reaches the shell")
def naws_mid_session(t):
    raw = Raw(t)
    try:
        raw.login()
        out = raw.run(bytes([IAC, SB, NAWS, 0, 80, 0, 24, IAC, SE]) + b"echo after")
        if b"command not found" in out or b"after" not in out:
            raise AssertionError("a window size report broke the next command: %r" % out[-200:])
    finally:
        raw.close()


@test("a two byte telnet command between keystrokes is ignored")
def nop_ignored(t):
    raw = Raw(t)
    try:
        raw.login()
        out = raw.run(b"echo n" + bytes([IAC, NOP]) + b"op")
        if b"nop" not in out:
            raise AssertionError("an IAC NOP inside a line changed it: %r" % out[-200:])
    finally:
        raw.close()


@test("a 0xFF byte in the output reaches the client doubled", needs=("cat", "rm"))
def output_ff_is_escaped(t):
    raw = Raw(t)
    path = "/telnet_ff.bin"
    try:
        raw.login()
        t.run("rm %s" % path)
        upload(t, t.portal(), "telnet_ff.bin", b"A\xffB")
        if "A" not in t.run("cat %s" % path):
            raise Skip("could not put a binary file on the target to print")
        out = raw.run(("cat %s" % path).encode())
        if b"A\xff\xffB" not in out:
            raise AssertionError("a 0xFF byte went out undoubled: %r" % out[-80:])
    finally:
        t.run("rm %s" % path)
        raw.close()


@test("a line ended by CR NUL, CR LF or a bare LF is run once")
def line_endings(t):
    raw = Raw(t)
    try:
        raw.login()
        for ending, word in ((b"\r\x00", b"crnul"), (b"\r\n", b"crlf"), (b"\n", b"lf")):
            raw.send(b"echo " + word + ending)
            out = raw.read_until(PROMPT_MARK)
            if out.count(word) < 1 or b"command not found" in out:
                raise AssertionError("a line ended by %r ran as %r" % (ending, out[-200:]))
    finally:
        raw.close()
