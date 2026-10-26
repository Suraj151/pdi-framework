#!/usr/bin/env python3

"""
Write the seed corpus for the fuzz harnesses.

A mutator starting from an empty corpus spends its first minutes rediscovering
the shape of a packet. These seeds are one well formed input per branch the
harness selects, so coverage starts inside the parser rather than at its length
check. The seeds are generated rather than checked in because most of them are
binary, and a length written out by hand goes stale the moment a header moves.
"""

import os
import struct
import sys

FUZZ_DIR = os.path.dirname(os.path.abspath(__file__))
CORPUS_DIR = os.path.join(FUZZ_DIR, "corpus")


def ssh_string(payload):
    if isinstance(payload, str):
        payload = payload.encode()
    return struct.pack(">I", len(payload)) + payload


def name_list(*names):
    return ssh_string(",".join(names))


def ssh_wire_seeds():
    """One input per branch of the ssh harness, selector byte first."""

    kexinit = bytes([20]) + bytes(range(16))
    for _ in range(10):
        kexinit += name_list("curve25519-sha256", "ecdh-sha2-nistp256")
    kexinit += bytes([0]) + struct.pack(">I", 0)

    ecdh = bytes([30]) + ssh_string(bytes(32))

    password = bytes([50]) + ssh_string("pdiStack") + ssh_string("ssh-connection")
    password += ssh_string("password") + bytes([0]) + ssh_string("pdiStack@123")

    publickey = bytes([50]) + ssh_string("pdiStack") + ssh_string("ssh-connection")
    publickey += ssh_string("publickey") + bytes([1]) + ssh_string("ssh-ed25519")
    publickey += ssh_string(ssh_string("ssh-ed25519") + ssh_string(bytes(32)))
    publickey += ssh_string(ssh_string("ssh-ed25519") + ssh_string(bytes(64)))

    channelreq = bytes([98]) + struct.pack(">I", 0) + ssh_string("pty-req") + bytes([1])
    channelreq += ssh_string("xterm-256color") + struct.pack(">IIII", 80, 24, 0, 0)
    channelreq += ssh_string(bytes([0]))

    channeldata = bytes([94]) + struct.pack(">I", 0) + ssh_string("ls -l\r\n")

    fields = ssh_string("a,b,c") + ssh_string(bytes(32))

    # an unencrypted packet as it arrives off the socket: length, padding
    # length, payload, padding
    body = bytes([21])
    padding = bytes(8)
    wire = struct.pack(">I", 1 + len(body) + len(padding))
    wire += bytes([len(padding)]) + body + padding

    return {
        "kexinit": bytes([0]) + kexinit,
        "ecdh_init": bytes([1]) + ecdh,
        "userauth_password": bytes([2]) + password,
        "userauth_publickey": bytes([2]) + publickey,
        "channel_request": bytes([3]) + channelreq,
        "channel_data": bytes([4]) + channeldata,
        "string_fields": bytes([5]) + fields,
        "wire_packet": bytes([6]) + wire,
    }


def sftp_packet(fxp_type, request_id, body=b""):
    payload = bytes([fxp_type]) + struct.pack(">I", request_id) + body
    return struct.pack(">I", len(payload)) + payload


def sftp_seeds():
    """Selector byte, chunk count byte, then the sftp stream itself."""

    init = struct.pack(">I", 5) + bytes([1]) + struct.pack(">I", 3)
    realpath = sftp_packet(16, 1, ssh_string("."))
    opendir = sftp_packet(11, 2, ssh_string("/"))
    readdir = sftp_packet(12, 3, ssh_string("handle"))
    close = sftp_packet(4, 4, ssh_string("handle"))
    stat = sftp_packet(17, 5, ssh_string("/etc"))

    session = init + realpath + opendir + readdir + close

    return {
        "channel_stream": bytes([0, 0]) + session,
        "channel_split": bytes([0, 3]) + session,
        "bolus_stream": bytes([1, 0]) + session,
        "bolus_short_close": bytes([1, 0]) + close,
        "stat": bytes([0, 0]) + init + stat,
    }


def http_seeds():
    """Whole requests, exactly as they arrive on the socket."""

    boundary = "----pdiboundary"
    body = "--%s\r\n" % boundary
    body += 'Content-Disposition: form-data; name="file"; filename="app.bin"\r\n'
    body += "Content-Type: application/octet-stream\r\n\r\n"
    body += "\x00\x01\x02binary payload\x00\r\n"
    body += "--%s--\r\n" % boundary

    upload = "POST /upload HTTP/1.1\r\nHost: pdi.local\r\n"
    upload += "Content-Type: multipart/form-data; boundary=%s\r\n" % boundary
    upload += "Content-Length: %d\r\n\r\n" % len(body)
    upload += body

    form = "username=pdiStack&password=pdiStack%40123&remember=1"
    login = "POST /login HTTP/1.1\r\nHost: pdi.local\r\n"
    login += "Content-Type: application/x-www-form-urlencoded\r\n"
    login += "Content-Length: %d\r\n\r\n%s" % (len(form), form)

    get = "GET /index.html?a=1&b=two HTTP/1.1\r\nHost: pdi.local\r\n"
    get += "Cookie: pdisession=deadbeef\r\nConnection: keep-alive\r\n\r\n"

    return {
        "get_with_query": get.encode(),
        "post_urlencoded": login.encode(),
        "post_multipart": upload.encode(),
        "bare_line": b"GET /\r\n\r\n",
    }


def http_response_seeds():
    """Mode byte, writer budget byte, then the reply as the server sent it."""

    ok = "HTTP/1.1 200 OK\r\nServer: SimpleHTTP/0.6 Python/3.12.3\r\n"
    ok += "Content-Type: application/octet-stream\r\nContent-Length: 12\r\n"
    ok += "Connection: close\r\n\r\nhello pdi!!\n"

    chunked = "HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\n"
    chunked += "8\r\npdistack\r\n4\r\nfuzz\r\n0\r\n\r\n"

    redirect = "HTTP/1.1 301 Moved Permanently\r\n"
    redirect += "Location: http://pdi.local/moved.bin\r\nContent-Length: 0\r\n\r\n"

    # a header whose value carries a status line of its own, which is the
    # shape that once overwrote the code the server actually sent
    spoofed = "HTTP/1.1 200 OK\r\nX-Note: HTTP/9.9 404 nope\r\n"
    spoofed += "Content-Length: 3\r\n\r\npdi"

    nolength = "HTTP/1.1 200 OK\r\nConnection: close\r\n\r\nno length declared"

    return {
        "buffered_ok": bytes([0, 0]) + ok.encode(),
        "buffered_chunked": bytes([0, 0]) + chunked.encode(),
        "buffered_redirect": bytes([0, 0]) + redirect.encode(),
        "buffered_spoofed_status": bytes([0, 0]) + spoofed.encode(),
        "streamed_ok": bytes([1, 0]) + ok.encode(),
        "streamed_chunked": bytes([1, 0]) + chunked.encode(),
        "streamed_nolength": bytes([1, 0]) + nolength.encode(),
        "streamed_aborted": bytes([1, 1]) + chunked.encode(),
        "url_plain": bytes([2, 0]) + b"http://pdi.local:8080/dir/file.bin?a=1&b=2",
        "url_secure": bytes([2, 0]) + b"https://user:pass@pdi.local/deep/path.bin",
    }


def crontab_seeds():
    """Value, low and high bytes first, then one table row."""

    head = bytes([30, 0, 59])

    return {
        "every_minute": head + b"* * * * * echo tick",
        "step": head + b"*/5 * * * * ls -l /etc",
        "range": head + b"0 9-17 * * 1-5 cat /proc/uptime",
        "list": head + b"0,15,30,45 * * * * df",
        "comment": head + b"# minute hour dom mon dow command",
        "ragged": head + b"  *   *  *  *  *   echo   spaced   out  ",
        "short_row": head + b"* * * echo not enough fields",
    }


def devices_seeds():
    """One /etc/devices.conf line."""

    return {
        "i2c_hex": b"i2c0 0x76 bme280",
        "i2c_decimal": b"i2c1 96 bme280",
        "spi_cs": b"spi0 cs=5 spiflash",
        "i2s_rx": b"i2s0 rx mic",
        "usb": b"usb0 3 keyboard",
        "tabs": b"\tuart1\t0\tgps\t",
        "comment": b"# bus target driver",
        "extra_field": b"i2c0 0x76 bme280 extra",
        "wide_target": b"i2c0 0x1ffffffff bme280",
    }


def mdns_seeds():
    """One query packet: a header asking one or more questions, then each question."""

    def name(*labels):
        return b"".join(bytes([len(l)]) + l for l in labels) + b"\x00"

    def query(*questions):
        header = bytes([0, 0, 0, 0, 0, len(questions), 0, 0, 0, 0, 0, 0])
        return header + b"".join(n + bytes([0, t, 0, 1]) for n, t in questions)

    host = name(b"pdi-a1b2c3", b"local")
    return {
        "host_a": query((host, 1)),
        "host_any": query((host, 255)),
        "enumerate": query((name(b"_services", b"_dns-sd", b"_udp", b"local"), 12)),
        "browse": query((name(b"_ssh", b"_tcp", b"local"), 12)),
        "resolve": query((name(b"pdi-a1b2c3", b"_https", b"_tcp", b"local"), 33)),
        "two_questions": query((name(b"other", b"local"), 1), (host, 1)),
        "compressed": query((b"\xc0\x0c", 1)),
    }


def mqtt_seeds():
    """What a broker sends after its CONNACK, which the harness puts in front."""

    publish0 = bytes([0x30, 0x0B, 0x00, 0x03]) + b"a/b" + b'{"x":1}'
    publish1 = bytes([0x32, 0x0D, 0x00, 0x03]) + b"a/b" + b"\x00\x05" + b'{"x":1}'
    publish2 = bytes([0x34, 0x0D, 0x00, 0x03]) + b"a/b" + b"\x00\x06" + b'{"x":2}'
    return {
        "publish_qos0": publish0,
        "publish_qos1": publish1,
        "publish_qos2_then_release": publish2 + bytes([0x62, 0x02, 0x00, 0x06]),
        "acks": bytes([0x40, 0x02, 0x00, 0x01, 0x50, 0x02, 0x00, 0x02, 0x70, 0x02, 0x00, 0x02]),
        "suback": bytes([0x90, 0x03, 0x00, 0x01, 0x01]),
        "pingresp": bytes([0xD0, 0x00]),
        "long_length": bytes([0x30, 0x80, 0x01]) + b"\x00\x01t" + b"x" * 125,
        "empty_payload": bytes([0x30, 0x03, 0x00, 0x01]) + b"t",
    }


def json_seeds():
    """Key and buffer width first, then the payload."""

    return {
        "gpio": bytes([1, 40]) + b'{"data":{"D10":{"mode":1,"val":1},"D1":{"mode":2,"val":0}}}',
        "version": bytes([7, 8]) + b'{"note":"latest: 9","latest":"12"}',
        "nested": bytes([0, 63]) + b'{"data":{"a0":{"cmp":1,"val":[1,2,{"x":"}"}]}}}',
        "escaped": bytes([8, 20]) + b'{"did":"a\\"b\\\\","channelhost" : "h"}',
        "unterminated": bytes([3, 5]) + b'{"a0":"never closed',
    }


def shell_seeds():
    """Keystrokes, including the escape sequences the line editor decodes."""

    return {
        "login": b"pdiStack\r\npdiStack@123\r\n",
        "command": b"pdiStack\r\npdiStack@123\r\nls -l /etc\r\n",
        "pipeline": b"pdiStack\r\npdiStack@123\r\ncat /etc/hosts | head 2 > /tmp/o\r\n",
        "arrow_keys": b"abc\x1b[D\x1b[C\x1b[A\x1b[B\x7f\x7f\r\n",
        "control_bytes": b"\x03\x04\x1a\x00\x7f\x09\x0b\r\n",
        "telnet_iac": b"\xff\xfd\x18\xff\xfb\x1f\xff\xfa\x1f\x00\x50\x00\x18\xff\xf0pdiStack\r\n",
        "cr_nul": b"pdiStack\r\x00pdiStack@123\r\x00",
    }


def config_seeds():
    """Files as the /etc readers expect to find them."""

    return {
        "ssh_config": b"port 22\npermitrootlogin yes\n# a comment\nmaxsessions 4\n",
        "hosts": b"127.0.0.1 localhost\n192.168.1.10   pdi.local  \n",
        "ragged": b"  key\tvalue with spaces   \n\n#\nnovalue\n\r\n",
    }


def dbrecord_seeds():
    """Table count and sizes first, then the bytes of the store."""

    blank = bytes([1, 32, 0]) + b"\xff" * 256
    zeroed = bytes([2, 32, 0, 64, 1]) + b"\x00" * 256
    return {
        "blank_store": blank,
        "zeroed_store": zeroed,
    }


TARGETS = {
    "fuzz_ssh_wire": ssh_wire_seeds,
    "fuzz_sftp": sftp_seeds,
    "fuzz_http": http_seeds,
    "fuzz_http_response": http_response_seeds,
    "fuzz_shell": shell_seeds,
    "fuzz_config": config_seeds,
    "fuzz_dbrecord": dbrecord_seeds,
    "fuzz_crontab": crontab_seeds,
    "fuzz_mdns": mdns_seeds,
    "fuzz_json": json_seeds,
    "fuzz_mqtt": mqtt_seeds,
    "fuzz_devices": devices_seeds,
}


def write_corpus():
    written = 0

    for target, builder in TARGETS.items():
        directory = os.path.join(CORPUS_DIR, target)
        os.makedirs(directory, exist_ok=True)

        for name, payload in builder().items():
            path = os.path.join(directory, "%s.bin" % name)
            with open(path, "wb") as handle:
                handle.write(payload)
            written += 1

    return written


if __name__ == "__main__":
    count = write_corpus()
    print("wrote %d seeds under %s" % (count, CORPUS_DIR))
    sys.exit(0)
