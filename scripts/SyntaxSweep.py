#!/usr/bin/env python3

"""
Compile every framework translation unit with -fsyntax-only under one C++
standard and one feature profile, against the mock device.

Two kinds of drift are checked here, and they are separate axes.

Standard: the three ports do not agree. arduinouno is gnu++11, esp8266 is
gnu++17 and esp32 is gnu++2a. Shared headers therefore have to hold at the
gnu++11 floor, and nothing in the normal build enforces that -- the host test
build compiles at C++17 and every board build sees only its own standard.

Profile: a code path behind #ifdef ENABLE_X only ever compiles when X is on. The
stock mock device turns every root service on, so a path that breaks with
storage or network absent goes unnoticed until a board without them is built.
The mock device config guards its four root flags plus the sealing capability
with PDI_NO_*, which is what lets a profile switch one off from the command
line.

With --port posix it also compiles devices/posix itself. Nothing here compiles the esp32,
esp8266 or arduinouno device layers -- that needs their real toolchains.
"""

import argparse
import concurrent.futures
import os
import subprocess
import sys


ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SRC = os.path.join(ROOT, "src")
POSIX_DEVICE = os.path.join(ROOT, "devices", "posix")

# which device layer to compile against. "mock" is the port the host tests use;
# "posix" is the same port without its test scaffolding, and is the only real
# port a syntax sweep can reach -- the others need their vendor sdk headers.
PORTS = {
    "mock": "MOCK_DEVICE_TEST",
    "posix": "PDI_DEVICE=posix",
}

STANDARDS = ("gnu++11", "gnu++17", "gnu++2a")

# each profile names the root flags it switches OFF. "full" is the stock mock
# device, which is what the standards matrix sweeps.
PROFILES = {
    "full": [],
    "no-network": ["PDI_NO_NETWORK_SERVICE"],
    "no-storage": ["PDI_NO_STORAGE_SERVICE"],
    "no-auth": ["PDI_NO_AUTH_SERVICE"],
    "no-cmd": ["PDI_NO_CMD_SERVICE"],
    "no-seal": ["PDI_NO_DB_SEALING"],
    "no-featureconf": ["PDI_NO_FEATURE_CONFIG_FILES"],
    "no-script": ["PDI_NO_SCRIPT_RUNNER"],
    "minimal": [
        "PDI_NO_NETWORK_SERVICE",
        "PDI_NO_STORAGE_SERVICE",
        "PDI_NO_AUTH_SERVICE",
        "PDI_NO_CMD_SERVICE",
        "PDI_NO_DB_SEALING",
        "PDI_NO_SCRIPT_RUNNER",
    ],
}

WARN_FLAGS = ["-Wall", "-Wno-unused-parameter", "-Wno-unused-variable"]

GREEN = "\033[32m"
RED = "\033[31m"
DIM = "\033[2m"
RESET = "\033[0m"


def say(message):
    print(message, flush=True)


def sources(excludes, port="mock"):
    found = []
    roots = [SRC]
    if "posix" == port:
        # the device layer itself is only reachable when it is the selected port
        roots.append(POSIX_DEVICE)
    for root in roots:
      for base, _dirs, files in os.walk(root):
        for name in files:
            if not name.endswith(".cpp"):
                continue
            path = os.path.join(base, name)
            rel = os.path.relpath(path, ROOT)
            if any(part in rel for part in excludes):
                continue
            found.append(rel)
    found.sort()
    return found


def check(compiler, std, profile, rel, failures, port="mock"):
    args = [
        compiler,
        "-fsyntax-only",
        "-std=%s" % std,
        "-D%s" % PORTS[port],
    ] + ["-D%s" % macro for macro in PROFILES[profile]] + [
        "-I", ROOT,
        "-I", os.path.join(ROOT, "src"),
    ] + WARN_FLAGS + [os.path.join(ROOT, rel)]

    result = subprocess.run(args, capture_output=True, text=True)
    if result.returncode != 0:
        failures.append((rel, result.stderr.strip()))
        return False
    return True


def main():
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--std", action="append", choices=STANDARDS,
                        help="standard to sweep, repeatable; defaults to all three")
    parser.add_argument("--profile", action="append", choices=sorted(PROFILES),
                        help="feature profile to sweep, repeatable; defaults to full")
    parser.add_argument("--port", action="append", choices=sorted(PORTS),
                        help="device layer to compile against; repeatable, defaults to mock")
    parser.add_argument("--compiler", default=os.environ.get("CXX", "g++"),
                        help="compiler to invoke (default: $CXX or g++)")
    parser.add_argument("--exclude", action="append", default=[],
                        help="skip any path containing this substring, repeatable")
    parser.add_argument("--quiet", action="store_true",
                        help="report only the failures and the totals")
    parser.add_argument("--jobs", type=int, default=os.cpu_count() or 1,
                        help="files compiled at once (default: every cpu)")
    args = parser.parse_args()

    standards = args.std if args.std else list(STANDARDS)
    profiles = args.profile if args.profile else ["full"]
    ports = args.port if args.port else ["mock"]

    say("%s%d standard(s), %d profile(s), %d port(s), compiler %s%s"
        % (DIM, len(standards), len(profiles), len(ports), args.compiler, RESET))

    worst = 0
    for port in ports:
      files = sources(args.exclude, port)
      if not files:
        say("%sno sources found for port %s%s" % (RED, port, RESET))
        return 1

      for std in standards:
        for profile in profiles:
            leg = "%s/%s/%s" % (port, std, profile)
            failures = []
            with concurrent.futures.ThreadPoolExecutor(max_workers=max(1, args.jobs)) as pool:
                results = pool.map(lambda rel: (rel, check(args.compiler, std, profile, rel, failures, port)),
                                   files)
                for rel, ok in results:
                    if not ok and not args.quiet:
                        say("%sFAIL%s %s [%s]" % (RED, RESET, rel, leg))
            failures.sort()

            if failures:
                worst = 1
                say("%s%s: %d of %d failed%s" % (RED, leg, len(failures), len(files), RESET))
                for rel, err in failures:
                    say("")
                    say("%s--- %s [%s]%s" % (DIM, rel, leg, RESET))
                    say(err)
            else:
                say("%s%s: %d of %d clean%s" % (GREEN, leg, len(files), len(files), RESET))

    return worst


if __name__ == "__main__":
    sys.exit(main())
