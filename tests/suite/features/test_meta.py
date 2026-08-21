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
