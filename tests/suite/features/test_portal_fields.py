#!/usr/bin/env python3

"""
Every text field the config pages save lands in a fixed buffer on the board.

A field the buffer cannot hold, with its terminator, must be refused and change
nothing: the pages used to copy as many bytes as the browser sent, which ran a
long value past its buffer into whatever the struct kept beside it — on one page
that was the gpio service's blinker pointers, and the next tick dereferenced
them. Each test posts the page back exactly as it read it, with one field made
too long, and expects the page to read back unchanged and the board to still be
answering.

The refusals change nothing, so they are safe on a board. The boundary tests,
which save a value one short of full, and anything on the wifi page, which
would take a board off the network, run only against the host process.
"""

from .registry import test, Skip
from .test_mqtt import read_form, write_form, GENERAL, LWT, PUBSUB
from ..driver.host_shell import HostShell

# past every buffer these pages copy into; the largest is the 500 byte mqtt password
OVERSIZE = "x" * 600

OTA = "/ota-config"
EMAIL = "/email-config"
WIFI = "/wifi-config"
GPIO_SERVER = "/gpio-server"
IOT = "/device-register-config"

WIFI_TEXT = 30
EMAIL_TEXT = 50
IP_TEXT = 20
IOT_ID_TEXT = 100
IOT_HOST_TEXT = 50


def page_or_skip(portal, path):
    if 404 == portal.get(path).status:
        raise Skip("%s is not in this build" % path)


def on_host(t):
    return isinstance(t.shell, HostShell)


def refuses_each(t, path, names):
    """Posts the page with each named field made too long, one at a time."""
    portal = t.portal()
    page_or_skip(portal, path)
    before = read_form(portal, path)

    for name in names:
        if name not in before:
            raise AssertionError("%s carries no %r field: %r" % (path, name, sorted(before)))

        fields = dict(before)
        fields[name] = OVERSIZE
        write_form(portal, path, fields)

        after = read_form(portal, path)
        if after != before:
            changed = dict((k, after.get(k)) for k in before if after.get(k) != before.get(k))
            raise AssertionError("a %d byte %s on %s was taken: %r" % (len(OVERSIZE), name, path, changed))

        if 200 != portal.get(path).status:
            raise AssertionError("the board stopped answering after a long %s on %s" % (name, path))


def boundary(t, path, name, width):
    """
    A value one short of the buffer is saved whole; one that fills it, leaving
    no room for the terminator, is refused.
    """
    if not on_host(t):
        raise Skip("saves a setting, so it runs against the host process only")

    # a saved wifi page ends the browser's session, so every read after a save
    # logs in again rather than reusing the cookie the save retired
    portal = t.portal()
    page_or_skip(portal, path)
    before = read_form(portal, path)
    try:
        fits = dict(before)
        fits[name] = "f" * (width - 1)
        write_form(portal, path, fits)
        saved = read_form(t.portal(), path).get(name)
        if saved != fits[name]:
            raise AssertionError("a %d byte %s came back %r" % (width - 1, name, saved))

        full = dict(fits)
        full[name] = "g" * width
        write_form(t.portal(), path, full)
        kept = read_form(t.portal(), path).get(name)
        if kept != fits[name]:
            raise AssertionError("a %d byte %s, with no room for its terminator, came back %r"
                                 % (width, name, kept))
    finally:
        write_form(t.portal(), path, before)


@test("the ota page refuses a host it cannot hold")
def ota_refuses_long_fields(t):
    refuses_each(t, OTA, ("hst",))


@test("the mqtt connection page refuses a field it cannot hold")
def mqtt_general_refuses_long_fields(t):
    refuses_each(t, GENERAL, ("hst", "clid", "usrn", "pswd"))


@test("the mqtt will page refuses a field it cannot hold")
def mqtt_lwt_refuses_long_fields(t):
    refuses_each(t, LWT, ("wtpc", "wmsg"))


@test("the mqtt topics page refuses a topic it cannot hold")
def mqtt_pubsub_refuses_long_fields(t):
    refuses_each(t, PUBSUB, ("ptpc0", "stpc0"))


@test("the email page refuses a field it cannot hold")
def email_refuses_long_fields(t):
    refuses_each(t, EMAIL, ("ml_dmn", "ml_srvr", "ml_usr", "ml_psw", "ml_frm", "ml_frnm", "ml_to", "ml_sub"))


@test("the gpio server page refuses a host it cannot hold")
def gpio_server_refuses_long_fields(t):
    refuses_each(t, GPIO_SERVER, ("hst",))


@test("the wifi page refuses a name, password or address it cannot hold")
def wifi_refuses_long_fields(t):
    if not on_host(t):
        raise Skip("the wifi page takes a board off the network, so it runs against the host process only")
    refuses_each(t, WIFI, ("sta_ssid", "sta_pswd", "sta_lip", "sta_gip", "sta_sip",
                           "ap_ssid", "ap_pswd", "ap_lip", "ap_gip", "ap_sip"))


def wifi_refusal_says(t, field, value, expect):
    """Posts the wifi page with one field changed and checks the reply names the limits."""
    if not on_host(t):
        raise Skip("the wifi page takes a board off the network, so it runs against the host process only")
    portal = t.portal()
    page_or_skip(portal, WIFI)
    before = read_form(portal, WIFI)
    fields = dict(before)
    fields[field] = value
    body = write_form(portal, WIFI, fields).body
    if "Config saved" in body:
        raise AssertionError("a %d byte %s was reported saved" % (len(value), field))
    for text in expect:
        if text not in body:
            raise AssertionError("the refusal of a %d byte %s does not say %r" % (len(value), field, text))
    if read_form(t.portal(), WIFI) != before:
        raise AssertionError("a refused %s changed the page" % field)


WIFI_LIMITS = ("Not saved", "4 to %d characters" % (WIFI_TEXT - 1), "password up to %d" % (WIFI_TEXT - 1),
               "address up to %d" % (IP_TEXT - 1))


@test("a wifi name too long for its buffer is refused with the limits in the reply")
def wifi_long_name_says_limits(t):
    wifi_refusal_says(t, "sta_ssid", "n" * WIFI_TEXT, WIFI_LIMITS)


@test("a wifi name shorter than the least it takes is refused with the limits in the reply")
def wifi_short_name_says_limits(t):
    wifi_refusal_says(t, "sta_ssid", "abc", WIFI_LIMITS)


@test("a wifi password too long for its buffer is refused with the limits in the reply")
def wifi_long_password_says_limits(t):
    wifi_refusal_says(t, "sta_pswd", "p" * WIFI_TEXT, WIFI_LIMITS)


@test("an access point name far past its buffer is refused with the limits in the reply")
def wifi_long_ap_name_says_limits(t):
    wifi_refusal_says(t, "ap_ssid", OVERSIZE, WIFI_LIMITS)


@test("a wifi address too long for its buffer is refused with the limits in the reply")
def wifi_long_address_says_limits(t):
    wifi_refusal_says(t, "sta_gip", "1" * IP_TEXT, WIFI_LIMITS)


@test("a wifi name that fills its buffer is refused and one short of it is saved")
def wifi_name_boundary(t):
    boundary(t, WIFI, "sta_ssid", WIFI_TEXT)


@test("an email field that fills its buffer is refused and one short of it is saved")
def email_field_boundary(t):
    boundary(t, EMAIL, "ml_dmn", EMAIL_TEXT)


@test("the device registration page refuses an id or host it cannot hold")
def iot_refuses_long_fields(t):
    portal = t.portal()
    page_or_skip(portal, IOT)

    answer = portal.post(IOT, {"csrf": portal.csrf(IOT), "duid": OVERSIZE, "dhst": "iot.example.com"})
    if "too long" not in answer.body:
        raise AssertionError("a %d byte device id was not refused: %r" % (len(OVERSIZE), answer.body[:200]))

    answer = portal.post(IOT, {"csrf": portal.csrf(IOT), "duid": "id", "dhst": OVERSIZE})
    if "too long" not in answer.body:
        raise AssertionError("a %d byte host was not refused: %r" % (len(OVERSIZE), answer.body[:200]))


def iot_refusal_says(t, duid, host, expect, unexpected):
    portal = t.portal()
    page_or_skip(portal, IOT)
    before = read_form(portal, IOT)
    body = portal.post(IOT, {"csrf": portal.csrf(IOT), "duid": duid, "dhst": host}).body
    if '"status":false' not in body:
        raise AssertionError("a %d byte id and %d byte host were not refused: %r" % (len(duid), len(host), body[:200]))
    if expect not in body:
        raise AssertionError("the refusal does not say %r: %r" % (expect, body[:200]))
    if unexpected in body:
        raise AssertionError("the refusal names the field that fits, %r: %r" % (unexpected, body[:200]))
    if read_form(t.portal(), IOT) != before:
        raise AssertionError("a refused registration changed the page")


IOT_ID_LIMIT = "Device id is too long: up to %d characters" % (IOT_ID_TEXT - 1)
IOT_HOST_LIMIT = "Registry host is too long: up to %d characters" % (IOT_HOST_TEXT - 1)


@test("a device id that fills its buffer is refused with its limit in the reply")
def iot_full_id_says_limit(t):
    iot_refusal_says(t, "i" * IOT_ID_TEXT, "iot.example.com", IOT_ID_LIMIT, "Registry host")


@test("a registry host that fills its buffer is refused with its limit in the reply")
def iot_full_host_says_limit(t):
    iot_refusal_says(t, "id", "h" * IOT_HOST_TEXT, IOT_HOST_LIMIT, "Device id")


@test("a device id and host both too long are refused naming the id first")
def iot_both_long_says_id_limit(t):
    iot_refusal_says(t, OVERSIZE, OVERSIZE, IOT_ID_LIMIT, "Registry host")
