#!/usr/bin/env python3

"""
The db command on the target's own record store.

The host tier covers every verb against the stack it builds; this suite asks
the same questions of a real medium — eeprom on one board, a file on another —
where the store was written by firmware that has been running for a while
rather than formatted a moment ago. The verbs are checked against each other,
so a status line and a listing that disagree about the store fail here.

These mirror tests/host/system/test_commands_db.cpp. Nothing here writes to the
store: save and restore change what a board falls back to, and a test run is
not the place to do that to someone's device.
"""

import re

from .registry import test, expect_in, expect_not_in


def status_value(out, label):
    """The number after a "label : " line in db status, or None."""
    match = re.search(r"%s\s*:\s*(-?\d+)" % re.escape(label), out)
    return int(match.group(1)) if match else None


def list_rows(out):
    """The record rows of db list, the header left out."""
    return [line.strip() for line in out.splitlines() if re.match(r"\s*\d+\s", line)]


@test("db status names a mounted medium and how full it is", needs=("db",))
def db_status_reports(t):
    out = t.run("db status")

    expect_in("medium", out, "the status names the medium")
    expect_in("mounted  : yes", out, "the store is mounted")

    used = status_value(out, "used")
    free = status_value(out, "free")
    if used is None or used <= 0:
        raise AssertionError("db status reported no bytes used: %r" % out)
    if free is None or free < 0:
        raise AssertionError("db status reported no free figure: %r" % out)


@test("db list has a row for every record db status counts", needs=("db",))
def db_list_matches_status(t):
    records = status_value(t.run("db status"), "records")
    rows = list_rows(t.run("db list"))

    if records is None:
        raise AssertionError("db status reported no record count")
    if len(rows) != records:
        raise AssertionError("db status counts %d records, db list shows %d"
                             % (records, len(rows)))

    for row in rows:
        if not any(state in row for state in ("default", "sealed", "stored")):
            raise AssertionError("a listing row with no known state: %r" % row)


@test("db verify passes every record on the target", needs=("db",))
def db_verify_passes(t):
    expect_in("every record verified", t.run("db verify"),
              "a store in normal use verifies")


@test("db refuses a verb it does not know", needs=("db",))
def db_refuses_unknown_verb(t):
    out = t.run("db bogus")
    expect_not_in("medium", out, "an unknown verb printed a status")
    expect_in("status|list|verify|save|restore", out, "the usage line")
