#!/usr/bin/env python3

"""
The contract every command keeps, row by row from the command scan: an option it
does not define and an operand past the ones it takes are named and refused with
nothing done; a number is read whole and range-checked, never wrapped or
defaulted; a failure names what failed and why in words; a given empty name is
shown as ''. Each refusal is read back the way the user sees it, and the state is
checked afterwards to prove nothing happened.
"""

from .registry import test, expect_in, expect_not_in, Skip

W = "cli_"
OK = "RC_OK"
NO = "RC_NO"
LONG = "w" * 300
HUGE = "99999999999999999999"


def status(t, command):
    """Whether a command succeeded, read through && and || rather than a raw code."""
    out = t.run("%s && echo %s || echo %s" % (command, OK, NO))
    if OK in out:
        return True
    if NO in out:
        return False
    raise AssertionError("no status came back from %r:\n%s" % (command, out))


def refused(t, command, said, usage=True):
    """The command is refused, names what it refused and, when asked, shows its usage line."""
    out = t.run(command)
    expect_in(said, out, command)
    if usage:
        expect_in("usage:", out, "%s shows its usage" % command)
    if status(t, command):
        raise AssertionError("%r was refused but reported success" % command)
    return out


def names(t, path="."):
    """The names a directory lists."""
    found = set()
    for line in t.run("ls -la %s" % path).splitlines():
        cols = line.split()
        if len(cols) >= 6:
            found.add(cols[-1])
    return found


def is_host(t):
    return "esp" not in t.run("uname -m").lower()


# ---- the argument contract --------------------------------------------------

@test("an option a command does not define is named, with the usage, and nothing is done",
      needs=("ls", "mkdir", "touch", "rm", "cat", "pwd", "wc"))
def unknown_option(t):
    t.workspace(W + "opt")
    t.run("echo keep > f")
    for command, word in (("ls -Z", "-Z"), ("ls -lZ", "-lZ"), ("ls --help", "--help"),
                          ("mkdir -Z", "-Z"), ("touch -Z", "-Z"), ("rm -Z f", "-Z"),
                          ("cat -Z", "-Z"), ("wc -Z f", "-Z"), ("pwd -Z", "-Z")):
        refused(t, command, "invalid option : %s" % word)
    left = names(t)
    if "-Z" in left or "f" not in left:
        raise AssertionError("a refused option still changed the directory: %r" % sorted(left))


@test("an operand past the ones a command takes is named, with the usage, and nothing is done",
      needs=("ls", "mkdir", "touch", "cd", "pwd", "cat", "cp", "mv"))
def extra_operand(t):
    t.workspace(W + "extra")
    t.run("echo keep > f")
    for command, word in (("pwd now", "now"), ("ls a b", "b"), ("mkdir a b", "b"), ("touch a b", "b"),
                          ("cd a b", "b"), ("cat f g", "g"), ("cp f g h", "h"), ("mv f g h", "h")):
        refused(t, command, "extra operand : %s" % word)
    left = names(t)
    for made in ("a", "b", "g", "h"):
        if made in left:
            raise AssertionError("a refused line still made %s: %r" % (made, sorted(left)))
    expect_in("keep", t.run("cat f"), "the file is untouched")


@test("a quoted word is one operand and a 300 character word is named whole",
      needs=("mkdir", "ls", "rm", "pwd"))
def quoted_and_long(t):
    t.workspace(W + "quote")
    t.run('mkdir "a b"')
    if "b" not in t.run('ls "a b"') and not status(t, 'ls "a b"'):
        raise AssertionError("a quoted name with a space was not taken as one operand")
    if not status(t, 'rm "a b"'):
        raise AssertionError("rm of a quoted name with a space failed")
    refused(t, "pwd " + LONG, "extra operand : " + LONG)


@test("commands that take no words refuse one", needs=("help", "whoami", "id", "uptime", "true", "false"))
def no_words(t):
    for command in ("whoami x", "id x y", "uptime x", "groups x x", "cls x", "who x"):
        name = command.split()[0]
        if name not in t.run("help"):
            continue
        out = t.run(command)
        expect_in("extra operand : x", out, command)


@test("an unknown sub-command is named with the usage", needs=("service", "db"))
def unknown_subcommand(t):
    for command in ("service zzz", "db zzz", "udevadm zzz", "net zzz"):
        if command.split()[0] not in t.run("help"):
            continue
        refused(t, command, "unknown sub-command : zzz")


@test("a name=value command refuses a bare word and a name it does not have", needs=("top",))
def name_value(t):
    refused(t, "top junk", "invalid option : junk")
    refused(t, "top z=1", "invalid option : z")


# ---- files -------------------------------------------------------------------

@test("ls lists names only, -a adds the hidden ones, -l long rows and -h short sizes",
      needs=("ls", "mkdir", "echo", "wc"))
def ls_forms(t):
    t.workspace(W + "ls")
    t.run("echo x > alpha.txt")
    t.run("echo x > .hidden")
    t.run("mkdir sub")
    for _ in range(4):
        t.run("echo %s >> big" % LONG)
    plain = t.run("ls")
    for name in ("alpha.txt", "sub", "big"):
        expect_in(name, plain, "ls")
    for absent in (".hidden", "rw-", "pdiStack", ".."):
        expect_not_in(absent, plain, "plain ls shows names only")
    every = t.run("ls -a")
    for name in (".hidden", "alpha.txt", "..", "sub"):
        expect_in(name, every, "ls -a")
    long_rows = t.run("ls -l")
    expect_in("-rw-", long_rows, "ls -l shows the mode")
    expect_in("drwx", long_rows, "ls -l shows a directory")
    expect_not_in(".hidden", long_rows, "ls -l leaves out hidden names")
    expect_in("1204", long_rows, "ls -l shows the size in bytes")
    expect_in(".hidden", t.run("ls -la"), "ls -la")
    human = t.run("ls -lah")
    expect_in("1.2K", human, "ls -lah shows 1204 bytes as 1.2K")
    expect_not_in("1204", human, "ls -lah does not show the bytes")
    expect_in(" 2 ", human, "a size under 1K is shown as bytes")
    lines = [l for l in t.run("ls | wc").split() if l.isdigit()]
    if not lines or int(lines[0]) < 3:
        raise AssertionError("ls into a pipe did not give one name per line: %r" % lines)
    expect_in("alpha.txt", t.run("ls alpha.txt"), "ls of one file")
    expect_in("-rw-", t.run("ls -l alpha.txt"), "ls -l of one file")
    expect_not_in("rw-", t.run("ls alpha.txt"), "ls of one file shows its name only")

@test("rm removes a file and an empty directory, refuses a full one, and rm -r removes it",
      needs=("rm", "mkdir", "ls", "echo"))
def rm_directory(t):
    t.workspace(W + "rm")
    t.run("echo a > f")
    t.run("mkdir empty")
    t.run("mkdir full")
    t.run("mkdir full/sub")
    t.run("echo deep > full/sub/g")
    if not status(t, "rm f") or not status(t, "rm empty"):
        raise AssertionError("rm of a file or an empty directory failed")
    out = t.run("rm full")
    expect_in("Directory not empty", out, "rm of a full directory")
    expect_in("deep", t.run("cat full/sub/g"), "a refused rm leaves everything inside")
    if not status(t, "rm -r full"):
        raise AssertionError("rm -r of a full directory failed")
    left = names(t)
    if left & {"f", "empty", "full"}:
        raise AssertionError("rm left %r" % sorted(left))
    expect_in("No such file", t.run("rm gone"), "rm of a missing name says so")
    refused(t, "rm", "usage:", usage=False)


@test("cp and mv name what they cannot do and copy or move into a directory",
      needs=("cp", "mv", "mkdir", "cat", "echo"))
def cp_mv(t):
    t.workspace(W + "cpmv")
    t.run("echo one > a")
    t.run("echo two > b")
    t.run("mkdir d")
    expect_in("are the same file", t.run("cp a a"), "cp onto itself")
    expect_in("one", t.run("cat a"), "cp onto itself keeps the file")
    expect_in("File exists", t.run("cp a b"), "cp over a file")
    expect_in("two", t.run("cat b"), "cp over a file keeps it")
    expect_in("Not found", t.run("cp nothere x"), "cp of a missing file")
    expect_in("Is a directory", t.run("cp d x"), "cp of a directory")
    if not status(t, "cp a d"):
        raise AssertionError("cp into a directory failed")
    expect_in("one", t.run("cat d/a"), "cp into a directory keeps the name")
    expect_in("are the same file", t.run("mv b b"), "mv onto itself")
    expect_in("Not found", t.run("mv nothere x"), "mv of a missing file")
    if not status(t, "mv b d"):
        raise AssertionError("mv into a directory failed")
    expect_in("two", t.run("cat d/b"), "mv into a directory keeps the name")
    expect_in("File exists", t.run("mv a d/b"), "mv over a file")
    expect_in("two", t.run("cat d/b"), "a refused mv keeps the file it would replace")
    expect_in("one", t.run("cat a"), "a refused mv keeps its source")
    t.run("mkdir e")
    t.run("echo three > e/a")
    expect_in("File exists", t.run("mv a e"), "mv into a directory holding the name")
    expect_in("three", t.run("cat e/a"), "a refused mv into a directory keeps what is there")


@test("cd names why it cannot change directory", needs=("cd", "pwd", "mkdir", "echo"))
def cd_reason(t):
    path = t.workspace(W + "cd")
    t.run("echo a > f")
    expect_in("Not a directory", t.run("cd f"), "cd into a file")
    expect_in("Not found", t.run("cd nothere"), "cd into a missing directory")
    expect_in(path, t.run("pwd"), "a refused cd stays where it was")


@test("mkdir and touch refuse a name past the limit the same way", needs=("mkdir", "touch", "ls"))
def name_limit(t):
    t.workspace(W + "name")
    long_name = "n" * 60
    expect_in("File name too long", t.run("mkdir " + long_name), "mkdir of a long name")
    expect_in("File name too long", t.run("touch " + long_name), "touch of a long name")
    if long_name in t.run("ls"):
        raise AssertionError("a long name was made after all")


@test("a given empty name is shown as '' in each message", needs=("du", "find", "sort", "stat"))
def empty_name(t):
    for command in ('du ""', 'find ""', 'sort ""', 'stat ""'):
        out = t.run(command)
        expect_in("''", out, command)


@test("head and tail read their count strictly", needs=("head", "tail", "echo"))
def head_tail_counts(t):
    t.workspace(W + "count")
    for i in range(1, 4):
        t.run("echo line%d >> f" % i)
    for command in ("head f", "tail f"):
        for count in ("-1", "x", "1x", "4294967296", HUGE):
            out = t.run("%s %s" % (command, count))
            if "invalid count : %s" % count not in out and "invalid option : %s" % count not in out:
                raise AssertionError("%s %s was not refused:\n%s" % (command, count, out))
            expect_not_in("line1", out, "%s %s printed a line" % (command, count))
            expect_not_in("line3", out, "%s %s printed a line" % (command, count))
        out = t.run("%s f 0" % command)
        expect_not_in("line1", out, "%s with 0 prints nothing" % command)
        expect_not_in("line3", out, "%s with 0 prints nothing" % command)
    expect_in("line1", t.run("head f 1"), "head 1")
    expect_not_in("line2", t.run("head f 1"), "head 1 stops at one")
    expect_in("line3", t.run("tail f 1"), "tail 1")
    expect_not_in("line2", t.run("tail f 1"), "tail 1 stops at one")
    for big in ("4", "2147483648", "4294967295"):
        out = t.run("head f %s" % big)
        if not all(("line%d" % i) in out for i in range(1, 4)):
            raise AssertionError("head f %s did not print the whole file:\n%s" % (big, out))
        out = t.run("tail f %s" % big)
        if not all(("line%d" % i) in out for i in range(1, 4)):
            raise AssertionError("tail f %s did not print the whole file:\n%s" % (big, out))
    t.run("mkdir d")
    expect_in("Is a directory", t.run("head d"), "head of a directory")
    expect_in("Is a directory", t.run("tail d"), "tail of a directory")


@test("grep matches every line with an empty pattern and refuses one it cannot read",
      needs=("grep", "echo"))
def grep_patterns(t):
    t.workspace(W + "grep")
    t.run("echo alpha > f")
    t.run("echo beta >> f")
    out = t.run('grep "" f')
    expect_in("alpha", out, "an empty pattern")
    expect_in("beta", out, "an empty pattern")
    refused(t, "grep '[' f", "invalid pattern : [")
    refused(t, "grep 'a\\' f", "invalid pattern : a\\")
    if not status(t, "grep '\\[' f") and "alpha" in t.run("grep '\\[' f"):
        raise AssertionError("an escaped bracket matched a line without one")
    refused(t, "grep a f g", "extra operand : g")


@test("tee and a redirect name why a file cannot be written", needs=("tee", "echo", "mkdir"))
def tee_reason(t):
    t.workspace(W + "tee")
    t.run("mkdir d")
    out = t.run("echo x | tee d")
    expect_in("tee: d : Is a directory", out, "tee to a directory")
    expect_in("x", out, "tee still copies its input to the output")
    expect_in("File name too long", t.run("echo x | tee " + "t" * 40), "tee to a long name")
    expect_in("Is a directory", t.run("echo x > d"), "a redirect to a directory")


@test("stat and ls show a device node as a device", needs=("stat", "ls"), mounts=("/dev",))
def device_type(t):
    expect_in("character special file", t.run("stat /dev/null"), "stat /dev/null")
    expect_in("crw-", t.run("ls -l /dev"), "ls shows /dev nodes as character devices")


@test("blkid names a node that is not there", needs=("blkid",))
def blkid_missing(t):
    expect_in("blkid: /dev/nothere : Not found", t.run("blkid /dev/nothere"), "blkid of a missing node")
    if status(t, "blkid /dev/nothere"):
        raise AssertionError("blkid of a missing node reported success")


@test("source names the command it could not find, not itself", needs=("source", "echo"))
def source_names(t):
    t.workspace(W + "src")
    t.run("echo nosuchcmd > s.sh")
    out = t.run("source s.sh")
    expect_in("nosuchcmd: command not found", out, "source")
    expect_not_in("source: command not found", out, "source")


# ---- numbers and permissions ---------------------------------------------------

@test("chmod takes an octal mode up to 0777 and refuses anything else, leaving the mode",
      needs=("chmod", "stat", "echo"))
def chmod_modes(t):
    t.workspace(W + "chmod")
    t.run("echo x > f")
    t.run("chmod 640 f")
    expect_in("0640", t.run("stat f"), "chmod 640")
    for mode in ("8", "9", "1000", "99999", "x", "-1", "0x1", HUGE):
        out = t.run("chmod %s f" % mode)
        if "invalid mode : %s" % mode not in out and "invalid option : %s" % mode not in out:
            raise AssertionError("chmod %s was not refused:\n%s" % (mode, out))
        expect_in("0640", t.run("stat f"), "a refused chmod %s left the mode" % mode)
    for mode, shown in (("0", "0000"), ("777", "0777"), ("0644", "0644")):
        t.run("chmod %s f" % mode)
        expect_in(shown, t.run("stat f"), "chmod %s" % mode)


@test("chown refuses a user past 65535 or not a number, never falling back to root",
      needs=("chown", "ls", "echo"), su=True)
def chown_users(t):
    t.become_root()
    t.workspace(W + "chown")
    t.run("echo x > f")
    t.run("chown 7:7 f")
    before = t.run("ls -l f")
    for user in ("65536", "70000:3", "a:b", "a", "-1", "1:65536", HUGE):
        out = t.run("chown %s f" % user)
        if "invalid user : %s" % user not in out and "invalid option : %s" % user not in out:
            raise AssertionError("chown %s was not refused:\n%s" % (user, out))
        if t.run("ls -l f") != before:
            raise AssertionError("a refused chown %s changed the owner:\n%s" % (user, t.run("ls -l f")))


@test("umask takes an octal mask up to 0777 and refuses anything else, keeping the mask",
      needs=("umask",))
def umask_values(t):
    start = t.run("umask").strip().splitlines()[-1].strip()
    try:
        for mask in ("8", "1000", "x", HUGE):
            out = t.run("umask %s" % mask)
            expect_in("invalid mode : %s" % mask, out, "umask %s" % mask)
            expect_in(start, t.run("umask"), "a refused umask %s kept the mask" % mask)
        expect_in("0077", t.run("umask 077"), "umask 077")
    finally:
        t.run("umask %s" % start)


@test("test compares numbers at the 64 bit limits and refuses what is not one",
      needs=("test",))
def test_numbers(t):
    for expr, want in (("9223372036854775807 -gt 9223372036854775806", True),
                       ("-9223372036854775808 -lt 0", True),
                       ("4294967296 -gt 4294967295", True),
                       ("2147483648 -eq 2147483648", True),
                       ("1 -gt 2", False)):
        if status(t, "test %s" % expr) != want:
            raise AssertionError("test %s gave the wrong answer" % expr)
    for word in (HUGE, "1x", "x"):
        expect_in("integer expression expected : %s" % word, t.run("test %s -gt 1" % word), "test %s" % word)


@test("ps, kill, pkill, renice and top read their numbers strictly", needs=("ps", "kill", "renice", "top"))
def process_numbers(t):
    refused(t, "ps 256", "invalid sid : 256")
    refused(t, "ps x", "invalid sid : x")
    expect_not_in("top -", t.run("ps"), "ps prints only the table")
    for pid in ("x", "32768", "4294967296"):
        refused(t, "kill %s" % pid, "invalid pid : %s" % pid)
    refused(t, "kill -1", "invalid option : -1")
    refused(t, "kill 3 1", "unsupported signal : 3")
    refused(t, "kill 99 1", "unsupported signal : 99")
    expect_in("no such pid : 32767", t.run("kill 32767"), "kill of a pid that is not there")
    refused(t, "pkill 3 nosuchtask", "unsupported signal : 3")
    for nice in ("20", "-21", "100", "x"):
        refused(t, "renice %s 1" % nice, "invalid nice : %s" % nice)
    refused(t, "top n=0", "invalid count : 0")
    refused(t, "top i=1", "invalid interval : 1")
    refused(t, "top u=x", "invalid sid : x")


# ---- sessions and the device -----------------------------------------------------

@test("an unknown option to a session command is refused before any prompt",
      needs=("logout", "useradd", "userdel", "whoami"))
def session_options(t):
    who = t.run("whoami").strip().splitlines()[-1]
    for command in ("logout -Z", "useradd -Z", "userdel -Z", "passwd -Z", "su -Z"):
        if command.split()[0] not in t.run("help"):
            continue
        out = t.run(command)
        expect_in("invalid option : -Z", out, command)
        expect_not_in("assword:", out, "%s asked for a password" % command)
    expect_in(who, t.run("whoami"), "a refused logout kept the session")


@test("reboot refuses any word and does not restart", needs=("reboot", "uptime"))
def reboot_words(t):
    if not is_host(t):
        raise Skip("a board would restart if this failed; host only")
    for command in ("reboot -Z", "reboot now", "reboot --help"):
        out = t.run(command)
        if "invalid option" not in out and "extra operand" not in out:
            raise AssertionError("%s was not refused:\n%s" % (command, out))
    expect_in("up", t.run("uptime"), "the target is still up")


@test("host and ping refuse an option instead of looking it up", needs=("host", "ping"))
def lookup_options(t):
    refused(t, "host -Z", "invalid option : -Z")
    refused(t, "ping -Z", "invalid option : -Z")
