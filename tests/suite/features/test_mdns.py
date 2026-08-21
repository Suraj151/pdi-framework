#!/usr/bin/env python3

"""
mDNS: what the board calls itself on the local network, and whether it answers.

Two halves. The shell half asks the service what it thinks it is advertising
and holds it against `net ip`; that runs anywhere. The network half puts a real
query on the multicast group and requires the answer to come **from the board**,
which needs the test machine on the same segment as the board and an access
point that forwards multicast — consumer hotspots often do not, so it skips
with that reason rather than failing.

Answers from anyone else are ignored on purpose. A developer machine usually
runs its own responder and will answer for a name it has cached, so a test that
accepts the first reply on the group ends up testing the test machine.
"""

import re

from .registry import test, expect_in, Skip
from ..driver import mdns

HOSTNAME_ROW = re.compile(r"hostname\s*:\s*(\S+)")
ADDRESS_ROW = re.compile(r"address\s*:\s*(\S+)")
SERVICES_ROW = re.compile(r"services\s*:\s*(.+)")


def mdns_status(t):
    """What `srvc status MDNS` reports, or a skip when the service is absent."""
    out = t.run("srvc status MDNS", timeout=max(t.timeout, 30))
    if "no such service" in out.lower() or "CmdErr" in out:
        raise Skip("no mdns service on this target")

    host = HOSTNAME_ROW.search(out)
    if not host:
        raise Skip("the mdns service reported no hostname:\n%s" % out)

    address = ADDRESS_ROW.search(out)
    services = SERVICES_ROW.search(out)

    return (host.group(1),
            address.group(1) if address else "",
            services.group(1).split() if services else [])


def answered(t, name, qtype, what):
    """Records the board itself sent, or a skip naming why none arrived."""
    address = t.address()

    try:
        records = mdns.ask(address, name, qtype)
    except mdns.MdnsError as err:
        raise Skip("cannot run an mdns query from here: %s" % err)

    if not records:
        raise Skip("no mdns answer for %s reached us from %s — the network "
                   "between here and the board has to carry multicast, and a "
                   "phone hotspot usually will not" % (what, address))

    return records


@test("the mdns service advertises a .local hostname", needs=("srvc",))
def hostname_is_local(t):
    host, _, _ = mdns_status(t)

    if not host.endswith(".local"):
        raise AssertionError("the advertised hostname is %r, which is not .local"
                             % host)


@test("the advertised address is the address the board is using",
      needs=("srvc", "net"))
def advertised_address_matches(t):
    from .test_net import own_address

    _, advertised, _ = mdns_status(t)
    if not advertised:
        raise Skip("the mdns service reported no address")

    expect_in(own_address(t), advertised,
              "the address mdns advertises against the station address")


@test("the services the build exposes are advertised", needs=("srvc",))
def services_are_advertised(t):
    """
    Whichever servers are compiled in should be the ones announced. The suite
    knows the build by what answered on its ports, so this checks the two agree
    for the transport it arrived on.
    """
    _, _, services = mdns_status(t)
    if not services:
        raise Skip("the mdns service advertises nothing on this build")

    joined = " ".join(services)
    for token in ("_tcp", ":"):
        if token not in joined:
            raise AssertionError("the service list is not host:port shaped: %s"
                                 % joined)


@test("the board answers an A query for its own name", needs=("srvc",), slow=True)
def answers_a_query(t):
    host, _, _ = mdns_status(t)

    records = answered(t, host, mdns.TYPE_A, "its own name")

    addresses = [r["address"] for r in records if "address" in r]
    if not addresses:
        raise AssertionError("the board answered without an A record: %s" % records)

    if t.address() not in addresses:
        raise AssertionError("the board answered %s for %s, but it is reachable at %s"
                             % (addresses, host, t.address()))


@test("the A record carries the configured time to live", needs=("srvc",), slow=True)
def a_record_ttl(t):
    """
    A record with no useful lifetime makes every client re-query constantly,
    which on a battery powered network is worse than not advertising at all.
    """
    host, _, _ = mdns_status(t)

    records = answered(t, host, mdns.TYPE_A, "its own name")
    a_records = [r for r in records if mdns.TYPE_A == r["type"]]
    if not a_records:
        raise AssertionError("no A record in the answer: %s" % records)

    ttl = a_records[0]["ttl"]
    if ttl <= 0 or ttl > 4500:
        raise AssertionError("the A record ttl is %d seconds, which is not a "
                             "sensible lifetime" % ttl)


@test("service enumeration lists what the board advertises",
      needs=("srvc",), slow=True)
def dns_sd_enumeration(t):
    """
    `_services._dns-sd._udp.local` is how a browser asks "what kinds of thing
    are here", and the answer should name one service type per advertisement.
    """
    _, _, services = mdns_status(t)
    if not services:
        raise Skip("the mdns service advertises nothing on this build")

    records = answered(t, "_services._dns-sd._udp.local", mdns.TYPE_PTR,
                       "the service enumeration")

    pointers = [r for r in records if mdns.TYPE_PTR == r["type"]]
    if len(pointers) < 1:
        raise AssertionError("enumeration answered without a PTR record: %s" % records)

    if len(pointers) != len(services):
        raise AssertionError("mdns enumerated %d service types but srvc lists %d: "
                             "%s vs %s"
                             % (len(pointers), len(services),
                                [p.get("target") for p in pointers], services))


@test("browsing a service type answers with how to reach it",
      needs=("srvc",), slow=True)
def dns_sd_browse(t):
    """
    A browse should come back with the whole bundle a client needs in one go —
    the pointer, the port to connect to, and the address to connect at — rather
    than making it ask three more times.
    """
    _, _, services = mdns_status(t)

    browsable = [s for s in services if "_tcp" in s]
    if not browsable:
        raise Skip("this build advertises no _tcp service to browse")

    # "_ssh_tcp:22" as srvc prints it -> "_ssh._tcp.local" on the wire
    name, _, port = browsable[0].partition(":")
    query = name.replace("_tcp", "._tcp") + ".local"

    records = answered(t, query, mdns.TYPE_PTR, query)
    kinds = set(r["type"] for r in records)

    for wanted, label in ((mdns.TYPE_PTR, "a PTR"), (mdns.TYPE_SRV, "an SRV"),
                          (mdns.TYPE_A, "an A")):
        if wanted not in kinds:
            raise AssertionError("browsing %s answered without %s record: %s"
                                 % (query, label, sorted(kinds)))

    srv = [r for r in records if mdns.TYPE_SRV == r["type"] and "port" in r]
    if srv and port.isdigit() and srv[0]["port"] != int(port):
        raise AssertionError("mdns advertises port %d for %s, srvc says %s"
                             % (srv[0]["port"], name, port))


@test("a name the board does not own gets no answer from it",
      needs=("srvc",), slow=True)
def unknown_name_is_not_answered(t):
    """
    Answering for names it does not own would poison every cache on the
    segment. Only the board's own replies count here, so a responder on the
    test machine answering does not make this pass or fail.
    """
    try:
        records = mdns.ask(t.address(), "not-this-board-at-all.local",
                           mdns.TYPE_A, attempts=2)
    except mdns.MdnsError as err:
        raise Skip("cannot run an mdns query from here: %s" % err)

    if records:
        raise AssertionError("the board answered for a name it does not own: %s"
                             % records)
