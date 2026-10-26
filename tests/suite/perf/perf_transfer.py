#!/usr/bin/env python3

"""
How fast files move over the network: sftp both ways.

The file size follows the room the root filesystem has, so a board with a small
filesystem still measures instead of skipping, and the result is a rate, so a
smaller file is still comparable with a larger one.
"""

import time

from .registry import perf, Skip
from ..features.test_sftp import sftp_open, patterned

TRANSFER_MAX = 65536
TRANSFER_MIN = 8192
BLOCK = 4096


def root_free(t):
    """The bytes free on the root filesystem, or None when df does not say."""
    for line in t.run("df").splitlines():
        parts = line.split()
        if len(parts) >= 5 and parts[0] == "/" and parts[4].isdigit():
            return int(parts[4])
    return None


def transfer_size(t):
    """
    The largest file up to TRANSFER_MAX the root can take with two blocks to
    spare, in whole blocks; skips when even TRANSFER_MIN does not fit.
    """
    free = root_free(t)
    if free is None:
        return TRANSFER_MAX
    size = min(TRANSFER_MAX, ((free // BLOCK) - 2) * BLOCK)
    if size < TRANSFER_MIN:
        raise Skip("the root filesystem has %d bytes free, a transfer needs %d" % (free, TRANSFER_MIN))
    return size


@perf("sftp download", unit="KB/s", needs=("rm",))
def sftp_download(t):
    size = transfer_size(t)
    body = patterned(size)
    path = "/perf_down.bin"
    client, sftp = sftp_open(t)
    try:
        with sftp.open(path, "wb") as fh:
            fh.write(body)

        with sftp.open(path, "rb") as fh:
            began = time.time()
            fh.prefetch(size)
            got = fh.read(size)
            took = time.time() - began
    finally:
        sftp.close()
        client.close()
        t.run("rm %s" % path)

    if got != body:
        raise AssertionError("a %d byte download came back as %d bytes" % (size, len(got)))
    return (size / 1024.0) / took


@perf("sftp upload", unit="KB/s", needs=("rm",))
def sftp_upload(t):
    size = transfer_size(t)
    body = patterned(size)
    path = "/perf_up.bin"
    client, sftp = sftp_open(t)
    try:
        fh = sftp.open(path, "wb")
        fh.set_pipelined(True)
        began = time.time()
        fh.write(body)
        fh.close()
        took = time.time() - began

        stored = sftp.stat(path).st_size
    finally:
        sftp.close()
        client.close()
        t.run("rm %s" % path)

    if stored != size:
        raise AssertionError("a %d byte upload stored %d bytes" % (size, stored))
    return (size / 1024.0) / took
