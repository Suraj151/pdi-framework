#!/usr/bin/env python3

"""
Registration and running for the transport-agnostic feature suites.

A test here is written against a shell and nothing else, so the same assertion
runs against the host process, a board on a serial cable and a board over ssh.
What the target cannot do it is skipped for, by name and with the reason, so a
build that compiled a service out reports that rather than a wall of failures.

Tests share one session: a board cannot be restarted between them the way a
host process can. So each works inside its own directory and puts back anything
it changed.
"""

import importlib
import os
import pkgutil
import re
import time

from ..driver.shell import PROMPT, CTRL_C, ShellError

IPV4 = re.compile(r"\b(\d{1,3}(?:\.\d{1,3}){3})\b")

GREEN = "\033[32m"
RED = "\033[31m"
YELLOW = "\033[33m"
DIM = "\033[2m"
RESET = "\033[0m"

TESTS = []


class Skip(Exception):
    """Raised by a test that finds the target cannot do what it needs."""


def expect_in(needle, haystack, what):
    if needle not in haystack:
        raise AssertionError("%s: expected %r in:\n%s" % (what, needle, haystack))


def expect_not_in(needle, haystack, what):
    if needle in haystack:
        raise AssertionError("%s: did not expect %r in:\n%s" % (what, needle, haystack))


def expect_any(needles, haystack, what):
    if not any(needle in haystack for needle in needles):
        raise AssertionError("%s: expected one of %r in:\n%s" % (what, needles, haystack))


class Test(object):

    def __init__(self, feature, name, fn, needs, mounts, services, su, slow):
        self.feature = feature
        self.name = name
        self.fn = fn
        self.needs = tuple(needs)
        self.mounts = tuple(mounts)
        self.services = tuple(services)
        self.su = su
        self.slow = slow

    @property
    def full_name(self):
        return "%s.%s" % (self.feature, self.name)


def test(name, needs=(), mounts=(), services=(), su=False, slow=False):
    """
    Register one test.

    needs/mounts/services are what the target must have for the test to mean
    anything; absent, it is skipped. su marks a test that changes the logged in
    user, so the runner puts the session back afterwards.
    """
    def register(fn):
        feature = fn.__module__.rsplit(".", 1)[-1]
        if feature.startswith("test_"):
            feature = feature[len("test_"):]
        TESTS.append(Test(feature, name, fn, needs, mounts, services, su, slow))
        return fn
    return register


class Target(object):
    """What a test is handed: a shell, what it can do, and a place to work."""

    def __init__(self, shell, caps, username, password, timeout=20.0, peer_factory=None):
        self.shell = shell
        self.caps = caps
        self.username = username
        self.password = password
        self.timeout = timeout
        self._peer_factory = peer_factory
        self._workspaces = []
        self._sync = 0
        self._portal_at = None
        self._portals = []
        self._finalizers = []

    def adopt_finalizers(self, other):
        """
        Take over another target's pending suite teardown.

        Used when a transport is re-dialled mid-run: the suite that registered
        the restore is still running, and the work still has to be undone.
        """
        self._finalizers = other._finalizers
        other._finalizers = []

    def at_exit(self, fn):
        """
        Run fn once, after the last test, instead of after this one.

        For state that is expensive to set up and has to be put back: a suite
        that reconfigures a persistent setting registers the restore here so
        the tests in between share one setup and the board is still left as it
        was found.
        """
        self._finalizers.append(fn)

    def finish(self):
        while self._finalizers:
            fn = self._finalizers.pop()
            try:
                fn()
            except Exception as err:
                print("       %scould not undo a suite's setup: %s%s"
                      % (DIM, err, RESET), flush=True)

    def dial(self, password=None):
        """
        Open another connection and hand it back unauthenticated.

        Raises ShellError when the target refuses, which a caller testing a
        refusal wants to see rather than have turned into a skip.
        """
        if self._peer_factory is None:
            raise Skip("this transport cannot open a second session")

        return self._peer_factory(password or self.password)

    def peer(self, login=True):
        """
        A second, independent session on the same target.

        Only a transport that can dial again has one: a board on a serial cable
        is a single terminal. Dialling is not enough either — a service that
        serves one client at a time accepts the connection and then never
        speaks to it, so the session is skipped unless it actually reaches a
        prompt. The caller closes what it opens.

        A target with a small session pool releases the previous peer a second
        or two after it goes away, and these tests open and close peers back to
        back. So a refusal is retried for a while before it is believed: the
        difference between "cannot" and "not yet" is the whole point of the
        skip, and getting it wrong reports a working target as a limited one.
        """
        deadline = time.time() + max(self.timeout, 20.0)
        last = None

        while True:
            try:
                peer = self.dial()
            except ShellError as err:
                last = "a second connection was refused: %s" % err
                peer = None

            if peer is not None:
                if not login:
                    return peer

                try:
                    peer.attach(self.username, self.password,
                                timeout=min(self.timeout, 12.0))
                    return peer
                except ShellError:
                    last = "the target serves one session at a time"
                    peer.close()

            if time.time() >= deadline:
                raise Skip(last)

            time.sleep(2.0)

    def address(self):
        """
        An address the target answers on from here.

        A network transport already holds one. A board on a serial cable is
        asked for its own, which is also the only honest source: the station
        address is whatever the operator's network handed it.
        """
        known = getattr(self.shell, "host", None)
        if known:
            return known

        for found in IPV4.findall(self.run("net ip")):
            if found not in ("0.0.0.0", "255.255.255.255"):
                return found

        raise Skip("the target has no address to reach its portal on")

    def portal(self, login=True, username=None, password=None):
        """
        A fresh conversation with the web portal, logged in by default.

        The address and port are found once and reused; the session is not, so
        one test cannot inherit another's cookie. The table holds only a couple
        of sessions, so every one opened here is logged out again in cleanup
        even if the test that opened it failed.
        """
        from ..driver.portal import Portal, PortalError

        if self._portal_at is None:
            found = Portal.reachable(self.address(), timeout=min(self.timeout, 15.0))
            self._portal_at = (found.host, found.port) if found else ()

        if not self._portal_at:
            raise Skip("the target serves no http portal")

        portal = Portal(self._portal_at[0], self._portal_at[1],
                        timeout=min(self.timeout, 15.0))
        self._portals.append(portal)

        if login:
            who = username or self.username
            answer = portal.login(who, password or self.password)
            if portal.session_cookie() is None:
                raise PortalError("the portal refused %s: status %d"
                                  % (who, answer.status))

        return portal

    def run(self, command, timeout=None):
        return self.shell.run(command, timeout or self.timeout)

    def workspace(self, name):
        """
        A directory of this test's own, emptied first in case a previous run
        died before it could clean up. Returns the path, with the session
        already in it.
        """
        path = "/" + name
        self.run("cd /")
        self.run("rm %s" % path)
        self.run("mkdir %s" % path)
        self.run("cd %s" % path)
        self._workspaces.append(path)
        return path

    def write(self, path, text):
        """
        Put a line of text in a file the only way a terminal can. Callers that
        need several lines call this once per line with >> .
        """
        self.run("echo %s > %s" % (text, path))

    def append(self, path, text):
        self.run("echo %s >> %s" % (text, path))

    def become_root(self):
        self.run("su %s %s" % (self.username, self.password))

    def resync(self):
        """
        Get back to a prompt before anything else is typed.

        A test that failed part way through an interactive command leaves that
        command waiting for input, and the next line typed — a cleanup command,
        or the first line of the next test — is swallowed as its answer. That
        turns one failure into a cascade of them, so the session is interrupted
        and confirmed at a prompt first.
        """
        for _ in range(3):
            self.shell.send_raw(CTRL_C)
            self.shell.drain(0.4, 4.0)

            # a token rather than a prompt: counting prompts goes wrong the
            # moment one arrives later than the drain waited, and from then on
            # every command reads the previous command's output. Reading up to
            # something only this call could have produced discards whatever
            # was queued, however much of it there is.
            self._sync += 1
            token = "sync%d" % self._sync
            try:
                self.shell.send_line("echo %s" % token)
                self.shell.expect(token, 8.0)
                self.shell.expect(PROMPT, 8.0)
                return True
            except ShellError:
                continue

        return False

    def cleanup(self):
        for portal in self._portals:
            try:
                portal.logout()
            except Exception:
                pass
        self._portals = []

        self.resync()
        self.run("cd /")
        for path in self._workspaces:
            self.run("rm %s" % path)
        self._workspaces = []


def discover():
    """Import every test_*.py beside this file so its decorators run."""
    package = os.path.dirname(os.path.abspath(__file__))
    for entry in sorted(pkgutil.iter_modules([package])):
        if entry.name.startswith("test_"):
            importlib.import_module("%s.%s" % (__package__, entry.name))

    return TESTS


class Lane(object):
    """One transport in the rotation, and how to open it again."""

    def __init__(self, label, target, reopen=None):
        self.label = label
        self.target = target
        self.reopen = reopen


def run_interleaved(lanes, only=None, verbose=False):
    """
    Run the suite once across several targets at the same time, rotating which
    one each test lands on. Returns (passed, failed, skipped).

    Every session stays open for the whole run, so the target is held at the
    multi-session load it would see in use, which running one transport after
    another never does. Tests still execute one at a time: they share the
    board's mqtt config, /etc/hosts and user store, so two running at once
    would corrupt each other rather than test anything.

    Each test runs once rather than once per transport, which is where the time
    goes. A test the assigned target cannot do is offered to the others before
    it is called a skip, so a transport-limited test still runs wherever it can.

    lanes is a list of Lane. A transport that dies is re-dialled and stays in
    the rotation; only one that cannot be re-opened is dropped.
    """
    discover()

    alive = list(lanes)
    for lane in alive:
        lane.target.resync()

    passed = 0
    failed = []
    skipped = []
    turn = 0
    started = time.time()
    touched = dict((lane.label, time.time()) for lane in lanes)
    declined = dict((lane.label, 0) for lane in lanes)

    for item in TESTS:
        if only and only not in item.full_name:
            continue

        if not alive:
            skipped.append((item.full_name, "not run, every connection was lost"))
            continue

        # offer the test to each target in turn, starting where the rotation
        # left off, until one of them is able to run it
        reasons = []
        ran = False

        for offset in range(len(alive)):
            lane = alive[(turn + offset) % len(alive)]
            label, target = lane.label, lane.target

            reason = target.caps.why_skip(item.needs, item.mounts, item.services)
            if reason:
                reasons.append("%s: %s" % (label, reason))
                declined[label] = declined.get(label, 0) + 1
                continue

            began = time.time()
            try:
                item.fn(target)
                took = (time.time() - began) * 1000.0
                passed += 1
                ran = True
                if verbose:
                    print("  %sok%s   %s %s[%s, %.0f ms]%s"
                          % (GREEN, RESET, item.full_name, DIM, label, took, RESET), flush=True)
                else:
                    print("  %sok%s   %s %s[%s]%s"
                          % (GREEN, RESET, item.full_name, DIM, label, RESET), flush=True)
            except Skip as reason:
                reasons.append("%s: %s" % (label, reason))
                declined[label] = declined.get(label, 0) + 1
            except Exception as err:
                failed.append(item.full_name)
                ran = True
                print("  %sFAIL%s %s %s[%s]%s"
                      % (RED, RESET, item.full_name, DIM, label, RESET), flush=True)
                print("       %s" % str(err).replace("\n", "\n       "), flush=True)
                if isinstance(err, ShellError):
                    _recover(alive, lane)
            finally:
                try:
                    target.cleanup()
                    if item.su:
                        target.become_root()
                except Exception as err:
                    print("       %scould not restore %s: %s%s"
                          % (DIM, label, err, RESET), flush=True)
                    if isinstance(err, ShellError):
                        _recover(alive, lane)

            if ran:
                touched[label] = time.time()
                break

        turn += 1
        _keep_awake(alive, touched)

        if not ran:
            note = "; ".join(reasons) if reasons else "no target could run it"
            skipped.append((item.full_name, note))
            print("  %sskip%s %s %s(%s)%s"
                  % (YELLOW, RESET, item.full_name, DIM, note, RESET), flush=True)

    for lane in lanes:
        lane.target.finish()

    elapsed = (time.time() - started) * 1000.0
    print("")

    # a transport that quietly declines everything still looks fine, because
    # another one picks the test up and reports the pass
    for lane in lanes:
        if declined.get(lane.label):
            print("%s%s declined %d test(s); another transport ran them%s"
                  % (YELLOW, lane.label, declined[lane.label], RESET))

    print("----------------------------------------------------------")
    total = passed + len(failed) + len(skipped)
    if failed:
        print("%s%d failed%s, %d passed, %d skipped, %d total in %.0f ms"
              % (RED, len(failed), RESET, passed, len(skipped), total, elapsed))
    else:
        print("%s%d passed%s, %d skipped, %d total in %.0f ms"
              % (GREEN, passed, RESET, len(skipped), total, elapsed))

    return passed, failed, skipped


# A shell the target considers idle is reclaimed after a few minutes, which is
# correct for an abandoned client and wrong for a session waiting its turn: a
# slow suite can leave one untouched for longer than the window. Well inside it,
# each waiting transport is given something harmless to do.
KEEPALIVE_AFTER = 60.0


def _keep_awake(alive, touched):
    now = time.time()

    for lane in list(alive):
        if now - touched.get(lane.label, now) < KEEPALIVE_AFTER:
            continue

        try:
            lane.target.run("pwd", min(lane.target.timeout, 10.0))
        except Exception:
            _recover(alive, lane)
        touched[lane.label] = time.time()


def _recover(alive, lane):
    """
    Put a lane back in service, or take it out of the rotation.

    A target is entitled to reclaim a shell it considers idle, and a slow test
    elsewhere can leave one untouched for longer than that. Losing the
    transport for the rest of the run would cost the coverage it was carrying,
    so it is dialled again before it is given up on.
    """
    try:
        if lane.target.resync():
            return
    except Exception:
        pass

    if lane.reopen is not None:
        try:
            fresh = lane.reopen()
            fresh.adopt_finalizers(lane.target)
            lane.target = fresh
            print("       %s%s was dropped and has been reconnected%s"
                  % (DIM, lane.label, RESET), flush=True)
            return
        except Exception as err:
            print("       %s%s could not be reconnected: %s%s"
                  % (DIM, lane.label, err, RESET), flush=True)

    if lane in alive:
        alive.remove(lane)

    print("       %s%s is gone; %d transport(s) left%s"
          % (DIM, lane.label, len(alive), RESET), flush=True)


def run(shell, caps, username, password, only=None, verbose=False, timeout=20.0,
        peer_factory=None):
    """
    Run every discovered test against one already logged in shell. Returns
    (passed, failed, skipped).
    """
    discover()

    target = Target(shell, caps, username, password, timeout, peer_factory)
    target.resync()

    passed = 0
    failed = []
    skipped = []

    started = time.time()
    lost = None
    aborted = None
    for index, item in enumerate(TESTS):
        if only and only not in item.full_name:
            continue

        reason = caps.why_skip(item.needs, item.mounts, item.services)
        if reason:
            skipped.append((item.full_name, reason))
            print("  %sskip%s %s %s(%s)%s" % (YELLOW, RESET, item.full_name, DIM, reason, RESET),
                  flush=True)
            continue

        began = time.time()
        try:
            item.fn(target)
            took = (time.time() - began) * 1000.0
            passed += 1
            if verbose:
                print("  %sok%s   %s %s(%.0f ms)%s" % (GREEN, RESET, item.full_name, DIM, took, RESET),
                      flush=True)
            else:
                print("  %sok%s   %s" % (GREEN, RESET, item.full_name), flush=True)
        except Skip as reason:
            skipped.append((item.full_name, str(reason)))
            print("  %sskip%s %s %s(%s)%s" % (YELLOW, RESET, item.full_name, DIM, reason, RESET),
                  flush=True)
        except Exception as err:
            failed.append(item.full_name)
            print("  %sFAIL%s %s" % (RED, RESET, item.full_name), flush=True)
            print("       %s" % str(err).replace("\n", "\n       "), flush=True)
            if isinstance(err, ShellError):
                lost = err
        finally:
            # a test that failed part way through still has a directory open
            # and may have left the session as another user
            try:
                target.cleanup()
                if item.su:
                    target.become_root()
            except Exception as err:
                print("       %scould not restore the session: %s%s" % (DIM, err, RESET), flush=True)
                if lost is None and isinstance(err, ShellError):
                    lost = err

        # Once the target has closed the connection every remaining test fails
        # on the same dead socket, which buries the one real failure under a
        # hundred identical ones and wastes the rest of the run. One resync
        # decides whether the transport is really gone before giving up on it.
        if lost is not None:
            alive = False
            try:
                alive = target.resync()
            except Exception:
                alive = False

            if not alive:
                remaining = TESTS[index + 1:]
                if only:
                    remaining = [t for t in remaining if only in t.full_name]
                aborted = (str(lost), len(remaining))
                for pending in remaining:
                    skipped.append((pending.full_name, "not run, the connection was lost"))
                break

            lost = None

    target.finish()

    elapsed = (time.time() - started) * 1000.0
    print("")
    if aborted is not None:
        print("%sthe target closed the connection: %s%s" % (RED, aborted[0], RESET))
        print("%sstopped here; %d further tests were not run%s" % (DIM, aborted[1], RESET))
    print("----------------------------------------------------------")
    total = passed + len(failed) + len(skipped)
    if failed:
        print("%s%d failed%s, %d passed, %d skipped, %d total in %.0f ms"
              % (RED, len(failed), RESET, passed, len(skipped), total, elapsed))
    else:
        print("%s%d passed%s, %d skipped, %d total in %.0f ms"
              % (GREEN, passed, RESET, len(skipped), total, elapsed))

    return passed, failed, skipped
