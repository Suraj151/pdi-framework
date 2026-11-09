#!/usr/bin/env python3

"""
free, logger, which, type, basename, dirname, seq and history: the small
commands scripts and cron jobs lean on. Each is driven the way an operator
types it, every printed line is read back, and each is checked against the
commands it is used with: free against /proc/meminfo, logger against the log
files, seq through wc and sort, basename and dirname through $(...).
"""

import random
import re
import time

from .registry import test, expect_in, expect_not_in
from ..driver.shell import PROMPT

OK = "RC_OK"
NO = "RC_NO"


def rows(out):
    """The non-blank lines of an answer, trimmed at the end only."""
    return [line.rstrip() for line in out.splitlines() if line.strip()]


def status(t, command):
    """Whether a command succeeded, read through && and || rather than a raw code."""
    out = t.run("%s && echo %s || echo %s" % (command, OK, NO))
    if OK in out:
        return True
    if NO in out:
        return False
    raise AssertionError("no status came back from %r:\n%s" % (command, out))


# ---- free ---------------------------------------------------------------------

def free_table(t, command):
    """The header and the Mem row of a free answer, checked for shape."""
    lines = [line for line in rows(t.run(command)) if line.strip()]
    header = [line for line in lines if "free" in line and "largest" in line]
    mem = [line for line in lines if line.startswith("Mem:")]
    if len(header) != 1 or len(mem) != 1:
        raise AssertionError("%s did not print one header and one Mem row:\n%s" % (command, "\n".join(lines)))
    return header[0], mem[0]


def free_values(t, command):
    header, mem = free_table(t, command)
    values = mem.split()[1:]
    if len(values) != 2:
        raise AssertionError("%s printed %r on its Mem row" % (command, mem))
    return header, mem, values


def meminfo_bytes(t, key):
    for line in rows(t.run("cat /proc/meminfo")):
        if line.startswith(key + ":"):
            return int(line.split()[1])
    raise AssertionError("/proc/meminfo has no %s line" % key)


@test("free prints a header and a Mem row whose columns line up under their names", needs=("free",))
def free_layout(t):
    header, mem = free_table(t, "free")
    for name in ("free", "largest"):
        end = header.index(name) + len(name)
        if len(mem) < end or mem[end - 1] == " " or (end < len(mem) and mem[end] != " "):
            raise AssertionError("the %s value does not end under its name:\n%s\n%s" % (name, header, mem))
    if not mem.split()[1].isdigit() or not mem.split()[2].isdigit():
        raise AssertionError("free printed non-numbers by default: %r" % mem)


@test("free counts in KiB by default, in bytes with -b and agrees with /proc/meminfo",
      needs=("free", "cat"))
def free_units(t):
    _, _, kib = free_values(t, "free")
    _, _, kk = free_values(t, "free -k")
    _, _, b = free_values(t, "free -b")
    meminfo = meminfo_bytes(t, "MemFree")
    for label, values in (("free", kib), ("free -k", kk), ("free -b", b)):
        if not all(v.isdigit() for v in values):
            raise AssertionError("%s printed %r" % (label, values))
    kib = [int(v) for v in kib]
    kk = [int(v) for v in kk]
    b = [int(v) for v in b]
    if b[1] > b[0] or kib[1] > kib[0]:
        raise AssertionError("the largest block is bigger than the free memory: %r %r" % (kib, b))
    if b[0] < 1024:
        raise AssertionError("free -b reports %d bytes free" % b[0])
    # the heap moves between two readings, so the units are compared by size, not equality
    for got, label in ((kib[0], "free"), (kk[0], "free -k")):
        if not (b[0] // 2048 <= got <= b[0] // 512):
            raise AssertionError("%s gave %d KiB against %d bytes from free -b" % (label, got, b[0]))
    if not (meminfo // 2 <= b[0] <= meminfo * 2):
        raise AssertionError("free -b gave %d bytes against MemFree %d" % (b[0], meminfo))


@test("free -h prints short sizes with their unit, and the last unit option given wins", needs=("free",))
def free_human(t):
    _, _, values = free_values(t, "free -h")
    for v in values:
        if not re.match(r"^\d+(\.\d)?[BKMG]$", v):
            raise AssertionError("free -h printed %r" % v)
    _, _, values = free_values(t, "free -h -b")
    if not all(v.isdigit() for v in values):
        raise AssertionError("free -h -b did not end in bytes: %r" % values)
    _, _, values = free_values(t, "free -bh")
    if not all(re.match(r"^\d+(\.\d)?[BKMG]$", v) for v in values):
        raise AssertionError("free -bh did not end in human sizes: %r" % values)


@test("free refuses an unknown option and an operand, naming the word", needs=("free",))
def free_refusals(t):
    out = t.run("free -x")
    expect_in("invalid option : -x", out, "free -x")
    expect_in("usage: free", out, "free -x")
    out = t.run("free 5")
    expect_in("extra operand : 5", out, "free 5")
    if status(t, "free -x") or status(t, "free 5"):
        raise AssertionError("a refused free reported success")
    if not status(t, "free"):
        raise AssertionError("free reported failure")


@test("free piped into grep keeps the Mem row whole", needs=("free", "grep"))
def free_into_grep(t):
    out = t.run("free | grep Mem")
    expect_in("Mem:", out, "free | grep Mem")
    if len(re.findall(r"\d+", out.split("Mem:", 1)[1])) != 2:
        raise AssertionError("the Mem row came through grep cut: %r" % out)


# ---- logger -------------------------------------------------------------------

LOG = "/var/log/syslog."

# a log outlives the run that wrote it, so each run's markers carry a token of their own
RUN = "%06x" % random.getrandbits(24)


def mark(name):
    """A marker no earlier run has written to the log."""
    return name + RUN


def log_has(t, level, marker):
    """The lines of one log file holding the marker."""
    return [line for line in rows(t.run("cat %s%s" % (LOG, level))) if marker in line]


@test("logger writes the words as one line tagged with the user, into the notice log by default",
      needs=("logger", "cat", "whoami"))
def logger_default(t):
    user = rows(t.run("whoami"))[-1].strip()
    word = mark("lgdef")
    out = t.run("logger %s one two three" % word)
    expect_not_in("CmdErr", out, "logger with a message")
    found = log_has(t, "success", word)
    if len(found) != 1 or not found[0].endswith("%s: %s one two three" % (user, word)):
        raise AssertionError("the notice log holds %r" % found)
    for level in ("error", "warning", "info"):
        if log_has(t, level, word):
            raise AssertionError("the default priority also reached the %s log" % level)


@test("logger -p sends each priority to its log, given alone, joined or after a facility",
      needs=("logger", "cat"))
def logger_priorities(t):
    cases = (("-p err", "error", mark("lgerr")), ("-p crit", "error", mark("lgcrit")),
             ("-p emerg", "error", mark("lgemerg")), ("-p alert", "error", mark("lgalert")),
             ("-p warning", "warning", mark("lgwarn")), ("-p user.warning", "warning", mark("lgfac")),
             ("-pinfo", "info", mark("lgjoin")), ("-p debug", "info", mark("lgdebug")),
             ("-p daemon.notice", "success", mark("lgnotice")))
    for option, level, word in cases:
        out = t.run("logger %s %s" % (option, word))
        expect_not_in("CmdErr", out, "logger %s" % option)
    for option, level, word in cases:
        if len(log_has(t, level, word)) != 1:
            raise AssertionError("logger %s did not leave one line in the %s log" % (option, level))


@test("logger -- takes a message that starts with a dash, and an empty message is still a line",
      needs=("logger", "cat", "whoami"))
def logger_dash_and_empty(t):
    word = "-" + mark("lgdash")
    t.run("logger -- %s degrees" % word)
    if len(log_has(t, "success", "%s degrees" % word)) != 1:
        raise AssertionError("logger -- did not log the dashed message")
    user = rows(t.run("whoami"))[-1].strip()
    out = t.run("logger ''")
    expect_not_in("CmdErr", out, "logger ''")
    last = rows(t.run("cat %ssuccess" % LOG))[-1]
    if not last.endswith("%s:" % user):
        raise AssertionError("logger '' did not leave an empty message last: %r" % last)


@test("logger with no message logs each line of a redirected file or a pipe", needs=("logger", "cat", "echo", "rm"))
def logger_input(t):
    first, second = mark("lgin1"), mark("lgin2")
    t.run("echo %s > /tmp/lgin.txt" % first)
    t.run("echo %s >> /tmp/lgin.txt" % second)
    try:
        t.run("logger -p err < /tmp/lgin.txt")
        t.run("cat /tmp/lgin.txt | logger -p warning")
    finally:
        t.run("rm /tmp/lgin.txt")
    for level in ("error", "warning"):
        found = [line.split(": ", 1)[-1] for line in log_has(t, level, "lgin") if RUN in line]
        if found != [first, second]:
            raise AssertionError("the %s log holds %r for the two input lines" % (level, found))


@test("logger refuses an unknown option, an unknown priority and a missing one, logging nothing",
      needs=("logger", "cat"))
def logger_refusals(t):
    word = mark("lgbad")
    out = t.run("logger -x %s" % word)
    expect_in("invalid option : -x", out, "logger -x")
    out = t.run("logger -p bogus %s" % word)
    expect_in("unknown priority : bogus", out, "logger -p bogus")
    out = t.run("logger -p")
    expect_in("-p needs a priority", out, "logger -p")
    if status(t, "logger") or status(t, "logger -p nope %s" % word):
        raise AssertionError("a refused logger reported success")
    for level in ("success", "error", "warning", "info"):
        if log_has(t, level, word):
            raise AssertionError("a refused logger still wrote to the %s log" % level)


@test("logger cuts a message longer than a log line and the next line still starts on its own",
      needs=("logger", "cat"))
def logger_long(t):
    word = mark("lglong")
    after = mark("lgafter")
    t.run("logger -p info %s%s" % (word, "x" * 400))
    t.run("logger -p info %s" % after)
    found = log_has(t, "info", word)
    if len(found) != 1 or len(found[0]) > 220:
        raise AssertionError("the long message left %r" % [len(f) for f in found])
    following = log_has(t, "info", after)
    if len(following) != 1 or word in following[0]:
        raise AssertionError("the line after the long one is not on its own: %r" % following)


# ---- which and type -------------------------------------------------------------

@test("which prints each name that is a command and nothing for one that is not", needs=("which",))
def which_names(t):
    out = rows(t.run("which ls echo"))
    if out[-2:] != ["ls", "echo"]:
        raise AssertionError("which ls echo printed %r" % out)
    out = rows(t.run("which ls nosuchcmd echo"))
    if out[-2:] != ["ls", "echo"] or any("nosuchcmd" in line for line in out):
        raise AssertionError("which with a missing name printed %r" % out)


@test("which and type succeed only when every name is a command, and scripts can test with them",
      needs=("which", "type", "echo"))
def which_status(t):
    for command, wanted in (("which ls", True), ("which nosuchcmd", False), ("which ls nosuchcmd", False),
                            ("type cat", True), ("type nosuchcmd", False), ("type cat nosuchcmd", False),
                            ("which ''", False), ("which 'ls -l'", False)):
        if status(t, command) != wanted:
            raise AssertionError("%s gave %s" % (command, "failure" if wanted else "success"))
    expect_in("hasls", t.run("which ls && echo hasls"), "which ls && echo")
    expect_in("nocmd", t.run("which nosuchcmd || echo nocmd"), "which nosuchcmd || echo")


@test("type says each name is a builtin or was not found, naming an empty one", needs=("type",))
def type_names(t):
    out = rows(t.run("type ls nosuchcmd"))
    expect_in("ls is a shell builtin", "\n".join(out), "type ls")
    expect_in("type: nosuchcmd: not found", "\n".join(out), "type nosuchcmd")
    expect_in("type: '': not found", t.run("type ''"), "type ''")


@test("which and type ask for a name and refuse an option", needs=("which", "type"))
def which_refusals(t):
    for command in ("which", "type"):
        if status(t, command):
            raise AssertionError("%s with no name succeeded" % command)
        expect_in("usage: %s" % command, t.run(command), command)
        expect_in("invalid option : -x", t.run("%s -x" % command), "%s -x" % command)


# ---- basename and dirname ---------------------------------------------------------

PATHS = (
    ("/a/b/c.txt", "c.txt", "/a/b"),
    ("/a/b/", "b", "/a"),
    ("a", "a", "."),
    ("/a", "a", "/"),
    ("/", "/", "/"),
    ("//", "/", "/"),
    ("a//b", "b", "a"),
    ("a/b//", "b", "a"),
    ("./x", "x", "."),
)


def last_row(t, command):
    out = rows(t.run(command))
    return out[-1].strip() if out else ""


@test("basename and dirname split paths the way linux does, the root and doubled slashes included",
      needs=("basename", "dirname"))
def basename_dirname_table(t):
    for path, base, parent in PATHS:
        got = last_row(t, "basename %s" % path)
        if got != base:
            raise AssertionError("basename %s gave %r, wanted %r" % (path, got, base))
        got = last_row(t, "dirname %s" % path)
        if got != parent:
            raise AssertionError("dirname %s gave %r, wanted %r" % (path, got, parent))


@test("basename removes a suffix only when the name is longer than it", needs=("basename",))
def basename_suffix(t):
    for args, wanted in (("/x/y.tar.gz .gz", "y.tar"), ("/x/y.c .c", "y"), ("c.txt c.txt", "c.txt"),
                         ("/x/y.c .h", "y.c"), ("/x/y.c ''", "y.c")):
        got = last_row(t, "basename %s" % args)
        if got != wanted:
            raise AssertionError("basename %s gave %r, wanted %r" % (args, got, wanted))


@test("basename and dirname of an empty path and inside $(...)", needs=("basename", "dirname", "echo"))
def basename_dirname_empty_and_capture(t):
    if last_row(t, "dirname ''") != ".":
        raise AssertionError("dirname '' did not print .")
    if not status(t, "basename ''"):
        raise AssertionError("basename '' failed")
    expect_in("[y.c|/x]", t.run('echo "[$(basename /x/y.c)|$(dirname /x/y.c)]"'), "basename and dirname in $(...)")


@test("basename and dirname ask for a path and refuse an extra one", needs=("basename", "dirname"))
def basename_dirname_refusals(t):
    for command in ("basename", "dirname"):
        if status(t, command):
            raise AssertionError("%s with no path succeeded" % command)
    expect_in("extra operand : c", t.run("basename a b c"), "basename a b c")
    expect_in("extra operand : b", t.run("dirname a b"), "dirname a b")


# ---- seq --------------------------------------------------------------------------

def numbers(t, command):
    return [line.strip() for line in rows(t.run(command))]


@test("seq counts from 1, between two numbers and by a step, up and down", needs=("seq",))
def seq_runs(t):
    for args, wanted in (("3", ["1", "2", "3"]), ("2 4", ["2", "3", "4"]), ("1 3 10", ["1", "4", "7", "10"]),
                         ("5 -2 1", ["5", "3", "1"]), ("-2 2", ["-2", "-1", "0", "1", "2"]),
                         ("-1 -1 -3", ["-1", "-2", "-3"]), ("7 7", ["7"]),
                         ("2147483646 2147483647", ["2147483646", "2147483647"]),
                         ("-2147483648 -2147483647", ["-2147483648", "-2147483647"])):
        got = numbers(t, "seq %s" % args)
        if got != wanted:
            raise AssertionError("seq %s printed %r, wanted %r" % (args, got, wanted))


@test("seq prints nothing and succeeds when the run cannot reach its end", needs=("seq",))
def seq_empty(t):
    for args in ("0", "5 1", "1 -1 5", "-1"):
        if numbers(t, "seq %s" % args):
            raise AssertionError("seq %s printed numbers" % args)
        if not status(t, "seq %s" % args):
            raise AssertionError("seq %s failed" % args)


@test("seq refuses words, fractions, numbers out of range, a zero step and a fourth operand",
      needs=("seq",))
def seq_refusals(t):
    for args, said in (("x", "invalid number : x"), ("1.5", "invalid number : 1.5"),
                       ("2147483648", "invalid number : 2147483648"), ("1 x 3", "invalid number : x"),
                       ("1 0 5", "the increment cannot be : 0"), ("1 2 3 4", "extra operand : 4")):
        out = t.run("seq %s" % args)
        expect_in(said, out, "seq %s" % args)
        if status(t, "seq %s" % args):
            raise AssertionError("seq %s succeeded" % args)
    if status(t, "seq"):
        raise AssertionError("seq with no number succeeded")


@test("seq piped into wc and sort, and driving a for loop in a script", needs=("seq", "wc", "sort", "echo"))
def seq_into_others(t):
    counts = t.run("seq 50 | wc").split()
    if not counts or counts[0] != "49" and counts[0] != "50":
        raise AssertionError("seq 50 | wc counted %r" % counts)
    if numbers(t, "seq 4 | sort -r") != ["4", "3", "2", "1"]:
        raise AssertionError("seq 4 | sort -r did not reverse")
    expect_in("[1 2 3]", t.run("echo [$(seq 3)]").replace("\r", "").replace("\n", " "), "seq in $(...)")


@test("seq of a long run can be stopped by a reader that takes no more", needs=("seq", "head"))
def seq_long_into_head(t):
    got = numbers(t, "seq 100000 | head 3")
    if got[-3:] != ["1", "2", "3"]:
        raise AssertionError("seq 100000 | head 3 printed %r" % got[-5:])
    expect_in("seqalive", t.run("echo seqalive"), "the shell after a long seq")


# ---- history ------------------------------------------------------------------------

HISTORY_ROW = re.compile(r"^\s*(\d+)(?:  (.*))?$")


def history_rows(t, command):
    found = []
    for line in rows(t.run(command)):
        match = HISTORY_ROW.match(line)
        if match:
            found.append((int(match.group(1)), match.group(2) or ""))
    return found


@test("history lists the lines typed, numbered from the oldest without gaps", needs=("history", "echo"))
def history_lists(t):
    t.run("echo hsfirst")
    t.run("echo hssecond")
    got = history_rows(t, "history")
    numbers_seen = [n for n, _ in got]
    if numbers_seen != list(range(numbers_seen[0], numbers_seen[0] + len(numbers_seen))):
        raise AssertionError("history numbers are not contiguous: %r" % numbers_seen)
    texts = [text for _, text in got]
    if "echo hsfirst" not in texts or "echo hssecond" not in texts or \
            texts.index("echo hsfirst") > texts.index("echo hssecond"):
        raise AssertionError("history does not hold the two lines in order: %r" % texts[-6:])


@test("history N lists only the last N lines and history 0 lists none", needs=("history", "echo"))
def history_last(t):
    t.run("echo hslast")
    got = history_rows(t, "history 2")
    if len(got) != 2:
        raise AssertionError("history 2 listed %d lines" % len(got))
    if history_rows(t, "history 0"):
        raise AssertionError("history 0 listed lines")


@test("history after the file has been trimmed holds no empty line", needs=("history", "echo"))
def history_after_trim(t):
    for i in range(30):
        t.run("echo hstrim%d" % i)
    got = history_rows(t, "history")
    empty = [n for n, text in got if not text.strip()]
    if empty:
        raise AssertionError("history holds empty lines at %r after a trim" % empty)
    if len(got) < 10:
        raise AssertionError("history kept only %d lines after a trim" % len(got))


@test("history refuses a count that is not a number and a second operand", needs=("history",))
def history_refusals(t):
    expect_in("invalid count : x", t.run("history x"), "history x")
    expect_in("extra operand : 2", t.run("history 1 2"), "history 1 2")
    if status(t, "history -1"):
        raise AssertionError("history -1 succeeded")


# ---- sleep --------------------------------------------------------------------------

def timed(t, command, timeout=30):
    start = time.time()
    out = t.run(command, timeout=timeout)
    return out, time.time() - start


@test("sleep waits the whole seconds given", needs=("sleep",))
def sleep_waits(t):
    for seconds in (1, 2):
        out, took = timed(t, "sleep %d" % seconds)
        expect_not_in("CmdErr", out, "sleep %d" % seconds)
        if took < seconds * 0.9 or took > seconds + 3.0:
            raise AssertionError("sleep %d took %.2f s" % (seconds, took))
    _, took = timed(t, "sleep 0")
    if took > 2.0:
        raise AssertionError("sleep 0 took %.2f s" % took)


@test("sleep refuses a fraction, a unit, a word, a number past its range, a negative one, an extra operand and none, without waiting",
      needs=("sleep",))
def sleep_refusals(t):
    for args, said in (("x", "invalid time interval : x (0 to 2147483)"), ("0.5", "invalid time interval : 0.5"),
                       ("1s", "invalid time interval : 1s"), ("2m", "invalid time interval : 2m"),
                       ("2147484", "invalid time interval : 2147484"),
                       ("-1", "invalid option : -1"), ("1 2", "extra operand : 2")):
        out, took = timed(t, "sleep %s" % args)
        expect_in(said, out, "sleep %s" % args)
        if took > 3.0:
            raise AssertionError("a refused sleep %s still waited %.2f s" % (args, took))
    for args in ("x", "", "1 2", "0.5"):
        if status(t, ("sleep %s" % args).strip()):
            raise AssertionError("sleep %r succeeded" % args)


@test("what follows sleep on its line runs after it by its result", needs=("sleep", "echo"))
def sleep_chains(t):
    out, took = timed(t, "sleep 1 && echo slA")
    expect_in("slA", out, "sleep 1 && echo")
    if took < 0.9:
        raise AssertionError("the chain did not wait: %.2f s" % took)
    expect_in("slB", t.run("sleep 1; echo slB"), "sleep; echo")
    expect_not_in("slC", t.run("sleep 1 || echo slC"), "sleep || echo")
    expect_in("slD", t.run("sleep x || echo slD"), "a refused sleep || echo")
    out, took = timed(t, "sleep 1 && sleep 1 && echo slE")
    expect_in("slE", out, "two sleeps chained")
    if took < 1.8:
        raise AssertionError("two chained sleeps took %.2f s" % took)


@test("a line typed while sleep waits runs once it is done", needs=("sleep", "echo"))
def sleep_typeahead(t):
    t.shell.send_line("sleep 2")
    start = time.time()
    t.shell.send_line("echo sltyped")
    t.shell.expect(re.compile(r"\nsltyped"), 15)
    if time.time() - start < 1.5:
        raise AssertionError("the typed line ran before the sleep was over")
    t.shell.expect(PROMPT, 15)
    expect_in("slafter", t.run("echo slafter"), "the shell after a typed-ahead line")


@test("ctrl c ends a sleep and drops the rest of its line, and the shell carries on",
      needs=("sleep", "echo"))
def sleep_interrupt(t):
    t.shell.send_line("sleep 60 && echo slnever")
    time.sleep(1.0)
    start = time.time()
    t.shell.send_raw("\x03")
    out = t.shell.expect("CmdErr : -3609", 10)
    if time.time() - start > 5.0:
        raise AssertionError("ctrl c took %.2f s to end the sleep" % (time.time() - start))
    out += t.shell.expect(PROMPT, 15)
    if any(line.strip() == "slnever" for line in out.splitlines()):
        raise AssertionError("the rest of an interrupted line ran:\n%s" % out)
    expect_in("slalive", t.run("echo slalive"), "the shell after ctrl c")


@test("a script sleeps between its lines and runs a chain after a sleep", needs=("sleep", "source", "echo", "rm"))
def sleep_in_script(t):
    t.run("echo 'echo sl1' > /tmp/slscript.sh")
    t.run("echo 'sleep 1 && echo sl2' >> /tmp/slscript.sh")
    t.run("echo 'echo sl3' >> /tmp/slscript.sh")
    try:
        out, took = timed(t, "source /tmp/slscript.sh")
    finally:
        t.run("rm /tmp/slscript.sh")
    found = [line.strip() for line in out.splitlines() if line.strip().startswith("sl")]
    if found != ["sl1", "sl2", "sl3"]:
        raise AssertionError("the script printed %r" % found)
    if took < 0.9:
        raise AssertionError("the script did not wait: %.2f s" % took)


@test("sleep waits inside a pipe, a capture and a redirect, and refuses there past the limit",
      needs=("sleep", "wc", "echo", "cat"))
def sleep_where_it_cannot_resume(t):
    try:
        for command in ("sleep 1 | wc", "echo $(sleep 1)slcap", "sleep 1 > /tmp/slred; echo slred"):
            out, took = timed(t, command)
            if took < 0.9:
                raise AssertionError("%s did not wait: %.2f s" % (command, took))
    finally:
        t.run("rm /tmp/slred")
    expect_in("slcap", t.run("echo $(sleep 1)slcap"), "sleep in $(...)")
    out, took = timed(t, "sleep 11 | wc")
    if took > 5.0:
        raise AssertionError("a refused long sleep in a pipe waited %.2f s" % took)
    expect_in("refused where nothing can resume it", t.run("cat /var/log/syslog.warning"),
              "the log after a refused long sleep")


@test("the board keeps serving while a session sleeps", needs=("sleep", "echo"), slow=True)
def sleep_does_not_hold_the_board(t):
    portal = t.portal(login=False)
    t.shell.send_line("sleep 4")
    time.sleep(0.5)
    start = time.time()
    answer = portal.get("/login")
    took = time.time() - start
    if answer.status not in (200, 301, 302):
        raise AssertionError("the portal answered %d while a session slept" % answer.status)
    if took > 2.5:
        raise AssertionError("the portal took %.2f s to answer while a session slept" % took)
    t.shell.expect(PROMPT, 15)
    expect_in("slserved", t.run("echo slserved"), "the shell after the sleep")
