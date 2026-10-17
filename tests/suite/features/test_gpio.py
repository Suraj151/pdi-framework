#!/usr/bin/env python3

"""
The gpio pages of the web portal, against the board's own pins.

Every page here writes the gpio record, so each test reads the page first and
puts it back afterwards. The values a page accepts are checked against what the
same service accepts over json: a mode past the last one, a value past what the
mode can hold, or a host longer than the buffer it lands in must not get in
through the portal either.

These mirror tests/host/system/test_gpio.cpp, which covers the service's json.
"""

import re

from .registry import test, expect_in, Skip
from .test_mqtt import read_form, write_form

MANAGE = "/gpio-manage"
SERVER = "/gpio-server"
MODES = "/gpio-config"
WRITE = "/gpio-write"
EVENTS = "/gpio-event"
MONITOR = "/gpio-monitor"
LIVE = "/listen-monitor"

HOST_BUFFER = 60

OFF, DOUT, DIN, BLINK, AOUT = "0", "1", "2", "3", "4"


def gpio_portal(t):
    """A logged in portal that serves the gpio pages, or a skip."""
    portal = t.portal()
    if 404 == portal.get(MODES).status:
        raise Skip("this build serves no gpio pages")
    return portal


class Restore(object):
    """Puts every gpio page back the way the test found it."""

    def __init__(self, portal):
        self.portal = portal
        self.saved = [(path, read_form(portal, path)) for path in (SERVER, MODES, WRITE)]

    def __enter__(self):
        return self

    def __exit__(self, *exc):
        for path, fields in self.saved:
            if fields:
                write_form(self.portal, path, fields)
        return False


def digital_pins(fields):
    """The digital pins the mode page offers, in order."""
    return sorted((int(k[1:]) for k in fields if re.match(r"^d\d+$", k)))


def set_modes(portal, modes):
    """Sets pin modes on the mode page, keeping every other pin as it is."""
    fields = read_form(portal, MODES)
    fields.update(modes)
    write_form(portal, MODES, fields)
    return read_form(portal, MODES)


@test("every gpio page renders", needs=())
def gpio_pages_render(t):
    portal = gpio_portal(t)
    for path in (MANAGE, SERVER, MODES, WRITE, EVENTS):
        answer = portal.get(path)
        if 200 != answer.status:
            raise AssertionError("%s answered %d" % (path, answer.status))
        expect_in("<html", answer.body, path)


@test("the gpio server settings read back as they were saved", needs=())
def gpio_server_round_trip(t):
    portal = gpio_portal(t)
    with Restore(portal):
        write_form(portal, SERVER, {"hst": "http://gpio.example.com:8081", "frq": "45"})
        got = read_form(portal, SERVER)

        if got.get("hst") != "http://gpio.example.com:8081":
            raise AssertionError("server read back as %r" % got.get("hst"))
        if got.get("frq") != "45":
            raise AssertionError("frequency read back as %r" % got.get("frq"))
        if "prt" in got:
            raise AssertionError("the page still offers a port field, which nothing reads: %r" % got)

        write_form(portal, SERVER, {"hst": "http://gpio.example.com:8081", "frq": ""})
        if read_form(portal, SERVER).get("frq") != "0":
            raise AssertionError("a blank frequency saved %r, not the default" % read_form(portal, SERVER).get("frq"))


@test("a shorter gpio host replaces a longer one whole", needs=())
def gpio_server_shorter_host(t):
    portal = gpio_portal(t)
    with Restore(portal):
        write_form(portal, SERVER, {"hst": "a-long-gpio-host.example.com", "prt": "80", "frq": "30"})
        write_form(portal, SERVER, {"hst": "ab.cd", "prt": "80", "frq": "30"})

        got = read_form(portal, SERVER).get("hst")
        if got != "ab.cd":
            raise AssertionError("the new host kept the tail of the old one: %r" % got)


@test("a gpio host longer than its buffer is refused and changes nothing", needs=())
def gpio_server_long_host(t):
    portal = gpio_portal(t)
    with Restore(portal):
        write_form(portal, SERVER, {"hst": "kept.example.com", "prt": "8081", "frq": "45"})
        before = read_form(portal, SERVER)

        write_form(portal, SERVER, {"hst": "h" * (HOST_BUFFER * 2), "prt": "8082", "frq": "50"})
        after = read_form(portal, SERVER)

        if after != before:
            raise AssertionError("a host past the buffer changed the page: %r became %r"
                                 % (before, after))

        # the bytes past the buffer were the service's pin blinkers, so a
        # spill shows up as the board going away on the next gpio tick
        if 200 != portal.get(MODES).status:
            raise AssertionError("the board stopped answering after the long host")


@test("pin modes read back as they were saved", needs=())
def gpio_modes_round_trip(t):
    portal = gpio_portal(t)
    with Restore(portal):
        pins = digital_pins(read_form(portal, MODES))
        if len(pins) < 2:
            raise Skip("this board offers fewer than two digital pins")

        got = set_modes(portal, {"d%d" % pins[0]: DOUT, "d%d" % pins[1]: DIN})

        if got.get("d%d" % pins[0]) != DOUT or got.get("d%d" % pins[1]) != DIN:
            raise AssertionError("modes read back as %r" % got)


@test("a pin mode past the last one is refused", needs=())
def gpio_mode_out_of_range(t):
    portal = gpio_portal(t)
    with Restore(portal):
        pin = "d%d" % digital_pins(read_form(portal, MODES))[0]
        set_modes(portal, {pin: DOUT})

        got = set_modes(portal, {pin: "200"})
        if got.get(pin) != DOUT:
            raise AssertionError("mode 200 replaced the pin's mode: %r" % got.get(pin))


@test("a value written to an output pin reads back", needs=())
def gpio_write_round_trip(t):
    portal = gpio_portal(t)
    with Restore(portal):
        pin = digital_pins(read_form(portal, MODES))[0]
        set_modes(portal, {"d%d" % pin: DOUT})

        write_form(portal, WRITE, {"d%d" % pin: "1"})
        got = read_form(portal, WRITE).get("d%d" % pin)
        if got != "1":
            raise AssertionError("the written value read back as %r" % got)


@test("a value past what the pin's mode can hold is refused", needs=())
def gpio_write_out_of_range(t):
    portal = gpio_portal(t)
    with Restore(portal):
        pins = digital_pins(read_form(portal, MODES))
        if len(pins) < 2:
            raise Skip("this board offers fewer than two digital pins")
        digital, analog = "d%d" % pins[0], "d%d" % pins[1]
        set_modes(portal, {digital: DOUT, analog: AOUT})

        write_form(portal, WRITE, {digital: "1", analog: "700"})
        write_form(portal, WRITE, {digital: "5", analog: "5000"})
        got = read_form(portal, WRITE)

        if got.get(digital) != "1":
            raise AssertionError("a digital output took 5: %r" % got.get(digital))
        if got.get(analog) != "700":
            raise AssertionError("an analog output took 5000: %r" % got.get(analog))


@test("the gpio monitor page renders", needs=())
def gpio_monitor_renders(t):
    portal = gpio_portal(t)
    answer = portal.get(MONITOR)
    if 200 != answer.status:
        raise AssertionError("the monitor answered %d" % answer.status)
    expect_in("<html", answer.body, "the monitor page")


@test("the live monitor carries every pin and continues its graph", needs=())
def gpio_live_monitor(t):
    portal = gpio_portal(t)
    portal.get(MONITOR)

    first = portal.get(LIVE).json()
    second = portal.get(LIVE).json()

    for key in ("x1", "y1", "x2", "y2", "d", "md"):
        if key not in first:
            raise AssertionError("the live monitor carried no %r: %r" % (key, first))

    if len(first["md"]) != 6:
        raise AssertionError("the mode names were %r" % first["md"])
    if not first["d"].get("data"):
        raise AssertionError("the live monitor carried no pins: %r" % first["d"])
    if second["x1"] != first["x2"] or second["y1"] != first["y2"]:
        raise AssertionError("the graph broke between two reads: %r then %r"
                             % ((first["x2"], first["y2"]), (second["x1"], second["y1"])))
