# 14. Device Layer & Porting Guide

The device layer is the only place vendor SDK and Arduino-core symbols are allowed. Everything above it talks through abstract interfaces. A port is one folder under [devices/](../devices/) implementing those interfaces for one MCU family.

## 14.1 What a port contains

```
  devices/<board>/
    <board>.h                   umbrella include for the SDK and core
    device_config.h             platform macros: flash strings, critical sections
    device_pdi.h                header aggregator — what the framework sees
    device_pdi.cpp              source aggregator — see below
    device_pdi.c                C-side source aggregator

    DeviceControlInterface      required
    DatabaseInterface           required
    InstanceInterface           required

    SerialInterface             with the serial service
    StorageInterface            with storage
    FileSystemInterface         with storage
    WiFiInterface               with WiFi
    HttpServerInterface         with the web server
    TcpClient / TcpServer       with networking
    UdpInterface                with networking
    NtpInterface / PingInterface with networking
    TlsClient / TlsServer       with TLS, on whichever SSL backend the board has
    cert loader                 per backend, loads PEM and DER off the filesystem
    TlsCertProvisioner          with on-device cert generation
    ExceptionsNotifier          optional crash capture

    config/DBTableSchema.json   this board's table layout
    core/                       vendor helpers, e.g. an EEPROM emulator
    threading/                  optional: the cooperative and preemptive lanes
```

The two ends of the spectrum are worth looking at. The **host** port targets the machine you develop on: real BSD sockets, real files, real ICMP for `ping`, and a restart that re-execs the process. It is what the test suite runs the whole framework against, so a shell, an ssh channel and the portal can all be driven without a board. At the other end, a port for a board with no radio and no filesystem beyond EEPROM supplies device control, database, serial, storage and the instance factory, and nothing more — no network, no web server.

Threading is where ports differ most and it matters least: one may build on the primitives an RTOS already provides, another on bare-metal context switching driven by a hardware timer. Both satisfy the same interfaces, and nothing above them can tell which it got.

## 14.2 Required versus optional

| Interface | When | Notes |
|---|---|---|
| device control | always | GPIO, reset, watchdog, yield, events, terminal |
| database | always | NVM for the config store |
| instance factory | always | hands out fresh clients, servers and handles |
| serial | with the serial service | the serial terminal |
| storage + filesystem | with storage | LittleFS, SPIFFS or an SD adapter |
| WiFi, HTTP server | with WiFi | station and AP, the embedded server |
| TCP client and server | with networking | MQTT, SMTP, OTA, telnet, SSH |
| NTP, ping | with networking | time sync and reachability |
| TLS client and server | with TLS | handed out when something asks for a secure address, so a build with TLS still reaches plain ones |
| cert provisioner | with on-device cert generation | free functions rather than a virtual interface |
| threading family | with contextual execution | also required by TLS, which runs off the main stack |
| GPIO and watchdog | always, folded in | implemented as part of device control rather than as separate classes |

A port is valid the moment the always rows compile and link. Everything else arrives as you turn flags on.

## 14.3 The two aggregators

Every port supplies a pair, with a strict split:

```
  device_pdi.h     which interface headers the framework can see
                   each include wrapped in its ENABLE_* guard, so unused interfaces cost nothing

  device_pdi.cpp   #includes the implementation .cpp files
                   the Arduino build flattens the port into one object file, which means
                   anything marked static there is per-port, not per-file — and those .cpp
                   files must never be included from outside this chain
```

The names are fixed rather than prefixed with the port, because the selector builds the include path from the port directory name alone.

A third aggregator, `device_pdi.c`, carries pure-C translation units. It is required even when a port has no C sources — `src/interface/pdi.c` includes it unconditionally — in which case it holds nothing but its file header. The umbrella header exists so each per-interface header can pull in the SDK once rather than repeating the plumbing.

## 14.4 How a board gets selected

```
  DeviceSetup.py -d <board>
        │  writes
        ▼
  devices/DeviceSetup.h        #define PDI_DEVICE <board>
        │  included by
        ▼
  devices/DeviceConfig.h       cascades into ENABLE_* flags, and pulls in
        │                      <board>/device_config.h so the platform macros exist
        │                      before any framework header is parsed. that header
        │                      defines DEVICE_<BOARD>, so the port names itself
        ▼
  src/config/Config.h          now everything under src/ sees flags and macros
        ▼
  src/interface/pdi.h          includes <board>/device_pdi.h
        ▼
  the port's interface headers  which transitively pull in the SDK
```

`PDI_DEVICE` names the port directory, and every selector builds its include path from that one macro. Setting it on the compiler command line takes precedence over the generated header, so a build matrix can cover every port from one tree without rewriting a file:

```
  -DPDI_DEVICE=<board>
```

Adding a board therefore touches **no** source file outside its own folder — only the architecture list in `library.properties` and `library.json`.

## 14.5 The singletons a port must define

Each port instantiates exactly one object per interface, under the name the framework expects — those names are part of the contract.

| Symbol | Required when |
|---|---|
| `__i_dvc_ctrl`, `__i_db`, `__i_instance` | always |
| `__serial_uart`, `__serial_uart1` | serial service — registered with `iSerialInterface`, which hands them out by `getSerialInstance(type)` rather than through a single global |
| `__i_storage`, `__i_fs` | storage |
| `__i_wifi`, `__i_http_server` | WiFi |
| `__i_ntp`, `__i_ping` | networking |
| `__i_cooperative_scheduler`, `__i_preemptive_scheduler` | contextual execution |

If the port skips a flag it must also skip the symbol — the name should not exist when its feature is off.

## 14.6 The optional threading port

Four pieces make the contextual lanes work: a scheduler for each lane, a context implementation that saves and restores CPU state, a matching mutex and condition variable, and a periodic tick source to drive preemption.

Leave the layer out and contextual tasks are simply unavailable — inline tasks keep working, because they run on the loop's own stack. [§4](scheduler.md#4-task-scheduler) covers the trade-offs.

Enabling TLS turns this layer on implicitly, because both SSL backends need more stack than the Arduino main context has; the handshake runs on a dedicated cooperative task instead.

## 14.7 Per-board database schema

Each port carries its own table schema describing what lives in NVM on that board, tuned to its capacity. The setup script turns it into C++ table sources. Format is in [§5.6](database.md#56-where-the-tables-come-from).

## 14.8 Porting, step by step

Say the board is `myboard`.

1. **Create the folder** with three files to start: the SDK umbrella header, the platform-macro header, and a copy of an existing board's table schema.
   ```
   devices/myboard/
     myboard.h
     device_config.h
     config/DBTableSchema.json
   ```
   Have `device_config.h` define `DEVICE_MYBOARD` right after its include guard, so the board names itself to the rest of the framework.
2. **Implement the three required interfaces** — device control, database, instance factory — each deriving from its abstract counterpart, each defining its `__i_*` global.
3. **Write the three aggregators** — `device_pdi.h`, `device_pdi.cpp` and `device_pdi.c` — mirroring an existing board's set and keeping only what you have implemented. The `.c` is required even if it stays empty.
4. **Nothing to register.** The selectors build their paths from `PDI_DEVICE`, so the folder name is the registration.
5. **Add the per-board limits** — pin counts, table count — and switch off any service the board cannot support, the way a port with no radio does.
6. **Generate the setup files**: `python3 DeviceSetup.py -d myboard`.
7. **Build the bundled example** for the new board. That is the first real validation.
8. **Add optional interfaces one flag at a time**, rebuilding as you go.
9. **Add TLS** if the board can carry it, by implementing the client and server against whichever SSL stack it has and wiring the factory to hand them out. The two existing ports implement the same contract on different libraries, so either is a usable reference.
10. **Add the threading port** if you want the contextual lanes, or if you turned on TLS. The bare-metal and the RTOS-backed implementations bracket the range of what this can look like.

## 14.9 Before you call it done

- The host port still compiles — proof that nothing under `src/` picked up a vendor header.
- The bundled example builds with every flag the board can support.
- Every `__i_*` symbol the flag set implies is defined exactly once.
- Microsecond time is monotonic across the platform's counter wrap, and `ps` shows non-zero CPU share for tasks with sub-millisecond callbacks after a few ticks.
- `service list` shows every service the build started, and stop and start actually freeze and resume them.
- Reboot and factory reset both round-trip without losing the database.
- With storage on, upload and download over SFTP work.
- With contextual execution on, a task on each lane runs and prints without corrupting a stack.
- With TLS on, the factory returns a live instance and a handshake against a known peer completes.
- With HTTPS on, the portal answers on 443 once the certificate and key are on the filesystem, and plain HTTP on 80 redirects to it.
- With on-device cert generation on, the `tls` command writes both files where the config says.
