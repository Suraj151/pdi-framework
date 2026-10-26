#!/usr/bin/env python3

"""
Sustained load, to find what one pass does not: a slow leak, a pool that stops
granting, a heap that drifts down and never comes back.

These are off by default, because they run for minutes and a normal run should
not. Set PDI_SOAK to the number of seconds each soak should last to turn them
on, e.g. PDI_SOAK=600. They report the heap at the start and the end, and fail
only on a hard fault — a crash, a refused session that never recovers, a shell
that stops answering, or a heap that ends a clear step below where it began.
"""

import os
import time

from .registry import test, Skip, expect_in
from .registry import meminfo


def soak_seconds():
    raw = os.environ.get("PDI_SOAK", "")
    try:
        seconds = int(raw)
    except ValueError:
        seconds = 0
    if seconds <= 0:
        raise Skip("set PDI_SOAK=<seconds> to run the soak suite")
    return seconds


def heap_free(t):
    info = meminfo(t)
    return info.get("MemFree") if info else None


def steady(t, before, after, note):
    """
    Fail when the heap ended a clear step below where it began. The heap moves
    between discrete levels, so a small dip is noise; a drop of more than an
    eighth of the starting free heap is a leak.
    """
    if before is None or after is None:
        return
    if after < before - before // 8:
        raise AssertionError("%s: free heap fell from %d to %d and did not recover"
                             % (note, before, after))


@test("the shell answers steadily under a long run of commands", slow=True,
      needs=("echo", "ls", "cat", "rm", "pwd"))
def command_soak(t):
    seconds = soak_seconds()
    before = heap_free(t)
    deadline = time.time() + seconds
    rounds = 0
    ws = t.workspace("soak_cmd")
    while time.time() < deadline:
        t.run("echo round%d > %s/f" % (rounds, ws))
        expect_in("round%d" % rounds, t.run("cat %s/f" % ws), "the file this round wrote")
        t.run("ls %s" % ws)
        t.run("pwd")
        rounds += 1
    after = heap_free(t)
    print("       %sran %d command rounds over %ds%s" % ("", rounds, seconds, ""), flush=True)
    steady(t, before, after, "command soak")


@test("sessions open and close for a long time without exhausting the pool", slow=True)
def session_churn_soak(t):
    seconds = soak_seconds()
    try:
        first = t.peer()
    except Skip:
        raise Skip("this transport cannot open a second session")
    first.close()

    before = heap_free(t)
    deadline = time.time() + seconds
    opened = 0
    refused_streak = 0
    while time.time() < deadline:
        try:
            peer = t.peer()
        except Skip:
            refused_streak += 1
            if refused_streak > 5:
                raise AssertionError("the session pool stopped granting after %d opens" % opened)
            time.sleep(3.0)
            continue
        refused_streak = 0
        opened += 1
        try:
            expect_in("churn", peer.run("echo churn", 15.0), "a churned session's own output")
        finally:
            peer.close()
    after = heap_free(t)
    print("       %sopened and closed %d sessions over %ds%s" % ("", opened, seconds, ""), flush=True)
    expect_in("alive", t.run("echo alive"), "the first session after the churn")
    steady(t, before, after, "session churn")


@test("the portal serves a long run of requests with a flat heap", slow=True)
def portal_soak(t):
    seconds = soak_seconds()
    portal = t.portal(login=True)
    before = heap_free(t)
    deadline = time.time() + seconds
    served = 0
    while time.time() < deadline:
        answer = portal.get("/", attempts=2)
        if answer.status >= 500:
            raise AssertionError("the portal answered %d after %d requests" % (answer.status, served))
        served += 1
    after = heap_free(t)
    print("       %sserved %d portal requests over %ds%s" % ("", served, seconds, ""), flush=True)
    steady(t, before, after, "portal soak")
