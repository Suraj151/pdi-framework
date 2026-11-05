# 13. Portable Interfaces

The interface layer is the contract between the framework and a device. Every type here is abstract: pure virtual methods, no state, no platform headers, depending only on standard types and other interfaces. Each is implemented by exactly one device-side class per build and reached through a single `__i_*` global.

## 13.1 Layout

```
  src/interface/pdi/
    iDatabaseInterface        NVM-backed blob store
    iLoggerInterface          the log sink
    iDeviceIotInterface       the application's hook into the IoT service

    drivers/                  bare-metal surfaces
      iGpioInterface          digital, analog, blink
      iWdtInterface           watchdog

    middlewares/              higher-level building blocks
      iDeviceControlInterface composite: gpio + wdt + utility + upgrade
      iClientInterface        stream client, then TCP, then TLS
      iServerInterface        TCP server, TLS server, HTTP server
      iNtpInterface           time sync and clock set
      iPingInterface          reachability and per-packet stats
      iUdpInterface           raw UDP
      iUpgradeInterface       the OTA primitive

    modules/                  standalone features
      serial/ storage/ wifi/  serial port, storage + filesystem, WiFi

    threading/                optional execution contexts
      iContext iMutex iCondvar iExecution + cooperative/ preemptive/

    impl/                     portable defaults a port can adopt as-is
      HttpServerInterfaceImpl · FileSystemInterfaceImpl
```

Three more interfaces live with the utilities rather than here, because they have no device dependency at all: the byte and line I/O contract that every stream-like thing derives from, the utility interface holding time, randomness and yielding, and the instance factory that hands out fresh clients, servers, sockets and filesystem handles.

## 13.2 Conventions

Interface types start with a lowercase `i`. Each header forward-declares the concrete class the port will define and, at the bottom, declares the singleton:

```cpp
extern DeviceControlInterface __i_dvc_ctrl;
```

That declaration is the coupling — the port's source file defines the matching variable, and everything above finds it by name.

Composites are built by multiple inheritance rather than by aggregation, which is why device control alone gives you GPIO, watchdog, utility and upgrade. And everything stream-like shares the terminal base, which is what lets the logger, the shell and the web writer target any of them without caring which.

## 13.3 Reference

### 13.3.1 Core, always required

| Interface | Implemented by | Used by | Key methods |
|---|---|---|---|
| `iDeviceControlInterface` | device | the orchestrator and every service | device init, reset, restart, erase config, device id and MAC, factory-request check, `getTerminal`, `handleEvents`, plus everything it inherits |
| `iDatabaseInterface` | device | the database service and every table | begin, clean, validate, report size, and the typed read/write templates |
| `iInstanceInterface` | device | anything needing a connection | new TCP client and server, new UDP socket, new TLS client and server, the **shared** outbound TCP and TLS clients, filesystem and utility handles |
| `iUtilityInterface` | inherited through device control | scheduler, event bus, logger, `/dev/random` | `wait`, `millis_now`, `micros_now`, `random_now`, `yield`, `log`, optional stack measurement |
| `iIOInterface`, `iTerminalInterface` | any stream | logger, shell, web writers | the write family overloaded for every primitive, timestamps, connect and disconnect |

### 13.3.2 Drivers

| Interface | Used by | Key methods |
|---|---|---|
| `iGpioInterface` | GPIO service, `/sys/class/gpio` | mode, write, read, pin mapping, exceptional-pin check, blinker create and release |
| `iGpioBlinkerInterface` | GPIO service in blink mode | configure, update, start, stop, running |
| `iWdtInterface` | long-running services, the scheduler | enable, disable, feed |
| `iSerialInterface` | the serial service, the shell, every driver | the stream calls, plus `type`, `select`, `probe` and `transfer` for a bus with peers on it |
| `iI2cSerialInterface` | I2C drivers | `readRegister` and `writeRegister`; a port supplies only `probe`, `select` and `transfer` |
| `iUsbHostSerialInterface` | usb drivers | `deviceCount` and `deviceAt` for what is attached; the endpoint is the selected target |
| `iDriverInterface` | the serial service | `probe` to confirm and take a device, `remove` to let it go |

### 13.3.3 Middlewares

| Interface | Used by | Notes |
|---|---|---|
| `iClientInterface` | terminals, transports | the I/O surface plus a timeout |
| `iTcpClientInterface` | OTA, MQTT, SMTP, HTTP | adds local and remote addresses, keep-alive, no-delay |
| `iTlsClientInterface` | the same consumers, when TLS is on | adds CA path, client cert and key, SNI hostname, peer verification |
| `iTcpServerInterface` | telnet, SSH, raw TCP | begin, has-client, accept, close |
| `iTlsServerInterface` | the HTTPS server | adds cert, key and client CA; `accept()` hands back a TLS-capable client transparently |
| `iHttpServerInterface` | the web server | routing, args and headers, `send(code, mime, body, chunked)`, and `begin(port, secure)` |
| `iUdpInterface` | the mDNS responder | begin, join multicast, send, packet callback, close |
| `iNtpInterface` | log timestamps, IoT, sessions, `date` | init, validity, get, set |
| `iPingInterface` | the WiFi internet check, the `ping` command | ping with a count and a per-packet callback, completion, stats |
| `iUpgradeInterface` | the OTA service | one call: upgrade from a path |

### 13.3.4 Modules

| Interface | Used by | Notes |
|---|---|---|
| `iSerialInterface` | serial service, logger, shell | derives from the client interface — serial is just another stream |
| `iStorageInterface` | the filesystem, LittleFS | byte-addressable read, write, erase, size |
| `iFileSystemInterface` | SSH, SFTP, every file command | file and directory CRUD, traversal, line and offset lookup, search, custom attributes |
| `iWiFiInterface` | WiFi service, `net` | station and AP, sync and async scan, NAPT, mode |
| `iNetifInterface` | `/sys/class/net`, `/proc/net` | one network link describing itself: name, kind, hardware address, addressing, link state. Traffic counters are optional — a link that cannot count says so |
| `iNetStackInterface` | `/proc/net/tcp` | the stack behind the links, walked for its TCP endpoints. Endpoints belong to the stack rather than to any one interface, so they are asked for here and not through a netif. Optional: a port that cannot enumerate its stack registers none |

### 13.3.5 Optional

| Interface | Implemented by | Notes |
|---|---|---|
| `iLoggerInterface` | the framework, not the device | the device supplies only its terminal; `LogManager` is the sole implementation |
| `iDeviceIotInterface` | the application | sample, data and reset hooks — the one interface whose implementer is your sketch |

That last one is the framework's deliberate extension point for "what to publish, and how often".

### 13.3.6 Threading, only for contextual execution

| Interface | Notes |
|---|---|
| `iContext` | raw register save and restore, CPU-specific |
| `iMutex` | lock and unlock, plus IRQ-safe critical variants |
| `iConditionVar` | wait, notify one, notify all |
| `iExecutionContext` | start, suspend, resume |
| `iExecutive` | stack pointer and size, entry point and argument, back-link to the task |
| `iExecutionScheduler` | schedule, mute, yield, sleep, run, and optional cross-scheduler hand-off |
| `iCooperative`, `iPreemptive` and their schedulers | specialisation tags for the two lanes |

The soft-IRQ machinery lives in the same header because it is the protocol between an ISR on the device side and the scheduler on the utility side:

```
   tick ISR ──▶ raise_softirq(bit)
                        │
   main context ──▶ fetch_softirq_bits() ──▶ scheduler acts
```

A port that wants preemption wires its tick ISR to exactly that.

## 13.4 Shared default implementations

Not every interface is worth rewriting per device. The portable defaults ship under `impl/`, and they are what a new port should reach for first.

The HTTP server implementation is a protocol-correct HTTP/1.1 server built on nothing but the TCP server and client interfaces — and with TLS enabled, the same file serves HTTPS by wrapping accepted connections. The filesystem side is larger: LittleFS on top of any storage interface, the dispatcher that routes one tree across several backends, and the generated filesystems behind `/proc`, `/sys`, `/dev` and `/tmp`. None of them are per-board.

So a new device needs to supply raw TCP and raw storage, and inherits the HTTP server, the HTTPS server, the whole filesystem and every synthetic mount for free. The TLS classes are deliberately not here, because SSL backends differ enough from one another that each port supplies its own pair.

## 13.5 What an implementation must promise

Construction is cheap and free of side effects. The singletons are constructed before `setup()` runs, so a constructor must not allocate, touch hardware, or open a port — that work belongs in an init method the orchestrator calls.

Methods do not block unless they say so. Anything that might take more than a few milliseconds — a TCP connect, an NTP fetch, an OTA download — either returns early or exposes a state machine, and the scheduler drives the long form.

Re-entrancy is not assumed. The framework is single-threaded by default; when a port enables contextual execution, the implementation rather than the caller is responsible for guarding shared state.

## 13.6 Adding an interface

The bar is whether at least two devices could implement it differently. If they could:

1. Pick the group — drivers for silicon, middlewares for network and device operations, modules for orthogonal features, threading for execution, top level for cross-cutting concerns.
2. Forward-declare the concrete class and declare the `extern` singleton at the bottom.
3. Guard it with the same flag that gates the service consuming it, so no existing port has to provide anything until it opts in.
4. Add an implementation to the host port so the off-device build still links.
5. Write it up here.

An interface with exactly one implementation is usually a sign the abstraction is premature — keep it in device-specific code until a second port needs it.
