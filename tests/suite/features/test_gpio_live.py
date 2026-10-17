#!/usr/bin/env python3

"""
A pin changed from outside, watched the way an operator watches it: on the
gpio monitor's live feed and on the dashboard's pin panel.

The gpio service applies its working copy once a second, so every change made
here — through the portal, a broker or the json a server sends — has to be on
both feeds within one tick plus a poll. A value the service refuses must never
appear on either, not even for one read. And the pins leave the board as well:
the gpio server is posted the same pins on its own schedule, so a change is
followed there too, against a server of the suite's own.
"""

import json
import time

from .registry import test, Skip
from .test_mqtt import read_form, write_form, connected, SUB_FIRST, PUBLISH_WAIT
from .test_gpio import gpio_portal, digital_pins, Restore, MODES, WRITE, SERVER
from ..driver.http_sink import HttpSink
from ..driver.mdns import local_address_towards

LIVE = "/listen-monitor"
DASHBOARD = "/listen-dashboard"

OFF, DOUT, DIN, BLINK, AOUT = 0, 1, 2, 3, 4

# one gpio tick, one poll and the time the save itself took to settle
TICK = 1.5

# how long the board's http client waits on a server before giving up
CLIENT_TIMEOUT = 2.0

DATA_URL = "/api/fordevice/data/"


def monitor_pins(portal):
    """The live monitor's pins as {label: (mode, value)}."""
    data = portal.get(LIVE).json()["d"]["data"]
    return dict((label, (pin["mode"], pin["val"])) for label, pin in data.items())


def dashboard_pins(portal):
    """The dashboard's pins as {label: (mode, value, analog)}; OFF pins are not listed."""
    return dict((row[0], (row[1], row[2], row[3])) for row in portal.get(DASHBOARD).json()["gp"])


def seen_within(portal, label, mode, value, timeout=TICK):
    """
    Polls both feeds until the pin reads as given on each, returning how long
    that took, or raises naming what each feed last said.

    A poll is two portal reads, which over https can take longer than the whole
    timeout, so at least two polls are made: the second starts after the first
    has seen both feeds, and a change that landed between its two reads is not
    mistaken for the feeds disagreeing.
    """
    began = time.time()
    last = (None, None)
    polls = 0
    while polls < 2 or time.time() - began < timeout:
        live = monitor_pins(portal).get(label)
        board = dashboard_pins(portal).get(label)
        polls += 1
        last = (live, board)
        if live == (mode, value) and board is not None and board[:2] == (mode, value):
            return time.time() - began
        time.sleep(0.1)
    raise AssertionError("%s did not read mode %d value %d within %.1f s: monitor said %r, dashboard %r"
                         % (label, mode, value, timeout, last[0], last[1]))


def idle_read(portal, path, reads=3):
    """The slowest of a few reads of a page, taken before the test loads the board."""
    worst = 0.0
    for _ in range(reads):
        began = time.time()
        portal.get(path)
        worst = max(worst, time.time() - began)
    return worst


def never_seen(portal, label, value, watch=TICK + 1.0):
    """Watches both feeds for a while and fails on any read carrying the value."""
    deadline = time.time() + watch
    while time.time() < deadline:
        live = monitor_pins(portal).get(label)
        board = dashboard_pins(portal).get(label)
        if (live and live[1] == value) or (board and board[1] == value):
            raise AssertionError("%s read %r on a feed after it was refused: monitor %r, dashboard %r"
                                 % (label, value, live, board))
        time.sleep(0.1)


def pick_pins(portal, count):
    pins = digital_pins(read_form(portal, MODES))
    if len(pins) < count:
        raise Skip("this board offers fewer than %d digital pins" % count)
    return pins[-count:]


def set_mode(portal, pin, mode):
    fields = read_form(portal, MODES)
    fields[pin] = str(mode)
    write_form(portal, MODES, fields)


def set_value(portal, pin, value):
    fields = read_form(portal, WRITE)
    if pin not in fields:
        raise AssertionError("the write page offers no %s after its mode was set: %r" % (pin, sorted(fields)))
    fields[pin] = str(value)
    write_form(portal, WRITE, fields)


@test("an output switched on, off and on again shows on both feeds within a tick")
def output_toggles_on_both_feeds(t):
    portal = gpio_portal(t)
    with Restore(portal):
        pin = pick_pins(portal, 1)[0]
        label, field = "D%d" % pin, "d%d" % pin

        set_mode(portal, field, DOUT)
        for value in (1, 0, 1):
            set_value(portal, field, value)
            seen_within(portal, label, DOUT, value)


@test("an analog output's value shows on both feeds within a tick")
def analog_output_on_both_feeds(t):
    portal = gpio_portal(t)
    with Restore(portal):
        pin = pick_pins(portal, 1)[0]
        label, field = "D%d" % pin, "d%d" % pin

        set_mode(portal, field, AOUT)
        for value in (700, 0, 1023):
            set_value(portal, field, value)
            seen_within(portal, label, AOUT, value)


@test("a blinking pin shows its mode and period on both feeds within a tick")
def blink_on_both_feeds(t):
    portal = gpio_portal(t)
    with Restore(portal):
        pin = pick_pins(portal, 1)[0]
        label, field = "D%d" % pin, "d%d" % pin

        set_mode(portal, field, BLINK)
        set_value(portal, field, 500)
        seen_within(portal, label, BLINK, 500)


@test("an input pin shows on both feeds as an input")
def input_on_both_feeds(t):
    portal = gpio_portal(t)
    with Restore(portal):
        pin = pick_pins(portal, 1)[0]
        label, field = "D%d" % pin, "d%d" % pin

        set_mode(portal, field, DIN)
        began = time.time()
        while time.time() - began < TICK:
            live = monitor_pins(portal).get(label)
            board = dashboard_pins(portal).get(label)
            if live and live[0] == DIN and board and board[0] == DIN:
                return
            time.sleep(0.1)
        raise AssertionError("%s never read as an input: monitor %r, dashboard %r"
                             % (label, monitor_pins(portal).get(label), dashboard_pins(portal).get(label)))


@test("switching a pin off takes it off the dashboard and zeroes it on the monitor")
def off_leaves_the_dashboard(t):
    portal = gpio_portal(t)
    with Restore(portal):
        pin = pick_pins(portal, 1)[0]
        label, field = "D%d" % pin, "d%d" % pin

        set_mode(portal, field, DOUT)
        set_value(portal, field, 1)
        seen_within(portal, label, DOUT, 1)

        set_mode(portal, field, OFF)
        began = time.time()
        while time.time() - began < TICK:
            if monitor_pins(portal).get(label) == (OFF, 0) and label not in dashboard_pins(portal):
                return
            time.sleep(0.1)
        raise AssertionError("%s switched off still read monitor %r, dashboard %r"
                             % (label, monitor_pins(portal).get(label), dashboard_pins(portal).get(label)))


@test("a value the pin's mode cannot hold never shows on either feed")
def refused_value_never_shows(t):
    portal = gpio_portal(t)
    with Restore(portal):
        pin = pick_pins(portal, 1)[0]
        label, field = "D%d" % pin, "d%d" % pin

        set_mode(portal, field, DOUT)
        set_value(portal, field, 1)
        seen_within(portal, label, DOUT, 1)

        fields = read_form(portal, WRITE)
        fields[field] = "5"
        write_form(portal, WRITE, fields)
        never_seen(portal, label, 5)


@test("two pins changed in one save both show, and a third is left alone")
def two_pins_one_save(t):
    portal = gpio_portal(t)
    with Restore(portal):
        a, b, c = pick_pins(portal, 3)

        fields = read_form(portal, MODES)
        fields["d%d" % a], fields["d%d" % b] = str(DOUT), str(AOUT)
        write_form(portal, MODES, fields)
        before = monitor_pins(portal).get("D%d" % c)

        fields = read_form(portal, WRITE)
        fields["d%d" % a], fields["d%d" % b] = "1", "321"
        write_form(portal, WRITE, fields)

        seen_within(portal, "D%d" % a, DOUT, 1)
        seen_within(portal, "D%d" % b, AOUT, 321)
        if monitor_pins(portal).get("D%d" % c) != before:
            raise AssertionError("a pin the save did not name changed: %r became %r"
                                 % (before, monitor_pins(portal).get("D%d" % c)))


@test("a gpio command from the broker shows on both feeds within a tick",
      needs=("service",), services=("MQTT",), slow=True)
def broker_command_on_both_feeds(t):
    state, _ = connected(t)
    if state.broker.wait_for_subscription(SUB_FIRST, PUBLISH_WAIT) is None:
        raise Skip("the client is not subscribed to %s yet" % SUB_FIRST)

    portal = gpio_portal(t)
    with Restore(portal):
        pin = pick_pins(portal, 1)[0]
        label = "D%d" % pin

        for value in (1, 0):
            command = {"data": {label: {"mode": DOUT, "val": value}}}
            if not state.broker.publish(SUB_FIRST, json.dumps(command)):
                raise Skip("the broker could not send to the client")
            seen_within(portal, label, DOUT, value, timeout=TICK + 2.0)


@test("a burst of broker commands for three pins all show within a tick",
      needs=("service",), services=("MQTT",), slow=True)
def broker_burst_on_both_feeds(t):
    state, _ = connected(t)
    if state.broker.wait_for_subscription(SUB_FIRST, PUBLISH_WAIT) is None:
        raise Skip("the client is not subscribed to %s yet" % SUB_FIRST)

    portal = gpio_portal(t)
    with Restore(portal):
        pins = pick_pins(portal, 3)
        for pin in pins:
            state.broker.publish(SUB_FIRST, json.dumps({"data": {"D%d" % pin: {"mode": DOUT, "val": 1}}}))
        for pin in pins:
            seen_within(portal, "D%d" % pin, DOUT, 1, timeout=TICK + 2.0)


@test("a broker command naming only a longer label leaves the shorter pin alone",
      needs=("service",), services=("MQTT",), slow=True)
def broker_command_label_is_whole(t):
    state, _ = connected(t)
    if state.broker.wait_for_subscription(SUB_FIRST, PUBLISH_WAIT) is None:
        raise Skip("the client is not subscribed to %s yet" % SUB_FIRST)

    portal = gpio_portal(t)
    with Restore(portal):
        pins = digital_pins(read_form(portal, MODES))
        if 1 not in pins or 10 not in pins:
            raise Skip("this board does not offer both D1 and D10")
        before = monitor_pins(portal).get("D1")

        state.broker.publish(SUB_FIRST, json.dumps({"data": {"D10": {"mode": DOUT, "val": 1}}}))
        seen_within(portal, "D10", DOUT, 1, timeout=TICK + 2.0)

        if monitor_pins(portal).get("D1") != before:
            raise AssertionError("D1 took D10's command: %r became %r" % (before, monitor_pins(portal).get("D1")))


@test("a broker command for a value the pin cannot hold never shows",
      needs=("service",), services=("MQTT",), slow=True)
def broker_command_refused(t):
    state, _ = connected(t)
    if state.broker.wait_for_subscription(SUB_FIRST, PUBLISH_WAIT) is None:
        raise Skip("the client is not subscribed to %s yet" % SUB_FIRST)

    portal = gpio_portal(t)
    with Restore(portal):
        pin = pick_pins(portal, 1)[0]
        label = "D%d" % pin

        state.broker.publish(SUB_FIRST, json.dumps({"data": {label: {"mode": DOUT, "val": 1}}}))
        seen_within(portal, label, DOUT, 1, timeout=TICK + 2.0)

        state.broker.publish(SUB_FIRST, json.dumps({"data": {label: {"mode": DOUT, "val": 7}}}))
        state.broker.publish(SUB_FIRST, json.dumps({"data": {label: {"mode": 99, "val": 0}}}))
        never_seen(portal, label, 7)
        if monitor_pins(portal).get(label) != (DOUT, 1):
            raise AssertionError("a refused command changed %s to %r" % (label, monitor_pins(portal).get(label)))


class GpioServer(object):
    """A sink the board posts its pins to, set up on the server page and put back after."""

    def __init__(self, t, every=2):
        self.portal = gpio_portal(t)
        self.saved = read_form(self.portal, SERVER)
        self.sink = HttpSink().start()
        address = local_address_towards(t.address())
        write_form(self.portal, SERVER, {"hst": "http://%s:%d" % (address, self.sink.port),
                                         "prt": str(self.sink.port), "frq": str(every)})

    def __enter__(self):
        return self

    def __exit__(self, *exc):
        try:
            write_form(self.portal, SERVER, self.saved)
        finally:
            self.sink.stop()
        return False

    def next_post(self, timeout=10.0):
        count = len(self.sink.requests(DATA_URL))
        post = self.sink.wait_for(DATA_URL, timeout, after=count)
        if post is None:
            raise AssertionError("the board posted nothing to its gpio server in %.0f s — it has to "
                                 "reach this machine on port %d" % (timeout, self.sink.port))
        return post


@test("the board posts its pins as json to the gpio server on its schedule")
def server_gets_pins_on_schedule(t):
    with GpioServer(t, every=2) as server:
        posts = [server.next_post() for _ in range(5)]
        first = posts[0]

        if "POST" != first.method:
            raise AssertionError("the pins went as a %s" % first.method)
        if "application/json" not in first.headers.get("content-type", ""):
            raise AssertionError("the pins were labelled %r" % first.headers.get("content-type"))

        body = json.loads(first.text())
        if not body.get("mac") or not body.get("data"):
            raise AssertionError("the post carried no mac or pins: %r" % body)

        # the schedule is fixed-rate, so one stall of the loop drops a slot and
        # leaves one long gap; the typical gap is what the schedule decides
        gaps = sorted(b.at - a.at for a, b in zip(posts, posts[1:]))
        median = (gaps[1] + gaps[2]) / 2.0
        if not 1.0 <= median <= 4.0:
            raise AssertionError("posts were a median %.1f s apart on a 2 s schedule: %s"
                                 % (median, ", ".join("%.1f" % g for g in gaps)))


@test("a pin changed on the board is in the next post to the gpio server")
def server_gets_a_change(t):
    with GpioServer(t, every=2) as server, Restore(server.portal):
        portal = server.portal
        pin = pick_pins(portal, 1)[0]
        label, field = "D%d" % pin, "d%d" % pin

        set_mode(portal, field, DOUT)
        set_value(portal, field, 1)
        seen_within(portal, label, DOUT, 1)

        deadline = time.time() + 10
        while time.time() < deadline:
            pins = json.loads(server.next_post().text())["data"]
            if pins.get(label) == {"mode": DOUT, "val": 1}:
                return
        raise AssertionError("%s never reached the gpio server as an output at 1" % label)


@test("a gpio server that refuses a post is still posted to afterwards")
def server_refusal_does_not_stop_posts(t):
    with GpioServer(t, every=2) as server:
        server.sink.status = 500
        server.next_post()
        server.next_post()
        server.sink.status = 200
        server.next_post()


@test("a gpio server that never answers does not stall the board")
def server_hang_does_not_stall(t):
    with GpioServer(t, every=2) as server:
        idle = idle_read(server.portal, MODES)
        server.sink.hang = True
        server.next_post()

        # a post blocks the loop until the client gives up, which is intended,
        # so a read may wait out one post but never more than that
        budget = idle + CLIENT_TIMEOUT + 1.0
        slow = 0
        for _ in range(10):
            began = time.time()
            if 200 != server.portal.get(MODES).status:
                raise AssertionError("the portal stopped answering while the gpio server hung")
            if time.time() - began > budget:
                slow += 1
        if slow > 3:
            raise AssertionError("%d of 10 portal reads took longer than %.1f s (idle read %.1f s) while the "
                                 "gpio server hung" % (slow, budget, idle))


@test("a post frequency of zero stops the posts")
def server_frequency_zero_stops(t):
    with GpioServer(t, every=2) as server:
        server.next_post()
        fields = read_form(server.portal, SERVER)
        fields["frq"] = "0"
        write_form(server.portal, SERVER, fields)

        time.sleep(1.0)
        count = len(server.sink.requests(DATA_URL))
        time.sleep(5.0)
        if len(server.sink.requests(DATA_URL)) != count:
            raise AssertionError("the board kept posting after its frequency was set to zero")
