# Fuzz tier

Eleven libFuzzer harnesses over the parsers that read bytes nobody on this device
chose. Most of them are reached **before any credential is checked** — everything
else in the framework is guarded by a login, so a length field trusted there is
trusted from an anonymous stranger. The rest read a reply, a stored table or a
stored file, where the bytes belong to whichever server was dialled or to
whoever last wrote the file.

```
python3 tests/run_tests.py --tier fuzz                       # 60s per harness
python3 tests/run_tests.py --tier fuzz --fuzz-seconds 600     # a longer soak
python3 tests/run_tests.py --tier fuzz --fuzz-target fuzz_ssh_wire --verbose
```

Needs **clang**. libFuzzer ships with the clang runtime and gcc has no
equivalent, so the tier configures a build directory of its own at
`tests/.build-fuzz` and rebuilds the framework there with coverage
instrumentation. The ordinary tiers keep using gcc and `tests/.build`.

## The harnesses

| Harness | Reaches | Why it is here |
|---|---|---|
| `fuzz_ssh_wire` | packet framing, KEXINIT, ECDH init, userauth, channel request and data, the length-prefixed field readers | every byte of this runs before the password is looked at |
| `fuzz_sftp` | channel-data reassembly, the request header and every string field, split across chunks | where AO lived, and where BB and BC were found |
| `fuzz_http` | request line, headers, query string, urlencoded form, multipart upload | the portal's front door, before the session cookie |
| `fuzz_http_response` | the reply reader: status line, header split, chunked decoder, streamed body, and the url parser ahead of them | the other direction — `wget`, ota and iot all read a reply from a server they did not write, and CV lived here |
| `fuzz_shell` | the terminal reader every transport shares: escape decoding, line editing, the login prompt | telnet hands it raw socket bytes |
| `fuzz_dbrecord` | the superblock and directory a mount believes | a bad sector or another firmware's image arrives looking like this |
| `fuzz_config` | the `/etc` key/value reader | an interrupted write leaves a file this reader still opens |
| `fuzz_crontab` | the `/etc/crontab` row split and the field grammar — stars, numbers, ranges, steps | the table is re-read every minute and what it names runs with the scheduler's privileges |
| `fuzz_mdns` | the responder's query reader — header, question count, labels, type — and the reply it builds in a fixed buffer | any host on the network multicasts to it, with no login |
| `fuzz_mqtt` | the mqtt client reading a broker: fixed header, remaining length, every packet type it dispatches, topic and payload into the data callback | whoever runs the broker, or sits between it and the board, picks these bytes — one malformed PUBLISH crashed the host process (DU) |
| `fuzz_json` | the json value reader — the key matched as a whole string token, the value copied into a fixed buffer — and the escaper | every remote payload goes through it: broker commands, the iot server's configuration, the published firmware version |

## Corpus and findings

Seeds are **generated, not checked in** — `MakeCorpus.py` writes one well formed
input per branch, so a run starts inside the parser rather than at its first
length check. Binary seeds written by hand go stale the moment a header moves.
Both `corpus/` and `findings/` are ignored by git; the corpus grows in place as
libFuzzer finds new coverage, and anything that crashes, hangs or runs the heap
away is saved under `findings/<harness>/`.

Reproduce one:

```
tests/.build-fuzz/fuzz/fuzz_ssh_wire tests/fuzz/findings/fuzz_ssh_wire/crash-<hash>
```

## Watch the coverage number, not the exit status

A harness that reaches nothing passes every run. `fuzz_http` first reported
**cov: 94** at 191k exec/s — `parseRequest` returns immediately when `m_server`
is null, so 17 million executions tested one early return. With a listener
handed to it, the same harness reports **cov: 3884**. Run a new harness with
`--verbose` once and look at `cov:` before believing a clean result.

## A finding left in `findings/` is not a live defect

The directory is gitignored and persists across runs, so an artefact there was true
of whatever build wrote it. `fuzz_sftp` carried a crash file from the BB/BC era for
two weeks that exits 0 against the current binary. **Replay it before believing it**,
and delete it once it is dead — otherwise every future run looks like it failed.

## The client models a peer that hung up

`FuzzClient::connected()` goes false once the input is drained, because a fuzz
input *is* a peer that sent N bytes and closed. Readers that wait for more then
leave by the connection-closed branch instead of sitting on a deadline. The
harnesses also put the device on its virtual clock (`useFreeClock()`), so a
`wait()` costs no wall clock.

Those two work together, and only together. On the virtual clock alone the
multipart drain loop spun forever: its escape is `!connected() || now -
lastprogress > STALL_TIMEOUT`, the peer never disconnected, and virtual time
only moves when something calls `wait()` — which that loop does not.

## Two more things to know before adding a harness

**Sessions own their client.** `LWSSHSession` closes and deletes the client it
was given, so a harness must hand it a heap allocation, never a stack object.
The same goes for the http *server* reader. `Http_Client` is the exception — its
`End()` only nulls the pointer and never deletes, so `fuzz_http_response` hands it
a stack object on purpose.

**The host is 64 bit and the boards are not.** `size_t` is 8 bytes here and 4
there, so an overflow that needs a 32 bit `size_t` will never fire under the
fuzzer — `parse_channel_request` and `parse_channel_data_request` both had one,
and both had to be found by reading. A clean fuzz run is evidence about the
parser's logic, not about its arithmetic on a device.
