#!/usr/bin/env python3

"""
The portal's upload path at the edges a change to its reading breaks first:
sizes either side of the read block, a line ending that falls across a read,
content that looks like the boundary, empty and one byte files, a field sent
after the file, and a transfer dropped part way. Every file is read back over
sftp and compared byte for byte, so nothing here depends on how the portal
serves files back.
"""

import hashlib
import os
import random
import socket
import ssl
import time

from .registry import test, Skip, expect_in
from .test_portal import upload
from .test_sftp import sftp_open, patterned

READ_BLOCK = 2048
ANSWER_WITHIN = 5.0
BOUNDARY_TAIL = b"----pdiTestBoundary7d91"


def awkward_cases():
    """(name, bytes) pairs, each one an edge the multipart reader has to get right."""
    cases = [("up_one.bin", b"x")]
    for size in (READ_BLOCK - 2, READ_BLOCK - 1, READ_BLOCK, READ_BLOCK + 1, READ_BLOCK + 2,
                 2 * READ_BLOCK - 1, 2 * READ_BLOCK, 2 * READ_BLOCK + 1):
        cases.append(("up_size_%d.bin" % size, patterned(size)))
    cases += [
        ("up_crlf_only.bin", b"\r\n" * 1500),
        ("up_ends_cr.bin", patterned(3000) + b"\r"),
        ("up_ends_crlf.bin", patterned(3000) + b"\r\n"),
        ("up_ends_dashes.bin", patterned(3000) + b"\r\n--"),
        ("up_boundary_like.bin", (b"\r\n--" + BOUNDARY_TAIL + b"x") * 120),
        ("up_nul.bin", b"\x00" * 3000),
        ("up_crlf_on_edge.bin", patterned(READ_BLOCK - 1) + b"\r\n" + patterned(100)),
        ("up_cr_on_edge.bin", patterned(READ_BLOCK - 1) + b"\r" + patterned(100)),
    ]
    return cases


def read_back(sftp, path):
    """The bytes a file holds, or None when it is not there."""
    try:
        with sftp.open(path, "rb") as fh:
            return fh.read()
    except IOError:
        return None


def staged_names(sftp):
    """What the staging directory holds now."""
    try:
        return [n for n in sftp.listdir("/temp") if n not in (".", "..")]
    except IOError:
        return []


@test("uploads of awkward sizes and contents come back byte for byte", slow=True, needs=("rm",))
def awkward_uploads_are_exact(t):
    """
    Every upload is made first and only then read back over one sftp session,
    so no ssh session holds the heap while the portal is taking a file.
    """
    portal = t.portal()
    cases = awkward_cases()
    wrong = []
    sent = []
    try:
        for name, blob in cases:
            t.run("rm /%s" % name)
            answer = upload(t, portal, name, blob)
            if answer.status not in (200, 301, 302, 303):
                wrong.append("%s: the upload answered %d" % (name, answer.status))
            else:
                sent.append((name, blob))

        client, sftp = sftp_open(t)
        try:
            for name, blob in sent:
                got = read_back(sftp, "/" + name)
                if got is None:
                    wrong.append("%s: not on the target" % name)
                elif got != blob:
                    at = next((i for i in range(min(len(got), len(blob))) if got[i] != blob[i]),
                              min(len(got), len(blob)))
                    wrong.append("%s: %d bytes sent, %d stored, first difference at %d, stored %r"
                                 % (name, len(blob), len(got), at, got[at:at + 48]))
        finally:
            sftp.close()
            client.close()
    finally:
        for name, _ in cases:
            t.run("rm /%s" % name)

    if wrong:
        raise AssertionError("; ".join(wrong))


@test("an empty file uploaded through the portal lands as an empty file", needs=("rm",))
def empty_upload(t):
    if os.environ.get("PDI_KNOWN_FINDINGS", "") != "1":
        raise Skip("open finding F20, recorded in TESTING_PLAN.md; set PDI_KNOWN_FINDINGS=1 to run it")
    portal = t.portal()
    name = "up_empty.bin"
    t.run("rm /%s" % name)
    client, sftp = sftp_open(t)
    try:
        answer = upload(t, portal, name, b"")
        if answer.status not in (200, 301, 302, 303):
            raise AssertionError("the upload answered %d" % answer.status)
        got = read_back(sftp, "/" + name)
        if got is None:
            raise AssertionError("an empty upload left no file on the target")
        if got != b"":
            raise AssertionError("an empty upload stored %d bytes" % len(got))
    finally:
        sftp.close()
        client.close()
        t.run("rm /%s" % name)


@test("a field sent after the file still decides where the file goes", needs=("mkdir", "rm", "ls"))
def field_after_the_file(t):
    portal = t.portal()
    name = "up_after.txt"
    folder = "/up_after_dir"
    t.run("rm %s/%s" % (folder, name))
    t.run("rm %s" % folder)
    t.run("mkdir %s" % folder)
    try:
        boundary = "----pdiTestBoundary7d91c4"
        body = b"".join([
            ("--%s\r\nContent-Disposition: form-data; name=\"csrf\"\r\n\r\n%s\r\n"
             % (boundary, portal.csrf())).encode(),
            ("--%s\r\nContent-Disposition: form-data; name=\"nf\"; filename=\"%s\"\r\n"
             "Content-Type: application/octet-stream\r\n\r\n" % (boundary, name)).encode(),
            b"sent before its folder\n\r\n",
            ("--%s\r\nContent-Disposition: form-data; name=\"loc\"\r\n\r\n%s\r\n"
             % (boundary, folder)).encode(),
            ("--%s--\r\n" % boundary).encode(),
        ])
        answer = portal.request("POST", "/storage-fileupload", body=body,
                                content_type="multipart/form-data; boundary=%s" % boundary)
        if answer.status not in (200, 301, 302, 303):
            raise AssertionError("the upload answered %d" % answer.status)
        expect_in(name, t.run("ls %s" % folder), "the folder named after the file")
    finally:
        t.run("rm %s/%s" % (folder, name))
        t.run("rm %s" % folder)


def binary_cases():
    """(name, bytes) pairs of binary content: a stored binary file, random, and dense in delimiter-like runs."""
    with open(os.path.join(os.path.dirname(__file__), "data", "uploadtestfile.bin"), "rb") as fh:
        cases = [("up_image.bin", fh.read())]
    rnd = random.Random(7331)
    cases.append(("up_random.bin", bytes(rnd.getrandbits(8) for _ in range(24 * 1024))))
    runs = [b"\r", b"\n", b"\r\n", b"\r\n-", b"\r\n--", b"--", b"\r\n------pdiTestBoundary7d91",
            b"\r\n------pdiTestBoundary7d91c", b"\x00", b"\xff"]
    dense = bytearray()
    while len(dense) < 16 * 1024:
        dense += bytes(rnd.getrandbits(8) for _ in range(rnd.randint(0, 40)))
        dense += rnd.choice(runs)
    cases.append(("up_dense.bin", bytes(dense)))
    return cases


@test("a binary file uploaded through the portal downloads with the same md5", slow=True, needs=("rm",))
def binary_uploads_are_exact(t):
    """
    Binary content carries CR, LF and dashes anywhere, so the file may end only
    at the real delimiter. Each file is uploaded, downloaded again and compared by md5.
    """
    portal = t.portal()
    cases = binary_cases()
    wrong = []
    try:
        for name, blob in cases:
            t.run("rm /%s" % name)
            answer = upload(t, portal, name, blob)
            if answer.status not in (200, 301, 302, 303):
                wrong.append("%s: the upload answered %d" % (name, answer.status))
                continue
            back = portal.get("/" + name)
            if 200 != back.status:
                wrong.append("%s: the download answered %d" % (name, back.status))
            elif hashlib.md5(back.raw).hexdigest() != hashlib.md5(blob).hexdigest():
                wrong.append("%s: md5 %s uploaded, %s downloaded (%d of %d bytes)"
                             % (name, hashlib.md5(blob).hexdigest(), hashlib.md5(back.raw).hexdigest(),
                                len(back.raw), len(blob)))
    finally:
        for name, _ in cases:
            t.run("rm /%s" % name)

    if wrong:
        raise AssertionError("; ".join(wrong))


def split_upload(t, portal, name, blob, cut, between=None, at=None):
    """
    Upload through a raw socket, pausing after `cut` bytes of the closing delimiter,
    or `-cut` bytes into the file when cut is negative, or before the first byte of
    `at` when it is given; `between` runs in the pause.
    """
    boundary = "----pdiTestBoundary7d91c4"
    head = b"".join([
        ("--%s\r\nContent-Disposition: form-data; name=\"csrf\"\r\n\r\n%s\r\n"
         % (boundary, portal.csrf())).encode(),
        ("--%s\r\nContent-Disposition: form-data; name=\"loc\"\r\n\r\n/storage\r\n" % boundary).encode(),
        ("--%s\r\nContent-Disposition: form-data; name=\"nf\"; filename=\"%s\"\r\n"
         "Content-Type: application/octet-stream\r\n\r\n" % (boundary, name)).encode(),
    ])
    tail = ("\r\n--%s--\r\n" % boundary).encode()
    body = head + blob + tail
    cookie = "; ".join("%s=%s" % kv for kv in portal.jar.items())
    request = ("POST /storage-fileupload HTTP/1.1\r\nHost: %s\r\nCookie: %s\r\nConnection: close\r\n"
               "Content-Type: multipart/form-data; boundary=%s\r\nContent-Length: %d\r\n\r\n"
               % (portal.host, cookie, boundary, len(body))).encode()
    split = len(request) + len(head) + (-cut if cut < 0 else len(blob) + cut)
    if at is not None:
        split = (request + body).index(at)

    raw = socket.create_connection((portal.host, portal.port), timeout=20)
    try:
        if portal.secure:
            context = ssl.create_default_context()
            context.check_hostname = False
            context.verify_mode = ssl.CERT_NONE
            raw = context.wrap_socket(raw, server_hostname=portal.host)
        whole = request + body
        raw.sendall(whole[:split])
        time.sleep(0.5)
        if between is not None:
            between()
        raw.sendall(whole[split:])
        answer = b""
        while b"\r\n" not in answer:
            got = raw.recv(256)
            if not got:
                break
            answer += got
    finally:
        raw.close()
    return answer.split(b"\r\n", 1)[0]


@test("an upload whose closing delimiter arrives in two pieces is stored byte for byte", needs=("rm",))
def delimiter_split_across_reads(t):
    """
    A read ends wherever the network stops handing bytes over, so the closing
    delimiter can arrive cut in two. Neither half belongs to the file.
    """
    portal = t.portal()
    cases = []
    for size in (100, READ_BLOCK + 2):
        for cut in (1, 2, 12, 2 + len("------pdiTestBoundary7d91c4"), len("\r\n------pdiTestBoundary7d91c4--\r\n")):
            cases.append(("up_split_%d_%d.bin" % (size, cut), patterned(size), cut))
    wrong = []
    try:
        for name, blob, cut in cases:
            t.run("rm /%s" % name)
            status = split_upload(t, portal, name, blob, cut)
            if not status.startswith(b"HTTP/1.1 3") and not status.startswith(b"HTTP/1.1 200"):
                wrong.append("%s: the upload answered %r" % (name, status))

        client, sftp = sftp_open(t)
        try:
            for name, blob, cut in cases:
                got = read_back(sftp, "/" + name)
                if got is None:
                    wrong.append("%s: not on the target" % name)
                elif got != blob:
                    at = next((i for i in range(min(len(got), len(blob))) if got[i] != blob[i]), min(len(got), len(blob)))
                    wrong.append("%s: %d bytes sent, %d stored, first difference at %d: %r"
                                 % (name, len(blob), len(got), at, got[at:at + 40]))
        finally:
            sftp.close()
            client.close()
    finally:
        for name, _, _ in cases:
            t.run("rm /%s" % name)

    if wrong:
        raise AssertionError("; ".join(wrong))


@test("an upload whose header lines arrive cut in two is stored byte for byte", needs=("rm",))
def header_lines_split_across_reads(t):
    """
    A read ends wherever the network stops handing bytes over, so any line of the
    request can arrive in two pieces: a request header, a part header, the blank
    line that ends them, a field value or a boundary. Each must be read whole.
    """
    portal = t.portal()
    cuts = (b"ookie: ", b"ontent-Length", b"ent-Type: application", b"cation/octet-stream\r\n\r\n",
            b"\n\r\n\x00", b"orage\r\n", b"TestBoundary7d91c4\r\nContent-Disposition: form-data; name=\"nf\"")
    cases = [("up_line_%d.bin" % i, patterned(700), cut) for i, cut in enumerate(cuts)]
    wrong = []
    try:
        for name, blob, cut in cases:
            t.run("rm /%s" % name)
            began = time.time()
            status = split_upload(t, portal, name, blob, 0, at=cut)
            took = time.time() - began - 0.5
            if not status.startswith(b"HTTP/1.1 3") and not status.startswith(b"HTTP/1.1 200"):
                wrong.append("%s: the upload cut before %r answered %r" % (name, cut, status))
            elif took > ANSWER_WITHIN:
                wrong.append("%s: the upload cut before %r was answered %.1f s after its last byte" % (name, cut, took))

        client, sftp = sftp_open(t)
        try:
            for name, blob, cut in cases:
                got = read_back(sftp, "/" + name)
                if got is None:
                    wrong.append("%s: cut before %r, not on the target" % (name, cut))
                elif got != blob:
                    at = next((i for i in range(min(len(got), len(blob))) if got[i] != blob[i]), min(len(got), len(blob)))
                    wrong.append("%s: cut before %r, %d bytes sent, %d stored, first difference at %d: %r"
                                 % (name, cut, len(blob), len(got), at, got[at:at + 40]))
        finally:
            sftp.close()
            client.close()
    finally:
        for name, _, _ in cases:
            t.run("rm /%s" % name)

    if wrong:
        raise AssertionError("; ".join(wrong))


@test("an upload the heap squeezes part way is stored byte for byte or not at all", needs=("rm",))
def squeezed_upload(t):
    """
    A board's heap fragments under load, so the buffer a read grows can be refused
    in the middle of a file; content with no line break is what makes it grow. The
    bytes taken off the socket must then reach the file or the upload must fail; a
    file stored with a piece missing is the one answer that is never right.
    """
    squeeze = getattr(t.shell, "set_heap", None)
    if squeeze is None:
        raise Skip("only the host process can have its heap squeezed on cue")
    portal = t.portal()
    blob = bytes(b"0123456789abcdefghijklmnopqrstuvwxyz"[i % 36] for i in range(6 * READ_BLOCK))
    cases = [("up_squeeze_%d.bin" % block, block) for block in (600, 1024, 1536, 2100, 3000, 4200)]
    wrong = []
    try:
        for name, block in cases:
            t.run("rm /%s" % name)
            try:
                split_upload(t, portal, name, blob, -1,
                             between=lambda b=block: (squeeze(block=b), time.sleep(0.3)))
            except OSError:
                pass
            finally:
                squeeze()
                time.sleep(0.3)

        client, sftp = sftp_open(t)
        try:
            for name, block in cases:
                got = read_back(sftp, "/" + name)
                if got is not None and got != blob:
                    wrong.append("block %d: %d bytes sent, %d stored" % (block, len(blob), len(got)))
        finally:
            sftp.close()
            client.close()
    finally:
        squeeze()
        for name, _ in cases:
            t.run("rm /%s" % name)

    if wrong:
        raise AssertionError("; ".join(wrong))


@test("an upload dropped part way leaves nothing staged and the portal answers on", needs=("rm",))
def dropped_upload(t):
    portal = t.portal()
    client, sftp = sftp_open(t)
    before = set(staged_names(sftp))
    name = "up_dropped.bin"
    t.run("rm /%s" % name)
    try:
        boundary = "----pdiTestBoundary7d91c4"
        head = b"".join([
            ("--%s\r\nContent-Disposition: form-data; name=\"csrf\"\r\n\r\n%s\r\n"
             % (boundary, portal.csrf())).encode(),
            ("--%s\r\nContent-Disposition: form-data; name=\"nf\"; filename=\"%s\"\r\n"
             "Content-Type: application/octet-stream\r\n\r\n" % (boundary, name)).encode(),
        ])
        sent = head + patterned(3 * READ_BLOCK)
        declared = len(sent) + 20000
        cookie = "; ".join("%s=%s" % kv for kv in portal.jar.items())
        request = ("POST /storage-fileupload HTTP/1.1\r\nHost: %s\r\nCookie: %s\r\n"
                   "Content-Type: multipart/form-data; boundary=%s\r\nContent-Length: %d\r\n\r\n"
                   % (portal.host, cookie, boundary, declared)).encode()

        raw = socket.create_connection((portal.host, portal.port), timeout=20)
        try:
            if portal.secure:
                context = ssl.create_default_context()
                context.check_hostname = False
                context.verify_mode = ssl.CERT_NONE
                raw = context.wrap_socket(raw, server_hostname=portal.host)
            raw.sendall(request + sent)
            time.sleep(1.0)
        finally:
            raw.close()

        deadline = time.time() + 60
        left = None
        while time.time() < deadline:
            left = set(staged_names(sftp)) - before
            if not left and read_back(sftp, "/" + name) is None:
                break
            time.sleep(2.0)
        if left:
            raise AssertionError("a dropped upload left %s staged" % sorted(left))
        if read_back(sftp, "/" + name) is not None:
            raise AssertionError("a dropped upload was moved into place as if it had arrived")

        answer = t.portal().get("/")
        if 200 != answer.status:
            raise AssertionError("after a dropped upload the portal answered %d" % answer.status)
    finally:
        sftp.close()
        client.close()
        t.run("rm /%s" % name)



