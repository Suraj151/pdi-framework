#!/usr/bin/env python3

"""
The idle and handshake timeouts, observed by waiting them out.

These are the behaviours only a clock reveals: a connection that stops being
used is reclaimed so its session slot is not held for ever, and a connection
that never finishes its handshake is not left half-open. Each test opens its
own connection to the board by address — independent of the transport the suite
is reached on — idles it, and checks the board honoured the timeout its config
promises (SshConfig.h / TelnetConfig.h). The shell timeouts are minutes long,
so all but the handshake one are slow.

The one thing that has to be arranged: while a separate connection is left to
time out, the suite's own lane must be kept alive, or it is reaped too and the
run cannot continue. _keepalive touches it on a period well inside its window.
"""

import socket
import time

from .registry import test, expect_in, Skip
from ..driver.shell import ShellError


def _keepalive(t, seconds, step=20.0):
    end = time.time() + seconds
    while time.time() < end:
        time.sleep(min(step, max(0.0, end - time.time())))
        try:
            t.run("pwd")
        except ShellError:
            pass


def _telnet_peer(t):
    from ..driver.connect import connect

    try:
        peer, _ = connect("telnet:%s" % t.address())
    except (ShellError, OSError) as err:
        raise Skip("cannot reach telnet on the board: %s" % err)

    try:
        peer.attach(t.username, t.password, timeout=min(t.timeout, 15.0))
    except ShellError as err:
        peer.close()
        raise Skip("cannot open a second telnet session: %s" % err)
    return peer


@test("an ssh connection that never handshakes is dropped", slow=True)
def ssh_handshake_idle_timeout(t):
    """A tcp client that opens the ssh port and never sends its version banner
    must be closed on SSH_HANDSHAKE_IDLE_MS, not held open."""
    try:
        sock = socket.create_connection((t.address(), 22), timeout=15)
    except OSError as err:
        raise Skip("no ssh server reachable: %s" % err)

    sock.settimeout(25)
    try:
        start = time.time()
        closed = False
        while time.time() - start < 30:
            try:
                if not sock.recv(256):
                    closed = True
                    break
            except socket.timeout:
                break

        elapsed = time.time() - start
        if not closed:
            raise AssertionError("the server held an un-handshaked connection past its handshake timeout")
        if elapsed < 3:
            raise AssertionError("the connection dropped at once (%.1fs), not on the idle timeout" % elapsed)
    finally:
        sock.close()


@test("an idle sftp session is closed after the sftp timeout", slow=True)
def sftp_idle_timeout(t):
    from .test_sftp import sftp_open

    client, sftp = sftp_open(t)
    try:
        sftp.listdir(".")
        _keepalive(t, 150)   # SSH_SFTP_IDLE_MS is 120s

        try:
            sftp.listdir(".")
        except Exception:
            return           # closed as promised
        raise AssertionError("the sftp session was still alive after its idle timeout")
    finally:
        try:
            sftp.close()
        except Exception:
            pass
        client.close()


@test("an idle telnet session is closed after the telnet timeout", slow=True)
def telnet_idle_timeout(t):
    peer = _telnet_peer(t)
    try:
        peer.run("pwd", timeout=8)
        _keepalive(t, 200)   # TELNET_SHELL_IDLE_MS is 180s

        try:
            peer.run("pwd", timeout=8)
        except ShellError:
            return           # closed as promised
        raise AssertionError("the telnet session was still alive after its idle timeout")
    finally:
        peer.close()


@test("a telnet session running a command is not reaped while it works", slow=True)
def busy_telnet_survives_idle(t):
    """The idle timeout must not reap a session that has a command in flight;
    a watch keeps one busy with no keyboard input for longer than the window."""
    peer = _telnet_peer(t)
    try:
        peer.send_line("watch c=whoami; i=5000; n=1000")
        peer.drain(2.0)

        _keepalive(t, 200)   # past the 180s telnet idle window

        peer.send_raw("\x03")
        peer.drain(1.0)
        expect_in(t.username, peer.run("whoami", timeout=8),
                  "the busy session survived past its idle window")
    finally:
        peer.close()
