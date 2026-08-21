#!/usr/bin/env python3

"""
Tier 1 smoke — the whole stack running as a host process.

These are the checks that only mean something once every layer is present at
the same time: the shell reaching a prompt over a real descriptor, the mounts
the storage stack brings up, the listeners the network services open, and the
config and filesystem surviving a restart. Feature depth belongs in the unit
tier and in the feature suites; this is the tier that proves the device boots.
"""

import os
import sys
import time

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

from suite.driver.host_shell import HostShell
from suite.driver.shell import PROMPT

CHECKS = []


def check(name):
    def register(fn):
        CHECKS.append((name, fn))
        return fn
    return register


def expect_in(needle, haystack, what):
    if needle not in haystack:
        raise AssertionError("%s: expected %r in:\n%s" % (what, needle, haystack))


@check("boots to a login prompt")
def boots(shell):
    banner = shell.wait_for_boot(timeout=30)
    expect_in("Starting PDIStack", banner, "boot banner")
    expect_in("Starting CMD Service", banner, "cmd service")


@check("every configured service starts")
def services_start(shell):
    shell.wait_for_boot(timeout=30)
    banner = shell.transcript
    for service in ("DB", "Serial", "WiFi", "GPIO", "FactoryReset", "Auth",
                    "UserStore", "HTTP Server", "Telnet", "SSH", "CMD"):
        expect_in("Starting %s Service" % service, banner, "service startup")


@check("the default credentials are accepted")
def login_succeeds(shell):
    shell.wait_for_boot(timeout=30)
    prompt = shell.login()
    if not PROMPT.search(prompt):
        raise AssertionError("no shell prompt after login:\n%s" % prompt)


@check("a wrong password is refused")
def login_refused(shell):
    shell.wait_for_boot(timeout=30)
    shell.send_line("pdiStack")
    shell.expect("Pass : ", timeout=20)
    shell.send_line("not-the-password")
    shell.expect("login: ", timeout=20)


@check("the session reports who and where it is")
def identity(shell):
    shell.wait_for_boot(timeout=30)
    shell.login()
    expect_in("/", shell.run("pwd"), "pwd")
    expect_in("pdiStack", shell.run("whoami"), "whoami")
    expect_in("uid=0", shell.run("id"), "id")


@check("every filesystem is mounted")
def mounts(shell):
    shell.wait_for_boot(timeout=30)
    shell.login()
    listing = shell.run("df")
    for mount in ("rootfs", "procfs", "sysfs", "devfs", "tmpfs"):
        expect_in(mount, listing, "df")


@check("the root directory is readable")
def root_listing(shell):
    shell.wait_for_boot(timeout=30)
    shell.login()
    listing = shell.run("ls /")
    expect_in("etc", listing, "ls /")


@check("the mount table lists every filesystem")
def mount_table(shell):
    shell.wait_for_boot(timeout=30)
    shell.login()
    table = shell.run("mount")
    for mount in ("/proc", "/sys", "/dev", "/tmp"):
        expect_in(mount, table, "mount")


@check("procfs reports the running version")
def procfs(shell):
    shell.wait_for_boot(timeout=30)
    shell.login()
    expect_in("PDI Stack version", shell.run("cat /proc/version"), "/proc/version")


@check("a gpio pin round trips through sysfs")
def sysfs_gpio(shell):
    shell.wait_for_boot(timeout=30)
    shell.login()

    shell.run("echo 1 > /sys/class/gpio/2/mode")
    expect_in("1", shell.run("cat /sys/class/gpio/2/mode"), "gpio mode")

    shell.run("echo 1 > /sys/class/gpio/2/value")
    expect_in("1", shell.run("cat /sys/class/gpio/2/value"), "gpio value")


@check("the user store is on disk")
def user_store(shell):
    shell.wait_for_boot(timeout=30)
    shell.login()
    expect_in("pdiStack", shell.run("cat /etc/passwd"), "/etc/passwd")


@check("the http, telnet and ssh listeners accept")
def listeners(shell):
    shell.wait_for_boot(timeout=30)
    for name, port in (("http", 80), ("telnet", 23), ("ssh", 22)):
        answered = shell.is_listening(port)
        if not answered:
            raise AssertionError("nothing accepting for %s on %d or its shadow" % (name, port))


@check("the ssh listener is the framework's own server")
def ssh_identifies_itself(shell):
    """
    Accepting is not enough. A developer machine tends to have its own ssh
    server on 22, and a check that only asks whether the port answers passes on
    that instead — which it did here until the identification string was read.
    """
    import socket

    shell.wait_for_boot(timeout=30)
    port = shell.is_listening(22)
    if not port:
        raise AssertionError("nothing accepting for ssh")

    connection = socket.create_connection(("127.0.0.1", port), timeout=3.0)
    try:
        banner = connection.recv(64).decode(errors="replace")
    finally:
        connection.close()

    if not banner.startswith("SSH-2.0-"):
        raise AssertionError("port %d did not answer with an ssh identification: %r" % (port, banner))

    expect_in("LWSSH", banner, "the ssh server is the framework's, not the host's")


@check("the http listener serves the portal")
def portal_serves_a_page(shell):
    """
    Same lesson as the ssh check, one layer up: a listener that accepts proves
    only that a socket is bound. The portal's routes are registered by
    controllers constructed in another translation unit, and when that
    registration is lost the port still accepts and then answers nothing at
    all. Asking for a page is what tells the two apart.
    """
    from .driver.portal import Portal

    shell.wait_for_boot(timeout=30)

    port = shell.is_listening(80)
    if not port:
        raise AssertionError("nothing accepting for http")

    answer = Portal("127.0.0.1", port, timeout=10.0).get("/")
    if 200 != answer.status:
        raise AssertionError("the portal home answered %d" % answer.status)

    expect_in("<html", answer.body, "the portal home page")


@check("a file written survives a restart")
def persistence(shell):
    shell.wait_for_boot(timeout=30)
    shell.login()
    shell.run("mkdir /var/smoke")
    shell.run("touch /var/smoke/kept")
    expect_in("kept", shell.run("ls /var/smoke"), "before restart")

    workdir = shell.workdir
    if shell.stop() != 0:
        raise AssertionError("pdid did not exit cleanly")

    again = HostShell(shell.binary, workdir=workdir)
    try:
        again.wait_for_boot(timeout=30)
        again.login()
        expect_in("kept", again.run("ls /var/smoke"), "after restart")
    finally:
        again.close()


@check("a tmpfs file does not survive a restart")
def tmpfs_is_volatile(shell):
    shell.wait_for_boot(timeout=30)
    shell.login()
    shell.run("echo scratch > /tmp/scratch.txt")
    expect_in("scratch", shell.run("cat /tmp/scratch.txt"), "before restart")

    workdir = shell.workdir
    if shell.stop() != 0:
        raise AssertionError("pdid did not exit cleanly")

    again = HostShell(shell.binary, workdir=workdir)
    try:
        again.wait_for_boot(timeout=30)
        again.login()
        listing = again.run("ls /tmp")
        if "scratch.txt" in listing:
            raise AssertionError("tmpfs content came back after a restart:\n%s" % listing)
    finally:
        again.close()


@check("a termination signal shuts the services down")
def shutdown(shell):
    shell.wait_for_boot(timeout=30)
    code = shell.stop()
    if code != 0:
        raise AssertionError("pdid exited with %d" % code)
    expect_in("pdid stopped", shell.transcript, "shutdown")


def run(binary, only=None, verbose=False):
    passed = 0
    failed = []

    started = time.time()
    for name, fn in CHECKS:
        if only and only not in name:
            continue

        shell = None
        try:
            shell = HostShell(binary)
            fn(shell)
            passed += 1
            print("  \033[32mok\033[0m   %s" % name, flush=True)
        except Exception as err:
            failed.append(name)
            print("  \033[31mFAIL\033[0m %s" % name, flush=True)
            print("       %s" % str(err).replace("\n", "\n       "), flush=True)
            if verbose and shell is not None:
                print("       --- transcript ---", flush=True)
                print("       " + shell.transcript.replace("\n", "\n       "), flush=True)
        finally:
            if shell is not None:
                shell.close()

    elapsed = (time.time() - started) * 1000.0
    print("")
    print("----------------------------------------------------------")
    if failed:
        print("\033[31m%d failed\033[0m, %d total in %.2f ms" % (len(failed), passed + len(failed), elapsed))
        return 1

    print("\033[32m%d passed\033[0m, %d total in %.2f ms" % (passed, passed, elapsed))
    return 0


if __name__ == "__main__":
    import argparse

    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("binary", help="path to pdid")
    parser.add_argument("--filter", help="run only checks whose name contains this")
    parser.add_argument("--verbose", action="store_true")
    opts = parser.parse_args()

    sys.exit(run(opts.binary, opts.filter, opts.verbose))
