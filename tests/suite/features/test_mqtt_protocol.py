#!/usr/bin/env python3

"""
The mqtt client against a broker that misbehaves on cue.

`test_mqtt.py` checks what the client says to a broker that answers well. This
suite makes the broker refuse the connection, withhold an acknowledgement, fall
silent, restart, and send packets that are oversized or malformed — and checks
the client's answer to each against MQTT 3.1.1: it acknowledges what it must,
retries at a steady pace rather than in a loop, notices a dead broker, and is
still applying commands afterwards. Every test leaves the broker answering and
the client connected again, so the suites after it start from a working link.
"""

import json
import time

from .registry import test, Skip
from .test_mqtt import connected, SUB_FIRST, PUBLISH_WAIT, KEEPALIVE
from .test_gpio import gpio_portal, Restore
from .test_gpio_live import pick_pins, seen_within, TICK, DOUT

PUBACK, PUBREC, PUBCOMP = 4, 5, 7
RECONNECT_WAIT = 60.0
BUFFER = 640


def subscribed(t):
    state, session = connected(t)
    if state.broker.wait_for_subscription(SUB_FIRST, PUBLISH_WAIT) is None:
        raise Skip("the client is not subscribed to %s yet" % SUB_FIRST)
    return state


def back_to_normal(state):
    """Puts the broker back to answering and waits for the client to be subscribed again."""
    broker = state.broker
    broker.connack_code = 0
    broker.ack_publishes = True
    broker.silent = False
    if broker.session is None or broker.wait_for_subscription(SUB_FIRST, RECONNECT_WAIT) is None:
        raise AssertionError("the client did not come back to a broker that answers again")


def command_applies(t, state, value):
    """A gpio command from the broker still reaches a pin, which is what a working client does."""
    portal = gpio_portal(t)
    with Restore(portal):
        pin = pick_pins(portal, 1)[0]
        label = "D%d" % pin
        state.broker.publish(SUB_FIRST, json.dumps({"data": {label: {"mode": int(DOUT), "val": value}}}))
        seen_within(portal, label, int(DOUT), value, timeout=TICK + 3.0)


@test("a qos 1 message from the broker is acknowledged with its own packet id",
      needs=("service",), services=("MQTT",), slow=True)
def inbound_qos1_acked(t):
    state = subscribed(t)
    state.broker.publish(SUB_FIRST, "{}", qos=1, packet_id=4321)
    deadline = time.time() + 6.0
    while time.time() < deadline:
        if 4321 in state.broker.acks_of(PUBACK):
            return
        time.sleep(0.2)
    raise AssertionError("no PUBACK for packet 4321; the client acknowledged %r" % state.broker.acks)


@test("a qos 2 message from the broker gets its PUBREC and then its PUBCOMP",
      needs=("service",), services=("MQTT",), slow=True)
def inbound_qos2_handshake(t):
    state = subscribed(t)
    state.broker.publish(SUB_FIRST, "{}", qos=2, packet_id=777)
    deadline = time.time() + 8.0
    while time.time() < deadline:
        if 777 in state.broker.acks_of(PUBREC) and 777 in state.broker.acks_of(PUBCOMP):
            return
        time.sleep(0.2)
    raise AssertionError("the qos 2 handshake for packet 777 did not complete: %r" % state.broker.acks)


@test("an oversized message from the broker does not garble the one after it",
      needs=("service",), services=("MQTT",), slow=True)
def oversized_inbound_message(t):
    state = subscribed(t)
    state.broker.publish(SUB_FIRST, "x" * (BUFFER * 3))
    command_applies(t, state, 1)
    command_applies(t, state, 0)


@test("a malformed packet from the broker leaves the client working",
      needs=("service",), services=("MQTT",), slow=True)
def malformed_inbound_packets(t):
    state = subscribed(t)
    for raw in (bytes([0x30, 0x04, 0xFF, 0xFF, 0x61, 0x62]),
                bytes([0xF0, 0x00]),
                bytes([0x00, 0x00]),
                bytes([0x30, 0xFF, 0xFF, 0xFF, 0xFF, 0x01])):
        state.broker.send_raw(raw)
        time.sleep(1.5)
    back_to_normal(state)
    command_applies(t, state, 1)


@test("a refused connection is retried at a steady pace, never in a tight loop",
      needs=("service",), services=("MQTT",), slow=True)
def refused_connection_backs_off(t):
    state = subscribed(t)
    broker = state.broker
    try:
        broker.connack_code = 5
        since = broker.connect_count()
        before = broker.message_count()
        broker.drop_client()
        time.sleep(25.0)
        attempts = broker.connect_count() - since
        if attempts < 2:
            raise AssertionError("a refused client tried %d times in 25 s — it stopped trying" % attempts)
        if attempts > 8:
            raise AssertionError("a refused client tried %d times in 25 s — that is a loop" % attempts)
        if broker.message_count() != before:
            raise AssertionError("the client published %d messages on a refused connection"
                                 % (broker.message_count() - before))
    finally:
        back_to_normal(state)


@test("a publish the broker never acknowledges ends the session instead of waiting forever",
      needs=("service",), services=("MQTT",), slow=True)
def missing_puback_reconnects(t):
    state = subscribed(t)
    broker = state.broker
    try:
        broker.ack_publishes = False
        since = broker.connect_count()
        if broker.wait_for_reconnect(since, timeout=PUBLISH_WAIT + 30.0) is None:
            raise AssertionError("the client waited on an unacknowledged qos 1 publish and never gave up")
    finally:
        back_to_normal(state)


@test("a broker that stops answering is noticed within the keepalive and reconnected to",
      needs=("service",), services=("MQTT",), slow=True)
def silent_broker_detected(t):
    state = subscribed(t)
    broker = state.broker
    try:
        since = broker.connect_count()
        began = time.time()
        broker.silent = True
        if broker.wait_for_reconnect(since, timeout=KEEPALIVE * 2 + 15) is None:
            raise AssertionError("the client never noticed a broker that stopped answering")
        took = time.time() - began
        if took > KEEPALIVE * 1.5 + 15:
            raise AssertionError("the client took %.0f s to notice a silent broker on a %d s keepalive"
                                 % (took, KEEPALIVE))
    finally:
        back_to_normal(state)


@test("a broker that restarts is reconnected to and subscribed to again",
      needs=("service",), services=("MQTT",), slow=True)
def broker_restart(t):
    state = subscribed(t)
    broker = state.broker
    since = broker.connect_count()
    broker.stop()
    time.sleep(3.0)
    broker.start()
    if broker.wait_for_reconnect(since, timeout=RECONNECT_WAIT) is None:
        raise AssertionError("the client never came back after the broker restarted")
    if broker.wait_for_subscription(SUB_FIRST, PUBLISH_WAIT) is None:
        raise AssertionError("the client came back but did not subscribe to %s again" % SUB_FIRST)
    command_applies(t, state, 1)
