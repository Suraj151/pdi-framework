# 12. Memory & Performance Notes

The constraints scattered through the preceding sections, collected in one place so you can size a build before compiling it.

## 12.1 Budget per target

Roughly what is left after the board's core, its network stack and the standard library are linked. Boards are grouped by the budget they bring, since that is what actually decides the answer. Orders of magnitude, not guarantees.

| Class of board | Flash | RAM | NVM | What fits |
|---|---|---|---|---|
| kilobytes of RAM, no radio | ~256 KB | ~8 KB | ~4 KB | the shell over serial, EEPROM settings, time, basic GPIO — no network, no filesystem |
| tens of kilobytes, with a radio | ~1 MB | ~50 KB free heap | ~4 KB | the full build with the portal over plain HTTP; contextual execution is comfortable |
| hundreds of kilobytes, with a radio | ~4 MB | ~250 KB free heap | ~4 KB | the full build, with room for a second contextual lane |

That table is why entire feature groups are gated on declared capability rather than assumed, and why the table limit is 5 where NVM is a kilobyte against 15 where it is a few.

What the stock `PdiStack` sketch takes on each board, built as it ships:

| Board | Flash | Other limit |
|---|---|---|
| esp32 (Minimal SPIFFS) | 1,855,139 of 1,966,080 B, 94% | — |
| esp8266 (NodeMCU defaults) | 866,264 of 1,048,576 B, 82% | IRAM 62,275 of 65,536 B, 95%; RAM 40,052 of 80,192 B |
| arduinomega (Mega 2560) | 84,076 of 253,952 B, 33% | RAM 2,788 of 8,192 B |

The esp8266's IRAM is the tightest of these: code that has to run from IRAM, such as an interrupt handler, has little room left.

## 12.2 Keeping strings out of RAM

Two macros, one purpose:

| Macro | On the ESP ports | Elsewhere |
|---|---|---|
| `RODT_ATTR("text")` | wraps the literal so it stays in flash | plain literal, same behaviour |
| `PROG_RODT_ATTR` | a storage qualifier that puts the variable in flash | empty; the variable is already read-only data |
| `PROG_RODT_PTR` | the right pointer type where program memory is a separate address space | a plain `const char*` |

The rule is simple: a literal used inline goes in `RODT_ATTR(...)`, and one held in a named variable is declared `static const char foo[] PROG_RODT_ATTR = "…"`. Every prompt, page fragment, log message and service name in the framework follows it, which is why the binary is dense rather than RAM-hungry.

When a consumer genuinely cannot take a flash pointer, `rofn::to_charptr()` copies the string into a fresh heap buffer and hands it over — and the caller owns it from there. It is an escape hatch, not a default; every call is an allocation.

## 12.3 The expensive features

A handful of choices dominate the budget.

**SSH** is the heaviest single flag. Turning it on effectively commits you to a device with heap to spare, and to compiling out whatever else was competing for it.

**TLS** costs about what SSH does in flash, and more than NAPT in heap. Where the heap is small and fixed the two cannot coexist — both want more of it than exists — and the failure lands at runtime, since nothing rejects the pairing at build time. Inbound HTTPS and outbound TLS share the same backend, so enabling both costs nothing extra.

What matters with TLS is per-session, not per-build:

```
   one live TLS session holds
        ├─ a worker task stack        ~6.5 KB
        ├─ record buffers             in + out
        └─ engine state               keys, cipher contexts, cert chain during validation

   a compact backend   10-15 KB per session   on a 30-40 KB working budget
   a full one          35-50 KB per session   mostly the 16 KB record buffers,
                                             which the SDK config can halve
```

Every byte of that comes back when the client disconnects — the worker exits, buffers are freed, state is torn down, though where an RTOS owns the threads its idle task reclaims the stack a moment later. Size for the worst-case number of *concurrent* sessions; an idle build always looks healthy because none of it is allocated yet.

**NAPT** is invisible in flash and expensive in heap, because the network stack holds the translation table. Leave it off unless the device is bridging its AP to the station link.

**The portal's controllers** each carry their own form-validation code, and there are a dozen. If you don't need the portal, drop the HTTP server even while keeping WiFi.

**Contextual execution** is cheap in flash and costs RAM for the lifetime of every task, since each one owns its stack. Two cooperative tasks at a kilobyte each is two kilobytes you don't get back until they exit.

**The containers allocate.** `pdiutil::string` and `pdiutil::vector` hide the heap but still use it, and repeated growth fragments. Reserve up front wherever the size is known, the way the scheduler reserves its task table.

**Sessions and their pools.** Every session slot is a fixed record in the table, so `PDI_MAX_SESSIONS` is a straight multiplier on static RAM — and because the table is sized from the telnet and SSH pools, growing a pool grows the table with it. A live session then costs a little heap on top for its line buffer and paths. On a tight port, shrink the pools rather than the table. An SSH session moving a file holds one incoming packet of `SSH_RX_PACKET_MAX` and up to `SSH_TX_QUEUE_MAX` of outgoing data at a time, whatever the size of the file; when the heap cannot give it that for a moment, it waits rather than ending the transfer. The script runner adds a little to each slot too — the variables the session holds, and where it has got to in any script it is running — which is part of why it is behind a flag you can drop.

## 12.4 Heap discipline

The framework is built to run for weeks or months between reboots, which shapes a few habits.

Don't allocate after `setup()`. Anything that allocates per request, per tick or per event will fragment eventually. Reserve container sizes at init. Reuse buffers — the portal reuses one scratch buffer, transports reuse a caller-owned client, the MQTT parser uses a static ring. Where a buffer must be held across ticks, hold and then free, the way a held command option is released when the command clears.

For the paths that genuinely can be skipped under pressure — a TLS handshake, a large page composition — use `pdiutil::safe_new<T>()` and `safe_new_array<T>(n)`. They refuse the allocation and return null when it would leave less than `PDI_SAFE_ALLOC_HEAP_MARGIN` (2 KB) of free heap, or when no single free block can hold it, which lets the caller back out cleanly instead of failing somewhere deep inside a library. The check reads the heap `malloc` itself draws from, and the allocation behind it never throws: when another task takes the memory between the check and the allocation, the call still returns null and the refusal is counted, so a short heap is never a reset. The containers in [§15.7](utility.md#157-pdistl) allocate through the same check. Every refusal is counted, and `/proc/meminfo` shows the count as `AllocRefused` — a number that keeps climbing is the first sign a board is running short.

When fragmentation does set in, the symptoms are recognisable: `register_task` returning -1, a transport getting null from an allocation, or a vector failing to grow.

## 12.5 CPU and tick budget

| Step of `serve()` | Typical cost |
|---|---|
| web server client handling | under 1 ms idle, 5-50 ms during a request |
| scheduler | under 1 ms idle; at most one inline task per pass |
| device yield | the vendor SDK's slice, 1-3 ms |
| event dispatch | under 1 ms unless a handler is slow |
| contextual ticks | whatever those tasks do |

Loop frequency runs from hundreds of hertz when idle down to around ten during heavy HTTP or SSH traffic. So anything needing sub-100 ms response belongs on a contextual lane rather than in an inline task, and shortening the WiFi connectivity check below five seconds starts competing with the loop's real work.

## 12.6 Boot profile

```
  power-on
    ├─ static init of every global                 10-50 ms
  setup()
    ├─ database: read every table from NVM        50-200 ms
    ├─ serial and terminal greeting                 ~10 ms
    ├─ WiFi init and station scan               500-3000 ms   ← dominates
    ├─ http, telnet, ssh listeners                  ~50 ms
    └─ shell prompt                                 ~10 ms
  loop()
```

One to four seconds to first prompt, depending mostly on how quickly the station associates.

## 12.7 Choices that pay off

Wrap every literal in `RODT_ATTR` — skipping it breaks nothing and quietly moves a few hundred bytes per file into RAM.

Use `char[]` for anything NVM-shaped and `pdiutil::string` for transient work; mixing them inside one config struct breaks the serialisation contract.

Use explicit-width integer types in config structs. `sizeof(int)` differs between 8-bit and 32-bit ports, and an NVM layout that depends on it is not portable.

Stay away from the `printf` family; the framework's own conversions save four to eight kilobytes by never linking libc's formatter.

Move firmware over OTA rather than SFTP: OTA writes the image straight into the update partition, while SFTP stores it as a file first ([§7.10](shell.md#710-sftp-and-scp-file-transfer)).

## 12.8 Looking at a running device

`ps` gives every scheduler task with its rolling CPU share, run count and interval — the fastest way to spot a service hogging the loop. `service list` shows service state and task counts, and `service status <name>` drills into one, covering database validity, WiFi state, MQTT connection and the rest. A port that implements the optional stack-measurement hook also lets you bracket critical work and read back a high-water mark.

Two behaviours to keep in mind while reading those numbers. The scheduler runs one inline task per pass, so twenty tasks at a 100 ms cadence will pace each other if the loop is turning over ten times a second. And a page send is capped per chunk rather than per response, which is why composed pages go out in three calls.
