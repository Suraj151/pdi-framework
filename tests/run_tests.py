#!/usr/bin/env python3

"""
Run the pdi framework test suite.

Tiers
  unit    native host build of the framework against the mock device
  system  the whole stack running as a host process, driven through its shell
  device  the same feature suite over serial or ssh against real hardware (pending)
"""

import argparse
import os
import shutil
import subprocess
import sys

TESTS_DIR = os.path.dirname(os.path.abspath(__file__))
HOST_DIR = os.path.join(TESTS_DIR, "host")
BUILD_DIR = os.path.join(TESTS_DIR, ".build")

TIERS = ("unit", "system", "device")

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


def run_unit(feature, verbose, list_only):
    binary = os.path.join(BUILD_DIR, "pdi_host_tests")
    if not os.path.exists(binary):
        fail("unit test binary missing at %s" % binary)
        return 1

    args = [binary]
    if list_only:
        args.append("--list")
    if feature:
        args += ["--filter", feature]
    if verbose:
        args.append("--verbose")

    return subprocess.run(args).returncode


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

        say("%sinterleaving %d transports: %s%s"
            % (DIM, len(lanes), ", ".join(lane.label for lane in lanes), RESET))
        _, failed, _ = registry.run_interleaved(lanes, only=feature, verbose=verbose)

        for lane, before in zip(lanes, began):
            label = lane.label
            after = read_uptime(lane.target.shell, timeout)
            if before is not None and after is not None and after < before:
                say("")
                fail("the target restarted during this run (seen on %s): %ds -> %ds"
                     % (label, before, after))
                fail("these results are void; re-run once the target stays up")
                return 1

        return 1 if failed else 0
    finally:
        for shell in shells:
            try:
                shell.close()
            except Exception:
                pass


def run_system_features(feature, verbose, timeout, username, password):
    binary = os.path.join(BUILD_DIR, "pdid")
    if not os.path.exists(binary):
        fail("pdid missing at %s" % binary)
        return 1

    sys.path.insert(0, TESTS_DIR)
    from suite.driver.host_shell import HostShell

    shell = HostShell(binary)
    try:
        return run_features(shell, feature, verbose, timeout, username, password, "pdid")
    finally:
        shell.close()


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
    args = parser.parse_args()

    tiers = args.tier if args.tier else ["unit", "system"]
    if args.device and "device" not in tiers:
        tiers.append("device")

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
        code = run_unit(args.feature, args.verbose, args.list)
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

    if failures:
        fail("failing tiers: %s" % ", ".join(failures))
        return 1

    return 0


if __name__ == "__main__":
    try:
        sys.exit(main())
    except KeyboardInterrupt:
        say("")
        fail("aborted by keyboard interrupt")
        sys.exit(130)
