#!/usr/bin/env python3

"""
The ssh server as a server, rather than as a way to reach the shell.

Every device-tier run over ssh already exercises the key exchange, the mac
negotiation and password auth — if those were broken nothing else would run at
all. What is left is the part a shell transport never touches: the host keys on
disk, the config that turns each auth method on, public-key authentication, the
keygen command, and the bound on how many sessions may be open at once.

These dial the ssh port themselves rather than using the session's transport,
so the same assertions run whether the suite arrived over serial, telnet or ssh.
"""

import base64

from .registry import test, expect_in, expect_not_in, Skip

SSH_DIR = "/etc/ssh"
AUTHORIZED_KEYS = "/.ssh/authorized_keys"

# an ed25519 public key blob is 32 bytes wrapped in an ssh string encoding
ED25519_PREFIX = "ssh-ed25519"


def ssh_dial(t, attempts=3, **kwargs):
    """
    Open an ssh connection to the target, whatever transport the suite is on.

    Returns the shell, or raises ShellError. Never turns a failure into a skip:
    a test whose subject is whether a credential is accepted has to be able to
    tell "refused" from "unreachable", and a helper that skips on both hides
    exactly the thing being measured.

    A handshake here is curve25519 and ed25519 on a microcontroller and the
    session pool is shallow, so a transport-level failure is retried; an
    authentication failure is returned immediately, because that is an answer.
    """
    import time

    from ..driver.ssh_shell import SshShell
    from ..driver.shell import ShellError

    last = None
    for attempt in range(attempts):
        try:
            return SshShell(t.address(), port=22, username=t.username, **kwargs)
        except ShellError as err:
            last = err
            if is_auth_failure(err):
                raise
            if attempt + 1 < attempts:
                time.sleep(3.0)

    raise last


def is_auth_failure(err):
    """Whether ssh answered and refused, as opposed to never answering."""
    return "authentication failed" in str(err).lower()


def require_ssh(t):
    """Skip unless the target has an ssh server that accepts the known password."""
    from ..driver.shell import ShellError

    try:
        shell = ssh_dial(t, password=t.password)
    except ShellError as err:
        raise Skip("the target has no reachable ssh server: %s" % err)

    shell.close()


def generate_ed25519(tmpdir, name):
    """
    A throwaway ed25519 key pair, written where paramiko can load it.

    Uses cryptography, which paramiko already depends on, so this costs no new
    dependency. Returns (private key path, the authorized_keys line).
    """
    try:
        from cryptography.hazmat.primitives import serialization
        from cryptography.hazmat.primitives.asymmetric import ed25519
    except Exception as err:
        raise Skip("cannot generate a test key: %s" % err)

    private = ed25519.Ed25519PrivateKey.generate()

    pem = private.private_bytes(
        encoding=serialization.Encoding.PEM,
        format=serialization.PrivateFormat.OpenSSH,
        encryption_algorithm=serialization.NoEncryption(),
    )

    path = "%s/id_ed25519_%s" % (tmpdir, name)
    with open(path, "wb") as fh:
        fh.write(pem)
    import os
    os.chmod(path, 0o600)

    opened = private.public_key().public_bytes(
        encoding=serialization.Encoding.OpenSSH,
        format=serialization.PublicFormat.OpenSSH,
    ).decode()

    return path, opened


def install_authorized_key(t, line):
    """Put one authorized_keys entry on the target, replacing what was there."""
    t.run("mkdir /.ssh")
    t.run("rm %s" % AUTHORIZED_KEYS)
    t.run("echo %s > %s" % (line, AUTHORIZED_KEYS))

    written = t.run("cat %s" % AUTHORIZED_KEYS)
    if ED25519_PREFIX not in written:
        raise Skip("the target would not store an authorized_keys entry:\n%s" % written)


def remove_authorized_keys(t):
    t.run("rm %s" % AUTHORIZED_KEYS)


@test("the host key directory holds a key and a config", needs=("ls",))
def host_keys_on_disk(t):
    listing = t.run("ls %s" % SSH_DIR)
    if "no such" in listing.lower() or "CmdErr" in listing:
        raise Skip("no %s on this target" % SSH_DIR)

    expect_in("ed25519", listing, "an ed25519 host key in %s" % SSH_DIR)
    expect_in("sshconfig", listing, "the ssh config in %s" % SSH_DIR)


@test("the private host key is not world readable", needs=("ls",))
def host_key_permissions(t):
    """
    A host key that anyone can read lets any account on the device impersonate
    the server. This is the same rule the user store is already held to.
    """
    listing = t.run("ls %s" % SSH_DIR)
    if "CmdErr" in listing:
        raise Skip("no %s on this target" % SSH_DIR)

    # Anything here that is not a public key or the config is private material,
    # whatever the algorithm. Naming the files to check instead would pass a
    # board whose exposed key is of a type the list does not mention.
    for line in listing.splitlines():
        fields = line.split()
        if len(fields) < 2:
            continue

        mode = fields[0]
        name = fields[-1]
        if mode.startswith("d") or name.endswith(".pub") or name == "sshconfig":
            continue

        # drwxr-xr-x style; the last triplet is other
        if len(mode) >= 10 and mode[7:10] != "---":
            raise AssertionError("%s is readable by other: %s" % (name, mode))


@test("the ssh config declares both authentication methods", needs=("cat",))
def ssh_config_contents(t):
    body = t.run("cat %s/sshconfig" % SSH_DIR)
    if "CmdErr" in body or "no such" in body.lower():
        raise Skip("no sshconfig on this target")

    expect_in("PasswordAuthentication", body, "the password auth switch")
    expect_in("PubkeyAuthentication", body, "the pubkey auth switch")


@test("the server presents an ed25519 host key")
def server_host_key_type(t):
    require_ssh(t)

    shell = ssh_dial(t, password=t.password)
    try:
        key = shell._client.get_transport().get_remote_server_key()
        if ED25519_PREFIX != key.get_name():
            raise AssertionError("the server offered %s, not %s"
                                 % (key.get_name(), ED25519_PREFIX))
    finally:
        shell.close()


@test("the host key is the same on every connection")
def host_key_is_stable(t):
    """
    A host key regenerated per boot or per connection would train every client
    to accept a changed key, which is the warning that makes ssh worth using.
    """
    require_ssh(t)

    seen = []
    for _ in range(2):
        shell = ssh_dial(t, password=t.password)
        try:
            seen.append(shell._client.get_transport().get_remote_server_key().asbytes())
        finally:
            shell.close()

    if seen[0] != seen[1]:
        raise AssertionError("the host key changed between two connections")


@test("an authorized public key is accepted without a password",
      needs=("echo", "cat", "mkdir", "rm"))
def pubkey_auth_accepted(t):
    import shutil
    import tempfile

    require_ssh(t)

    workdir = tempfile.mkdtemp(prefix="pdi-ssh-")
    keypath, authorized = generate_ed25519(workdir, "authorized")

    install_authorized_key(t, authorized)
    try:
        # no password is passed at all, so only the key can open this
        shell = ssh_dial(t, key_filename=keypath)
        try:
            shell.attach(t.username, t.password, timeout=max(t.timeout, 30.0))
            expect_in(t.username, shell.run("whoami", t.timeout),
                      "a session opened with a key alone")
        finally:
            shell.close()
    finally:
        remove_authorized_keys(t)
        shutil.rmtree(workdir, ignore_errors=True)


@test("a public key that is not authorized is refused",
      needs=("echo", "cat", "mkdir", "rm"))
def pubkey_auth_refused(t):
    """
    Both halves are checked here on purpose. Asserting only that a stranger's
    key fails would pass just as well on a server where key auth never works
    at all, so the authorized key is proven to open a session first, and only
    then is the stranger's key tried against the same authorized_keys.
    """
    import shutil
    import tempfile

    from ..driver.shell import ShellError

    require_ssh(t)

    workdir = tempfile.mkdtemp(prefix="pdi-ssh-")
    good_path, good_line = generate_ed25519(workdir, "authorized")
    stranger_path, _ = generate_ed25519(workdir, "stranger")

    install_authorized_key(t, good_line)
    try:
        opened = ssh_dial(t, key_filename=good_path)
        opened.close()

        try:
            shell = ssh_dial(t, key_filename=stranger_path)
        except ShellError as err:
            if not is_auth_failure(err):
                raise AssertionError("the stranger's key did not get an answer at "
                                     "all, so nothing was proven: %s" % err)
            return

        shell.close()
        raise AssertionError("a key that is not in authorized_keys was accepted")
    finally:
        remove_authorized_keys(t)
        shutil.rmtree(workdir, ignore_errors=True)


@test("sshkgen writes a key pair where it is told", needs=("sshkgen", "ls", "rm"))
def sshkgen_creates_a_key(t):
    """
    Generating into a scratch directory, never into /etc/ssh: replacing the
    host key of a board someone is using would invalidate every client's known
    hosts entry.
    """
    path = t.workspace("ws_sshkey")

    out = t.run("sshkgen t=1,f=%s" % path, timeout=max(t.timeout, 60))
    if "CmdErr" in out and "not found" in out:
        raise Skip("no sshkgen on this target")

    listing = t.run("ls %s" % path)
    expect_in("ed25519", listing, "a generated ed25519 key in %s" % path)


@test("the number of concurrent sessions is bounded")
def session_pool_is_bounded(t):
    """
    A board cannot serve unbounded sessions, and the bound must be enforced by
    refusing rather than by degrading the ones already open. Whatever the limit
    is, the sessions opened up to it keep working afterwards.
    """
    opened = []
    try:
        for _ in range(6):
            try:
                shell = ssh_dial(t, attempts=1, password=t.password)
                shell.attach(t.username, t.password, timeout=min(t.timeout, 12.0))
            except Exception:
                break
            opened.append(shell)

        if not opened:
            raise Skip("no ssh session could be opened")

        if len(opened) >= 6:
            raise AssertionError("the session pool accepted 6 sessions without a bound")

        # the sessions inside the bound must be unharmed by the refusal
        expect_in(t.username, opened[0].run("whoami", t.timeout),
                  "the first session still works after the pool filled")
    finally:
        for shell in opened:
            try:
                shell.close()
            except Exception:
                pass


def _transport_with_mac(t, mac, attempts=4):
    """An authenticated ssh transport that will only accept `mac`, so a
    successful auth proves the server negotiated exactly that algorithm.

    paramiko needs the board's curve25519 name registered (ssh_shell does it),
    and a small ssh pool means a half-open retry can be refused until an earlier
    attempt is reaped, so the connection is retried before it is believed."""
    import socket
    import time
    import paramiko
    from ..driver import ssh_shell  # noqa: F401  registers curve25519

    last = None
    for attempt in range(attempts):
        try:
            sock = socket.create_connection((t.address(), 22), timeout=15)
            transport = paramiko.Transport(sock)
            transport.get_security_options().digests = (mac,)
            transport.start_client(timeout=15)
            transport.auth_password(t.username, t.password)
            return transport
        except Exception as err:
            last = err
            if attempt + 1 < attempts:
                time.sleep(12)

    raise AssertionError("could not negotiate %s in %d tries: %s" % (mac, attempts, last))


@test("the ssh server negotiates hmac-sha2-256 with a modern client")
def ssh_negotiates_sha2_256(t):
    require_ssh(t)
    try:
        import paramiko  # noqa: F401
    except ImportError as err:
        raise Skip("paramiko is not installed: %s" % err)

    try:
        transport = _transport_with_mac(t, "hmac-sha2-256")
    except Exception as err:
        raise AssertionError("the server would not negotiate hmac-sha2-256: %s" % err)

    try:
        if not transport.is_authenticated():
            raise AssertionError("hmac-sha2-256 was offered but the session did not authenticate")
        negotiated = (getattr(transport, "remote_mac", "") or "") + (getattr(transport, "local_mac", "") or "")
        if "sha2-256" not in negotiated:
            raise AssertionError("the negotiated mac was %r, not hmac-sha2-256" % negotiated)
    finally:
        transport.close()


@test("the ssh server still accepts a client that offers only hmac-sha1")
def ssh_falls_back_to_sha1(t):
    require_ssh(t)
    try:
        import paramiko  # noqa: F401
    except ImportError as err:
        raise Skip("paramiko is not installed: %s" % err)

    try:
        transport = _transport_with_mac(t, "hmac-sha1")
    except Exception as err:
        raise AssertionError("the server would not fall back to hmac-sha1: %s" % err)

    try:
        if not transport.is_authenticated():
            raise AssertionError("hmac-sha1 was offered but the session did not authenticate")
    finally:
        transport.close()
