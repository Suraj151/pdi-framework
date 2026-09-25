#!/usr/bin/env python3

"""
The download command, against an origin stood up for the run.

Two things make this suite different from the others. The first is that the
board has to reach back to this machine, so a failure to connect is reported as
a skip with the reason rather than as a wall of failures — a firewall or an
access point with client isolation stops it and neither is a defect in the
command.

Everything here is fetched over plain http and nothing here uses https, which
is deliberate rather than incidental. A tls handshake needs ram the smallest
supported board does not have to spare, so putting one in this suite would make
a download test fail for a reason that has nothing to do with downloading. The
tls stack has its own suite, gated on its own flag. Anything added below is
http, and the origin serves nothing else.

The second thing is the progress display, which is the reason several of these
tests exist. The bar rewrites one line with carriage returns, so a carriage
return that survives in the output marks a redraw: the shell driver has already
taken the line-ending CR off the end of every line by the time a test sees it.
The helpers below fold CRLF anyway, so they read the same against a raw capture
as against a driver that has normalised it.

The last test in the file is the one that matters most: a command run after a
download must answer for itself. A display that redraws mid-command is exactly
the shape that has desynchronised this harness before, and a download that left
the reader a command behind would show up there and nowhere else.
"""

import re

from .registry import test, expect_in, expect_not_in, Skip
from ..driver.mdns import local_address_towards
from ..driver.http_origin import Origin

W = "wt_"

SIZED = 4000
NOLENGTH = 3000

PERCENT = re.compile(r"\]\s*(\d+)%")


class Fixture(object):
    """One origin, shared by the whole suite."""

    def __init__(self):
        self.origin = None
        self.address = ""
        self.error = None

    def stop(self):
        if self.origin is not None:
            self.origin.stop()
            self.origin = None


_fixture = None


def fixture(t):
    global _fixture

    if _fixture is None:
        state = Fixture()
        try:
            state.address = local_address_towards(t.address())
            state.origin = Origin("0.0.0.0", 0).start()
            t.at_exit(state.stop)
        except Exception as err:
            state.error = "could not stand up an origin for the board: %s" % err
        _fixture = state

    if _fixture.error:
        raise Skip(_fixture.error)

    return _fixture


def url_for(t, path):
    state = fixture(t)
    return "http://%s:%d%s" % (state.address, state.origin.port, path)


def fetch(t, name, path, timeout=90.0):
    """Download one path to a file of ours and hand back what the shell said."""
    dest = "/" + W + name
    t.run("rm %s" % dest)
    out = t.run("wget %s %s" % (dest, url_for(t, path)), timeout=timeout)
    return dest, out


def progress_frames(out):
    """
    The bar's redraws, in order.

    CRLF is folded to LF first so a line ending can never be counted as a
    redraw, whether or not the transport already stripped it.
    """
    frames = []
    for line in out.replace("\r\n", "\n").split("\n"):
        if "\r" in line:
            frames.extend(part for part in line.split("\r") if part.strip())
    return frames


def percentages(out):
    return [int(found) for frame in progress_frames(out)
            for found in PERCENT.findall(frame)]


def outcome(out):
    """The command's last word, with the progress line taken out of the way."""
    text = out.replace("\r\n", "\n")
    kept = []
    for line in text.split("\n"):
        kept.append(line.split("\r")[-1] if "\r" in line else line)
    return "\n".join(kept)


def cleanup(t, *names):
    for name in names:
        t.run("rm /%s%s" % (W, name))


@test("wget saves what the url names and reports the size", needs=("wget",))
def saves_the_body(t):
    dest, out = fetch(t, "a.bin", "/sized.bin")
    try:
        expect_in("saved", outcome(out), "the command reported no saved file")
        expect_in(str(SIZED), outcome(out), "the size reported is not the body's")
        expect_in(dest, outcome(out), "the path reported is not the one asked for")

        listed = t.run("ls /")
        expect_in(W + "a.bin", listed, "the file is not on the filesystem")
    finally:
        cleanup(t, "a.bin")


@test("the progress bar redraws one line instead of scrolling", needs=("wget",))
def redraws_in_place(t):
    dest, out = fetch(t, "b.bin", "/drip.bin")
    try:
        frames = progress_frames(out)
        if len(frames) < 2:
            raise AssertionError("expected the bar to redraw, saw %d frame(s) in:\n%r"
                                 % (len(frames), out))

        # every redraw has to land on the same line: if the bar emitted
        # newlines it would scroll the terminal for the whole transfer
        folded = out.replace("\r\n", "\n")
        carrying = [line for line in folded.split("\n") if "\r" in line]
        if len(carrying) != 1:
            raise AssertionError("the bar spread over %d lines, expected 1:\n%r"
                                 % (len(carrying), out))
    finally:
        cleanup(t, "b.bin")


@test("the progress bar finishes at one hundred percent", needs=("wget",))
def finishes_at_full(t):
    dest, out = fetch(t, "c.bin", "/drip.bin")
    try:
        seen = percentages(out)
        if not seen:
            raise AssertionError("the bar reported no percentage at all in:\n%r" % out)
        if seen[-1] != 100:
            raise AssertionError("the bar stopped at %d%%, expected 100%%; frames:\n%s"
                                 % (seen[-1], "\n".join(progress_frames(out))))

        expect_in("%d/%d" % (SIZED, SIZED), progress_frames(out)[-1],
                  "the last frame does not show the whole body as transferred")
    finally:
        cleanup(t, "c.bin")


@test("the reported progress never goes backwards", needs=("wget",))
def progress_is_monotonic(t):
    dest, out = fetch(t, "d.bin", "/drip.bin")
    try:
        seen = percentages(out)
        if len(seen) < 2:
            raise AssertionError("too few frames to judge progress: %r" % seen)

        for before, after in zip(seen, seen[1:]):
            if after < before:
                raise AssertionError("progress went %d%% then %d%%: %r" % (before, after, seen))

        if seen[0] > 50:
            raise AssertionError("the bar opened at %d%%, so it is not counting from the "
                                 "start of the body: %r" % (seen[0], seen))
    finally:
        cleanup(t, "d.bin")


@test("a body with no declared length reports bytes rather than a percentage",
      needs=("wget",))
def unknown_length_counts_bytes(t):
    dest, out = fetch(t, "e.bin", "/nolength.bin")
    try:
        text = outcome(out)
        if "saved" not in text:
            raise Skip("this target did not complete a body with no declared length")

        expect_in(str(NOLENGTH), text, "the size reported is not the body's")
        expect_not_in("%", "".join(progress_frames(out)),
                      "a percentage was shown for a body whose length is unknown")
    finally:
        cleanup(t, "e.bin")


@test("a second download replaces the file rather than appending to it", needs=("wget",))
def replaces_rather_than_appends(t):
    dest, out = fetch(t, "f.bin", "/sized.bin")
    try:
        expect_in("saved", outcome(out), "the first download did not land")

        again = t.run("wget %s %s" % (dest, url_for(t, "/sized.bin")), timeout=90.0)
        text = outcome(again)
        expect_in(str(SIZED), text, "the second download did not report the body's size")
        expect_not_in(str(SIZED * 2), text, "the second download appended to the first")
    finally:
        cleanup(t, "f.bin")


@test("a name the filesystem cannot hold is refused before anything is fetched",
      needs=("wget",))
def refuses_a_long_name_up_front(t):
    state = fixture(t)
    before = state.origin.request_count()

    long_name = "/" + W + "b" * 40 + ".bin"
    out = t.run("wget %s %s" % (long_name, url_for(t, "/sized.bin")))
    try:
        expect_in("longer than", outcome(out), "an oversized name was not refused")

        if progress_frames(out):
            raise AssertionError("a refused name still drew a progress bar:\n%r" % out)

        if state.origin.request_count() != before:
            raise AssertionError("a name that cannot be written still fetched the body")
    finally:
        t.run("rm %s" % long_name)


@test("a body larger than the free space is refused without writing it", needs=("wget",))
def refuses_when_it_cannot_fit(t):
    dest, out = fetch(t, "g.bin", "/huge.bin", timeout=120.0)
    try:
        text = outcome(out)
        expect_in("not enough space", text, "an oversized body was not refused")
        expect_not_in("saved", text, "an oversized body was reported as saved")

        expect_not_in(W + "g.bin", t.run("ls /"),
                      "a refused download left a file behind")
    finally:
        cleanup(t, "g.bin")


@test("a url that is not found fails and leaves no file", needs=("wget",))
def missing_url_leaves_nothing(t):
    dest, out = fetch(t, "h.bin", "/nothing-here.bin")
    try:
        expect_not_in("saved", outcome(out), "a 404 was reported as a saved file")
        expect_not_in(W + "h.bin", t.run("ls /"),
                      "a failed download left a partial file behind")
    finally:
        cleanup(t, "h.bin")


@test("a header that looks like a status line does not refuse the download",
      needs=("wget",))
def a_spoofed_status_header_is_ignored(t):
    """
    CV on hardware.

    Every reply this origin sends carries a Server value containing "HTTP/",
    so the whole suite covers the defect; this one adds the deliberate case,
    a header whose value is a complete status line naming a different code.
    A reader that scans for the status anywhere reports that code instead and
    refuses a healthy transfer.
    """
    dest, out = fetch(t, "j.bin", "/spoofed.bin")
    try:
        text = outcome(out)
        expect_in("saved", text, "a reply with a status-shaped header was refused")
        expect_in("512", text, "the size reported is not the body's")
        expect_not_in("404", text, "the status came from a header, not the status line")
    finally:
        cleanup(t, "j.bin")


@test("a reply whose lines end in a bare feed is read", needs=("wget",))
def a_bare_line_feed_reply_is_read(t):
    """
    CX on hardware, and the board is the only place the real cost shows.

    A blank line with no carriage return trims away to nothing, and the reader
    passed that to strlen. On the host a sanitizer names it; on a device it is
    a read of address zero, so the failure to watch for is the board dropping
    the transport rather than a wrong answer.
    """
    dest, out = fetch(t, "k.bin", "/barefeed.bin")
    try:
        expect_in("saved", outcome(out), "a reply with bare line feeds was not read")
        expect_in("512", outcome(out), "the size reported is not the body's")

        alive = t.run("echo wtalive")
        expect_in("wtalive", alive, "the shell stopped answering after a bare-feed reply")
    finally:
        cleanup(t, "k.bin")


@test("a url that carries no host is refused without crashing", needs=("wget",))
def a_url_without_a_host_is_refused(t):
    """
    CY on hardware. A url ending in a colon sent the parser past the end of
    its buffer, so what matters is that the shell is still there afterwards.
    """
    out = t.run("wget /%sl.bin http:" % W)
    expect_not_in("saved", outcome(out), "a url with no host reported a saved file")

    alive = t.run("echo wtparse")
    expect_in("wtparse", alive, "the shell stopped answering after a malformed url")


@test("a url padded with spaces is still fetched", needs=("wget",))
def a_padded_url_is_fetched(t):
    """CY's other half: the fix must not refuse a url with leading blanks."""
    state = fixture(t)
    dest = "/" + W + "m.bin"
    t.run("rm %s" % dest)
    url = url_for(t, "/small.txt")
    try:
        out = t.run('wget %s "   %s"' % (dest, url), timeout=90.0)
        expect_in("saved", outcome(out), "a url with leading spaces was refused")
    finally:
        cleanup(t, "m.bin")


@test("a plain http url is still fetched on a build that carries tls",
      needs=("wget", "tls"))
def plain_http_on_a_tls_build(t):
    """
    The gate is tls rather than https, and the fetch below is still plain http.

    A build with tls in it has a secure client sitting alongside the plain one,
    and the failure this guards against is every outbound request taking the
    secure one — which turns an http url into a handshake against a server that
    is not speaking tls, and reports as a refused download rather than as a
    configuration mistake. The client is chosen from the url's scheme, so a
    plain fetch has to arrive here exactly as it does on a build without tls.
    """
    dest, out = fetch(t, "n.bin", "/sized.bin")
    try:
        expect_in("saved", outcome(out), "the download did not report a saved file")
        expect_in(str(SIZED), outcome(out), "the size reported is not the body's")

        listed = t.run("ls /")
        expect_in(W + "n.bin", listed, "the file is not on the filesystem")
    finally:
        cleanup(t, "n.bin")


@test("the shell still answers for itself after a download", needs=("wget",))
def shell_is_not_left_behind(t):
    """
    The one that guards the display rather than the download.

    A command that redraws the terminal has desynchronised this harness before:
    the reader returns at the wrong moment and from then on every command reads
    the previous command's output. That failure is invisible in the download's
    own result and shows up only in whatever runs next, so this asks two
    questions whose answers could not come from any earlier command.
    """
    dest, out = fetch(t, "i.bin", "/drip.bin")
    try:
        expect_in("saved", outcome(out), "the download did not land")

        marker = t.run("echo wtsync1")
        expect_in("wtsync1", marker, "the shell did not answer the command after a download")
        expect_not_in("saved", marker,
                      "the command after a download read the download's output")

        again = t.run("echo wtsync2")
        expect_in("wtsync2", again, "the shell did not answer the second command")
        expect_not_in("wtsync1", again,
                      "the shell is a command behind after a download")
    finally:
        cleanup(t, "i.bin")
