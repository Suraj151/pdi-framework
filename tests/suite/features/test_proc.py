#!/usr/bin/env python3

"""
Tasks and services as the shell reports them.

Mostly read-only about the live services, so a signal is never aimed at one of
them. Where a positive signal has to be proven, the test makes its own task to
aim at: `watch` and `top` register a background scheduler task the session owns,
so `renice`, `pkill` and `killall` can act on that and never on a service. The
task is torn down again by name, or by pid if the signal under test failed.
"""

import re
import time

from .registry import test, expect_in, expect_not_in, expect_any, Skip


def _ps_rows(text):
    """The task rows of a ps listing, each split into its columns."""
    rows = []
    for line in text.splitlines():
        parts = line.split()
        if len(parts) >= 10 and parts[0].isdigit():
            rows.append(parts)
    return rows


def _settled_run(t, command):
    """
    Run a command with the transport quiet first, and use this for every read
    taken while one of these tests has a watch task running.

    A watch task redraws the terminal on its own account, and a reply is read up
    to the next prompt, so a redraw that lands mid-command ends the read early
    and every command after it returns the reply belonging to the one before.
    Measured on a board: with a watch task redrawing at 1200 ms, 11 of 12 marker
    commands came back with another command's reply, and the lag grew as it went;
    with the transport drained first, 0 of 10 did. Without this the tests here
    read a table that is one or more commands out of date, and time out against a
    task that was reaped long before.
    """
    t.shell.drain(0.6, 4.0)
    return t.run(command)


def _read_ps(t, tries=6):
    """
    A ps listing, re-read until it is whole. A background task started by these
    tests clears the display and redraws the prompt around its output, which can
    end a plain read early; a listing without its header is one of those, so it
    is read again.
    """
    out = ""
    for _ in range(tries):
        out = _settled_run(t, "ps")
        if "PID" in out and "NAME" in out:
            return out
    return out


def _nice_of(text, pid):
    for parts in _ps_rows(text):
        if parts[0] == str(pid):
            return parts[4]
    return None


def _watch_pids(t):
    """
    The pids currently listed as 'watch', or None when the listing could not be
    read at all.

    A watch task clears the display as it runs, which can cut a reading short.
    A short reading holds no rows, so telling it apart from a genuinely empty
    table matters: read as an empty table it says the task is gone, which passes
    a reap assertion that should have waited. Callers poll, so one unreadable
    listing costs a retry rather than the six a settled read would spend.
    """
    text = _settled_run(t, "ps")
    if "PID" not in text or "NAME" not in text:
        return None
    return [parts[0] for parts in _ps_rows(text) if parts[-1] == "watch"]


def _start_background_watch(t, interval_ms):
    """
    Register a session-owned scheduler task named 'watch' and return its pid.

    The watched command touches a file in a scratch directory rather than
    printing, so it does not pour command output into the shell between the reads
    these tests make. watch itself still clears the display once per interval, so
    the tests that follow judge the signal by its effect on the process table,
    not by reading a reply back over that same clearing stream. A short interval
    is kept so the scheduler visits the task often enough to apply a pending
    signal within the poll window.

    Registration is waited for rather than timed. A fixed drain races the board:
    under load the task can appear after it, which reads as a task that never
    started, and any watch already present would be mistaken for this one.
    Returning the pid lets a caller signal what it started instead of whatever
    the table happens to list first.
    """
    before = set(_watch_pids(t) or [])

    target = t.workspace("watch_signal")
    t.shell.send_line("watch c=touch %s/tick,i=%d,n=100" % (target, interval_ms))
    t.shell.drain(2.0, 5.0)

    for _ in range(12):
        listed = _watch_pids(t)
        if listed is not None:
            fresh = [pid for pid in listed if pid not in before]
            if fresh:
                return fresh[0]
        time.sleep(0.5)

    raise Skip("could not start a background task to signal")


WATCH_SIGNAL_INTERVAL_MS = 1200

SIGNAL_DELIVERY_TURNS = 40


def _await_signal(interval_ms, delivered):
    """
    Wait for a signalled task to take delivery, counted in that task's own turns
    rather than in seconds.

    A signal is only recorded as pending against the task. The scheduler
    consumes it when that task next reaches the front of its sorted list, which
    needs the task to come due and then to win on vruntime, so the only wait
    that means anything is a multiple of the interval the task was given.

    The turn count is generous on purpose. These tasks run inline on the
    cooperative loop, so a turn comes round when the rest of the loop allows it
    and the wait is not a stable quantity to calibrate against. A watch task also
    redraws a terminal, which is expensive, so it takes a large charge on the run
    it does make and waits for that debt before it is picked again. The budget is
    sized to absorb that spread rather than to match any measured figure.
    """
    deadline = time.time() + (SIGNAL_DELIVERY_TURNS * interval_ms) / 1000.0
    while True:
        time.sleep(0.5)
        if delivered():
            return True
        if time.time() >= deadline:
            return False


def _await_watch_gone(t, pid=None):
    """
    Wait for a signalled watch task to leave the table, and say why it did not.

    True once it has gone, False only when a listing that could be read still
    holds it. A listing that came back short is neither answer, so it costs a
    retry rather than counting as the task still running. A whole window that
    never yielded one readable listing is itself a failure, reported as that
    rather than as a task refusing its signal: the shell not being serviced for
    the length of the window is the scheduler's business, and skipping it would
    retire the assertion this test exists to make.

    Every reading here goes through _settled_run, so a listing is this command's
    own reply rather than an earlier one still in the stream.
    """
    deadline = time.time() + (SIGNAL_DELIVERY_TURNS * WATCH_SIGNAL_INTERVAL_MS) / 1000.0
    readable = False
    while True:
        time.sleep(0.5)
        listed = _watch_pids(t)
        if listed is not None:
            readable = True
            gone = (pid not in listed) if pid is not None else (listed == [])
            if gone:
                return True
        if time.time() >= deadline:
            break

    if not readable:
        raise AssertionError(
            "no readable process table in %d turns, so the signal could not be judged"
            % SIGNAL_DELIVERY_TURNS)
    return False


def _reap_watch_by_pid(t):
    """
    Kill every watch task, wait for the table to clear, and leave the transport
    quiet again.

    A watch task writes to the terminal on its own account, so its last redraw
    can arrive after the command that killed it. Read against the next command
    it becomes that command's reply, and the damage outlives this test: a
    listing test that followed one of these was handed the `cd /` echoed by a
    workspace several commands earlier. Settling on a token is what stops a
    signal test spending its mess on whatever runs next, the same way the tests
    that drive top and watch interactively already do.
    """
    for pid in (_watch_pids(t) or []):
        t.run("kill 9 %s" % pid)

    _await_signal(WATCH_SIGNAL_INTERVAL_MS, lambda: _watch_pids(t) == [])
    t.resync()


@test("ps prints a header", needs=("ps",))
def ps_header(t):
    out = t.run("ps")
    expect_in("PID", out, "ps header")
    expect_any(("CMD", "NAME"), out, "ps names its tasks")


@test("ps lists the running tasks", needs=("ps",))
def ps_lists(t):
    out = t.run("ps")

    rows = [line for line in out.splitlines() if line.strip() and "PID" not in line]
    if not rows:
        raise AssertionError("ps listed no tasks at all:\n%s" % out)


@test("ps can be filtered by owner", needs=("ps",))
def ps_filtered(t):
    everything = t.run("ps")
    mine = t.run("ps %s" % t.username)

    if len(mine.splitlines()) > len(everything.splitlines()):
        raise AssertionError("a filtered ps listed more than the whole table:\n%s" % mine)


@test("kill of a pid that is not there reports it", needs=("kill",))
def kill_absent(t):
    out = t.run("kill 60000")
    expect_any(("no such", "not found", "CmdErr", "failed"), out, "kill of an absent pid")


@test("service list names the running services", needs=("service",))
def service_list(t):
    out = t.run("service list")
    expect_in("SERVICE", out, "service header")
    expect_in("STATE", out, "service header")

    if not t.caps.services:
        raise AssertionError("service listed no services:\n%s" % out)


@test("service status describes one service", needs=("service",))
def service_status(t):
    name = next(iter(sorted(t.caps.services)), None)
    if name is None:
        raise Skip("no services listed on this target")

    out = t.run("service status %s" % name)
    expect_in("service", out, "service status")
    expect_in(name, out, "the service it was asked about")
    expect_in("state", out, "service status")


@test("service status of an unknown service is refused", needs=("service",))
def service_unknown(t):
    expect_any(("no such service", "CmdErr"), t.run("service status nosuchsvc"),
               "service status of a name that is not there")


@test("uptime reports a duration", needs=("uptime",))
def uptime_reports(t):
    out = t.run("uptime")
    expect_any(("up", "day", "min", "sec", ":"), out, "uptime")


@test("uptime grows", needs=("uptime", "ping"), slow=True)
def uptime_grows(t):
    """
    The reading is weighted by the unit each field carries. Adding the fields up
    instead makes the total fall every time one of them rolls over, so an uptime
    that grew from 0h 59m 29s to 1h 0m 31s reads as 88 going back to 32.
    """
    units = (("d", 86400), ("h", 3600), ("m", 60), ("s", 1))

    def seconds(text):
        total = 0
        for value, unit in re.findall(r"(\d+)\s*([dhms])", text):
            for name, scale in units:
                if name == unit:
                    total += int(value) * scale
        return total

    first = seconds(t.run("uptime"))
    # ping to an address nothing answers is a portable way to spend a couple of
    # seconds on the target rather than on this side of the wire
    t.run("ping 192.0.2.1 2", timeout=30)
    second = seconds(t.run("uptime"))

    if second < first:
        raise AssertionError("uptime went backwards: %d then %d" % (first, second))


@test("help lists every registered command", needs=("help",))
def help_lists(t):
    out = t.run("help")
    expect_in("Registered commands", out, "help header")

    for name in ("ls", "cd", "help"):
        expect_in(name, out, "help lists %s" % name)


@test("help prints a usage line for each command", needs=("help",))
def help_usage(t):
    out = t.run("help")

    for line in out.splitlines():
        if line.strip().startswith("ls "):
            if len(line.split()) < 2:
                raise AssertionError("ls has no usage text in help:\n%s" % line)
            return

    raise AssertionError("no ls row in help:\n%s" % out)


@test("an unknown command is reported as not found", needs=("pwd",))
def unknown_command(t):
    expect_in("nosuchcommand: command not found", t.run("nosuchcommand"),
              "an unknown command names itself")

    # the arguments are not part of the name, the way a shell reports it
    expect_in("nosuchcmd: command not found", t.run("nosuchcmd one two"),
              "an unknown command with arguments")

    expect_in("/", t.run("pwd"), "the shell is still usable")


@test("an empty line reports nothing", needs=("pwd",))
def empty_line_is_silent(t):
    # the not-found result is also what an empty line carries, so this is the
    # case that tells the report apart from the default
    expect_not_in("command not found", t.run(""), "an empty line")
    expect_in("/", t.run("pwd"), "the shell is still usable")


@test("ps prints the task table under its column names and nothing else", needs=("ps",))
def ps_shows_only_the_table(t):
    out = _read_ps(t)
    expect_in("PID", out, "ps names its columns")
    expect_not_in("top -", out, "ps leaves the summary line to top")
    expect_not_in("free heap", out, "ps leaves the summary line to top")


@test("top renders the process table and stops after n iterations", needs=("top",), slow=True)
def top_runs_and_stops(t):
    """
    top clears the display and redraws a frame each interval, so a timed drain
    can return between frames with nothing in it. Reading up to a token that a
    rendered frame must contain waits for a real frame however the timing falls.
    The summary line (with the free heap) is printed first, then the PID..NAME
    header, so reading up to NAME captures all three in one frame.
    """
    t.shell.send_line("top i=500,n=2")
    frame = t.shell.expect("NAME", timeout=max(t.timeout, 12))

    expect_in("PID", frame, "top shows the ps header")
    expect_in("free heap", frame, "top shows the free heap")

    if not t.resync():
        raise AssertionError("top did not return the shell to a prompt")
    expect_in("/", t.run("pwd"), "the shell is usable after top")


@test("watch re-runs a command and stops after n iterations",
      needs=("watch", "whoami"), slow=True)
def watch_runs_and_stops(t):
    t.shell.send_line("watch c=whoami,i=400,n=2")
    frame = t.shell.expect(t.username, timeout=max(t.timeout, 12))

    expect_in(t.username, frame, "watch shows the watched command's output")

    if not t.resync():
        raise AssertionError("watch did not return the shell to a prompt")
    expect_in("/", t.run("pwd"), "the shell is usable after watch")


@test("renice changes a task's nice value", needs=("renice", "ps"))
def renice_changes_nice(t):
    rows = _ps_rows(_read_ps(t))
    if not rows:
        raise Skip("no tasks to renice")

    pid = rows[0][0]
    original = rows[0][4]
    target = "7" if original != "7" else "5"

    try:
        t.run("renice %s %s" % (target, pid))
        after = _nice_of(_read_ps(t), pid)
        if after != target:
            raise AssertionError("renice set nice to %s, ps shows %s" % (target, after))
    finally:
        t.run("renice %s %s" % (original, pid))


@test("renice of an absent pid is refused", needs=("renice",))
def renice_absent(t):
    expect_any(("no such", "not found", "CmdErr", "failed"),
               t.run("renice 5 60000"), "renice of a pid that is not there")


@test("killall of an unmatched name signals nothing", needs=("killall",))
def killall_no_match(t):
    expect_in("signaled 0", t.run("killall nosuchtaskxyz"),
              "killall with nothing to match")


@test("pkill of an unmatched name signals nothing", needs=("pkill",))
def pkill_no_match(t):
    expect_in("signaled 0", t.run("pkill nosuchtaskxyz"),
              "pkill with nothing to match")


@test("killall signals a task by name and it is reaped",
      needs=("killall", "watch", "ps"), slow=True)
def killall_reaps_named_task(t):
    _start_background_watch(t, WATCH_SIGNAL_INTERVAL_MS)

    try:
        t.run("killall watch")

        if not _await_watch_gone(t):
            raise AssertionError("killall did not reap the watch task")
    finally:
        _reap_watch_by_pid(t)


@test("pkill signals a task by name and it is reaped",
      needs=("pkill", "watch", "ps"), slow=True)
def pkill_reaps_named_task(t):
    _start_background_watch(t, WATCH_SIGNAL_INTERVAL_MS)

    try:
        t.run("pkill watch")

        if not _await_watch_gone(t):
            raise AssertionError("pkill did not reap the watch task")
    finally:
        _reap_watch_by_pid(t)


@test("kill terminates a task by pid", needs=("kill", "watch", "ps"), slow=True)
def kill_by_pid_reaps(t):
    pid = _start_background_watch(t, WATCH_SIGNAL_INTERVAL_MS)

    reaped = False
    try:
        t.run("kill 9 %s" % pid)
        reaped = _await_watch_gone(t, pid)
        if not reaped:
            raise AssertionError("kill did not reap the task with pid %s" % pid)
    finally:
        if not reaped:
            _reap_watch_by_pid(t)


def _watch_state(t, pid):
    for parts in _ps_rows(_read_ps(t)):
        if parts[0] == pid and parts[-1] == "watch":
            return parts[2]
    return None


@test("kill stops and continues a task", needs=("kill", "watch", "ps"), slow=True)
def kill_stop_cont(t):
    """
    STOP is taken up when the task next reaches the front of the sorted list,
    so the wait is counted in the task's own turns. CONT is quicker to land: a
    stopped task neither runs nor advances its due time, so it stays due with
    the least vruntime and wins the next selection. The state column is T while
    stopped, S/r once resumed.
    """
    pid = _start_background_watch(t, WATCH_SIGNAL_INTERVAL_MS)

    try:
        t.run("kill 19 %s" % pid)
        if not _await_signal(WATCH_SIGNAL_INTERVAL_MS, lambda: _watch_state(t, pid) == "T"):
            raise AssertionError("STOP did not move the task to the stopped state")

        t.run("kill 18 %s" % pid)
        if not _await_signal(WATCH_SIGNAL_INTERVAL_MS,
                             lambda: _watch_state(t, pid) in ("S", "r", "R")):
            raise AssertionError("CONT did not resume the task")
    finally:
        _reap_watch_by_pid(t)


SAFE_SERVICES = ("MQTT", "OTA", "MDNS", "Email", "GPIO", "FactoryReset")


@test("the syslog service writes log lines to a file", needs=("ls",))
def syslog_writes_to_file(t):
    logs = [line for line in t.run("ls -la /var/log").splitlines() if "syslog." in line]
    if not logs:
        raise Skip("no syslog files on this target")

    def _size(line):
        parts = line.split()
        for field in parts:
            if field.isdigit():
                return int(field)
        return 0

    if not any(_size(line) > 0 for line in logs):
        raise AssertionError("syslog files exist but are all empty:\n%s" % "\n".join(logs))


@test("service restarts a service and it stays active", needs=("service",), slow=True)
def service_restart(t):
    service = None
    for line in t.run("service list").splitlines():
        parts = line.split()
        if len(parts) >= 2 and parts[0] in SAFE_SERVICES and parts[1] == "active":
            service = parts[0]
            break
    if service is None:
        raise Skip("no service safe to restart is active")

    expect_in("restart", t.run("service restart %s" % service, timeout=15).lower(),
              "service restart reports it acted")
    time.sleep(1.5)
    expect_in("active", t.run("service status %s" % service, timeout=10),
              "the service is active again after a restart")


@test("a quoted option value keeps the separator inside it",
      needs=("watch", "echo"), slow=True)
def watch_quoted_inner_command(t):
    """
    watch's c= carries a whole command, which may use the same separator for
    its own options. Quoting it is what keeps that separator out of watch's
    parse; if it leaked, i= and n= would never be seen and the watch would
    never stop.
    """
    t.shell.send_line('watch c="echo a,b",i=400,n=2')
    t.shell.drain(1.0, 8.0)

    if not t.resync():
        raise AssertionError("watch never stopped, so it did not see n= past "
                             "the comma in its quoted command")
    expect_in("/", t.run("pwd"), "the shell is usable after watch")
