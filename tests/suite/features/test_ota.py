#!/usr/bin/env python3

"""
The firmware update check against an update server of the suite's own.

The board asks its configured server for the latest published version on a
fixed schedule, naming itself by mac. Whatever comes back — a version, a
refusal, something that is not json, or nothing at all — the board has to keep
asking on schedule and keep answering everything else meanwhile. Fetching and
flashing an image is the port's own work and the host process skips it, so
that half stays with the device tier.
"""

import base64
import time

from .registry import test, Skip
from .test_mqtt import read_form, write_form
from .test_gpio_live import idle_read, CLIENT_TIMEOUT
from ..driver.http_sink import HttpSink
from ..driver.portal import PortalError
from ..driver.mdns import local_address_towards

PAGE = "/ota-config"
VERSION_URL = "/api/fordevice/ota-version"
EVERY = 15.0


class UpdateServer(object):

    def __init__(self, t):
        self.t = t
        self.portal = t.portal()
        if 404 == self.portal.get(PAGE).status:
            raise Skip("this build serves no ota page")
        self.saved = read_form(self.portal, PAGE)
        self.sink = HttpSink().start()
        self.sink.reply = b'{"latest":"0"}'
        address = local_address_towards(t.address())
        write_form(self.portal, PAGE, {"hst": "http://%s:%d" % (address, self.sink.port), "prt": str(self.sink.port)})

    def __enter__(self):
        return self

    def __exit__(self, *exc):
        try:
            write_form(self.t.portal(), PAGE, self.saved)
        finally:
            self.sink.stop()
        return False

    def next_check(self, timeout=EVERY * 2 + 5):
        count = len(self.sink.requests(VERSION_URL))
        check = self.sink.wait_for(VERSION_URL, timeout, after=count)
        if check is None:
            raise AssertionError("the board asked its update server nothing in %.0f s" % timeout)
        return check


@test("the board asks its update server for the latest version on its schedule")
def checks_on_schedule(t):
    with UpdateServer(t) as server:
        first = server.next_check()
        second = server.next_check()
        if "GET" != first.method:
            raise AssertionError("the version check went as a %s" % first.method)
        # the schedule is fixed-rate with catch up, so a check run late by d
        # seconds is followed by one d seconds early
        gap = second.at - first.at
        if not EVERY - 6 <= gap <= EVERY + 6:
            raise AssertionError("two version checks were %.0f s apart on a %.0f s schedule" % (gap, EVERY))


@test("the version check names the board by its mac and authorises with it")
def check_names_the_board(t):
    with UpdateServer(t) as server:
        mac = t.portal().get("/listen-dashboard").json()["w"]["mc"]
        check = server.next_check()
        if "mac_id=%s" % mac not in check.path:
            raise AssertionError("the check %r does not name the board's mac %s" % (check.path, mac))
        auth = check.headers.get("authorization", "")
        scheme, _, encoded = auth.partition("@")
        if not encoded or base64.b64decode(encoded).decode(errors="replace") != "ota:%s" % mac:
            raise AssertionError("the check authorised as %r" % auth)
        if check.headers.get("user-agent") != "pdistack":
            raise AssertionError("the check carried user agent %r" % check.headers.get("user-agent"))


def keeps_checking(t, setup, what):
    with UpdateServer(t) as server:
        setup(server)
        server.next_check()
        server.next_check()
        if 200 != t.portal(login=False).get("/login").status:
            raise AssertionError("the board stopped answering after %s" % what)


@test("an update server that refuses keeps being asked on schedule")
def refusal_survived(t):
    keeps_checking(t, lambda s: setattr(s.sink, "status", 500), "a refused version check")


@test("a reply that is not json keeps the checks coming")
def garbage_survived(t):
    keeps_checking(t, lambda s: setattr(s.sink, "reply", b"<html>not json"), "a reply that was not json")


@test("a reply with no version keeps the checks coming")
def no_version_survived(t):
    keeps_checking(t, lambda s: setattr(s.sink, "reply", b'{"other":"1"}'), "a reply with no version")


@test("an update server that never answers does not stall the board")
def hanging_server(t):
    with UpdateServer(t) as server:
        idle = idle_read(t.portal(login=False), "/login")
        server.sink.hang = True
        server.next_check()

        # a check may block the loop until the client gives up, which is
        # intended, so a read may wait out one check but never more than that
        budget = idle + CLIENT_TIMEOUT + 1.0
        worst = 0.0
        for _ in range(8):
            began = time.time()
            try:
                status = t.portal(login=False).get("/login", attempts=1).status
            except PortalError as err:
                raise AssertionError("the portal stopped answering while the update server hung: %s" % err)
            if 200 != status:
                raise AssertionError("the portal answered %s while the update server hung" % status)
            worst = max(worst, time.time() - began)
            time.sleep(0.5)
        if worst > budget:
            raise AssertionError("the portal took %.1f s to answer while the update server hung (budget %.1f s, "
                                 "idle read %.1f s)" % (worst, budget, idle))


@test("a board with no update server configured asks nothing")
def no_host_no_check(t):
    with UpdateServer(t) as server:
        server.next_check()
        write_form(t.portal(), PAGE, {"hst": "", "prt": "80"})
        time.sleep(2.0)
        count = len(server.sink.requests(VERSION_URL))
        time.sleep(EVERY + 5)
        if len(server.sink.requests(VERSION_URL)) != count:
            raise AssertionError("the board kept asking after its update server was cleared")
