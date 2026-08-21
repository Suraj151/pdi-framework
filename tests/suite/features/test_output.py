#!/usr/bin/env python3

"""
Command output larger than a single tcp segment, read back whole.

This is the regression suite for the truncation class: a shell whose transport
is tcp (telnet, ssh, or a tls channel) used to drop everything past the send
buffer, so a long listing came back with its tail missing and nothing said so.
It surfaced first as the capability probe believing a board had a third of its
commands, because `help` is the largest thing the shell prints and it was the
first output to overflow.

A board reached over a serial cable cannot truncate this way, but the tests are
transport blind on purpose: the same assertion that catches the tcp regression
confirms the serial path agrees.
"""

from .registry import test, expect_in


SPAN = ("login", "logout", "whoami", "help", "ls", "cat", "reboot", "who")


@test("the whole command list survives the transport", needs=("help",))
def help_is_not_truncated(t):
    listing = t.run("help")

    for name in SPAN:
        if name not in t.caps.commands:
            continue
        expect_in(name, listing, "help lists %s" % name)


@test("a listing longer than a send buffer is returned whole",
      needs=("touch", "ls", "rm"), mounts=("/",), slow=True)
def long_listing_is_whole(t):
    t.workspace("wt_bigls")

    count = 48
    names = ["outfile_%03d.txt" % i for i in range(count)]
    for name in names:
        t.run("touch %s" % name)

    listing = t.run("ls")
    missing = [name for name in names if name not in listing]
    if missing:
        raise AssertionError(
            "ls dropped %d of %d entries past the send buffer; first gone was %s"
            % (len(missing), count, missing[0]))
