#!/usr/bin/env python3

"""
Tasks and services as the shell reports them.

Mostly read-only about the live services, so a signal is never aimed at one of
them. Where a positive signal has to be proven, the test makes its own task to
aim at: `watch` and `top` register a background scheduler task the session owns,
so `renice`, `pkill` and `killall` can act on that and never on a service. The
task is torn down again by name, or by pid if the signal under test failed.
"""

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


def _read_ps(t, tries=6):
    """
    A ps listing, re-read until it is whole. A background task started by these
    tests clears the display and redraws the prompt around its output, which can
    end a plain read early; a listing without its header is one of those, so it
    is read again.
    """
    out = ""
    for _ in range(tries):
        out = t.run("ps")
        if "PID" in out and "NAME" in out:
            return out
    return out


def _nice_of(text, pid):
    for parts in _ps_rows(text):
        if parts[0] == str(pid):
            return parts[4]
    return None


def _has_task(text, name):
    return any(parts[-1] == name for parts in _ps_rows(text))


def _start_background_watch(t, interval_ms):
    """
    Register a session-owned scheduler task named 'watch' to signal, or skip.

    The watched command touches a file in a scratch directory rather than
    printing, so it does not pour command output into the shell between the reads
    these tests make. watch itself still clears the display once per interval, so
    the tests that follow judge the signal by its effect on the process table,
    not by reading a reply back over that same clearing stream. A short interval
    is kept so the scheduler visits the task often enough to apply a pending
    signal within the poll window.
    """
    target = t.workspace("watch_signal")
    t.shell.send_line("watch c=touch %s/tick; i=%d; n=100" % (target, interval_ms))
    t.shell.drain(2.0, 5.0)
    if not _has_task(_read_ps(t), "watch"):
        raise Skip("could not start a background task to signal")


def _reap_watch_by_pid(t):
    for parts in _ps_rows(_read_ps(t)):
        if parts[-1] == "watch":
            t.run("kill 9 %s" % parts[0])


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


@test("srvc list names the running services", needs=("srvc",))
def srvc_list(t):
    out = t.run("srvc list")
    expect_in("SERVICE", out, "srvc header")
    expect_in("STATE", out, "srvc header")

    if not t.caps.services:
        raise AssertionError("srvc listed no services:\n%s" % out)


@test("srvc status describes one service", needs=("srvc",))
def srvc_status(t):
    name = next(iter(sorted(t.caps.services)), None)
    if name is None:
        raise Skip("no services listed on this target")

    out = t.run("srvc status %s" % name)
    expect_in("service", out, "srvc status")
    expect_in(name, out, "the service it was asked about")
    expect_in("state", out, "srvc status")


@test("srvc status of an unknown service is refused", needs=("srvc",))
def srvc_unknown(t):
    expect_any(("no such service", "CmdErr"), t.run("srvc status nosuchsvc"),
               "srvc status of a name that is not there")


@test("uptime reports a duration", needs=("uptime",))
def uptime_reports(t):
    out = t.run("uptime")
    expect_any(("up", "day", "min", "sec", ":"), out, "uptime")


@test("uptime grows", needs=("uptime", "ping"), slow=True)
def uptime_grows(t):
    def seconds(text):
        digits = [int(part) for part in "".join(
            ch if ch.isdigit() else " " for ch in text).split()]
        return sum(digits)

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


@test("ps reports the free heap", needs=("ps",))
def ps_shows_free_heap(t):
    expect_in("free heap", _read_ps(t), "the ps header names the free heap")


@test("top renders the process table and stops after n iterations", needs=("top",), slow=True)
def top_runs_and_stops(t):
    """
    top clears the display and redraws a frame each interval, so a timed drain
    can return between frames with nothing in it. Reading up to a token that a
    rendered frame must contain waits for a real frame however the timing falls.
    The summary line (with the free heap) is printed first, then the PID..NAME
    header, so reading up to NAME captures all three in one frame.
    """
    t.shell.send_line("top i=400; n=2")
    frame = t.shell.expect("NAME", timeout=max(t.timeout, 12))

    expect_in("PID", frame, "top shows the ps header")
    expect_in("free heap", frame, "top shows the free heap")

    if not t.resync():
        raise AssertionError("top did not return the shell to a prompt")
    expect_in("/", t.run("pwd"), "the shell is usable after top")


@test("watch re-runs a command and stops after n iterations",
      needs=("watch", "whoami"), slow=True)
def watch_runs_and_stops(t):
    t.shell.send_line("watch c=whoami; i=400; n=2")
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
    _start_background_watch(t, 1200)

    try:
        t.run("killall watch")

        for _ in range(8):
            time.sleep(1.0)
            if not _has_task(_read_ps(t), "watch"):
                return
        raise AssertionError("killall did not reap the watch task")
    finally:
        _reap_watch_by_pid(t)


@test("pkill signals a task by name and it is reaped",
      needs=("pkill", "watch", "ps"), slow=True)
def pkill_reaps_named_task(t):
    _start_background_watch(t, 1200)

    try:
        t.run("pkill watch")

        for _ in range(8):
            time.sleep(1.0)
            if not _has_task(_read_ps(t), "watch"):
                return
        raise AssertionError("pkill did not reap the watch task")
    finally:
        _reap_watch_by_pid(t)


@test("kill terminates a task by pid", needs=("kill", "watch", "ps"), slow=True)
def kill_by_pid_reaps(t):
    _start_background_watch(t, 1200)

    reaped = False
    try:
        rows = [parts for parts in _ps_rows(_read_ps(t)) if parts[-1] == "watch"]
        if not rows:
            raise Skip("no task to kill")

        t.run("kill 9 %s" % rows[0][0])
        for _ in range(8):
            time.sleep(1.0)
            if not _has_task(_read_ps(t), "watch"):
                reaped = True
                return
        raise AssertionError("kill did not reap the task by pid")
    finally:
        if not reaped:
            _reap_watch_by_pid(t)


def _watch_state(t):
    for parts in _ps_rows(_read_ps(t)):
        if parts[-1] == "watch":
            return parts[2]
    return None


@test("kill stops and continues a task", needs=("kill", "watch", "ps"), slow=True)
def kill_stop_cont(t):
    """
    STOP/CONT are applied when the scheduler next visits the task, which is
    interval-gated, so the state is polled across several visits rather than read
    once. Each read can also land on one of the task's display clears and come
    back short, so the window is kept wide enough to see a clean one. The state
    column is T while stopped, S/r once resumed.
    """
    _start_background_watch(t, 2000)

    reaped = False
    try:
        rows = [parts for parts in _ps_rows(_read_ps(t)) if parts[-1] == "watch"]
        if not rows:
            raise Skip("no task to signal")
        pid = rows[0][0]

        t.run("kill 19 %s" % pid)
        stopped = False
        for _ in range(16):
            time.sleep(0.6)
            if _watch_state(t) == "T":
                stopped = True
                break
        if not stopped:
            raise AssertionError("STOP did not move the task to the stopped state")

        t.run("kill 18 %s" % pid)
        resumed = False
        for _ in range(16):
            time.sleep(0.6)
            if _watch_state(t) in ("S", "r", "R"):
                resumed = True
                break
        if not resumed:
            raise AssertionError("CONT did not resume the task")
    finally:
        _reap_watch_by_pid(t)


SAFE_SERVICES = ("MQTT", "OTA", "MDNS", "Email", "GPIO", "FactoryReset")


@test("the syslog service writes log lines to a file", needs=("ls",))
def syslog_writes_to_file(t):
    logs = [line for line in t.run("ls /var/log").splitlines() if "syslog." in line]
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


@test("srvc restarts a service and it stays active", needs=("srvc",), slow=True)
def srvc_restart(t):
    service = None
    for line in t.run("srvc list").splitlines():
        parts = line.split()
        if len(parts) >= 2 and parts[0] in SAFE_SERVICES and parts[1] == "active":
            service = parts[0]
            break
    if service is None:
        raise Skip("no service safe to restart is active")

    expect_in("restart", t.run("srvc restart %s" % service, timeout=15).lower(),
              "srvc restart reports it acted")
    time.sleep(1.5)
    expect_in("active", t.run("srvc status %s" % service, timeout=10),
              "the service is active again after a restart")
