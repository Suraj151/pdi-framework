#!/usr/bin/env python3

"""
Gpio alerts: a rule on an input pin, the input driven past it, and the alert
caught where it lands — a mail at a mail server of the suite's own, or a post
at a gpio server of the suite's own.

A rule is checked on every gpio tick against the input's reading. It fires on
the tick its condition becomes true, holds off repeating while the condition
stays true (five minutes after a delivered alert, two after a refused one), and
re-arms the moment the condition clears. Inputs are driven through the host
process's simulated pins, the way a wire to the board would drive them, so this
runs against the host process only.
"""

import json
import re
import time

from .registry import test, Skip
from .test_mqtt import read_form, write_form
from .test_gpio import gpio_portal, Restore, MODES, SERVER, EVENTS
from .test_smtp import PAGE as EMAIL_PAGE, SETTINGS as EMAIL_SETTINGS
from ..driver.host_shell import HostShell
from ..driver.http_sink import HttpSink
from ..driver.smtp_server import SmtpServer
from ..driver.mdns import local_address_towards

DIN, AIN = "2", "5"
EQUAL, GREATER, LESS = "0", "1", "2"
EVENT_URL = "/api/fordevice/event/"

# one tick to sample, one to be sure, and the delivery itself
FIRE_WAIT = 4.0
QUIET_WATCH = 4.0
SELECT = re.compile(r"<select\s+name='([^']+)'>(.*?)</select>", re.S)
OPTION = re.compile(r"<option\s+value='([^']*)'[^>]*>([^<]*)</option>")


def options(body, name):
    """The options of one select on a page, as {label: value}."""
    for found, inner in SELECT.findall(body):
        if found == name:
            return dict((label, value) for value, label in OPTION.findall(inner))
    return {}


class Alerts(object):
    """
    One test's rules, pins and alert destinations, put back afterwards. A pin
    switched off clears its rules, so putting the mode page back clears them.
    """

    def __init__(self, t):
        if not isinstance(t.shell, HostShell):
            raise Skip("an input is driven through the host process's simulated pins")
        self.t = t
        self.portal = gpio_portal(t)
        if 404 == self.portal.get(EVENTS).status:
            raise Skip("this build serves no gpio event page")
        self.restore = Restore(self.portal)
        self.address = local_address_towards(t.address())
        self.sink = HttpSink().start()
        self.mail = SmtpServer().start()
        self.saved_email = read_form(self.portal, EMAIL_PAGE) if 404 != self.portal.get(EMAIL_PAGE).status else None
        self.inputs = {}

        write_form(self.portal, SERVER, {"hst": "http://%s:%d" % (self.address, self.sink.port),
                                         "prt": str(self.sink.port), "frq": "0"})
        if self.saved_email is not None:
            fields = dict(EMAIL_SETTINGS)
            fields.update({"ml_srvr": self.address, "ml_prt": str(self.mail.port)})
            write_form(self.portal, EMAIL_PAGE, fields)

    def __enter__(self):
        return self

    def __exit__(self, *exc):
        try:
            self.t.shell.set_inputs({})
            self.restore.__exit__(*exc)
            if self.saved_email is not None:
                write_form(self.t.portal(), EMAIL_PAGE, self.saved_email)
        finally:
            self.sink.stop()
            self.mail.stop()
        return False

    def drive(self, label, level):
        self.inputs[label] = level
        self.t.shell.set_inputs(self.inputs)

    def channel(self, name):
        body = self.portal.get(EVENTS).body
        for select, inner in SELECT.findall(body):
            if select.startswith(("al", "anlt")):
                for value, label in OPTION.findall(inner):
                    if label == name:
                        return value
        raise Skip("this build offers no %s alert channel" % name)

    def rule(self, label, level=None, comparator=None, value=None, channel="HTTPSERVER"):
        """Adds a rule on an input pin and returns its slot."""
        analog = label.startswith("A")
        field = label.lower()
        modes = read_form(self.portal, MODES)
        if field not in modes:
            raise Skip("this board offers no %s" % label)
        modes[field] = AIN if analog else DIN
        write_form(self.portal, MODES, modes)

        body = self.portal.get(EVENTS).body
        slots_before = set(re.findall(r"name='%s(\d+)'" % ("aval" if analog else "d"), body))
        offered = options(body, "ifsl")
        if label not in offered:
            raise AssertionError("the event page offers no %s once it is an input: %r" % (label, offered))
        write_form(self.portal, EVENTS, {"ifsl": offered[label]})

        body = self.portal.get(EVENTS).body
        added = set(re.findall(r"name='%s(\d+)'" % ("aval" if analog else "d"), body)) - slots_before
        if 1 != len(added):
            raise AssertionError("adding a rule on %s made %r new slots" % (label, sorted(added)))
        slot = added.pop()

        chosen = self.channel(channel)
        if analog:
            write_form(self.portal, EVENTS, {"a" + slot: comparator, "aval" + slot: str(value), "anlt" + slot: chosen})
        else:
            write_form(self.portal, EVENTS, {"d" + slot: str(level), "al" + slot: chosen})
        return slot

    def remove(self, label, slot):
        prefix = "anlt" if label.startswith("A") else "al"
        fields = {prefix + slot: self.channel("DEL")}
        if label.startswith("A"):
            fields.update({"a" + slot: EQUAL, "aval" + slot: "0"})
        else:
            fields.update({"d" + slot: "0"})
        write_form(self.portal, EVENTS, fields)

    def posts(self):
        return self.sink.requests(EVENT_URL)

    def wait_post(self, after=0, timeout=FIRE_WAIT):
        return self.sink.wait_for(EVENT_URL, timeout, after=after)

    def quiet(self, after, watch=QUIET_WATCH):
        time.sleep(watch)
        return len(self.posts()) == after


@test("a rule on a digital input posts to the gpio server once the input meets it")
def digital_rule_posts(t):
    with Alerts(t) as alerts:
        alerts.drive("D4", 0)
        alerts.rule("D4", level=1)
        if not alerts.quiet(0):
            raise AssertionError("an alert went out before the input met the rule: %r" % alerts.posts())

        began = time.time()
        alerts.drive("D4", 1)
        post = alerts.wait_post()
        if post is None:
            raise AssertionError("no alert reached the gpio server within %.0f s of the input going high" % FIRE_WAIT)
        body = json.loads(post.text())
        if body.get("alrtkey") != "D4":
            raise AssertionError("the alert named %r, not D4: %r" % (body.get("alrtkey"), body))
        if not body.get("mac"):
            raise AssertionError("the alert carried no mac: %r" % body)
        if "POST" != post.method:
            raise AssertionError("the alert went as a %s" % post.method)
        if time.time() - began > FIRE_WAIT:
            raise AssertionError("the alert took %.1f s" % (time.time() - began))


@test("a rule on a digital input mails once the input meets it")
def digital_rule_mails(t):
    with Alerts(t) as alerts:
        if alerts.saved_email is None:
            raise Skip("this build serves no email page")
        alerts.drive("D4", 0)
        alerts.rule("D4", level=1, channel="MAIL")
        alerts.drive("D4", 1)

        mail = alerts.mail.wait_for_mail(FIRE_WAIT + 6.0)
        if mail is None:
            raise AssertionError("no alert mail arrived; the server saw %r" % alerts.mail.sessions)
        if "alrtkey" not in mail.body or "D4" not in mail.body:
            raise AssertionError("the alert mail did not name D4: %r" % mail.body)
        if mail.recipients != ["<%s>" % EMAIL_SETTINGS["ml_to"]]:
            raise AssertionError("the alert went to %r" % mail.recipients)


@test("a rule watching for low fires when the input goes low, not high")
def digital_rule_low(t):
    with Alerts(t) as alerts:
        alerts.drive("D4", 1)
        alerts.rule("D4", level=0)
        if not alerts.quiet(0):
            raise AssertionError("a rule for low fired on a high input")
        alerts.drive("D4", 0)
        if alerts.wait_post() is None:
            raise AssertionError("a rule for low did not fire when the input went low")


def comparator_case(t, comparator, threshold, quiet_levels, firing_level):
    with Alerts(t) as alerts:
        alerts.drive("A0", quiet_levels[0])
        alerts.rule("A0", comparator=comparator, value=threshold)
        for level in quiet_levels:
            alerts.drive("A0", level)
            if not alerts.quiet(0, watch=2.5):
                raise AssertionError("the rule fired at %d: %r" % (level, alerts.posts()))
        alerts.drive("A0", firing_level)
        post = alerts.wait_post()
        if post is None:
            raise AssertionError("the rule did not fire at %d" % firing_level)
        if json.loads(post.text()).get("alrtkey") != "A0":
            raise AssertionError("the alert named %r" % post.text())


@test("a greater-than rule fires only above its threshold")
def analog_greater(t):
    comparator_case(t, GREATER, 500, (499, 500), 501)


@test("a less-than rule fires only below its threshold")
def analog_less(t):
    comparator_case(t, LESS, 500, (501, 500), 499)


@test("an equal rule fires only at its value")
def analog_equal(t):
    comparator_case(t, EQUAL, 500, (499, 501), 500)


@test("a condition that holds alerts once, not on every tick")
def held_condition_alerts_once(t):
    with Alerts(t) as alerts:
        alerts.drive("D4", 0)
        alerts.rule("D4", level=1)
        alerts.drive("D4", 1)
        if alerts.wait_post() is None:
            raise AssertionError("the rule never fired")
        if not alerts.quiet(1, watch=8.0):
            raise AssertionError("a held condition alerted %d times in 8 s" % len(alerts.posts()))


@test("a condition that clears and returns alerts again")
def condition_rearms(t):
    with Alerts(t) as alerts:
        alerts.drive("D4", 0)
        alerts.rule("D4", level=1)
        alerts.drive("D4", 1)
        if alerts.wait_post() is None:
            raise AssertionError("the rule never fired")
        alerts.drive("D4", 0)
        time.sleep(2.5)
        alerts.drive("D4", 1)
        if alerts.wait_post(after=1) is None:
            raise AssertionError("the rule did not fire again after its condition cleared and returned")


@test("an alert the server refuses is not resent on every tick")
def refused_alert_not_hammered(t):
    with Alerts(t) as alerts:
        alerts.sink.status = 500
        alerts.drive("D4", 0)
        alerts.rule("D4", level=1)
        alerts.drive("D4", 1)
        if alerts.wait_post() is None:
            raise AssertionError("the rule never fired")
        if not alerts.quiet(1, watch=8.0):
            raise AssertionError("a refused alert was resent %d times in 8 s" % (len(alerts.posts()) - 1))


@test("an alert the server refused is retried after about two minutes", slow=True)
def refused_alert_retried(t):
    with Alerts(t) as alerts:
        alerts.sink.status = 500
        alerts.drive("D4", 0)
        alerts.rule("D4", level=1)
        alerts.drive("D4", 1)
        first = alerts.wait_post()
        if first is None:
            raise AssertionError("the rule never fired")
        alerts.sink.status = 200
        second = alerts.wait_post(after=1, timeout=150.0)
        if second is None:
            raise AssertionError("a refused alert was never retried in 150 s")
        gap = second.at - first.at
        if not 100.0 <= gap <= 140.0:
            raise AssertionError("a refused alert was retried after %.0f s, not about 120" % gap)


@test("a deleted rule sends nothing")
def deleted_rule_is_silent(t):
    with Alerts(t) as alerts:
        alerts.drive("D4", 0)
        slot = alerts.rule("D4", level=1)
        alerts.remove("D4", slot)
        alerts.drive("D4", 1)
        if not alerts.quiet(0):
            raise AssertionError("a deleted rule still alerted: %r" % alerts.posts())


@test("switching a pin off clears its rules")
def off_clears_rules(t):
    with Alerts(t) as alerts:
        alerts.drive("D4", 0)
        alerts.rule("D4", level=1)
        modes = read_form(alerts.portal, MODES)
        modes["d4"] = "0"
        write_form(alerts.portal, MODES, modes)
        modes["d4"] = DIN
        write_form(alerts.portal, MODES, modes)
        alerts.drive("D4", 1)
        if not alerts.quiet(0):
            raise AssertionError("a rule outlived its pin being switched off: %r" % alerts.posts())


@test("two rules on two pins fire on their own pins only")
def two_rules_independent(t):
    with Alerts(t) as alerts:
        alerts.drive("D4", 0)
        alerts.drive("D5", 0)
        alerts.rule("D4", level=1)
        alerts.rule("D5", level=1)
        alerts.drive("D5", 1)
        post = alerts.wait_post()
        if post is None or json.loads(post.text()).get("alrtkey") != "D5":
            raise AssertionError("D5 going high alerted %r" % (post and post.text()))
        if not alerts.quiet(1):
            raise AssertionError("D4 alerted without going high: %r" % alerts.posts())


@test("a rule outlives a restart of the gpio service", needs=("service",))
def rule_survives_restart(t):
    with Alerts(t) as alerts:
        alerts.drive("D4", 0)
        alerts.rule("D4", level=1)
        out = t.run("service restart GPIO")
        if "restarted GPIO" not in out:
            raise AssertionError("the gpio service did not restart: %r" % out)
        time.sleep(2.0)
        alerts.drive("D4", 1)
        if alerts.wait_post(timeout=FIRE_WAIT + 2.0) is None:
            raise AssertionError("the rule did not fire after the gpio service restarted")


@test("an alert to a mail server that hangs does not take the board away for long")
def hanging_mail_alert(t):
    with Alerts(t) as alerts:
        if alerts.saved_email is None:
            raise Skip("this build serves no email page")
        alerts.mail.hang.add("greeting")
        alerts.drive("D4", 0)
        alerts.rule("D4", level=1, channel="MAIL")
        alerts.drive("D4", 1)
        time.sleep(1.5)

        worst = 0.0
        deadline = time.time() + 12.0
        while time.time() < deadline:
            began = time.time()
            status = t.portal(login=False).get("/login", attempts=1).status
            worst = max(worst, time.time() - began)
            if 200 != status:
                raise AssertionError("the portal answered %d while an alert hung" % status)
        if worst > 8.0:
            raise AssertionError("the portal took %.1f s to answer while an alert's mail server hung" % worst)
