#!/usr/bin/env python3

"""
Devices and drivers, the way a user meets them: udevadm, /sys/bus and
/etc/devices.conf.

The serial service looks on every serial interface the port made every couple
of seconds, binds a driver to what turned up and lets go of what has gone. The
tests that need something plugged in run against the host process, whose
simulated buses take their devices from a file; the rest run on any target.
"""

import re
import time

from .registry import test, Skip, expect_in, expect_not_in
from ..driver.shell import CTRL_C

DEVICES_CONF = "/etc/devices.conf"
SCAN_WAIT = 8.0


def plugger(t):
    """The way to plug a device in, or a skip on a target with real buses."""
    plug = getattr(t.shell, "plug_devices", None)
    if plug is None:
        raise Skip("only the host process can have a device plugged in on cue")
    return plug


def wait_for(t, command, needle, present=True):
    """Re-run the command until the needle shows, or stops showing."""
    deadline = time.time() + SCAN_WAIT
    out = ""
    while time.time() < deadline:
        out = t.run(command)
        if (needle in out) == present:
            return out
        time.sleep(0.5)
    raise AssertionError("%r %s in %r within %.0f s:\n%s"
                         % (needle, "never showed" if present else "never went away",
                            command, SCAN_WAIT, out))


class Declarations(object):
    """Puts /etc/devices.conf back the way it was found."""

    def __init__(self, t):
        self.t = t
        self.saved = None

    def __enter__(self):
        listing = self.t.run("ls /etc")
        if "devices.conf" in listing:
            self.saved = [line for line in self.t.run("cat %s" % DEVICES_CONF).splitlines()
                          if line.strip()]
        return self

    def _replace(self, lines):
        """
        Puts the lines in place in one step: written beside the file, then moved
        over it. Written in place line by line, a scan between two lines finds a
        declaration missing, lets that device go, and binds it again as a new one
        once the line comes back, which leaves a mounted card unreachable.
        """
        temp = DEVICES_CONF + ".tmp"
        self.t.run("rm %s" % temp)
        if not lines:
            self.t.run("rm %s" % DEVICES_CONF)
        else:
            for line in lines:
                self.t.run("echo '%s' >> %s" % (line, temp))
            self.t.run("mv %s %s" % (temp, DEVICES_CONF))
        self.t.run("udevadm trigger")

    def write(self, lines):
        """
        The declarations the board started with, then the test's own, so a
        device the board really has bound stays bound through a test about
        other lines.
        """
        self._replace(list(self.saved or []) + list(lines))

    def __exit__(self, *exc):
        self._replace(list(self.saved or []))


@test("udevadm info lists the devices under a header", needs=("udevadm",))
def info_header(t):
    out = t.run("udevadm info")
    expect_in("BUS", out, "the header of the device list")
    expect_in("DRIVER", out, "the header of the device list")
    expect_in("STATE", out, "the header of the device list")


@test("udevadm refuses a verb it does not know", needs=("udevadm",))
def unknown_verb(t):
    expect_in("CmdErr", t.run("udevadm sideways"), "an unknown udevadm verb")


@test("udevadm trigger answers and the shell carries on", needs=("udevadm", "echo"))
def trigger_answers(t):
    t.run("udevadm trigger")
    expect_in("stillhere", t.run("echo stillhere"), "the shell after a trigger")


@test("sysfs lists a bus for the serial port every board has", needs=("ls",), mounts=("/sys",))
def sysfs_bus(t):
    expect_in("bus", t.run("ls /sys"), "/sys")
    expect_in("uart", t.run("ls /sys/bus"), "/sys/bus")


@test("a declared device that is not there is listed as absent",
      needs=("udevadm", "echo", "rm", "cat", "ls"), mounts=("/",))
def declared_absent(t):
    with Declarations(t) as conf:
        conf.write(["i2c1 0x77 bme280"])
        out = t.run("udevadm info")
        expect_in("1-0077", out, "a declared device in udevadm info")
        expect_in("absent", out, "the state of a declared device with nothing there")


@test("a declaration taken out of the file is forgotten",
      needs=("udevadm", "echo", "rm", "cat", "ls"), mounts=("/",))
def declaration_removed(t):
    with Declarations(t) as conf:
        conf.write(["i2c1 0x77 bme280"])
        conf.write([])
        expect_not_in("1-0077", t.run("udevadm info"), "a device no longer declared")


@test("a usb device plugged in is found within a scan and named by its ids",
      needs=("udevadm", "cat"), mounts=("/sys",))
def usb_plugged(t):
    plug = plugger(t)
    try:
        plug(["usb0 2 046d c31c 03"])
        wait_for(t, "udevadm info", "1-2")
        expect_in("046d", t.run("cat /sys/bus/usb/devices/1-2/idVendor"), "the vendor id")
        expect_in("c31c", t.run("cat /sys/bus/usb/devices/1-2/idProduct"), "the product id")
        expect_in("03", t.run("cat /sys/bus/usb/devices/1-2/bDeviceClass"), "the class")
    finally:
        plug([])
        wait_for(t, "udevadm info", "1-2", present=False)


@test("a usb device unplugged is gone within a scan", needs=("udevadm", "ls"), mounts=("/sys",))
def usb_unplugged(t):
    plug = plugger(t)
    plug(["usb0 4 0781 5567 08"])
    wait_for(t, "udevadm info", "1-4")
    plug([])
    wait_for(t, "udevadm info", "1-4", present=False)
    expect_not_in("1-4", t.run("ls /sys/bus/usb/devices"), "an unplugged device's directory")


@test("a declared chip that answers but has no driver is listed as failed",
      needs=("udevadm", "echo", "rm", "cat", "ls"), mounts=("/",))
def declared_without_driver(t):
    plug = plugger(t)
    with Declarations(t) as conf:
        try:
            plug(["i2c0 0x76 60"])
            conf.write(["i2c0 0x76 bme280"])
            out = wait_for(t, "udevadm info", "failed")
            expect_in("0-0076", out, "the declared chip in udevadm info")
        finally:
            plug([])


EEPROM_DECLARATION = "i2c0 0x50 at24 chip=24c02 size=256 page=8"
EEPROM_FILE = "/sys/bus/i2c/devices/0-0050/eeprom"


def eeprom_bound(t):
    """Whether an eeprom is bound on the target already."""
    for line in t.run("udevadm info").splitlines():
        if "at24" in line and "bound" in line:
            return True
    return False


@test("a declared eeprom that answers is bound and reads through its eeprom file",
      needs=("udevadm", "echo", "rm", "cat", "ls", "hexdump"), mounts=("/", "/sys"))
def eeprom_plugged(t):
    plug = plugger(t)
    with Declarations(t) as conf:
        try:
            plug(["i2c0 0x50 70 64 69 21"])
            conf.write([EEPROM_DECLARATION])
            wait_for(t, "udevadm info", "bound")
            listing = t.run("ls /sys/bus/i2c/devices/0-0050")
            expect_in("eeprom", listing, "the eeprom file of a bound eeprom")
            expect_in("256", listing, "the size of the eeprom file")
            expect_in("pdi!", t.run("hexdump %s" % EEPROM_FILE), "the bytes the eeprom holds")
        finally:
            plug([])


@test("a declared eeprom unplugged loses its eeprom file within a scan",
      needs=("udevadm", "echo", "rm", "cat", "ls"), mounts=("/", "/sys"))
def eeprom_unplugged(t):
    plug = plugger(t)
    with Declarations(t) as conf:
        try:
            plug(["i2c0 0x50 70 64 69 21"])
            conf.write([EEPROM_DECLARATION])
            wait_for(t, "udevadm info", "bound")
            plug([])
            wait_for(t, "udevadm info", "absent")
            expect_not_in("0-0050", t.run("ls /sys/bus/i2c/devices"), "an unplugged eeprom's directory")
        finally:
            plug([])


@test("a declared eeprom without its size is refused",
      needs=("udevadm", "echo", "rm", "cat", "ls"), mounts=("/",))
def eeprom_without_size(t):
    plug = plugger(t)
    with Declarations(t) as conf:
        try:
            plug(["i2c0 0x50 70 64 69 21"])
            conf.write(["i2c0 0x50 at24 chip=24c02 page=8"])
            out = wait_for(t, "udevadm info", "failed")
            expect_in("0-0050", out, "the refused eeprom in udevadm info")
        finally:
            plug([])


@test("the eeprom the board has bound reads through its eeprom file",
      needs=("udevadm", "ls", "hexdump"), mounts=("/sys",))
def eeprom_on_board(t):
    if not eeprom_bound(t):
        raise Skip("no eeprom declared and bound on this target")
    expect_in("eeprom", t.run("ls /sys/bus/i2c/devices/0-0050"), "the eeprom file of the bound eeprom")
    expect_in("0000", t.run("hexdump %s" % EEPROM_FILE), "the first line of the eeprom")


CARD_DECLARATION = "spi0 cs=12 mmc_spi"
CARD_PLUGGED = "spi0 12 sd hc 4096"


def card_bound(t):
    """Whether an SD card is bound on the target already."""
    for line in t.run("udevadm info").splitlines():
        if "mmc_spi" in line and "bound" in line:
            return True
    return False


@test("a declared SD card is a disk that lsblk, /dev and /sys/block show",
      needs=("udevadm", "echo", "rm", "cat", "ls", "lsblk"), mounts=("/", "/sys", "/dev"))
def card_plugged(t):
    plug = plugger(t)
    with Declarations(t) as conf:
        try:
            plug([CARD_PLUGGED])
            conf.write([CARD_DECLARATION])
            wait_for(t, "udevadm info", "bound")
            out = t.run("lsblk")
            expect_in("mmcblk0", out, "the card in lsblk")
            expect_in("4194304", out, "the card's size in lsblk")
            expect_in("disk", out, "the card's type in lsblk")
            expect_in("mmcblk0", t.run("ls /dev"), "the card's node under /dev")
            expect_in("8192", t.run("cat /sys/block/mmcblk0/size"), "the card's size in sectors")
        finally:
            plug([])


@test("what is written to the SD card's disk reads back",
      needs=("udevadm", "echo", "rm", "cat", "ls", "head"), mounts=("/", "/dev"))
def card_write_read(t):
    plug = plugger(t)
    with Declarations(t) as conf:
        try:
            plug([CARD_PLUGGED])
            conf.write([CARD_DECLARATION])
            wait_for(t, "udevadm info", "bound")
            t.run("echo hello from the card > /dev/mmcblk0")
            expect_in("hello from the card", t.run("head /dev/mmcblk0 1"), "the line written to the card")
        finally:
            plug([])


@test("a declared SD card with nothing behind its chip select is listed absent",
      needs=("udevadm", "echo", "rm", "cat", "ls"), mounts=("/",))
def card_missing(t):
    plugger(t)
    with Declarations(t) as conf:
        conf.write([CARD_DECLARATION])
        out = wait_for(t, "udevadm info", "absent")
        expect_in("spi0.12", out, "the missing card in udevadm info")
        expect_not_in("mmcblk0", t.run("ls /dev"), "a disk node for a slot with no card")


@test("the SD card the board has bound is listed by lsblk",
      needs=("udevadm", "lsblk"), mounts=("/dev",))
def card_on_board(t):
    if not card_bound(t):
        raise Skip("no SD card declared and bound on this target")
    expect_in("mmcblk0", t.run("lsblk"), "the bound card in lsblk")


MIC_DECLARATION = "i2s0 rx inmp441 sck=3 ws=4 sd=5"
RECORDING = "/rec.wav"
RECORD_WAIT = 10.0


def mic_bound(t):
    """Whether a microphone is bound on the target already."""
    for line in t.run("udevadm info").splitlines():
        if "inmp441" in line and "bound" in line:
            return True
    return False


RECORD_ROOM = 48 * 1024


def recording_path(t):
    """
    Where a one second recording fits: the card when one is mounted with room,
    else the root; a skip when neither has room for it.
    """
    for mount, path in (("/mnt", "/mnt/rec.wav"), ("/", RECORDING)):
        if mount != "/" and mount not in t.caps.mounts:
            continue
        for line in t.run("df").splitlines():
            cols = line.split()
            if len(cols) >= 5 and cols[0] == mount and cols[4].isdigit() \
                    and int(cols[4]) >= RECORD_ROOM:
                return path
    raise Skip("no mounted filesystem has %d bytes free for a recording" % RECORD_ROOM)


def record_one_second(t, path=None):
    """Records a second into a file with room for it and gives back what it said."""
    path = path or RECORDING
    t.run("rm %s" % path)
    try:
        t.shell.send_line("arecord -d 1 %s" % path)
        t.shell.expect("recording %s" % path, RECORD_WAIT)
        return t.shell.expect(re.compile(r"recorded [0-9]+ bytes to %s" % re.escape(path)), RECORD_WAIT)
    finally:
        t.resync()
        t.run("rm %s" % path)


@test("a declared mic that streams is bound and gets a capture node",
      needs=("udevadm", "echo", "rm", "cat", "ls"), mounts=("/", "/dev"))
def mic_bound_with_node(t):
    plug = plugger(t)
    with Declarations(t) as conf:
        try:
            plug(["i2s0 noise"])
            conf.write([MIC_DECLARATION])
            wait_for(t, "udevadm info", "bound")
            expect_in("pcmC0D0c", t.run("ls /dev/snd"), "the capture node of a bound mic")
        finally:
            plug([])


@test("a declared mic with a flat data line is refused",
      needs=("udevadm", "echo", "rm", "cat", "ls"), mounts=("/",))
def mic_silent_refused(t):
    plug = plugger(t)
    with Declarations(t) as conf:
        try:
            plug(["i2s0 silent"])
            conf.write([MIC_DECLARATION])
            out = wait_for(t, "udevadm info", "failed")
            expect_in("i2s0.0", out, "the refused mic in udevadm info")
        finally:
            plug([])


@test("arecord records a second from a plugged mic",
      needs=("udevadm", "arecord", "echo", "rm", "cat", "ls"), mounts=("/",))
def arecord_plugged(t):
    plug = plugger(t)
    with Declarations(t) as conf:
        try:
            plug(["i2s0 noise"])
            conf.write([MIC_DECLARATION])
            wait_for(t, "udevadm info", "bound")
            expect_in("recorded 32000 bytes", record_one_second(t), "a second at the default rate")
        finally:
            plug([])


@test("arecord records a second from the mic the board has bound",
      needs=("udevadm", "arecord", "rm"), mounts=("/",))
def arecord_on_board(t):
    if not mic_bound(t):
        raise Skip("no mic declared and bound on this target")
    expect_in("recorded", record_one_second(t, recording_path(t)), "a recording from the bound mic")


@test("arecord with no mic bound says so", needs=("udevadm", "arecord", "ls"), mounts=("/",))
def arecord_without_mic(t):
    if mic_bound(t):
        raise Skip("a mic is bound on this target")
    expect_in("no capture device", t.run("arecord -d 1 %s" % RECORDING), "arecord with no mic")
    expect_not_in("rec.wav", t.run("ls /"), "a recording that never started")


@test("udevadm monitor prints a device as it comes and goes", needs=("udevadm",))
def monitor_events(t):
    plug = plugger(t)
    try:
        t.shell.send_line("udevadm monitor")
        t.shell.expect("monitor will print the received events", SCAN_WAIT)
        plug(["usb0 5 046d c52b 03"])
        t.shell.expect("add usb0 1-5", SCAN_WAIT)
        plug([])
        t.shell.expect("remove usb0 1-5", SCAN_WAIT)
    finally:
        plug([])
        t.shell.send_raw(CTRL_C)
        t.resync()
