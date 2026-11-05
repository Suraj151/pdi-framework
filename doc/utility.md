# 15. Utility Library

[src/utility/](../src/utility/) is the foundation: small, dependency-light primitives every layer above uses. Anything here may include the abstract interfaces, but never a device header — the same source file compiles under every port with no conditionals.

Most of these have already appeared in passing. This is the index.

| Component | What it is |
|---|---|
| interface foundations | the three abstract bases the framework rests on: stream I/O, utility, instance factory |
| type definitions | `task_t`, task states and signals, `session_t`, callback aliases, input sequences, addresses |
| database engine | [§5](database.md#5-database-layer) |
| task scheduler | [§4](scheduler.md#4-task-scheduler) |
| command base | [§7](shell.md#7-command-line--terminal) |
| event bus | cross-service publish and subscribe |
| string operations | bounded C-string helpers for fixed-size NVM data |
| data conversions | integer, hex and BCD conversions with no `printf` |
| Base64 | encode, decode, and a unique-key generator |
| regex | a minimal engine behind `grep`, and a check that a pattern can be matched as written |
| error text | the words a user is shown for an error code |
| safe alloc | heap-checked allocation that refuses to breach a margin |
| queues | a byte ring, a record queue, and a length-prefixed parser |
| crypto | hashes, HMAC, AES, Curve25519, Ed25519, RSA |
| PdiSTL | a trimmed standard library for constrained targets |
| umbrella header | one include that pulls the whole foundation in |

## 15.1 Event bus

`__utl_event` is a synchronous publisher. Services add listeners at boot and fire events from state changes, without any of them taking a direct dependency on another. Subscribing returns an id and `remove_event_listener(id)` gives it back, which is what lets a service that listens rather than scheduling a task be stopped and restarted without leaving listeners behind. Event names are centralised, and the usage patterns are in [§6.4](services.md#64-the-event-bus).

## 15.2 String operations

Bounded, `printf`-free helpers — substring search, trim, compare, find-and-replace, a small JSON field extractor, IPv4 conversions, case folding. Every one takes an explicit length bound, defaulting to 300, because the NVM config strings are fixed-size arrays that are often not null-terminated. Keep the bound.

## 15.3 Data conversions

Integer, hex and BCD conversions in both directions with no `stdio` dependency. Using these instead of libc's formatter is worth four to eight kilobytes of flash on the smaller targets.

Two kinds of reader sit side by side. `StringTo*` read as much of a number as the text starts with, for text already known to hold one, and hold at their type's limits rather than wrapping. `ParseUint32`, `ParseInt64` and `ParseOctal` (with a largest value) take only text that is one whole number that fits, and say whether it was, which is what anything a user typed goes through. `BytesToSizeString` writes a size in bytes or in K to E, and `FilePermsToString` the ten character `ls -l` mode, its first letter from the entry's type.

`ErrorToText(code)` gives the words for a framework, storage, littlefs or FAT code — `File exists`, `Directory not empty`, `No such file or directory` — or null for a code that has none yet, so a message can fall back to the number.

## 15.4 Base64

Encode and decode, plus a unique-key generator used for curve25519 private keys, SSH keygen seeds and SFTP file handles. Web session and CSRF tokens do not come from here — they are drawn from the platform random source behind `random_now()`, which is the hardware RNG on both ESP ports.

## 15.5 Queues

Three layers, useful when byte-level discipline matters:

```
  RINGBUF        a byte ring — put, get, init over a caller-supplied buffer
      │
  QUEUE          length-tagged records on top of the ring
      │
  PROTO_PARSER   length-prefixed packets with a completion callback
```

MQTT uses the parser to reassemble frames out of a TCP byte stream, which is what they exist for. New code that just needs a container should reach for `pdiutil::vector` instead. One parser belongs to one stream — it carries state across calls.

## 15.6 Crypto

A small, production-quality kit. Everything is plain functions over fixed-size buffers, with no allocation and no hidden global state beyond contexts you own.

| Kind | What's there |
|---|---|
| hashes | SHA-1, SHA-256, SHA-512, each with streaming and one-shot forms |
| HMAC | HMAC-SHA1, HMAC-SHA256 |
| symmetric | AES-128 and AES-256 in ECB, CBC and CTR |
| key agreement | Curve25519, including the bridge from an Ed25519 private key |
| signing | Ed25519, and RSA with a portable big-integer layer and PKCS#1 v1.5 |

The Curve25519 and Ed25519 code comes from the standard portable reference. The RSA and big-integer layer is self-contained and device-agnostic, and the caller injects the RNG.

Every long asymmetric operation yields through one shared hook, `crypto_set_yield_hook` — the RSA ladder, the Ed25519 scalar multiplications and the Curve25519 ladder all call it from their inner loops. Install it around the operation and clear it afterwards, including on an early return, or the next caller inherits it. That is what carries on-device keygen and a handshake through without disabling the watchdog — on some ports disabling it stops the hardware feed rather than suspending it, which defeats the purpose entirely. SSH leans on all of it: host key generation, the key exchange, host-key signing, public-key authentication, and AES-CTR for transport encryption.

One property worth knowing before you rely on it: constant-time behaviour holds only where the upstream implementation provides it. Ed25519 verification is constant-time; the table-based AES and the big-integer path are not hardened against timing observation.

## 15.7 PdiSTL

A trimmed subset of the C++ standard library for targets that have no libstdc++, in [src/utility/stl/](../src/utility/stl/). It started from ArduinoSTL by way of uClibc++ and is being reworked for small heaps, one container at a time. Containers live under `pdiutil::` — `string`, `vector`, `function`, the smart pointers and the usual set — and algorithms under `pdistd::`. The C-library wrappers come along so the framework builds on toolchains that ship none.

In framework code, write `pdiutil::string` and `pdiutil::vector` with the namespace spelled out, never a `using namespace`. It keeps a later swap to the host standard library mechanical.

Every allocation a container makes goes through the same heap check as `pdiutil::safe_new` ([§12](memory-performance.md#12-memory--performance-notes)). An empty `vector` or `string` holds no memory until the first element arrives. When the heap check refuses to grow one, it is left exactly as it was — same size, same contents — so code that must not lose data checks the size after an append, insert or resize. A range copied in — `insert` or `assign` of a range, or a vector built from one — takes room for the whole range first, so it lands whole or not at all, never in part; `assign` keeps the old contents when the new ones do not fit. `pdiutil::to_string` follows the same rule: on a heap too low to hold the digits it returns an empty string.

`pdiutil::string` keeps room for its terminator whenever it grows, so `c_str()` and `data()` are plain reads that never touch the heap. Every search — `find`, `rfind` and the `find_*_of` family — works in place without allocating, and moving a string hands its buffer over rather than copying it, so `return` and a growing `vector` of strings cost no copies. It is built on `vector<char>` but offers only the members that keep the terminator intact; `emplace_back`, `shrink_to_fit` and a conversion to `vector<char>&` are deliberately absent.

`pdiutil::function` follows `std::function`: copies, moves, captures, `swap`, `target<T>()` and comparison with `nullptr` behave as the standard says, and a function built from a null function pointer or an empty function is empty. How it stores the callable is chosen by `__PDISTD_FUNCTION_INLINE_STORAGE__` at the top of `src/utility/stl/functional`. At `0`, the default, every callable goes to the heap behind a single pointer — one pointer per function, as small in static RAM as a function can be, at the cost of an allocation per callable. At `1` a callable no bigger than `__PDISTD_FUNCTION_INLINE_BYTES__` (one pointer) — a lambda that captures nothing, `[this]`, one reference, a plain function — is held inside the function itself with no heap at all, at two pointers per function. Either way a larger callable goes to the heap through the same check as everything else, and a refusal leaves the function empty. Things that differ from the standard: calling an empty function returns a default value instead of throwing, pointers to members are not callable targets, and `target<T>()` exists only at `1`.

`pdiutil::string` is not `std::string` — some methods are missing or named differently, and it has no small-string buffer, so a non-empty string is always a heap allocation. Check the header before assuming an API is there.

## 15.8 The umbrella header

`utility/Utility.h` is the single include that pulls in the scheduler variant the config selected, the event bus, conversions, string operations, the queues, Base64, I/O and the command base. It is the right include for a file that genuinely consumes the whole foundation, and the wrong one for a file that needs a single helper — it carries twenty-odd headers with it.
