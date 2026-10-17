#!/usr/bin/env python3

"""
The file commands, over whatever transport the target is reached by.

These mirror tests/host/system/test_commands_fs.cpp. The difference is what
they are allowed to believe: the C++ versions check the filesystem directly
through __i_fs, and here there is only the shell, so every result is read back
with a command. That is the point — on real flash it is the command output an
operator sees that has to be right.
"""

from .registry import test, expect_in, expect_not_in, Skip

# a real board's rootfs is flash the user cares about, so everything happens in
# one directory per test and is removed again by the runner
W = "wt_"


def multiline(t):
    """
    A file with more than one line in it, and the lines themselves.

    Built here rather than borrowed from the user store, which only has a
    second line on a target that happens to have a second account — head and
    tail then went untested on every single account board. `echo >>` appends,
    so the file is made to order and the assertions are the same everywhere.
    """
    path = t.workspace(W + "multi") + "/lines.txt"
    wanted = ["first line", "second line", "third line"]

    t.run("echo %s > %s" % (wanted[0], path))
    for line in wanted[1:]:
        t.run("echo %s >> %s" % (line, path))

    body = t.run("cat %s" % path)
    lines = [line.strip() for line in body.splitlines() if line.strip() in wanted]

    if len(lines) < 2:
        raise Skip("this target could not be given a multi-line file")

    return path, lines


@test("ls lists what was created", needs=("ls", "touch", "mkdir", "rm"))
def ls_lists(t):
    t.workspace(W + "ls")
    t.run("touch one.txt")
    t.run("touch two.txt")

    out = t.run("ls")
    expect_in("one.txt", out, "ls")
    expect_in("two.txt", out, "ls")


@test("ls takes a directory argument", needs=("ls", "touch", "cd"))
def ls_argument(t):
    path = t.workspace(W + "lsarg")
    t.run("touch listed.txt")
    t.run("cd /")

    expect_in("listed.txt", t.run("ls %s" % path), "ls with a path")


@test("ls shows the dot entries and the mode", needs=("ls", "touch"))
def ls_mode(t):
    t.workspace(W + "lsmode")
    t.run("touch moded.txt")

    out = t.run("ls")
    expect_in("..", out, "parent entry")
    expect_in("-rw-", out, "mode column")


@test("mkdir creates a directory", needs=("mkdir", "ls"))
def mkdir_creates(t):
    t.workspace(W + "mkdir")
    t.run("mkdir sub")

    expect_in("sub", t.run("ls"), "ls after mkdir")
    expect_in("d", t.run("ls"), "directory mode")


@test("touch creates an empty file", needs=("touch", "wc"))
def touch_creates(t):
    t.workspace(W + "touch")
    t.run("touch fresh.txt")

    expect_in("fresh.txt", t.run("ls"), "ls after touch")
    expect_in("0", t.run("wc fresh.txt"), "size of an empty file")


@test("a command needing an argument reports one missing", needs=("touch", "mkdir"))
def missing_argument(t):
    t.workspace(W + "noarg")

    # the parser used to read past the end of the line here
    expect_in("CmdErr", t.run("touch"), "touch with no argument")
    expect_in("CmdErr", t.run("mkdir"), "mkdir with no argument")


@test("a command that needs no argument still runs", needs=("pwd", "ls"))
def no_argument_needed(t):
    expect_in("/", t.run("pwd"), "pwd")
    expect_in(".", t.run("ls"), "ls")


@test("echo prints its argument", needs=("echo",))
def echo_prints(t):
    expect_in("hello there", t.run("echo hello there"), "echo")


@test("echo writes to a file when redirected", needs=("echo", "cat"))
def echo_redirects(t):
    t.workspace(W + "echo")
    t.run("echo written by echo > note.txt")

    expect_in("note.txt", t.run("ls"), "ls after redirect")
    expect_in("written by echo", t.run("cat note.txt"), "cat")


@test("cat prints the contents", needs=("echo", "cat"))
def cat_prints(t):
    t.workspace(W + "cat")
    t.run("echo first line > c.txt")

    expect_in("first line", t.run("cat c.txt"), "cat")


@test("cat of a missing file prints no content", needs=("cat",))
def cat_missing(t):
    t.workspace(W + "catmiss")

    expect_not_in("absent content", t.run("cat absent.txt"), "cat of a missing file")


@test("head prints from the top", needs=("cat", "head", "echo", "mkdir", "rm"))
def head_top(t):
    path, lines = multiline(t)

    out = t.run("head %s 1" % path)
    expect_in(lines[0], out, "head printed the first line")
    expect_not_in(lines[-1], out, "head stopped before the last line")


@test("tail prints from the bottom", needs=("cat", "tail", "echo", "mkdir", "rm"))
def tail_bottom(t):
    path, lines = multiline(t)

    out = t.run("tail %s 1" % path)
    expect_in(lines[-1], out, "tail printed the last line")
    expect_not_in(lines[0], out, "tail stopped after the first line")


@test("head and tail agree with the whole file", needs=("cat", "head", "tail", "wc", "echo", "mkdir", "rm"))
def head_tail_span(t):
    path, lines = multiline(t)

    everything = t.run("head %s %d" % (path, len(lines)))
    for line in lines:
        expect_in(line, everything, "head over the whole file")

    expect_in(str(len(lines)), t.run("wc %s" % path), "wc line count")


@test("wc counts lines words and bytes", needs=("echo", "wc"))
def wc_counts(t):
    t.workspace(W + "wc")
    t.run("echo one two > counted.txt")

    # a redirect terminates the line it writes, so the file is one whole line
    # and the byte count includes the terminator
    out = t.run("wc counted.txt")
    expect_in("1", out, "line count")
    expect_in("2", out, "word count")
    expect_in("8", out, "byte count")


@test("grep finds a match and reports where", needs=("echo", "grep"))
def grep_finds(t):
    t.workspace(W + "grep")
    t.run("echo needle here > hay.txt")

    out = t.run("grep needle hay.txt")
    expect_in("needle", out, "grep")
    expect_in("hay.txt", out, "grep names the file")


@test("grep reports nothing when there is no match", needs=("echo", "grep"))
def grep_no_match(t):
    t.workspace(W + "grepno")
    t.run("echo nothing here > hay.txt")

    expect_not_in("needle here", t.run("grep needle hay.txt"), "grep with no match")


@test("hexdump shows offsets and ascii", needs=("echo", "hexdump"))
def hexdump_shows(t):
    t.workspace(W + "hex")
    t.run("echo AB > bin.txt")

    out = t.run("hexdump bin.txt")
    expect_in("41", out, "hex for A")
    expect_in("42", out, "hex for B")
    expect_in("AB", out, "ascii column")


@test("cp leaves both copies", needs=("echo", "cp", "cat"))
def cp_both(t):
    t.workspace(W + "cp")
    t.run("echo copy me > src.txt")
    t.run("cp src.txt dst.txt")

    listing = t.run("ls")
    expect_in("src.txt", listing, "source still there")
    expect_in("dst.txt", listing, "copy made")
    expect_in("copy me", t.run("cat dst.txt"), "contents copied")


@test("mv renames within a directory", needs=("echo", "mv", "cat"))
def mv_renames(t):
    t.workspace(W + "mv")
    t.run("echo move me > before.txt")
    t.run("mv before.txt after.txt")

    listing = t.run("ls")
    expect_not_in("before.txt", listing, "old name gone")
    expect_in("after.txt", listing, "new name there")
    expect_in("move me", t.run("cat after.txt"), "contents moved")


@test("rm removes a file", needs=("touch", "rm"))
def rm_file(t):
    t.workspace(W + "rm")
    t.run("touch doomed.txt")
    expect_in("doomed.txt", t.run("ls"), "before rm")

    t.run("rm doomed.txt")
    expect_not_in("doomed.txt", t.run("ls"), "after rm")


@test("rm removes a directory", needs=("mkdir", "rm"))
def rm_directory(t):
    t.workspace(W + "rmdir")
    t.run("mkdir gone")
    expect_in("gone", t.run("ls"), "before rm")

    # rm here takes a directory without a flag, deliberately
    t.run("rm gone")
    expect_not_in("gone", t.run("ls"), "after rm")


@test("a file written by one command is read by another", needs=("echo", "cp", "mv", "cat", "wc"))
def command_chain(t):
    t.workspace(W + "chain")
    t.run("echo chained > a.txt")
    t.run("cp a.txt b.txt")
    t.run("mv b.txt c.txt")

    expect_in("chained", t.run("cat c.txt"), "contents through the chain")
    expect_in("1", t.run("wc c.txt"), "one line")


@test("a redirect replaces what was there before", needs=("echo", "cat", "wc"))
def redirect_truncates(t):
    t.workspace(W + "trunc")
    t.run("echo the original contents > note.txt")
    expect_in("the original contents", t.run("cat note.txt"), "first write")

    t.run("echo shorter > note.txt")

    out = t.run("cat note.txt")
    expect_in("shorter", out, "second write")
    expect_not_in("original", out, "the old contents are gone")
    expect_in("8", t.run("wc note.txt"), "byte count after truncation")


@test("a long line survives a write and read back", needs=("echo", "cat", "wc"))
def long_line_round_trip(t):
    t.workspace(W + "round")
    text = "abcdefghij" * 6

    t.run("echo %s > long.txt" % text)

    expect_in(text, t.run("cat long.txt"), "readback of a 60 byte line")
    expect_in("61", t.run("wc long.txt"), "byte count including the terminator")


@test("a value written through sysfs keeps its value", needs=("echo", "cat"), mounts=("/sys",))
def sysfs_value_write(t):
    # a redirect terminates its line, and a file that is really a value has to
    # read that as the value rather than as the terminator
    t.run("echo 1 > /sys/class/gpio/2/mode")
    expect_in("1", t.run("cat /sys/class/gpio/2/mode"), "gpio mode after a redirect")

    t.run("echo 0 > /sys/class/gpio/2/mode")
    expect_in("0", t.run("cat /sys/class/gpio/2/mode"), "gpio mode set back")


@test("fedit writes a new file the shell can read back",
      needs=("fedit", "cat", "rm"), mounts=("/",), slow=True)
def fedit_writes_a_file(t):
    """
    The editor is the only way to put a multi-line file on the target, and a new
    file has no original to replace, which is the save path that used to fail.
    Type a line and commit it with ENTER, open the menu with Ctrl+C (a single
    byte, unlike ESC which a transport cannot tell from an arrow key), then !w
    and ENTER to save. The menu acts on the token when ENTER is pressed.
    """
    from ..driver.shell import PROMPT

    path = "/wt_fedit.txt"
    body = "edited by fedit"
    t.run("rm %s" % path)
    t.run("rm %s.tmp" % path)

    try:
        t.shell.send_line("fedit %s" % path)
        t.shell.expect("ESC", t.timeout)
        t.shell.drain(0.5)

        t.shell.send_raw(body)
        t.shell.drain(0.5)
        t.shell.send_raw("\r")
        t.shell.drain(0.5)

        t.shell.send_raw("\x03")
        t.shell.expect("save", t.timeout)
        t.shell.send_line("!w")
        t.shell.expect("saved", t.timeout)
        t.shell.expect(PROMPT, t.timeout)

        expect_in(body, t.run("cat %s" % path), "fedit saved the typed text")
    finally:
        t.resync()
        t.run("rm %s" % path)
        t.run("rm %s.tmp" % path)


@test("fedit cancels an edit without changing the file",
      needs=("fedit", "echo", "cat", "rm"), mounts=("/",), slow=True)
def fedit_cancel_keeps_original(t):
    from ..driver.shell import PROMPT

    path = "/wt_fecancel.txt"
    t.run("rm %s" % path)
    t.run("rm %s.tmp" % path)
    t.run("echo keepthis > %s" % path)

    try:
        t.shell.send_line("fedit %s" % path)
        t.shell.expect("ESC", t.timeout)
        t.shell.drain(0.5)
        t.shell.send_raw("JUNKEDIT")
        t.shell.drain(0.5)
        t.shell.send_raw("\x03")
        t.shell.expect("cancel", t.timeout)
        t.shell.send_line("!c")
        t.shell.expect("cancelled", t.timeout)
        t.shell.expect(PROMPT, t.timeout)

        out = t.run("cat %s" % path)
        expect_in("keepthis", out, "cancel kept the original content")
        expect_not_in("JUNKEDIT", out, "cancel discarded the edit")
    finally:
        t.resync()
        t.run("rm %s" % path)
        t.run("rm %s.tmp" % path)


@test("fedit keeps the permissions of the file it saves",
      needs=("fedit", "echo", "chmod", "ls", "rm"), mounts=("/",), slow=True)
def fedit_save_keeps_permissions(t):
    """
    The editor rebuilds the file through a scratch copy and renames it into
    place, which is the move that loses metadata. A config holding a secret is
    0600, and an edit must not be what opens it up: /etc/shadow and the ssh host
    keys go through this same path.
    """
    from ..driver.shell import PROMPT

    path = "feperm.txt"
    t.workspace(W + "feperm")
    t.run("echo secret > %s" % path)
    t.run("chmod 600 %s" % path)

    expect_in("-rw-------", t.run("ls"), "the file starts unreadable to others")

    t.shell.send_line("fedit %s" % path)
    t.shell.expect("ESC", t.timeout)
    t.shell.drain(0.5)
    t.shell.send_raw("X")
    t.shell.drain(0.5)
    t.shell.send_raw("\x03")
    t.shell.expect("save", t.timeout)
    t.shell.send_line("!w")
    t.shell.expect("saved", t.timeout)
    t.shell.expect(PROMPT, t.timeout)

    expect_in("-rw-------", t.run("ls"),
              "the saved file kept its permissions")


@test("cp copies a file across mounts",
      needs=("cp", "echo", "cat", "rm"), mounts=("/", "/tmp"))
def cp_across_mounts(t):
    src = "/wt_xm.txt"
    dst = "/tmp/wt_xm.txt"
    t.run("rm %s" % dst)
    t.run("echo crossmount > %s" % src)

    try:
        t.run("cp %s %s" % (src, dst))
        expect_in("crossmount", t.run("cat %s" % dst),
                  "the copy across mounts carries the content")
    finally:
        t.run("rm %s" % src)
        t.run("rm %s" % dst)
