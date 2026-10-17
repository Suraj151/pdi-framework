#!/usr/bin/env python3

"""
Syslog forwarding, received for real on this machine.

The collector a board forwards to is fixed when the firmware is built
(SYSLOG_REMOTE_HOST, SYSLOG_REMOTE_PORT), and a normal build leaves it empty. So
nothing here assumes a host or a port: both are read from the board's own
`service status SYSLOG`, and the tests run only when the board has resolved its
collector to this machine. A build that does not forward, has no collector, has
not resolved it, or points at another machine skips with the reason. The
machine listens on the collector's udp port for the length of one test.

A line is raised on demand by asking the board's web server to take a body far
over its limit. The server refuses it before reading anything and logs the size
it was offered, so every test picks a size of its own and can tell its line
from anything else the board happens to log.

Two runs against two boards share this machine and the one port. A test that
finds the port taken waits for the other to finish rather than sharing it,
because the kernel would hand each datagram to only one of two listeners.
"""

import errno
import random
import socket
import ssl
import time

from .registry import test, Skip
from ..driver.mdns import local_address_towards
from ..driver.portal import Portal, _tls_context

PORT_WAIT = 180.0
LINE_WAIT = 10.0
SILENCE_WAIT = 4.0


def status_field(t, field):
    """One field of `service status SYSLOG`, or None when the build does not show it."""
    for line in t.run("service status SYSLOG").splitlines():
        name, _, value = line.partition(":")
        if name.strip() == field:
            return value.strip()
    return None


def state_of(t):
    return status_field(t, "state")


def collector_of(t):
    """
    The (host, port) this build forwards to, or a skip saying why there is none
    worth testing from here.
    """
    target = status_field(t, "collector")
    if target is None:
        raise Skip("this build does not forward syslog")
    if not target or target == "(none)":
        raise Skip("this build has no syslog collector configured")

    host, _, port = target.rpartition(":")
    resolved = status_field(t, "resolved")
    if not resolved or resolved == "no":
        raise Skip("the board has not resolved its collector %s" % host)

    here = local_address_towards(t.address())
    if resolved != here:
        raise Skip("the collector is %s (%s), not this machine (%s)" % (host, resolved, here))
    return resolved, int(port)


class Collector(object):
    """This machine's end of the forwarding, held for one test."""

    def __init__(self, port):
        self.port = port
        self.sock = None

    def __enter__(self):
        deadline = time.time() + PORT_WAIT
        while True:
            sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
            try:
                sock.bind(("0.0.0.0", self.port))
                self.sock = sock
                return self
            except OSError as err:
                sock.close()
                if err.errno != errno.EADDRINUSE or time.time() >= deadline:
                    raise Skip("could not listen on udp %d: %s" % (self.port, err))
                time.sleep(1.0)

    def __exit__(self, *exc):
        if self.sock is not None:
            self.sock.close()
            self.sock = None
        return False

    def wait_for(self, source, needle, wait):
        """The first datagram from source that carries needle, or None."""
        deadline = time.time() + wait
        while time.time() < deadline:
            self.sock.settimeout(max(0.1, deadline - time.time()))
            try:
                data, peer = self.sock.recvfrom(2048)
            except socket.timeout:
                break
            text = data.decode("latin1")
            if peer[0] == source and needle in text:
                return text
        return None


def raise_line(t):
    """
    Makes the board log one line of this test's own. Returns the text to look
    for in the datagram.
    """
    portal = Portal.reachable(t.address())
    if portal is None:
        raise Skip("the board's web server does not answer, so no line can be raised")

    size = 1000000 + random.randint(1, 899999)
    request = ("POST /syslog-probe HTTP/1.1\r\nHost: %s\r\nContent-Type: application/json\r\n"
               "Content-Length: %d\r\nConnection: close\r\n\r\n" % (portal.host, size))

    raw = socket.create_connection((portal.host, portal.port), timeout=portal.timeout)
    try:
        conn = _tls_context().wrap_socket(raw, server_hostname=portal.host) if portal.secure else raw
        conn.sendall(request.encode("ascii"))
        try:
            conn.recv(1024)
        except (socket.timeout, ssl.SSLError, OSError):
            pass
    finally:
        raw.close()

    return "body of %d bytes refused" % size


def expect_forwarded(t, collector, why):
    needle = raise_line(t)
    got = collector.wait_for(t.address(), needle, LINE_WAIT)
    if got is None:
        raise AssertionError("%s: no datagram carrying %r arrived" % (why, needle))
    return got


@test("a syslog line reaches the collector the board was built with",
      needs=("service",), services=("SYSLOG",))
def a_line_reaches_the_collector(t):
    _, port = collector_of(t)
    with Collector(port) as collector:
        got = expect_forwarded(t, collector, "a running service")

    board = t.address()
    if not got.startswith("<131>"):
        raise AssertionError("an error line should carry priority 131 (local0, err): %r" % got)
    if (" %s pdi: " % board) not in got:
        raise AssertionError("the header should name the board %s and the tag pdi: %r" % (board, got))
    if got.endswith("\n"):
        raise AssertionError("the datagram kept the line's newline: %r" % got)


@test("a stopped syslog service forwards nothing and a started one resumes",
      needs=("service",), services=("SYSLOG",))
def stop_and_start(t):
    _, port = collector_of(t)
    with Collector(port) as collector:
        try:
            t.run("service stop SYSLOG")
            needle = raise_line(t)
            leaked = collector.wait_for(t.address(), needle, SILENCE_WAIT)
            if leaked is not None:
                raise AssertionError("a stopped service still forwarded: %r" % leaked)
        finally:
            t.run("service start SYSLOG")

        if state_of(t) != "active":
            raise AssertionError("service start left SYSLOG %r" % state_of(t))
        expect_forwarded(t, collector, "after the service was started again")


@test("a restarted syslog service still forwards",
      needs=("service",), services=("SYSLOG",))
def restart_keeps_forwarding(t):
    _, port = collector_of(t)
    with Collector(port) as collector:
        try:
            for _ in range(3):
                t.run("service restart SYSLOG")
        finally:
            if state_of(t) != "active":
                t.run("service start SYSLOG")

        expect_forwarded(t, collector, "after three restarts")
