#!/usr/bin/env python3

"""
More than one session at a time, and what a session leaves behind.

These need a transport that can dial the target twice, so they skip on a board
reached over a single serial cable. They are the only tests that cover what
happens when a client disappears rather than logging out — the case a network
shell hits routinely and a serial console never does.
"""

from .registry import test, expect_in, expect_not_in, Skip
from ..driver.shell import PROMPT


@test("a second session can log in while the first is open", needs=("whoami",))
def second_session(t):
    peer = t.peer()
    try:
        expect_in(t.username, peer.run("whoami", t.timeout), "the second session")
        expect_in(t.username, t.run("whoami"), "the first session still works")
    finally:
        peer.close()


@test("who lists both sessions", needs=("who",))
def who_lists_both(t):
    peer = t.peer()
    try:
        listing = t.run("who")
        rows = [line for line in listing.splitlines()
                if t.username in line and "USER" not in line]
        if len(rows) < 2:
            raise AssertionError("who showed %d session(s) with two open:\n%s"
                                 % (len(rows), listing))
    finally:
        peer.close()


@test("each session has its own working directory", needs=("cd", "pwd", "mkdir"))
def sessions_have_own_pwd(t):
    path = t.workspace("ws_pwd")

    peer = t.peer()
    try:
        # the first session is inside the workspace, the second is not
        expect_in(path, t.run("pwd"), "the first session's directory")
        expect_not_in(path, peer.run("pwd", t.timeout), "the second session's directory")
    finally:
        peer.close()


@test("a session that drops mid-prompt does not hand it to the next client",
      needs=("su", "whoami"))
def dropped_prompt_is_not_inherited(t):
    """
    A client that disappears while a command is waiting for input used to leave
    that command owned by a recycled session slot, so the next client to land on
    it was answering a prompt it never saw. The teardown path releases the
    session's commands now; this is what checks it on a real target.
    """
    first = t.peer()
    try:
        # leave su waiting for a username, then vanish without answering
        first.send_line("su")
        first.expect("user: ", t.timeout)
    finally:
        first.close()

    second = t.peer()
    try:
        # a fresh client must get a shell, not the abandoned prompt
        out = second.run("whoami", t.timeout)
        expect_in(t.username, out, "the new session runs its own command")
        expect_not_in("user:", out, "no inherited prompt")
        expect_not_in("Pass", out, "no inherited prompt")
    finally:
        second.close()


@test("a dropped session is not left in the session table", needs=("who",))
def dropped_session_is_released(t):
    before = len([line for line in t.run("who").splitlines() if t.username in line])

    peer = t.peer()
    peer.close()

    # the service notices the drop on its next pass rather than immediately
    for _ in range(10):
        after = len([line for line in t.run("who").splitlines() if t.username in line])
        if after <= before:
            return

    raise AssertionError("the session table kept the dropped client: %d before, %d after"
                         % (before, after))


@test("a full session table refuses cleanly instead of granting a dead slot",
      needs=("whoami",), slow=True)
def full_table_refuses_cleanly(t):
    """
    The table used to grant a login when it was already full and then have
    nowhere to put the session, so the client authenticated onto a slot that
    could not run a command. A refusal is the right answer; a granted session
    that cannot run whoami is the bug. The pool must also come back once the
    extra sessions close.
    """
    from ..driver.shell import ShellError

    peers = []
    filled = False
    try:
        for _ in range(8):
            try:
                peer = t.dial()
                peer.attach(t.username, t.password, timeout=min(t.timeout, 12.0))
            except ShellError:
                filled = True
                break

            peers.append(peer)
            expect_in(t.username, peer.run("whoami", t.timeout),
                      "a granted session could not run a command")

        if not peers:
            raise Skip("could not open a session to fill the table")
        if not filled:
            raise Skip("the table did not fill in eight connections")
    finally:
        for peer in peers:
            try:
                peer.close()
            except Exception:
                pass

    recovered = t.peer()
    try:
        expect_in(t.username, recovered.run("whoami", t.timeout),
                  "the pool did not recover after the sessions closed")
    finally:
        recovered.close()


@test("a wrong password is refused on a fresh connection", needs=("whoami",))
def wrong_password_refused(t):
    """
    Two authentication models, one expectation. Telnet and serial ask at a
    login prompt inside the session; ssh settles it in the handshake, so the
    refusal arrives as a failure to connect at all.
    """
    from ..driver.shell import ShellError

    try:
        peer = t.dial(password="not-the-password")
    except ShellError:
        return

    try:
        try:
            peer.expect("login: ", min(t.timeout, 12.0))
        except ShellError:
            # accepted by the tcp stack and then never spoken to: the same
            # one-client-at-a-time case the rest of this suite skips for
            raise Skip("the target serves one session at a time")

        peer.send_line(t.username)
        peer.expect("Pass : ", t.timeout)
        peer.send_line("not-the-password")

        # back to a login prompt, not a shell
        peer.expect("login: ", t.timeout)
    finally:
        peer.close()


@test("logout ends the session", needs=("logout", "whoami"))
def logout_ends_session(t):
    """
    A network shell closes its channel on logout; the same command on a serial
    console drops back to a login prompt. Either way the session must stop
    running commands as the user. Driven on a peer so the test's own shell lives.
    """
    from ..driver.shell import ShellError

    peer = t.peer()
    try:
        expect_in(t.username, peer.run("whoami", t.timeout), "the peer is logged in")

        try:
            peer.send_line("logout")
        except ShellError:
            return

        try:
            after = peer.run("whoami", min(t.timeout, 8.0))
        except ShellError:
            return

        expect_not_in(t.username, after,
                      "the session still ran a command as the user after logout")
    finally:
        peer.close()
