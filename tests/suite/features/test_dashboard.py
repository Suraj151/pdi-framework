#!/usr/bin/env python3

"""
The dashboard's live feed: the one json object its page polls for the radio,
storage, heap, busiest tasks, sessions and pins.

The page parses the whole object at once, so one value that breaks the json —
a network name with a quote in it — blanks every panel, not just its own. Each
test parses what the board sent rather than searching it for text.
"""

import time

from .registry import test, Skip
from .test_mqtt import read_form, write_form
from ..driver.host_shell import HostShell

PAGE = "/dashboard"
LIVE = "/listen-dashboard"
WIFI = "/wifi-config"

KEYS = ("w", "nwt", "fs", "up", "hp", "hb", "tc", "ps", "se", "gm", "gp", "dv")
TASK_ROWS = 4

# a quote, a backslash and a tab: each one breaks json written without escaping
AWKWARD_SSID = 'pdi "lab"\\2\tg'


def live(t, portal=None):
    portal = portal or t.portal()
    if 404 == portal.get(PAGE).status:
        raise Skip("this build serves no dashboard")

    answer = portal.get(LIVE)
    if 200 != answer.status:
        raise AssertionError("the live feed answered %d" % answer.status)

    try:
        return answer.json()
    except ValueError as error:
        raise AssertionError("the live feed is not json (%s): %r" % (error, answer.body[:300]))


@test("the live feed is one json object carrying every panel")
def feed_carries_every_panel(t):
    feed = live(t)

    missing = [key for key in KEYS if key not in feed]
    if missing:
        raise AssertionError("the feed carried no %r: %r" % (missing, sorted(feed)))

    for key in ("c", "nm", "ip", "mc", "rs", "nt"):
        if key not in feed["w"]:
            raise AssertionError("the radio panel carried no %r: %r" % (key, feed["w"]))


@test("the heap and task figures are whole numbers a board could have")
def feed_figures_are_plausible(t):
    feed = live(t)

    for key in ("up", "hp", "hb", "tc"):
        if not isinstance(feed[key], int) or feed[key] < 0:
            raise AssertionError("%s was %r" % (key, feed[key]))

    if feed["hp"] == 0 or feed["hb"] > feed["hp"]:
        raise AssertionError("the largest free block %d is not within the free heap %d" % (feed["hb"], feed["hp"]))
    if feed["tc"] < 1:
        raise AssertionError("the feed counted no tasks at all")


@test("the busiest tasks come back ranked, at most four of them")
def feed_ranks_the_busiest_tasks(t):
    feed = live(t)
    rows = feed["ps"]

    if not rows:
        raise AssertionError("the feed listed no tasks while %d are running" % feed["tc"])
    if len(rows) > TASK_ROWS:
        raise AssertionError("the feed listed %d tasks, more than its %d rows" % (len(rows), TASK_ROWS))

    shares = []
    for row in rows:
        if 4 != len(row) or not isinstance(row[0], int) or not row[1] or 1 != len(row[2]):
            raise AssertionError("a task row was %r" % (row,))
        whole, _, hundredths = row[3].partition(".")
        if not whole.isdigit() or 2 != len(hundredths) or not hundredths.isdigit():
            raise AssertionError("a cpu share read %r, not a two place percentage" % row[3])
        shares.append(float(row[3]))

    if shares != sorted(shares, reverse=True):
        raise AssertionError("the tasks were not ranked by cpu share: %r" % rows)


@test("the uptime moves on between two reads")
def feed_uptime_advances(t):
    portal = t.portal()
    first = live(t, portal)
    time.sleep(1.5)
    second = live(t, portal)

    if second["up"] <= first["up"]:
        raise AssertionError("the uptime read %d then %d" % (first["up"], second["up"]))


@test("the sessions panel lists the shell session and the browser reading it")
def feed_lists_sessions(t):
    feed = live(t)

    names = [row[0] for row in feed["se"]]
    vias = [row[1] for row in feed["se"]]

    if t.username not in names:
        raise AssertionError("%s is logged in but the panel listed %r" % (t.username, feed["se"]))
    if "web" not in vias:
        raise AssertionError("the browser reading the feed was not listed: %r" % feed["se"])

    for row in feed["se"]:
        if 4 != len(row) or row[2] < row[3] or row[3] < 0:
            raise AssertionError("a session row was %r: idle longer than logged in" % (row,))


@test("the live feed refuses a client that has not logged in")
def feed_refuses_strangers(t):
    stranger = t.portal(login=False)
    if 404 == stranger.get(PAGE).status:
        raise Skip("this build serves no dashboard")

    answer = stranger.get(LIVE)
    if 401 != answer.status:
        raise AssertionError("a client with no session was answered %d" % answer.status)
    if '"hp"' in answer.body:
        raise AssertionError("a client with no session was sent the feed")


@test("a network name with quotes and a backslash still reaches the page as json")
def feed_survives_an_awkward_ssid(t):
    if not isinstance(t.shell, HostShell):
        raise Skip("renaming the network takes a board off it, so this runs against the host process only")

    portal = t.portal()
    if 404 == portal.get(WIFI).status:
        raise Skip("this build serves no wifi page")

    before = read_form(portal, WIFI)
    try:
        fields = dict(before)
        fields["sta_ssid"] = AWKWARD_SSID
        write_form(portal, WIFI, fields)

        # a saved wifi page restarts the radio and ends the browser session
        deadline = time.time() + 15
        feed = None
        while time.time() < deadline:
            feed = live(t, t.portal())
            if AWKWARD_SSID == feed["w"]["nm"]:
                return
            time.sleep(1)

        raise AssertionError("the network name came back %r" % (feed and feed["w"]["nm"],))
    finally:
        write_form(t.portal(), WIFI, before)
