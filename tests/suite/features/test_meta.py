#!/usr/bin/env python3

"""
Where the session is, what is mounted, and who may touch what.

Mirrors tests/host/system/test_commands_meta.cpp, read back through the shell.
"""

from .registry import test, expect_in, expect_not_in

W = "wm_"


@test("pwd starts at the root", needs=("pwd",))
def pwd_root(t):
    t.run("cd /")
    expect_in("/", t.run("pwd"), "pwd")


@test("cd moves into a directory", needs=("cd", "pwd", "mkdir"))
def cd_moves(t):
    path = t.workspace(W + "cd")
    expect_in(path, t.run("pwd"), "pwd after cd")


@test("cd into a missing directory leaves us where we were", needs=("cd", "pwd", "mkdir"))
def cd_missing(t):
    path = t.workspace(W + "cdstay")
    t.run("cd /nosuchplace")

    expect_in(path, t.run("pwd"), "pwd after a failed cd")


@test("cd to a relative name appends to the current directory", needs=("cd", "pwd", "mkdir"))
def cd_relative(t):
    path = t.workspace(W + "rel")
    t.run("mkdir inner")
    t.run("cd inner")

    expect_in(path + "/inner", t.run("pwd"), "pwd after a relative cd")


@test("cd to dot dot goes back up", needs=("cd", "pwd", "mkdir"))
def cd_up(t):
    path = t.workspace(W + "up")
    t.run("mkdir deep")
    t.run("cd deep")
    t.run("cd ..")

    expect_in(path, t.run("pwd"), "pwd after cd ..")


@test("a relative file resolves against the current directory", needs=("cd", "echo", "cat", "mkdir"))
def relative_file(t):
    path = t.workspace(W + "resolve")
    t.run("echo local file > here.txt")
    t.run("cd /")

    expect_in("local file", t.run("cat %s/here.txt" % path), "absolute read of a relative write")


@test("df reports every mount with its sizes", needs=("df",))
def df_reports(t):
    out = t.run("df")
    for name in ("rootfs", "procfs", "sysfs", "devfs", "tmpfs"):
        expect_in(name, out, "df")

    expect_in("TOTAL", out, "df header")
    expect_in("FREE", out, "df header")


@test("mount lists the prefixes and types", needs=("mount",))
def mount_lists(t):
    out = t.run("mount")
    for prefix in ("/proc", "/sys", "/dev", "/tmp"):
        expect_in(prefix, out, "mount")

    expect_in("littlefs", out, "the backing store is named")


@test("chmod changes the mode bits", needs=("chmod", "touch", "ls"))
def chmod_changes(t):
    t.workspace(W + "chmod")
    t.run("touch f.txt")

    t.run("chmod 400 f.txt")
    expect_in("-r--", t.run("ls"), "mode after chmod 400")

    t.run("chmod 640 f.txt")
    expect_in("-rw-r--", t.run("ls"), "mode after chmod 640")


@test("chown changes the owner", needs=("chown", "touch", "ls"))
def chown_changes(t):
    t.workspace(W + "chown")
    t.run("touch owned.txt")

    t.run("chown 7 owned.txt")
    expect_in("7", t.run("ls"), "owner column after chown")


@test("chown takes a separate group", needs=("chown", "touch", "ls"))
def chown_group(t):
    t.workspace(W + "chowng")
    t.run("touch grouped.txt")

    t.run("chown 7:9 grouped.txt")
    out = t.run("ls")
    expect_in("7", out, "owner column")
    expect_in("9", out, "group column")


@test("umask prints the current value", needs=("umask",))
def umask_prints(t):
    expect_in("0", t.run("umask"), "umask")


@test("umask takes a new value and keeps it", needs=("umask",))
def umask_keeps(t):
    before = t.run("umask").strip()
    try:
        t.run("umask 077")
        expect_in("077", t.run("umask"), "umask after being set")
    finally:
        t.run("umask %s" % (before.split()[-1] if before.split() else "022"))


@test("the umask takes bits off a new file", needs=("umask", "touch", "ls", "mkdir"))
def umask_applies(t):
    t.workspace(W + "umask")
    before = t.run("umask").strip()
    try:
        t.run("umask 077")
        t.run("touch masked.txt")

        out = t.run("ls")
        expect_in("-rw-------", out, "a file made under umask 077")
        expect_not_in("-rw-r--r--", out, "no group or other bits")
    finally:
        t.run("umask %s" % (before.split()[-1] if before.split() else "022"))


def uname_rc(t, command):
    for line in t.run("%s; echo rc=$?" % command).splitlines():
        if line.strip().startswith("rc="):
            return line.strip()[3:]
    return None


@test("uname with no option prints the system name alone", needs=("uname",))
def uname_system_name(t):
    words = t.run("uname").split()
    if 1 != len(words):
        raise AssertionError("uname printed %r; the system name is one word" % words)


@test("uname -n prints the host name the environment holds", needs=("uname", "echo"))
def uname_host_matches_env(t):
    host = t.run("echo $HOSTNAME").strip()
    expect_in(host, t.run("uname -n"), "uname -n against $HOSTNAME")


@test("uname -a prints every field in linux order", needs=("uname", "echo", "cat"))
def uname_all_in_order(t):
    words = t.run("uname -a").split()
    if 5 != len(words):
        raise AssertionError("uname -a printed %d fields, expected 5: %r" % (len(words), words))
    if words[0] != t.run("uname -s").strip():
        raise AssertionError("uname -a does not start with the system name: %r" % words)
    if words[1] != t.run("echo $HOSTNAME").strip():
        raise AssertionError("the second field of uname -a is not the host name: %r" % words)
    if words[2] not in t.run("cat /proc/version"):
        raise AssertionError("the release uname -a prints is not the one /proc/version names: %r" % words)


@test("uname prints its fields in one order whatever order the options are given", needs=("uname",))
def uname_order_is_fixed(t):
    if t.run("uname -rs").split() != t.run("uname -s -r").split():
        raise AssertionError("uname -rs and uname -s -r printed different orders")


@test("uname refuses an option it does not know", needs=("uname", "echo"))
def uname_unknown_option(t):
    if "0" == uname_rc(t, "uname -x"):
        raise AssertionError("uname -x reported success")


@test("hostname prints the host name the environment holds", needs=("hostname", "echo"))
def hostname_matches_env(t):
    host = t.run("echo $HOSTNAME").strip()
    if not host or t.run("hostname").strip() != host:
        raise AssertionError("hostname did not print $HOSTNAME (%r)" % host)


@test("hostname and uname -n agree", needs=("hostname", "uname"))
def hostname_matches_uname(t):
    if t.run("hostname").strip() != t.run("uname -n").strip():
        raise AssertionError("hostname and uname -n named different hosts")


@test("hostname refuses an argument it cannot act on", needs=("hostname", "echo"))
def hostname_refuses_argument(t):
    if "0" == uname_rc(t, "hostname newname"):
        raise AssertionError("hostname with a name reported success but the name cannot be set")
