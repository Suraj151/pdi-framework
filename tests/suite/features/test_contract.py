#!/usr/bin/env python3

"""
The contract every command keeps, checked the same way for all of them.

A small linux has the same expectations of its tools as a big one: a command
that needs an argument says so when given none rather than doing something, an
unknown option is turned away, a missing file is an error and not a crash, the
exit status reflects what happened, and output flows through a pipe and a
redirect. These are table driven so a new command is one row, and so the whole
set stays honest as commands change.

The shell reports a command's result as rc=<code>: 0 for success, a negative
pdi_err_t otherwise. A usage line is what a command prints when its arguments
are wrong.
"""

from .registry import test, Skip, expect_in, expect_not_in


def rc(t, command):
    """The exit status the shell records for a command."""
    out = t.run("%s; echo rc=%s" % (command, "$?"))
    for line in out.splitlines():
        line = line.strip()
        if line.startswith("rc="):
            try:
                return int(line[len("rc="):])
            except ValueError:
                return None
    return None


# commands that need at least one argument and should print a usage line with
# none. The value is a word the usage line must contain.
NEEDS_ARG = {
    "cat": "usage: cat",
    "chmod": "usage: chmod",
    "chown": "usage: chown",
    "cp": "usage: cp",
    "grep": "usage: grep",
    "head": "usage: head",
    "hexdump": "usage: hexdump",
    "host": "usage: host",
    "kill": "usage: kill",
    "mkdir": "usage: mkdir",
    "mkfs": "usage: mkfs",
    "mv": "usage: mv",
    "ping": "usage: ping",
    "rm": "usage: rm",
    "tail": "usage: tail",
    "test": "usage: test",
    "touch": "usage: touch",
    "umount": "usage: umount",
    "wc": "usage: wc",
    "wget": "usage: wget",
}

# commands that take a file and should report an error, not crash, on one that
# is not there
FILE_ARG = ("cat", "wc", "head", "tail", "hexdump", "grep")


def contract_needs_arg(name, needle):
    def run(t):
        if name not in t.caps.commands:
            raise Skip("the target has no %s" % name)
        out = t.run(name)
        expect_in(needle, out, "%s with no argument prints its usage" % name)
        code = rc(t, name)
        if code == 0:
            raise AssertionError("%s with no argument reported success (rc=0)" % name)
    return run


for _name, _needle in NEEDS_ARG.items():
    test("%s with no argument prints its usage and fails" % _name, needs=(_name,))(
        contract_needs_arg(_name, _needle))


@test("a missing file is an error for every file command", needs=("echo", "rm"))
def missing_file_is_error(t):
    present = [c for c in FILE_ARG if c in t.caps.commands]
    if not present:
        raise Skip("the target has none of the file commands")
    missing = "/contract_absent_%d" % 999
    t.run("rm %s" % missing)
    for name in present:
        arg = "x %s" % missing if name == "grep" else missing
        code = rc(t, "%s %s" % (name, arg))
        if code == 0:
            raise AssertionError("%s of a missing file reported success" % name)
        expect_in("alive", t.run("echo alive"), "the shell after %s of a missing file" % name)


@test("a successful command reports rc=0", needs=("echo",))
def success_is_zero(t):
    if rc(t, "echo done") != 0:
        raise AssertionError("echo of a word did not report rc=0")


@test("a command that is not a command reports a not-found status", needs=("echo",))
def unknown_command_status(t):
    out = t.run("definitelynotacommand")
    expect_in("not found", out, "an unknown command is reported")
    if rc(t, "definitelynotacommand") == 0:
        raise AssertionError("an unknown command reported success")


@test("output flows through a pipe", needs=("echo", "wc", "rm"))
def pipe_flows(t):
    out = t.run("echo one two three | wc")
    cols = out.split()
    if len(cols) < 3 or cols[1] != "3":
        raise AssertionError("echo of three words piped to wc did not count 3 words:\n%s" % out)


@test("a redirect replaces a file and reads back", needs=("echo", "cat", "rm"))
def redirect_round_trips(t):
    path = t.workspace("contract_redir") + "/f"
    t.run("echo first > %s" % path)
    t.run("echo second > %s" % path)
    out = t.run("cat %s" % path)
    expect_in("second", out, "the redirect's final content")
    expect_not_in("first", out, "a replaced redirect keeps nothing of the old content")


@test("an append adds without discarding", needs=("echo", "cat", "rm"))
def append_adds(t):
    path = t.workspace("contract_append") + "/f"
    t.run("echo line1 > %s" % path)
    t.run("echo line2 >> %s" % path)
    out = t.run("cat %s" % path)
    expect_in("line1", out, "the first line survives an append")
    expect_in("line2", out, "the appended line is there")


@test("a failing command stops an && chain", needs=("echo", "test"))
def and_chain_stops(t):
    out = t.run("test a = b && echo SHOULDNOTPRINT")
    expect_not_in("SHOULDNOTPRINT", out, "the right side of && after a false left side")


@test("a failing command lets an || chain run", needs=("echo", "test"))
def or_chain_runs(t):
    out = t.run("test a = b || echo RANAFTEROR")
    expect_in("RANAFTEROR", out, "the right side of || after a false left side")


@test("quoting keeps a blank as one argument", needs=("echo",))
def quoting_one_arg(t):
    out = t.run('echo "one two"')
    expect_in("one two", out, "a quoted blank stays in one argument")
