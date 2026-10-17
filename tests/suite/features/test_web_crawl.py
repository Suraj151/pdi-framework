#!/usr/bin/env python3

"""
The portal crawled the way a browser meets it: every route it registers, every
link, form and script url on every page, and the HTTP underneath.

The route list is read from the source, so a route added or renamed without a
test noticing fails here rather than going unvisited. Pages are fetched over a
raw socket as well as through the driver, because the chunk framing is the
part that breaks a board: a page is sent from one fixed buffer, one chunk at a
time, and a chunk built bigger than that buffer overruns it.
"""

import json
import os
import re
import socket
import threading
import time
import urllib.parse

from .registry import test, Skip
from .test_mqtt import read_form, write_form
from ..driver.host_shell import HostShell
from ..driver import portal as portal_driver

ROUTES_FILE = os.path.join(os.path.dirname(__file__), "..", "..", "..", "src", "webserver", "routes", "Routes.h")
ROUTE = re.compile(r'#define\s+(WEB_SERVER_\w+_ROUTE)\s+"([^"]+)"')

# one chunk is one fill of the page buffer; past this the board's heap is at risk
CHUNK_LIMIT = 1200

# routes that act rather than show, and answer with a redirect or a reply of their own
ACTIONS = ("/logout", "/storage-fileupload", "/storage-filedel")
API = ("/listen-dashboard", "/listen-monitor", "/storage-filelist")
OPEN = ("/login", "/logout")

LINK = re.compile(r"""(?:href|src|action)\s*=\s*['"]([^'"#]+)['"]""", re.I)
SCRIPT_URL = re.compile(r"""(?:open\(\s*'GET'\s*,\s*|location\.href\s*=\s*)'(/[^'?]*)""")


def routes():
    with open(ROUTES_FILE) as source:
        return dict((path, name) for name, path in ROUTE.findall(source.read()))


def pages(portal):
    """Every page route the build serves, with its body, logged in."""
    found = {}
    for path in sorted(routes()):
        if path in ACTIONS or path in API:
            continue
        answer = portal.get(path)
        if 404 != answer.status:
            found[path] = answer
    return found


class Raw(object):
    """One HTTP exchange on a socket of its own, nothing smoothed over."""

    def __init__(self, portal):
        self.portal = portal

    def open(self):
        sock = socket.create_connection((self.portal.host, self.portal.port), timeout=self.portal.timeout)
        if self.portal.secure:
            sock = portal_driver._tls_context().wrap_socket(sock, server_hostname=self.portal.host)
        return sock

    def cookie(self):
        return "Cookie: %s\r\n" % self.portal._cookie_header() if self.portal.jar else ""

    def exchange(self, head, body=b"", read_for=10.0):
        sock = self.open()
        try:
            sock.sendall(head.encode() + body)
            return self.read_all(sock, read_for)
        finally:
            sock.close()

    @staticmethod
    def read_all(sock, read_for):
        data = b""
        deadline = time.time() + read_for
        sock.settimeout(1.0)
        while time.time() < deadline:
            try:
                chunk = sock.recv(8192)
            except socket.timeout:
                continue
            except OSError:
                break
            if not chunk:
                break
            data += chunk
        return data


def status_of(reply):
    first = reply.split(b"\r\n", 1)[0].decode(errors="replace")
    parts = first.split(" ")
    return int(parts[1]) if len(parts) > 1 and parts[1].isdigit() else None


def chunks_of(reply):
    """The sizes of a chunked body's chunks, or None when the body was not chunked."""
    head, _, body = reply.partition(b"\r\n\r\n")
    if b"chunked" not in head.lower():
        return None
    sizes = []
    while body:
        line, _, rest = body.partition(b"\r\n")
        size = int(line.split(b";")[0].strip() or b"0", 16)
        if 0 == size:
            break
        sizes.append(size)
        body = rest[size + 2:]
    return sizes


def still_answers(portal, what):
    fresh = portal_driver.Portal(portal.host, portal.port, timeout=portal.timeout, secure=portal.secure)
    if 200 != fresh.get("/login").status:
        raise AssertionError("the portal stopped answering after %s" % what)


@test("the route list names the routes the portal registers")
def route_list_is_read(t):
    found = routes()
    if len(found) < 20:
        raise AssertionError("only %d routes were read from %s" % (len(found), ROUTES_FILE))


@test("every route but the login form turns away a client that has not logged in")
def every_route_needs_a_login(t):
    stranger = t.portal(login=False)
    opened = []
    for path in sorted(routes()):
        if path in OPEN or "/" == path:
            continue
        answer = stranger.get(path)
        if 404 == answer.status:
            continue
        if path in API and 401 == answer.status:
            continue
        if answer.status in (301, 302, 303, 307) and "login" in answer.location:
            continue
        opened.append("%s answered %d" % (path, answer.status))
    if opened:
        raise AssertionError("a client with no session was not turned away: %s" % "; ".join(opened))


@test("every page in the build answers 200 with html once logged in")
def every_page_answers(t):
    portal = t.portal()
    bad = []
    for path, answer in pages(portal).items():
        if answer.status in (301, 302, 303) and path in ("/", "/login"):
            continue
        if 200 != answer.status or "text/html" not in answer.headers.get("content-type", ""):
            bad.append("%s: %d %s" % (path, answer.status, answer.headers.get("content-type")))
        elif "</html>" not in answer.body.lower():
            bad.append("%s ends before </html>" % path)
    if bad:
        raise AssertionError("pages that did not render whole: %s" % "; ".join(bad))


@test("every json endpoint answers json that parses")
def every_api_parses(t):
    portal = t.portal()
    portal.get("/gpio-monitor")
    tried = 0
    for path in API:
        answer = portal.get(path)
        if 404 == answer.status:
            continue
        tried += 1
        if 200 != answer.status:
            raise AssertionError("%s answered %d" % (path, answer.status))
        if "application/json" not in answer.headers.get("content-type", ""):
            raise AssertionError("%s labels its json %r" % (path, answer.headers.get("content-type")))
        try:
            json.loads(answer.body)
        except ValueError as error:
            raise AssertionError("%s is not json (%s): %r" % (path, error, answer.body[:200]))
    if 0 == tried:
        raise Skip("this build serves no json endpoint")


@test("no page sends a chunk bigger than the board's page buffer can hold")
def every_chunk_fits(t):
    portal = t.portal()
    raw = Raw(portal)
    over = []
    for path in sorted(pages(portal)):
        reply = raw.exchange("GET %s HTTP/1.1\r\nHost: %s\r\n%sConnection: close\r\n\r\n"
                             % (path, portal.host, raw.cookie()))
        sizes = chunks_of(reply)
        if sizes:
            biggest = max(sizes)
            if biggest > CHUNK_LIMIT:
                over.append("%s sent a %d byte chunk" % (path, biggest))
    if over:
        raise AssertionError("chunks past %d bytes: %s" % (CHUNK_LIMIT, "; ".join(over)))


@test("every link, script url and form action on every page answers")
def every_link_answers(t):
    portal = t.portal()
    known = set(routes())
    broken = []
    for path, answer in pages(portal).items():
        targets = set(LINK.findall(answer.body)) | set(SCRIPT_URL.findall(answer.body))
        for target in targets:
            split = urllib.parse.urlsplit(target)
            if split.scheme or split.netloc or not split.path.startswith("/"):
                continue
            if split.path in ACTIONS or split.path in ("/logout",):
                continue
            if split.path not in known:
                broken.append("%s links to %s, which is no route" % (path, split.path))
                continue
            status = portal.get(target).status
            if status >= 400 and not (split.path in API and 401 == status):
                broken.append("%s links to %s, which answered %d" % (path, target, status))
    if broken:
        raise AssertionError("; ".join(sorted(set(broken))))


@test("every settings form posted back unchanged reads back unchanged")
def every_form_round_trips(t):
    on_host = isinstance(t.shell, HostShell)
    portal = t.portal()
    tried = []
    # the wifi page restarts the radio and email sends a mail, so a board keeps them
    for path in ("/gpio-server", "/gpio-config", "/ota-config", "/mqtt-lwt-config", "/mqtt-pubsub-config",
                 "/email-config", "/wifi-config"):
        if not on_host and path in ("/wifi-config", "/email-config"):
            continue
        if 404 == portal.get(path).status:
            continue
        before = read_form(portal, path)
        if not before:
            continue
        write_form(portal, path, before)
        after = read_form(t.portal(), path)
        if after != before:
            changed = dict((k, (before.get(k), after.get(k))) for k in set(before) | set(after)
                           if before.get(k) != after.get(k))
            raise AssertionError("%s did not round trip: %r" % (path, changed))
        tried.append(path)
    if not tried:
        raise Skip("this build serves no settings form")


@test("the live feeds tell a browser not to cache them")
def live_feeds_are_not_cached(t):
    portal = t.portal()
    portal.get("/gpio-monitor")
    for path in ("/listen-dashboard", "/listen-monitor"):
        answer = portal.get(path)
        if 404 == answer.status:
            continue
        if "no-cache" not in answer.headers.get("cache-control", "") and "no-store" not in answer.headers.get("cache-control", ""):
            raise AssertionError("%s carries cache-control %r" % (path, answer.headers.get("cache-control")))


@test("a response that says the connection closes is followed by the close")
def close_means_close(t):
    portal = t.portal(login=False)
    raw = Raw(portal)
    began = time.time()
    reply = raw.exchange("GET /login HTTP/1.1\r\nHost: %s\r\nConnection: close\r\n\r\n" % portal.host, read_for=8.0)
    took = time.time() - began
    if b"connection: close" not in reply.lower():
        raise AssertionError("the reply did not say it would close: %r" % reply[:200])
    if took > 3.0:
        raise AssertionError("the server said close but held the connection %.1f s" % took)


@test("a keep-alive connection is answered twice")
def keep_alive_answers_twice(t):
    portal = t.portal(login=False)
    raw = Raw(portal)
    sock = raw.open()
    try:
        request = "GET /login HTTP/1.1\r\nHost: %s\r\nConnection: keep-alive\r\n\r\n" % portal.host
        sock.sendall(request.encode())
        first = Raw.read_all(sock, 2.0)
        sock.sendall(request.encode())
        second = Raw.read_all(sock, 2.0)
    finally:
        sock.close()
    if 200 != status_of(first):
        raise AssertionError("the first request on the connection got %r" % first[:80])
    if 200 != status_of(second):
        raise AssertionError("the second request on a kept connection got %r" % second[:80])


@test("a HEAD request is answered with no body")
def head_has_no_body(t):
    portal = t.portal(login=False)
    raw = Raw(portal)
    reply = raw.exchange("HEAD /login HTTP/1.1\r\nHost: %s\r\nConnection: close\r\n\r\n" % portal.host, read_for=4.0)
    if status_of(reply) not in (200, 405, 501):
        raise AssertionError("HEAD answered %r" % reply[:80])
    head, _, body = reply.partition(b"\r\n\r\n")
    if 200 == status_of(reply) and body.strip() and b"chunked" not in head.lower():
        raise AssertionError("HEAD carried a %d byte body" % len(body))
    still_answers(portal, "a HEAD request")


@test("a method no route accepts is refused and the portal keeps answering")
def unknown_method_refused(t):
    portal = t.portal()
    raw = Raw(portal)
    for method in ("PUT", "DELETE", "PATCH", "OPTIONS", "BREW"):
        reply = raw.exchange("%s /gpio-server HTTP/1.1\r\nHost: %s\r\n%sContent-Length: 0\r\nConnection: close\r\n\r\n"
                             % (method, portal.host, raw.cookie()), read_for=4.0)
        if 405 != status_of(reply):
            raise AssertionError("%s was answered %r, not 405" % (method, reply[:80]))
        if b"allow: get, post, head" not in reply.lower():
            raise AssertionError("the 405 for %s named no allowed methods: %r" % (method, reply[:200]))
    still_answers(portal, "unusual methods")


@test("a malformed request line is refused and the portal keeps answering")
def malformed_request_line(t):
    portal = t.portal(login=False)
    raw = Raw(portal)
    for line in ("GARBAGE\r\n\r\n", "GET\r\n\r\n", "GET / HTTP/9.9\r\n\r\n", "\r\n\r\n",
                 "GET /%s HTTP/1.1\r\n\r\n" % ("a" * 3000), "\x00\x01\x02\xff\r\n\r\n"):
        raw.exchange(line, read_for=3.0)
    still_answers(portal, "malformed request lines")


@test("an oversized header is refused and the portal keeps answering")
def oversized_header(t):
    portal = t.portal(login=False)
    raw = Raw(portal)
    raw.exchange("GET /login HTTP/1.1\r\nHost: %s\r\nX-Pad: %s\r\nConnection: close\r\n\r\n"
                 % (portal.host, "p" * 16384), read_for=4.0)
    many = "".join("X-H%d: v\r\n" % i for i in range(300))
    raw.exchange("GET /login HTTP/1.1\r\nHost: %s\r\n%sConnection: close\r\n\r\n" % (portal.host, many), read_for=4.0)
    still_answers(portal, "an oversized header")


@test("a request with no Host header is still answered")
def no_host_header(t):
    portal = t.portal(login=False)
    reply = Raw(portal).exchange("GET /login HTTP/1.0\r\n\r\n", read_for=4.0)
    if status_of(reply) not in (200, 400):
        raise AssertionError("a request with no Host was answered %r" % reply[:80])
    still_answers(portal, "a request with no Host header")


@test("a POST whose body is shorter than its Content-Length does not hang the portal")
def short_post_body(t):
    portal = t.portal()
    raw = Raw(portal)
    sock = raw.open()
    try:
        sock.sendall(("POST /gpio-server HTTP/1.1\r\nHost: %s\r\n%sContent-Type: application/x-www-form-urlencoded\r\n"
                      "Content-Length: 500\r\n\r\nhst=a" % (portal.host, raw.cookie())).encode())
        time.sleep(0.5)
        began = time.time()
        still_answers(t.portal(login=False), "a short POST body")
        if time.time() - began > 10.0:
            raise AssertionError("another client waited %.0f s behind a short POST" % (time.time() - began))
    finally:
        sock.close()


@test("a client that sends its request a byte at a time does not stall the others")
def slow_client(t):
    portal = t.portal(login=False)
    raw = Raw(portal)
    sock = raw.open()
    stop = threading.Event()

    def drip():
        try:
            for byte in ("GET /login HTTP/1.1\r\nHost: %s\r\n\r\n" % portal.host).encode():
                if stop.is_set():
                    return
                sock.sendall(bytes([byte]))
                time.sleep(0.15)
        except OSError:
            pass

    dripper = threading.Thread(target=drip, daemon=True)
    dripper.start()
    try:
        slow = 0
        for _ in range(5):
            began = time.time()
            still_answers(t.portal(login=False), "a slow client")
            if time.time() - began > 4.0:
                slow += 1
        if slow > 1:
            raise AssertionError("%d of 5 requests waited past 4 s behind a slow client" % slow)
    finally:
        stop.set()
        dripper.join(timeout=10)
        sock.close()


@test("several clients at once are all answered")
def concurrent_clients(t):
    portal = t.portal()
    results = []

    def fetch():
        try:
            results.append(portal.get("/dashboard").status)
        except Exception as error:
            results.append(repr(error))

    workers = [threading.Thread(target=fetch) for _ in range(4)]
    for worker in workers:
        worker.start()
    for worker in workers:
        worker.join(timeout=60)
    if results.count(200) != len(workers):
        raise AssertionError("four clients at once got %r" % results)


@test("a connection opened and closed without a request leaves the portal answering")
def idle_connections(t):
    portal = t.portal(login=False)
    raw = Raw(portal)
    for _ in range(10):
        raw.open().close()
    held = [raw.open() for _ in range(3)]
    try:
        still_answers(portal, "connections that sent nothing")
    finally:
        for sock in held:
            sock.close()
