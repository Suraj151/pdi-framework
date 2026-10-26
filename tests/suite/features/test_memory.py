#!/usr/bin/env python3

"""
What the framework does when the heap refuses it.

A board's heap is shared by every service and fragments over hours, so a
request the device could normally serve is refused at the moment another
service holds the room. The rule tested here is the one every such path must
keep: refuse that one operation cleanly, keep the protocol in step, and go on
serving once memory is back. A refusal must never write past a buffer, send a
half built packet, or take a service down.

Only the host process can have its heap squeezed on cue, through the limits
pdid reads from its heap input file: `block` caps the largest free block, the
way fragmentation does, `free` caps the total. Every test lifts the limits
again before it ends, whatever happened.
"""

import time

from .registry import test, Skip, expect_in, ssh_port
from .test_sftp import sftp_open, patterned

SETTLE = 0.3


def squeezer(t):
    """The way to squeeze the heap, or a skip on a target with a real one."""
    squeeze = getattr(t.shell, "set_heap", None)
    if squeeze is None:
        raise Skip("only the host process can have its heap squeezed on cue")
    return squeeze


def squeeze(t, free=None, block=None):
    squeezer(t)(free=free, block=block)
    time.sleep(SETTLE)


def lift(t):
    squeezer(t)()
    time.sleep(SETTLE)


@test("an sftp read the heap cannot hold is refused and the session reads on afterwards",
      needs=("rm",))
def sftp_read_refused_then_served(t):
    squeezer(t)
    name = "/mem_sftp_read.bin"
    body = patterned(8192)
    t.run("rm %s" % name)
    client, sftp = sftp_open(t)
    try:
        with sftp.open(name, "wb") as fh:
            fh.write(body)

        with sftp.open(name, "rb") as fh:
            single = getattr(fh, "_read", None)
            if single is None:
                raise Skip("this paramiko has no single-request read")

            squeeze(t, block=1024)
            refused = False
            try:
                single(4096)
            except IOError:
                refused = True
            finally:
                lift(t)

            if not refused:
                raise AssertionError("a read reply larger than the largest free block was answered "
                                     "with data instead of an error")

            fh.seek(0)
            got = single(2048)
            if got != body[:len(got)] or 0 == len(got):
                raise AssertionError("after the refusal the same session read %d wrong bytes"
                                     % len(got))
    finally:
        lift(t)
        sftp.close()
        client.close()
        t.run("rm %s" % name)


@test("an ssh packet the heap cannot hold ends only its own session")
def ssh_packet_refused_ends_one_session(t):
    squeezer(t)
    try:
        import paramiko
    except ImportError as err:
        raise Skip("paramiko is not installed: %s" % err)

    from ..driver import ssh_shell  # noqa: F401

    client = paramiko.SSHClient()
    client.set_missing_host_key_policy(paramiko.AutoAddPolicy())
    client.connect(t.address(), port=ssh_port(t), username=t.username, password=t.password,
                   timeout=30, banner_timeout=30, auth_timeout=30, allow_agent=False,
                   look_for_keys=False)
    try:
        channel = client.invoke_shell()
        time.sleep(1.0)
        squeeze(t, block=1500)
        try:
            channel.send("echo " + "x" * 3000 + "\r")
            time.sleep(2.0)
        finally:
            lift(t)
    finally:
        client.close()

    expect_in("still here", t.run("echo still here"),
              "the shell that was not sending the large packet")

    client2, sftp = sftp_open(t)
    try:
        if not sftp.normalize(".").startswith("/"):
            raise AssertionError("a fresh ssh session after the refusal did not work")
    finally:
        sftp.close()
        client2.close()


@test("a squeezed heap leaves every transport answering once it is lifted",
      needs=("echo",))
def every_transport_survives_a_squeeze(t):
    squeezer(t)
    t.portal(login=False)
    peer = None
    try:
        peer = t.peer()
    except Skip:
        peer = None

    squeeze(t, block=256)
    try:
        for _ in range(2):
            try:
                t.run("echo squeezed")
            except Exception:
                pass
            if peer is not None:
                try:
                    peer.run("echo squeezed", 10.0)
                except Exception:
                    pass
            try:
                portal = t.portal(login=False)
                portal.get("/", attempts=1)
            except Exception:
                pass
    finally:
        lift(t)

    expect_in("after", t.run("echo after"), "the main session once the heap is back")
    if peer is not None:
        try:
            expect_in("peer after", peer.run("echo peer after", 20.0),
                      "the second session once the heap is back")
        finally:
            peer.close()

    portal = t.portal(login=True)
    status = portal.get("/").status
    if status >= 500:
        raise AssertionError("the portal answered %d once the heap was back" % status)

    client, sftp = sftp_open(t)
    try:
        sftp.normalize(".")
    finally:
        sftp.close()
        client.close()
