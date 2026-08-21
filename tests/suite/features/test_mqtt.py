#!/usr/bin/env python3

"""
The mqtt client, tested by being the broker it talks to.

A broker of our own is stood up on a free port and the board is pointed at it
through the web portal, so every assertion is about what actually went out on
the wire: the CONNECT flags, the will, the topics, the qos, the keepalive.
Reading the config back from the portal would only prove the portal stored it.

The machine running these tests usually already has a broker on 1883 for real
work. Pointing the board at that one would put test traffic on it and leave
retained messages behind, so this binds an ephemeral port instead and takes it
away again at the end.

The board's mqtt config lives in its database and outlives the run, so it is
read first and written back afterwards. Setting it up costs a reconnect and
five seconds of settling, so the whole suite shares one broker and one
connection, set up on the first test that needs it and restored once at the
end through `Target.at_exit`. The tests that have to disturb that connection —
dropping it, changing the topics — come last on purpose.
"""

import re
import time

from .registry import test, expect_in, Skip
from ..driver.mdns import local_address_towards
from ..driver.mqtt_broker import MqttBroker

GENERAL = "/mqtt-general-config"
LWT = "/mqtt-lwt-config"
PUBSUB = "/mqtt-pubsub-config"

CLIENT_ID = "pditest"
WILL_TOPIC = "pditest/will"
WILL_MESSAGE = "pditest-gone"
PUB_FIRST = "pditest/pub0"
PUB_SECOND = "pditest/pub1"
SUB_FIRST = "pditest/sub0"
SUB_SECOND = "pditest/sub1"
PUBLISH_EVERY = 5
KEEPALIVE = 30

# the client waits five seconds before reconnecting after a config change, and
# gives the tcp connect five more
CONNECT_WAIT = 90.0
PUBLISH_WAIT = 60.0

TAG = re.compile(r"<(/?)(input|select|option)\b([^>]*)>", re.I)
ATTR = re.compile(r"([\w-]+)\s*=\s*'([^']*)'")


def read_form(portal, path):
    """
    What a browser would submit from a page, as it stands now.

    Checkboxes count only when checked and selects contribute the option that
    is selected, so feeding this straight back to the same path restores the
    page exactly. The csrf token is left out: it is per-session and the caller
    fetches a fresh one.
    """
    body = portal.get(path).body
    fields = {}
    select = None

    for closing, kind, attributes in TAG.findall(body):
        attrs = dict(ATTR.findall(attributes))
        kind = kind.lower()

        if "select" == kind:
            select = None if closing else attrs.get("name")
        elif "option" == kind:
            if select and "selected" in attrs:
                fields[select] = attrs.get("value", "")
        elif "input" == kind:
            name = attrs.get("name")
            if not name or "csrf" == name:
                continue
            if "checkbox" == attrs.get("type", "").lower():
                if "checked" in attrs:
                    fields[name] = attrs.get("value", "on")
            else:
                fields[name] = attrs.get("value", "")

    return fields


def pubsub_fields(publish_every=PUBLISH_EVERY, second_subscription=SUB_SECOND):
    """The pub/sub page as this suite wants it, with the two knobs tests move."""
    return {
        "ptpc0": PUB_FIRST, "pqos0": "1", "prtn0": "retain",
        "ptpc1": PUB_SECOND, "pqos1": "0",
        "pfrq": str(publish_every),
        "stpc0": SUB_FIRST, "sqos0": "1",
        "stpc1": second_subscription, "sqos1": "0",
    }


def wait_for_traffic(broker, timeout):
    """
    Any sign of life from the client: a publish or a keepalive.

    Which of the two it is depends on the build and on whether the client has
    anything to say, and neither makes it more alive than the other — so a test
    about liveness must not insist on one particular packet.
    """
    messages = broker.message_count()
    pings = broker.ping_count()

    deadline = time.time() + timeout
    while time.time() < deadline:
        if broker.message_count() > messages or broker.ping_count() > pings:
            return True
        time.sleep(0.5)

    return False


def portal_for(target, attempts=3):
    """
    A logged in portal, retried.

    The board keeps a couple of web sessions and a run that ended badly can
    leave one behind until it expires, so the first login of a run is
    occasionally refused. That is setup rather than an assertion — the portal
    suite is what tests logging in — so it is retried instead of failing the
    mqtt test that happened to go first.
    """
    last = None

    for attempt in range(attempts):
        try:
            return target.portal()
        except Exception as err:
            last = err
            if attempt + 1 < attempts:
                time.sleep(5.0)

    raise Skip("could not log in to the portal to configure mqtt: %s" % last)


def write_form(portal, path, fields):
    body = dict(fields)
    body["csrf"] = portal.csrf(path)
    return portal.post(path, body)


class Fixture(object):
    """One broker, one configured connection, shared by the whole suite."""

    def __init__(self):
        self.broker = None
        self.saved = {}
        self.address = ""
        self.error = None

    def restore(self, target):
        """Put the board's config back and take the broker away."""
        try:
            if self.saved:
                portal = portal_for(target)
                for path in (PUBSUB, LWT, GENERAL):
                    if path in self.saved:
                        write_form(portal, path, self.saved[path])
        finally:
            if self.broker is not None:
                self.broker.stop()
                self.broker = None


_fixture = None


def build(target):
    """Stand up the broker, point the board at it, wait for it to arrive."""
    state = Fixture()

    portal = portal_for(target)
    for path in (GENERAL, LWT, PUBSUB):
        state.saved[path] = read_form(portal, path)

    if "hst" not in state.saved.get(GENERAL, {}):
        state.error = "this build serves no mqtt config page"
        return state

    state.address = local_address_towards(target.address())
    state.broker = MqttBroker("0.0.0.0", 0).start()

    # restore even if the configuration below fails half way through
    target.at_exit(lambda: state.restore(target))

    write_form(portal, PUBSUB, pubsub_fields())
    write_form(portal, LWT, {
        "wtpc": WILL_TOPIC, "wmsg": WILL_MESSAGE, "wqos": "1", "wrtn": "retain",
    })
    # general last: it is the one that reconnects the client
    write_form(portal, GENERAL, {
        "hst": state.address, "prt": str(state.broker.port),
        "clid": CLIENT_ID, "usrn": "pdiuser", "pswd": "pdipass",
        "kpalv": str(KEEPALIVE), "cln": "clean",
    })

    if state.broker.wait_for_connect(CONNECT_WAIT) is None:
        state.error = ("the board never reached the test broker at %s:%d — it "
                       "has to be able to open a tcp connection back to this "
                       "machine, which a firewall or an access point with "
                       "client isolation will stop"
                       % (state.address, state.broker.port))

    return state


def fixture(target):
    global _fixture

    if _fixture is None:
        _fixture = build(target)

    if _fixture.error:
        raise Skip(_fixture.error)

    return _fixture


def connected(target):
    """The fixture, with the client currently connected."""
    state = fixture(target)

    session = state.broker.session
    if session is None:
        raise Skip("the client is not connected to the test broker")

    return state, session


@test("the mqtt service is running", needs=("srvc",), services=("MQTT",))
def service_is_running(t):
    out = t.run("srvc status MQTT", timeout=max(t.timeout, 30))
    expect_in("active", out, "the mqtt service state")


@test("the board connects to the broker it was configured with",
      needs=("srvc",), services=("MQTT",), slow=True)
def connects_to_configured_broker(t):
    _, session = connected(t)

    if session.client_id != CLIENT_ID:
        raise AssertionError("the client announced itself as %r, not the "
                             "configured %r" % (session.client_id, CLIENT_ID))


@test("the keepalive and clean session flags are the configured ones",
      needs=("srvc",), services=("MQTT",), slow=True)
def connect_flags_match_config(t):
    _, session = connected(t)

    if session.keepalive != KEEPALIVE:
        raise AssertionError("the connect asked for a keepalive of %d, not the "
                             "configured %d" % (session.keepalive, KEEPALIVE))

    if not session.clean_session:
        raise AssertionError("clean session was configured but the connect did "
                             "not set the flag")


@test("the configured credentials are sent at connect",
      needs=("srvc",), services=("MQTT",), slow=True)
def credentials_are_sent(t):
    _, session = connected(t)

    if "pdiuser" != session.username:
        raise AssertionError("the connect carried username %r, not the "
                             "configured one" % session.username)

    if "pdipass" != session.password:
        raise AssertionError("the connect carried password %r, not the "
                             "configured one" % session.password)


@test("the last will is registered with the broker at connect",
      needs=("srvc",), services=("MQTT",), slow=True)
def will_is_registered(t):
    """
    A will only works if the broker is told about it while the client is still
    healthy. Nothing the client does later can make up for leaving it out here.
    """
    _, session = connected(t)

    if session.will is None:
        raise AssertionError("a will was configured but the connect carried none")

    if session.will.topic != WILL_TOPIC:
        raise AssertionError("the will is registered on %r, not the configured %r"
                             % (session.will.topic, WILL_TOPIC))

    expect_in(WILL_MESSAGE, session.will.text, "the will message")

    if 1 != session.will.qos:
        raise AssertionError("the will was registered at qos %d, not the "
                             "configured 1" % session.will.qos)

    if not session.will.retain:
        raise AssertionError("the will was configured to be retained but was "
                             "not registered that way")


@test("the board publishes on its configured topic",
      needs=("srvc",), services=("MQTT",), slow=True)
def publishes_on_configured_topic(t):
    state, _ = connected(t)

    message = state.broker.wait_for_message(PUBLISH_WAIT, topic=PUB_FIRST)
    if message is None:
        raise AssertionError("nothing was published on %s within %.0f seconds, "
                             "with the publish frequency set to %d — the "
                             "publish task runs on schedule, so check that the "
                             "build has anything to put in the payload"
                             % (PUB_FIRST, PUBLISH_WAIT, PUBLISH_EVERY))


@test("a publish carries the configured qos and retain flag",
      needs=("srvc",), services=("MQTT",), slow=True)
def publish_honours_qos_and_retain(t):
    state, _ = connected(t)

    message = state.broker.wait_for_message(PUBLISH_WAIT, topic=PUB_FIRST)
    if message is None:
        raise Skip("nothing was published on %s to inspect" % PUB_FIRST)

    if 1 != message.qos:
        raise AssertionError("the publish went out at qos %d, not the "
                             "configured 1" % message.qos)

    if not message.retain:
        raise AssertionError("the publish was configured to be retained but the "
                             "retain flag was not set")


@test("a qos 1 publish is not repeated once it has been acknowledged",
      needs=("srvc",), services=("MQTT",), slow=True)
def qos1_publish_settles(t):
    """
    A client that ignores PUBACK keeps retrying the same message forever, which
    looks like it is working until the broker's log fills up. At qos 1 the
    count over two publish intervals should track the interval, not run away.
    """
    state, _ = connected(t)

    if state.broker.wait_for_message(PUBLISH_WAIT, topic=PUB_FIRST) is None:
        raise Skip("nothing was published on %s to count" % PUB_FIRST)

    before = state.broker.message_count()
    window = PUBLISH_EVERY * 4
    time.sleep(window)
    sent = state.broker.message_count() - before

    # two topics, one publish each per interval, and a wide allowance for the
    # boundaries of the window
    most = 2 * (window // PUBLISH_EVERY + 2)
    if sent > most:
        raise AssertionError("%d messages arrived in %d seconds with a %d "
                             "second publish interval — acknowledged messages "
                             "are being retried" % (sent, window, PUBLISH_EVERY))


@test("both configured publish topics are used",
      needs=("srvc",), services=("MQTT",), slow=True)
def second_publish_topic_is_used(t):
    """The config holds two publish slots; filling only the first is a bug."""
    state, _ = connected(t)

    message = state.broker.wait_for_message(PUBLISH_WAIT, topic=PUB_SECOND)
    if message is None:
        raise AssertionError("nothing was published on the second topic %s "
                             "within %.0f seconds" % (PUB_SECOND, PUBLISH_WAIT))

    if 0 != message.qos:
        raise AssertionError("the second topic went out at qos %d, not the "
                             "configured 0" % message.qos)


@test("the board subscribes to its configured topics",
      needs=("srvc",), services=("MQTT",), slow=True)
def subscribes_to_configured_topics(t):
    state, _ = connected(t)

    for topic, wanted in ((SUB_FIRST, 1), (SUB_SECOND, 0)):
        qos = state.broker.wait_for_subscription(topic, PUBLISH_WAIT)
        if qos is None:
            raise AssertionError("the client never subscribed to %s" % topic)
        if qos != wanted:
            raise AssertionError("the client subscribed to %s at qos %d, not "
                                 "the configured %d" % (topic, qos, wanted))


@test("a message from the broker does not disturb the client",
      needs=("srvc",), services=("MQTT",), slow=True)
def inbound_message_is_survived(t):
    """
    What the board does with the payload depends on what is compiled in, so
    this asserts the part that is always true: receiving one must not drop the
    connection or silence the client.
    """
    state, _ = connected(t)

    if state.broker.wait_for_subscription(SUB_FIRST, PUBLISH_WAIT) is None:
        raise Skip("the client is not subscribed to %s yet" % SUB_FIRST)

    before = state.broker.connect_count()
    if not state.broker.publish(SUB_FIRST, "{\"pditest\":1}"):
        raise Skip("the broker could not send to the client")

    if not wait_for_traffic(state.broker, KEEPALIVE + 15):
        raise AssertionError("the client went quiet after being sent a message "
                             "on a topic it subscribed to")

    if state.broker.connect_count() != before:
        raise AssertionError("the client reconnected after being sent a message "
                             "on a topic it subscribed to")


@test("the connection is kept alive across a keepalive period",
      needs=("srvc",), services=("MQTT",), slow=True)
def connection_is_kept_alive(t):
    """
    The broker drops a client that says nothing for one keepalive, so something
    has to arrive inside every period. Which packet does it is not the client's
    obligation: traffic of its own counts, and a client that is publishing has
    no reason to add a ping. Insisting on a PINGREQ here asserted the wrong
    thing and failed against a perfectly healthy client.
    """
    state, _ = connected(t)

    before = state.broker.connect_count()

    if not wait_for_traffic(state.broker, KEEPALIVE):
        raise AssertionError("nothing arrived from the client within the %d "
                             "second keepalive, so a broker would drop it"
                             % KEEPALIVE)

    if state.broker.connect_count() != before:
        raise AssertionError("the client reconnected during the keepalive "
                             "period rather than holding the connection open")


@test("an idle client sends a keepalive", needs=("srvc",), services=("MQTT",),
      slow=True)
def idle_client_pings(t):
    """
    The ping only has to appear when there is nothing else to send, so the
    publish cycle is turned off to produce that state. `keepAliveTick` is reset
    by any inbound packet other than a qos 0 publish, and at qos 1 every PUBACK
    resets it — so with publishing on, a correct client never pings at all.
    """
    state, _ = connected(t)

    portal = portal_for(t)
    write_form(portal, PUBSUB, pubsub_fields(publish_every=0))

    try:
        quiet = state.broker.message_count()
        deadline = time.time() + 20.0
        while time.time() < deadline:
            if state.broker.message_count() == quiet:
                break
            quiet = state.broker.message_count()
            time.sleep(5.0)

        if not state.broker.wait_for_ping(KEEPALIVE + 20):
            raise AssertionError("the client published nothing and sent no "
                                 "keepalive within %d seconds, so a broker "
                                 "would drop it as dead"
                                 % (KEEPALIVE + 20))
    finally:
        write_form(portal_for(t), PUBSUB, pubsub_fields())


@test("the will is delivered when the connection drops",
      needs=("srvc",), services=("MQTT",), slow=True)
def will_is_delivered_on_drop(t):
    """
    The whole point of a will: the socket dies without a DISCONNECT and the
    broker publishes on the client's behalf. Dropping it from this side is the
    only way to produce that without unplugging the board.
    """
    state, _ = connected(t)

    delivered = len(state.broker.wills_delivered)
    state.broker.drop_client()

    deadline = time.time() + 30.0
    while time.time() < deadline:
        if len(state.broker.wills_delivered) > delivered:
            break
        time.sleep(0.5)
    else:
        raise AssertionError("the connection was dropped and no will was "
                             "delivered, so the will registered at connect "
                             "would never reach anyone")

    will = state.broker.wills_delivered[-1]
    expect_in(WILL_MESSAGE, will.text, "the delivered will message")


@test("the client comes back after the connection is dropped",
      needs=("srvc",), services=("MQTT",), slow=True)
def reconnects_after_drop(t):
    """
    Runs after the will test, which has already dropped the socket, so this is
    the same event examined from the other side: a board that never reconnects
    is off the network until someone reboots it.
    """
    state, _ = connected(t)

    before = state.broker.connect_count()
    state.broker.drop_client()

    if state.broker.wait_for_reconnect(before, CONNECT_WAIT) is None:
        raise AssertionError("the client did not reconnect within %.0f seconds "
                             "of the connection being dropped" % CONNECT_WAIT)


@test("removing a topic from the config unsubscribes it",
      needs=("srvc",), services=("MQTT",), slow=True)
def removed_topic_is_unsubscribed(t):
    """
    Changing the pub/sub config must let go of what it dropped. Left
    subscribed, the board keeps receiving on a topic its owner believes it no
    longer listens to. Runs last: it changes what the other tests set up.
    """
    state, _ = connected(t)

    if state.broker.wait_for_subscription(SUB_SECOND, PUBLISH_WAIT) is None:
        raise Skip("the client never subscribed to %s to drop it" % SUB_SECOND)

    portal = portal_for(t)
    write_form(portal, PUBSUB, pubsub_fields(second_subscription=""))

    deadline = time.time() + PUBLISH_WAIT
    while time.time() < deadline:
        if SUB_SECOND in state.broker.unsubscribed:
            return
        time.sleep(1.0)

    raise AssertionError("%s was removed from the config but the client never "
                         "unsubscribed from it" % SUB_SECOND)


@test("a placeholder in the client id is substituted before connecting",
      needs=("srvc",), services=("MQTT",), slow=True)
def mac_placeholder_is_substituted(t):
    """
    `[mac]` in the client id is how one firmware image gives every board a
    distinct identity. Sent through literally, every board on the estate would
    connect as the same client and knock each other off the broker.
    """
    state, _ = connected(t)

    before = state.broker.connect_count()

    portal = portal_for(t)
    write_form(portal, GENERAL, {
        "hst": state.address, "prt": str(state.broker.port),
        "clid": "pditest-[mac]", "usrn": "pdiuser", "pswd": "pdipass",
        "kpalv": str(KEEPALIVE), "cln": "clean",
    })

    session = state.broker.wait_for_reconnect(before, CONNECT_WAIT)
    if session is None:
        raise Skip("the client did not come back after the client id changed")

    if "[mac]" in session.client_id:
        raise AssertionError("the client connected as %r, with the placeholder "
                             "sent literally" % session.client_id)

    if not session.client_id.startswith("pditest-"):
        raise AssertionError("the client connected as %r, which is not the "
                             "configured id with the placeholder filled in"
                             % session.client_id)

    if len(session.client_id) <= len("pditest-"):
        raise AssertionError("the placeholder was dropped rather than "
                             "substituted: the client connected as %r"
                             % session.client_id)


@test("clearing one field does not discard the rest of the form",
      needs=("srvc",), services=("MQTT",), slow=True)
def clearing_a_field_keeps_the_submission(t):
    """
    A config page has to be able to empty a field, and emptying one must not
    quietly throw away the whole submission. Both halves are asserted in a
    single post: the host is cleared while the port and client id change, and
    all three have to take. Runs last because clearing the host disconnects the
    client from the test broker.
    """
    state = fixture(t)

    portal = portal_for(t)
    write_form(portal, GENERAL, {
        "hst": "", "prt": "1999", "clid": "pdicleared",
        "usrn": "", "pswd": "", "kpalv": str(KEEPALIVE), "cln": "clean",
    })

    stored = read_form(portal_for(t), GENERAL)

    if stored.get("hst"):
        raise AssertionError("the host was submitted empty but the page still "
                             "reports %r" % stored.get("hst"))

    for field, wanted in (("prt", "1999"), ("clid", "pdicleared")):
        if stored.get(field) != wanted:
            raise AssertionError("%s is %r, not the submitted %r — clearing one "
                                 "field discarded the rest of the form"
                                 % (field, stored.get(field), wanted))
