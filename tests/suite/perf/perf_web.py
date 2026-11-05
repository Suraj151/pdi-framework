#!/usr/bin/env python3

"""
How fast the web side moves data: a portal page, a portal upload, and a file the
board fetches itself with wget. These are the paths that size their buffers from
port macros, so they are the ones a change in buffer sizing has to be judged on.
"""

import time

from .registry import perf, Skip
from .perf_transfer import transfer_size
from ..features.test_portal import upload
from ..features.test_wget import fetch
from ..features.test_sftp import patterned

UPLOAD_MAX = 32768


@perf("portal page", unit="ms", better="lower")
def portal_page(t):
    portal = t.portal()
    began = time.time()
    answer = portal.get("/")
    took = (time.time() - began) * 1000.0
    if 200 != answer.status:
        raise AssertionError("the portal home page answered %d" % answer.status)
    return took


@perf("portal upload", unit="KB/s", needs=("rm",))
def portal_upload(t):
    size = min(UPLOAD_MAX, transfer_size(t))
    name = "perf_portal.bin"
    portal = t.portal()
    t.run("rm /%s" % name)
    try:
        began = time.time()
        answer = upload(t, portal, name, patterned(size))
        took = time.time() - began
        if answer.status not in (200, 301, 302, 303):
            raise AssertionError("the upload answered %d" % answer.status)
        stored = t.run("ls /%s" % name)
        if str(size) not in stored:
            raise AssertionError("a %d byte upload is not on the target: %s" % (size, stored.strip()))
    finally:
        t.run("rm /%s" % name)
    return (size / 1024.0) / took


@perf("wget download", unit="KB/s", needs=("wget", "rm", "ls"))
def wget_download(t):
    size = transfer_size(t)
    began = time.time()
    dest, out = fetch(t, "perf_wget.bin", "/perf-%d.bin" % size, timeout=180.0)
    took = time.time() - began
    try:
        stored = t.run("ls %s" % dest)
        if str(size) not in stored:
            raise AssertionError("wget stored %s, wanted %d bytes" % (stored.strip(), size))
    finally:
        t.run("rm %s" % dest)
    return (size / 1024.0) / took
