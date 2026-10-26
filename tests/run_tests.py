#!/usr/bin/env python3

"""
Run the pdi framework test suite.

Tiers
  unit    native host build of the framework against the mock device
  system  the whole stack running as a host process, driven through its shell
  device  the same feature suite over serial or ssh against real hardware (pending)
  fuzz    libFuzzer harnesses over the parsers that face the network pre-auth
"""

import argparse
import os
import re
import shutil
import signal
import subprocess
import sys
import time

TESTS_DIR = os.path.dirname(os.path.abspath(__file__))
HOST_DIR = os.path.join(TESTS_DIR, "host")
BUILD_DIR = os.path.join(TESTS_DIR, ".build")
FUZZ_DIR = os.path.join(TESTS_DIR, "fuzz")
FUZZ_BUILD_DIR = os.path.join(TESTS_DIR, ".build-fuzz")

TIERS = ("unit", "system", "device", "fuzz", "perf")

# one per parser that a peer reaches before any credential is checked
FUZZ_TARGETS = (
    "fuzz_ssh_wire",
    "fuzz_sftp",
    "fuzz_http",
    "fuzz_http_response",
    "fuzz_shell",
    "fuzz_dbrecord",
    "fuzz_config",
    "fuzz_crontab",
    "fuzz_mdns",
    "fuzz_json",
    "fuzz_mqtt",
    "fuzz_devices",
)

GREEN = "\033[32m"
RED = "\033[31m"
DIM = "\033[2m"
RESET = "\033[0m"


def say(message):
    print(message, flush=True)


def fail(message):
    say("%s%s%s" % (RED, message, RESET))


def require_tool(name):
    if shutil.which(name) is None:
        fail("%s not found on PATH" % name)
        return False
    return True


def configure(sanitize):
    if not require_tool("cmake"):
        return False

    args = [
        "cmake",
        "-S", HOST_DIR,
        "-B", BUILD_DIR,
        "-DCMAKE_BUILD_TYPE=Debug",
        "-DPDI_TEST_SANITIZE=%s" % ("ON" if sanitize else "OFF"),
    ]

    result = subprocess.run(args, capture_output=True, text=True)
    if result.returncode != 0:
        fail("cmake configure failed")
        say(result.stdout)
        say(result.stderr)
        return False

    return True


def build():
    args = ["cmake", "--build", BUILD_DIR, "-j", str(os.cpu_count() or 2)]
    result = subprocess.run(args, capture_output=True, text=True)
    if result.returncode != 0:
        fail("build failed")
        say(result.stdout)
        say(result.stderr)
        return False

    warnings = [line for line in result.stderr.splitlines() if "warning:" in line]
    if warnings:
        say("%s%d compiler warning(s); build/ holds the full log%s" % (DIM, len(warnings), RESET))

    return True


def configure_fuzz():
    """libFuzzer ships with clang, so the whole tree is built by clang here."""

    if not require_tool("cmake"):
        return False

    if shutil.which("clang++") is None:
        fail("clang++ not found on PATH; the fuzz tier cannot be built with gcc")
        return False

    args = [
        "cmake",
        "-S", HOST_DIR,
        "-B", FUZZ_BUILD_DIR,
        "-DCMAKE_BUILD_TYPE=Debug",
        "-DCMAKE_C_COMPILER=clang",
        "-DCMAKE_CXX_COMPILER=clang++",
        "-DPDI_BUILD_FUZZ=ON",
    ]

    result = subprocess.run(args, capture_output=True, text=True)
    if result.returncode != 0:
        fail("fuzz cmake configure failed")
        say(result.stdout)
        say(result.stderr)
        return False

    return True


def build_fuzz(targets):
    args = ["cmake", "--build", FUZZ_BUILD_DIR, "-j", str(os.cpu_count() or 2)]
    for target in targets:
        args += ["--target", target]

    result = subprocess.run(args, capture_output=True, text=True)
    if result.returncode != 0:
        fail("fuzz build failed")
        say(result.stdout)
        say(result.stderr)
        return False

    return True


def run_fuzz(targets, seconds, verbose, list_only, jobs=1):
    """Run each harness for a bounded time, up to `jobs` at once, and report anything it saved."""

    if list_only:
        for target in targets:
            say("fuzz.%s" % target)
        return 0

    if not configure_fuzz():
        return 1
    if not build_fuzz(targets):
        return 1

    subprocess.run([sys.executable, os.path.join(FUZZ_DIR, "MakeCorpus.py")],
                   capture_output=True, text=True)

    environment = dict(os.environ)
    # the framework keeps service state alive on purpose, so the leak check at
    # exit would report it against every harness
    environment["ASAN_OPTIONS"] = "detect_leaks=0:abort_on_error=0"
    environment["UBSAN_OPTIONS"] = "halt_on_error=1:print_stacktrace=1"

    def one(target):
        binary = os.path.join(FUZZ_BUILD_DIR, "fuzz", target)
        if not os.path.exists(binary):
            return target, None, binary

        corpus = os.path.join(FUZZ_DIR, "corpus", target)
        findings = os.path.join(FUZZ_DIR, "findings", target)
        os.makedirs(corpus, exist_ok=True)
        os.makedirs(findings, exist_ok=True)

        args = [
            binary,
            corpus,
            "-max_total_time=%d" % seconds,
            "-max_len=4096",
            # a single allocation past this is still caught, and the headroom
            # keeps a large corpus under the sanitizer from reading as a finding
            "-rss_limit_mb=4096",
            "-timeout=10",
            "-print_final_stats=1",
            "-artifact_prefix=%s%s" % (findings, os.sep),
        ]

        return target, subprocess.run(args, env=environment, capture_output=True, text=True), binary

    import concurrent.futures

    with concurrent.futures.ThreadPoolExecutor(max_workers=max(1, jobs)) as pool:
        runs = list(pool.map(one, targets))

    failed = 0
    for target, result, binary in runs:
        if result is None:
            fail("%s missing at %s" % (target, binary))
            failed += 1
            continue

        summary = [line for line in result.stderr.splitlines()
                   if line.startswith("stat::") or line.startswith("#") and "DONE" in line]

        if result.returncode != 0:
            fail("  fail %s" % target)
            # a whole run's log is thousands of lines; the report and the saved
            # input are at the end of it
            say("\n".join(result.stderr.splitlines()[-60:]))
            failed += 1
        else:
            say("  %sok%s   %s" % (GREEN, RESET, target))
            if verbose:
                for line in summary:
                    say("       %s%s%s" % (DIM, line, RESET))

    say("")
    if failed:
        fail("%d of %d harnesses found something; see tests/fuzz/findings"
             % (failed, len(targets)))
        return 1

    say("%s%d harnesses clean%s at %ds each" % (GREEN, len(targets), RESET, seconds))
    return 0


UNIT_SUMMARY = re.compile(r"(?:(\d+) failed, )?(\d+) passed, (\d+) total")
UNIT_TIMES = os.path.join(BUILD_DIR, "unit-suite-times.txt")
FEATURE_TIMES = os.path.join(BUILD_DIR, "feature-suite-times.txt")
SHADOW_BASE_FIRST = 2000
SHADOW_BASE_STEP = 500
SHADOW_PORTS = (22, 23, 80, 443)


def load_times(path):
    """Seconds each suite took on its last run, so the slowest can start first."""
    taken = {}
    if os.path.exists(path):
        with open(path) as times:
            for line in times:
                parts = line.split()
                if len(parts) == 2:
                    taken[parts[0]] = float(parts[1])
    return taken


def save_times(path, taken):
    """Keeps the suites this run did not touch, so a filtered run does not forget them."""
    merged = load_times(path)
    merged.update(taken)
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, "w") as times:
        for suite in sorted(merged):
            times.write("%s %.3f\n" % (suite, merged[suite]))


def run_unit(feature, verbose, list_only, jobs=1):
    binary = os.path.join(BUILD_DIR, "pdi_host_tests")
    if not os.path.exists(binary):
        fail("unit test binary missing at %s" % binary)
        return 1

    if jobs > 1 and not list_only and (not feature or "." not in feature):
        return run_unit_parallel(binary, feature, verbose, jobs)

    args = [binary]
    if list_only:
        args.append("--list")
    if feature:
        args += ["--filter", feature]
    if verbose:
        args.append("--verbose")

    return subprocess.run(args).returncode


def run_unit_parallel(binary, feature, verbose, jobs):
    """
    One process per suite, at most `jobs` at a time, each in a scratch working
    directory of its own, its tests in the order they are written. The suites
    that took longest last time start first, a suite's output is printed whole
    when it ends, and the totals are added up.
    """
    import concurrent.futures
    import tempfile

    listing = subprocess.run([binary, "--list"] + (["--filter", feature] if feature else []),
                             capture_output=True, text=True)
    suites = []
    for line in listing.stdout.splitlines():
        name = line.strip().split(".", 1)[0]
        if name and name not in suites:
            suites.append(name)
    if not suites:
        fail("no unit suite matches %r" % feature)
        return 1

    last = load_times(UNIT_TIMES)
    suites.sort(key=lambda s: -last.get(s, 0.0))

    say("%s%d suites over %d workers%s" % (DIM, len(suites), jobs, RESET))
    began = time.time()

    def one(suite):
        workdir = tempfile.mkdtemp(prefix="pdi-unit-%s-" % suite)
        started = time.time()
        try:
            result = subprocess.run([binary, "--filter", suite] + (["--verbose"] if verbose else []),
                                    capture_output=True, text=True, cwd=workdir)
        finally:
            shutil.rmtree(workdir, ignore_errors=True)
        return suite, result, time.time() - started

    totals = [0, 0, 0]
    failures = []
    taken = {}
    with concurrent.futures.ThreadPoolExecutor(max_workers=jobs) as pool:
        for future in concurrent.futures.as_completed([pool.submit(one, s) for s in suites]):
            suite, result, seconds = future.result()
            taken[suite] = seconds
            text = result.stdout + result.stderr
            found = UNIT_SUMMARY.findall(COLOUR.sub("", text))
            if found:
                failed, passed, total = found[-1]
                totals[0] += int(passed)
                totals[1] += int(failed or 0)
                totals[2] += int(total)
            if 0 != result.returncode or not found:
                failures.append(suite)
                sys.stdout.write(text)
            elif verbose:
                sys.stdout.write(text)

    save_times(UNIT_TIMES, taken)

    say("")
    say("%d passed, %d failed, %d total in %.1f s over %d workers"
        % (totals[0], totals[1], totals[2], time.time() - began, jobs))
    if failures:
        fail("unit suites with a failure or no result: %s" % ", ".join(sorted(failures)))
        return 1
    return 0


def run_system(feature, verbose, list_only):
    binary = os.path.join(BUILD_DIR, "pdid")
    if not os.path.exists(binary):
        fail("pdid missing at %s" % binary)
        return 1

    sys.path.insert(0, TESTS_DIR)
    from suite import system_smoke

    if list_only:
        for name, _ in system_smoke.CHECKS:
            say("system.%s" % name)
        return 0

    return system_smoke.run(binary, feature, verbose)


def read_uptime(shell, timeout):
    """
    The target's uptime in seconds, or None when it does not report one.

    Used to tell a run apart from a run the target restarted in the middle of.
    """
    import re

    try:
        out = shell.run("uptime", timeout)
    except Exception:
        return None

    found = re.search(r"(?:(\d+)d\s*)?(?:(\d+)h\s*)?(?:(\d+)m\s*)?(\d+)s", out)
    if not found:
        return None

    days, hours, minutes, seconds = (int(part or 0) for part in found.groups())
    return ((days * 24 + hours) * 60 + minutes) * 60 + seconds


def run_features(shell, feature, verbose, timeout, username, password, description,
                 peer_factory=None):
    """
    The transport-agnostic suites against one already opened target. The same
    function serves the host process and a board; only the shell differs.
    """
    from suite.driver.caps import Capabilities
    from suite.features import registry

    say("%sprobing %s%s" % (DIM, description, RESET))
    shell.attach(username, password, timeout=max(timeout, 30))

    caps = Capabilities.probe(shell)
    say("%s%s%s" % (DIM, caps.summary(), RESET))
    for note in caps.notes:
        say("%s%s%s" % (DIM, note, RESET))

    began_at = read_uptime(shell, timeout)

    _, failed, _ = registry.run(shell, caps, username, password,
                                only=feature, verbose=verbose, timeout=timeout,
                                peer_factory=peer_factory)

    # A target that restarted part way through invalidates everything after it,
    # and the failures it leaves behind read convincingly as framework defects:
    # a session pool that stops accepting, a client that never reconnects. Say
    # so plainly rather than leaving the tally to be believed.
    ended_at = read_uptime(shell, timeout)
    if began_at is not None and ended_at is not None and ended_at < began_at:
        say("")
        fail("the target restarted during this run: uptime went %ds -> %ds"
             % (began_at, ended_at))
        fail("these results are void; re-run once the target stays up")
        return 1

    if began_at is None or ended_at is None:
        say("")
        fail("the target's uptime could not be read, so a restart cannot be ruled out")

    return 1 if failed else 0


def run_device(spec, feature, verbose, timeout, username, password, key_filename,
               reset=False, input_delay=None):
    sys.path.insert(0, TESTS_DIR)
    from suite.driver.connect import connect, parse
    from suite.driver.shell import ShellError

    try:
        shell, description = connect(spec, username=username, password=password,
                                     key_filename=key_filename, reset=reset)
        if input_delay is not None:
            shell.input_delay = input_delay
    except ShellError as err:
        fail(str(err))
        return 1

    # a network transport can dial the target again; a serial cable cannot, and
    # the tests that need a second session skip rather than pretend
    scheme, _ = parse(spec)
    peer_factory = None
    if scheme in ("ssh", "telnet"):
        def peer_factory(peer_password=None):
            peer, _ = connect(spec, username=username,
                              password=peer_password or password,
                              key_filename=key_filename, reset=False)
            return peer

    try:
        return run_features(shell, feature, verbose, timeout, username, password, description,
                            peer_factory)
    except Exception as err:
        fail("device tier failed: %s" % err)
        return 1
    finally:
        shell.close()


def run_device_interleaved(specs, feature, verbose, timeout, username, password,
                           key_filename, reset=False, input_delay=None):
    """
    One pass of the suite spread across several transports at once, each test
    landing on the next in rotation while every session stays open.
    """
    sys.path.insert(0, TESTS_DIR)
    from suite.driver.connect import connect, parse
    from suite.driver.caps import Capabilities
    from suite.features import registry
    from suite.driver.shell import ShellError

    lanes = []
    shells = []

    def open_target(spec, probe=True, do_reset=None):
        scheme, _ = parse(spec)
        shell, description = connect(spec, username=username, password=password,
                                     key_filename=key_filename,
                                     reset=reset if do_reset is None else do_reset)
        if input_delay is not None:
            shell.input_delay = input_delay
        shells.append(shell)

        if probe:
            say("%sprobing %s%s" % (DIM, description, RESET))
        shell.attach(username, password, timeout=max(timeout, 30))
        caps = Capabilities.probe(shell)
        if probe:
            say("%s%s: %s%s" % (DIM, scheme, caps.summary(), RESET))

        # A transport that connected but named no commands did not answer help,
        # so it is a dead lane rather than a limited one. Carrying it makes the
        # run report the transport count it was asked for while a third of the
        # tests land on a shell that cannot run them.
        if not caps.commands:
            raise ShellError("%s connected but answered no commands, so it is "
                             "not a usable lane" % scheme)

        peer_factory = None
        if scheme in ("ssh", "telnet"):
            def peer_factory(peer_password=None, _spec=spec):
                peer, _ = connect(_spec, username=username,
                                  password=peer_password or password,
                                  key_filename=key_filename, reset=False)
                return peer

        return registry.Target(shell, caps, username, password, timeout, peer_factory)

    try:
        for spec in specs:
            scheme, _ = parse(spec)
            try:
                target = open_target(spec)
            except ShellError as err:
                fail("%s: %s" % (spec, err))
                continue

            lanes.append(registry.Lane(scheme, target,
                                       lambda _spec=spec: open_target(_spec, probe=False,
                                                                      do_reset=False)))

        if not lanes:
            fail("no transport could be opened")
            return 1

        began = [read_uptime(lane.target.shell, timeout) for lane in lanes]

        # Every lane drives the same board, so a command waits behind the other
        # lanes' work as well as its own. The per-command budget is a
        # single-transport figure; hold it fixed here and the mode's own
        # contention reads as a failure.
        if len(lanes) > 1:
            shared = timeout * len(lanes)
            for lane in lanes:
                lane.target.timeout = shared
            say("%sper-command budget %.0fs (%.0fs x %d lanes)%s"
                % (DIM, shared, timeout, len(lanes), RESET))

        say("%sinterleaving %d transports: %s%s"
            % (DIM, len(lanes), ", ".join(lane.label for lane in lanes), RESET))
        _, failed, _ = registry.run_interleaved(lanes, only=feature, verbose=verbose)

        # A lane that died takes the restart check with it, and a check that
        # cannot run is not a check that passed: say so rather than going quiet
        # exactly when a restart is most likely.
        confirmed = 0
        unreadable = []

        for lane, before in zip(lanes, began):
            label = lane.label
            after = read_uptime(lane.target.shell, timeout)

            if before is None or after is None:
                unreadable.append(label)
                continue

            if after < before:
                say("")
                fail("the target restarted during this run (seen on %s): %ds -> %ds"
                     % (label, before, after))
                fail("these results are void; re-run once the target stays up")
                return 1

            confirmed += 1

        if 0 == confirmed:
            say("")
            fail("no transport could be asked whether the target restarted (%s)"
                 % ", ".join(unreadable) if unreadable else "no lane answered")
            fail("a restart cannot be ruled out, so read this tally with that in mind")
        elif unreadable:
            say("%s%s could not be asked about a restart; %d transport(s) say it stayed up%s"
                % (DIM, ", ".join(unreadable), confirmed, RESET))

        return 1 if failed else 0
    finally:
        for shell in shells:
            try:
                shell.close()
            except Exception:
                pass


def board_name(shell, timeout):
    """The target's own name for itself, which keys its perf baselines."""
    try:
        out = shell.run("cat /etc/hostname", timeout).strip()
    except Exception:
        return None
    lines = [line.strip() for line in out.splitlines() if line.strip()]
    return lines[0] if lines and " " not in lines[0] else None


def run_perf_lanes(lanes, feature, verbose, timeout, record, board):
    """
    The perf tier over lanes already opened (label -> Target). A run the target
    restarted during is void: nothing is judged as final and nothing is stored.
    """
    from suite.perf import registry as perf_registry

    first = next(iter(lanes.values()))
    board = board or board_name(first.shell, timeout)
    if not board:
        fail("the target names no board in /etc/hostname; pass --perf-board")
        return 1

    say("%sperf on %s over %s%s" % (DIM, board, ", ".join(lanes), RESET))
    began = read_uptime(first.shell, timeout)
    ok, failed, skipped, commit = perf_registry.run(lanes, board, only=feature,
                                                    record=record, verbose=verbose)
    ended = read_uptime(first.shell, timeout)

    if began is not None and ended is not None and ended < began:
        fail("the target restarted during the perf run: %ds -> %ds; nothing was stored"
             % (began, ended))
        return 1

    commit()
    say("")
    say("%d measured, %d failed, %d skipped" % (ok + len(failed), len(failed), len(skipped)))
    if failed:
        fail("slower than baseline or failed to measure: %s" % ", ".join(failed))
        return 1
    return 0


def run_perf_device(specs, feature, verbose, timeout, username, password, key_filename,
                    record, board):
    sys.path.insert(0, TESTS_DIR)
    from suite.driver.connect import connect, parse
    from suite.driver.caps import Capabilities
    from suite.features import registry
    from suite.driver.shell import ShellError

    lanes = {}
    shells = []
    try:
        for spec in specs:
            scheme, _ = parse(spec)
            try:
                shell, description = connect(spec, username=username, password=password,
                                             key_filename=key_filename, reset=False)
                shells.append(shell)
                shell.attach(username, password, timeout=max(timeout, 30))
            except ShellError as err:
                fail("%s: %s" % (spec, err))
                continue
            caps = Capabilities.probe(shell)
            lanes[scheme] = registry.Target(shell, caps, username, password, timeout)

        if not lanes:
            fail("no transport could be opened")
            return 1
        return run_perf_lanes(lanes, feature, verbose, timeout, record, board)
    finally:
        for shell in shells:
            try:
                shell.close()
            except Exception:
                pass


def run_perf_host(feature, verbose, timeout, username, password, record, board):
    binary = os.path.join(BUILD_DIR, "pdid")
    if not os.path.exists(binary):
        fail("pdid missing at %s" % binary)
        return 1

    sys.path.insert(0, TESTS_DIR)
    from suite.driver.host_shell import HostShell
    from suite.driver.connect import connect
    from suite.driver.caps import Capabilities
    from suite.features import registry

    shell = HostShell(binary)
    others = []
    try:
        shell.attach(username, password, timeout=max(timeout, 30))
        caps = Capabilities.probe(shell)
        lanes = {"host": registry.Target(shell, caps, username, password, timeout)}

        telnet_port = shell.is_listening(23)
        ssh_port = shell.is_listening(22)
        for label, spec in (("telnet", "telnet:127.0.0.1:%d" % telnet_port if telnet_port else None),
                            ("ssh", "ssh:%s@127.0.0.1:%d" % (username, ssh_port) if ssh_port else None)):
            if spec is None:
                continue
            peer, _ = connect(spec, username=username, password=password, reset=False)
            others.append(peer)
            peer.attach(username, password, timeout=max(timeout, 30))
            lanes[label] = registry.Target(peer, Capabilities.probe(peer), username, password,
                                           timeout)

        return run_perf_lanes(lanes, feature, verbose, timeout, record, board or "host")
    finally:
        for peer in others:
            try:
                peer.close()
            except Exception:
                pass
        shell.close()


def run_system_features(feature, verbose, timeout, username, password):
    binary = os.path.join(BUILD_DIR, "pdid")
    if not os.path.exists(binary):
        fail("pdid missing at %s" % binary)
        return 1

    sys.path.insert(0, TESTS_DIR)
    from suite.driver.host_shell import HostShell

    from suite.driver.connect import connect

    def peer_factory(peer_password=None):
        peer, _ = connect("telnet:127.0.0.1:23", username=username,
                          password=peer_password or password, reset=False)
        return peer

    shell = HostShell(binary)
    try:
        return run_features(shell, feature, verbose, timeout, username, password, "pdid",
                            peer_factory)
    finally:
        shell.close()


# suites that multicast on a fixed group and port hear every other host
# process's traffic, so they run once nothing else is running
SOLO_SUITES = ("mdns",)

# the longest suites start first, so the run is not left waiting on one of them
SLOW_FIRST = ("timeouts", "shell", "mqtt", "scp", "gpio_live", "sessions", "script", "shell", "portal",
              "cron", "proc", "wget", "ssh", "sftp", "fs", "terminal")

SUMMARY = re.compile(r"(?:(\d+) failed, )?(\d+) passed, (\d+) skipped, (\d+) total")
COLOUR = re.compile(r"\x1b\[[0-9;]*m")


def feature_suites(feature):
    sys.path.insert(0, TESTS_DIR)
    from suite.features import registry

    names = sorted(set(item.feature for item in registry.discover()))
    if feature:
        names = [n for n in names if n == feature.split(".")[0] or feature in n]
    rank = dict((name, i) for i, name in enumerate(SLOW_FIRST))
    last = load_times(FEATURE_TIMES)
    return sorted(names, key=lambda n: (-last.get(n, 0.0), rank.get(n, len(SLOW_FIRST)), n))


def shadow_bases(count):
    """
    One shadow port base per worker, each with its four well known ports free.
    Bases sit below the kernel's ephemeral range, so a client socket elsewhere on
    the machine can never be holding a worker's port when its pdid comes up.
    """
    import socket

    bases = []
    base = SHADOW_BASE_FIRST
    while len(bases) < count and base + SHADOW_BASE_STEP < 32768:
        free = True
        for port in SHADOW_PORTS:
            probe = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            try:
                probe.bind(("127.0.0.1", base + port))
            except OSError:
                free = False
            finally:
                probe.close()
            if not free:
                break
        if free:
            bases.append(base)
        base += SHADOW_BASE_STEP
    return bases


def feature_jobs(suites):
    """
    What the parallel run hands its workers: one whole suite each, its tests in
    the order they are written, the suites that took longest last time first.
    Each job is (filter, label, expected seconds).
    """
    suite_times = load_times(FEATURE_TIMES)
    jobs = [(suite + ".", suite, suite_times.get(suite, 0.0)) for suite in suites]
    jobs.sort(key=lambda job: -job[2])
    return jobs


def run_system_features_parallel(feature, jobs, verbose, timeout, username, password):
    """
    One child run per suite, each with its own host process, scratch store and
    port range, at most `jobs` at a time, the longest of the last run first. A
    suite's tests run in the order they are written, and its output is printed
    whole when it ends, so two suites never interleave on the screen.
    """
    import subprocess
    import tempfile

    suites = feature_suites(feature)
    if not suites:
        fail("no feature suite matches %r" % feature)
        return 1

    queue = feature_jobs([s for s in suites if s not in SOLO_SUITES])
    solo = feature_jobs([s for s in suites if s in SOLO_SUITES])
    say("%s%d suites over %d workers%s" % (DIM, len(suites), jobs, RESET))

    bases = shadow_bases(jobs)
    if len(bases) < jobs:
        say("%sonly %d free shadow port ranges; running %d workers%s" % (DIM, len(bases), len(bases), RESET))
        jobs = len(bases)
    slots = [None] * jobs
    totals = [0, 0, 0, 0]
    failures = []
    taken = {}
    began = time.time()

    def start(slot, job):
        selector, label, _ = job
        log = tempfile.TemporaryFile(mode="w+")
        env = dict(os.environ)
        env["PDI_PORT_BASE"] = str(bases[slot])
        argv = [sys.executable, os.path.abspath(__file__), "--features-only",
                "--feature", selector, "--timeout", str(timeout),
                "--user", username, "--password", password]
        if verbose:
            argv.append("--verbose")
        proc = subprocess.Popen(argv, stdout=log, stderr=subprocess.STDOUT, env=env)
        slots[slot] = (job, proc, log, time.time())

    def finish(slot):
        job, proc, log, started = slots[slot]
        selector, label, _ = job
        slots[slot] = None
        log.seek(0)
        text = log.read()
        log.close()
        taken[label] = time.time() - started
        say("%s── %s (%.0f s)%s" % (DIM, label, taken[label], RESET))
        sys.stdout.write(text)
        found = SUMMARY.findall(COLOUR.sub("", text))
        if found:
            failed, passed, skipped, total = found[-1]
            totals[0] += int(passed)
            totals[1] += int(failed or 0)
            totals[2] += int(skipped)
            totals[3] += int(total)
        if 0 != proc.returncode or not found:
            failures.append(label)

    while queue or solo or any(slots):
        for slot in range(jobs):
            if slots[slot] is not None and slots[slot][1].poll() is not None:
                finish(slot)
        if queue:
            for slot in range(jobs):
                if slots[slot] is None and queue:
                    start(slot, queue.pop(0))
        elif solo and not any(slots):
            start(0, solo.pop(0))
        time.sleep(0.2)

    save_times(FEATURE_TIMES, taken)
    say("")
    say("%d passed, %d failed, %d skipped, %d total in %.0f s over %d workers"
        % (totals[0], totals[1], totals[2], totals[3], time.time() - began, jobs))
    if failures:
        fail("suites with a failure or no result: %s" % ", ".join(failures))
        return 1
    return 0


def main():
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--tier", choices=TIERS, action="append",
                        help="tier to run; repeatable, defaults to every available tier")
    parser.add_argument("--feature", help="run one suite, or one test as suite.name")
    parser.add_argument("--device", action="append",
                        help="target for the device tier, e.g. serial:/dev/ttyUSB0. Repeatable")
    parser.add_argument("--mode", choices=("sequential", "interleave"), default="sequential",
                        help="with several --device targets: sequential runs the whole suite over "
                             "each transport in turn; interleave runs it once, every session open "
                             "and each test taking the next transport in rotation")
    parser.add_argument("--list", action="store_true", help="list tests instead of running them")
    parser.add_argument("--verbose", action="store_true", help="print per test timing")
    parser.add_argument("--no-sanitize", action="store_true",
                        help="build without the address and UB sanitizers")
    parser.add_argument("--keep-going", action="store_true",
                        help="run every selected tier even after one fails")
    parser.add_argument("--user", default="pdiStack", help="login for the target")
    parser.add_argument("--password", default="pdiStack@123", help="password for the target")
    parser.add_argument("--key", help="private key file, for an ssh target")
    parser.add_argument("--timeout", type=float, default=20.0,
                        help="seconds to wait for one command to answer")
    parser.add_argument("--features", action="store_true",
                        help="also run the feature suites against the host process")
    parser.add_argument("--reset", action="store_true",
                        help="reset a serial target before testing; drops its network for a while")
    parser.add_argument("--input-delay", type=float, default=None,
                        help="seconds to pause before each send on a serial target")
    parser.add_argument("--fuzz-target", action="append", choices=FUZZ_TARGETS,
                        help="run one fuzz harness instead of all of them. Repeatable")
    parser.add_argument("--fuzz-seconds", type=int, default=60,
                        help="seconds to spend in each fuzz harness")
    parser.add_argument("--jobs", type=int, default=os.cpu_count() or 1,
                        help="host processes run at once for the unit suites, the feature suites "
                             "and the fuzz harnesses (default: every cpu; 1 runs them in turn)")
    parser.add_argument("--perf-record", action="store_true",
                        help="perf tier: take this run's numbers as the baselines")
    parser.add_argument("--perf-board",
                        help="perf tier: the name baselines are kept under, when the "
                             "target's /etc/hostname cannot be read")
    parser.add_argument("--features-only", action="store_true", help=argparse.SUPPRESS)
    args = parser.parse_args()

    if args.features_only:
        return run_system_features(args.feature, args.verbose, args.timeout, args.user, args.password)

    tiers = args.tier if args.tier else ["unit", "system"]
    if args.device and "device" not in tiers and "perf" not in tiers:
        tiers.append("device")
    if args.fuzz_target and "fuzz" not in tiers:
        tiers.append("fuzz")

    failures = []
    built = False

    if "unit" in tiers or "system" in tiers:
        if not args.list:
            say("%sbuilding host tests%s" % (DIM, RESET))
        if not configure(not args.no_sanitize):
            return 1
        if not build():
            return 1
        built = True

    if "unit" in tiers and built:
        code = run_unit(args.feature, args.verbose, args.list, args.jobs)
        if code != 0:
            failures.append("unit")
            if not args.keep_going:
                return code

    if "system" in tiers and built:
        if not args.list:
            say("")
            say("%ssystem%s" % (DIM, RESET))
        code = run_system(args.feature, args.verbose, args.list)
        if code != 0:
            failures.append("system")
            if not args.keep_going:
                return code

        if args.features and not args.list:
            say("")
            say("%ssystem features%s" % (DIM, RESET))
            if args.jobs > 1:
                code = run_system_features_parallel(args.feature, args.jobs, args.verbose,
                                                    args.timeout, args.user, args.password)
            else:
                code = run_system_features(args.feature, args.verbose, args.timeout,
                                           args.user, args.password)
            if code != 0:
                failures.append("system features")
                if not args.keep_going:
                    return code

    if "device" in tiers:
        if not args.device:
            fail("the device tier needs --device, e.g. --device serial:/dev/ttyUSB0")
            return 1

        if args.list:
            sys.path.insert(0, TESTS_DIR)
            from suite.features import registry
            for item in registry.discover():
                say("device.%s" % item.full_name)
        else:
            say("")
            say("%sdevice%s" % (DIM, RESET))
            if args.mode == "interleave" and len(args.device) > 1:
                code = run_device_interleaved(args.device, args.feature, args.verbose,
                                              args.timeout, args.user, args.password,
                                              args.key, args.reset, args.input_delay)
            else:
                code = 0
                for spec in args.device:
                    if len(args.device) > 1:
                        say("")
                        say("%s%s%s" % (DIM, spec, RESET))
                    one = run_device(spec, args.feature, args.verbose, args.timeout,
                                     args.user, args.password, args.key, args.reset,
                                     args.input_delay)
                    if one != 0:
                        code = one
                        if not args.keep_going:
                            break
            if code != 0:
                failures.append("device")
                if not args.keep_going:
                    return code

    if "perf" in tiers:
        if args.list:
            sys.path.insert(0, TESTS_DIR)
            from suite.perf import registry as perf_registry
            for item in perf_registry.discover():
                say("perf.%s" % item.full_name)
        else:
            say("")
            say("%sperf%s" % (DIM, RESET))
            if args.device:
                code = run_perf_device(args.device, args.feature, args.verbose, args.timeout,
                                       args.user, args.password, args.key, args.perf_record,
                                       args.perf_board)
            else:
                if not built:
                    if not configure(not args.no_sanitize) or not build():
                        return 1
                    built = True
                code = run_perf_host(args.feature, args.verbose, args.timeout, args.user,
                                     args.password, args.perf_record, args.perf_board)
            if code != 0:
                failures.append("perf")
                if not args.keep_going:
                    return code

    if "fuzz" in tiers:
        if not args.list:
            say("")
            say("%sfuzz%s" % (DIM, RESET))
        code = run_fuzz(args.fuzz_target if args.fuzz_target else list(FUZZ_TARGETS),
                        args.fuzz_seconds, args.verbose, args.list, args.jobs)
        if code != 0:
            failures.append("fuzz")
            if not args.keep_going:
                return code

    if failures:
        fail("failing tiers: %s" % ", ".join(failures))
        return 1

    return 0


def _stop_on_term(signum, frame):
    """A kill stops the run the way Ctrl+C does, so suites still put a board back."""
    raise KeyboardInterrupt()


if __name__ == "__main__":
    signal.signal(signal.SIGTERM, _stop_on_term)
    try:
        sys.exit(main())
    except KeyboardInterrupt:
        say("")
        fail("aborted by keyboard interrupt")
        sys.exit(130)
