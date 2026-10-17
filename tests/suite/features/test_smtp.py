#!/usr/bin/env python3

"""
The email service sending real mail, to a mail server of the suite's own.

The board is the client, so the server here is scripted: it answers each
command as a mail server does, keeps every byte it was sent and each message
as it would be delivered, and can refuse any step or hang at it. The email page
sends a test mail when it is saved with its test box ticked, which is the one
send an operator can trigger, so every test drives that. The page's own
settings are put back afterwards.
"""

import time

from .registry import test, Skip
from .test_mqtt import read_form, write_form
from ..driver.smtp_server import SmtpServer
from ..driver.mdns import local_address_towards

PAGE = "/email-config"
TEST_MESSAGE = "this is test mail from pdistack"

SETTINGS = {
    "ml_dmn": "device.test",
    "ml_usr": "user@device.test",
    "ml_psw": "s3cret",
    "ml_frm": "user@device.test",
    "ml_frnm": "Device",
    "ml_to": "ops@device.test",
    "ml_sub": "Alarm",
}

# a step the client waits on has a five second timeout of its own, plus connect
SEND_WAIT = 20.0


class MailServer(object):
    """A scripted server the email page points at, put back the way it was after."""

    def __init__(self, t):
        self.t = t
        self.portal = t.portal()
        if 404 == self.portal.get(PAGE).status:
            raise Skip("this build serves no email page")
        self.saved = read_form(self.portal, PAGE)
        self.server = SmtpServer().start()
        self.address = local_address_towards(t.address())

    def __enter__(self):
        return self

    def __exit__(self, *exc):
        try:
            write_form(self.t.portal(), PAGE, self.saved)
        finally:
            self.server.stop()
        return False

    def send_test_mail(self, **overrides):
        fields = dict(SETTINGS)
        fields.update({"ml_srvr": self.address, "ml_prt": str(self.server.port), "tstml": "test"})
        fields.update(overrides)
        before = len(self.server.sessions)
        write_form(self.t.portal(), PAGE, fields)
        return before

    def conversation(self, after):
        session = self.server.wait_for_session(SEND_WAIT, after=after)
        if session is None:
            raise AssertionError("the board never talked to the mail server at %s:%d — it has to "
                                 "reach this machine" % (self.address, self.server.port))
        return session


@test("a test mail from the email page arrives with the saved addresses and subject")
def test_mail_arrives(t):
    with MailServer(t) as mail:
        mail.send_test_mail()
        got = mail.server.wait_for_mail(SEND_WAIT)
        if got is None:
            raise AssertionError("no mail arrived; the conversation was %r" % mail.server.sessions)

        if got.sender != "<user@device.test>":
            raise AssertionError("the sender was %r" % got.sender)
        if got.recipients != ["<ops@device.test>"]:
            raise AssertionError("the recipients were %r" % got.recipients)
        if got.header("Subject") != "Alarm":
            raise AssertionError("the subject was %r in %r" % (got.header("Subject"), got.lines[:6]))
        if "Device" not in (got.header("From") or ""):
            raise AssertionError("the from header was %r" % got.header("From"))
        if got.header("To") != "ops@device.test":
            raise AssertionError("the to header was %r" % got.header("To"))
        if TEST_MESSAGE not in got.body:
            raise AssertionError("the body was %r" % got.body)


@test("the board greets with the saved domain and logs in with the saved credentials")
def greeting_and_login(t):
    with MailServer(t) as mail:
        mail.send_test_mail()
        got = mail.server.wait_for_mail(SEND_WAIT)
        if got is None:
            raise AssertionError("no mail arrived")
        if got.helo != "device.test":
            raise AssertionError("the board greeted as %r" % got.helo)
        if (got.username, got.password) != ("user@device.test", "s3cret"):
            raise AssertionError("the board logged in as %r / %r" % (got.username, got.password))


@test("every command and line the board sends ends in CRLF, with no space after a colon")
def wire_format(t):
    with MailServer(t) as mail:
        after = mail.send_test_mail()
        session = mail.conversation(after)
        raw = mail.server.raw

        for at, byte in enumerate(raw):
            if 0x0A == byte and (0 == at or 0x0D != raw[at - 1]):
                raise AssertionError("a bare line feed at byte %d: %r" % (at, raw[max(0, at - 30):at + 5]))
        for command in session["commands"]:
            if command.upper().startswith(("MAIL FROM", "RCPT TO")) and ": " in command:
                raise AssertionError("%r has a space after its colon" % command)
        if "QUIT" not in [c.upper() for c in session["commands"]]:
            raise AssertionError("the board did not end with QUIT: %r" % session["commands"])


@test("saving the email page without the test box sends no mail")
def save_without_test_sends_nothing(t):
    with MailServer(t) as mail:
        mail.send_test_mail(tstml="")
        time.sleep(6.0)
        if mail.server.sessions:
            raise AssertionError("a save without the test box opened %d conversations" % len(mail.server.sessions))


@test("a refused login stops the send before the message")
def refused_login(t):
    with MailServer(t) as mail:
        mail.server.replies["PASS"] = "535 5.7.8 credentials invalid"
        after = mail.send_test_mail()
        session = mail.conversation(after)
        if any(c.upper().startswith("MAIL") for c in session["commands"]):
            raise AssertionError("the board went on after a refused login: %r" % session["commands"])
        if mail.server.mails:
            raise AssertionError("a mail was delivered after a refused login")


@test("a refused recipient stops the send before the message")
def refused_recipient(t):
    with MailServer(t) as mail:
        mail.server.replies["RCPT"] = "550 5.1.1 no such user"
        after = mail.send_test_mail()
        session = mail.conversation(after)
        if "DATA" in [c.upper() for c in session["commands"]]:
            raise AssertionError("the board sent DATA after a refused recipient: %r" % session["commands"])


@test("a server that does not greet ready is not talked to")
def refused_greeting(t):
    with MailServer(t) as mail:
        mail.server.replies["greeting"] = "554 no service"
        after = mail.send_test_mail()
        session = mail.conversation(after)
        if session["commands"]:
            raise AssertionError("the board talked to a server that refused it: %r" % session["commands"])


@test("a long multi-line greeting reply is read whole")
def long_ehlo_reply(t):
    with MailServer(t) as mail:
        lines = ["250-extension %02d that the server advertises at some length" % i for i in range(20)]
        mail.server.replies["EHLO"] = "\r\n".join(lines + ["250 AUTH LOGIN"])
        mail.send_test_mail()
        if mail.server.wait_for_mail(SEND_WAIT) is None:
            raise AssertionError("no mail arrived after a %d line greeting reply" % (len(lines) + 1))


def answers_while(t, mail, what, budget):
    began = time.time()
    status = None
    while time.time() - began < 30.0:
        try:
            status = t.portal(login=False).get("/login", attempts=1).status
            break
        except Exception:
            time.sleep(0.5)
    took = time.time() - began
    if 200 != status:
        raise AssertionError("the board stopped answering while %s" % what)
    if took > budget:
        raise AssertionError("the board was unreachable for %.0f s while %s" % (took, what))


@test("a mail server that never greets times the send out and the board keeps answering")
def hang_at_greeting(t):
    with MailServer(t) as mail:
        mail.server.hang.add("greeting")
        mail.send_test_mail()
        answers_while(t, mail, "the mail server never greeted", 15.0)


@test("a mail server that hangs after the message times the send out and the board keeps answering")
def hang_at_end(t):
    with MailServer(t) as mail:
        mail.server.hang.add("END")
        mail.send_test_mail()
        answers_while(t, mail, "the mail server hung after the message", 15.0)


@test("two test mails in a row both arrive")
def two_mails(t):
    with MailServer(t) as mail:
        mail.send_test_mail()
        if mail.server.wait_for_mail(SEND_WAIT) is None:
            raise AssertionError("the first mail never arrived")
        mail.send_test_mail()
        if mail.server.wait_for_mail(SEND_WAIT, after=1) is None:
            raise AssertionError("the second mail never arrived")
