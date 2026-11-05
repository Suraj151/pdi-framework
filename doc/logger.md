# 9. Logger

The logger is deliberately the smallest interesting subsystem in the framework: four levels, a printf-style formatter, and one shared object. It splits along a single axis — console lines go to the serial terminal, syslog lines go to the terminal *and* a file, and optionally to a remote collector. The interesting part is how it disappears. When a level's gate is off, every call site for that level compiles to nothing at all.

Start reading at [src/interface/pdi/impl/log/LogManager.h](../src/interface/pdi/impl/log/LogManager.h).

## 9.1 One logger

`__log_manager` is the single logger for the whole stack on every port. Its contract is two methods — initialise with an I/O interface, and log a level with a format string. It holds exactly one pointer: the terminal it writes to.

The device port does not implement a logger. Its only job is to expose its serial terminal, which the stack wires in during boot:

```cpp
__log_manager.init(__i_dvc_ctrl.getTerminal());
```

That happens inside `initialize()`, so there is no lifecycle call to remember.

For a syslog line, the manager formats it, echoes it to the console, and hands it to whatever sink is registered — the syslog service, which owns the file and the network. The manager itself never touches either. With no sink registered, a syslog line is simply a console line.

## 9.2 Levels

| Level | Console | Console + file | Use it for |
|---|---|---|---|
| info | `LogI` | `SysLogI` | state transitions and lifecycle |
| error | `LogE` | `SysLogE` | recoverable failures, rejected input |
| warning | `LogW` | `SysLogW` | unusual conditions |
| success | `LogS` | `SysLogS` | confirmation of something that mattered |

Each macro is variadic, so the same one takes a bare string or a format:

```cpp
LogI("Starting WiFi service");
LogI("NTP validity : %d", valid);
```

Reach for `Log*` for chatty development tracing that should never reach flash, and `SysLog*` for lines worth surviving a reboot. The framework's own error paths use `SysLogE`.

## 9.3 Flags

| Flag | Effect |
|---|---|
| `ENABLE_CONSOLE_LOG_INFO` / `_ERROR` / `_WARNING` / `_SUCCESS` | enables that level's console macro |
| `ENABLE_CONSOLE_LOG_ALL` | all four |
| `ENABLE_SYSLOG_SERVICE` | the file sink behind `SysLog*`; needs storage, and is defined inside that guard so it is simply absent on ports without a filesystem |
| `ENABLE_SYSLOG_FORWARD` | also ships each line to a collector; needs syslog and network, and stays idle until a host is configured |

With every console gate off and syslog off, the build is silent: nothing on serial, nothing on flash. When syslog is off, `SysLog*` degrades to the matching console macro, so call sites never need an `#ifdef`.

## 9.4 How a level disappears

```
  DataTypeDef.h        #define LogI(f, args...)          ← included early, everywhere
        │                                                  a no-op, so LogI() always compiles
        ▼
  LogMacros.h          #if defined(LogI) && gate-is-on
                       #undef  LogI
                       #define LogI(f, args...) __log_manager.log(INFO_LOG, RODT_ATTR(f), ##args)
```

`LogMacros.h` deliberately has no include guard, because the undef-and-redefine dance is idempotent: re-including it after the gates become visible upgrades the stubs in place. It reaches the service layer through `ServiceProvider.h` and the device layer through each port's base header, which covers everything.

The result is identical to hand-stripping every call site with `#ifdef`, while the code reads naturally.

## 9.5 Flash strings and the formatter

Every macro wraps its format string in `RODT_ATTR`, which keeps the literal in flash rather than RAM — a few hundred call sites then cost flash they were going to occupy anyway and no RAM at all. The manager copies the format into a short-lived buffer before walking it.

The formatter is the framework's own and understands `%d`, `%u`, `%x`, `%c`, `%f` and `%s`. There are no width, precision or length modifiers, so cast a 64-bit value down before logging it. Since every message goes through the formatter, a literal percent sign needs escaping — write `%%`, or pass the text as a `%s` argument.

Console output streams to the terminal character by character with no line buffer, which is what keeps it usable on the RAM-starved boards.

## 9.6 Where syslog goes

```
  SysLogE("…")
      │
  LogManager   format ──▶ console echo
      │
      └─▶ sink: SyslogServiceProvider
              ├─▶ /var/log/syslog.<level>     appended, NTP-timestamped
              └─▶ RFC 3164 datagram over UDP  when forwarding is on
```

The service registers itself as the sink during boot, before the other services start, so their startup lines are captured too.

| Level | File |
|---|---|
| info | `/var/log/syslog.info` |
| error | `/var/log/syslog.error` |
| warning | `/var/log/syslog.warning` |
| success | `/var/log/syslog.success` |

Files split by level, because level is the only identity a line carries. Each file line is prefixed with the NTP date, re-stamped after any embedded newline, and rendered as dashes until the clock syncs. `/var/log` is created on the first write. A file grows to 8 KB and then restarts from empty, and an assembled line is capped at 200 bytes. A re-entry guard drops anything logged from inside a syslog write, so the sink cannot recurse into itself.

Read them like any other file — `cat /var/log/syslog.error`, `tail /var/log/syslog.info 20`, `grep MQTT /var/log/syslog.info`.

Remote forwarding builds the datagram directly on the UDP interface, with no vendor syslog library: `<PRI>timestamp hostname pdi: message`, where PRI comes from the level under the `local0` facility, the timestamp comes from NTP once synced, and the hostname is the device address. The collector host is resolved through the name resolver once the station has an IP, and `service status syslog` shows the target, whether it resolved, and the socket state. File writing and forwarding are independent — files work with no network at all, and forwarding rides on top when there is one.

Two things follow from the design. Levels are compile-time, so turning `LogI` back on means a rebuild. And the logger shares the serial terminal with the shell, so on a single-UART board their output interleaves — which is exactly what you want while watching a device boot.
