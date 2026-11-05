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

from .registry import test, Skip, expect_in, expect_not_in, ssh_port
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


@test("an sftp read the heap cannot hold whole is answered short, with the right bytes",
      needs=("rm",))
def sftp_read_answered_short(t):
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
            try:
                got = single(4096)
            finally:
                lift(t)

            if not got or len(got) >= 4096:
                raise AssertionError("a read larger than the heap could hold came back as %d bytes, "
                                     "not a short read" % len(got or b""))
            if got != body[:len(got)]:
                raise AssertionError("the short read carried the wrong %d bytes" % len(got))

            fh.seek(len(got))
            rest = single(2048)
            if rest != body[len(got):len(got) + len(rest)] or 0 == len(rest):
                raise AssertionError("the read after the short one came back wrong")
    finally:
        lift(t)
        sftp.close()
        client.close()
        t.run("rm %s" % name)


@test("an sftp read the heap cannot hold even short is refused and the session reads on afterwards",
      needs=("rm",))
def sftp_read_refused_then_served(t):
    squeezer(t)
    name = "/mem_sftp_tight.bin"
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

            squeeze(t, block=600)
            refused = False
            try:
                single(4096)
            except IOError:
                refused = True
            finally:
                lift(t)

            if not refused:
                raise AssertionError("a read with no room even for a short reply was answered "
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


@test("a portal page the heap cannot build is refused at once, and served once memory is back")
def portal_page_refused_then_served(t):
    squeezer(t)
    portal = t.portal(login=False)
    portal.timeout = 6.0
    squeeze(t, block=1024)
    try:
        began = time.time()
        try:
            status = portal.get("/", attempts=1).status
        except Exception as err:
            raise AssertionError("a squeezed portal gave no answer in %.1f s (%s)"
                                 % (time.time() - began, type(err).__name__))
        if 503 != status:
            raise AssertionError("a page the heap cannot build answered %d, not 503" % status)
    finally:
        lift(t)

    status = t.portal(login=False).get("/").status
    if 200 != status:
        raise AssertionError("the portal answered %d once the heap was back" % status)


@test("an sftp upload the heap cannot hold one packet of is refused, and arrives whole once memory is back",
      needs=("rm", "cat"))
def sftp_upload_in_smaller_pieces(t):
    squeezer(t)
    name = "/mem_sftp_upload.bin"
    body = patterned(16384)
    t.run("rm %s" % name)
    client, sftp = sftp_open(t)
    refused = False
    try:
        squeeze(t, block=700)
        try:
            with sftp.open(name, "wb") as fh:
                fh.write(body)
        except Exception:
            refused = True
        finally:
            lift(t)
    finally:
        try:
            sftp.close()
        except Exception:
            pass
        client.close()
    if not refused:
        raise AssertionError("an upload went on with no packet the heap could hold, so it cannot have been checked")
    expect_in("SSH: session closed, a packet could not be read", t.run("cat /var/log/syslog.error"),
              "the refused upload names why in the system log")

    client, sftp = sftp_open(t)
    try:
        with sftp.open(name, "wb") as fh:
            fh.write(body)
        with sftp.open(name, "rb") as fh:
            got = fh.read()
        if got != body:
            raise AssertionError("once memory was back %d bytes sent, %d stored" % (len(body), len(got)))
    finally:
        sftp.close()
        client.close()
        t.run("rm %s" % name)


@test("an append to /tmp the heap cannot grow the file for is reported, not dropped",
      needs=("echo", "rm", "wc"))
def tmp_append_refused_by_heap(t):
    """
    /tmp keeps a file in memory, so an append needs the heap as well as the
    disk's own room. Appends go on under a squeezed heap until the file has to
    move to a bigger block; each one must either grow the file or fail where
    it can be seen, never report success and keep nothing.
    """
    squeezer(t)
    name = "/tmp/mem_append.bin"
    line = "y" * 190

    def counts():
        got = t.run("wc %s" % name).split()
        if len(got) < 3 or not all(c.isdigit() for c in got[:3]):
            raise AssertionError("wc gave no counts, so growth cannot be judged:\n%s" % " ".join(got))
        return got[:3]

    t.run("rm %s" % name)
    client, sftp = sftp_open(t)
    try:
        with sftp.open(name, "wb") as fh:
            fh.write(patterned(1500))
        for _ in range(12):
            before = counts()
            squeeze(t, block=1024)
            try:
                out = t.run("echo %s >> %s; echo rc=$?" % (line, name))
            finally:
                lift(t)
            status = [l for l in out.splitlines() if l.strip().startswith("rc=")]
            reported = ("cannot write" in out) or (status and status[-1].strip() != "rc=0")
            if reported:
                break
            if before == counts():
                raise AssertionError("an append the heap could not take stored nothing and reported success:\n%s"
                                     % out)
    finally:
        sftp.close()
        client.close()
        t.run("rm %s" % name)



@test("a typed line the heap cannot grow keeps what it holds and the shell answers on", needs=("echo",))
def typed_line_refused_growth(t):
    squeezer(t)
    t.run("echo warm")
    squeeze(t, block=400)
    try:
        try:
            t.run("echo " + "Q" * 1500, timeout=30)
        except Exception:
            pass
    finally:
        lift(t)

    expect_in("after", t.run("echo after", timeout=30), "the shell once the heap is back")


@test("grep over a line the heap cannot hold whole says so rather than answering no match",
      needs=("grep", "rm"))
def grep_line_refused_growth(t):
    squeezer(t)
    name = "/mem_grep_long.txt"
    body = b"short line\n" + b"x" * 5000 + b"MARK" + b"y" * 1000 + b"\nlast line\n"
    t.run("rm %s" % name)
    client, sftp = sftp_open(t)
    try:
        with sftp.open(name, "wb") as fh:
            fh.write(body)
    finally:
        sftp.close()
        client.close()

    squeeze(t, block=2048)
    try:
        infile = t.run("grep MARK %s" % name, timeout=60)
        redirected = t.run("grep MARK < %s" % name, timeout=60)
    finally:
        lift(t)

    try:
        expect_in("%s:2: too long, matched on its first" % name, infile,
                  "grep in a file answered no match on a line it could not hold")
        expect_in("2: too long, matched on its first", redirected,
                  "grep on its input answered no match on a line it could not hold")
        expect_in("last", t.run("grep last %s" % name, timeout=30), "grep once the heap is back")
    finally:
        t.run("rm %s" % name)


@test("userdel the heap squeezes part way never drops another account",
      needs=("useradd", "userdel", "cat"))
def userdel_refused_growth_keeps_others(t):
    squeezer(t)
    keep = ["mkeep1", "mkeep2"]
    gone = "mgone"
    for name in keep + [gone]:
        t.run("userdel u=%s" % name)
        t.run("useradd u=%s p=Strong#Pass123" % name)
    try:
        for block in (96, 64, 48, 40, 32, 24, 16):
            squeeze(t, block=block)
            try:
                t.run("userdel u=%s" % gone, timeout=30)
            except Exception:
                pass
            finally:
                lift(t)
            passwd = t.run("cat /etc/passwd", timeout=30)
            shadow = t.run("cat /etc/shadow", timeout=30)
            for name in keep + ["pdiStack"]:
                expect_in(name + ":", passwd, "userdel under a %d B block dropped %s from /etc/passwd" % (block, name))
                expect_in(name + ":", shadow, "userdel under a %d B block dropped %s from /etc/shadow" % (block, name))
            if (gone + ":") not in passwd:
                t.run("useradd u=%s p=Strong#Pass123" % gone)
    finally:
        lift(t)
        for name in keep + [gone]:
            t.run("userdel u=%s" % name)


@test("a typed line the heap cut short is not run as a shorter command", needs=("echo", "cat", "rm"))
def typed_line_cut_not_run(t):
    squeezer(t)
    name = "/mem_cut_line.txt"
    t.run("rm %s" % name)
    t.run("echo warm")
    squeeze(t, block=400)
    out = ""
    try:
        try:
            out = t.run("echo START" + "Q" * 1500 + "END > %s" % name, timeout=30)
        except Exception as err:
            out = str(err)
    finally:
        lift(t)
    stored = t.run("cat %s" % name, timeout=30)
    try:
        if "END" not in stored:
            if out.count("START") > 1:
                raise AssertionError("a line cut short by the heap ran as a shorter command:\n%s" % out)
            expect_in("line too long, not run", out, "a line cut short by the heap was dropped without a word")
    finally:
        t.run("rm %s" % name)


@test("head and tail print a line longer than the heap can hold whole", needs=("head", "tail", "rm"))
def head_tail_long_line(t):
    squeezer(t)
    name = "/mem_head_long.txt"
    body = b"short line\n" + b"h" * 6000 + b"TAILMARK\n"
    t.run("rm %s" % name)
    client, sftp = sftp_open(t)
    try:
        with sftp.open(name, "wb") as fh:
            fh.write(body)
    finally:
        sftp.close()
        client.close()

    squeeze(t, block=2048)
    try:
        head = t.run("head %s 2" % name, timeout=60)
        tail = t.run("tail %s 1" % name, timeout=60)
    finally:
        lift(t)

    try:
        expect_in("TAILMARK", head, "head printed a long line cut short")
        expect_in("TAILMARK", tail, "tail printed a long line cut short")
    finally:
        t.run("rm %s" % name)
