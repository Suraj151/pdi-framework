#!/usr/bin/env python3

"""
How fast a session's terminal answers and how fast it can stream output.

The stream is a script loop that the script runner's own cap stops after a fixed
number of passes, so every run prints the same lines whatever the board, and the
rate shows the transport's write path: a change that turns each small write into
its own segment shows here as a fall to a third.
"""

import time

from .registry import perf
from ..features.test_script import script

LOOP_PASSES = 1000
LATENCY_ROUNDS = 10


def stream_rate(t):
    path = script(t, "perfspin", ["while test a = a", "echo spin", "done"])
    try:
        began = time.time()
        out = t.run("source %s" % path, timeout=max(t.timeout, 180.0))
        took = time.time() - began
    finally:
        t.run("rm %s" % path)

    seen = out.count("spin")
    if seen < LOOP_PASSES // 2:
        raise AssertionError("the loop printed %d lines, expected about %d" % (seen, LOOP_PASSES))
    return seen / took


def round_trip(t):
    began = time.time()
    for _ in range(LATENCY_ROUNDS):
        t.run("pwd")
    return (time.time() - began) * 1000.0 / LATENCY_ROUNDS


@perf("telnet output", unit="lines/s", lane="telnet", repeat=3, needs=("source", "test", "echo", "rm"))
def telnet_output(t):
    return stream_rate(t)


@perf("ssh output", unit="lines/s", lane="ssh", repeat=3, needs=("source", "test", "echo", "rm"))
def ssh_output(t):
    return stream_rate(t)


@perf("telnet command round trip", unit="ms", better="lower", lane="telnet", needs=("pwd",))
def telnet_round_trip(t):
    return round_trip(t)


@perf("ssh command round trip", unit="ms", better="lower", lane="ssh", needs=("pwd",))
def ssh_round_trip(t):
    return round_trip(t)
