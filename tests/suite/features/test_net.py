#!/usr/bin/env python3

"""
Name resolution, reachability and the clock.

Mirrors tests/host/system/test_commands_net.cpp, minus the parts that depend on
a scripted radio. A real board's network is whatever the operator's is, so the
tests that need one look first and skip when there is nothing to reach.
"""

import re

from .registry import test, expect_in, expect_not_in, expect_any, Skip

IPV4 = re.compile(r"\b(\d{1,3}(?:\.\d{1,3}){3})\b")


# TEST-NET-1 from RFC 5737: reserved for documentation and routed nowhere
UNREACHABLE = "192.0.2.1"

_answers_everything = {}


def an_unreachable_address(t):
    """
    An address nothing will answer, or a skip. A simulated target may answer
    every address there is, and on one of those a loss assertion would only be
    testing the simulation.
    """
    cached = _answers_everything.get(id(t))
    if cached is None:
        probe = t.run("ping %s 1" % UNREACHABLE, timeout=40)
        cached = "0 received" not in probe
        _answers_everything[id(t)] = cached

    if cached:
        raise Skip("this target answers every address, including %s" % UNREACHABLE)

    return UNREACHABLE


def own_address(t):
    """The board's own address, or a skip when it is not on a network."""
    out = t.run("net ip")
    for found in IPV4.findall(out):
        if found not in ("0.0.0.0", "255.255.255.255"):
            return found

    raise Skip("the target has no address")


_replies = {}


def a_replying_host(t):
    """
    A host that answers this target's pings, or a skip.

    Deliberately not the target's own address: lwIP does not route a ping to
    the station address it is already holding, so those packets always time
    out and an assertion about replies would never be testing replies. The
    gateway is the one host a target with a network is known to reach, and it
    is asked once whether it really answers rather than assumed to.
    """
    cached = _replies.get(id(t))
    if cached is not None:
        if not cached:
            raise Skip("no host answers this target's pings")
        return cached

    gateway = None
    for line in t.run("net ip").splitlines():
        if "gateway" in line.lower():
            found = IPV4.findall(line)
            if found and found[0] not in ("0.0.0.0", "255.255.255.255"):
                gateway = found[0]
                break

    if gateway is not None and "0 received" in t.run("ping %s 1" % gateway, timeout=40):
        gateway = None

    _replies[id(t)] = gateway or False
    if gateway is None:
        raise Skip("no host answers this target's pings")

    return gateway


@test("host resolves an address literal to itself", needs=("host",))
def host_literal(t):
    expect_in("192.168.4.7", t.run("host 192.168.4.7"), "host of a literal")


@test("host reports a name it cannot resolve", needs=("host",))
def host_unresolvable(t):
    out = t.run("host no.such.name.invalid", timeout=30)
    expect_not_in("192.168", out, "no address for a name that has none")


@test("host without a name asks for one", needs=("host",))
def host_no_argument(t):
    expect_any(("host", "CmdErr", "usage"), t.run("host"), "host with no argument")


@test("the hosts file is readable", needs=("cat",))
def hosts_file(t):
    out = t.run("cat /etc/hosts")
    if "no such" in out.lower() or "CmdErr" in out:
        raise Skip("no /etc/hosts on this target")

    expect_in("localhost", out, "/etc/hosts")


@test("a name in the hosts file resolves to its address", needs=("host", "cat"))
def hosts_file_resolves(t):
    body = t.run("cat /etc/hosts")
    entry = None
    for line in body.splitlines():
        parts = line.split()
        if len(parts) >= 2 and IPV4.match(parts[0]):
            entry = (parts[0], parts[1])
            break

    if entry is None:
        raise Skip("no usable entry in /etc/hosts")

    address, name = entry
    expect_in(address, t.run("host %s" % name), "host of a name from the hosts file")


@test("ping reports every reply", needs=("ping",), slow=True)
def ping_replies(t):
    address = a_replying_host(t)

    out = t.run("ping %s 2" % address, timeout=40)
    expect_in("PING", out, "ping banner")
    expect_in("2 transmitted", out, "packets sent")
    expect_in("2 received", out, "replies counted")


@test("ping reports a host that never answers", needs=("ping",), slow=True)
def ping_unanswered(t):
    address = an_unreachable_address(t)

    out = t.run("ping %s 2" % address, timeout=40)
    expect_in("2 transmitted", out, "packets sent")
    expect_in("0 received", out, "nothing came back")
    expect_in("100% loss", out, "loss line")


@test("ping summarises the round trip times", needs=("ping",), slow=True)
def ping_summary(t):
    address = a_replying_host(t)

    out = t.run("ping %s 2" % address, timeout=40)
    expect_in("rtt min/avg/max", out, "ping summary")


@test("ping defaults to four packets", needs=("ping",), slow=True)
def ping_default_count(t):
    out = t.run("ping %s" % UNREACHABLE, timeout=60)
    expect_in("4 transmitted", out, "the default count")


@test("ping caps the count", needs=("ping",), slow=True)
def ping_caps(t):
    out = t.run("ping %s 99" % UNREACHABLE, timeout=120)
    expect_in("10 transmitted", out, "the count is clamped")


@test("ping of a name it cannot resolve says so", needs=("ping",), slow=True)
def ping_unresolvable(t):
    expect_any(("cannot resolve", "CmdErr"), t.run("ping no.such.name.invalid", timeout=40),
               "ping of an unresolvable name")


@test("net ip reports the addresses", needs=("net",))
def net_ip(t):
    out = t.run("net ip")
    expect_any(("IP", "ip", "addr"), out, "net ip")


@test("net without a subcommand is not accepted", needs=("net",))
def net_bare(t):
    expect_any(("CmdErr", "usage", "net"), t.run("net"), "net with no subcommand")


def a_synced_clock(t):
    """The printed time, or a skip when the board has never been told one."""
    out = t.run("date")
    if "not synced" in out:
        raise Skip("the clock has never been set on this target")

    return out


@test("date prints a time", needs=("date",))
def date_prints(t):
    out = a_synced_clock(t)
    if not re.search(r"\d{4}", out):
        raise AssertionError("date printed nothing that looks like a time:\n%s" % out)


@test("date in utc answers as well as local", needs=("date",))
def date_utc(t):
    a_synced_clock(t)
    utc = t.run("date -u")

    # a board sitting at UTC prints the same thing both ways, so this only
    # asserts that the utc form answers with a time at all
    if not re.search(r"\d{4}", utc):
        raise AssertionError("date -u printed no time:\n%s" % utc)


@test("net scansta scans and reports the wifi networks in range", needs=("net",), slow=True)
def net_scansta(t):
    import re
    out = t.run("net scansta", timeout=30)
    expect_in("Found networks", out, "the scan reports its result")

    found = re.search(r"Found networks:\s*(\d+)", out)
    if found is None:
        raise AssertionError("scan did not report a count:\n%s" % out[:200])
    if int(found.group(1)) < 1:
        raise AssertionError("scan found no networks at all:\n%s" % out[:200])


@test("date prints a custom strftime format", needs=("date",))
def date_custom_format(t):
    import re
    out = t.run("date +%Y")
    if not re.search(r"20\d\d", out):
        raise AssertionError("date +%%Y did not print a year: %r" % out)


@test("tdctl reports the clock status", needs=("tdctl",))
def tdctl_reports(t):
    out = t.run("tdctl")
    expect_any(("time", "Time", "zone", "NTP", "ntp", "clock"), out, "tdctl")
