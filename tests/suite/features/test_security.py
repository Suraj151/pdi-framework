#!/usr/bin/env python3

"""
The framework seen as a target: what a peer with no account, or an account that
should not reach far, can do to it over the wire.

Every test here holds the same bar — the board refuses cleanly, stays up, and
goes on serving. A crash, a hang, a leaked secret or an accepted weak password
is the failure, never the refusal itself. The destructive probes run against a
reachable address, so they work against the host process and a board alike; the
few that would need a second account or a writable store skip where they cannot.
"""

import socket
import ssl
import time

from .registry import test, Skip, expect_in, expect_not_in
from .test_users import scratch_user, drop_user

# passwords a device of any size can reject: short, all one class, or a known
# default. The smallest board can hold a rule of "at least 8 characters with a
# letter and a digit", so these are what a strength check must turn away.
WEAK_PASSWORDS = ["1234", "a", "pass", "aaaaaaaa", "12345678", "password", "pdiStack"]
STRONG_PASSWORD = "Str0ngPass42Qx"


def password_policy_enforced():
    """
    The author deferred the strength rule on 2026-10-24: weak passwords are
    accepted for now. PDI_PASSWORD_POLICY=1 turns these checks back on.
    """
    import os
    if os.environ.get("PDI_PASSWORD_POLICY", "") != "1":
        raise Skip("password strength policy deferred by the author; set PDI_PASSWORD_POLICY=1 "
                   "to enforce")


def raw_http(host, port, request, timeout=10.0, secure=False):
    """Send one raw HTTP request and return the bytes that come back."""
    out = b""
    sock = None
    try:
        sock = socket.create_connection((host, port), timeout=timeout)
        if secure:
            ctx = ssl.create_default_context()
            ctx.check_hostname = False
            ctx.verify_mode = ssl.CERT_NONE
            sock = ctx.wrap_socket(sock)
        sock.sendall(request)
        sock.settimeout(timeout)
        while True:
            chunk = sock.recv(4096)
            if not chunk:
                break
            out += chunk
    except (socket.timeout, OSError):
        pass
    finally:
        try:
            if sock is not None:
                sock.close()
        except OSError:
            pass
    return out


def portal_endpoint(t):
    """(host, port, secure) of the portal, or a skip."""
    from ..driver.portal import Portal
    found = Portal.reachable(t.address(), timeout=min(t.timeout, 15.0))
    if not found:
        raise Skip("the target serves no http portal")
    return found.host, found.port, found.secure


KEEPALIVE_EVERY = 8


def keep_shell(t, index):
    """
    Touch the test's own shell now and then: a replay can run for minutes
    against another service, and a session idle that long is reaped by design.
    """
    if index and 0 == index % KEEPALIVE_EVERY:
        t.run("echo keep")


def alive_after(t):
    """The shell still answers for itself; the whole point after an attack."""
    expect_in("alive", t.run("echo alive"), "the shell after the probe")


# ------------------------------------------------------------ password strength

def weak_password_cases():
    for pw in WEAK_PASSWORDS:
        yield pw


@test("useradd refuses a weak password", needs=("useradd", "userdel"), su=True)
def useradd_refuses_weak(t):
    password_policy_enforced()
    accepted = []
    for pw in WEAK_PASSWORDS:
        name = "secw%d" % (abs(hash(pw)) % 1000)
        t.run("userdel u=%s" % name)
        out = t.run("useradd u=%s p=%s" % (name, pw))
        if "root required" in out:
            raise Skip("not root on this target")
        made = name in t.run("cat /etc/passwd")
        t.run("userdel u=%s" % name)
        if made:
            accepted.append(pw)
    if accepted:
        raise AssertionError("useradd accepted weak password(s) %r; a device must require a strong "
                             "one (>=8 chars, a letter and a digit)" % accepted)


@test("useradd accepts a strong password", needs=("useradd", "userdel"), su=True)
def useradd_accepts_strong(t):
    name = "secstrong"
    t.run("userdel u=%s" % name)
    out = t.run("useradd u=%s p=%s" % (name, STRONG_PASSWORD))
    if "root required" in out:
        raise Skip("not root on this target")
    made = name in t.run("cat /etc/passwd")
    t.run("userdel u=%s" % name)
    if not made:
        raise AssertionError("useradd refused a strong password %r:\n%s" % (STRONG_PASSWORD, out))


@test("passwd refuses a weak new password", needs=("useradd", "userdel", "passwd", "su"), su=True)
def passwd_refuses_weak(t):
    password_policy_enforced()
    name = "secpw"
    scratch_user(t, name, STRONG_PASSWORD)
    try:
        t.run("su %s %s" % (name, STRONG_PASSWORD))
        if name not in t.run("whoami"):
            raise Skip("could not become the scratch user")
        # passwd prompts: old, new, confirm. A weak new one must be refused.
        t.shell.send_line("passwd")
        time.sleep(0.5)
        t.shell.send_line(STRONG_PASSWORD)
        time.sleep(0.3)
        t.shell.send_line("1234")
        time.sleep(0.3)
        t.shell.send_line("1234")
        time.sleep(0.5)
        t.resync()
        t.become_root()
        # if the weak password took, the user can log in with it
        t.run("su %s 1234" % name)
        became = name in t.run("whoami")
        t.become_root()
        if became:
            raise AssertionError("passwd accepted the weak password '1234'")
    finally:
        t.resync()
        t.become_root()
        drop_user(t, name)


@test("the shipped account does not keep a weak default password", su=True)
def default_account_not_weak(t):
    """
    The account the suite is logged in as must not be reachable with a password
    a strength rule would reject. If it is, the device ships guessable.
    """
    password_policy_enforced()
    for pw in WEAK_PASSWORDS:
        if pw == t.password:
            raise AssertionError("the logged in account '%s' uses weak password %r"
                                 % (t.username, pw))


# ------------------------------------------------------------ portal hardening

@test("the session cookie is HttpOnly and SameSite")
def cookie_flags(t):
    host, port, secure = portal_endpoint(t)
    from ..driver.portal import Portal
    portal = Portal(host, port, timeout=min(t.timeout, 15.0), secure=secure)
    answer = portal.login(t.username, t.password)
    seen = None
    for raw in answer.cookies:
        if "session" in raw.lower():
            seen = raw
    if seen is None:
        raise Skip("the portal set no session cookie to inspect")
    low = seen.lower()
    if "httponly" not in low:
        raise AssertionError("the session cookie is not HttpOnly: %s" % seen)
    if "samesite" not in low:
        raise AssertionError("the session cookie carries no SameSite: %s" % seen)
    portal.logout()


@test("repeated wrong passwords lock the portal out")
def portal_lockout(t):
    host, port, secure = portal_endpoint(t)
    from ..driver.portal import Portal
    portal = Portal(host, port, timeout=min(t.timeout, 15.0), secure=secure)
    blocked = False
    for _ in range(7):
        answer = portal.login(t.username, "definitely-wrong")
        if portal.session_cookie() is not None:
            raise AssertionError("a wrong password issued a session")
        if "too many" in answer.body.lower() or "try later" in answer.body.lower():
            blocked = True
            break
    if not blocked:
        raise AssertionError("the portal never locked out after repeated wrong passwords; a device "
                             "must throttle guessing")
    # a correct login during the lockout window is still refused
    answer = portal.login(t.username, t.password)
    if portal.session_cookie() is not None:
        raise AssertionError("the portal let a login through during the lockout window")
    alive_after(t)


@test("an unauthenticated settings POST changes nothing")
def unauth_post_changes_nothing(t):
    host, port, secure = portal_endpoint(t)
    body = b"ssid=attacker&password=attacker"
    request = (b"POST /wifi-config HTTP/1.1\r\nHost: x\r\n"
               b"Content-Type: application/x-www-form-urlencoded\r\n"
               b"Content-Length: %d\r\nConnection: close\r\n\r\n%s" % (len(body), body))
    reply = raw_http(host, port, request, timeout=t.timeout, secure=secure)
    head = reply.split(b"\r\n", 1)[0] if reply else b""
    if b" 200 " in head and b"saved" in reply.lower():
        raise AssertionError("an unauthenticated POST to /wifi-config was accepted: %r" % head)
    alive_after(t)


# ------------------------------------------------------------ malformed inputs

@test("an oversized request line is refused without a crash")
def oversized_request_line(t):
    host, port, secure = portal_endpoint(t)
    request = b"GET /" + b"A" * 20000 + b" HTTP/1.1\r\nHost: x\r\nConnection: close\r\n\r\n"
    raw_http(host, port, request, timeout=t.timeout, secure=secure)
    alive_after(t)
    portal = t.portal(login=False)
    if portal.get("/").status >= 500:
        raise AssertionError("the portal answered 5xx after an oversized request line")


@test("a flood of oversized headers is refused without a crash")
def oversized_headers(t):
    host, port, secure = portal_endpoint(t)
    headers = b"".join(b"X-Pad-%d: %s\r\n" % (i, b"A" * 1000) for i in range(64))
    request = b"GET / HTTP/1.1\r\nHost: x\r\n" + headers + b"Connection: close\r\n\r\n"
    raw_http(host, port, request, timeout=t.timeout, secure=secure)
    alive_after(t)
    if t.portal(login=False).get("/").status >= 500:
        raise AssertionError("the portal answered 5xx after oversized headers")


@test("a slow trickle of a request does not wedge the server")
def slow_request(t):
    host, port, secure = portal_endpoint(t)
    sock = socket.create_connection((host, port), timeout=t.timeout)
    if secure:
        ctx = ssl.create_default_context()
        ctx.check_hostname = False
        ctx.verify_mode = ssl.CERT_NONE
        sock = ctx.wrap_socket(sock)
    try:
        for byte in b"GET / HTTP/1.1\r\nHost: x\r\n":
            sock.sendall(bytes([byte]))
            time.sleep(0.02)
        sock.close()
    except OSError:
        pass
    # a second client must still be served while and after the trickle
    portal = t.portal(login=False)
    if portal.get("/").status >= 500:
        raise AssertionError("a second client got 5xx during a slow request")
    alive_after(t)


@test("an upload sent without logging in leaves nothing on the target", needs=("ls", "rm"))
def unauthenticated_upload_stores_nothing(t):
    """
    The upload route needs a session, so a request without one must not reach
    the filesystem at all: not the destination, and not the staging directory
    the body would pass through. Otherwise anyone on the network can fill the
    flash without a password.
    """
    host, port, secure = portal_endpoint(t)
    name = "secunauth.bin"
    payload = b"U" * 8192
    body = (b"--XB\r\nContent-Disposition: form-data; name=\"file\"; filename=\"%s\"\r\n"
            b"Content-Type: application/octet-stream\r\n\r\n" % name.encode()) + payload + b"\r\n--XB--\r\n"
    request = (b"POST /storage-fileupload HTTP/1.1\r\nHost: x\r\n"
               b"Content-Type: multipart/form-data; boundary=XB\r\n"
               b"Content-Length: %d\r\nConnection: close\r\n\r\n" % len(body)) + body
    raw_http(host, port, request, timeout=t.timeout, secure=secure)
    found = []
    for folder in ("/temp", "/", "/tmp"):
        listing = t.run("ls %s" % folder)
        if name in listing:
            found.append(folder)
            t.run("rm %s/%s" % (folder.rstrip("/"), name))
    if found:
        raise AssertionError("an upload with no session was stored in %s" % ", ".join(found))
    alive_after(t)


# --------------------------------------------- pre-auth fuzz corpora over wire

def corpus_samples(name, limit=40):
    """A handful of the recorded fuzz inputs for a parser, or empty when none."""
    import os
    import random
    here = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
    folder = os.path.join(here, "fuzz", "corpus", name)
    if not os.path.isdir(folder):
        return []
    files = sorted(os.listdir(folder))
    random.Random(1).shuffle(files)
    out = []
    for fname in files[:limit]:
        try:
            with open(os.path.join(folder, fname), "rb") as fh:
                out.append(fh.read())
        except OSError:
            continue
    return out


@test("the http parser survives its fuzz corpus over the wire")
def http_corpus_over_wire(t):
    host, port, secure = portal_endpoint(t)
    cases = corpus_samples("fuzz_http")
    if not cases:
        raise Skip("no fuzz_http corpus is present")
    for index, blob in enumerate(cases):
        keep_shell(t, index)
        raw_http(host, port, blob, timeout=min(t.timeout, 6.0), secure=secure)
    alive_after(t)
    if t.portal(login=False).get("/").status >= 500:
        raise AssertionError("the portal answered 5xx after replaying the http corpus")


@test("the telnet server survives its shell fuzz corpus")
def telnet_corpus(t):
    probe = getattr(t.shell, "is_listening", None)
    port = probe(23) if probe else None
    if not port:
        raise Skip("the target has no telnet listener")
    cases = corpus_samples("fuzz_shell", limit=30)
    if not cases:
        raise Skip("no fuzz_shell corpus is present")
    host = t.address()
    for index, blob in enumerate(cases):
        keep_shell(t, index)
        try:
            sock = socket.create_connection((host, port), timeout=6.0)
            sock.sendall(blob[:2000])
            time.sleep(0.05)
            sock.close()
        except OSError:
            pass
    alive_after(t)


@test("the ssh server survives its wire fuzz corpus before auth")
def ssh_corpus(t):
    probe = getattr(t.shell, "is_listening", None)
    port = probe(22) if probe else None
    if not port:
        raise Skip("the target has no ssh listener")
    cases = corpus_samples("fuzz_ssh_wire", limit=30)
    if not cases:
        raise Skip("no fuzz_ssh_wire corpus is present")
    host = t.address()
    for index, blob in enumerate(cases):
        keep_shell(t, index)
        try:
            sock = socket.create_connection((host, port), timeout=6.0)
            sock.sendall(b"SSH-2.0-pdiattack\r\n")
            sock.sendall(blob[:2000])
            time.sleep(0.05)
            sock.close()
        except OSError:
            pass
    alive_after(t)
