# PDI Framework Tests

Everything runs from one command:

```
python3 tests/run_tests.py
```

Exit status is zero only when every selected test passed.

## Common invocations

```
python3 tests/run_tests.py                    # unit and system tiers
python3 tests/run_tests.py --list             # list every test as suite.name
python3 tests/run_tests.py --feature sha256   # one suite
python3 tests/run_tests.py --feature sha256.hashes_abc
python3 tests/run_tests.py --verbose          # per test timing
python3 tests/run_tests.py --no-sanitize      # drop ASan and UBSan

python3 tests/run_tests.py --tier system --features        # feature suites vs the host process
python3 tests/run_tests.py --device ssh:pdiStack@<ip>   --password <pw>
python3 tests/run_tests.py --device telnet:<ip>         --password <pw>
python3 tests/run_tests.py --device serial:/dev/ttyUSB0 --password <pw>
```

`--tier` is repeatable and defaults to `unit` and `system`; naming a `--device` adds the device
tier. `--user`, `--key`, `--timeout` and `--keep-going` do what they sound like. `--reset` power
cycles a serial target before testing, which drops its network for a while, so it is off by
default.

### Several transports at once

`--device` is repeatable, and `--mode` decides what that means:

```
python3 tests/run_tests.py --tier device --mode interleave \
    --device serial:/dev/ttyUSB1 --device ssh:<ip> --device telnet:<ip> --password <pw>
```

| mode | what it does |
|---|---|
| `sequential` (default) | the whole suite over each transport in turn — every assertion runs once per transport |
| `interleave` | one pass of the suite, every session open at once, each test taking the next transport in rotation |

`interleave` opens all the sessions and keeps them open for the whole run, then hands test 1 to the
first transport, test 2 to the second and so on. Tests still execute **one at a time**: they share
the board's mqtt config, hosts file and user store, so two at once would corrupt each other rather
than test anything. Each line says where it ran:

```
ok   meta.pwd starts at the root [ssh]
ok   meta.cd moves into a directory [telnet]
ok   meta.cd into a missing directory leaves us where we were [serial]
```

It is roughly as many times faster as there are transports, because each assertion runs once rather
than once each. Two details make it behave:

- **A test the assigned transport cannot do is offered to the others before it is called a skip**, so
  the session tests still run when the rotation hands them to a serial cable — they move to ssh.
- **A transport that dies drops out of the rotation** (`serial is gone; 2 transport(s) left`) and the
  remaining tests redistribute, rather than the run ending.

The trade is redundancy for time: an assertion runs on one transport, so a fault specific to another
can hide. Use it for a fast confidence pass; use `sequential` before a release.

Open a serial target **first** when mixing it with network ones — opening the port can reset the
board, and it is better that happens before the ssh and telnet sessions attach than under them.

Both modes read the target's `uptime` before and after and declare the run **void** if it went
backwards. A board that restarts mid-run leaves failures that read convincingly as framework
defects — a session pool that stops accepting, a client that never reconnects — so this is checked
rather than trusted.

## How the host build works

`src/` contains no Arduino or vendor SDK includes — every SDK dependency lives under `devices/`.
The test build therefore compiles the real framework sources against the mock device adapter in
`devices/mockdevice/`, with `MOCK_DEVICE_TEST` defined on the compiler command line. That one gate is
the only thing that selects the mock device; `devices/DeviceSetup.h`, which records the board you
build firmware for, is never read or written by a test run.

The build is a normal CMake project in `tests/host/`, and it puts its artifacts in `tests/.build/`
(git-ignored). Address and UB sanitizers are on by default, so out-of-bounds accesses, overlapping
copies and undefined shifts fail the run rather than passing quietly.

## Tiers

| Tier | What it is |
|---|---|
| `unit` | Native host binary linking the framework against the mock device |
| `system` | The whole stack running as a host process (`pdid`) you can ssh, sftp and curl |
| `device` | The same feature suites over serial, telnet or ssh against real hardware, one transport at a time or all of them interleaved |

The unit tier asserts on functions. The other two drive the framework the way a user does — through
a shell, an ssh channel, the web portal, an mqtt broker — and the same feature suites run against
both, so the host process catches most of it in seconds and the board confirms it for real.

### Feature suites

`tests/suite/features/` holds one file per area: terminal, filesystem, users, sessions, processes,
networking, name resolution, the web portal, the ssh server, sftp, mDNS and mqtt. They are written
against a `Target` (`registry.py`) rather than a transport, so one source runs everywhere.

Where a target genuinely cannot do something, the test **skips with the reason**. A capability
difference is found by *attempting* the thing, never by branching on the transport's name — a test
that asks "am I on serial?" stops testing the target and starts testing the harness. `--mode
interleave` keeps that honest: any test can land on any transport, so one that quietly depended on
a particular one fails the moment the rotation moves it.

Tests that need a service to talk to bring their own: `mqtt_broker.py` is a small MQTT 3.1.1 broker
and `mdns.py` a multicast DNS client, both dependency-free and both bound to a port of their own.
Pointing a test at a broker or responder that already runs on the developer's machine tests the
machine, not the board.

State that outlives a run — the hosts file, the mqtt config in the device database — is read first
and written back through `Target.at_exit`, so a run leaves the board as it found it.

## Adding a test

For the unit tier, drop a `.cpp` file in `tests/host/unit/`. CMake globs the directory, so there is
nothing to register.

```cpp
#include <pditest.h>
#include <utility/StringOperations.h>

TEST(stringops, strtrim_removes_surrounding_spaces)
{
    char buf[32];
    strcpy(buf, "   padded   ");
    ASSERT_STREQ(__strtrim(buf), "padded");
}
```

The first argument to `TEST` is the suite, which is what `--feature` filters on. Group tests for one
module under one suite name.

## Assertions

| Macro | Checks |
|---|---|
| `ASSERT_TRUE(c)` / `ASSERT_FALSE(c)` | a condition |
| `ASSERT_EQ` `ASSERT_NE` `ASSERT_LT` `ASSERT_LE` `ASSERT_GT` `ASSERT_GE` | a comparison, printing both values |
| `ASSERT_STREQ` / `ASSERT_STRNE` | C strings, null safe |
| `ASSERT_MEMEQ(a, b, len)` | a byte range, printing both as hex |
| `ASSERT_NEAR(a, b, tol)` | a float within a tolerance |
| `ASSERT_NULL` / `ASSERT_NOT_NULL` | a pointer |
| `FAIL(reason)` | fails outright |

Each one reports file, line, the expression and the two values, then ends that test and moves to the
next. A failing test never stops the rest of the run.

## Writing tests that mean something

Read the implementation before asserting on it. Several suites here started out asserting a
plausible contract rather than the real one — `Uint32ToString`'s pad argument right-pads with spaces
rather than zero-filling, and `regex_match` answers with the index of the match rather than a
boolean, so a match at position zero reads as false. A test that encodes a guess is worse than no
test, because it fails against correct code.

For a feature suite, add a function to a file in `tests/suite/features/` and decorate it:

```python
@test("the shell reports the current directory", needs=("pwd", "cd"))
def pwd_follows_cd(t):
    t.run("cd /etc")
    expect_in("/etc", t.run("pwd"), "pwd after cd")
```

`needs`, `mounts` and `services` are capability requirements — the runner skips the test with a
reason when the target lacks them, rather than failing it.
