#!/usr/bin/env python3

"""
A throwaway SMTP server for one board to send mail to.

Deliberately not a real mail server: one that relays would deliver test mail
somewhere, and one that does not cannot be told to refuse a login or hang
half way through a message on cue. This binds a port of its own, answers each
command with whatever the test has set for it, keeps every byte the board sent,
and keeps each message as it would be delivered — transparency dots removed.

Every reply can be replaced: a test sets `replies["RCPT"] = "550 no such user"`
to see a refusal handled, or names a step in `hang` to have the server read
that command and never answer it.
"""

import base64
import socket
import threading
import time


class Mail(object):

    def __init__(self):
        self.sender = None
        self.recipients = []
        self.lines = []
        self.username = None
        self.password = None
        self.helo = None

    @property
    def text(self):
        return "\n".join(self.lines)

    def header(self, name):
        for line in self.lines:
            if line.lower().startswith(name.lower() + ":"):
                return line.split(":", 1)[1].strip()
            if "" == line:
                break
        return None

    @property
    def body(self):
        if "" in self.lines:
            return "\n".join(self.lines[self.lines.index("") + 1:])
        return ""


class SmtpServer(object):

    DEFAULTS = {
        "greeting": "220 pditest ESMTP ready",
        "EHLO": "250-pditest greets you\r\n250-SIZE 1048576\r\n250 AUTH LOGIN PLAIN",
        "HELO": "250 pditest",
        "AUTH": "334 VXNlcm5hbWU6",
        "USER": "334 UGFzc3dvcmQ6",
        "PASS": "235 2.7.0 Accepted",
        "MAIL": "250 2.1.0 OK",
        "RCPT": "250 2.1.5 OK",
        "DATA": "354 Go ahead",
        "END": "250 2.0.0 queued",
        "QUIT": "221 2.0.0 closing",
        "RSET": "250 OK",
        "NOOP": "250 OK",
    }

    def __init__(self, host="0.0.0.0", port=0):
        self._host = host
        self._port = port
        self._sock = None
        self._thread = None
        self._stop = threading.Event()
        self._lock = threading.Lock()
        self.replies = dict(self.DEFAULTS)
        self.hang = set()
        self.mails = []
        self.sessions = []
        self.raw = b""

    @property
    def port(self):
        return self._sock.getsockname()[1] if self._sock else self._port

    def start(self):
        self._sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self._sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        self._sock.bind((self._host, self._port))
        self._sock.listen(4)
        self._sock.settimeout(0.5)
        self._thread = threading.Thread(target=self._serve, daemon=True)
        self._thread.start()
        return self

    def stop(self):
        self._stop.set()
        if self._thread is not None:
            self._thread.join(timeout=5.0)
        if self._sock is not None:
            self._sock.close()
            self._sock = None

    def reset(self):
        with self._lock:
            self.replies = dict(self.DEFAULTS)
            self.hang = set()
            self.mails = []
            self.sessions = []
            self.raw = b""

    def wait_for_mail(self, timeout, after=0):
        deadline = time.time() + timeout
        while time.time() < deadline:
            with self._lock:
                if len(self.mails) > after:
                    return self.mails[after]
            time.sleep(0.1)
        return None

    def wait_for_session(self, timeout, after=0):
        """The commands of a conversation past the given count, once it has ended."""
        deadline = time.time() + timeout
        while time.time() < deadline:
            with self._lock:
                done = [s for s in self.sessions if s["ended"]]
                if len(done) > after:
                    return done[after]
            time.sleep(0.1)
        return None

    def _serve(self):
        while not self._stop.is_set():
            try:
                conn, _ = self._sock.accept()
            except socket.timeout:
                continue
            except OSError:
                return
            threading.Thread(target=self._converse, args=(conn,), daemon=True).start()

    def _reply(self, conn, key):
        text = self.replies.get(key, "")
        if text:
            conn.sendall(text.encode() + b"\r\n")

    def _converse(self, conn):
        session = {"commands": [], "ended": False}
        with self._lock:
            self.sessions.append(session)
        conn.settimeout(30.0)
        mail = Mail()
        state = "command"
        buffer = b""
        try:
            if "greeting" in self.hang:
                self._hold()
                return
            self._reply(conn, "greeting")
            while not self._stop.is_set():
                try:
                    chunk = conn.recv(4096)
                except socket.timeout:
                    return
                if not chunk:
                    return
                with self._lock:
                    self.raw += chunk
                buffer += chunk
                while b"\r\n" in buffer:
                    line, _, buffer = buffer.partition(b"\r\n")
                    text = line.decode(errors="replace")
                    state = self._handle(conn, text, state, mail, session)
                    if "quit" == state:
                        return
        except OSError:
            pass
        finally:
            session["ended"] = True
            conn.close()

    def _hold(self):
        while not self._stop.is_set():
            time.sleep(0.2)

    def _answer(self, conn, key):
        if key in self.hang:
            self._hold()
            raise OSError("held")
        self._reply(conn, key)

    def _handle(self, conn, text, state, mail, session):
        if "data" == state:
            if "." == text:
                with self._lock:
                    self.mails.append(mail)
                self._answer(conn, "END")
                return "command"
            mail.lines.append(text[1:] if text.startswith(".") else text)
            return "data"

        if "user" == state:
            mail.username = base64.b64decode(text).decode(errors="replace")
            self._answer(conn, "USER")
            return "pass"
        if "pass" == state:
            mail.password = base64.b64decode(text).decode(errors="replace")
            self._answer(conn, "PASS")
            return "command"

        session["commands"].append(text)
        verb = text.split(" ", 1)[0].split(":", 1)[0].upper()

        if verb in ("EHLO", "HELO"):
            mail.helo = text.split(" ", 1)[1] if " " in text else ""
            self._answer(conn, verb)
            return "command"
        if "AUTH" == verb:
            self._answer(conn, "AUTH")
            return "user" if self.replies.get("AUTH", "").startswith("334") else "command"
        if "MAIL" == verb:
            mail.sender = text.split(":", 1)[1].strip() if ":" in text else None
            self._answer(conn, "MAIL")
            return "command"
        if "RCPT" == verb:
            mail.recipients.append(text.split(":", 1)[1].strip() if ":" in text else None)
            self._answer(conn, "RCPT")
            return "command"
        if "DATA" == verb:
            self._answer(conn, "DATA")
            return "data" if self.replies.get("DATA", "").startswith("354") else "command"
        if "QUIT" == verb:
            self._answer(conn, "QUIT")
            return "quit"
        if verb in self.replies:
            self._answer(conn, verb)
        else:
            conn.sendall(b"502 5.5.2 command not recognised\r\n")
        return "command"
