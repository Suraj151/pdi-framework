#!/usr/bin/env python3

"""
Several kinds of work on one target at once: a large sftp transfer runs in its own
ssh session while the shell under test, a second terminal and the web portal keep
asking for things. Each piece must finish whole and every answer must be right, so
one task is shown not to starve, cut or reset another. The transfer is as large as
the target can hold, up to 10 MB, on a mounted card when there is one.
"""

import hashlib
import random
import threading
import time

from .registry import test, expect_in, Skip
from .test_sftp import sftp_open

WANT = 10 * 1024 * 1024
FLOOR = 256 * 1024
MARGIN = 256 * 1024


def free_bytes(t):
    """Free bytes of each mount df lists, by mount point."""
    free = {}
    for line in t.run("df").splitlines():
        cols = line.split()
        if len(cols) >= 5 and cols[0].startswith("/") and cols[4].isdigit():
            free[cols[0]] = int(cols[4])
    return free


def pick_place(t, name):
    """A path and a size for the transfer: /mnt when a card is mounted there, else the root."""
    free = free_bytes(t)
    for mount in ("/mnt", "/"):
        room = min(free.get(mount, 0) // 2, free.get(mount, 0) - MARGIN)
        if room >= FLOOR:
            size = min(WANT, room)
            path = (mount.rstrip("/") or "") + "/" + name
            return path, size
    raise Skip("no filesystem has room for a %d byte transfer in half of its free space" % FLOOR)


def uptime(t):
    out = t.run("cat /proc/uptime").split()
    for word in out:
        try:
            return float(word)
        except ValueError:
            continue
    raise AssertionError("/proc/uptime gave no number: %r" % out)


class Transfer(threading.Thread):
    """An upload with its writes pipelined, then a download with every read in flight."""

    def __init__(self, t, path, body, download=True, upload=True):
        super().__init__(daemon=True)
        self.t, self.path, self.body = t, path, body
        self.download, self.upload = download, upload
        self.error = None
        self.got = None
        self.seconds = 0.0
        self.client, self.sftp = sftp_open(t)

    def run(self):
        began = time.time()
        client, sftp = self.client, self.sftp
        try:
            if self.upload:
                self.put(sftp)
            if self.download:
                with sftp.open(self.path, "rb") as fh:
                    pieces = fh.readv([(offset, 32768) for offset in range(0, len(self.body), 32768)])
                    self.got = b"".join(pieces)
        except Exception as err:
            self.error = "%s: %r" % (type(err).__name__, err)
        finally:
            self.seconds = time.time() - began
            for closer in (sftp, client):
                try:
                    if closer is not None:
                        closer.close()
                except Exception:
                    pass

    def put(self, sftp):
        with sftp.open(self.path, "wb") as fh:
            fh.set_pipelined(True)
            for offset in range(0, len(self.body), 32768):
                fh.write(self.body[offset:offset + 32768])


def load_while(t, busy, label):
    """
    Keeps the shell, a second terminal when there is one, and the portal asking for
    things until busy() is false; every answer must arrive and be right.
    """
    try:
        portal = t.portal()
    except Skip:
        portal = None
    try:
        peer = t.peer()
    except Skip:
        peer = None

    counts = {"shell": 0, "peer": 0, "portal": 0}
    slowest = {"shell": 0.0, "peer": 0.0, "portal": 0.0}
    failures = []

    def timed(kind, ask):
        began = time.time()
        try:
            return ask()
        finally:
            slowest[kind] = max(slowest[kind], time.time() - began)
    turn = 0
    try:
        while busy() or turn < 2:
            turn += 1
            marker = "stress_%s_%d" % (label, turn)
            checks = (
                ("shell", lambda: t.run("echo %s" % marker), marker),
                ("shell", lambda: t.run("ps"), "NAME"),
                ("shell", lambda: t.run("cat /proc/meminfo"), "MemFree"),
                ("shell", lambda: t.run("uptime"), "up"),
                ("shell", lambda: t.run("ls -la /"), "proc"),
            )
            for kind, ask, wanted in checks:
                try:
                    out = timed(kind, ask)
                    if wanted not in out:
                        failures.append("%s answered without %r: %r" % (kind, wanted, out[-200:]))
                    counts[kind] += 1
                except Exception as err:
                    failures.append("%s did not answer: %r" % (kind, err))
            if peer is not None:
                try:
                    out = timed("peer", lambda: peer.run("uptime", 30.0))
                    if "up" not in out:
                        failures.append("the second terminal answered uptime with %r" % out[-200:])
                    counts["peer"] += 1
                except Exception as err:
                    failures.append("the second terminal did not answer: %r" % err)
            if portal is not None:
                try:
                    status = timed("portal", lambda: portal.get("/dashboard")).status
                    if 200 != status:
                        failures.append("the portal answered %d" % status)
                    counts["portal"] += 1
                except Exception as err:
                    failures.append("the portal did not answer: %r" % err)
            if len(failures) > 5:
                break
    finally:
        if peer is not None:
            try:
                peer.close()
            except Exception:
                pass
        if portal is not None:
            try:
                portal.logout()
            except Exception:
                pass
    print("       slowest answer during the load: shell %.1f s, second terminal %.1f s, portal %.1f s (%s)"
          % (slowest["shell"], slowest["peer"], slowest["portal"], ", ".join("%s %d" % kv for kv in sorted(counts.items()))),
          flush=True)
    return counts, failures, portal is not None, peer is not None


def check_whole(transfer, body, what):
    if transfer.error:
        raise AssertionError("%s broke after %.0f s: %s" % (what, transfer.seconds, transfer.error))
    if transfer.got is not None and hashlib.md5(transfer.got).hexdigest() != hashlib.md5(body).hexdigest():
        raise AssertionError("%s read back %d bytes that differ from the %d written"
                             % (what, len(transfer.got), len(body)))


@test("a large sftp upload and download stay whole while the shell, a second terminal and the portal keep answering",
      needs=("rm", "df", "ps", "uptime", "ls", "cat", "echo"), slow=True)
def transfer_with_shell_and_portal(t):
    path, size = pick_place(t, "cc_big.bin")
    body = random.Random(size).randbytes(size)
    t.run("rm %s" % path)
    began = uptime(t)
    transfer = Transfer(t, path, body)
    transfer.start()
    try:
        counts, failures, had_portal, had_peer = load_while(t, transfer.is_alive, "one")
    finally:
        transfer.join(timeout=7200)
    try:
        check_whole(transfer, body, "a %d byte transfer" % size)
        if failures:
            raise AssertionError("while %d bytes moved over sftp: %s" % (size, "; ".join(failures[:6])))
        if counts["shell"] < 4:
            raise AssertionError("the shell was asked only %d times during the transfer" % counts["shell"])
        if had_portal and counts["portal"] < 1:
            raise AssertionError("the portal was never asked during the transfer")
        if uptime(t) <= began:
            raise AssertionError("the target restarted during the transfer")
    finally:
        t.run("rm %s" % path)


@test("an upload and a download in two sessions at once both stay whole while the shell answers",
      needs=("rm", "df", "ps", "uptime", "ls", "cat", "echo"), slow=True)
def two_transfers_with_shell(t):
    path, size = pick_place(t, "cc_two.bin")
    size = max(FLOOR, size // 3)
    first = random.Random(size).randbytes(size)
    second = random.Random(size + 1).randbytes(size)
    t.run("rm %s" % path)
    t.run("rm %s.2" % path)

    seeded = Transfer(t, path, first, download=False)
    seeded.run()
    check_whole(seeded, first, "the first upload")

    down = Transfer(t, path, first, upload=False)
    try:
        up = Transfer(t, path + ".2", second, download=False)
    except Skip as err:
        down.sftp.close()
        down.client.close()
        raise Skip("the target has no room for two transfer sessions beside this shell: %s" % err)

    began = uptime(t)
    down.start()
    up.start()
    try:
        counts, failures, _, _ = load_while(t, lambda: down.is_alive() or up.is_alive(), "two")
    finally:
        down.join(timeout=7200)
        up.join(timeout=7200)
    try:
        check_whole(down, first, "the download")
        check_whole(up, second, "the upload beside it")
        back = Transfer(t, path + ".2", second, upload=False)
        back.start()
        try:
            more, late, _, _ = load_while(t, back.is_alive, "back")
        finally:
            back.join(timeout=7200)
        print("       seconds taken: download %.0f, upload beside it %.0f, read back %.0f"
              % (down.seconds, up.seconds, back.seconds), flush=True)
        check_whole(back, second, "the upload made beside a download, read back")
        failures += late
        counts["shell"] += more["shell"]
        if failures:
            raise AssertionError("while two transfers ran: %s" % "; ".join(failures[:6]))
        if counts["shell"] < 4:
            raise AssertionError("the shell was asked only %d times during the transfers" % counts["shell"])
        if uptime(t) <= began:
            raise AssertionError("the target restarted during the transfers")
    finally:
        t.run("rm %s" % path)
        t.run("rm %s.2" % path)
