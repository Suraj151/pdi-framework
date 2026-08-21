#!/usr/bin/env python3

"""
The web portal of a pdi target, driven the way a browser drives it.

One connection per request, closed straight after. The server serves a single
client at a time, so holding a keep-alive connection open between assertions
would make the next one wait for the idle timeout; and a request that arrives
while the previous client is still being served is refused rather than queued,
which is why every request is retried a few times before it is called a
failure.

Nothing here is imported unless a portal test runs, so a target without an http
server costs nothing.
"""

import http.client
import json
import re
import socket
import time
import urllib.parse

DEFAULT_PORT = 80

# a listener the host refuses at its real port moves here, see the mock
# TcpServerInterface
SHADOW_PORT_BASE = 10000

# <input type='hidden' name='csrf' value='...'> — the page builder quotes
# attributes with apostrophes
CSRF_INPUT = re.compile(r"name='csrf'\s+value='([0-9a-fA-F]+)'")


class PortalError(Exception):
    pass


class Response(object):

    def __init__(self, status, headers, raw):
        self.status = status
        self.raw = raw
        self.body = raw.decode(errors="replace")

        self.headers = {}
        self.cookies = []
        for key, value in headers:
            lowered = key.lower()
            if "set-cookie" == lowered:
                self.cookies.append(value)
            self.headers[lowered] = value

    @property
    def location(self):
        return self.headers.get("location", "")

    def json(self):
        try:
            return json.loads(self.body)
        except ValueError as err:
            raise PortalError("the response was not json: %s\n%s" % (err, self.body[:400]))

    def __repr__(self):
        return "<Response %d %d bytes>" % (self.status, len(self.body))


class Portal(object):
    """An http conversation with one target, remembering its cookies."""

    def __init__(self, host, port=DEFAULT_PORT, timeout=20.0):
        self.host = host
        self.port = port
        self.timeout = timeout
        self.jar = {}

    @classmethod
    def reachable(cls, host, port=DEFAULT_PORT, timeout=20.0):
        """
        A portal that answers, or None.

        The shadow port is tried first for the same reason the shell transports
        try it first: the machine running the tests may well have its own web
        server on port 80, and asking there first would test that one.
        """
        candidates = [port]
        if port < SHADOW_PORT_BASE:
            candidates.insert(0, SHADOW_PORT_BASE + port)

        for candidate in candidates:
            portal = cls(host, candidate, timeout)
            try:
                answer = portal.get("/")
            except (PortalError, OSError):
                continue

            if answer.status and "text/html" in answer.headers.get("content-type", ""):
                return portal

        return None

    def _cookie_header(self):
        return "; ".join("%s=%s" % pair for pair in self.jar.items())

    def _remember(self, response):
        for raw in response.cookies:
            pair = raw.split(";", 1)[0]
            if "=" not in pair:
                continue
            name, _, value = pair.partition("=")
            name = name.strip()
            if value:
                self.jar[name] = value
            else:
                self.jar.pop(name, None)

    def request(self, method, path, body=None, content_type=None, attempts=4):
        headers = {
            "Host": "%s:%d" % (self.host, self.port),
            "Connection": "close",
            "Accept": "*/*",
        }

        if self.jar:
            headers["Cookie"] = self._cookie_header()
        if content_type:
            headers["Content-Type"] = content_type
        if body is not None:
            headers["Content-Length"] = str(len(body))

        last = None
        for attempt in range(attempts):
            connection = http.client.HTTPConnection(self.host, self.port,
                                                    timeout=self.timeout)
            try:
                connection.request(method, path, body=body, headers=headers)
                raw = connection.getresponse()
                answer = Response(raw.status, raw.getheaders(), raw.read())
                self._remember(answer)
                return answer
            except (http.client.HTTPException, OSError, socket.timeout) as err:
                last = err
            finally:
                connection.close()

            if attempt + 1 < attempts:
                time.sleep(0.5 + attempt)

        raise PortalError("%s %s failed after %d attempts: %s"
                          % (method, path, attempts, last))

    def get(self, path, attempts=4):
        return self.request("GET", path, attempts=attempts)

    def post(self, path, fields, attempts=4):
        body = urllib.parse.urlencode(fields)
        return self.request("POST", path, body=body,
                            content_type="application/x-www-form-urlencoded",
                            attempts=attempts)

    def post_multipart(self, path, fields, files, attempts=4):
        """
        A multipart body built by hand so a file's bytes go out untouched.

        files is (field, filename, bytes); the bytes are written straight into
        the body, which is what a test for byte exact upload needs.
        """
        boundary = "----pdiTestBoundary7d91c4"
        parts = []

        for name, value in fields.items():
            parts.append(("--%s\r\nContent-Disposition: form-data; name=\"%s\"\r\n\r\n"
                          % (boundary, name)).encode())
            parts.append(str(value).encode())
            parts.append(b"\r\n")

        for name, filename, blob in files:
            parts.append(("--%s\r\nContent-Disposition: form-data; name=\"%s\"; filename=\"%s\"\r\n"
                          "Content-Type: application/octet-stream\r\n\r\n"
                          % (boundary, name, filename)).encode())
            parts.append(blob)
            parts.append(b"\r\n")

        parts.append(("--%s--\r\n" % boundary).encode())
        body = b"".join(parts)

        return self.request("POST", path, body=body,
                            content_type="multipart/form-data; boundary=%s" % boundary,
                            attempts=attempts)

    def login(self, username, password):
        return self.post("/login", {"username": username, "password": password})

    def logout(self):
        return self.get("/logout")

    def session_cookie(self):
        """The token the portal issued, or None while logged out."""
        for name, value in self.jar.items():
            if "session" in name and value:
                return value
        return None

    def csrf(self, path="/login-config"):
        """
        The csrf token of this session, read off a page that carries a form.

        Read afresh each time rather than cached: a token belongs to a session,
        and a test that logs out and back in gets a new one.
        """
        page = self.get(path)
        found = CSRF_INPUT.search(page.body)
        if not found:
            raise PortalError("no csrf token on %s (status %d)" % (path, page.status))

        return found.group(1)

    def forget(self):
        self.jar = {}
