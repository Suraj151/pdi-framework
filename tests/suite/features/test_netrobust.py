#!/usr/bin/env python3

"""
The network stack against a link and a peer that behave badly.

lwIP's harder paths — a sender that dribbles one small segment at a time, a peer
that resets in the middle of a body, a reader that stops reading so the window
closes, a connection that stalls — are reached only by accident in the other
suites. Here an impairing proxy sits between the framework and a server of our
own and makes those paths happen on purpose. The server side is reached the same
way, by impairing the bytes the framework's own server sends back.

The rule throughout: the operation completes or fails with an error, the shell
answers afterwards, and nothing is left hung. These drive the framework's HTTP
client (wget), because it is the client with a server double to misbehave as.
"""

import socket

from .registry import test, Skip, expect_in
from ..driver.http_origin import Origin
from ..driver.impair import ImpairProxy, Direction
from ..driver.mdns import local_address_towards

_ORIGIN = {}


def origin(t):
    """One shared origin, reachable from the target, for this run."""
    if "origin" not in _ORIGIN:
        try:
            address = local_address_towards(t.address())
            serv = Origin("0.0.0.0", 0).start()
        except Exception as err:
            raise Skip("could not stand up an origin: %s" % err)
        _ORIGIN["origin"] = serv
        _ORIGIN["address"] = address
        t.at_exit(serv.stop)
    return _ORIGIN["origin"], _ORIGIN["address"]


def proxy_for(t, to_target=None, from_target=None):
    serv, address = origin(t)
    proxy = ImpairProxy("127.0.0.1", serv.port, to_target=to_target,
                        from_target=from_target, listen_host="0.0.0.0")
    return proxy, address


def wget_through(t, proxy, address, path, dest="/nr_dl.bin", timeout=120.0):
    url = "http://%s:%d%s" % (address, proxy.port, path)
    t.run("rm %s" % dest)
    out = t.run("wget %s %s" % (dest, url), timeout=timeout)
    return out


def alive(t):
    expect_in("alive", t.run("echo alive"), "the shell after the transfer")


@test("a download arrives whole when the link sends one byte at a time",
      needs=("wget", "rm", "cat"))
def tiny_segments(t):
    proxy, address = proxy_for(t, from_target=Direction(chunk=1))
    try:
        out = wget_through(t, proxy, address, "/sized.bin")
    finally:
        proxy.close()
    size = t.run("wc /nr_dl.bin")
    t.run("rm /nr_dl.bin")
    if "4000" not in out or "saved" not in out:
        raise AssertionError("a 4000 byte body over 1-byte segments did not finish:\n%s" % out)
    if "4000" not in size:
        raise AssertionError("the saved file is not 4000 bytes: %s" % size)
    alive(t)


@test("a download fails cleanly when the server resets mid-body",
      needs=("wget", "rm"))
def reset_mid_body(t):
    proxy, address = proxy_for(t, from_target=Direction(reset_after=1000))
    try:
        out = wget_through(t, proxy, address, "/sized.bin", timeout=60.0)
    finally:
        proxy.close()
    t.run("rm /nr_dl.bin")
    # it must come back to a prompt, not hang, whatever it says about the body
    alive(t)


@test("a download survives a server that stalls partway", needs=("wget", "rm"))
def stall_partway(t):
    proxy, address = proxy_for(t, from_target=Direction(stall=3.0, stall_after=1200))
    try:
        out = wget_through(t, proxy, address, "/sized.bin", timeout=90.0)
    finally:
        proxy.close()
    t.run("rm /nr_dl.bin")
    alive(t)


@test("a connect that never completes times out rather than hanging",
      needs=("wget", "rm"))
def dead_peer(t):
    # a listener that accepts and then says nothing: the connect succeeds but no
    # response ever comes, which is the timeout the client must enforce
    serv, address = origin(t)
    dead = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    dead.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    dead.bind(("0.0.0.0", 0))
    dead.listen(4)
    port = dead.getsockname()[1]
    try:
        url = "http://%s:%d/sized.bin" % (address, port)
        t.run("rm /nr_dl.bin")
        t.run("wget /nr_dl.bin %s" % url, timeout=90.0)
    finally:
        dead.close()
    t.run("rm /nr_dl.bin")
    alive(t)


@test("the server keeps serving a second client while one reads slowly",
      needs=("wget",))
def server_side_slow_reader(t):
    """
    A client that reads a page one small piece at a time, with a tiny receive
    buffer so its window keeps closing, holds the framework's own http server's
    send path open. A second client must still be answered.
    """
    host, port, secure = (None, None, False)
    from ..driver.portal import Portal
    found = Portal.reachable(t.address(), timeout=min(t.timeout, 15.0))
    if not found:
        raise Skip("the target serves no http portal")
    if found.secure:
        raise Skip("the slow-reader proxy is plain http only")

    proxy = ImpairProxy(found.host, found.port,
                        from_target=Direction(chunk=64, delay=0.05, rcvbuf=512),
                        listen_host="127.0.0.1")
    slow = None
    try:
        slow = socket.create_connection((proxy.host, proxy.port), timeout=t.timeout)
        slow.sendall(b"GET / HTTP/1.1\r\nHost: x\r\nConnection: close\r\n\r\n")
        slow.recv(64)  # take only a little, then leave the window shut

        fast = Portal(found.host, found.port, timeout=min(t.timeout, 15.0), secure=False)
        answer = fast.get("/", attempts=1)
        if answer.status >= 500 or not answer.raw:
            raise AssertionError("the second client got %d while one read slowly" % answer.status)
    finally:
        if slow is not None:
            try:
                slow.close()
            except OSError:
                pass
        proxy.close()
    alive(t)
