#!/usr/bin/env python3

"""
The web portal: who it lets in, and what it lets them do.

Driven over http rather than through the shell, but the shell is still here —
it is what says whether the portal really did what it reported. A file said to
be uploaded is read back on the target; a file said to be deleted is looked for
there. That pairing is the point: neither view alone can tell a working portal
from one that answers politely and does nothing.

Skipped whole when the target serves no http port, so a build without the web
server reports that rather than a page of failures.
"""

import socket
import time

from .registry import test, expect_in, expect_not_in, Skip

# every settings page the portal can carry; a build without a service simply
# does not register its route, and that answers 404
SETTINGS_PAGES = (
    ("/dashboard", "the dashboard"),
    ("/wifi-config", "the wifi settings"),
    ("/login-config", "the change password page"),
    ("/ota-config", "the firmware page"),
    ("/email-config", "the email settings"),
    ("/mqtt-manage", "the mqtt settings"),
    ("/gpio-manage", "the gpio settings"),
    ("/device-register-config", "the iot registration page"),
    ("/storage", "the storage browser"),
)

# a token of the right shape that was never issued
FORGED_TOKEN = "deadbeef" * 4


def a_page(portal, path, what):
    answer = portal.get(path)
    if 404 == answer.status:
        raise Skip("%s is not in this build" % what)

    if 200 != answer.status:
        raise AssertionError("%s answered %d" % (what, answer.status))

    expect_in("<html", answer.body, what)
    return answer


def upload(t, portal, name, blob, location="/storage"):
    """Put a file on the target through the portal and return the response."""
    return portal.post_multipart(
        "/storage-fileupload",
        {"csrf": portal.csrf(), "loc": location},
        [("nf", name, blob)],
    )


def listing(portal, path=""):
    """The storage browser's own view of a directory."""
    answer = portal.get("/storage-filelist%s" % ("?cp=%s" % path if path else ""))
    if 200 != answer.status:
        raise AssertionError("the file list answered %d" % answer.status)

    return answer.json()


@test("an unauthenticated request for a settings page is sent to the login form")
def unauthenticated_is_redirected(t):
    portal = t.portal(login=False)

    answer = portal.get("/dashboard")
    if answer.status not in (301, 302, 303, 307, 308):
        raise AssertionError("an unauthenticated dashboard answered %d, not a redirect"
                             % answer.status)

    expect_in("/login", answer.location, "where an unauthenticated client is sent")


@test("the wrong password is refused and issues no session")
def wrong_password_refused(t):
    portal = t.portal(login=False)

    portal.login(t.username, "not-the-password")
    if portal.session_cookie() is not None:
        raise AssertionError("the portal issued a session for a wrong password")

    # and the failure must not have locked out the account for the next test
    portal.forget()
    portal.login(t.username, t.password)
    if portal.session_cookie() is None:
        raise AssertionError("the right password was refused after a wrong one")


@test("logging in issues a session cookie a script cannot read")
def session_cookie_attributes(t):
    portal = t.portal(login=False)

    answer = portal.login(t.username, t.password)
    if not answer.cookies:
        raise AssertionError("logging in set no cookie (status %d)" % answer.status)

    issued = answer.cookies[0]
    for attribute in ("HttpOnly", "Path=/", "SameSite"):
        expect_in(attribute, issued, "the session cookie")


@test("a token that was never issued is not a session")
def forged_token_refused(t):
    portal = t.portal(login=False)

    # the name the portal itself uses, learned by logging in and out again
    portal.login(t.username, t.password)
    name = None
    for key, value in portal.jar.items():
        if "session" in key:
            name = key
    portal.logout()

    if name is None:
        raise Skip("the portal named no session cookie")

    portal.jar = {name: FORGED_TOKEN}
    answer = portal.get("/dashboard")

    if 200 == answer.status:
        raise AssertionError("a forged token was accepted as a session")


@test("logging out stops the cookie working")
def logout_ends_the_session(t):
    portal = t.portal()

    if 200 != portal.get("/dashboard").status:
        raise AssertionError("the dashboard was not reachable while logged in")

    held = dict(portal.jar)
    portal.logout()

    # the cookie the browser was holding, replayed after the logout
    portal.jar = held
    answer = portal.get("/dashboard")

    if 200 == answer.status:
        raise AssertionError("the session survived a logout")


@test("two logins are two different sessions")
def sessions_are_distinct(t):
    first = t.portal()
    token = first.session_cookie()
    first.logout()

    second = t.portal()
    if second.session_cookie() == token:
        raise AssertionError("a second login reissued the same token")


@test("an unknown path does not leave its session behind for the next client")
def session_does_not_outlive_the_request(t):
    """
    A request for something that is not a route is answered by the not found
    handler, which resolves the session like any other. If that session is
    still current when the next request arrives, a client with no cookie at all
    is served as whoever asked last — including files it should never see.
    """
    portal = t.portal()
    portal.get("/no-such-path-here")

    stranger = t.portal(login=False)
    answer = stranger.get("/dashboard")

    if 200 == answer.status:
        raise AssertionError("a client with no cookie was served the dashboard")


@test("every settings page in the build renders")
def settings_pages_render(t):
    portal = t.portal()

    rendered = 0
    for path, what in SETTINGS_PAGES:
        try:
            a_page(portal, path, what)
            rendered += 1
        except Skip:
            continue

    if 0 == rendered:
        raise Skip("this build carries no settings pages")


@test("the firmware page offers both ways to flash")
def firmware_page_offers_both_routes(t):
    """
    Two paths to new firmware: fetched from an ota host, or flashed from an
    image already on storage. The page is only read here — nothing on a real
    board is ever asked to flash by the suite.
    """
    portal = t.portal()

    page = a_page(portal, "/ota-config", "the firmware page")
    expect_in("action='/ota-config'", page.body, "the ota host form")
    expect_in("Flash From Storage", page.body, "the flash from storage section")
    expect_in("csrf", page.body, "the firmware form carries a csrf token")


@test("an unknown page is reported as not found")
def unknown_page_is_not_found(t):
    portal = t.portal()

    answer = portal.get("/no-such-page-at-all")
    if 404 != answer.status:
        raise AssertionError("an unknown page answered %d" % answer.status)


@test("a state changing post without a csrf token is refused")
def post_without_csrf_refused(t):
    portal = t.portal()

    answer = portal.post("/login-config", {
        "cpswd": t.password,
        "npswd": "no-such-password",
        "rpswd": "no-such-password",
    })

    if 200 == answer.status:
        raise AssertionError("a post with no csrf token was accepted")

    # and the password it tried to set must not work
    other = t.portal(login=False)
    other.login(t.username, "no-such-password")
    if other.session_cookie() is not None:
        raise AssertionError("the refused post changed the password anyway")


@test("a state changing post with the wrong csrf token is refused")
def post_with_wrong_csrf_refused(t):
    portal = t.portal()

    answer = portal.post("/login-config", {
        "csrf": FORGED_TOKEN,
        "cpswd": t.password,
        "npswd": "no-such-password",
        "rpswd": "no-such-password",
    })

    if 200 == answer.status:
        raise AssertionError("a post with a forged csrf token was accepted")

    other = t.portal(login=False)
    other.login(t.username, "no-such-password")
    if other.session_cookie() is not None:
        raise AssertionError("the refused post changed the password anyway")


@test("a settings page given its fields in a GET query saves nothing")
def get_query_saves_nothing(t):
    from .test_mqtt import read_form
    from urllib.parse import urlencode

    portal = t.portal()
    tried = 0
    for path, change in (("/gpio-server", {"hst": "http://get.example", "prt": "81", "frq": "9"}),
                         ("/ota-config", {"hst": "http://get.example", "prt": "81"}),
                         ("/mqtt-general-config", {"hst": "get.example", "prt": "1884"}),
                         ("/email-config", {"ml_dmn": "get.example", "ml_srvr": "get.example"})):
        if 404 == portal.get(path).status:
            continue
        before = read_form(portal, path)
        fields = dict(before)
        fields.update(change)
        portal.get("%s?%s" % (path, urlencode(fields)))
        after = read_form(portal, path)
        if after != before:
            raise AssertionError("a GET to %s saved the page: %r became %r" % (path, before, after))
        tried += 1

    if 0 == tried:
        raise Skip("this build serves none of the settings pages")


@test("a file is not deleted by a GET to the delete route", needs=("echo", "cat", "rm"))
def get_does_not_delete(t):
    name = "portal_get_kept.txt"
    t.run("echo kept > /%s" % name)
    try:
        t.portal().get("/storage-filedel?df=/%s&loc=/storage" % name)
        expect_in("kept", t.run("cat /%s" % name), "the file after a GET to the delete route")
    finally:
        t.run("rm /%s" % name)


@test("the csrf token is not the session token")
def csrf_is_its_own_secret(t):
    portal = t.portal()

    token = portal.csrf()
    if token == portal.session_cookie():
        raise AssertionError("the csrf token is the session token, so a cookie "
                             "that leaks carries the csrf secret with it")


@test("the storage browser lists a file made on the target", needs=("echo", "rm"))
def listing_shows_a_shell_file(t):
    portal = t.portal()
    name = "portal_seen.txt"

    t.run("rm /%s" % name)
    t.run("echo portal > /%s" % name)

    try:
        names = [item["n"] for item in listing(portal)["lst"]]
        if name not in names:
            raise AssertionError("the file list did not show %s:\n%s" % (name, names))
    finally:
        t.run("rm /%s" % name)


@test("a file name carrying markup and quotes is listed as text", needs=("echo", "rm"))
def listing_carries_an_awkward_name(t):
    portal = t.portal()
    name = '<i>q"x\\.txt'
    quoted = "'/%s'" % name

    t.run("rm %s" % quoted)
    t.run("echo portal > %s" % quoted)

    try:
        if name not in t.run("ls /"):
            raise Skip("the shell could not make %r on this target" % name)

        names = [item["n"] for item in listing(portal)["lst"]]
        if name not in names:
            raise AssertionError("the file list did not carry %r whole:\n%s" % (name, names))
    finally:
        t.run("rm %s" % quoted)


@test("the storage page writes listed names as text, never as markup")
def storage_page_never_parses_a_name(t):
    page = t.portal().get("/storage")
    if 404 == page.status:
        raise Skip("this build serves no storage browser")

    # a name is the one value here a user other than the viewer chooses, so
    # a name parsed as markup runs that user's script with the viewer's session
    for at in [i for i in range(len(page.body)) if page.body.startswith("innerHTML=", i)]:
        assigned = page.body[at + len("innerHTML="):at + len("innerHTML=") + 40]
        if "it.n" in assigned or "it.l" in assigned or "location" in assigned or assigned.startswith("`"):
            raise AssertionError("the page parses listed text as markup: innerHTML=%s" % assigned)


@test("the storage browser reports permissions and ownership", needs=("echo", "rm"))
def listing_reports_permissions(t):
    portal = t.portal()
    name = "portal_perms.txt"

    t.run("rm /%s" % name)
    t.run("echo portal > /%s" % name)

    try:
        row = None
        for item in listing(portal)["lst"]:
            if item["n"] == name:
                row = item

        if row is None:
            raise AssertionError("the file list did not show %s" % name)

        if "p" not in row or len(row["p"]) < 9:
            raise AssertionError("no permission string in the listing row: %s" % row)

        if "o" not in row:
            raise Skip("this build reports no ownership in the file list")

        if not row["o"]:
            raise AssertionError("the listing reported an empty owner: %s" % row)
    finally:
        t.run("rm /%s" % name)


@test("a file uploaded through the portal is on the target", needs=("cat", "rm"))
def upload_reaches_the_target(t):
    portal = t.portal()
    name = "portal_up.txt"

    t.run("rm /%s" % name)
    try:
        answer = upload(t, portal, name, b"uploaded by the portal\n")
        if answer.status not in (200, 301, 302, 303):
            raise AssertionError("the upload answered %d" % answer.status)

        expect_in("uploaded by the portal", t.run("cat /%s" % name),
                  "what the target holds after an upload")
    finally:
        t.run("rm /%s" % name)


@test("an uploaded file comes back byte for byte", needs=("rm",))
def upload_is_byte_exact(t):
    """
    A nul in the middle, a bare carriage return and a line ending that is not
    the platform's: each one of these has been lost by a parser that treated
    the body as text. The file is fetched back over http because that is the
    only view that can show every byte.
    """
    portal = t.portal()
    name = "portal_bin.dat"
    blob = b"head\x00\r\nmid\xff\xfe\x00tail\r\n" + bytes(range(256))

    t.run("rm /%s" % name)
    try:
        upload(t, portal, name, blob)

        fetched = portal.get("/%s" % name)
        if 200 != fetched.status:
            raise Skip("this build does not serve stored files back over http")

        if fetched.raw != blob:
            raise AssertionError("the file changed in transit: sent %d bytes, "
                                 "got %d back\nsent: %r\ngot:  %r"
                                 % (len(blob), len(fetched.raw),
                                    blob[:48], fetched.raw[:48]))
    finally:
        t.run("rm /%s" % name)


@test("a file uploaded through the portal belongs to whoever uploaded it",
      needs=("rm",))
def upload_is_owned_by_the_uploader(t):
    portal = t.portal()
    name = "portal_owned.txt"

    t.run("rm /%s" % name)
    try:
        upload(t, portal, name, b"owned\n")

        row = None
        for item in listing(portal)["lst"]:
            if item["n"] == name:
                row = item

        if row is None:
            raise AssertionError("the uploaded file is not in the listing")

        if "o" not in row:
            raise Skip("this build reports no ownership in the file list")

        if row["o"] != t.username:
            raise AssertionError("the upload landed owned by %r, not %r"
                                 % (row["o"], t.username))
    finally:
        t.run("rm /%s" % name)


@test("a file deleted through the portal is gone from the target",
      needs=("echo", "cat", "rm"))
def delete_removes_the_file(t):
    portal = t.portal()
    name = "portal_del.txt"

    t.run("rm /%s" % name)
    t.run("echo doomed > /%s" % name)

    try:
        answer = portal.post("/storage-filedel", {
            "csrf": portal.csrf(),
            "df": "/%s" % name,
            "loc": "/storage",
        })

        if answer.status not in (200, 301, 302, 303):
            raise AssertionError("the delete answered %d" % answer.status)

        left = t.run("cat /%s" % name)
        expect_not_in("doomed", left, "the file after the portal deleted it")
    finally:
        t.run("rm /%s" % name)


@test("a delete the logged in user may not do is refused",
      needs=("useradd", "userdel", "echo", "chmod", "cat", "rm"))
def delete_honours_permissions(t):
    """
    The portal runs a file operation as the user whose cookie it carries, so a
    file that user cannot touch from the shell must not be removable from the
    browser either.
    """
    name = "portal_guarded.txt"
    account = "portalusr"

    t.run("userdel u=%s" % account)
    out = t.run("useradd u=%s p=%s" % (account, "portalpw1"))
    if "root required" in out:
        raise Skip("not root on this target")
    if "added" not in out and "exists" not in out:
        raise Skip("this target would not make a scratch account: %s" % out)

    t.run("rm /%s" % name)
    t.run("echo guarded > /%s" % name)
    t.run("chmod 600 /%s" % name)

    try:
        portal = t.portal(username=account, password="portalpw1")

        answer = portal.post("/storage-filedel", {
            "csrf": portal.csrf(),
            "df": "/%s" % name,
            "loc": "/storage",
        })

        if answer.status not in (200, 301, 302, 303):
            raise AssertionError("the delete answered %d" % answer.status)

        expect_in("guarded", t.run("cat /%s" % name),
                  "a file the portal user may not delete")
    finally:
        t.run("rm /%s" % name)
        t.run("userdel u=%s" % account)


def drain(sock, quiet=1.0):
    """Read until the peer has said nothing for a while, and return what came."""
    got = b""
    sock.settimeout(quiet)
    while True:
        try:
            chunk = sock.recv(1024)
        except socket.timeout:
            return got
        if not chunk:
            return got
        got += chunk


@test("a plain http request is answered while a secure client idles on the server")
def plain_http_not_starved(t):
    """
    The server serves one client at a time and hands an idle keep-alive client
    over when someone else is waiting. A browser that has just loaded a secure
    page keeps that connection open, and a request to port 80 behind it was only
    answered once the keep-alive ran out, thirty seconds later. The secure
    connection is held idle here, as the browser holds it, and the plain request
    has to be answered well inside that window.
    """
    from ..driver.portal import _tls_context, DEFAULT_PORT

    portal = t.portal(login=False)
    if not portal.secure:
        raise Skip("this target serves plain http, so there is no second port to wait on")

    budget = 10.0
    held = _tls_context().wrap_socket(
        socket.create_connection((portal.host, portal.port), timeout=portal.timeout))
    plain = None
    try:
        held.sendall(b"GET /login HTTP/1.1\r\nHost: %s\r\nConnection: keep-alive\r\n\r\n"
                     % portal.host.encode())
        if b"HTTP/1." not in drain(held):
            raise AssertionError("the secure connection was never answered")

        began = time.time()
        plain = socket.create_connection((portal.host, DEFAULT_PORT), timeout=budget)
        plain.sendall(b"GET / HTTP/1.1\r\nHost: %s\r\nConnection: close\r\n\r\n"
                      % portal.host.encode())
        try:
            first = plain.recv(64)
        except socket.timeout:
            raise AssertionError("port %d answered nothing in %.0f s while a secure client idled"
                                 % (DEFAULT_PORT, budget))

        waited = time.time() - began
        if not first.startswith(b"HTTP/1."):
            raise AssertionError("port %d answered %r" % (DEFAULT_PORT, first))
        if waited >= budget:
            raise AssertionError("port %d took %.1f s while a secure client idled"
                                 % (DEFAULT_PORT, waited))
    finally:
        held.close()
        if plain is not None:
            plain.close()
