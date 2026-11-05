# 17. Test Suite

Everything runs from one command:

```
python3 tests/run_tests.py
```

Exit status is zero only when every selected test passed. `tests/README.md` is the full reference;
this section is the shape of it.

## 17.1 Four tiers

| Tier | What it is | Needs |
|---|---|---|
| `unit` | Native binary linking the framework against the host port | a compiler |
| `system` | The whole stack as a host process (`pdid`) you can ssh, sftp and curl | a compiler |
| `device` | The same feature suites over serial, telnet or ssh | a board |
| `fuzz` | libFuzzer harnesses over the parsers that face the network before login | clang |

```
python3 tests/run_tests.py --tier unit
python3 tests/run_tests.py --tier system --features
python3 tests/run_tests.py --device ssh:pdiStack@<ip> --password <pw>
python3 tests/run_tests.py --tier fuzz --fuzz-seconds 60
```

The host tiers use every cpu. Each unit suite, each feature suite and each fuzz harness runs as a
process of its own, the slowest suites of the previous run first, and `scripts/SyntaxSweep.py`
compiles its files side by side. `--jobs N` caps how many run at once; `--jobs 1` runs them in turn,
which is the way to read one failure on its own. A suite that passes only after another has run is
a defect in that suite, and running each one alone is what shows it.

The unit tier asserts on functions and runs under Address and UB sanitizers, so an out-of-bounds
read fails the run rather than passing quietly. The middle two drive the framework the way a user
does — a shell, an ssh channel, the web portal, an MQTT broker. The fuzz tier feeds the ssh wire,
sftp, http, shell, database-record and config parsers whatever a stranger could send before they
have authenticated.

With several `--device` targets, `--mode interleave` opens all of them at once and gives each test
the next transport in rotation. It is roughly three times faster than running the suite per
transport, and it is the only mode where the transports contend for the board, which is where
session and pool defects show up.

## 17.2 How the host build works

`src/` contains no Arduino or vendor SDK includes; every SDK dependency lives under `devices/`. The
test build compiles the real framework sources against the host port in `devices/posix/` with
`MOCK_DEVICE_TEST` defined on the command line. That gate is the only thing selecting the host
port — `devices/DeviceSetup.h`, which records the board you build firmware for, is never read or
written by a test run, so testing never disturbs your build.

`scripts/SyntaxSweep.py` compiles every translation unit on its own, across the C++ standards the
boards use and across feature-flag profiles with services switched off. It is what catches a file
that only ever compiled because something else happened to include a header first, and a guard that
was never exercised because no board turns that combination on.

## 17.3 Feature suites

`tests/suite/features/` holds one file per area: terminal, shell grammar, filesystem, generated
filesystems, users, sessions, processes, networking, name resolution, portal, ssh, sftp, scp,
timeouts, mDNS and MQTT, plus two that cut across them: `cli_contract`, one test per row of the
command contract (unknown options, extra operands, numbers at their limits, failures named in words),
and `concurrent`, which runs a large sftp upload and download while the shell, a second terminal and
the portal keep being asked for things, checking every answer, the md5 and that the target did not
restart. Each is written against a `Target` rather than a transport, so one source
runs against the host process and against a board over any of the three transports.

Two rules keep the results honest. A capability difference is found by **attempting** the thing and
skipping with the reason, never by branching on the transport's name. And a test that needs a peer
service brings its own — the suite ships a small MQTT broker and a multicast DNS client, each bound
to a port of its own, because pointing a test at a broker already running on the developer's machine
tests the machine rather than the board.

State that outlives a run, such as the hosts file or the MQTT config in the device database, is read
first and written back afterwards, so a run leaves the board as it found it.

## 17.4 Adding a test

A unit test is a `.cpp` file dropped in `tests/host/unit/`; CMake globs the directory.

```cpp
TEST(stringops, strtrim_removes_surrounding_spaces)
{
    char buf[32];
    strcpy(buf, "   padded   ");
    ASSERT_STREQ(__strtrim(buf), "padded");
}
```

A feature test is a decorated function in `tests/suite/features/`:

```python
@test("the shell reports the current directory", needs=("pwd", "cd"))
def pwd_follows_cd(t):
    t.run("cd /etc")
    expect_in("/etc", t.run("pwd"), "pwd after cd")
```

`needs`, `mounts` and `services` declare what the target must have; the runner skips with a reason
when it does not.

A test that writes files does it inside `t.workspace("wt_name")`: a directory of its own, emptied
first and removed when the test ends, pass or fail. If the directory cannot be made — on a full
flash, say — the test fails there instead of writing into `/`. A file the framework only reads from
one fixed place, such as `/.ssh/authorized_keys`, goes in a directory made with
`t.make_dir("/.ssh")`: when the test ends that directory is removed with everything in it if the
test made it, and left alone if it was already there. Anything a test still leaves at the
top of `/` is removed when the run ends; the runner lists `/` before the first test and takes away
only names that were not there then, never the framework's own (`etc`, `var`, `.ssh`, `.env` and
the like).
