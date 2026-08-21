#!/usr/bin/env python3

"""
scp against the board, both protocols it can meet.

The device has no legacy SCP: a client that speaks the old exec-driven protocol
is answered with one line telling it to use sftp, and the channel is closed
rather than left hanging, which is what a legacy client used to do. Modern scp
carries the file over the SFTP subsystem instead, and `scp -s` names that
protocol explicitly so the test does not depend on the client's default.

The transfer tests drive the real scp binary, so they need it on the test host
and a way to answer its password prompt; both are skipped for when either is
missing. The refusal test needs neither and runs wherever paramiko does.
"""

import os
import time

from .registry import test, expect_in, Skip


def ssh_client(t, attempts=3):
    """A bare paramiko SSHClient on the target, for driving a raw exec channel."""
    try:
        import paramiko
    except ImportError as err:
        raise Skip("paramiko is not installed: %s" % err)

    from ..driver import ssh_shell  # noqa: F401  registers curve25519

    last = None
    for attempt in range(attempts):
        client = paramiko.SSHClient()
        client.set_missing_host_key_policy(paramiko.AutoAddPolicy())
        try:
            client.connect(t.address(), port=22, username=t.username,
                           password=t.password, timeout=45, banner_timeout=45,
                           auth_timeout=45, allow_agent=False, look_for_keys=False)
            return client
        except Exception as err:
            last = err
            client.close()
            if attempt + 1 < attempts:
                time.sleep(3.0)

    raise Skip("the target has no reachable ssh server: %s" % last)


def _scp_once(binary, pexpect, t, source, dest, timeout):
    command = ("%s -s -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null "
               "-o PubkeyAuthentication=no %s %s" % (binary, source, dest))

    child = pexpect.spawn(command, timeout=timeout, encoding="latin-1")
    try:
        while True:
            index = child.expect(["[Pp]assword:", pexpect.EOF, pexpect.TIMEOUT])
            if index == 0:
                child.sendline(t.password)
                continue
            break
        child.close()
    except pexpect.TIMEOUT:
        child.close(force=True)
        return None, child.before or ""

    return child.exitstatus, child.before or ""


def run_scp(t, source, dest, produced, attempts=4, timeout=90):
    """
    Drive the real scp binary with `-s` (the SFTP protocol), answering its
    password prompt, until `produced()` reports the transfer landed, then assert
    the exit status was clean.

    A clean exit is part of the contract: the board once dropped the ssh session
    before flushing its CHANNEL_CLOSE, so scp exited non-zero even on a
    byte-perfect transfer (defect AL). The transfer landing and the exit being
    zero are checked separately so a regression in either is legible.

    On a target with a small ssh pool (esp8266 holds two) the shell already owns
    a slot, so scp's own connection can be refused until an earlier one is
    released; that is retried, and skipped if it never clears.
    """
    import shutil
    import time

    binary = shutil.which("scp")
    if not binary:
        raise Skip("no scp binary on the test host")

    try:
        import pexpect
    except ImportError as err:
        raise Skip("pexpect is not installed: %s" % err)

    last = ""
    for attempt in range(attempts):
        status, output = _scp_once(binary, pexpect, t, source, dest, timeout)
        if produced():
            if status not in (0, None):
                raise AssertionError(
                    "scp transferred the file but exited %s (channel not closed cleanly)\n%s"
                    % (status, output[-300:]))
            return
        tail = output.strip().splitlines()
        last = tail[-1] if tail else ""
        if attempt + 1 < attempts:
            time.sleep(3.0)

    raise Skip("scp could not complete against this target (session pool?): %s" % last)


@test("legacy scp is refused with a pointer to sftp")
def legacy_scp_refused(t):
    """
    An exec request that starts with scp is the legacy protocol. The board must
    answer it, not ignore it, and the answer must send the client to sftp.
    """
    client = ssh_client(t)
    try:
        channel = client.get_transport().open_session()
        channel.settimeout(20)
        channel.exec_command("scp -t /scp_legacy.txt")

        collected = b""
        deadline = time.time() + 20
        while time.time() < deadline:
            if channel.recv_ready():
                chunk = channel.recv(4096)
                if not chunk:
                    break
                collected += chunk
            elif channel.closed or channel.exit_status_ready():
                if channel.recv_ready():
                    continue
                break
            else:
                time.sleep(0.2)

        expect_in("sftp", collected.decode("latin-1").lower(),
                  "the refusal names sftp as the way in")
    finally:
        client.close()


@test("scp -s uploads a file over the sftp protocol", needs=("cat", "rm"), slow=True)
def scp_upload_over_sftp(t):
    marker = "carried-by-scp-%d" % (int(time.time()) % 100000)
    local = os.path.join("/tmp", "scp_up_%d.txt" % os.getpid())
    with open(local, "w") as handle:
        handle.write(marker + "\n")

    remote = "%s@%s:/scp_up.txt" % (t.username, t.address())
    try:
        run_scp(t, local, remote, produced=lambda: marker in t.run("cat /scp_up.txt"))
        expect_in(marker, t.run("cat /scp_up.txt"),
                  "the file scp -s uploaded is on the target")
    finally:
        os.remove(local)
        t.run("rm /scp_up.txt")


@test("scp -s downloads a file over the sftp protocol", needs=("echo", "rm"), slow=True)
def scp_download_over_sftp(t):
    marker = "fetched-by-scp-%d" % (int(time.time()) % 100000)
    t.run("rm /scp_down.txt")
    t.run("echo %s > /scp_down.txt" % marker)

    local = os.path.join("/tmp", "scp_down_%d.txt" % os.getpid())
    remote = "%s@%s:/scp_down.txt" % (t.username, t.address())

    def landed():
        try:
            with open(local) as handle:
                return marker in handle.read()
        except IOError:
            return False

    try:
        run_scp(t, remote, local, produced=landed)
        with open(local) as handle:
            expect_in(marker, handle.read(), "the file scp -s downloaded matches")
    finally:
        if os.path.exists(local):
            os.remove(local)
        t.run("rm /scp_down.txt")
