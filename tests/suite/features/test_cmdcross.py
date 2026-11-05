#!/usr/bin/env python3

"""
The everyday commands checked against one another: the two ends of a file
against the whole of it, a count against a size, a copy against its source, a
status against && and ||, an identity against the session it came from. One
command's answer is the oracle for another's, so a fault in either shows.
"""

from .registry import test, expect_in, expect_not_in

W = "cx_"
OK = "RC_OK"
NO = "RC_NO"
LINES = ["l1", "l2", "l3", "l4", "l5"]


def rows(out):
    return [line.strip() for line in out.splitlines() if line.strip()]


def status(t, command):
    out = t.run("%s && echo %s || echo %s" % (command, OK, NO))
    if OK in out:
        return True
    if NO in out:
        return False
    raise AssertionError("no status came back from %r:\n%s" % (command, out))


def write_lines(t, path, lines):
    t.run("echo %s > %s" % (lines[0], path))
    for line in lines[1:]:
        t.run("echo %s >> %s" % (line, path))


def counts(t, command):
    got = t.run(command).split()
    if len(got) < 3 or not all(c.isdigit() for c in got[:3]):
        raise AssertionError("%s gave no counts: %r" % (command, got))
    return [int(c) for c in got[:3]]


@test("head and tail take the two ends of a file and together make the whole of it",
      needs=("head", "tail", "cat", "echo", "mkdir"))
def head_tail_cover_file(t):
    t.workspace(W + "ends")
    write_lines(t, "f", LINES)
    whole = rows(t.run("cat f"))
    top = rows(t.run("head f 2"))
    bottom = rows(t.run("tail f 3"))
    if top != LINES[:2] or bottom != LINES[2:]:
        raise AssertionError("head 2 %r, tail 3 %r" % (top, bottom))
    if top + bottom != whole:
        raise AssertionError("head and tail do not add up to cat")
    if rows(t.run("head f 9")) != whole or rows(t.run("tail f 9")) != whole:
        raise AssertionError("asking past the end did not give the whole file")


@test("head and tail read a pipe the way they read a file, and pick out a middle line",
      needs=("head", "tail", "cat", "echo", "mkdir"))
def head_tail_pipes(t):
    t.workspace(W + "ends2")
    write_lines(t, "f", LINES)
    if rows(t.run("cat f | head 2")) != rows(t.run("head f 2")):
        raise AssertionError("head of a pipe differs from head of the file")
    if rows(t.run("cat f | tail 2")) != rows(t.run("tail f 2")):
        raise AssertionError("tail of a pipe differs from tail of the file")
    if rows(t.run("head f 3 | tail 1")) != ["l3"]:
        raise AssertionError("head 3 | tail 1 did not give the third line")
    if rows(t.run("cat f | head 4 | tail 2 | head 1")) != ["l3"]:
        raise AssertionError("a four stage pipeline lost its place")


@test("grep names the file, line and column in a file, and only line and column in a pipe",
      needs=("grep", "cat", "echo", "mkdir"))
def grep_shapes(t):
    path = t.workspace(W + "grep")
    write_lines(t, "f", ["alpha", "beta", "gamma", "delta"])
    if rows(t.run("grep mm f")) != ["%s/f:3:2:gamma" % path]:
        raise AssertionError("grep in a file gave:\n%s" % t.run("grep mm f"))
    if rows(t.run("cat f | grep ta")) != ["2:2:beta", "4:3:delta"]:
        raise AssertionError("grep in a pipe gave:\n%s" % t.run("cat f | grep ta"))
    if rows(t.run("grep '^[bd]' f")) != ["%s/f:2:0:beta" % path, "%s/f:4:0:delta" % path]:
        raise AssertionError("grep with an anchor and a set gave:\n%s" % t.run("grep '^[bd]' f"))


@test("grep piped into wc counts the matching lines", needs=("grep", "wc", "cat", "echo", "mkdir"))
def grep_into_wc(t):
    t.workspace(W + "grepwc")
    write_lines(t, "f", ["x1", "y2", "x3", "x4", "z5"])
    if counts(t, "cat f | grep x | wc")[0] != 3:
        raise AssertionError("grep x | wc did not count three lines")
    if counts(t, "cat f | grep '[yz]' | wc")[0] != 2:
        raise AssertionError("grep with a set | wc did not count two lines")


@test("grep that finds nothing fails, so && and || can branch on it",
      needs=("grep", "echo", "mkdir"))
def grep_no_match_status(t):
    t.workspace(W + "grepst")
    t.run("echo something > f")
    if not status(t, "grep some f"):
        raise AssertionError("grep with a match did not succeed")
    if status(t, "grep nothing f"):
        raise AssertionError("grep with no match succeeded")
    if status(t, "cat f | grep nothing"):
        raise AssertionError("grep of a pipe with no match succeeded")
    if not status(t, "cat f | grep thing"):
        raise AssertionError("grep of a pipe with a match did not succeed")
    if status(t, "grep nothing ."):
        raise AssertionError("grep of a directory with no match succeeded")


@test("wc of a file and of the same bytes through a pipe agree, and follow an append",
      needs=("wc", "cat", "echo", "mkdir"))
def wc_file_and_pipe(t):
    t.workspace(W + "wc")
    write_lines(t, "f", ["one two", "three"])
    direct = counts(t, "wc f")
    piped = counts(t, "cat f | wc")
    if direct != piped or direct != [2, 3, 14]:
        raise AssertionError("wc f %r, cat f | wc %r" % (direct, piped))
    t.run("echo four five six >> f")
    if counts(t, "wc f") != [3, 6, 28]:
        raise AssertionError("wc after an append gave %r" % counts(t, "wc f"))


@test("hexdump shows each byte, sixteen to a row, with the text beside it",
      needs=("hexdump", "echo", "mkdir"))
def hexdump_rows(t):
    t.workspace(W + "hex")
    t.run("echo l1 > f")
    out = t.run("hexdump f")
    expect_in("00000000  6c 31 0a", out, "hexdump bytes")
    expect_in("|l1.|", out, "hexdump text column")
    t.run("echo 0123456789abcdef >> f")
    out = t.run("hexdump f")
    expect_in("00000010", out, "hexdump second row")


@test("cp leaves an exact copy, and the copy stays put when the source changes",
      needs=("cp", "diff", "cat", "echo", "mkdir"))
def cp_copy(t):
    t.workspace(W + "cp")
    write_lines(t, "src", LINES)
    t.run("cp src dst")
    if not status(t, "diff src dst"):
        raise AssertionError("cp did not leave an exact copy")
    t.run("echo l6 >> src")
    if status(t, "diff src dst"):
        raise AssertionError("the copy followed a change to its source")
    if rows(t.run("cat dst")) != LINES:
        raise AssertionError("the copy changed")


@test("mv moves the file whole and leaves nothing behind", needs=("mv", "cp", "diff", "ls", "echo", "mkdir"))
def mv_moves(t):
    t.workspace(W + "mv")
    write_lines(t, "src", LINES)
    t.run("cp src keep")
    t.run("mv src moved")
    if not status(t, "diff keep moved"):
        raise AssertionError("mv changed the file it moved")
    expect_not_in(" src", t.run("ls -la"), "ls -la after mv")
    if status(t, "cat src"):
        raise AssertionError("the source is still there after mv")


@test("dd copies whole blocks, a prefix with count, and the rest with skip",
      needs=("dd", "diff", "cat", "wc", "echo", "mkdir"))
def dd_blocks(t):
    t.workspace(W + "dd")
    write_lines(t, "f", LINES)
    out = t.run("dd if=f of=whole")
    expect_in("15 bytes copied", out, "dd of the whole file")
    if not status(t, "diff f whole"):
        raise AssertionError("dd did not copy the file exactly")
    out = t.run("dd if=f of=prefix bs=3 count=2")
    expect_in("2+0 records in", out, "dd count")
    if rows(t.run("cat prefix")) != LINES[:2]:
        raise AssertionError("dd bs=3 count=2 did not copy the first two lines")
    t.run("dd if=f of=rest bs=3 skip=3")
    if rows(t.run("cat rest")) != LINES[3:]:
        raise AssertionError("dd bs=3 skip=3 did not copy the last two lines")


@test("true and false drive && and ||, and test agrees with what is on disk",
      needs=("true", "false", "test", "echo", "mkdir"))
def true_false_test(t):
    t.workspace(W + "tf")
    if not status(t, "true") or status(t, "false"):
        raise AssertionError("true or false gave the wrong status")
    expect_in("ran", t.run("false || echo ran"), "false ||")
    expect_not_in("ran", t.run("false && echo ran"), "false &&")
    t.run("echo x > f")
    t.run("mkdir d")
    if not status(t, "test -f f") or status(t, "test -d f"):
        raise AssertionError("test -f / -d wrong on a file")
    if not status(t, "test -d d") or status(t, "test -f d"):
        raise AssertionError("test -f / -d wrong on a directory")
    if status(t, "test -e nowhere"):
        raise AssertionError("test -e found a path that is not there")


@test("uname, hostname, id, whoami and groups describe the same machine and user",
      needs=("uname", "hostname", "id", "whoami", "groups"))
def identity_agrees(t):
    host = rows(t.run("hostname"))[-1]
    full = t.run("uname -a")
    expect_in(host, full, "uname -a carries the hostname")
    expect_in(rows(t.run("uname -s"))[-1], full, "uname -a carries the kernel name")
    me = rows(t.run("whoami"))[-1]
    ident = rows(t.run("id"))[-1]
    expect_in("(%s)" % me, ident, "id names the user whoami gives")
    gid = ident.split("gid=")[-1].split()[0].split("(")[0]
    expect_in(gid, t.run("groups"), "groups carries the gid id gives")


@test("read splits a file's first line into variables and $(...) carries a count",
      needs=("read", "echo", "wc", "cat", "mkdir"))
def read_and_capture(t):
    t.workspace(W + "read")
    t.run("echo first second third > f")
    try:
        t.run("read A B < f")
        expect_in("first", t.run("echo $A"), "read A")
        expect_in("second third", t.run("echo $B"), "read B takes the rest")
        expect_in("1 3 19", t.run("echo $(cat f | wc)"), "a count captured with $(...)")
    finally:
        t.run("unset A")
        t.run("unset B")


@test("touch, chmod, ls and stat agree on a file's mode, and rm makes every one of them lose it",
      needs=("touch", "chmod", "ls", "stat", "rm", "find", "mkdir"))
def mode_and_removal(t):
    t.workspace(W + "mode")
    t.run("touch f")
    t.run("chmod 640 f")
    expect_in("-rw-r-----", t.run("ls -la f"), "ls -la after chmod 640")
    expect_in("(0640/-rw-r-----)", t.run("stat f"), "stat after chmod 640")
    t.run("rm f")
    if status(t, "stat f"):
        raise AssertionError("stat found a removed file")
    if rows(t.run("find . -name f")):
        raise AssertionError("find found a removed file")
    expect_not_in(" f", t.run("ls -la"), "ls -la after rm")


@test("mkdir and rm of a directory show in find and du", needs=("mkdir", "rm", "find", "du", "echo"))
def dirs_in_find_and_du(t):
    t.workspace(W + "dirs")
    t.run("mkdir d")
    t.run("echo abc > d/x")
    if "./d" not in rows(t.run("find . -type d")):
        raise AssertionError("find -type d did not show a new directory")
    expect_in("./d", t.run("du"), "du lists a new directory")
    t.run("rm d/x")
    t.run("rm d")
    if "./d" in rows(t.run("find .")):
        raise AssertionError("find still shows a removed directory")


@test("an exported variable shows in env and unset takes it away", needs=("export", "env", "unset", "echo"))
def env_round_trip(t):
    try:
        t.run("export CXVAR=crossvalue")
        expect_in("CXVAR=crossvalue", t.run("env"), "env after export")
        expect_in("crossvalue", t.run("echo $CXVAR"), "expansion after export")
    finally:
        t.run("unset CXVAR")
    expect_not_in("CXVAR=", t.run("env"), "env after unset")


@test("a directory listing shows the filesystems mounted in it, and find crosses into them",
      needs=("ls", "find", "mount"), mounts=("/proc",))
def mounts_listed_and_crossed(t):
    root = t.run("ls -la /")
    for line in rows(t.run("mount")):
        point = line.split()[0] if line.split() else ""
        if point.count("/") == 1 and len(point) > 1:
            expect_in(" " + point[1:], root, "ls / shows the mount point %s" % point)
    expect_in("/proc/meminfo", t.run("find / -name meminfo"), "find from / into /proc")
    if "/proc" not in rows(t.run("find / -type d -name proc")):
        raise AssertionError("find / -type d did not list the /proc mount point")
    if 1 != rows(t.run("ls -la /")).count([l for l in rows(t.run("ls -la /")) if l.endswith(" proc")][0]):
        raise AssertionError("ls / listed /proc more than once")
