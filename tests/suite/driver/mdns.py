#!/usr/bin/env python3

"""
A minimal multicast DNS client, enough to ask a target for its own records.

Written by hand rather than pulled from a library for the same reason the rest
of the harness is: one dependency-free file that does exactly what the tests
need and can be read in one sitting.

The one rule that matters here is **only trust an answer that came from the
target**. A developer machine usually runs its own responder, and a docker
bridge may run another; both will happily answer for a name they have cached,
so a client that accepts the first reply tests the wrong host. Every reply is
filtered by source address.
"""

import socket
import struct
import time

GROUP = "224.0.0.251"
PORT = 5353

TYPE_A = 1
TYPE_PTR = 12
TYPE_TXT = 16
TYPE_SRV = 33
TYPE_ANY = 255


class MdnsError(Exception):
    pass


def local_address_towards(host):
    """The address of the interface that reaches host, for the multicast join."""
    probe = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    try:
        probe.connect((host, 9))
        return probe.getsockname()[0]
    finally:
        probe.close()


def encode_question(name, qtype):
    header = struct.pack(">HHHHHH", 0, 0, 1, 0, 0, 0)
    labels = b"".join(bytes([len(part)]) + part.encode()
                      for part in name.split(".") if part)
    return header + labels + b"\x00" + struct.pack(">HH", qtype, 1)


def read_name(data, offset):
    """Decode a possibly compressed name; returns (name, offset after it)."""
    parts = []
    jumped = False
    end = offset
    seen = 0

    while offset < len(data):
        length = data[offset]
        if 0 == length:
            offset += 1
            if not jumped:
                end = offset
            break
        if 0xC0 == (length & 0xC0):
            pointer = ((length & 0x3F) << 8) | data[offset + 1]
            if not jumped:
                end = offset + 2
            offset = pointer
            jumped = True
            seen += 1
            if seen > 8:
                break
            continue
        parts.append(data[offset + 1:offset + 1 + length].decode(errors="replace"))
        offset += 1 + length
        if not jumped:
            end = offset

    return ".".join(parts), end


def parse_records(data):
    """Every answer record as a dict; questions are skipped over."""
    if len(data) < 12:
        return []

    qd, an, ns, ar = struct.unpack(">HHHH", data[4:12])
    offset = 12

    for _ in range(qd):
        _, offset = read_name(data, offset)
        offset += 4

    records = []
    for _ in range(an + ns + ar):
        if offset >= len(data):
            break
        name, offset = read_name(data, offset)
        if offset + 10 > len(data):
            break
        rtype, rclass, ttl, rdlen = struct.unpack(">HHIH", data[offset:offset + 10])
        offset += 10
        rdata = data[offset:offset + rdlen]
        offset += rdlen

        entry = {"name": name, "type": rtype, "ttl": ttl,
                 "cache_flush": bool(rclass & 0x8000), "rdata": rdata}

        if TYPE_A == rtype and 4 == len(rdata):
            entry["address"] = ".".join(str(b) for b in rdata)
        elif rtype in (TYPE_PTR,):
            entry["target"], _ = read_name(data, offset - rdlen)
        elif TYPE_SRV == rtype and rdlen >= 6:
            entry["port"] = struct.unpack(">H", rdata[4:6])[0]
            entry["target"], _ = read_name(data, offset - rdlen + 6)

        records.append(entry)

    return records


def ask(target, name, qtype=TYPE_A, timeout=6.0, attempts=4, gap=3.0):
    """
    Ask on the multicast group and return the records the target itself sent.

    Retried because a responder is required to suppress a repeat of an answer
    it has just given, and because consumer access points forward multicast
    unevenly. Returns [] when the target never answers.
    """
    local = local_address_towards(target)

    for attempt in range(attempts):
        sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        try:
            sock.bind(("", PORT))
            sock.setsockopt(socket.IPPROTO_IP, socket.IP_ADD_MEMBERSHIP,
                            socket.inet_aton(GROUP) + socket.inet_aton(local))
            sock.setsockopt(socket.IPPROTO_IP, socket.IP_MULTICAST_IF,
                            socket.inet_aton(local))
            sock.settimeout(0.5)

            sock.sendto(encode_question(name, qtype), (GROUP, PORT))

            deadline = time.time() + timeout
            while time.time() < deadline:
                try:
                    data, addr = sock.recvfrom(4096)
                except socket.timeout:
                    continue
                except OSError:
                    break

                # anyone may answer on this group; only the target counts
                if addr[0] != target:
                    continue
                if not (data[2] & 0x80):
                    continue

                found = parse_records(data)
                if found:
                    return found
        except OSError as err:
            raise MdnsError("cannot listen on %s:%d: %s" % (GROUP, PORT, err))
        finally:
            sock.close()

        if attempt + 1 < attempts:
            time.sleep(gap)

    return []
