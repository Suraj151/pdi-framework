# PDI Framework — Portable Device Interface Stack

PDI is a modular C++ stack for embedded devices. Application code is written once against clean interfaces (`i*Interface`) and runs on ESP32, ESP8266, Arduino UNO — or any board that has a device port. Batteries included: WiFi, HTTP/HTTPS server, MQTT, OTA, SSH, Telnet, SFTP, SMTP, GPIO, terminal CLI, and a configurable task scheduler.

<p align="center">
  <img width="500" src="https://github.com/Suraj151/pdi-framework/blob/master/doc/pdi-framework.jpg">
</p>

## Capabilities

- **Portable architecture.** Services depend on abstract interfaces, not vendor SDKs. Adding a new board means writing an adapter — the application and service layers stay unchanged.
- **Bundled services.** WiFi captive portal, HTTP/HTTPS web portal, MQTT client, OTA updates, SSH server, Telnet server, SFTP subsystem, SMTP client, GPIO control (local + MQTT/HTTP), NVM database, TLS (BearSSL / mbedTLS), ESPNOW mesh, Auth, Device-IoT.
- **Compile-time feature gating.** Each capability is wrapped in an `ENABLE_*` flag; disabled features contribute zero flash.
- **Configurable task scheduler.** Inline, cooperative, and preemptive modes; priority-and-policy scheduling; POSIX nice; per-task signals (KILL/TERM/STOP/CONT) with `ps`/`top`/`kill`/`pkill`/`killall`/`renice`.
- **Service supervisor (systemd-lite).** `srvc list / status / start / stop / restart` — every service tracks its scheduler tasks and can be paused or resumed at runtime.
- **Virtual filesystem (VFS).** Multiple backends mounted under one tree with longest-prefix routing; POSIX-style permissions, ownership, and per-session umask enforced at the VFS layer; multi-user aware (`/etc/passwd` + `/etc/shadow`). Includes a read-only `/proc` with live system nodes, a read/write `/sys` exposing GPIO as files (`echo 1 > /sys/class/gpio/5/value`), a `/dev` with byte-stream nodes (`/dev/null`, `/dev/zero`, `/dev/random`), and a RAM-backed `/tmp` scratch filesystem.
- **Linux-style CLI on serial / Telnet / SSH.** `ls`, `cat`, `echo`, `grep`, `head`, `tail`, `wc`, `hexdump`, `df`, `mount`, `chmod`, `chown`, `umask`, `uptime`, `mv`, `cp`, `mkdir`, `touch`, `rm`, `watch`, `srvc`, `ps`, `top`, `kill`, `pkill`, `killall`, `renice`, `net`, `iot`, `ssh`, `tls`, `reboot`, and more. (GPIO is driven as files via `/sys` — see below.)
- **On-device file transfer.** `scp` (single file) and interactive `sftp` over the SSH tunnel.
- **Web portal for configuration.** Session-based login, per-service settings pages, GPIO control, storage browser, MQTT tester, Email tester.
- **Persistent config store.** Address-based table engine with JSON-driven codegen for schema tables.
- **Cross-platform build.** Arduino Library Manager install or manual clone; single Python script switches the active device port.
- **Extensible.** New services, commands, web pages, and device ports plug in through documented base classes.

## Quick Start

1. **Install** from the **Arduino Library Manager** (search "pdi-framework"). Builds for ESP32 by default.
   For ESP8266 / Arduino UNO, run: `python3 scripts/DeviceSetup.py -d <board>`.
2. Open **File → Examples → pdi-framework → PdiStack**, then compile and upload.
3. On your phone / laptop, look for the WiFi network **`pdiStack`** — password **`pdiStack@123`**.
4. Browse to **http://192.168.0.1** and log in as **`pdiStack` / `pdiStack@123`**.

That's it — the device is now running a web portal, remote shell, and file transfer.

Prefer a remote shell? `ssh pdiStack@<device-ip>` or `telnet <device-ip>` (default port 22 / 23).
Copy files? `scp -s file pdiStack@<device-ip>:/path` or `sftp -P 22 pdiStack@<device-ip>`.

Full install / build details (manual clone paths, autogen script, board-package versions, git-ignored files) are in [§2 Build & Toolchain](#2-build--toolchain).

### Supported Boards

| Device | Arduino board version |
|---|---|
| esp32 | 3.3.3 |
| esp8266 | 3.1.2 |
| arduinouno | 1.8.6 |

**Note:** not every board exposes every capability — for example Arduino UNO has no WiFi, so the web server / MQTT / OTA services are disabled at compile time on that port.

## What's Inside

**Services** — WiFi · HTTP/S server · MQTT · OTA · SSH · Telnet · SFTP · SMTP · GPIO · Serial · Terminal · Database · TLS · ESPNOW · Auth · Device-IoT.
Per-service reference in [§6 Service Providers](#6-service-providers).

**Utilities** — Task Scheduler (inline / cooperative / preemptive), Event bus, Queues, String helpers, Data converters, Crypto, PdiSTL, Reset Factory.
Full inventory in [§15 Utility Library](#15-utility-library).

**Storage** — VFS with mountable backends (LittleFS root + `/proc` + `/sys` + `/dev` synthetics + RAM-backed `/tmp`), POSIX permissions/ownership with per-session umask, multi-user file access control.
Details in [§6.2.11 Storage](#6211-storage-interface-init-no-provider).

**CLI** — 40+ built-in commands including `ls mkdir touch mv cp cat echo head tail wc hexdump grep df mount chmod chown umask srvc ps top kill pkill killall renice net watch iot ssh tls reboot uptime groups useradd userdel passwd ...`.
Full command reference in [§7.7 Built-in command inventory](#77-built-in-command-inventory).

**Extras** — Captive portal, GPIO events over MQTT/HTTP/Email, NAT (ESP8266 lwIP — see [§2.4.1](#241-nat-and-mesh)), Mesh via ESPNOW.

## A Peek at the Terminal and Web UI

<table>
  <tr>
    <td width="50%"><img src="https://github.com/Suraj151/pdi-framework/blob/master/doc/terminal-ls.png" width="100%"></td>
    <td width="50%"><img src="https://github.com/Suraj151/pdi-framework/blob/master/doc/terminal-ssh.png" width="100%"></td>
  </tr>
  <tr>
    <td width="50%"><img src="https://github.com/Suraj151/pdi-framework/blob/master/doc/terminal-ps.png" width="100%"></td>
    <td width="50%"><img src="https://github.com/Suraj151/pdi-framework/blob/master/doc/terminal-usradd.png" width="100%"></td>
  </tr>
  <tr>
    <td width="50%"><img src="https://github.com/Suraj151/pdi-framework/blob/master/doc/terminal-net.png" width="100%"></td>
    <td width="50%"><img src="https://github.com/Suraj151/pdi-framework/blob/master/doc/terminal-help.png" width="100%"></td>
  </tr>
  <tr>
    <td width="50%"><img src="https://github.com/Suraj151/esp8266-framework/blob/master/doc/portal_home_menu.png" width="100%"></td>
    <td width="50%"><img src="https://github.com/Suraj151/esp8266-framework/blob/master/doc/gpio-control-menu.png" width="100%"></td>
  </tr>
  <tr>
    <td width="50%"><img src="https://github.com/Suraj151/esp8266-framework/blob/master/doc/mqtt-submenu.png" width="100%"></td>
    <td width="50%"><img src="https://github.com/Suraj151/esp8266-framework/blob/master/doc/storage-home.png" width="100%"></td>
  </tr>
</table>

## Want to Dig Deeper?

The **[Detailed Documentation](#detailed-documentation)** below is an in-tree reference for contributors and porters. Jump straight to what interests you:

- **[1. Architecture Overview](#1-architecture-overview)** — layered model, ports & adapters, runtime lifecycle.
- **[2. Build & Toolchain](#2-build--toolchain)** — install, switch board, autogen scripts, vendored externals.
- **[3. Configuration System](#3-configuration-system)** — every `ENABLE_*` flag and its dependencies.
- **[4. Task Scheduler](#4-task-scheduler)** — three modes, four policies, decision tree.
- **[5. Database Layer](#5-database-layer)** — schema model, address-based store, JSON codegen.
- **[6. Service Providers](#6-service-providers)** — per-service init flow, CLI / web surface, events.
- **[7. Command Line / Terminal](#7-command-line--terminal)** — full CLI reference & how to add a command.
- **[8. Web Server](#8-web-server)** — request lifecycle, routes, views, "how to add a page".
- **[9. Logger](#9-logger)** — levels, macros, and the zero-cost-when-disabled pattern.
- **[10. Transports](#10-transports)** — HTTP, MQTT, SMTP client internals.
- **[11. Examples Walkthrough](#11-examples-walkthrough)** — bundled sketches, line by line.
- **[12. Memory & Performance Notes](#12-memory--performance-notes)** — flash/RAM cost per feature.
- **[13. Portable Interfaces](#13-portable-interfaces)** — the "ports" every device adapter implements.
- **[14. Device Layer & Porting Guide](#14-device-layer--porting-guide)** — how to add a new board.
- **[15. Utility Library](#15-utility-library)** — scheduler, event bus, string ops, embedded STL, crypto.
- **[16. Extending the Framework](#16-extending-the-framework)** — adding services, commands, pages.
- **[17. Troubleshooting & FAQ](#17-troubleshooting--faq)** — common issues and fixes.

# Detailed Documentation

Each section below is self-contained but cross-links to source paths so you can jump straight to the code.

---

## 1. Architecture Overview

PDI Framework is structured as a layered, ports-and-adapters stack. The lower layers describe **what** a device must be able to do (abstract interfaces); the upper layers describe **what the application offers** (services, transports, web server, CLI). The two are bridged at compile time by a single device-selection macro and a set of `ENABLE_*` feature flags.

### 1.1 Layered model

```
                       ┌────────────────────────────────────────┐
   Application sketch  │  examples/PdiStack/*.ino               │
                       └───────────────────┬────────────────────┘
                                           │ uses
                       ┌───────────────────▼────────────────────┐
   Orchestrator        │  PDIStack  (src/PdiStack.{h,cpp})      │
                       └───────────────────┬────────────────────┘
                                           │ wires up
   ┌───────────────────────────────────────┼───────────────────────────────────────┐
   │                                       │                                       │
   ▼                                       ▼                                       ▼
┌──────────────────┐         ┌──────────────────────────┐          ┌───────────────────────┐
│  Service Layer   │         │  Web Server / CLI / SSH  │          │  Transports           │
│  service_provider│◄────────┤  webserver/, cmd/, shell │◄─────────┤  transports/http,     │
│  /*ServiceProv.  │         │                          │          │  mqtt, smtp           │
└────────┬─────────┘         └──────────────┬───────────┘          └──────────┬────────────┘
         │                                  │                                 │
         │ all consume only                 │                                 │
         ▼                                  ▼                                 ▼
                       ┌──────────────────────────────────────────────────┐
   Utilities           │  src/utility/  (Scheduler, EventUtil, Database,  │
                       │  StringOps, CommandBase, crypto, pdistl, queue,  │
                       │  fiber, Base64, DataTypeConversions, ...)        │
                       └────────────────────────┬─────────────────────────┘
                                                │ + abstract contracts
                                                ▼
                       ┌──────────────────────────────────────────────────┐
   Interface Layer     │  src/interface/pdi/   (iDeviceControlInterface,  │
   (the "ports")       │  iClientInterface, iServerInterface, iWifi*,     │
                       │  iStorage*, iFileSystem*, iSerial*, iGpio*,      │
                       │  iLogger*, iDatabase*, iNtp*, iPing*, iWdt*,     │
                       │  threading: iExecution, iMutex, iCondvar, ...)   │
                       └────────────────────────┬─────────────────────────┘
                                                │ implemented by
                                                ▼
                       ┌──────────────────────────────────────────────────┐
   Device Layer        │  devices/esp32/, esp8266/, arduinouno/,          │
   (the "adapters")    │  mockdevice/   — concrete *Interface.cpp +       │
                       │  <name>_pdi.h aggregator                         │
                       └──────────────────────────────────────────────────┘
```

**Direction of dependency is strictly downward.** A service never includes a device header directly; it talks to an `i*Interface` pointer. The only place device-specific code is reached from above is the aggregator [src/interface/pdi.h](src/interface/pdi.h), which chooses *one* `<device>_pdi.h` based on the `DEVICE_*` macro emitted by `scripts/DeviceSetup.py` into [devices/DeviceSetup.h](devices/DeviceSetup.h).

### 1.2 The five layers

| Layer | Path | Role | Depends on |
|---|---|---|---|
| Application | `examples/` | The user's sketch — calls `PdiStack.initialize()` then `PdiStack.serve()` in `loop()` | Orchestrator |
| Orchestrator | [src/PdiStack.cpp](src/PdiStack.cpp), [src/PdiStack.h](src/PdiStack.h) | Single global `PDIStack PdiStack` that conditionally wires every enabled service | Services + Interfaces |
| Service Layer | [src/service_provider/](src/service_provider/) | One subclass of `ServiceProvider` per feature (WiFi, MQTT, OTA, GPIO, Auth, Cmd, …); registers itself in a global table | Utilities + Interfaces |
| Utilities | [src/utility/](src/utility/) | Reusable primitives with no device knowledge (scheduler, event bus, database engine, crypto, STL subset, fiber) | Interfaces only |
| Interface Layer | [src/interface/pdi/](src/interface/pdi/) | Pure abstract C++ ports (`i*Interface`) grouped into `drivers/`, `middlewares/`, `modules/`, `threading/` | — |
| Device Layer | [devices/](devices/) | Concrete adapters per MCU; each provides a `<name>_pdi.h` aggregator (e.g. [devices/esp32/esp32_pdi.h](devices/esp32/esp32_pdi.h)) that pulls in the relevant `*Interface.h` files for that board | Vendor SDK / Arduino core |

### 1.3 The `ServiceProvider` contract

Every feature service derives from [ServiceProvider](src/service_provider/ServiceProvider.h) and self-registers into a global table indexed by a `service_t` enum:

```cpp
ServiceProvider(service_t st, const char *_svc_name)
    : m_service_t(st), m_service_name(_svc_name), m_service_routine_task_id(-1) {
    m_services[st] = this;          // self-registration
}
```

The base class defines a uniform lifecycle — `initService` (required override, called once by `PDIStack::initialize`), `stopService` (optional teardown), `printConfigToTerminal` / `printStatusToTerminal` (optional; back the `srvc` CLI command), plus a static `getService(st)` lookup so services can find each other without `#include` cycles. Because every provider conforms to this shape, the `srvc` terminal command can enumerate, configure and inspect any service generically.

### 1.4 The interface contract (the "ports")

Interfaces are grouped by *role* rather than by feature:

| Group | Examples | Who implements | Who consumes |
|---|---|---|---|
| `drivers/` | `iGpioInterface`, `iWdtInterface` | Device adapter | GPIO service, framework core |
| `middlewares/` | `iClientInterface`, `iServerInterface`, `iNtpInterface`, `iPingInterface`, `iDeviceControlInterface`, `iUpgradeInterface` | Device adapter | OTA, MQTT, Email, HTTP, IoT services |
| `modules/serial`, `modules/storage`, `modules/wifi` | `iSerialInterface`, `iStorageInterface`, `iFileSystemInterface`, `iWiFiInterface`, `iHttpServerInterface` | Device adapter | Serial, Storage, WiFi services |
| `threading/` | `iExecution`, `iMutex`, `iCondvar`, `iContext` + `cooperative/`, `preemptive/` | Device adapter (if supported) | Task scheduler in contextual mode |
| Top-level | `iDatabaseInterface`, `iLoggerInterface`, `iDeviceIotInterface` | Device adapter (DB, logger) / framework (IoT) | Database service, logger macros, IoT service |

A device port is considered "complete" when it provides at least: `iDeviceControlInterface`, `iDatabaseInterface`, `iSerialInterface` (if `ENABLE_SERIAL_SERVICE`), and any group required by the feature flags it intends to support. See [devices/esp32/esp32_pdi.h](devices/esp32/esp32_pdi.h) for the canonical aggregator pattern — note how each include is itself wrapped in the matching `ENABLE_*` guard so an unused interface costs zero code size.

### 1.5 Compile-time composition (feature gating)

There is **no runtime plugin system**. Every optional capability is selected at compile time through `ENABLE_*` macros declared in [devices/DeviceConfig.h](devices/DeviceConfig.h). Macros cascade: enabling `ENABLE_NETWORK_SERVICE` is what makes `ENABLE_MQTT_SERVICE`, `ENABLE_HTTP_SERVER`, `ENABLE_OTA_SERVICE`, etc. meaningful for that device. Dependencies are encoded as nested `#if` blocks; e.g. SSH is only enabled if storage is available:

```c
#ifdef ENABLE_NETWORK_SERVICE
  #define ENABLE_MQTT_SERVICE
  #define ENABLE_WIFI_SERVICE
  #if defined(ENABLE_STORAGE_SERVICE)
    #define ENABLE_SSH_SERVICE
  #endif
  ...
#endif
```

The same flag drives **three** independent things, in lockstep:

1. Whether the **device aggregator** includes the corresponding `*Interface.h` (see [devices/esp32/esp32_pdi.h](devices/esp32/esp32_pdi.h)).
2. Whether the **orchestrator** includes the corresponding `*ServiceProvider.h` (see [src/PdiStack.h](src/PdiStack.h)).
3. Whether `PDIStack::initialize()` calls the service's `initService(...)` (see [src/PdiStack.cpp](src/PdiStack.cpp)).

That triple-guard is intentional — it lets a feature be removed entirely from a build (zero flash, zero RAM, zero static-init cost) just by commenting one `#define`.

### 1.6 Runtime lifecycle

```
boot
 │
 ├─ static init of PDIStack PdiStack       (src/PdiStack.cpp)
 │     ├─ __utl_event.begin(&__i_dvc_ctrl)
 │     ├─ __task_scheduler.setUtilityInterface(...)
 │     └─ (if WiFi) m_client = __i_instance.getNewT(cp|ls)ClientInstance()
 │                  (TLS picked at compile time via ENABLE_TLS_SERVICE)
 │
setup()
 │
 ├─ PdiStack.initialize()                  (src/PdiStack.cpp)
 │     ├─ __i_dvc_ctrl.initDeviceSpecificFeatures()
 │     ├─ acquire terminal, attach to ServiceProvider base
 │     ├─ __database_service.initService()                (always)
 │     └─ each enabled service .initService(args)         (conditional)
 │
loop()
 │
 └─ PdiStack.serve()                        (src/PdiStack.cpp)
       ├─ __web_server.handle_clients()                    (if enabled)
       ├─ __task_scheduler.run()                           (inline tasks)
       ├─ __i_dvc_ctrl.yield()
       ├─ __i_dvc_ctrl.handleEvents()
       ├─ __i_cooperative_scheduler.tick_from_loop()       (if contextual)
       └─ __i_preemptive_scheduler.yield()                 (if contextual)
```

Two scheduling lanes coexist here:

- **Inline tasks** advance every `serve()` tick — best for periodic, short, non-blocking work.
- **Contextual tasks** (cooperative / preemptive) advance on their own stacks; the loop only *yields* to them. These exist only on devices whose port provides `iExecution` + a matching scheduler under [src/interface/pdi/threading/](src/interface/pdi/threading/).

Section [5. Task Scheduler] will cover the choice between modes in depth.

### 1.7 Global instances and naming

The framework exposes a small set of well-known global symbols, all prefixed with `__`. The naming is uniform so a contributor can grep for any one of them:

| Symbol | Provided by | Purpose |
|---|---|---|
| `__i_dvc_ctrl` | Device layer | The single `iDeviceControlInterface` for this build |
| `__i_db`, `__i_fs`, `__i_wifi`, `__i_http_server`, `__i_ntp`, `__i_ping`, `__i_serial` | Device layer | Interface singletons, only defined when the matching flag is on |
| `__task_scheduler`, `__utl_event` | Utility layer | Cross-service scheduler and event bus |
| `__database_service`, `__wifi_service`, `__mqtt_service`, `__ota_service`, ... | Service layer | One per `ServiceProvider` subclass |
| `__i_cooperative_scheduler`, `__i_preemptive_scheduler` | Threading port | Contextual execution lanes |
| `PdiStack` | Orchestrator | The single application-facing facade |

Services find each other by global symbol when the dependency is fixed (e.g. `__mqtt_service` reads from `__database_service`), and via `ServiceProvider::getService(st)` when a generic lookup is required (e.g. the `srvc` CLI command).

---

## 2. Build & Toolchain

PDI Framework targets the **Arduino IDE / arduino-cli** build system. This section is the integrator's reference: install, board versions, what the scripts do, and the Arduino Library Manager flow. The framework compiles clean against `-std=c++14` (or newer) with GCC variadic-macro extensions — there are no hidden compiler-flag dependencies beyond that, so PlatformIO or a raw `make` build works if you reproduce the standard Arduino-core defines.

### 2.1 Supported boards

| Board | Arduino IDE board package version |
|---|---|
| Arduino UNO | 1.8.6 |
| ESP8266 | 3.1.2 |
| ESP32 | 3.3.3 |

These are the versions the current `devices/<board>/` adapters are written against — vendor SDK headers and Arduino-core APIs move, so newer board packages may break a port until the adapter is updated.

### 2.2 Installation flow

Two paths — Library Manager (recommended) or manual git clone.

**Library Manager (default target: ESP32):**

1. **Install the device's Arduino board package** at the version above through the Boards Manager.
2. **Install pdi-framework** from Tools → Manage Libraries → search for `pdi-framework`.
3. **Open the bundled example** in the IDE: File → Examples → pdi-framework → PdiStack. Select an **ESP32** board, compile, flash.

That's it for ESP32. The library ships with placeholder DB-table headers and a [`__has_include("DeviceSetup.h")`](devices/DeviceConfig.h) guard in `devices/DeviceConfig.h` that falls back to ESP32 when no `DeviceSetup.h` is present, so a fresh Library Manager install builds out of the box for ESP32 with no extra steps.

**To build for ESP8266 or Arduino UNO** (or any other supported device), generate the per-device setup files first:

```
cd <your-Arduino-libraries-path>/pdi-framework/scripts
python3 DeviceSetup.py -d esp8266        # or arduinouno
```

This writes [devices/DeviceSetup.h](devices/DeviceSetup.h) with `#define DEVICE_<NAME>` and regenerates [src/database/tables/](src/database/tables/) for the target. Switching boards later is the same one-liner against the new device name.

**Manual git clone (development / contributor install):**

1. Install the board package (as above).
2. Clone into your Arduino `libraries/` directory:
   ```
   cd ~/Arduino/libraries
   git clone https://github.com/Suraj151/pdi-framework.git
   cd pdi-framework
   ```
   Linux/macOS: `~/Arduino/libraries/`. Windows: `%USERPROFILE%\AppData\Local\Arduino15\packages\<vendor>\hardware\<arch>\<ver>\libraries\` (cross-arch install) **or** `Documents\Arduino\libraries\` (per-user).
3. (Optional) `python3 scripts/DeviceSetup.py -d <board>` if you need a non-ESP32 target.
4. Open the bundled example, select your board, compile, flash.

LittleFS is now vendored in-tree under [external/littlefs/](external/littlefs/) — no submodules to pull.

### 2.3 What the scripts do

| Script | Purpose | When to re-run |
|---|---|---|
| [`scripts/DeviceSetup.py`](scripts/DeviceSetup.py) | Writes `devices/DeviceSetup.h` with `#define DEVICE_<NAME>` ([§14.4](#144-device-selection-flow)) and then calls `CreateDBSourceFromJson.py` | When switching target device |
| [`scripts/CreateDBSourceFromJson.py`](scripts/CreateDBSourceFromJson.py) | Wipes [src/database/tables/](src/database/tables/) and regenerates it from the per-device `DBTableSchema.json` by calling `JsonToCpp.py` ([§5.5](#5-database-layer)) | When changing the schema for the active device |
| [`scripts/JsonToCpp.py`](scripts/JsonToCpp.py) | Generic JSON-to-C++ codegen used by the table generator | Always indirectly |
| [`scripts/Util.py`](scripts/Util.py) | Shared helpers: template substitution, kebab-to-PascalCase, `clang-format` invocation, device-folder path resolution | Library — not invoked directly |
| [`scripts/GenTlsCerts.py`](scripts/GenTlsCerts.py) | OpenSSL-backed off-device TLS cert generator: EC/RSA keypair, multi-DNS/multi-IP SANs, optional CA mode, CSR signing. Output to `certs/`; upload via SFTP to the `TLS_DEFAULT_*_PATH` defaults. On-device equivalent for esp32 is the `tls` CLI command. | When provisioning HTTPS on esp8266 (no on-device gen), or seeding an esp32 cert signed by a stable dev CA |

Generated output goes through `clang-format --style=Microsoft -i` if `clang-format` is on `PATH`, so generated headers look hand-written. If the formatter is missing, files are written un-formatted and still compile fine.

### 2.4 Vendored externals

The framework no longer carries any git submodules. Two external bodies of code are vendored directly in the repo:

- [external/littlefs/](external/littlefs/) — the LittleFS filesystem (vendored, not a submodule). Used by the storage interface on esp8266/esp32 for `ENABLE_STORAGE_SERVICE` and everything downstream of it (SSH/SFTP, CLI history, file commands). AVR builds skip storage so they don't reach this code.
- [lwip/](lwip/) — a vendored copy of customised lwIP 1.4 for the legacy NAPT path (see [§2.4.1 NAT and Mesh](#241-nat-and-mesh) below). The modern path uses the lwIP shipped with the ESP8266 core; this folder is opt-in by manually replacing the board-package's lwIP.

#### 2.4.1 NAT and Mesh

Two board-level networking features rely on the vendored externals and the ESP SDKs. They are **not** exposed as regular `ServiceProvider` classes — they're compile-time / runtime capabilities layered onto WiFi.

**NAT (ESP8266).** Network Address Translation remaps one IP address space into another by rewriting IP-header fields on packets in transit. With NAT active the ESP8266 can extend an upstream (station) network — devices joining the ESP's AP reach the same internet as the STA link.

- From ESP8266 Arduino core **v2.6.x** onwards, an initial NAT example is provided using **lwIP v2** (IPv4 only). This is the modern path and is on by default when a lwIP v2 variant is selected in the Arduino IDE Tools menu.
- The older **NAPT** (network address & port translation) path uses the customised **lwIP 1.4** vendored at [lwip/](lwip/). To test it, rename `…esp8266/tools/sdk/lwip` to `…esp8266/tools/sdk/lwip.org`, copy this repo's `lwip/` folder in its place, and select the "lwIP 1.4 compile from source" variant in the Arduino Tools menu.
- Which path is active is decided at compile time by which lwIP variant the IDE builds against; there is no runtime switch.
- Related runtime knob: `ENABLE_NAPT` in `WiFiServiceProvider` (§6.2.4) — the WiFi service schedules a one-shot NAPT-enable after the STA link comes up.

**Mesh (ESPNOW).** ESP-NOW is the Espressif peer-to-peer link-layer protocol available on esp8266/esp32. The framework wraps it into a small API surface so applications can build **mesh networks, broadcasts, and hop-distance topologies** without touching the raw ESPNOW driver.

- Configuration is done at compile time / from the application; there is no web-portal page for ESPNOW.
- Works well as a companion to WiFi station mode — the same radio serves both.
- Related runtime knobs on the WiFi service: `ENABLE_DYNAMIC_SUBNETTING`, `ENABLE_INTERNET_BASED_CONNECTIONS` (§6.2.4) — used with ESPNOW to build a subnetting hierarchy where each device knows its hop distance from the main hub.

### 2.5 Library Manager: how the ESP32 default works

The library installs cleanly through Arduino Library Manager because three things hold true at first build:

1. **`devices/DeviceSetup.h` is optional.** [`devices/DeviceConfig.h`](devices/DeviceConfig.h) starts with:
   ```c
   #if __has_include("DeviceSetup.h")
   #include "DeviceSetup.h"
   #else
   #define DEVICE_ESP32
   #endif
   ```
   so a fresh install (no setup script run) builds as if `DEVICE_ESP32` were set.
2. **The per-port platform header falls back to ESP32.** The cascade right after the `DEVICE_*` selector ends with `#else #include "esp32/esp32_device_config.h"`, so any unrecognised `DEVICE_*` (or no `DEVICE_*` at all) lands on ESP32's `RODT_ATTR` / `strcat_ro` / `CRITICAL_SECTION_*` definitions.
3. **DB table headers ship as ESP32 placeholders.** [`src/database/tables/`](src/database/tables/) is no longer wiped at install time — the checked-in `*.h` placeholders are ESP32-shaped and let `DatabaseServiceProvider` link without running the codegen.

To build for a non-default board (ESP8266 / Arduino UNO / any future port), run the setup script — the regenerated `DeviceSetup.h` overrides the fallback, and the regenerated table headers replace the placeholders. Switching back to ESP32 later either means re-running `DeviceSetup.py -d esp32` or deleting `devices/DeviceSetup.h` (the `__has_include` fallback then takes over again).

#### 2.5.1 Per-port capability flags

The new `DEVICE_SUPPORTS_*` gates in the per-port `<board>_device_config.h` files keep the device-selection logic in `DeviceConfig.h` board-agnostic:

| Macro | Defined by | Effect |
|---|---|---|
| `DEVICE_SUPPORTS_TLS` | esp8266 / esp32 device-config | Allows `ENABLE_TLS_SERVICE` to actually take effect — DeviceConfig auto-`#undef`s it on ports without this flag |
| `DEVICE_SUPPORTS_CONTEXTUAL_EXECUTION` | esp8266 / esp32 device-config | Same shape as TLS — auto-undef when the port can't host cooperative/preemptive lanes |
| `DEVICE_SUPPORTS_TLS_CERT_GENERATION` | esp32 device-config only | Gates `ENABLE_TLS_CERT_GENERATION` instead of the older `DEVICE_ESP32` hard-coding |
| `MAX_DIGITAL_GPIO_PINS`, `MAX_ANALOG_GPIO_PINS`, `MAX_DB_TABLES` | each per-port device-config | Per-board limits, previously inlined in `DeviceConfig.h`'s `DEVICE_*` cascade |
| `ENABLE_NETWORK_SERVICE`, `ENABLE_AUTH_SERVICE`, `ENABLE_STORAGE_SERVICE`, `ENABLE_GPIO_BASIC_ONLY` | each per-port device-config | Per-board defaults (e.g. AVR omits network/auth/storage; esp* enable them) |

The contract: anything that's *truly* per-board lives in the per-port `<board>_device_config.h`; the central [DeviceConfig.h](devices/DeviceConfig.h) only carries cross-board feature flags and the `DEVICE_SUPPORTS_*` auto-undef chains. A new port just needs to set the right `DEVICE_SUPPORTS_*` macros and the framework's optional services fall in line.

### 2.6 Gotchas

- **The default install is ESP32-only.** Without running the setup script, `devices/DeviceConfig.h`'s `__has_include` fallback selects `DEVICE_ESP32` and the ESP32 placeholder DB tables. If you flash an ESP8266 or AVR build without first running `DeviceSetup.py -d <board>`, the binary still compiles but contains ESP32-shaped table addresses and feature flags — and will misbehave at runtime against the actual hardware. Always run the script when switching off the ESP32 default.
- **Switching boards without re-running setup.** `devices/DeviceSetup.h` keeps the *previous* device macro. Either re-run `DeviceSetup.py -d <new-board>`, or delete `devices/DeviceSetup.h` to fall back to the ESP32 default.
- **Stale generated table headers after a board switch.** `CreateDBSourceFromJson.py` wipes and rewrites `src/database/tables/` for the active target. If you switch from a custom port back to ESP32 and forget to re-run the script, the previously generated headers stay in place — they're git-ignored at that point and won't be restored by `git pull`. Re-run the script (or `git checkout src/database/tables/` to restore the ESP32 placeholders).

---

## 3. Configuration System

Configuration in PDI Framework is **layered, compile-time, and additive**. There is no `.ini`, no runtime config parser, and no over-the-air feature toggling. Every choice — which services exist, how large each table is, what the defaults are, which interfaces a port supplies — is fixed when you build.

This section is the canonical reference for: every `ENABLE_*` flag and what it pulls in, the per-service `*Config.h` files and the structs they declare for the database, and the dependency relationships you must respect when turning features on or off.

### 3.1 Three tiers of configuration

The configuration system has three distinct tiers, each addressing a different concern:

| Tier | Lives in | Owns | Audience |
|---|---|---|---|
| **Device tier** | [devices/DeviceSetup.h](devices/DeviceSetup.h) (autogen) + [devices/DeviceConfig.h](devices/DeviceConfig.h) + each port's `devices/<board>/<board>_device_config.h` | `DEVICE_*` selector, `ENABLE_*` feature flags + per-board limits, and per-port platform macros (`RODT_ATTR`, `PROG_RODT_ATTR`, `strcat_ro` family, `CRITICAL_SECTION_ENTER/EXIT`) | Integrator: "what board, what features"; porter: "how the board names flash / interrupts" |
| **Common tier** | [src/config/Common.h](src/config/Common.h), [src/config/GlobalConfig.h](src/config/GlobalConfig.h) | Cross-cutting constants (`MAX_SCHEDULABLE_TASKS`, default user/pass, base time durations), the always-present `global_config` table | Framework author + integrator |
| **Service tier** | One `*Config.h` per service under [src/config/](src/config/) | Per-service tuning knobs **and** the runtime config struct(s) persisted to NVM | Service author + advanced integrator |

The tiers are pulled in via the single entry point [src/config/Config.h](src/config/Config.h), which conditionally includes each service-tier header based on the `ENABLE_*` flags decided by the device tier. Every translation unit in the framework reaches the configuration system through this one header.

### 3.2 Inclusion graph

```
devices/DeviceSetup.h              (autogenerated: #define DEVICE_<NAME>)
        │
        ▼
devices/DeviceConfig.h             (cascades DEVICE_* into ENABLE_* flags;
        │                           also pulls in devices/<board>/<board>_device_config.h
        │                           for per-port platform macros — RODT_ATTR,
        │                           PROG_RODT_ATTR, strcat_ro family,
        │                           CRITICAL_SECTION_ENTER/EXIT)
        ▼
src/config/Common.h                (defaults that other configs reference)
        │
        ▼
src/config/GlobalConfig.h          (the always-loaded `global_config` table)
        │
        ▼
src/config/Config.h                ── conditional includes ──┐
                                                              │
   ┌────────────────────┬───────────────────┬─────────────────┼─────────────────┐
   ▼                    ▼                   ▼                 ▼                 ▼
WifiConfig.h     ServerConfig.h     HttpConfig.h        StorageConfig.h    OtaConfig.h
                                          ⋮                  ⋮                 ⋮
                              (one per ENABLE_*-gated service)
```

Two implications:

1. A `*Config.h` for a disabled service is **never** included — so the struct it declares does not exist in the build, and the table address it would occupy is reclaimed.
2. Every config file may freely include `Common.h` for shared defaults, but **must not** include other service configs — that would couple two services through configuration. The orchestrator owns ordering.

### 3.3 Device-tier flags — full reference

Every `ENABLE_*` (and `ALLOW_*` / `IGNORE_*` / `AUTO_*`) flag lives in [devices/DeviceConfig.h](devices/DeviceConfig.h) and acts as a triple gate: it controls (1) which interface a device exposes, (2) which service the orchestrator includes, and (3) which `*Config.h` is pulled into `Config.h`.

#### 3.3.1 Service flags

| Flag | Default on | Brings in | Requires | Memory cost (approx) |
|---|---|---|---|---|
| `ENABLE_SERIAL_SERVICE` | yes (all boards) | SerialServiceProvider + `SerialConfig.h` | `iSerialInterface` | low |
| `ENABLE_STORAGE_SERVICE` | yes (default; auto-disabled on UNO) | `StorageConfig.h` + filesystem | `iStorageInterface` + `iFileSystemInterface` | medium (FS overhead) |
| `ENABLE_GPIO_SERVICE` | yes | GpioServiceProvider + `GpioConfig.h` | `iGpioInterface` (via `iDeviceControlInterface`) | low; `ENABLE_GPIO_BASIC_ONLY` trims it further for uno |
| `ENABLE_CMD_SERVICE` | yes | CommandLineServiceProvider | A terminal source (serial / telnet / ssh) | low |
| `ENABLE_AUTH_SERVICE` | non-uno | AuthServiceProvider + `ServerConfig.h` (`login_credential` table) | Storage | low |
| `ENABLE_NETWORK_SERVICE` | non-uno | Umbrella — gates everything below | `iTcpClient/Server`, `iNtp`, `iPing`, `iWiFi` | — |
| `ENABLE_WIFI_SERVICE` | with network | WiFiServiceProvider + `WifiConfig.h` | `iWiFiInterface` | medium |
| `ENABLE_HTTP_SERVER` | with network | WebServer + `ServerConfig.h` + `HttpConfig.h` | `iHttpServerInterface` (or `HttpServerInterfaceImpl`) | medium |
| `ENABLE_HTTPS_SERVER` | off; with `TLS_SERVICE` | Switches `WebServer::initService` to `begin(HTTPS_DEFAULT_PORT=443, secure=true)` with cert/key paths from `TlsConfig.h` | `ENABLE_TLS_SERVICE` (+ optional `ENABLE_HTTPS_SERVER_MTLS` for client-cert auth) | high (TLS handshake + per-session buffers) |
| `ENABLE_HTTP_CLIENT` | with network | HttpHelper + `HttpConfig.h` | `iTcpClientInterface` | low |
| `ENABLE_MQTT_SERVICE` | with network | MqttServiceProvider + `MqttConfig.h` | TCP client | medium |
| `ENABLE_OTA_SERVICE` | with network | OtaServiceProvider + `OtaConfig.h` | TCP client + `iUpgradeInterface` | low |
| `ENABLE_EMAIL_SERVICE` | with network | EmailServiceProvider + `EmailConfig.h` | TCP client | low |
| `ENABLE_TELNET_SERVICE` | with network | TelnetServiceProvider | TCP server | low |
| `ENABLE_SSH_SERVICE` | with network **and** storage | SSHServiceProvider | TCP server + FS | **high** (keys, crypto, SFTP) |
| `ENABLE_DEVICE_IOT` | with network | DeviceIotServiceProvider + `DeviceIotConfig.h` | TCP client | low (app supplies hooks) |
| `ENABLE_TLS_SERVICE` | off | TLS-capable client/server (`iTlsClientInterface` / `iTlsServerInterface`) + `TlsConfig.h`; forces `ENABLE_CONTEXTUAL_EXECUTION` on as TLS runs on a dedicated cooperative task | esp8266 (BearSSL) or esp32 (mbedTLS) backend in the port | **high** — see [§12.4](#124-the-expensive-features-called-out) |
| `ENABLE_TLS_CERT_GENERATION` | off (esp32 only) | `tls` CLI command + `TlsCertProvisioner` (mbedTLS issuer) | `ENABLE_TLS_SERVICE` + esp32 | medium (mbedTLS keygen frames) |
| `ENABLE_SERVER_TLS_CERT_GENERATION_AT_RUNTIME` | off | Listener on `EVENT_WIFI_STA_GOT_IP` that auto-mints a self-signed cert for the device's IP if `/etc/http/server.crt` is missing | `ENABLE_TLS_CERT_GENERATION` | low at idle, one-shot keygen cost on first boot |
| `ENABLE_CONTEXTUAL_EXECUTION` | off (esp8266 + esp32) | Cooperative + preemptive scheduler lanes | `iExecution`, `iMutex`, `iCondvar`, `iContext` + scheduler globals | high (per-task stacks) |
| `ENABLE_TIMER_TASK_SCHEDULER` | off (removed from default header) | Timer-backed task scheduler variant | Device timer | depends |

#### 3.3.2 Behaviour flags

| Flag | Effect |
|---|---|
| `ENABLE_GPIO_BASIC_ONLY` | Reduces GPIO service to digital-only (used on uno) |
| `ENABLE_DYNAMIC_SUBNETTING` | AP subnet/gateway picked at runtime instead of static |
| `ENABLE_NAPT` | Network address & port translation; lets AP clients reach the STA's network. Increases heap |
| `IGNORE_FREE_RELAY_CONNECTIONS` | Skip already-connected SSIDs during scan to avoid loops in mesh |
| `ALLOW_WIFI_CONFIG_MODIFICATION`, `ALLOW_WIFI_SSID_PASSKEY_CONFIG_MODIFICATION_ONLY` | Restrict the WiFi config web form |
| `ALLOW_MQTT_CONFIG_MODIFICATION`, `ALLOW_OTA_CONFIG_MODIFICATION` | Same for MQTT / OTA forms |
| `AUTO_FACTORY_RESET_ON_INVALID_CONFIGS` | If DB checksum invalid at boot, reset to defaults instead of halting |
| `CONFIG_CLEAR_TO_DEFAULT_ON_FACTORY_RESET` | Factory reset writes default structs back instead of zeroing |
| `ENABLE_LOG_ALL` / `_INFO` / `_WARNING` / `_ERROR` / `_SUCCESS` | Per-level log gates; if none are set, the logger interface is dropped entirely |

#### 3.3.3 Per-device limits

| Macro | Purpose | Typical values |
|---|---|---|
| `MAX_DIGITAL_GPIO_PINS` | Size of GPIO config table | 14 (uno), 12 (esp32 WROOM/S2/S3/C6), 8 (esp32 C3/H2), 9 (esp8266) |
| `MAX_ANALOG_GPIO_PINS` | Same, analog side | 5 (uno), 4 (esp32 all variants), 1 (esp8266) |
| `MAX_DB_TABLES` | Upper bound on registered tables | 5 (uno), 15 (esp*) |
| `MAX_SCHEDULABLE_TASKS` | Inline scheduler slot count | 25 (in `Common.h`) |
| `MAX_FACTORY_RESET_CALLBACKS` | Reset-hook count | `MAX_SCHEDULABLE_TASKS` |
| `WIFI_STATION_CONNECT_ATTEMPT_TIMEOUT` | STA connect retry budget (s) | 1 |
| `WIFI_CONNECTIVITY_CHECK_DURATION` | Inter-check interval (ms) | 5000 |
| `INTERNET_CONNECTIVITY_CHECK_DURATION` | Internet recheck cadence | same as above |

### 3.4 The service-tier `*Config.h` pattern

Every per-service config file follows the same shape so the framework can treat them uniformly. Using [WifiConfig.h](src/config/WifiConfig.h) as the canonical example:

```cpp
#define WIFI_CONFIGS_BUF_SIZE 30
#define DEFAULT_SSID         USER          // 1. defaults
#define DEFAULT_PASSPHRASE   PASSPHRASE
const uint8_t DEFAULT_AP_LOCAL_IP[] = {192, 168, 0, 1};
...

#define ALLOW_WIFI_CONFIG_MODIFICATION    // 2. policy switches (optional)

typedef struct { ... } __status_wifi_t;   // 3. runtime-only status (not persisted)
extern __status_wifi_t __status_wifi;

struct wifi_configs {                     // 4. the persisted struct
  wifi_configs() { clear(); }             //    - default-constructible
  void clear() { ... }                    //    - zero/default initialisable
  char sta_ssid[WIFI_CONFIGS_BUF_SIZE];   //    - fixed-size members only (no pointers)
  ...
};

const int wifi_config_size = sizeof(wifi_configs) + 5;  // 5. on-NVM size (+5 = framing)
using wifi_config_table = wifi_configs;                 // 6. canonical table alias
```

The six numbered conventions are part of the contract — the database layer ([§5](#5-database-layer)) and the web forms rely on every config struct providing them.

#### 3.4.1 Why `+5` in `*_config_size`

Each persisted table is wrapped on NVM with a small framing record (table id + checksum). The `+ 5` byte allowance is the framework's fixed overhead per table, summed at boot to compute total NVM usage and validated against `iDatabaseInterface::getMaxDBSize()`.

### 3.5 Per-service config files — what's in each

| File | Always-on table(s) | Headline knobs |
|---|---|---|
| [Common.h](src/config/Common.h) | — | `USER`, `PASSPHRASE`, `MAX_SCHEDULABLE_TASKS`, time durations, HTTP request budget |
| [GlobalConfig.h](src/config/GlobalConfig.h) | `global_config_table` (version, year, firmware) | `CONFIG_START`, `CONFIG_VERSION`, `FIRMWARE_VERSION`, `RELEASE`, `LAUNCH_UNIX_TIME` |
| [WifiConfig.h](src/config/WifiConfig.h) | `wifi_config_table` (STA+AP creds, IPs) | `WIFI_CONFIGS_BUF_SIZE`, default IPs, modification policies |
| [ServerConfig.h](src/config/ServerConfig.h) | `login_credential_table` (web/CLI user) | `SERVER_SESSION_NAME`, `SERVER_COOKIE_MAX_AGE` (300 s) |
| [HttpConfig.h](src/config/HttpConfig.h) | — (transient `http_param_t`) | `HTTP_CLIENT_BUF_SIZE`, request size limits, `HTTPS_DEFAULT_PORT`, `HTTPS_HSTS_MAX_AGE_SECONDS` |
| [TlsConfig.h](src/config/TlsConfig.h) | — | `TLS_IBUF_SIZE` / `TLS_OBUF_SIZE` (per-session record buffers, 2 KB / 1 KB), `TLS_TASK_STACK_SIZE` (6500 B on esp8266 cooperative TLS task), `TLS_TASK_POLL_MS`; default FS paths `TLS_DEFAULT_SERVER_CERT_PATH`, `TLS_DEFAULT_SERVER_KEY_PATH`, `TLS_DEFAULT_CLIENT_CA_PATH`, `TLS_DEFAULT_OUTBOUND_CA_BUNDLE_PATH` |
| [OtaConfig.h](src/config/OtaConfig.h) | `ota_config_table` (host, port, version, freq) | OTA check cadence |
| [MqttConfig.h](src/config/MqttConfig.h) | `mqtt_general_config_table`, `mqtt_lwt_config_table`, `mqtt_pubsub_config_table` | Broker, LWT, publish/subscribe slots, `MQTT_CONFIG_TYPE` selector |
| [GpioConfig.h](src/config/GpioConfig.h) | `gpio_config_table` | Pin map, `GPIO_MODE` enum, event conditions/channels, graph dimensions |
| [EmailConfig.h](src/config/EmailConfig.h) | `email_config_table` | SMTP host/port/auth, default subject |
| [DeviceIotConfig.h](src/config/DeviceIotConfig.h) | `device_iot_config_table` | Config/OTP URLs, channel keys, sampling/keepalive bounds |
| [SerialConfig.h](src/config/SerialConfig.h) | — | `SERIAL_MODE`, baud, interface enum |
| [StorageConfig.h](src/config/StorageConfig.h) | — | Mount point, max path, FS limits |
| [SshConfig.h](src/config/SshConfig.h) | — | `SSHKeyAlgorithm`, key/file size caps |
| [NetworkConfig.h](src/config/NetworkConfig.h) | — | Network-wide timeouts |
| [EventConfig.h](src/config/EventConfig.h) | — | `event_name` enum, channel registry |

> **Pattern note:** the always-on tables (`global_config_table`, `login_credential_table` when auth is on, `wifi_config_table` when wifi is on) form the framework's minimum NVM footprint. Everything else is opt-in via the matching `ENABLE_*`.

### 3.6 Dependency rules (read before flipping a flag)

The most common foot-gun is enabling a service whose dependency is off. The rules:

- `ENABLE_HTTP_SERVER` ⇒ `ENABLE_WIFI_SERVICE` (web server must run somewhere) ⇒ `ENABLE_NETWORK_SERVICE`.
- `ENABLE_MQTT_SERVICE` / `ENABLE_OTA_SERVICE` / `ENABLE_EMAIL_SERVICE` / `ENABLE_DEVICE_IOT` ⇒ `ENABLE_NETWORK_SERVICE` (need TCP client) and *usually* `ENABLE_WIFI_SERVICE` (need a network).
- `ENABLE_SSH_SERVICE` ⇒ `ENABLE_NETWORK_SERVICE` **and** `ENABLE_STORAGE_SERVICE` (key persistence, SFTP).
- `ENABLE_TELNET_SERVICE` ⇒ `ENABLE_NETWORK_SERVICE` and benefits from `ENABLE_CMD_SERVICE` (no CLI = no terminal value).
- `ENABLE_AUTH_SERVICE` ⇒ `ENABLE_STORAGE_SERVICE` (credentials need to survive reboot) — though framed via the always-present DB.
- `ENABLE_DEVICE_IOT` ⇒ the **application** must implement `iDeviceIotInterface` (see [§13.3.5](#13-portable-interfaces)) and pass its instance to `__device_iot_service.initService(...)`.
- `ENABLE_CONTEXTUAL_EXECUTION` ⇒ the port supplies the full threading interface family ([§13.3.6](#13-portable-interfaces)) — both esp8266 and esp32 ship this today.
- `ENABLE_TLS_SERVICE` ⇒ implicitly turns on `ENABLE_CONTEXTUAL_EXECUTION` (TLS runs on its own cooperative task — see [TlsConfig.h](src/config/TlsConfig.h)). Cannot coexist with `ENABLE_NAPT` on esp8266 (both want too much heap). Auto-`#undef`ed on UNO along with storage and contextual execution.
- `ENABLE_HTTPS_SERVER` ⇒ `ENABLE_TLS_SERVICE` **and** `ENABLE_HTTP_SERVER` **and** `ENABLE_STORAGE_SERVICE` (cert/key are loaded from FS at `/etc/http/server.{crt,key}`). Optional `ENABLE_HTTPS_SERVER_MTLS` adds `/etc/http/client-ca.crt` mTLS verification.
- `ENABLE_TLS_CERT_GENERATION` ⇒ `ENABLE_TLS_SERVICE` **and** `DEVICE_ESP32` (mbedTLS issuer API). The `tls` CLI command is gated on this flag.
- `ENABLE_SERVER_TLS_CERT_GENERATION_AT_RUNTIME` ⇒ `ENABLE_TLS_CERT_GENERATION`. Registers a listener on `EVENT_WIFI_STA_GOT_IP` that mints a self-signed cert for the device IP the first time it boots without one.
- Disabling **all** `ENABLE_LOG_*` removes `iLoggerInterface` from the build — your port may then omit its `LoggerInterface.{h,cpp}`.

The header guards in [devices/DeviceConfig.h](devices/DeviceConfig.h) enforce most of these by structurally wrapping the dependent flags inside `#ifdef ENABLE_NETWORK_SERVICE`. The rest you are responsible for.

### 3.7 Common build shapes

A few preset configurations contributors typically reach for:

| Goal | Flags to keep | Flags to drop |
|---|---|---|
| **Smallest possible** (uno-class) | `SERIAL`, `CMD`, `GPIO_BASIC_ONLY` | Everything else |
| **Offline gateway** | Add `STORAGE`, `AUTH` | Everything network |
| **Headless networked node** | Add `NETWORK`, `WIFI`, `MQTT`, `OTA`, `NTP` | `HTTP_SERVER`, `SSH`, `EMAIL` |
| **Full portal** | Default `DeviceConfig.h` (everything on) | — |
| **Diagnostics build** | Default + `ENABLE_LOG_ALL` | — |
| **Concurrent demo** (esp8266 or esp32) | Default + `ENABLE_CONTEXTUAL_EXECUTION` | — |
| **HTTPS portal** | Default + `ENABLE_TLS_SERVICE` + `ENABLE_HTTPS_SERVER` (esp32 also: `ENABLE_TLS_CERT_GENERATION`) | `ENABLE_NAPT` on esp8266 |
| **HTTPS + mTLS** | Above + `ENABLE_HTTPS_SERVER_MTLS` | — |

### 3.8 Changing defaults vs persisted values

A frequent point of confusion: editing `DEFAULT_SSID` in `WifiConfig.h` does **not** change the SSID a running device uses. The defaults are only consulted on:

1. First boot (NVM empty / checksum invalid).
2. Factory reset (if `CONFIG_CLEAR_TO_DEFAULT_ON_FACTORY_RESET` is set).

To change a live value, edit it through the web portal, the CLI (`net connsta`, `iot sethost`, etc.), or directly via `DatabaseServiceProvider::set(...)` and persist the table. The defaults govern the **fallback** state, not the **current** state.

### 3.9 Anti-patterns

Avoid:

- Adding runtime branches on `ENABLE_*` macros — they are compile-time. Use `#ifdef` instead, since the unreached branch otherwise pulls symbols the link will fail on.
- Putting a `std::string` / `pdiutil::string` or any pointer in a `*_config_table` struct — NVM serialisation requires fixed-size, POD layout.
- Cross-including `*Config.h` files. If a value is shared, hoist it to `Common.h`.
- Hard-coding feature checks in services against device macros (`#if defined(DEVICE_ESP32)`) — gate on the *capability* flag (`ENABLE_WIFI_SERVICE`), not the *board*. The board may change; the capability is what the code actually needs.

---

## 4. Task Scheduler

The scheduler is the framework's pacemaker. Every periodic background activity — NTP refresh, MQTT reconnect, OTA poll, GPIO blink, web session expiry, watch-dog feed, log heartbeat — runs through it. There is one inline scheduler that every device gets for free, plus two optional contextual lanes (cooperative and preemptive) that a port may add by implementing the threading interfaces from [§13.3.6](#13-portable-interfaces).

Implementation: [src/utility/TaskScheduler.h](src/utility/TaskScheduler.h), [src/utility/TaskScheduler.cpp](src/utility/TaskScheduler.cpp). Global instance: `__task_scheduler`.

### 4.1 Three task modes

| Mode | Enum value | Where the work runs | Stack used | Needs port support? | Use when |
|---|---|---|---|---|---|
| `TASK_MODE_INLINE` | `0` (default) | Inside `__task_scheduler.run()` on the main loop's stack | Main `loop()` stack | No | The task is short and non-blocking — periodic polls, timeouts, blink ticks. **This is the default for nearly everything.** |
| `TASK_MODE_COOPERATIVE` | `1` | On a dedicated stack via `__i_cooperative_scheduler.run()`, advancing only when the task explicitly `yield()`s or `sleep()`s | Per-task stack you size | Yes — port must provide `iCooperativeScheduler` | The task wants to *appear* sequential (`while(1) { do; sleep(500); }`) but you accept that one misbehaving task can starve others. |
| `TASK_MODE_PREEMPTIVE` | `2` | On a dedicated stack interrupted by a timer ISR via `__i_preemptive_scheduler.yield()` | Per-task stack you size | Yes — port must provide `iPreemptiveScheduler`, `iContext`, tick ISR | The task may block or run long and you cannot trust it to yield. Higher overhead per switch. |
| `TASK_MODE_MAX` | `3` | — | — | — | Sentinel; do not use |

All three modes coexist in the same process. `PDIStack::serve()` ticks each lane in order ([src/PdiStack.cpp](src/PdiStack.cpp)):

```cpp
__task_scheduler.run();                 // inline lane
__i_dvc_ctrl.yield();
__i_dvc_ctrl.handleEvents();
#ifdef ENABLE_CONTEXTUAL_EXECUTION
__i_cooperative_scheduler.tick_from_loop();  // cooperative lane (yields one slice per serve())
__i_preemptive_scheduler.yield();       // preemptive lane (only yields; ISR-driven)
#endif
```

> `tick_from_loop()` exists on every `iExecutionScheduler` since the TLS work landed — for cooperative schedulers it forwards to `run()` (one slice from the main loop), and preemptive schedulers leave it as a no-op because they're driven by a hardware timer. The orchestrator now calls `tick_from_loop()` instead of `run()` so any port that needs a different main-loop entry point (timer-only, deferred-tick, etc.) can override just that hook.

### 4.2 Task policies (scheduling discipline within a mode)

Independent of mode, each task carries a `task_policy_t` that influences which **inline** task runs first when several are due at the same tick:

| Policy | Enum | Selection rule |
|---|---|---|
| `TASK_POLICY_FIFO` | `0` (default) | Priority biased with logarithmic lateness aging boost (base `policy_boost = 320`, `policy_cap = 50`). |
| `TASK_POLICY_ROUNDROBIN` | `1` | Equal time slices — favours tasks that ran least recently (log-scaled with `320` multiplier). |
| `TASK_POLICY_DEADLINE` | `2` | Earliest deadline first — boost = `2.0 × 320`. |
| `TASK_POLICY_FAIRSHARE` | `3` | Favours tasks that have consumed the **least** cumulative CPU time so far. Boost = `1.5 × 320`. |

The scoring formula is in [TaskScheduler::computeScore](src/utility/TaskScheduler.cpp). All four share the base term `effective_priority × 100`, then add a scaled logarithmic policy boost (`policy_boost × min(floor(log2(ms + 1)), policy_cap)` where `policy_boost = 320`). This guarantees that no low-priority task waits longer than **5 seconds** before its accumulated policy score overcomes high-priority priority offsets and grants an execution turn.

> Policies apply to the inline lane. Contextual lanes have their own per-implementation scheduling.

### 4.3 The `task_t` record

Defined in [src/utility/DataTypeDef.h](src/utility/DataTypeDef.h). A POD carrying everything the scheduler needs to make scoring, signalling, and observability decisions. Stored by value in `pdiutil::vector<task_t>` reserved to `MAX_SCHEDULABLE_TASKS` slots — no heap churn during steady-state.

Every member uses the `m_` prefix. Grouped for readability:

| Group | Fields | Purpose |
|---|---|---|
| identity | `m_task_id`, `m_name`, `m_owner` | PID (unique per slot), human-readable name (RO ptr — may be PROGMEM), owning session id (0 = kernel) |
| callback | `m_task` | The `CallBackVoidArgFn` to invoke |
| schedule | `m_duration`, `m_last_millis`, `m_max_attempts`, `m_task_priority`, `m_nice`, `m_task_policy`, `m_task_mode` | Interval / last run / remaining attempts / base priority / POSIX nice (-20..19) / policy (FIFO/RR/DEADLINE/FAIRSHARE) / mode (INLINE/COOP/PREEMPT) |
| lifecycle state | `m_state`, `m_pending_sig` | READY / RUNNING / SLEEPING / STOPPED / ZOMBIE; queued signal (SIG_NONE / HUP / KILL / TERM / CONT / STOP) consumed at the top of `handle_tasks` |
| observability | `m_created_ms`, `m_run_count`, `m_task_exec_us`, `m_total_exec_us` | Registration timestamp (ms), how many times the callback has fired, last exec time in µs, cumulative exec time in µs — powers `ps` / `top` %CPU |
| contextual | `m_task_exec` (ifdef `ENABLE_CONTEXTUAL_EXECUTION`) | Pointer to the per-lane executive |

Exec-time bookkeeping is µs-granular via [`iUtilityInterface::micros_now()`](src/utility/iUtilityInterface.h) — see §14 for port hooks. Callbacks finishing under 1 ms are counted correctly (they don't just round to zero).

### 4.4 Public API by use case

Every registration entry point (`register_task`, `setInterval`, `setTimeout`, `updateInterval`) takes trailing `const char* name = nullptr`, `uint8_t owner = 0` — populate them to make the task visible in `ps` / `top` and reachable by `pkill NAME`. Service subclasses normally don't call these directly; they use the `ServiceProvider` wrappers (see §6.1) which auto-supply the service's name and owner=0.

**Registration & scheduling**

| Want to… | Call | Returns | Notes |
|---|---|---|---|
| Run something once after N ms | `setTimeout(fn, dur, now, prio, name, owner)` | task id | Sets `m_max_attempts = 1`; task is auto-removed after firing |
| Run something every N ms | `setInterval(fn, dur, now, prio, name, owner)` | task id | `m_max_attempts = -1` |
| Reschedule an existing timeout | `updateTimeout(id, fn, dur, now)` | id | Replaces the callback/duration in place |
| Reschedule an existing interval | `updateInterval(id, fn, dur, prio, last, maxAtt, name, owner)` | id | Falls back to `register_task` if id not found |
| Register a task with everything spelled out | `register_task(fn, dur, prio, last, maxAtt, name, owner)` | id | The lowest-level entry point |
| Cancel a one-shot | `clearTimeout(id)` | bool | Marks the task for removal next sweep (see §4.7) |
| Cancel a periodic | `clearInterval(id)` | bool | Same |
| Look up a task | `get_task(id)` | `task_t*` or `nullptr` | Useful to mutate `m_task_policy` / `m_task_mode` after registration |
| Force a re-sort mid-sweep | `rebaseAndRestartPrioTasks()` | void | Breaks out of the current `handle_tasks` loop on the next iteration |

**Metadata mutation (used by `ps` / `renice` / `srvc`)**

| Want to… | Call | Returns | Notes |
|---|---|---|---|
| Attach / change a task's name after registration | `setTaskName(id, name)` | bool | Name pointer must outlive the task (RO literal / PROGMEM). Backs `pkill` name lookup |
| Attach an owning session id | `setTaskOwner(id, sid)` | bool | Used by kill's owner check; 0 = kernel |
| Change POSIX nice (-20..19) | `setTaskNice(id, nice)` | bool | Clamped in range. Follow with `rebaseAndRestartPrioTasks` for immediate re-sort |
| Read the owning session id | `getTaskOwner(id)` | `uint8_t` | Returns 0 if not found |

**Signals (§4.5 details how they're consumed)**

| Want to… | Call | Returns | Notes |
|---|---|---|---|
| Queue a signal on a specific task | `sendSignal(id, sig)` | bool | Consumed on the next `handle_tasks` iteration |
| Broadcast a signal to every task with a given name | `sendSignalByName(name, sig, requester_sid, is_root)` | uint16_t hits | Applies owner check per task unless `is_root`. Used by `pkill` / `killall` / `srvc stop` |

**Rendering**

| Want to… | Call | Returns | Notes |
|---|---|---|---|
| Print POSIX-style ps view | `printPsToTerminal(t, filter_owner=0xFF)` | void | Backs the `ps` / `top` CLI commands. Filter by owner sid or `0xFF` for all |
| Promote a task to a contextual lane | `scheduleUnderExecSched(sched, id, mode, stackdepth)` | int (negative on failure — typically `-1`/`-99` if the port doesn't supply the scheduler or the task couldn't be queued) | Requires `ENABLE_CONTEXTUAL_EXECUTION` |

`register_task` returns `-1` if the slot table is full. Callers must check the return value before assuming the task was accepted.

### 4.5 How a tick is processed

`__task_scheduler.run()` calls `handle_tasks()`, which on every tick:

1. Reads `now = m_util->millis_now()`.
2. Builds an array of indices and sorts it via `getSortedTaskList` using `computeScore` plus a 3 ms tolerance window on due-times. `computeScore` folds `m_nice` in as `effective_priority = m_task_priority - m_nice` before weighting by policy. **The vector itself is not reordered** — only an indirection table — so task ids stay stable.
3. Iterates the sorted indices, skipping any task whose `m_task_mode != TASK_MODE_INLINE` when contextual execution is compiled in.
4. **Consumes `m_pending_sig` before scheduling.** Under one critical section: the sig is cleared, then dispatched as
   - `SIG_KILL` / `SIG_TERM` → task marked ZOMBIE, callback nulled, `m_max_attempts = 0`; iteration `continue`s.
   - `SIG_STOP` → `m_state = TASK_STATE_STOPPED`.
   - `SIG_CONT` → if currently STOPPED, `m_state = TASK_STATE_SLEEPING`.
   - `SIG_HUP` → accepted but currently no-op (reserved for a future user handler hook).
5. If `m_state == TASK_STATE_STOPPED` after signal consumption, skip this task's execution — it stays in the slot but the scheduler ignores it until a `SIG_CONT` arrives.
6. For each due task (`now - m_last_millis ≥ m_duration`), samples `m_util->micros_now()` immediately **before and after** the callback, invokes it, updates
   - `m_task_exec_us = end - start` (µs delta — sub-ms callbacks now register non-zero),
   - `m_total_exec_us +=` that delta (lifetime accumulator; feeds `%CPU`),
   - `m_run_count++`,
   - `m_state` flipping `SLEEPING → RUNNING → SLEEPING` around the callback (or `→ ZOMBIE` if this was the last attempt).
7. Advances `m_last_millis` by whole multiples of `m_duration` (catch-up, capped at 3 rounds to avoid runaway after a long stall) and decrements `m_max_attempts` if finite.
8. Calls `m_util->yield()` after each task so the platform can service WiFi / interrupts.
9. If `m_rebase_start_priotask` was set (either by another task calling `rebaseAndRestartPrioTasks` or implicitly at the top of `handle_tasks`), breaks out of the loop after running **one** task — the next tick will re-sort. This guarantees a newly-registered high-priority task is picked up on the very next tick.
10. Calls `remove_expired_tasks` to drop everything with `m_max_attempts == 0`.

The "run-one-then-break" pattern (point 9) is the key reason the scheduler keeps low jitter without preemption: each `serve()` iteration runs at most one inline task before yielding back to the platform, the web server, and the contextual lanes.

### 4.6 Choosing a mode — decision tree

```
Does your task do real work for more than a few ms,
OR call a blocking SDK API (TLS handshake, large flash write)?
│
├─ No  → TASK_MODE_INLINE  (the default; no port effort)
│
└─ Yes → Is the work sequential / stateful enough that you'd write
         it as `while(1){ step(); sleep(N); }` if you could?
         │
         ├─ Yes → TASK_MODE_COOPERATIVE
         │        (you control yields; cheap context switches;
         │         starvation possible — peer tasks rely on you)
         │
         └─ No, it might block unpredictably → TASK_MODE_PREEMPTIVE
                  (timer ISR forces switches; safe against badly
                   behaved code; higher per-switch overhead;
                   needs `iContext` + tick ISR from the port)
```

If your port does not provide the contextual interfaces, **the only honest choice is INLINE plus splitting the work into a state machine**. Calling `scheduleUnderExecSched` with a missing scheduler is a no-op.

### 4.7 Threading and lifecycle gotchas

A few sharp edges worth knowing before you use the API in anger:

- **`remove_task` does not erase immediately.** It marks the task by zeroing `m_task`, dropping priority, setting `m_max_attempts = 0`, and flipping `m_state = TASK_STATE_ZOMBIE`; the actual `vector::erase` happens in `remove_expired_tasks` at the *end* of `handle_tasks`. This is intentional — erasing during iteration was a bug source ([TaskScheduler.cpp](src/utility/TaskScheduler.cpp)). Don't dereference an id immediately after `clearInterval`.
- **Tasks are stored by value.** Lambdas that capture a `this` pointer or large state work fine, but be mindful that captures live until the task is removed *and* the next sweep runs.
- **The scheduler is not re-entrant.** Don't call `__task_scheduler.run()` from inside a task. Use the contextual lanes if you need nested execution.
- **`m_last_millis = 0` means "first run".** On the first dispatch the scheduler stamps `m_last_millis = now` instead of doing catch-up, so a freshly registered interval will fire after one full `m_duration` from the moment it is first considered.
- **Catch-up is capped at 3 rounds.** A scheduler that has been frozen for >3× the task duration will resume fresh rather than firing all the back-runs. Long blocks (OTA, deep sleep) should re-register their tasks if they need exact-count semantics.
- **`MAX_SCHEDULABLE_TASKS` defaults to 25** ([src/config/Common.h](src/config/Common.h)). `PDIStack::PDIStack()` configures the scheduler with that limit. If you raise it, every service's worst-case footprint grows along with `MAX_FACTORY_RESET_CALLBACKS`.
- **No clock skew tolerance beyond 3 ms.** If your `iUtilityInterface::millis_now()` jitters more than that, you will see tasks reshuffle order between runs. The 3 ms window in `getSortedTaskList` is per-comparison, not per-tick.

### 4.8 Contextual scheduling — extra API

When `ENABLE_CONTEXTUAL_EXECUTION` is on, the inline scheduler **registers** the task (you get an id) and then a second call **promotes** it to a contextual lane:

```cpp
int tid = __task_scheduler.register_task([&](){
    uint32_t i = 0;
    while (1) {
        // long-running body, allowed to sleep
        __i_dvc_ctrl.getTerminal()->writeln((uint32_t)i++);
        __i_cooperative_scheduler.sleep(500);  // yield to peers
    }
});
__task_scheduler.scheduleUnderExecSched(
    &__i_cooperative_scheduler,
    tid,
    TASK_MODE_COOPERATIVE,
    1 * 1024                  // per-task stack bytes
);
```

Stack sizing is your responsibility — there is no automatic measurement unless the port supports `iUtilityInterface::can_measure_stack()`. A safe starting point on esp8266 is 1 KiB cooperative / 2 KiB preemptive; bump it if your task uses `pdiutil::string`, JSON parsing, or TLS.

Cross-lane primitives:

- `iCooperativeScheduler::sleep(ms)` / `::yield()` — must be called from inside the cooperative task; the inline lane's `sleep()` is a no-op by design ([TaskScheduler.cpp](src/utility/TaskScheduler.cpp)).
- `iPreemptiveScheduler::yield()` — voluntary yield from a preemptive task; the tick ISR will preempt it anyway.
- `iMutex` / `iConditionVar` — required whenever data crosses lanes. The inline lane does not need them.

### 4.9 Inspecting and tuning at runtime

Every runtime tool below is available at the shell — no debugger, no recompile.

**Observability**

- **`ps`** — POSIX-style view of every registered task: PID, owner session, state (`R` running / `S` sleeping / `T` stopped / `Z` zombie), priority, nice, policy (`F`/`R`/`D`/`S`), rolling `%CPU`, run count, interval, name. Filter by owner with `ps <sid>`. Source: [printPsToTerminal](src/utility/TaskScheduler.cpp).
- **`top`** — same view re-rendered on a scheduler tick (`top i=<ms>; n=<iters>; u=<sid>`), clearing the screen each pass. Stops with Ctrl+C. Source: [TopCommand](src/service_provider/cmd/commands/TopCommand.h).
- **`watch`** — general periodic wrapper (`watch c=<cmd>; i=<ms>; n=<iters>`); use for anything not already tied to a scheduler view.

**Signals & lifecycle**

Task-scheduler analogues of POSIX signals: `SIG_HUP=1`, `SIG_KILL=9`, `SIG_TERM=15`, `SIG_CONT=18`, `SIG_STOP=19` (see `enum Signal` in [DataTypeDef.h](src/utility/DataTypeDef.h)). Delivery is queued on `m_pending_sig` and consumed at the top of the next `handle_tasks` iteration under one critical section.

- **`kill [<sig>] <PID>`** — send by PID. Default sig is `SIG_TERM` (15). 1 arg = pid, 2 args = sig then pid. SIG_HUP is accepted but currently no-op.
- **`pkill [<sig>] <NAME>`** — send by task name. Default TERM. Prints `signaled N task(s)`.
- **`killall [<sig>] <NAME>`** — same as `pkill` but default is SIG_KILL (impolite).
- **`renice <nice> <PID>`** — change POSIX nice (-20..19) on a live task; auto-triggers `rebaseAndRestartPrioTasks` so the new sort order takes effect on the next tick.

Permission model on all four: root (uid=0) can touch any task; other users only tasks whose `m_owner` matches the current session id. Compiled out entirely when `ENABLE_AUTH_SERVICE` is off.

**Services (systemd-lite)**

- **`srvc list`** — every registered service with state (`inactive` / `active` / `stopped` / `dead`) and task counts.
- **`srvc status <name>`** — per-service detail: state, tracked PID list.
- **`srvc start <name>`** — SIG_CONT every task owned by the service.
- **`srvc stop <name>`** — SIG_STOP every task owned. Task-level freeze, not resource release.
- **`srvc restart <name>`** — STOP then CONT.

`start` / `stop` / `restart` require root. Service ownership of tasks lives in `ServiceProvider::m_service_task_ids[]` (see §6.1), so name matching is by _service_, not by task name — renaming a task in the future won't break lifecycle.

---

## 5. Database Layer

The database layer persists framework configuration to non-volatile memory. It is **not** a general-purpose KV store, a relational DB, or a filesystem; it is a fixed-address, fixed-size, type-safe table store, sized to fit in a few KB of EEPROM-emulated flash. Every persisted struct from [§3](#3-configuration-system) reaches NVM through it.

It is split across the same three-tier shape the rest of the framework uses:

| Tier | Component | Path | Role |
|---|---|---|---|
| Port | `iDatabaseInterface` + concrete `DatabaseInterface` | [src/interface/pdi/iDatabaseInterface.h](src/interface/pdi/iDatabaseInterface.h), `devices/<board>/DatabaseInterface.*` | Raw NVM read/write/checksum for one block at an address |
| Engine | `Database` + `DatabaseTable<addr, T>` | [src/utility/Database.{h,cpp}](src/utility/Database.h), [src/database/core/DatabaseTable.h](src/database/core/DatabaseTable.h) | Table registry, address-overlap checks, typed `get/set/clear` |
| Service | `DatabaseServiceProvider` | [src/service_provider/database/DatabaseServiceProvider.h](src/service_provider/database/DatabaseServiceProvider.h) | Public `get_*_table` / `set_*_table` APIs, factory-reset wiring |

The per-table classes themselves are **autogenerated** from a JSON schema by [scripts/CreateDBSourceFromJson.py](scripts/CreateDBSourceFromJson.py) — they live in [src/database/tables/](src/database/tables/) and should never be hand-edited.

### 5.1 Mental model

NVM is treated as a flat byte address space owned by the port:

```
NVM (size = __i_db.getMaxDBSize())
0          5                                                                  end
├──────────┼──────────┬──────────┬──────────┬──────────┬──────────┬──── ... ──┤
│ reserved │ Global   │ Login    │ WiFi     │ OTA      │ Gpio     │           │
│  prefix  │  @5      │  @50     │  @150    │  @300    │  @500    │  unused   │
└──────────┴──────────┴──────────┴──────────┴──────────┴──────────┴──── ... ──┘
                ↑                                                            ↑
        CONFIG_START                                          getMaxDBSize() - footer
```

Each table occupies `[address, address + sizeof(T) + 5)` — `+5` is the per-table framing the port writes (table id byte + 4-byte CRC/checksum). Addresses are **hard-coded** per table in the autogenerated header; the engine's only address-time check is "does this table overlap or run past the limit". There is no allocator, no relocation, no defragmentation.

### 5.2 The port — `iDatabaseInterface`

[iDatabaseInterface.h](src/interface/pdi/iDatabaseInterface.h). The device-side contract: open/erase/validate the NVM region, report its size, plus three implementation-required template methods (`saveConfig<T>` / `loadConfig<T>` / `clearConfig<T>`) that each port defines inline in `DatabaseInterface.h` — templates can't be virtual, so the base declares them only as commented contracts and every port must supply them. Singleton: `extern DatabaseInterface __i_db;`.

### 5.3 The engine — `Database` and `DatabaseTable<addr, T>`

The engine is purely an in-RAM registry of `struct_tables { addr, size, instance* }` records — it owns no bytes itself.

#### `DatabaseTableAbstractLayer`
Every table inherits this. Its constructor self-registers the table into a static array, bounded by `MAX_TABLES = MAX_DB_TABLES` ([Database.h](src/utility/Database.h)) — so tables are visible to the engine *before* `main()` runs. No dynamic dispatch needed for registration.

#### `DatabaseTable<uint16_t addr, class Table>`
The CRTP-style template each generated class inherits from:

```cpp
class WiFiTable : public DatabaseTable<WIFI_CONFIG_TABLE_ADDRESS, wifi_config_table> {};
extern WiFiTable __wifi_table;
```

This gives every table `boot()` (self-registration into `__database.m_database_tables`, called once by `Database::init_database`) plus `get(T*)` / `set(T*)` / `clear()` — thin typed wrappers that dispatch to `__i_db.{loadConfig,saveConfig,clearConfig}<T>(addr, ...)`.

#### `Database` (singleton `__database`)
Holds the registry vector and enforces three rules during `register_table`:

1. The new table's `address` must be **greater than** `last_table.address + last_table.size + 2` (no overlap, with a 2-byte gap).
2. The new table's tail (`address + size + 2`) must be **less than** `m_max_db_size`.
3. Total registered tables must be `≤ MAX_DB_TABLES`.

Violation: `register_table` returns `false` and the table is silently skipped. Callers must verify, but the typical flow (auto-registration at boot) just leaves the table unavailable — a hint to bump `MAX_DB_TABLES` in `DeviceConfig.h` or compact the address map.

`clear_all()` iterates every registered table and calls its `clear()` — used by the factory-reset path.

### 5.4 The service — `DatabaseServiceProvider`

`DatabaseServiceProvider` ([source](src/service_provider/database/DatabaseServiceProvider.h)) is what the rest of the framework actually calls. It does three things:

1. **Defines the global table singletons.** `GlobalTable __global_table;`, `WiFiTable __wifi_table;`, … each guarded by the matching `ENABLE_*` flag so disabled features cost zero ([DatabaseServiceProvider.cpp](src/service_provider/database/DatabaseServiceProvider.cpp)).
2. **Owns the boot sequence** in `initService` ([](src/service_provider/database/DatabaseServiceProvider.cpp)):
   ```cpp
   __i_db.beginConfigs(__i_db.getMaxDBSize());     // open NVM
   __database.init_database(__i_db.getMaxDBSize());// reserves vector, calls boot() on every table
   if (AUTO_FACTORY_RESET_ON_INVALID_CONFIGS) {
       __task_scheduler.setInterval([&]{
           if (!__i_db.isValidConfigs()) {
               clear_default_tables();
               __factory_reset.factory_reset();
           }
       }, 5000, now);
   }
   if (CONFIG_CLEAR_TO_DEFAULT_ON_FACTORY_RESET) {
       __utl_event.add_event_listener(EVENT_FACTORY_RESET, [&]{ clear_default_tables(); });
   }
   ```
3. **Exposes typed `get_*_table` / `set_*_table` pairs**, one per persisted struct. These are thin pass-throughs to `__<name>_table.get/set` and are the **only** public surface other services should touch.

> Services should never call `__i_db` directly or hold a raw `__<name>_table` pointer. Going through `__database_service` keeps the persistence boundary one-way: services depend on the service, not on the port or codegen layout.

### 5.5 The autogen pipeline

Per-board tables come from a per-device schema, not the framework root:

```
devices/<board>/config/DBTableSchema.json     ← edit this to add/move a table
        │
        │  scripts/DeviceSetup.py -d <board>
        │      → invokes scripts/CreateDBSourceFromJson.py
        │           → invokes scripts/JsonToCpp.py
        ▼
src/database/tables/<TableName>.h             ← generated; do not edit
```

The schema's `defItems` list each table — each entry names the C++ class, the backing struct alias, the address macro + numeric offset, and the `extern` global the framework will link against. The full schema template (class shape, parent `DatabaseTable<addr, type>`, header includes, post-declaration `extern`) lives at the top of [devices/esp32/config/DBTableSchema.json](devices/esp32/config/DBTableSchema.json); copy an existing `defItems` entry when adding a table. A generated table is always a `final-shape, no-member` subclass of `DatabaseTable<…>` — all logic lives in the template.

#### 5.5.1 Default address map (esp* schema)

| Table | Address | Backing struct | Service flag |
|---|---|---|---|
| Global | `CONFIG_START` (5) | `global_config_table` | always |
| Login | 50 | `login_credential_table` | `AUTH` ∥ `HTTP_SERVER` |
| WiFi | 150 | `wifi_config_table` | `WIFI` |
| OTA | 300 | `ota_config_table` | `OTA` |
| GPIO | 500 | `gpio_config_table` | `GPIO` |
| MQTT General | 700 | `mqtt_general_config_table` | `MQTT` |
| MQTT LWT | 1500 | `mqtt_lwt_config_table` | `MQTT` |
| MQTT PubSub | 1700 | `mqtt_pubsub_config_table` | `MQTT` |
| Email | 2000 | `email_config_table` | `EMAIL` |
| Device IoT | 2600 | `device_iot_config_table` | `DEVICE_IOT` |

The gaps between addresses are deliberate growth headroom for the backing struct — if you add a field to `wifi_configs`, the WiFi table can grow up to `300 - 150 - 5 = 145` bytes before colliding with OTA. Going beyond requires moving OTA (and migrating).

### 5.6 Boot sequence (end-to-end)

```
static-init time
    each `<Table>Table __<name>_table;` global is constructed
        DatabaseTableAbstractLayer ctor pushes `this` into m_instances[]

setup() → PdiStack.initialize() → __database_service.initService()
    __i_db.beginConfigs(maxSize)              // port mounts NVM
    __database.init_database(maxSize)
        m_database_tables.reserve(MAX_TABLES)
        for each pre-registered instance:
            instance->boot()
                register_table({addr, sizeof(T), this})
                    if (no overlap && fits in maxSize) push_back
    if AUTO_FACTORY_RESET_ON_INVALID_CONFIGS:
        every 5s → if !__i_db.isValidConfigs() → clear_all + __factory_reset
    if CONFIG_CLEAR_TO_DEFAULT_ON_FACTORY_RESET:
        listen for EVENT_FACTORY_RESET → clear_all
```

After this, any service can call `__database_service.get_wifi_config_table(&cfg)` and trust the contents.

### 5.7 Read / write semantics

- **Reads are full-struct.** There is no per-field read. Always allocate the full `T` on the stack, read into it, mutate, write back.
- **Writes are immediate.** `set_*_table` returns after the port's `saveConfig` returns — there is no journal, no commit phase. On flash-backed ports this is a flash write per call; **batch your `set` calls** if you are mutating several fields in a row.
- **Endianness and packing.** Structs are written raw via `memcpy`. The framework assumes the same toolchain reads the same bytes back, so a port that targets a different ABI cannot share an NVM image with another port. This is a tradeoff for zero serialisation overhead.
- **Concurrency.** None. The engine is single-threaded; if you use contextual execution, guard `get`/`set` calls with an `iMutex` if more than one lane touches the same table.

### 5.8 Factory reset

Two configurable behaviours:

| Macro | Effect |
|---|---|
| `AUTO_FACTORY_RESET_ON_INVALID_CONFIGS` | Every 5 s, check `__i_db.isValidConfigs()`. If false, `clear_all` + invoke `__factory_reset`. Recovers from corrupted flash without bricking the device |
| `CONFIG_CLEAR_TO_DEFAULT_ON_FACTORY_RESET` | On `EVENT_FACTORY_RESET` (fired by `FactoryResetServiceProvider` after a 6-7s flash-button hold), clear all tables to default. Without this, `factory_reset` only triggers a reboot |

Other services can hook the same event ([EventConfig.h](src/config/EventConfig.h)) to perform their own cleanup (delete IoT cache, clear MQTT retained, etc.).

### 5.9 Adding a new table

To persist a new struct (say `metrics_config` for a new metrics service):

1. **Define the persisted struct** in a new or existing `src/config/<service>Config.h`, following the [§3.4](#3-configuration-system) contract — POD layout, default ctor calls `clear()`, fixed-size members only:
   ```cpp
   struct metrics_configs { ... char host[40]; uint16_t port; ... };
   const int metrics_config_size = sizeof(metrics_configs) + 5;
   using metrics_config_table = metrics_configs;
   ```
2. **Pick a free address** in the per-device schema. Use the gap calculator: `prev_addr + prev_size + 5 < your_addr < next_addr`. For esp* the schema currently runs to 2600 + `sizeof(device_iot_configs) + 5`, so anything beyond that is fair game.
3. **Add a `defItems` entry** to every `devices/<board>/config/DBTableSchema.json` that should carry the table:
   ```json
   {
     "defItemName": "MetricsTable",
     "defItemDesc": "Metrics table handles metrics service configs",
     "defItemArg": "metrics_config_table",
     "defItemAddressKey": "METRICS_CONFIG_TABLE_ADDRESS",
     "defItemAddressValue": "3000",
     "defItemExtVar": "__metrics_table"
   }
   ```
4. **Regenerate**: `python3 scripts/DeviceSetup.py -d <board>` — this rewrites `src/database/tables/MetricsTable.h`.
5. **Add the include + global + accessors** in `DatabaseServiceProvider.{h,cpp}` under the matching `ENABLE_METRICS_SERVICE` guard, mirroring `WiFiTable` / `OtaTable` / etc.
6. **If you exceeded `MAX_DB_TABLES`**, raise it in `devices/DeviceConfig.h` (currently 15 for esp*, 5 for uno).

### 5.10 Gotchas

- **Adding a field to a struct rewrites the on-flash layout.** Existing devices will see `isValidConfigs() == false` on next boot and (if `AUTO_FACTORY_RESET_ON_INVALID_CONFIGS` is on) get reset to defaults. There is no schema migration. Bump `CONFIG_VERSION` to make this intentional.
- **Address collisions are not flagged loudly.** `Database::register_table` returns `false` and the table is silently absent. If `get_wifi_config_table` is returning defaults after a schema edit, check whether your address overlaps with a neighbour.
- **`get_last_table` walks linearly.** Registration is O(n²) in table count — fine for 15, painful if you push past 50.
- **`Database::register_table` only compares against the *last* table.** Inserting a table out of address order (i.e. with an address lower than an already-registered one) bypasses the overlap check. Always keep `defItemAddressValue` strictly increasing in schema order, or define them in ascending order in `DatabaseServiceProvider.cpp`.
- **The port's templates must be visible at every call site.** Define `saveConfig`/`loadConfig`/`clearConfig` inline in `DatabaseInterface.h`, not in `.cpp`, or the link will fail for unused types.
- **The codegen blows away the output directory.** `CreateDBSourceFromJson.py` does `cleanpath(outpath)` before writing — anything else in `src/database/tables/` is deleted. Don't park unrelated files there.

---

## 6. Service Providers

A *service provider* is the framework's unit of feature. Each one lives under [src/service_provider/](src/service_provider/), derives from [`ServiceProvider`](src/service_provider/ServiceProvider.h) ([§1.3](#1-architecture-overview)), and is gated by exactly one `ENABLE_*` flag ([§3.3](#3-configuration-system)). This section is the per-service reference: what it does, what it depends on, what its public API looks like, and how it surfaces in the CLI and web portal.

### 6.1 Common shape

Every provider follows the same skeleton:

```cpp
class XServiceProvider : public ServiceProvider {
public:
    XServiceProvider() : ServiceProvider(SERVICE_X, RODT_ATTR("X")) {}
    bool initService(void* arg = nullptr) override;     // wired from PDIStack::initialize
    bool stopService()                  override;       // optional; base impl reaps every tracked task
    void printConfigToTerminal(iTerminalInterface*) override;   // free-form status hook
    void printStatusToTerminal(iTerminalInterface*) override;   // free-form status hook
};
extern XServiceProvider __x_service;
```

Conventions that hold across all providers:

- The global instance is `__<name>_service` (matches [§1.7](#1-architecture-overview) naming).
- The constructor passes a `service_t` enum value and a `RODT_ATTR` flash-string name; the base class self-registers it into `m_services[]` so the `srvc` CLI can enumerate it generically.
- Long-running work is driven by `__task_scheduler`, but **never called directly**. Use the base wrappers so every task is auto-named (from `m_service_name`), owner-tagged (0 = kernel), and tracked for `srvc stop/start/restart` (§4.9):
  - `serviceSetInterval(fn, dur, now_millis, prio=DEFAULT)` — periodic.
  - `serviceSetTimeout(fn, dur, now_millis, prio=DEFAULT)` — one-shot.
  - `serviceUpdateInterval(id, fn, dur, prio, last_millis, max_attempts)` — reschedule in-place.
  Each mirrors the underlying [`TaskScheduler`](src/utility/TaskScheduler.h) call 1:1 — only the trailing `name`/`owner` args are auto-supplied. All other timing knobs stay explicit.
- The returned id joins a fixed-size list `m_service_task_ids[MAX_SERVICE_TASKS]` (default 6; raise in [ServiceProvider.h](src/service_provider/ServiceProvider.h) if a service needs more). Overflow is silently dropped for that slot. Backward-compat: the base still exposes `m_service_routine_task_id`, kept in sync with the first tracked id.
- `stopService()` in the base now reaps **every** tracked task (used to be primary-only — old leak). Override it if you also need to close sockets or release buffers, and call the base at the end.
- Cross-service triggers flow through `__utl_event` events ([EventConfig.h](src/config/EventConfig.h)), not direct calls — this keeps the dependency graph one-way through the orchestrator.
- Persisted config lives in NVM through `__database_service.get_<name>_config_table` / `set_<name>_config_table` ([§5.4](#5-database-layer)); services never touch `__i_db` directly.

**Lifecycle helpers on the base (all called by `srvc`):**

| Call | Purpose |
|---|---|
| `trackServiceTask(id)` / `untrackServiceTask(id)` / `isServiceTaskTracked(id)` | Explicit list membership. Wrappers call `trackServiceTask` automatically; use `untrackServiceTask` when you cancel a task by other means |
| `signalAllServiceTasks(sig)` | Deliver a signal (§4.9) to every tracked task. Returns hit count |
| `countServiceTasks(running, stopped, zombie)` | Bucketed lifecycle counts used by `srvc list`/`srvc status` |
| `getServiceTaskCount()` / `getServiceTaskId(idx)` | Iterate for status prints |

### 6.2 Service reference

The order below matches the orchestrator's init order in [PdiStack.cpp](src/PdiStack.cpp).

#### 6.2.1 `DatabaseServiceProvider` — `__database_service`

| Flag | Always on (boot dependency) |
|---|---|
| Source | [database/DatabaseServiceProvider.{h,cpp}](src/service_provider/database/DatabaseServiceProvider.h) |
| Depends on | `iDatabaseInterface`, `__factory_reset`, `__utl_event` |
| Init does | `__i_db.beginConfigs(max)` → `__database.init_database(max)` → boots every registered `<Name>Table` → schedules 5 s `isValidConfigs` check → listens for `EVENT_FACTORY_RESET` |
| CLI surface | None directly; every other service's `srvc` config is fetched through it |

Fully documented in [§5. Database Layer](#5-database-layer).

#### 6.2.2 `DeviceFactoryReset` — `__factory_reset`

| Flag | Always on |
|---|---|
| Source | [device/FactoryResetServiceProvider.{h,cpp}](src/service_provider/device/FactoryResetServiceProvider.h) |
| Depends on | `iDeviceControlInterface::isDeviceFactoryRequested`, `__utl_event` |
| Init does | Schedules `check_device_factory_request()` periodically to poll the flash-button (6-7 s hold) |
| CLI surface | Triggered by `reboot` after factory action; web portal has a "Reset Factory" form |
| Event contract | Other services listen for `EVENT_FACTORY_RESET` to drop their own caches before the reboot |

#### 6.2.3 `SerialServiceProvider` — `__serial_service`

| Flag | `ENABLE_SERIAL_SERVICE` |
|---|---|
| Source | [transport/SerialServiceProvider.{h,cpp}](src/service_provider/transport/SerialServiceProvider.h) |
| Depends on | `iSerialInterface` |
| Init does | Opens the serial port at the configured baud, hooks `processSerial` into `iDeviceControlInterface::handleEvents` |
| CLI surface | Provides the **default terminal** for the CLI ([§7](#7-command-line--terminal)); `srvc` prints baud and active port. Also exposes JSON payload apply/append hooks that Device-IoT uses for cross-serial sensor read/write |

#### 6.2.4 `WiFiServiceProvider` — `__wifi_service`

| Flag | `ENABLE_WIFI_SERVICE` |
|---|---|
| Source | [network/WiFiServiceProvider.{h,cpp}](src/service_provider/network/WiFiServiceProvider.h) |
| Depends on | `iWiFiInterface` (passed via `initService(&__i_wifi)`), `__database_service` (wifi table), optionally `iPingInterface` |
| Init does | Configures AP from `wifi_config_table`, kicks off STA scan, registers periodic connectivity check (`WIFI_CONNECTIVITY_CHECK_DURATION = 5000 ms`), schedules NAPT-enable one-shot after STA up |
| Config knobs | `IGNORE_FREE_RELAY_CONNECTIONS`, `ENABLE_DYNAMIC_SUBNETTING`, `ENABLE_NAPT`, `ENABLE_INTERNET_BASED_CONNECTIONS` |
| CLI surface | `net ip`, `net scansta`, `net connsta,<ssid>,<pass>` |
| Web surface | "WiFi" section (STA + AP forms, scan, current status), gated by `ALLOW_WIFI_CONFIG_MODIFICATION` |
| Event contract | Emits `EVENT_WIFI_STA_CONNECTED` / `EVENT_WIFI_STA_GOT_IP` / `EVENT_WIFI_STA_DISCONNECTED` / `EVENT_WIFI_AP_STA(DIS)CONNECTED` / `EVENT_WIFI_INTERNET_UP` / `EVENT_WIFI_INTERNET_DOWN`; consumers (MQTT, OTA, Email, IoT, TLS cert provisioner) react |

#### 6.2.5 `OtaServiceProvider` — `__ota_service`

| Flag | `ENABLE_OTA_SERVICE` |
|---|---|
| Source | [device/OtaServiceProvider.{h,cpp}](src/service_provider/device/OtaServiceProvider.h) |
| Depends on | `iTcpClientInterface`, `iUpgradeInterface` (via `__i_dvc_ctrl`), `iHttpClientHelper`, OTA config table |
| Init does | `setInterval(handleOta, ota_freq_ms)` from `ota_config_table::ota_request_freq` |
| Wire protocol | `GET /api/fordevice/ota-version?mac_id=<mac>&duid=<duid>` → JSON `{ "latest": <ver>, … }`; if newer, `GET /api/fordevice/ota-bin?...&version=<latest>` and apply via `iUpgradeInterface::Upgrade`. HTTP Basic auth (`ota:<mac>` b64), User-Agent `pdistack`; TLS when `ENABLE_TLS_SERVICE` is on |
| Upgrade strategy | Compile-time selector: `MAKE_STREAM_DIRECT_OTA_UPGRADE` (default) streams body bytes into `Update.write()`; `MAKE_STORAGE_DEPENDENT_OTA_UPGRADE` downloads to `<tempdir>/fw.bin` first (needs `ENABLE_STORAGE_SERVICE`); neither ⇒ SDK `httpUpdate` / `ESPhttpUpdate` fallback |
| Config knobs | `ALLOW_OTA_CONFIG_MODIFICATION`; `ota_config_table` (host, port, version, frequency) |
| CLI surface | `srvc` config print |
| Web surface | "OTA" section |

#### 6.2.6 `GpioServiceProvider` — `__gpio_service`

| Flag | `ENABLE_GPIO_SERVICE` (`ENABLE_GPIO_BASIC_ONLY` trims to digital-only for uno) |
|---|---|
| Source | [device/GpioServiceProvider.{h,cpp}](src/service_provider/device/GpioServiceProvider.h) |
| Depends on | `iGpioInterface` (folded into `__i_dvc_ctrl`), `gpio_config_table`, optionally `iTcpClientInterface` (POST sensor data), `__email_service` (event alerts) |
| Init does | Loads `gpio_config_table`, schedules `handleGpioOperations` (mode/value tick) and `handleGpioModes` (table refresh every `GPIO_TABLE_UPDATE_DURATION = 300000 ms`) |
| Modes | `OFF`, `DIGITAL_WRITE`, `DIGITAL_READ`, `DIGITAL_BLINK`, `ANALOG_WRITE`, `ANALOG_READ` ([GpioConfig.h](src/config/GpioConfig.h)) |
| CLI surface | sysfs files at `/sys/class/gpio/<pin>/{value,mode}` — e.g. `echo 1 > /sys/class/gpio/5/value` |
| Web surface | "GPIO" section + "GPIO Events" submenu (per-pin event conditions over `EMAIL` / `HTTP` channels) |

#### 6.2.7 `MqttServiceProvider` — `__mqtt_service`

| Flag | `ENABLE_MQTT_SERVICE` |
|---|---|
| Source | [transport/MqttServiceProvider.{h,cpp}](src/service_provider/transport/MqttServiceProvider.h) |
| Depends on | `iTcpClientInterface`, `mqtt_general_config_table`, `mqtt_lwt_config_table`, `mqtt_pubsub_config_table` |
| Init does | Opens broker connection (with LWT), subscribes from `pubsub` table, schedules publish driver |
| Callback types | `MqttPublishDataCallback`, `MqttSubscribeDataCallback` — applications register these (via `setMqtt*DataCallback`) to inject / receive payloads |
| Config knobs | `ENABLE_MQTT_DEFAULT_PAYLOAD`, `ALLOW_MQTT_CONFIG_MODIFICATION` |
| CLI surface | `srvc` config |
| Web surface | "MQTT" section with General / LWT / PubSub sub-forms |

#### 6.2.8 `EmailServiceProvider` — `__email_service`

| Flag | `ENABLE_EMAIL_SERVICE` |
|---|---|
| Source | [email/EmailServiceProvider.{h,cpp}](src/service_provider/email/EmailServiceProvider.h) |
| Depends on | `iTcpClientInterface`, `email_config_table`, SMTP transport ([src/transports/smtp/](src/transports/smtp/)) |
| Init does | Loads `email_config_table`; schedules `handleEmail` if periodic mail enabled |
| Config knobs | `DEFAULT_MAIL_HOST`, `DEFAULT_MAIL_PORT` (2525), `DEFAULT_MAIL_FROM/TO/SUBJECT`, `DEFAULT_MAIL_FREQUENCY` (300 s) |
| Used by | `GpioServiceProvider` for event-over-email |
| Web surface | "Email" section with credential form + "Test" button |

#### 6.2.9 `DeviceIotServiceProvider` — `__device_iot_service`

| Flag | `ENABLE_DEVICE_IOT` |
|---|---|
| Source | [iot/DeviceIotServiceProvider.{h,cpp}](src/service_provider/iot/DeviceIotServiceProvider.h) |
| Depends on | `iTcpClientInterface`, `iDeviceIotInterface` (**implemented by the application**, see [§13.3.5](#13-portable-interfaces)), `__mqtt_service`, `__database_service` (iot table) |
| Init does | Bootstraps registration via `DEVICE_IOT_OTP_REQ_URL` → `DEVICE_IOT_CONFIG_REQ_URL`, then `configureMQTT()` against the returned channel, then schedules `handleSensorData` at the configured `sample rate` |
| Application hook | The user's sketch calls `initDeviceIotSensor(iDeviceIotInterface*)` to register a sensor implementing `iDeviceIotInterface::{ init, sampleHook, dataHook(payload), resetSampleHook }` |
| Config keys (server-controlled) | `channelhost`, `channelport`, `channelread`, `channelwrite`, `token`, `keepalive`, `samplerate`, `datarate`, `reconfig`, … (all in [DeviceIotConfig.h](src/config/DeviceIotConfig.h)) |
| CLI surface | `iot setid,<duid>`, `iot getid`, `iot sethost,<host>`, `iot gethost` |
| Note | Currently owns its own MQTT config (host / port / topics / credentials) independent of the regular `__mqtt_service` tables — see the notes below |

**Enabling.** The service is **disabled by default**. To enable, uncomment `ENABLE_DEVICE_IOT` in [devices/DeviceConfig.h](devices/DeviceConfig.h) and rebuild.

**Bootstrap flow.** On boot the service performs two HTTP calls against the configured IoT host before opening the MQTT channel:

1. `GET DEVICE_IOT_OTP_REQ_URL` — response JSON must carry an `otp` (`DEVICE_IOT_OTP_KEY`) and a `status` (`DEVICE_IOT_OTP_STATUS_KEY`).
2. `GET DEVICE_IOT_CONFIG_REQ_URL` — response JSON carries the channel config. Both URLs are template strings in [config/DeviceIotConfig.h](src/config/DeviceIotConfig.h) and substitute `[mac]` and `[duid]` at request time.

**Server-response config keys.** The keys the parser looks for in the config response (all defined in [config/DeviceIotConfig.h](src/config/DeviceIotConfig.h)):

| JSON key | Macro | Used for |
|---|---|---|
| `did` | `DEVICE_IOT_CONFIG_DEVICEID_KEY` | Device id assigned by the server |
| `token` | `DEVICE_IOT_CONFIG_CHANNEL_TOKEN_KEY` | MQTT `password` |
| `channelhost` | `DEVICE_IOT_CONFIG_CHANNEL_HOST_KEY` | MQTT broker host |
| `channelport` | `DEVICE_IOT_CONFIG_CHANNEL_PORT_KEY` | MQTT broker port |
| `channelread` | `DEVICE_IOT_CONFIG_CHANNEL_READ_KEY` | Subscribe topic (also used as LWT topic) |
| `channelwrite` | `DEVICE_IOT_CONFIG_CHANNEL_WRITE_KEY` | Publish topic |
| `ifaceread` / `ifacewrite` / `ifaceevent` | `DEVICE_IOT_CONFIG_INTERFACE_*_KEY` | Per-interface routing hints |
| `datarate` | `DEVICE_IOT_CONFIG_DATA_RATE_KEY` | Publish frequency (bounded by `SENSOR_DATA_PUBLISH_FREQ_*_LIMIT`) |
| `samplerate` | `DEVICE_IOT_CONFIG_SAMPLING_RATE_KEY` | Samples per publish (bounded by `SENSOR_DATA_SAMPLES_PER_PUBLISH_MAX_LIMIT`) |
| `keepalive` | `DEVICE_IOT_CONFIG_MQTT_KEEP_ALIVE_KEY` | MQTT keepalive (bounded by `DEVICE_IOT_MQTT_KEEP_ALIVE_MIN..MAX`) |
| `reconfig` | `DEVICE_IOT_CONFIG_RECONFIGURE_KEY` | Server-triggered re-registration |

**MQTT credential shape** (built in `configureMQTT()` — see [DeviceIotServiceProvider.cpp](src/service_provider/iot/DeviceIotServiceProvider.cpp)):

- `client_id` = HTTP-basic-style base64 of `mac:<device-mac>` (via `Http_Client::BuildBasicAuthorization`), **not** the raw MAC.
- `username` = the Device Unique ID (DUID) held in `device_iot_configs.device_iot_duid` — set / read via the `iot setid,<duid>` / `iot getid` CLI commands.
- `password` = the `token` returned by the server in the config response.
- `host` / `port` come from the server's `channelhost` / `channelport` at runtime. The **fallback** defaults (`DEVICE_IOT_DEFAULT_CHANNEL_DATA_HOST` = `192.168.0.100`, `..._PORT` = `1883`) live in [config/DeviceIotConfig.h](src/config/DeviceIotConfig.h) if you need to change the compile-time default.
- LWT topic = `channelread`; LWT payload = `{"duid":"<duid>"}` (see `DEVICE_IOT_MQTT_WILL_TOPIC`).

**IoT host address.** The IoT HTTP host used for the OTP + config bootstrap is stored in `device_iot_configs.device_iot_host` and is set / read via the `iot sethost,<host>` / `iot gethost` CLI commands.

**Independence from the regular MQTT service.** The regular MQTT configs (in [config/MqttConfig.h](src/config/MqttConfig.h) or configured through the web portal) are overwritten by `configureMQTT()` when Device-IoT boots — `__database_service.set_mqtt_*_config_table(...)` is called with the server-supplied values. If you use both, be aware that Device-IoT wins on startup.

The working example sketch is walked through in [§11.6 DeviceIotExample](#116-deviceiotexample--implementing-ideviceiotinterface).

#### 6.2.10 `AuthServiceProvider` — `__auth_service`

| Flag | `ENABLE_AUTH_SERVICE` |
|---|---|
| Source | [auth/AuthServiceProvider.{h,cpp}](src/service_provider/auth/AuthServiceProvider.h) |
| Depends on | `__database_service` (legacy `login_credential_table`); [`__user_store_service`](#6217-userstoreservice--__user_store_service) when `/etc/shadow` exists |
| Used by | Web `AuthMiddleware` ([§8](#8-web-server)); CLI login/su/passwd; every command with `needauth()` |
| Model | **Session-aware delegator** — `isAuthorized(u, p)` is a pure verifier that routes to `__user_store_service.verifyPassword` when `/etc/shadow` exists, else falls back to the LoginTable string check. `setAuthorized(bool)` / `getAuthorized()` / `getUsername()` read+write the **current** `session_t` via `SessionManager::current()`. No global auth bit any more — each session is authorised independently |
| Persistence | LoginTable row is the bootstrap seed only. Actual user directory lives in `/etc/passwd` + `/etc/shadow` (see [§6.2.17](#6217-userstoreservice--__user_store_service)) |

#### 6.2.11 Storage (interface init, no provider)

`ENABLE_STORAGE_SERVICE` doesn't have its own `ServiceProvider` subclass — the FS is used directly by SSH/SFTP, `UserStoreService`, the file-oriented CLI commands (`ls`/`cd`/`mv`/`cp`/`rm`/`mkdir`/`touch`/`cat`/`echo`/`fwrite`/`head`/`tail`/`wc`/`df`/`grep`/`hexdump`/`chmod`/`chown`/`umask`/`mount`), and any application code.

The global `__i_fs` is a **`VfsDispatcher`** ([src/interface/pdi/impl/modules/storage/VfsDispatcher.{h,cpp}](src/interface/pdi/impl/modules/storage/VfsDispatcher.h)) — an `iFileSystemInterface` implementation that routes every call to a mounted backend selected by **longest-prefix path match**. Per-device `FileSystemInterface` (LittleFS via [external/LittleFSWrapper.{h,cpp}](external/LittleFSWrapper.h) → [FileSystemInterfaceImpl](src/interface/pdi/impl/modules/storage/FileSystemInterfaceImpl.h)) is now `__i_rootfs`, mounted at `/` during `PdiStack::initialize`:

```cpp
__i_fs.mount(FILE_SEPARATOR, &__i_rootfs, "rootfs", VFS_TYPE_LITTLEFS);
#ifdef ENABLE_PROCFS
__i_fs.mount("/proc", &__i_procfs, "procfs", VFS_TYPE_PROCFS);
#endif
#ifdef ENABLE_SYSFS
__i_fs.mount("/sys", &__i_sysfs, "sysfs", VFS_TYPE_SYSFS);
#endif
#ifdef ENABLE_DEVFS
__i_fs.mount("/dev", &__i_devfs, "devfs", VFS_TYPE_DEVFS);
#endif
#ifdef ENABLE_TMPFS
__i_fs.mount("/tmp", &__i_tmpfs, "tmpfs", VFS_TYPE_TMPFS);
#endif
__i_fs.init();
```

Mount table config lives in [config/VfsConfig.h](src/config/VfsConfig.h) — `VFS_MAX_MOUNTS` (default 5), `VFS_MOUNT_PREFIX_MAX` (15), `VFS_MOUNT_NAME_MAX` (11), plus per-backend enable flags (`ENABLE_PROCFS`, `ENABLE_SYSFS`, `ENABLE_DEVFS`, `ENABLE_TMPFS`, all default on). The default five slots are exactly filled by rootfs + procfs + sysfs + devfs + tmpfs — enabling further synthetic backends needs `VFS_MAX_MOUNTS` raised (per-port on tight-RAM targets; `ENABLE_TMPFS` in particular is worth undefining on Arduino UNO since it holds file content in RAM). Inspect the mount table at runtime with the `mount` command ([§7.7](#77-built-in-command-inventory)); `df` reports total/used/free per mount.

**procfs** ([src/interface/pdi/impl/modules/storage/ProcFs.{h,cpp}](src/interface/pdi/impl/modules/storage/ProcFs.h)) — a read-only synthetic filesystem mounted at `/proc`; node contents are generated on each read. Works with the regular file commands (`cd /proc`, `ls`, `cat`, `head`, `wc`, `hexdump`, `grep`); all nodes are `0444`, root-owned, and writes return an error.

| Node | Content |
|---|---|
| `/proc/uptime` | seconds since boot: `<sec>.<centisec> <sec>.<centisec>` (Linux layout) |
| `/proc/version` | `PDI Stack version <RELEASE> (<CONFIG_VERSION>)` |

**sysfs** ([src/interface/pdi/impl/modules/storage/SysFs.{h,cpp}](src/interface/pdi/impl/modules/storage/SysFs.h)) — a synthetic filesystem mounted at `/sys` that exposes device peripherals as **read/write** files. GPIO pins (gated on `ENABLE_GPIO_SERVICE`) are laid out one directory per pin, listing only non-exceptional pins in the unified digital+analog table:

| Node | Access | Content |
|---|---|---|
| `/sys/class/gpio/<pin>/value` | `0666` | current reading / write value (decimal) |
| `/sys/class/gpio/<pin>/mode` | `0666` | GPIO mode: `0`=OFF `1`=DIGITAL_WRITE `2`=DIGITAL_READ `3`=DIGITAL_BLINK `4`=ANALOG_WRITE `5`=ANALOG_READ |

Reads return the value the GPIO service maintains (refreshed each operation tick for read-mode pins); writes drive `__gpio_service.m_gpio_config_copy` + persist + re-apply modes. This is the CLI path to GPIO — write mode first, then value: `echo 3 > /sys/class/gpio/4/mode` then `echo 500 > /sys/class/gpio/4/value` blinks GPIO 4 at 500 ms. Directory nodes are `0555`, all nodes root-owned. Write with the `echo` command's `>` redirection ([§7.7](#77-built-in-command-inventory)); the `fwrite` editor's tmp-file flow does not apply to synthetic nodes.

**devfs** ([src/interface/pdi/impl/modules/storage/DevFs.{h,cpp}](src/interface/pdi/impl/modules/storage/DevFs.h)) — a synthetic filesystem mounted at `/dev` exposing byte-stream device nodes. All nodes are `0666`, root-owned; the `/dev` directory is `0555`.

| Node | `cat` (read) | `echo … >` (write) |
|---|---|---|
| `/dev/null` | EOF — yields nothing | discarded |
| `/dev/zero` | `0x00` bytes | discarded |
| `/dev/random` | random bytes (`__i_dvc_ctrl.random_now()`) | discarded |
| `/dev/urandom` | same as `random` (no separate entropy pool on MCU) | discarded |

Unbounded reads (`/dev/zero`, `/dev/random`, `/dev/urandom`) are **capped at `DEVFS_STREAM_READ_MAX` bytes (default 64) per read call** — unlike Linux they are not infinite, so `cat /dev/zero` terminates instead of spinning the MCU into a watchdog reset. `random_now()` is an `iUtilityInterface` method: hardware RNG on esp32 (`esp_random`) / esp8266 (`os_random`), a portable micros-seeded xorshift default on UNO / mock (non-cryptographic).

**tmpfs** ([src/interface/pdi/impl/modules/storage/TmpFs.{h,cpp}](src/interface/pdi/impl/modules/storage/TmpFs.h)) — a **RAM-backed read/write** filesystem mounted at `/tmp`. Unlike the synthetic proc/sys/dev backends it actually stores file content and directories in the heap (a `tmpfs_node_t` table), so the full command surface works — `mkdir`, `touch`, `echo … >`, `cat`, `head`, `tail`, `wc`, `grep`, `cp`, `mv`, `rm`, `chmod`, `chown`. Files are stamped with the creating session's uid/gid and `perms & ~umask` (same `currentOwner`/`currentUmask`/`nowEpoch` hooks the LittleFS root uses), so the dispatcher's permission gate enforces access normally. Everything is lost on reboot.

Budgets live in [config/TmpFsConfig.h](src/config/TmpFsConfig.h): `TMPFS_MAX_BYTES` (total content, default 4096), `TMPFS_MAX_NODES` (files + dirs, default 24), `TMPFS_MAX_PATH` (default 63). `df` reports usage against `TMPFS_MAX_BYTES`. Example: `mkdir /tmp/work; echo hello > /tmp/work/a.txt; cat /tmp/work/a.txt`. A single `/tmp` mount is the whole RAM-filesystem surface — sufficient scratch space for the target devices without a second RAM store.

**Permissions & ownership** are advisory at the wrapper (bits stored via `lfs_setattr`) and enforced at the dispatcher. `file_info_t` carries `m_type`/`m_size`/`m_name`/`m_ctime`/`m_mtime`/`m_perms`/`m_uid`/`m_gid` and every FS entry is stamped on create with the current session's uid/gid (via the protected `currentOwner()` hook, overridden in `FileSystemInterfaceImpl` to pull from `SessionManager`). Default file/dir perms are `0644` / `0755` masked by the caller's umask (`FILE_UMASK_DEFAULT 0022`, per-session via `currentUmask()`).

`VfsDispatcher` gates every write/read/mutate at entry via three helpers:

| Op class | Gate | Behavior |
|---|---|---|
| `editFile`, `writeFile`, `deleteFile`, `deleteDirectory`, `touch` | `checkAccess(path, W)` | POSIX owner/group/other bits; root (uid=0) bypasses |
| `readFile`, `getDirFileList`, content-search family (`findInFile`, `readLineInFile`, …) | `checkAccess(path, R)` | Same lookup |
| `setFileAttr`, `removeFileAttr`, `setFilePermissions` | `checkOwnerOrRoot(path)` | Owner of the file or root |
| `setFileOwner` | `checkRoot()` | Root only |
| `createFile`, `createDirectory`, `rename`, `copyFile`, `moveFile`, `isFileExist`, `isDirExist`, `getFileMeta`, `getFileAttr`, `getFileSize` | ungated | Deferred (parent-dir traversal) / metadata-only |

Missing files are treated as "allowed" (creation defer); missing uid/gid attrs on legacy entries default to **root-owned** (0/0).

**Privileged scope** — a setuid analog. `beginPrivileged()` / `endPrivileged()` on the dispatcher increment a depth counter; while depth > 0 all three check helpers return `true`. Used by `UserStoreService::verifyPassword` / `setPassword` to read `/etc/shadow` (0600) on behalf of a non-root session during `su`/`login`/`passwd`. Scope kept as narrow as possible around the shadow read.

Cross-mount `copyFile`/`rename`/`moveFile` (source and destination on different backends, e.g. `cp /tmp/a.txt /home/a.txt`) stream the source file chunk-by-chunk into the destination backend — the first chunk creates/truncates, the rest append; a mid-transfer failure removes the partial destination. Directories don't cross boundaries, and an existing destination is refused. A cross-mount `mv`/`rename` is a stream-copy followed by deleting the source. Same-backend operations still route straight to that backend's native call. Symlinks (`FILE_TYPE_LINK` / `ln -s`) and runtime `mount`/`umount` remain unimplemented.

#### 6.2.12 `WebServer` — `__web_server`

| Flag | `ENABLE_HTTP_SERVER` (+ optional `ENABLE_HTTPS_SERVER` for TLS) |
|---|---|
| Source | [webserver/WebServer.{h,cpp}](src/webserver/WebServer.h) |
| Wired in PdiStack | `__web_server.initService(this->m_server)` where `m_server = &__i_http_server` |
| HTTP/HTTPS dispatch | If `ENABLE_HTTPS_SERVER` + `ENABLE_TLS_SERVICE` are on, `initService` calls `setServerCertificatePath` / `setServerPrivateKeyPath` (plus `setClientCertificateAuthorityPath` when `ENABLE_HTTPS_SERVER_MTLS`) using the defaults from [TlsConfig.h](src/config/TlsConfig.h), then `begin(HTTPS_DEFAULT_PORT=443, secure=true)`. Otherwise `begin(HTTP_DEFAULT_PORT=80)`. |
| Loop tick | `__web_server.handle_clients()` is called every `PdiStack::serve()` iteration |

Full breakdown lives in [§8. Web Server](#8-web-server) — it has its own router, middleware chain, controllers, views, and session handler that don't fit the `ServiceProvider` shape.

#### 6.2.13 `TelnetServiceProvider` — `__telnet_service`

| Flag | `ENABLE_TELNET_SERVICE` |
|---|---|
| Source | [transport/TelnetServiceProvider.{h,cpp}](src/service_provider/transport/TelnetServiceProvider.h) |
| Depends on | `iTcpServerInterface`, `__cmd_service`, `__auth_service` |
| Init does | Binds port 23 (configurable via `initService(&port)`); accepts one client, hands its `iClientInterface*` to the CLI |
| Loop tick | Driven by `PdiStack::serve` indirectly through the scheduler |

#### 6.2.14 `SSHServer` — `__sshserver_service`

| Flag | `ENABLE_SSH_SERVICE` (⇒ `ENABLE_NETWORK_SERVICE` ∧ `ENABLE_STORAGE_SERVICE`) |
|---|---|
| Source | [shell/ssh/SSHServiceprovider.{h,cpp}](src/service_provider/shell/ssh/SSHServiceprovider.h) (in `LWSSH` namespace) |
| Depends on | `iTcpServerInterface`, `iFileSystemInterface`, crypto primitives ([src/utility/crypto/](src/utility/crypto/)), `__auth_service` |
| Key algorithms | See `SSHKeyAlgorithm` enum in [SshConfig.h](src/config/SshConfig.h) — Ed25519 / Curve25519-based |
| File transfer | SFTP subsystem implemented on top of the file system; supports interactive `sftp` (REALPATH, STAT, OPENDIR/READDIR, OPEN/READ/WRITE, MKDIR/RMDIR, REMOVE, RENAME, FSTAT; SETSTAT is no-op; READLINK/SYMLINK report unsupported) and SCP via `scp -s` (see [§7.9 SFTP / SCP file transfer](#79-sftp--scp-file-transfer)) |
| Cost | **Highest of any service** — keys, hash, symmetric AES, large per-session buffers |

#### 6.2.15 `CommandLineServiceProvider` — `__cmd_service`

| Flag | `ENABLE_CMD_SERVICE` |
|---|---|
| Source | [cmd/CommandLineServiceProvider.{h,cpp}](src/service_provider/cmd/CommandLineServiceProvider.h) |
| Inherits | `ServiceProvider`, `CommandExecutionInterface` |
| Depends on | `__auth_service`, `SessionManager` ([§6.2.18](#6218-sessionmanager)), every command in [cmd/commands/](src/service_provider/cmd/commands/) |
| Init does | Registers all command handlers; `PdiStack` calls `SessionManager::attach(serialTerminal)` at boot so the serial slot is populated before first input |
| Terminal binding | `useTerminal(t)` attaches a `session_t` for terminal `t` (idempotent) and draws the login prompt. `processTerminalInput(t)` looks up the session via `SessionManager::findByTerminal(t)`, sets it as current for the tick, then dispatches. Each in-flight command carries `m_owner = session` so cross-session `getCommandWaitingForUserInput` never returns another session's prompt |
| Built-in commands | Files (`ls`/`cd`/`pwd`/`mkdir`/`touch`/`mv`/`cp`/`rm`/`cat`/`echo`/`fwrite`/`head`/`tail`/`wc`/`df`/`grep`/`hexdump`/`mount`/`chmod`/`chown`/`umask`), auth+users (`login`/`logout`/`whoami`/`id`/`who`/`su`/`passwd`/`useradd`/`userdel`/`groups`), device (`net`/`srvc`/`ps`/`top`/`kill`/`pkill`/`killall`/`renice`/`ssh`/`tls`/`iot`/`reboot`/`watch`/`uptime`/`cls`/`help`) — full reference in [§7.7](#77-built-in-command-inventory) |
| Multi-session | Up to `PDI_MAX_SESSIONS` (default 3) sessions run concurrently across serial + telnet + ssh. Each has its own linebuf, cursor, history-walk, cwd, auth, username, and in-flight commands. See [§7.8](#78-multi-terminal-session-lifecycle) |

#### 6.2.16 TLS (no provider; transport hookup + cert provisioning)

`ENABLE_TLS_SERVICE` doesn't ship its own `ServiceProvider` either — it lives at the **interface + port** level and is consumed by anything that asks `iInstanceInterface` for a fresh client/server. The orchestrator's only direct involvement: when the flag is on, `PdiStack`'s shared HTTP client (`m_client`) is allocated via `__i_instance.getNewTlsClientInstance()` instead of TCP, so OTA / MQTT / Email / IoT / GPIO-post automatically run over TLS.

| Flag | `ENABLE_TLS_SERVICE` (+ `ENABLE_HTTPS_SERVER`, `ENABLE_HTTPS_SERVER_MTLS`, `ENABLE_TLS_CERT_GENERATION`, `ENABLE_SERVER_TLS_CERT_GENERATION_AT_RUNTIME`) |
|---|---|
| Backends | **esp8266** → BearSSL: [TlsClientInterface](devices/esp8266/TlsClientInterface.h), [TlsServerInterface](devices/esp8266/TlsServerInterface.h), [BearSSLCertLoader](devices/esp8266/BearSSLCertLoader.h). **esp32** → mbedTLS: [TlsClientInterface](devices/esp32/TlsClientInterface.h), [TlsServerInterface](devices/esp32/TlsServerInterface.h), [MbedTLSCertLoader](devices/esp32/MbedTLSCertLoader.h) |
| Cert / key paths | Loaded from FS at runtime via the per-port `*CertLoader` namespace. Defaults from [TlsConfig.h](src/config/TlsConfig.h): `/etc/http/server.crt`, `/etc/http/server.key`, `/etc/http/client-ca.crt`, `/etc/ssl/ca-bundle.crt` |
| Threading | TLS handshakes (EC keygen / ECDSA sign) overflow the default cont_t stack on esp8266, so `ENABLE_TLS_SERVICE` forces `ENABLE_CONTEXTUAL_EXECUTION` and runs BearSSL on a dedicated cooperative task sized by `TLS_TASK_STACK_SIZE` (default 6500 B) |
| Outbound default | `PdiStack::PDIStack()` calls `setVerifyPeer(false)` on the bundled client so encrypted-but-unverified TLS works out of the box. For production, use `setCertificateAuthorityPath(TLS_DEFAULT_OUTBOUND_CA_BUNDLE_PATH)` and drop the `setVerifyPeer(false)` line |
| HTTPS server | [HttpServerInterfaceImpl](src/interface/pdi/impl/middlewares/HttpServerInterfaceImpl.h) has a TLS branch (`begin(port, secure=true)`) that wraps each accepted client in the port's TLS server. `Strict-Transport-Security` is emitted when `HTTPS_HSTS_MAX_AGE_SECONDS` is non-zero |
| Cert provisioning (esp32) | [devices/esp32/TlsCertProvisioner.{h,cpp}](devices/esp32/TlsCertProvisioner.h). `generateCert(...)` issues a self-signed EC/RSA cert with optional CA bit, IPv4/DNS SANs, custom validity. `ensureServerCert(...)` creates one only if missing — wired to `EVENT_WIFI_STA_GOT_IP` when `ENABLE_SERVER_TLS_CERT_GENERATION_AT_RUNTIME` is on |
| Off-device cert generation | [scripts/GenTlsCerts.py](scripts/GenTlsCerts.py) — OpenSSL-backed alternative for boards without on-device gen (esp8266). EC/RSA, multi-DNS/IP SANs, CA mode, CSR re-signing. Output under `certs/`; upload to the `TLS_DEFAULT_*_PATH` paths |
| CLI | `tls q=1,t=<EC|RSA>,l=<bits>,n=<CN/DNS>,i=<IPv4>` — generates a server cert on-device (esp32 only). See [§7.7](#77-built-in-command-inventory). |

#### 6.2.17 `UserStoreService` — `__user_store_service`

| Flag | `ENABLE_AUTH_SERVICE` + `ENABLE_STORAGE_SERVICE` |
|---|---|
| Source | [user/UserStoreService.{h,cpp}](src/service_provider/user/UserStoreService.h) |
| Config | [config/UserStoreConfig.h](src/config/UserStoreConfig.h) — `USER_STORE_PASSWD_PATH="/etc/passwd"`, `USER_STORE_SHADOW_PATH="/etc/shadow"`, `USER_STORE_SALT_LEN=8`, `USER_STORE_ROOT_UID=0`, `USER_STORE_DEFAULT_SHELL="cmd"` |
| Depends on | `__i_fs`, `__database_service` (bootstrap only), `utility/crypto/hash/sha256`, `utility/DataTypeConversions` (hex helpers) |
| Files | **`/etc/passwd`** — one line per user: `username:x:uid:gid:home:shell` (mode `0644`, world-readable — `id`/`groups` need it). **`/etc/shadow`** — `username:hexhash:hexsalt` (32-byte SHA-256 of `salt‖password`, 8-byte random salt) stamped `0600` (root-only, since it holds the hashes) |
| Public API | `findUserByName`, `findUserByUid`, `addUser(record, password)` (writes both files, rolls back passwd row on shadow failure), `removeUser` (removes from both), `verifyPassword` (constant-time hash compare), `setPassword` |
| Bootstrap | On `initService`, if `/etc/passwd` doesn't exist, seeds it with the LoginTable admin as `uid=0, gid=0, home=/, shell=cmd` and hashes the admin password into `/etc/shadow`. Idempotent — reboot doesn't clobber. Also **retroactively re-stamps** `/etc/shadow` to `0600` on every boot in case a pre-perms build left it `0644` |
| Privileged reads | `verifyPassword` and `setPassword` need to read/write `/etc/shadow` on behalf of a non-root session (during `su`/`login`/`passwd`). They bracket the shadow access in `__i_fs.beginPrivileged()` / `endPrivileged()` — a setuid analog. Scope is kept as narrow as possible; every other UserStore method runs unprivileged |
| useradd default gid | `useradd` assigns `gid = uid` (each user in their own group). No `/etc/group` yet — group semantics land in a future step |
| Init order | Called from `PdiStack::initialize` after `__i_fs.init()` and before the CLI, so FS is ready but auth still degrades cleanly to LoginTable if shadow write fails |
| Auth path | `AuthServiceProvider::isAuthorized(u, p)` (§6.2.10) prefers this service when `/etc/shadow` exists, else falls back to LoginTable. That's the switch that turns pdi-framework from single-record auth into a multi-user OS |

#### 6.2.18 `SessionManager`

Not a `ServiceProvider` — a static registry that owns the per-session state.

| Source | [session/SessionManager.{h,cpp}](src/service_provider/session/SessionManager.h) |
|---|---|
| Config | [config/SessionConfig.h](src/config/SessionConfig.h) — `PDI_MAX_SESSIONS` (default 3). Override to 1 on AVR-class devices |
| State type | `session_t` in [utility/DataTypeDef.h](src/utility/DataTypeDef.h) — `m_sid`, `m_state`, `m_terminal*`, `m_loginAt`, `m_lastActivityAt`, `m_linebuf`, `m_cursor`, `m_historyIdx`, `m_origTypedPrefix`, `m_prevArgSize`, `m_autoCompleteIdx`, `m_prevCmdSize`, `m_cwd`, `m_lastCwd`, `m_umask` (storage-gated), `m_isAuthorized`, `m_username`, `m_uid`, `m_gid` (auth-gated) |
| API | `attach(terminal)` / `detach(terminal)` / `findByTerminal` / `current` / `setCurrent` / `getByIndex(i)` / `maxSessions()` / `activeCount()`. Storage-gated: `getPWD`, `getLastPWD`, `setPWD`, `changeDirectory`, `getCurrentUmask`, `setCurrentUmask`. Auth-gated: `getCurrentUid`, `getCurrentGid` — session-scoped cwd/uid/gid/umask with `__i_fs` fallback pre-attach |
| Login-time caching | `AuthServiceProvider::setAuthorized(true)` looks up the resolved `user_record` via `__user_store_service.findUserByName` and caches `m_uid` / `m_gid` on the session; `m_umask` is reset to `FILE_UMASK_DEFAULT` (`0022`). On logout / clear, all three reset to 0/0/`FILE_UMASK_DEFAULT`. The cache is what `VfsDispatcher` reads on every FS check (single dereference — no `/etc/passwd` scan per file op) |
| Wire-in | `CommandLineServiceProvider::useTerminal` calls `attach`; `processTerminalInput` calls `findByTerminal` + `setCurrent`; SSH's USERAUTH_SUCCESS attaches early so auth state anchors to the SSH channel; Telnet/SSH `closeSession` calls `detach(theClient)` explicitly |

### 6.3 Init order and why it matters

`PDIStack::initialize` orders the calls deliberately:

```
1. database_service        ← every other service may need persisted config
2. serial_service          ← provides the boot terminal for log output
3. wifi_service            ← brings up the network
4. ota_service             ← may pre-empt all other startup if a new build is queued
5. gpio_service            ← can fire alerts via http/email once those exist
6. mqtt_service            ← may publish boot status
7. email_service           ← used by gpio events
8. factory_reset           ← always-on; final hook so all services have registered listeners
9. device_iot_service      ← needs MQTT already up
10. auth_service           ← gates everything CLI/Web sees
11. storage init           ← brings the FS up (needed by SSH)
12. web_server             ← needs auth + storage
13. telnet, ssh            ← need network + storage + cmd
14. cmd_service            ← started last so all `srvc` listings are complete
```

Two implications you cannot violate:
- A service initialised *later* can call into one initialised *earlier*; the reverse is undefined.
- A service that needs another service in its constructor will hit a static-init order trap. Defer all cross-service lookups to `initService` or runtime.

### 6.4 Cross-service event bus

Direct calls between services are reserved for known-earlier dependencies (OTA → device control). Anything fan-out happens through `__utl_event`. Common event names from [EventConfig.h](src/config/EventConfig.h):

| Event | Fired by | Typical listeners |
|---|---|---|
| `EVENT_FACTORY_RESET` | `__factory_reset.factory_reset()` | Database (clear tables), IoT (drop cache), GPIO (reset pins) |
| `EVENT_WIFI_STA_CONNECTED` / `_STA_DISCONNECTED` | `__wifi_service` | MQTT (re/dis-connect), OTA (suspend), IoT (suspend) |
| `EVENT_WIFI_STA_GOT_IP` | `__wifi_service` (after DHCP / static IP latches) | Cert provisioner (auto-mint with IP in SAN), services that need a stable address |
| `EVENT_WIFI_AP_STACONNECTED` / `_STADISCONNECTED` | `__wifi_service` | Captive-portal flows, per-client tracking |
| `EVENT_WIFI_INTERNET_UP` / `_DOWN` | `__wifi_service` connectivity poller | OTA, IoT, Email |
| `EVENT_GPIO_TRIGGER` | `__gpio_service` event detector | Email, MQTT, HTTP-post |
| `EVENT_SERIAL_AVAILABLE` | Serial input bridge | Sketch hooks |
| `EVENT_OTA_*` | `__ota_service` | Logger, web portal status |

Use `__utl_event.add_event_listener(name, [&](void* e){ … })` to subscribe; `__utl_event.fire(name, ptr)` to publish.

### 6.5 Writing a new service

To add (say) a `MetricsServiceProvider`:

1. **Pick a feature flag** in [devices/DeviceConfig.h](devices/DeviceConfig.h) (`ENABLE_METRICS_SERVICE`) and a `service_t` enum value in [ServiceProvider.h](src/service_provider/ServiceProvider.h), guarded by the flag.
2. **Add the persisted struct** ([§3.4](#3-configuration-system)) and the DB table ([§5.9](#5-database-layer)).
3. **Create [src/service_provider/metrics/MetricsServiceProvider.{h,cpp}](src/service_provider/metrics/MetricsServiceProvider.h)** deriving from `ServiceProvider`. Register scheduler work through the base wrappers so the task is auto-named, owner-tagged, and tracked (§6.1) — this makes it visible in `ps`, killable via `pkill Metrics`, and controllable via `srvc stop Metrics`:
   ```cpp
   MetricsServiceProvider() : ServiceProvider(SERVICE_METRICS, RODT_ATTR("Metrics")) {}
   bool initService(void* arg) override {
       __database_service.get_metrics_config_table(&m_cfg);
       this->serviceSetInterval(
           [&]{ this->tick(); }, m_cfg.interval_ms, __i_dvc_ctrl.millis_now());
       return ServiceProvider::initService(arg);
   }
   ```
4. **Wire it in** the orchestrator: include guarded in [PdiStack.h](src/PdiStack.h), call `__metrics_service.initService(...)` from [PdiStack.cpp](src/PdiStack.cpp) in the right slot per [§6.3](#6-service-providers).
5. **Surface it**: implement `printConfigToTerminal` / `printStatusToTerminal` so `srvc` lists it; optionally add a CLI command under [cmd/commands/](src/service_provider/cmd/commands/) and a web controller under [webserver/controllers/](src/webserver/controllers/).
6. **Use events, not direct calls**, when you need to react to other services' state.

### 6.6 Gotchas

- **Constructor side-effects are a trap.** Allocating, reading the DB, or touching `__i_*` interfaces in your service's constructor will run before `setup()`, before the device's interface globals are initialised. Always defer to `initService`.
- **Don't `new` in `initService` either.** Use a value member or a static buffer; the embedded heap can fragment over months of uptime.
- **The base `stopService` reaps every task in `m_service_task_ids[]`** — as long as you registered through the `serviceSet*` wrappers (§6.1), no override is needed. If you registered a task via raw `__task_scheduler.setInterval(...)` (usually a mistake now), it'll leak on stop unless you also call `trackServiceTask(id)` or explicitly `clearInterval` it in your override.
- **`SERVICE_DATABASE` and `SERVICE_FACTORY` are unconditional** in the `service_t` enum — every build always has them. Don't guard them with `ENABLE_*` even if your minimal build "doesn't need" them.
- **`printConfigToTerminal` is called from the CLI thread** (whichever lane the terminal is attached to). It must not block. If your config print is slow (e.g. iterates 50 MQTT topics), paginate via repeated `srvc` calls instead.
- **One global per service.** The `m_services[SERVICE_X]` slot is filled by the constructor; instantiating a second `XServiceProvider` overwrites the pointer and silently breaks `getService(SERVICE_X)`.

---

## 7. Command Line / Terminal

The CLI is the framework's universal control plane. The same set of commands is reachable over **serial** by default, **telnet** on port 23, and **SSH** on port 22 — with login, command history, autocomplete, in-place editing, and a SFTP file-transfer subsystem. The trick that makes one implementation cover three channels: every terminal source surfaces as an `iTerminalInterface*`, and the CLI rebinds that pointer per session.

Implementation: [src/service_provider/cmd/](src/service_provider/cmd/) (`__cmd_service`) plus the parser primitives in [src/utility/CommandBase.{h,cpp}](src/utility/CommandBase.h). User-facing command table is in [§7.7 Built-in command inventory](#77-built-in-command-inventory).

### 7.1 Layered model

```
  Application stream    Serial port   Telnet TCP session   SSH channel
                            │                │                  │
                            └────────────────┴──────────────────┘
                                             │  iTerminalInterface*
                                             ▼
                            ┌──────────────────────────────────────┐
                            │  CommandLineServiceProvider          │  ← src/service_provider/cmd/
                            │  __cmd_service                       │
                            │  ─ owns the input ring buffer        │
                            │  ─ binds one terminal at a time      │
                            │  ─ history + autocomplete + editing  │
                            └──────────┬───────────────────────────┘
                                       │ dispatch
                                       ▼
                            ┌──────────────────────────────────────┐
                            │  CommandBase registry                │  ← src/utility/CommandBase.h
                            │  static vector<CommandProp>          │     +  src/service_provider/cmd/commands/
                            │  ─ each command self-registers       │
                            │  ─ parses `cmd opt=val[,opt=val]…`   │
                            │  ─ supports free-arg, holding opts,  │
                            │    waiting-for-input, abort signals  │
                            └──────────┬───────────────────────────┘
                                       │ delegate to service
                                       ▼
                            ┌──────────────────────────────────────┐
                            │  Service Providers (§6)              │
                            │  __wifi_service, __gpio_service,     │
                            │  __task_scheduler, __i_fs, …         │
                            └──────────────────────────────────────┘
```

### 7.2 The terminal contract

Any source that wants to feed the CLI must implement `iTerminalInterface` (defined in [src/utility/iIOInterface.h](src/utility/iIOInterface.h)). It extends `iIOInterface` with the byte-level I/O methods (`write`/`writeln` overloaded for every primitive, `readStringUntil`, `readLine`, `with_timestamp`) plus terminal-specific affordances. The framework already provides three implementations:

| Source | Class | Provided by |
|---|---|---|
| Serial port | `SerialInterface` (also satisfies `iSerialInterface : iClientInterface`) | Device port |
| Telnet client session | `iClientInterface*` returned by `iTcpServerInterface::accept()` | Device port (TCP) |
| SSH channel | LWSSH session wrapper | [src/service_provider/shell/ssh/](src/service_provider/shell/ssh/) |

When a telnet or SSH client connects, the corresponding service calls `__cmd_service.useTerminal(client)`, which attaches a fresh `session_t` slot via `SessionManager` and starts a login prompt on that client. Serial keeps its own slot (pinned at boot). Each session's input/output stays isolated — see [§7.8](#78-multi-terminal-session-lifecycle).

### 7.3 Input sequences

The CLI does line editing in-process, so it has to recognise control sequences character-by-character. The enum is in [DataTypeDef.h](src/utility/DataTypeDef.h):

| Sequence | Action |
|---|---|
| `CMD_TERM_INSEQ_ENTER` | Submit current line for parsing/execution |
| `CMD_TERM_INSEQ_BACKSPACE_CHAR` / `_DELETE_CHAR` / `_DELETE` | Edit |
| `CMD_TERM_INSEQ_LEFT_ARROW` / `_RIGHT_ARROW` | Move cursor in-line |
| `CMD_TERM_INSEQ_UP_ARROW` / `_DOWN_ARROW` | Walk history (requires `ENABLE_STORAGE_SERVICE`) |
| `CMD_TERM_INSEQ_HOME` / `_END` | Line start / end |
| `CMD_TERM_INSEQ_PAGE_UP` / `_PAGE_DOWN` | Scroll long output |
| `CMD_TERM_INSEQ_TAB` | Autocomplete (cycles registered commands matching the typed prefix) |
| `CMD_TERM_INSEQ_ESC` | Cancel current line; for `fwrite`, finalize the file |
| `CMD_TERM_INSEQ_CTRL_C` / `_CTRL_Z` | Abort the running command; the base `executeTermInputAction` returns `CMD_RESULT_ABORTED` |

Long-running commands (`watch`, `fwrite`) opt-in to receive these sequences mid-execution by overriding `executeTermInputAction(cmd_term_inseq_t)`.

### 7.4 The `CommandBase` (`cmd_t`) contract

Every command is a `struct CommandBase` (typedef'd `cmd_t`). The struct owns parsing — you only fill in `execute(cmd_term_inseq_t)`. See [src/utility/CommandBase.h](src/utility/CommandBase.h).

#### Construction

A command class:
1. Calls `SetCommand("ls")` to declare its name (max `CMD_SIZE_MAX = 8` chars).
2. Calls `AddOption("p")`, `AddOption("m")`, … for each named option (max `CMD_OPTION_MAX = 3`, each name ≤ `CMD_OPTION_SIZE_MAX = 3` chars).
3. Optionally `setAcceptArgsOptions(true)` to also accept positional/free args.
4. Optionally `setCmdOptionSeparator(",")` (default), `";"`, or `" "`.
5. Implements `execute(cmd_term_inseq_t)` returning a `cmd_result_t`.
6. **Overrides `getUsage() const`** to return an `RODT_ATTR`-wrapped one-line usage string. Format: `"<verb> [args]  brief description"`. `help` prints it under the command name, and `CommandBase::ResultToTerminal` prints it under `CmdErr` whenever the command returns `CMD_RESULT_ARGS_MISSING` / `CMD_RESULT_ARGS_ERROR` / `CMD_RESULT_INVALID_OPTION` — so bad-argument branches never need to inline the string themselves.
7. Optionally overrides `needauth()` to require login, `executeTermInputAction()` to handle CTRL+C/ESC/etc., `stopRunningInBackground()` for cleanup.
8. Registers itself via `CommandBase::RegisterCommand("ls", &ListFSCommand::Registrar)` so `__cmd_service` can dispatch by name.

#### Parsing rules

Given input `<cmd> p=4,m=3,v=500` (a command declaring options `p`, `m`, `v`):
- Tokenise the **command name** at the first space (or end).
- Look up the option separator (default `,`).
- For each `key=value` pair, find the key in the declared options and store `optionval` + `optionvalsize`.
- If `setAcceptArgsOptions(true)` and **no** `=` is present after the command, treat the rest as free positional args mapped into `m_options[0..]` in order.
- On success: `m_result = CMD_RESULT_OK`, then `execute()` runs.

Inside `execute`, options are read by name:

```cpp
auto pin = RetrieveOption(CMD_OPTION_NAME_P);
if (pin == nullptr) return CMD_RESULT_ARGS_MISSING;
int p = atoi(pin->optionval);
```

#### Multi-iteration commands

For commands that don't finish in a single tick (e.g. `watch`, `fwrite`), `execute` returns `CMD_RESULT_INCOMPLETE`. The service keeps the command active, increments `m_iterations`, and re-enters `execute` on every subsequent input. `Clear()` runs only when `m_result != CMD_RESULT_INCOMPLETE`.

#### Holding option values

By default, parsed `optionval` points into the live receive buffer — so once the next character arrives, the pointer is invalid. If a command needs the value to survive across iterations, it calls `holdOptionValue("c")` which copies the bytes into a `new char[]` owned by the option, freed on `Clear()`. `WatchCommand` uses this for its `c=<command>` payload that has to live for many ticks.

### 7.5 Result codes

| `cmd_result_t` | Meaning | What the dispatcher does |
|---|---|---|
| `CMD_RESULT_OK` | Success | Print blank line; clear options |
| `CMD_RESULT_INCOMPLETE` | Multi-iter; keep me active | Do not clear; wait for next input |
| `CMD_RESULT_ARGS_ERROR` / `_ARGS_MISSING` / `_INVALID_OPTION` | Bad usage | Print `CmdErr : <n>` **and** `usage: <getUsage()>` if the command provides one — no need for bad-argument branches to inline usage strings themselves |
| `CMD_RESULT_NOT_FOUND` / `_INVALID` | No such command / can't parse | Print error |
| `CMD_RESULT_NEED_AUTH` / `_WRONG_CREDENTIAL` | Login required / failed | Re-prompt via the login flow |
| `CMD_RESULT_ABORTED` | CTRL+C / CTRL+Z hit | Print error; stop iteration |
| `CMD_RESULT_FAILED` | Command logic failure | Print error |
| `CMD_RESULT_TERMINAL_ERR` / `_TERMINAL_ABORTED` / `_TERMINAL_HOLD_BUFFER` | Terminal-side states | Special-cased by the service |

### 7.6 `CommandLineServiceProvider`

[CommandLineServiceProvider.h](src/service_provider/cmd/CommandLineServiceProvider.h) — the runtime around `CommandBase`. It owns the in-flight command list (`m_cmdlist`, session-tagged via `m_owner`) and the persistent history file path; the receive buffer, cursor, history-walk, autocomplete indexes, and cwd all live **on each `session_t`** (see [§6.2.18](#6218-sessionmanager)). Dispatch: `processTerminalInput(t)` resolves the session via `SessionManager::findByTerminal(t)`, sets it current for the tick, then drives `executeCommand`. `useTerminal(t)` attaches a session for a fresh client and draws its login prompt.

#### History & autocomplete

- **History** is persisted only when `ENABLE_STORAGE_SERVICE` is on. The file `CMD_TERMINAL_HISTORY_STATIC_FILEPATH = "/.term_history"` is capped at `CMD_TERMINAL_HISTORY_MAX_LINES = 25`. Older entries are rotated out.
- **Autocomplete** walks `CommandBase::m_cmd_registry` looking for names that start with the current input prefix. Subsequent TABs cycle through matches (`m_cmdAutoCompleteIndex`).
- Off devices without storage, ↑/↓ produce no output (no backing store) — autocomplete still works because the registry lives in RAM.

### 7.7 Built-in command inventory

Names come from [CommandCommon.h](src/service_provider/cmd/commands/CommandCommon.h); each `<Name>Command.h` defines the class.

#### User-facing command table

| Command | Options | Brief |
|---|---|---|
| ls [\<dir>] | | List files/dirs with mode / owner / group / mtime / size. No arg → current dir; absolute path → listed as-is; relative path → joined with PWD. Owner+group show human names (numeric uid/gid if user record is missing). e.g. **ls**, **ls /proc**, **ls scripts** |
| mkdir \<dir> | | Create directory. Perms = `0755 & ~umask`; owner = current session's uid/gid. e.g. **mkdir /home/scripts** |
| touch \<file> | | Create the file empty if missing (perms = `0644 & ~umask`, owner = current session's uid/gid); bump `mtime` if it exists. e.g. **touch /home/notes.txt** |
| mv \<src> \<dst> | | Move / rename file or dir. e.g. **mv /home/a.txt /home/b.txt** |
| cp \<src> \<dst> | | Copy file. e.g. **cp /home/a.txt /home/b.txt** |
| pwd | | Print current working directory. e.g. **pwd** |
| rm \<file_or_dir> | | Remove file or directory. Requires `+w` on the target for non-root. e.g. **rm /home/notes.txt** |
| cat \<file> | | Print file contents to terminal (renamed from `fread`). Requires `+r` on the file for non-root. e.g. **cat /home/notes.txt**, **cat /proc/uptime** |
| echo \<text> [> \<file>] | | Print `<text>`; with `>` redirection, write it to `<file>` via a single `writeFile` (no tmp/append dance — works on synthetic nodes). Requires `+w` on the target for non-root. e.g. **echo hello**, **echo 1 > /sys/class/gpio/5/value**, **echo x > /dev/null**. Note: comma is the shell's option separator, so `<text>` should not contain commas. |
| fwrite \<file> | f=\<file> v=\<value> | Open file in append mode; type content line by line. Press **ESC** to save & exit. Requires `+w` on the file for non-root. e.g. **fwrite /home/notes.txt** or **fwrite f=/home/notes.txt v=hello** |
| head \<file> [N] | | Print first N lines (default 10); constant memory. e.g. **head /home/log.txt 5** |
| tail \<file> [N] | | Print last N lines (default 10); constant memory. e.g. **tail /home/log.txt 5** |
| wc \<file> | | Print line / word / byte counts (Linux `wc` order). e.g. **wc /home/log.txt** |
| df | | Print one row per mounted VFS backend: `MOUNT NAME TOTAL USED FREE` in bytes. e.g. **df** |
| mount | | List active VFS mount points with prefix / type / backend name. e.g. **mount** |
| chmod \<octal> \<path> | | Set POSIX permission bits (e.g. `0644`, `0755`). Owner-or-root only (enforced by VFS). e.g. **chmod 0644 /etc/passwd** |
| chown \<uid>[:\<gid>] \<path> | | Change owning uid (and optionally gid) of a file/dir. Root-only. If `:gid` is omitted, `gid = uid`. e.g. **chown 1001 /home/alice** or **chown 1001:1001 /home/alice** |
| umask [\<octal>] | | Print current session's umask; with an arg, set it. Applied at file/dir creation as `default_perms & ~umask`. Default `0022`. e.g. **umask 0027** |
| hexdump \<file> | | `hexdump -C` layout: offset, 16 hex bytes, ASCII. e.g. **hexdump /home/bin.dat** |
| grep \<pattern> \<file_or_dir> | | Search a file or dir (recursive). Output `path:line:col:content` (vim/vscode jump format). Regex subset: `.` `*` `+` `?` `^` `$` `[abc]` `[a-z]` `[^abc]` `\\<char>`. No alternation / groups / backrefs. e.g. **grep ^ERROR /home/log.txt** |
| cls | | Clear screen. e.g. **cls** |
| cd \<dir> | | Change directory. e.g. **cd /home/scripts** |
| login | u=\<user> p=\<pass> | Interactive login (three-line: user then pass). Also inline as `login u=<u>, p=<p>` (comma separator on this legacy command). |
| logout | | End the session. Serial returns to `login:` prompt; Telnet/SSH channel is closed. |
| whoami | | Print current session's username. e.g. **whoami** |
| id | | Print `uid=N(username) gid=N` for the current session's user. e.g. **id** |
| who | | List active authenticated sessions across serial/telnet/ssh: `USER TTY SID LOGIN IDLE` in seconds. e.g. **who** |
| groups | | Print the current user's primary gid. e.g. **groups** |
| su u=\<user> p=\<pass> | u, p (space-separated) | Switch user in the current session. Prompts interactively when args omitted. On success sets session identity + caches uid/gid + jumps to target's home dir. Password verification enters a privileged VFS scope to read `/etc/shadow` (0600). e.g. **su u=alice p=alice123** |
| passwd p=\<curr> n=\<new> c=\<confirm> | p, n, c (space) | Change own password. Three-phase interactive when omitted (`current:` / `new:` / `confirm:`), all echo-suppressed. Enters a privileged VFS scope for the `/etc/shadow` update. e.g. **passwd p=oldpw n=newpw c=newpw** |
| useradd u=\<user> p=\<pass> | u, p (space) | **Root-only.** Create a new user. UID auto-assigned to next free slot ≥1; `gid = uid`; home=`/`, shell=`cmd`. Writes both `/etc/passwd` and `/etc/shadow` (rolls back on shadow failure). e.g. **useradd u=alice p=alice123** |
| userdel u=\<user> | u (space) | **Root-only.** Delete a user from `/etc/passwd` + `/etc/shadow`. Refuses self-delete and uid=0 (root). e.g. **userdel u=alice** |
| srvc list \| status \<name> \| start \<name> \| stop \<name> \| restart \<name> | positional, space-separated | Service supervisor (systemd-lite). `list` prints every service with state; `status <name>` shows tracked PIDs; `start`/`stop`/`restart` deliver `SIG_CONT`/`SIG_STOP` (both) to every task the service owns. Root required for start/stop/restart. e.g. **srvc list**, **srvc stop GPIO** |
| ps [\<sid>] | | List active scheduler tasks POSIX-style with owner, state, %CPU, runs, interval, name. Optional positional filter by owner session id. e.g. **ps** or **ps 1** |
| top | i=\<ms>; n=\<iters>; u=\<sid> | Same view as `ps`, refreshed on a scheduler task at `i` ms (default 2000, min 500). `n` bounds iterations (omit for forever). Stop with Ctrl+C. e.g. **top i=1500; n=10** |
| kill [\<sig>] \<pid> | | Deliver a signal to a scheduler task. 1 arg = pid (default TERM). 2 args = sig then pid. Accepted signals: 9 KILL / 15 TERM / 18 CONT / 19 STOP. Root can hit any task; other users only tasks they own. e.g. **kill 8** or **kill 9 8** |
| pkill [\<sig>] \<name> | | Same as `kill` but matches by task name; hits every task with that name. 1 arg = name (default TERM). 2 args = sig then name. Prints `signaled N task(s)`. e.g. **pkill MQTT** or **pkill 19 MQTT** |
| killall [\<sig>] \<name> | | Like `pkill` but default signal is `SIG_KILL` (impolite). e.g. **killall GPIO** |
| renice \<nice> \<pid> | nice signed -20..19 | Change POSIX nice on a live task. Auto-triggers scheduler resort. Same owner/root gate as `kill`. e.g. **renice -5 10** |
| ssh q=\<query>,t=\<algo> | q=\<query> t=\<algo> | SSH command. q=1 (SSH_COMMAND_QUERY_KEYGEN) creates keypair of given algo. e.g. **ssh q=1,t=2** |
| net \<options> | ip, scansta, connsta | Query network params. **ip** shows STA/AP info; **scansta** lists nearby SSIDs; **connsta** joins one. e.g. **net connsta,\<ssid>,\<password>** |
| reboot | | Reboot the device. e.g. **reboot** |
| watch | c=\<command> i=\<interval_ms> n=\<iterations> | Run a command periodically. Default interval 1 s, infinite iterations. Stop with Ctrl+C. Options separated by `;`. e.g. **watch c=net ip; i=3000; n=10** |
| iot \<options> | setid, getid, sethost, gethost | Manage IoT config. **setid/getid** for device unique ID; **sethost/gethost** for IoT HTTP host. e.g. **iot setid,\<DeviceID>** or **iot sethost,\<HostAddress>** |
| help | | List every registered command with its one-line usage (from each command's `getUsage()`; name column padded to 12 chars for alignment). Available before login. Tab-completion works on partial names. e.g. **help** |
| uptime | | Time since boot as `up Xd Yh Zm Ws`. Wraps at ~49.7 days. e.g. **uptime** |
| tls q=\<query>,t=\<algo>,l=\<bits>,n=\<CN/DNS>,i=\<IPv4> | q (1=CERTGEN), t (0=EC, 1=RSA), l (key bits / curve size), n (CN or DNS SAN), i (IPv4 SAN) | On-device TLS cert generation. esp32 builds with `ENABLE_TLS_CERT_GENERATION` only. Output at `TLS_DEFAULT_SERVER_CERT_PATH` / `TLS_DEFAULT_SERVER_KEY_PATH`. e.g. **tls q=1,t=0,l=256,n=device.local,i=192.168.1.50** |

Each command's implementation lives in [src/service_provider/cmd/commands/](src/service_provider/cmd/commands/) — one `<Name>Command.h` per verb, with names registered in [CommandCommon.h](src/service_provider/cmd/commands/CommandCommon.h).

**Path arguments** work the POSIX way in every file command (`ls`, `cd`, `cat`, `cp`, `mv`, `rm`, `mkdir`, `touch`, `chmod`, `chown`, `head`, `tail`, `wc`, `grep`, `hexdump`, `fwrite`): a leading `/` is treated as an absolute path, anything else resolves against the session's current directory. `cd` additionally supports `~` (home) and `-` (previous directory).

**Positional vs named options.** Commands with ≤2 required args use the **positional** style (`setAcceptArgsOptions(true)` + space separator, args at `m_options[0]`/`[1]`) — matches POSIX and reduces typing: `chmod 0644 /etc/passwd`, `renice -5 10`, `kill 9 12`. Commands with an optional leading arg (`kill [<sig>] <pid>`, `pkill`, `killall`) dispatch on **arg count** — 1 arg = target, 2 args = sig then target — no numeric-vs-name ambiguity. Named options (`x=y`) are retained for commands with many optional slots, sensitive input (`login`/`su`/`passwd`/`useradd`/`userdel`), or optional-in-the-middle patterns (`top i=… n=… u=…`).

### 7.8 Multi-terminal session lifecycle

Up to `PDI_MAX_SESSIONS` (default 3) sessions can run concurrently — one each across serial, telnet, and ssh — with fully independent state. There is one `CommandLineServiceProvider` singleton dispatcher, and one `SessionManager::m_sessions[PDI_MAX_SESSIONS]` slot array; each slot holds its own `linebuf`, `cursor`, `history/autocomplete` cursors, `cwd`, `isAuthorized`, `username`, `uid`, `gid`, `umask`, `loginAt`, `lastActivityAt` (see [§6.2.18](#6218-sessionmanager)). The uid/gid/umask trio is populated at login (`AuthServiceProvider::setAuthorized(true)`) so every downstream FS check is a single dereference — no per-op `/etc/passwd` scan.

```
boot
 └─ PdiStack::initialize
       __i_fs.init()
       __user_store_service.initService()          // bootstraps /etc/passwd + /etc/shadow if missing
       SessionManager::attach(serialTerminal)      // pins slot 0 for serial
       login prompt on serial

(any time) telnet client connects on :23
 └─ TelnetServiceProvider::handle()
       accept() → iClientInterface* c
       __cmd_service.useTerminal(c) ─┐
                                      ├─ SessionManager::attach(c) → next free slot
                                      └─ startInteraction() draws login: on c
       every tick with c-side data:
           processTerminalInput(c) → findByTerminal(c) → SessionManager::setCurrent(s)
                                    → session-scoped input/exec
       on disconnect:
           SessionManager::detach(c)               // slot freed

(any time) SSH client connects on :22
 └─ USERAUTH_SUCCESS:
       SessionManager::attach(m_sshclient) + setAuthorized(true)   // anchor auth to SSH slot
    channel-open:
       __cmd_service.useTerminal(m_sshclient)     // idempotent attach + prompt
    shell data ticks:
       processTerminalInput(m_sshclient) → same session-scoped path
    close:
       SessionManager::detach(m_sshclient)
```

Two invariants make it work:

1. **Per-tick context switch.** At the top of `processTerminalInput(t)`, cmd_service calls `SessionManager::setCurrent(findByTerminal(t))` and re-points the static `ServiceProvider::m_terminal` to `t`. Downstream code (prompt drawing, new command instances, `__auth_service` delegators) automatically sees the right session for this tick without any explicit parameter passing.

2. **Owner-tagged in-flight commands.** Every `cmd_t` created via `getCommandToExecute` gets `m_owner = SessionManager::current()`. `getCommandWaitingForUserInput` and `getActiveCommandByName` filter by owner, so a Telnet-side `login` prompt waiting for a username never picks up SSH-side keystrokes as its input.

Constraints:
- Telnet transport currently accepts **one client at a time** ([TelnetServiceProvider.cpp](src/service_provider/transport/TelnetServiceProvider.cpp)); the session layer supports more, but Telnet needs a client-vector refactor to exercise it.
- Long-running commands (e.g. `watch`) capture their `m_terminal` and `m_owner` at creation time, so their output continues to flow to the correct session regardless of what other sessions do in later ticks.

### 7.9 SFTP / SCP file transfer

The SSH service ([§6.2.14](#6-service-providers)) opens an SFTP subsystem on demand. Two clients are supported: `scp -s …` (force SFTP) for single-file copies, and interactive `sftp` for full session browsing. Both ride the same handlers in `handleChannelSubsystemSftpRequest`.

#### User-facing commands

Upload a file from the desktop to the device (as user `pdiStack`):

```
scp -s <desktop-path-to-test.txt> pdiStack@<device-ip>:<dest-path-to-store-test.txt>
```

Download a file from the device to the desktop:

```
scp -s pdiStack@<device-ip>:<dest-path-to-test.txt> <desktop-path-to-store-test.txt>
```

Open an interactive SFTP shell to browse and manage the device's storage:

```
sftp -P 22 pdiStack@<device-ip>
```

The following SFTP operations are implemented (sufficient for OpenSSH `sftp` client interactive use):

| Operation | sftp command | Purpose |
|---|---|---|
| `REALPATH` | `pwd`, `cd` | resolve relative / `.` / `..` paths to absolute |
| `STAT` / `LSTAT` / `FSTAT` | `cd`, `ls`, `stat` | file/dir attributes (size, type) |
| `OPENDIR` / `READDIR` / `CLOSE` | `ls`, `ls -l` | directory listing (paginated, 16 entries/response) |
| `OPEN` / `READ` / `WRITE` / `CLOSE` | `get`, `put` | file read / create / write |
| `MKDIR` / `RMDIR` | `mkdir`, `rmdir` | directory create / remove |
| `REMOVE` | `rm` | delete file |
| `RENAME` | `rename` | rename file or directory (does not overwrite) |
| `SETSTAT` / `FSETSTAT` | `chmod`, `chown` | accepted but no-op (FS has no perm/time storage) |
| `READLINK` / `SYMLINK` | — | reported as unsupported (no symlink layer) |

Idle SFTP sessions are reaped automatically after the configured timeout (default 60 s) so a suspended or dead client never wedges the single-session slot. Interactive SSH shell sessions are NOT idle-reaped — only the SFTP subsystem is.

**Note** the framework handles the SSH chunks in smaller size which can make the file transfer little bit slower. On top if we are overwriting an existing file then it can add the file-edit overhead as well. So speed can be as slow as 0.2 kbps and as fast as 1 kbps.

Implementation notes:
- Bolus chunks (the SFTP protocol's data records) are received small (≤256 B per the SSH crypto window) and streamed straight into `__i_fs` — there is no full-file RAM buffer.
- File-edit overhead (overwriting an existing file) is significant on flash filesystems; the README's 0.2-1 kB/s figure reflects this.
- Only **one SFTP handle at a time** is tracked per session — opening a second file/dir while one is still open reuses the slot. Sufficient for `sftp` interactive use (one operation in flight) but not for parallel transfers.
- Directory listings are cached once at `OPENDIR` time into the session (`Sftp::dir_entries`) and paginated across multiple `READDIR` responses (16 entries per response). Memory is released on `CLOSE` or session destruction (RAII).
- SFTP sessions only have an **idle timeout** (default 60 s, in `handle()`); plain SSH shell sessions are not idle-reaped because a human may pause at the prompt indefinitely.

### 7.10 Background commands and CTRL+C semantics

`watch` and `top` are the two built-ins that run as scheduler tasks in the background of a shell session. Both follow the same template — the model any user-supplied long-running command should copy:

1. Parse args and hold any pointers that must outlive a tick (e.g. `holdOptionValue("c")` in `watch` so the child command string survives).
2. Register a scheduler task via `__task_scheduler.register_task(fn, interval, ..., name)` with the command name so it shows up in `ps`.
3. Set `m_runinbackground = true` and return `CMD_RESULT_INCOMPLETE` so the dispatcher keeps the command instance alive.
4. Override `stopRunningInBackground()` to call `__task_scheduler.remove_task(...)` on the tracked id and clear `m_runinbackground`.

**Ctrl+C dispatch is generic.** [CommandLineServiceProvider.cpp](src/service_provider/cmd/CommandLineServiceProvider.cpp) walks its `m_cmdlist` and calls `stopRunningInBackground()` on **every** command with `isRunningInBackground() == true` owned by the current session. So Ctrl+C works out of the box for any new background command — no per-command wiring in the shell dispatcher.

For a live inspection of what's currently running in the background, use `ps` — background commands appear with their `CMD_NAME_*` as the row name (e.g. `watch`, `top`).

### 7.11 Adding a new command

To add `temp` (read a temperature sensor):

1. **Decide options.** Say `t=<unit>` (C/F) — one option, default C.
2. **Add the name** to [CommandCommon.h](src/service_provider/cmd/commands/CommandCommon.h): `#define CMD_NAME_TEMP "temp"`.
3. **Create [src/service_provider/cmd/commands/TempCommand.h](src/service_provider/cmd/commands/TempCommand.h)**:
   ```cpp
   struct TempCommand : public CommandBase {
       TempCommand() {
           Clear();
           SetCommand(CMD_NAME_TEMP);
           AddOption(CMD_OPTION_NAME_T);
       }
       const char* getUsage() const override {
           return RODT_ATTR("temp [t=C|F]  read the temperature sensor (default Celsius)");
       }
       bool needauth() override { return true; }
       cmd_result_t execute(cmd_term_inseq_t) override {
           auto unit = RetrieveOption(CMD_OPTION_NAME_T);
           bool celsius = !(unit && unit->optionval[0] == 'F');
           float t = readSensor(celsius);
           m_terminal->writeln(t);
           return CMD_RESULT_OK;
       }
       static void* Registrar(void*) { static TempCommand cmd; return &cmd; }
   };
   ```
   The `getUsage()` string is what `help` prints and what `CommandBase::ResultToTerminal` prints under `CmdErr` on `CMD_RESULT_ARGS_MISSING` / `_ARGS_ERROR` / `_INVALID_OPTION` — one source of truth per command, always in flash.
4. **Register it** by including the header in [CommandLineServiceProvider.h](src/service_provider/cmd/CommandLineServiceProvider.h) **and** calling `CommandBase::RegisterCommand(CMD_NAME_TEMP, &TempCommand::Registrar)` from `CommandLineServiceProvider::initService` (mirror the pattern used by sibling commands).
5. **Done.** `temp` will autocomplete via TAB, appear in history, respect CTRL+C if you ever make it multi-iteration, and show up in `help` with its usage line.

### 7.12 Gotchas

- **`CMD_SIZE_MAX = 8`** — command names are silently truncated past 8 chars. Use short verbs.
- **`CMD_OPTION_MAX = 3`** and **`CMD_OPTION_SIZE_MAX = 3`** — a command with 4+ options or option names longer than 2 chars (`+ null`) must split into sub-commands or use positional args.
- **One bound terminal at a time.** Telnet and SSH cannot both have the prompt simultaneously — the *first* connection wins until it disconnects. There is no session multiplexing.
- **History needs storage.** Without `ENABLE_STORAGE_SERVICE` the ↑/↓ keys are dead; the autocomplete via TAB still works (registry lives in RAM).
- **Option values are pointers into the input buffer by default.** If your `execute` may live across input arrivals (CTRL+C handler, multi-iteration), `holdOptionValue` first.
- **`needauth()` is the only gate.** A command returning `true` only fires after a successful `login`; setting it on a "harmless" command still adds friction. Use it for anything mutating state.
- **`watch` semicolon-separates options because the inner command can use commas.** When chaining your own composite commands, pick a separator that doesn't appear in payloads.
- **`fwrite` blocks the line until ESC.** Don't run it over telnet/SSH if you also rely on the watchdog firing inside the session — it does, but the terminal is unavailable for `srvc` / `ps` until you exit.
- **No quoted args.** Values cannot contain `,` (or whichever separator), `=`, or spaces inside an option value. Escape at the producer or accept the constraint.

---

## 8. Web Server

The web server is the **on-device admin portal**. When `ENABLE_HTTP_SERVER` is on, every device runs an HTTP/1.1 server on its access-point IP (`192.168.0.1` by default) that lets a phone or laptop log in, configure WiFi, drive GPIO pins, watch the dashboard, upload files, and manage MQTT/OTA/Email/IoT — without flashing or serial cables. With `ENABLE_HTTPS_SERVER` (+ `ENABLE_TLS_SERVICE`) the same portal serves on port 443 with TLS-wrapped sockets — no controller / page / route changes, just a different `begin()` call and a cert/key on disk. See [§8.7.1 HTTPS wiring](#871-https-wiring--certificates) for the full flow.

Structurally it is the framework's largest subsystem (≈7 KLOC), but it follows a small handful of concepts: an `HttpServer` orchestrator, a `RouteHandler` that maps URIs to callbacks, a `Middleware` chain (currently mostly auth), a `WebResourceProvider` that owns the response buffer and the static fragments, per-feature `Controller`s, and a set of HTML page templates broken into header/body/footer.

Implementation: [src/webserver/](src/webserver/). Global instance: `__web_server`. Wired into `PdiStack::initialize` ([PdiStack.cpp](src/PdiStack.cpp)) and ticked from `PdiStack::serve` via `__web_server.handle_clients()` ([PdiStack.cpp](src/PdiStack.cpp)).

### 8.1 Layered model

```
              ┌────────────────────────────────────────────┐
  iHttpServer │  iHttpServerInterface  (port: §13.3.3)      │  ← bytes in/out, route registration
  Interface   │  default: HttpServerInterfaceImpl          │
              └──────────────────────┬─────────────────────┘
                                     │  on(uri, cb), arg(), header(), send()
              ┌──────────────────────▼─────────────────────┐
              │  HttpServer  ─ owns one instance of every  │  ← src/webserver/WebServer.{h,cpp}
              │  *Controller, ENABLE_*-gated.              │     (subclasses ServiceProvider)
              └──────────────────────┬─────────────────────┘
                                     │  controllers self-boot
              ┌──────────────────────▼─────────────────────┐
              │  Controller (base)  +  HomeController,     │  ← src/webserver/controllers/
              │  LoginController, DashboardController,     │
              │  OtaController, WiFiConfigController,      │
              │  GpioController, MqttController,           │
              │  EmailConfigController, DeviceIotController│
              │  StorageController                          │
              └────────┬──────────────────────────┬────────┘
                       │ register_route(uri, fn,  │  build response via
                       │   middleware, redirect)  │  m_web_resource & helpers
                       ▼                          ▼
              ┌──────────────────────┐  ┌──────────────────────────────────┐
              │  RouteHandler        │  │  WebResourceProvider (m_web_     │  ← resources/
              │  __web_route_handler │  │  resource): page buffer, server  │
              └──────────┬───────────┘  │  ptr, DB accessors, MIME helpers │
                         │              └────────────────┬─────────────────┘
                         ▼                               │
              ┌──────────────────────┐                   │  emits HTML using
              │  Middleware          │                   ▼
              │  ├─ AUTH_MIDDLEWARE  │   ┌──────────────────────────────────┐
              │  │   (session check) │   │  pages/  (Header, Footer,        │
              │  └─ EwSessionHandler │   │  HomePage, Dashboard, LoginPage, │
              │      (cookie ops)    │   │  *ConfigPage, StorageListPage,   │
              └──────────────────────┘   │  NotFound, LogoutPage)           │
                                         │  helpers/  (DynamicPageBuild     │
                                         │  Helper, HtmlTagsAndAttr,        │
                                         │  icon/SvgIcons)                  │
                                         └──────────────────────────────────┘
```

Reading the diagram: a request hits the port (`iHttpServerInterface::handleClient` ⇒ matches a URI to a registered lambda) ⇒ the lambda runs middleware ⇒ on pass, the controller method runs, pulls config from `__database_service`, composes HTML using header/page/footer fragments + `DynamicPageBuildHelper`, and `send`s through the server. On fail, middleware sends a `301` to `/login`.

### 8.2 `HttpServer` orchestrator

[WebServer.h](src/webserver/WebServer.h). Despite its name, this class is a `ServiceProvider` ([§6.2.12](#6-service-providers)) — but unlike the others, it directly owns its sub-objects as value members rather than registering as singletons elsewhere.

| Member | Always present | `ENABLE_*` gated |
|---|---|---|
| `HomeController m_home_controller` | yes | — |
| `DashboardController m_dashboard_controller` | yes | — |
| `LoginController m_login_controller` | yes | — |
| `OtaController m_ota_controller` | | `ENABLE_OTA_SERVICE` |
| `WiFiConfigController m_wificonfig_controller` | | `ENABLE_WIFI_SERVICE` |
| `GpioController m_gpio_controller` | | `ENABLE_GPIO_SERVICE` |
| `MqttController m_mqtt_controller` | | `ENABLE_MQTT_SERVICE` |
| `EmailConfigController m_emailconfig_controller` | | `ENABLE_EMAIL_SERVICE` |
| `DeviceIotController m_device_iot_controller` | | `ENABLE_DEVICE_IOT` |
| `StorageController m_storage_controller` | | `ENABLE_STORAGE_SERVICE` |

`initService(iHttpServerInterface*)` stashes the server pointer, attaches it to `__web_resource.m_server`, and iterates the static `Controller::m_controllers` registry calling each `controller->boot()`. Every controller's constructor pushes itself onto that registry (see [§8.4](#8-web-server)), so booting is automatic at static-init plus one pass at `initService` time.

The final step in `initService` is the `begin()` call — and this is where the HTTPS branch lives:

```cpp
#if defined(ENABLE_HTTPS_SERVER) && defined(ENABLE_TLS_SERVICE)
this->m_server->setServerCertificatePath(TLS_DEFAULT_SERVER_CERT_PATH);   // /etc/http/server.crt
this->m_server->setServerPrivateKeyPath(TLS_DEFAULT_SERVER_KEY_PATH);     // /etc/http/server.key
  #ifdef ENABLE_HTTPS_SERVER_MTLS
this->m_server->setClientCertificateAuthorityPath(TLS_DEFAULT_CLIENT_CA_PATH);
  #endif
this->m_server->begin(HTTPS_DEFAULT_PORT, /*secure=*/true);               // 443
#else
this->m_server->begin(HTTP_DEFAULT_PORT);                                 // 80
#endif
```

No controller, page, route, middleware, or session code is conditional on TLS — the only thing the build flips is which `begin()` overload runs at the bottom of `initService`. Cert/key paths are macros from [TlsConfig.h](src/config/TlsConfig.h) so a sketch can override them by `#define`-ing before any framework include.

`handle_clients()` is a one-liner: `m_server->handleClient()`. All actual work happens inside the route lambdas the controllers registered.

### 8.3 Route registry — `RouteHandler` + `Routes.h`

URIs are constants in [routes/Routes.h](src/webserver/routes/Routes.h). The full set:

| Route | Purpose | Middleware |
|---|---|---|
| `/` | Home / menu landing | None |
| `/login` | Login form + POST handler | None |
| `/logout` | Drop session, redirect to `/login` | AUTH |
| `/login-config` | Change username/password | AUTH |
| `/dashboard` | Live device summary | AUTH |
| `/listen-dashboard` | Long-poll/SSE-style dashboard updates | AUTH |
| `/wifi-config` | WiFi STA/AP form | AUTH |
| `/ota-config` | OTA host/port/version | AUTH |
| `/email-config` | SMTP credentials | AUTH |
| `/gpio-manage` | Per-pin GPIO panel | AUTH |
| `/gpio-server`, `/gpio-config`, `/gpio-write`, `/gpio-event` | GPIO subforms | AUTH |
| `/gpio-monitor`, `/listen-monitor` | Live GPIO graphs (analog) | AUTH |
| `/mqtt-manage`, `/mqtt-general-config`, `/mqtt-lwt-config`, `/mqtt-pubsub-config` | MQTT subforms | AUTH |
| `/device-register-config` | IoT register / OTP | AUTH |
| `/storage`, `/storage-fileupload`, `/storage-filelist`, `/storage-filedel` | File browser & upload | AUTH |

[RouteHandler](src/webserver/handlers/RouteHandler.h) extends `Middleware` and adds two methods:

```cpp
void register_route(const char* uri,
                    CallBackVoidArgFn fn,
                    middlwares level = NO_MIDDLEWARE,
                    const char* redirect = WEB_SERVER_LOGIN_ROUTE);
void register_not_found_fn(CallBackVoidArgFn fn);
```

Global instance: `__web_route_handler`. Controllers reach it via `m_route_handler` (a base-class pointer set in `Controller`'s ctor).

### 8.4 `Controller` base class and the controller registry

[Controller.h](src/webserver/controllers/Controller.h). Each controller:

- Inherits `Controller`, which on construction pushes `this` into the static `Controller::m_controllers` vector.
- Holds protected pointers `m_web_resource` (the response buffer / page helpers) and `m_route_handler` (the global route registry).
- Implements `boot()` to register its routes.

A typical controller body (paraphrased from [LoginController.h](src/webserver/controllers/LoginController.h)):

```cpp
class LoginController : public Controller {
public:
    LoginController() : Controller("LoginController") {}

    void boot() override {
        m_route_handler->register_route(
            WEB_SERVER_LOGIN_ROUTE,
            [&] { this->handleLoginRoute(); });

        m_route_handler->register_route(
            WEB_SERVER_LOGOUT_ROUTE,
            [&] { this->handleLogoutRoute(); });

        m_route_handler->register_route(
            WEB_SERVER_LOGIN_CONFIG_ROUTE,
            [&] { this->handleLoginConfigRoute(); },
            AUTH_MIDDLEWARE);                    // gated
    }

private:
    void handleLoginRoute();
    void handleLogoutRoute();
    void handleLoginConfigRoute();
};
```

#### Controller inventory

| Controller | Source | Routes owned |
|---|---|---|
| `HomeController` | [HomeController.h](src/webserver/controllers/HomeController.h) | `/`, plus the `register_not_found_fn` 404 handler |
| `LoginController` | [LoginController.h](src/webserver/controllers/LoginController.h) | `/login`, `/logout`, `/login-config` |
| `DashboardController` | [DashboardController.h](src/webserver/controllers/DashboardController.h) | `/dashboard`, `/listen-dashboard` |
| `WiFiConfigController` | [WiFiConfigController.h](src/webserver/controllers/WiFiConfigController.h) | `/wifi-config` |
| `OtaController` | [OtaController.h](src/webserver/controllers/OtaController.h) | `/ota-config` |
| `GpioController` | [GPIOController.h](src/webserver/controllers/GPIOController.h) (~900 lines) | All `/gpio-*` |
| `MqttController` | [MQTTController.h](src/webserver/controllers/MQTTController.h) (~560 lines) | All `/mqtt-*` |
| `EmailConfigController` | [EmailConfigController.h](src/webserver/controllers/EmailConfigController.h) | `/email-config` |
| `DeviceIotController` | [DeviceIotController.h](src/webserver/controllers/DeviceIotController.h) | `/device-register-config` |
| `StorageController` | [StorageController.h](src/webserver/controllers/StorageController.h) | All `/storage-*` |

### 8.5 Middleware

[Middleware.h](src/webserver/middlewares/Middleware.h). Three levels:

| Level | Action |
|---|---|
| `NO_MIDDLEWARE` | Pass straight through to the route handler |
| `AUTH_MIDDLEWARE` | Check `EwSessionHandler::has_active_session()`; on fail, send `301` with `Location: <redirect>` (default `/login`) and abort the route |
| `API_MIDDLEWARE` | Currently a no-op stub — slot reserved for API-key/HMAC validation |

The middleware machinery extends `EwSessionHandler` rather than being a separate chain. Adding a new middleware level means: extend the `middlwares` enum, add an `else if` branch in `handle_middleware`, and route handlers can opt in with the new value.

### 8.6 Sessions — `EwSessionHandler`

[SessionHandler.h](src/webserver/handlers/SessionHandler.h). The framework's session model is **cookie-based**:

- Session name comes from `login_credential_table::session_name` (default `pdi_session` from [ServerConfig.h](src/config/ServerConfig.h)).
- Cookie max-age comes from `login_credential_table::cookie_max_age` (default 300 s).
- `EW_COOKIE_BUFF_MAX_SIZE = 60` is the working buffer used to read/parse the incoming `Cookie:` header.
- `create_session()` builds a `Set-Cookie` header with a derived token and writes it on the login response.
- `has_active_session()` parses the request's `Cookie:` header against the persisted credentials.
- `send_inactive_session_headers()` overwrites with an expired cookie — used on `/logout` and 301 redirects.

The session token is derived per-login rather than rotated periodically; a logout invalidates it on the client only. There is no server-side session table — the auth layer is intentionally minimal.

### 8.7 Response composition — `WebResourceProvider` and pages

[WebResource.h](src/webserver/resources/WebResource.h). Holds:

- `iHttpServerInterface* m_server` — the current request/response context.
- The page buffer (`PAGE_HTML_MAX_SIZE = 1800` bytes — yes, **per-page HTML is capped at ~1.8 KB**; bigger pages stream in chunks via `send(..., send_in_chunks=true)`).
- Accessor shortcuts into `__database_service` so controllers don't need to import the DB headers.

`collect_resource(iHttpServerInterface*)` is called from `HttpServer::initService` to stash the server pointer that controllers will reach through `m_web_resource->m_server` for `arg()`, `hasArg()`, `header()`, `addHeader()`, `send()`.

#### Pages

[src/webserver/pages/](src/webserver/pages/) ships HTML fragments as raw C strings declared in `PROG_RODT_ATTR` (program memory). The composition pattern is fixed three-part:

```cpp
m_web_resource->m_server->send(
    HTTP_RESP_OK, MIME_TYPE_HTML, headerHtml,  /*chunked=*/true);
m_web_resource->m_server->send(
    HTTP_RESP_OK, MIME_TYPE_HTML, bodyHtml,    /*chunked=*/true);
m_web_resource->m_server->send(
    HTTP_RESP_OK, MIME_TYPE_HTML, footerHtml,  /*chunked=*/true);
```

[Header.h](src/webserver/pages/Header.h) and [Footer.h](src/webserver/pages/Footer.h) are the framing every page reuses. The middle slot is one of [HomePage.h](src/webserver/pages/HomePage.h), [Dashboard.h](src/webserver/pages/Dashboard.h), [LoginPage.h](src/webserver/pages/LoginPage.h), [WiFiConfigPage.h](src/webserver/pages/WiFiConfigPage.h), [OtaConfigPage.h](src/webserver/pages/OtaConfigPage.h), [GpioConfigPage.h](src/webserver/pages/GpioConfigPage.h), [MqttConfigPage.h](src/webserver/pages/MqttConfigPage.h), [EmailConfigPage.h](src/webserver/pages/EmailConfigPage.h), [DeviceIotPage.h](src/webserver/pages/DeviceIotPage.h), [LoginConfigPage.h](src/webserver/pages/LoginConfigPage.h), [LogoutPage.h](src/webserver/pages/LogoutPage.h), [StorageListPage.h](src/webserver/pages/StorageListPage.h), [NotFound.h](src/webserver/pages/NotFound.h).

#### Helpers

| Helper | Purpose |
|---|---|
| [DynamicPageBuildHelper.h](src/webserver/helpers/DynamicPageBuildHelper.h) | Convert C structs into HTML form inputs (text/number/checkbox/select), generate tables, escape values, build option lists — the bulk of dynamic markup |
| [HtmlTagsAndAttr.h](src/webserver/helpers/HtmlTagsAndAttr.h) | String constants for common tag/attr names — keeps allocations down |
| [icon/SvgIcons.h](src/webserver/helpers/icon/SvgIcons.h) | Inline SVGs used in the menu / dashboard |

#### 8.7.1 HTTPS wiring & certificates

When `ENABLE_HTTPS_SERVER` is on alongside `ENABLE_TLS_SERVICE`, the **same** `HttpServerInterfaceImpl` ([src/interface/pdi/impl/middlewares/HttpServerInterfaceImpl.{h,cpp}](src/interface/pdi/impl/middlewares/HttpServerInterfaceImpl.h)) runs in TLS mode. The split of responsibility is:

| Layer | What it does in HTTPS mode |
|---|---|
| `HttpServer::initService` ([§8.2](#82-httpserver-orchestrator)) | Calls `setServerCertificatePath` / `setServerPrivateKeyPath` (+ `setClientCertificateAuthorityPath` if `ENABLE_HTTPS_SERVER_MTLS`), then `begin(HTTPS_DEFAULT_PORT, secure=true)`. Paths come from [TlsConfig.h](src/config/TlsConfig.h) macros. |
| `iHttpServerInterface` ([§13.3.3](#13-portable-interfaces)) | Adds the TLS-only methods `setServerCertificatePath`, `setServerPrivateKeyPath`, `setClientCertificateAuthorityPath`, and the `secure` arg on `begin(port, secure)`. All three setters are no-op stubs in the non-TLS build, so existing ports compile unchanged. |
| `HttpServerInterfaceImpl::begin(port, secure=true)` | Switches the listener over to `__i_instance.getNewTlsServerInstance()`. Each accepted client is a `iTlsClientInterface*` masquerading as the same `iClientInterface` shape the request parser already uses — so handler code never sees raw TLS frames. |
| Port-level TLS backend | esp8266 → BearSSL, esp32 → mbedTLS — loads PEM/DER from the FS at the configured paths via `TlsCryptoLoader` ([devices/esp8266/BearSSLCertLoader.{h,cpp}](devices/esp8266/BearSSLCertLoader.h), [devices/esp32/MbedTLSCertLoader.{h,cpp}](devices/esp32/MbedTLSCertLoader.h)). |
| Cert provisioning | Two paths — on-device `tls q=1,t=…,n=…,i=…` CLI command (esp32 only, gated on `ENABLE_TLS_CERT_GENERATION`; auto-mint at first STA-got-IP if `ENABLE_SERVER_TLS_CERT_GENERATION_AT_RUNTIME`); off-device [scripts/GenTlsCerts.py](scripts/GenTlsCerts.py) for esp8266. See [§6.2.16](#6-service-providers). |

Required filesystem layout (defaults from [TlsConfig.h](src/config/TlsConfig.h) — overridable per project):

| Path macro | Default | Purpose |
|---|---|---|
| `TLS_DEFAULT_SERVER_CERT_PATH` | `/etc/http/server.crt` | HTTPS server certificate (PEM; may be a chain) |
| `TLS_DEFAULT_SERVER_KEY_PATH` | `/etc/http/server.key` | HTTPS server private key (PEM; EC or RSA) |
| `TLS_DEFAULT_CLIENT_CA_PATH` | `/etc/http/client-ca.crt` | CA bundle for mTLS client-cert verification (only if `ENABLE_HTTPS_SERVER_MTLS`) |

Upload these via SFTP ([§6.2.14](#6-service-providers)) after first boot, then `reboot` — the HTTPS listener picks them up on the next `initService` pass. The storage service creates `/etc/http/` lazily; you don't need to `mkdir` it ahead of time.

HTTPS-specific response headers worth knowing:

- **`Strict-Transport-Security`** — emitted by the HTTPS server when `HTTPS_HSTS_MAX_AGE_SECONDS` ([HttpConfig.h](src/config/HttpConfig.h)) is non-zero. Default is **0** (header not sent). Turn on only with a CA-signed cert; with a self-signed cert HSTS hard-fails the browser click-through and locks the portal out.

### 8.8 Request lifecycle (end to end)

```
client request
    │
    ▼
iHttpServerInterface::handleClient()
    │  parse method + URI + headers + args
    │
    ▼
matching route lambda registered in __web_route_handler
    │  (the lambda was captured by Controller::boot())
    │
    ▼
Middleware::handle_middleware(level, redirect)
    │  if AUTH and !has_active_session(): send 301 → /login, return
    │
    ▼
Controller::handleXxxRoute()
    │  read args/headers via m_web_resource->m_server->arg(name)
    │  pull config: __database_service.get_<x>_config_table(&cfg)
    │  on POST: validate, set table, optionally fire event / restart service
    │  build HTML: header + page + footer through send(..., chunked=true)
    │
    ▼
iHttpServerInterface::send(code, mime, body, chunked)
    │
    ▼
bytes back to client
```

### 8.9 Per-route data flow examples

**`/wifi-config` POST** (from [WiFiConfigController.h](src/webserver/controllers/WiFiConfigController.h)):
1. Middleware: AUTH passes.
2. Read `arg("sta_ssid")`, `arg("sta_password")`, AP fields.
3. `__database_service.get_wifi_config_table(&cur)` to preserve untouched fields.
4. Apply args to the struct.
5. `__database_service.set_wifi_config_table(&cur)`.
6. Emit success page; schedule `__wifi_service.configure_wifi_station(...)` via `__task_scheduler.setTimeout(..., 1)` so the response flushes before the radio drops.
7. Device reconnects with new credentials.

**`/listen-monitor` (analog GPIO live graph)**:
- Browser issues repeated GETs (long-poll style); each returns the latest analog sample as JSON.
- `GpioController::handleAnalogMonitor` reads the pin via `__i_dvc_ctrl.gpioRead(ANALOG_READ, pin)`, builds `{ "p":N, "v":val }`, sends.

**`/storage-fileupload` (multipart)**:
- `StorageController` reads the upload via the server's chunked body API, writes to the file system via `__i_fs.createFile/appendFile`.
- Filename is sanitised against `MIN_ACCEPTED_ARG_SIZE`.

### 8.10 Adding a new page

To add a `/metrics` page that shows live counters:

1. **Add the route constant** in [routes/Routes.h](src/webserver/routes/Routes.h):
   ```c
   #define WEB_SERVER_METRICS_ROUTE "/metrics"
   ```
2. **Create `MetricsPage.h`** under [src/webserver/pages/](src/webserver/pages/) with the body HTML as a `PROG_RODT_ATTR` const.
3. **Create `MetricsController`** under [src/webserver/controllers/](src/webserver/controllers/):
   ```cpp
   class MetricsController : public Controller {
   public:
       MetricsController() : Controller("MetricsController") {}
       void boot() override {
           m_route_handler->register_route(
               WEB_SERVER_METRICS_ROUTE,
               [&]{ this->handleMetrics(); },
               AUTH_MIDDLEWARE);
       }
   private:
       void handleMetrics() {
           m_web_resource->m_server->send(HTTP_RESP_OK, MIME_TYPE_HTML, headerHtml, true);
           m_web_resource->m_server->send(HTTP_RESP_OK, MIME_TYPE_HTML, metricsHtml, true);
           m_web_resource->m_server->send(HTTP_RESP_OK, MIME_TYPE_HTML, footerHtml, true);
       }
   };
   ```
4. **Add a value-member** in [WebServer.h](src/webserver/WebServer.h) (under the matching `ENABLE_*` if optional), plus `#include` of the controller header. No registry plumbing — the base-class constructor self-registers.
5. **Add a menu entry** in the existing home/header page so users can navigate to it.

### 8.11 Gotchas

- **Page size cap (`PAGE_HTML_MAX_SIZE = 1800`).** A single non-chunked `send` is truncated to 1.8 KB. Always pass `send_in_chunks=true` for composed pages, and split big HTML into header/body/footer fragments.
- **Cookie storage is single-row.** Only one logged-in user at a time is well-supported; concurrent sessions reuse the same cookie token.
- **`API_MIDDLEWARE` is a stub.** Don't assume `API_MIDDLEWARE` enforces anything — if you're shipping an API surface, add the check now.
- **Page strings live in flash (`PROG_RODT_ATTR`).** Don't `strcpy` them into RAM unless you have to; pass them straight to `send()` which handles flash-string reads.
- **All controller routes go through one global `RouteHandler`.** Two controllers registering the same URI silently overwrite each other on the underlying `iHttpServerInterface::on()` — make URI constants unique.
- **`handle_clients` is called every `serve()` tick.** Long controller methods stall the entire main loop. If a route needs >100 ms of work (e.g. mass file delete), schedule it via `__task_scheduler.setTimeout(..., 1, now())` and return a 202 immediately.
- **No CSRF protection.** Forms rely on session cookies only; same-origin assumed. Don't expose the portal on the WAN.
- **Static-init order across controllers.** `Controller::m_controllers` is a static vector; controllers constructed during static init push into it. If your controller's constructor side-effects depend on another global, defer to `boot()` — which only runs from `initService`.
- **HTTPS or HTTP, not both.** The current `WebServer::initService` calls `begin()` exactly once — either on `HTTPS_DEFAULT_PORT (443)` with `secure=true` or on `HTTP_DEFAULT_PORT (80)`. Building with `ENABLE_HTTPS_SERVER` flips the whole portal to TLS; there's no parallel plaintext listener. If you need both (e.g. redirect 80 → 443), drive a second `iTcpServerInterface` from a sketch.
- **HTTPS fails closed if cert/key are missing.** With `ENABLE_HTTPS_SERVER`, the listener refuses connections (or never starts, depending on the port) when `/etc/http/server.crt` / `/etc/http/server.key` aren't on the FS. Provision before flipping the flag — or use `ENABLE_SERVER_TLS_CERT_GENERATION_AT_RUNTIME` on esp32 so an `EVENT_WIFI_STA_GOT_IP` listener auto-mints a self-signed pair on first boot.
- **Self-signed cert + browser HSTS is a foot-gun.** Default `HTTPS_HSTS_MAX_AGE_SECONDS = 0` keeps the header off. If you turn HSTS on while serving a self-signed cert, the browser pins HTTPS-only for the configured age and hard-fails the click-through — you'll lose portal access until the pin expires or you clear it manually.
- **Session cookie isn't marked `Secure`.** The framework's session cookie ([§8.6](#86-sessions--ewsessionhandler)) doesn't carry the `Secure` attribute even under HTTPS, so it'll fall through plaintext if you ever ran the portal on HTTP. After switching to HTTPS, factory-reset the credential row (or change the session name) to invalidate any HTTP-issued cookies still in client caches.

---

## 9. Logger

The logger is intentionally the smallest subsystem in the framework. There is **no log buffer, no rotating sink, no per-tag filtering, no JSON formatter** — just four levels, a printf-style varargs helper, and an interface that ports usually implement with one line per method. The cleverness is in *how it disappears*: when no log level is enabled, every `LogI(...)` / `LogE(...)` / `LogFmtI(...)` site in the codebase compiles to nothing — zero flash, zero RAM, zero runtime cost.

Implementation: [src/interface/pdi/iLoggerInterface.h](src/interface/pdi/iLoggerInterface.h) (port contract), [src/utility/DataTypeDef.h](src/utility/DataTypeDef.h) (default no-op macros), `devices/<board>/LoggerInterface.{h,cpp}` (port-side overrides).

### 9.1 Levels

| `logger_type_t` | Macro | Format-macro | When to use |
|---|---|---|---|
| `INFO_LOG` | `LogI(msg)` | `LogFmtI(fmt, args...)` | State transitions, lifecycle ("`Starting MQTT Service`") |
| `ERROR_LOG` | `LogE(msg)` | `LogFmtE(fmt, args...)` | Recoverable failures, validation rejects |
| `WARNING_LOG` | `LogW(msg)` | `LogFmtW(fmt, args...)` | Unusual conditions, deprecated paths |
| `SUCCESS_LOG` | `LogS(msg)` | `LogFmtS(fmt, args...)` | Confirmation of high-value operations (`"OTA applied"`) |

Plus one lifecycle macro:

- **`LOGBEGIN`** — expands to `__i_logger.init()`, called once from [`PDIStack::initialize`](src/PdiStack.cpp) to set up the logger sink (typically `Serial.begin(115200)`).

### 9.2 The four feature flags

| Flag | Effect |
|---|---|
| `ENABLE_LOG_INFO` | Enables `LogI` / `LogFmtI` |
| `ENABLE_LOG_ERROR` | Enables `LogE` / `LogFmtE` |
| `ENABLE_LOG_WARNING` | Enables `LogW` / `LogFmtW` |
| `ENABLE_LOG_SUCCESS` | Enables `LogS` / `LogFmtS` |
| `ENABLE_LOG_ALL` | Shorthand for *all of the above* |

All five are commented out by default in [devices/DeviceConfig.h](devices/DeviceConfig.h). The shipping configuration is **silent** — release builds emit nothing on Serial.

### 9.3 The disappearing-macro pattern

The trick that makes the logger zero-cost when off is a two-layer macro definition. **Layer 1** lives in [DataTypeDef.h](src/utility/DataTypeDef.h) and is the unconditional stub:

```cpp
#define LOGBEGIN
#define LogI(v)              // info log
#define LogE(v)              // error log
#define LogW(v)              // warning log
#define LogS(v)              // success log
#define LogFmtI(f, args...)
#define LogFmtE(f, args...)
#define LogFmtW(f, args...)
#define LogFmtS(f, args...)
```

Every translation unit that includes any utility header sees these — so `LogI("...")` is always legal C++, even on a build with no logger. It just expands to nothing.

**Layer 2** lives in the *device's* `LoggerInterface.h` (e.g. [devices/esp32/LoggerInterface.h](devices/esp32/LoggerInterface.h)) and runs only if (a) that header is actually included and (b) the matching `ENABLE_LOG_*` is on:

```cpp
#if defined(LogI) && (defined(ENABLE_LOG_INFO) || defined(ENABLE_LOG_ALL))
#undef LogI
#define LogI(v) __i_logger.log_info(RODT_ATTR(v))
#endif
```

Whether the device's `LoggerInterface.h` gets included is itself gated in the per-board aggregator ([devices/esp32/esp32_pdi.h](devices/esp32/esp32_pdi.h)):

```cpp
#if defined(LOGBEGIN) && \
    (defined(ENABLE_LOG_ALL) || defined(ENABLE_LOG_INFO) || \
     defined(ENABLE_LOG_ERROR) || defined(ENABLE_LOG_WARNING) || \
     defined(ENABLE_LOG_SUCCESS))
#include "LoggerInterface.h"
#endif
```

So the cascade is:

```
No ENABLE_LOG_* set
    ↓
devices/<board>/LoggerInterface.h is NOT included
    ↓
Layer-2 #undef/#define never runs
    ↓
Macros stay as the empty stubs from DataTypeDef.h
    ↓
Every Log* call site compiles to nothing
    ↓
__i_logger symbol is never referenced → linker drops it
```

Net effect: a *zero-byte* logger when disabled, identical to manually `#ifdef`-stripping every call site — but written naturally.

### 9.4 The port contract — `iLoggerInterface`

Pure-virtual sink with `init()` (called once via `LOGBEGIN`), a generic `log(type, content)` dispatcher, per-level fast paths (`log_info` / `log_error` / `log_warning` / `log_success`), and a printf-style `log_format(fmt, type, ...)`. Singleton: `extern LoggerInterface __i_logger;`. The per-device subclass typically just forwards each method to `Serial.print` — see [LoggerInterface.cpp](devices/esp32/LoggerInterface.cpp).

### 9.5 The flash-string convention

Every macro wraps the argument in `RODT_ATTR(...)`:

```cpp
#define LogI(v) __i_logger.log_info(RODT_ATTR(v))
```

That means the literal string lives in flash memory (`PROGMEM` on AVR, `.rodata` on ESP), and the logger reads through it as a flash pointer. On AVR/ESP8266 this is what keeps a verbose `LogI("...")` from chewing through RAM — a few hundred log call sites cost flash space they were going to occupy anyway, and zero RAM.

The implementation side has to honour this: `log_info(const char* s)` may be receiving a *flash pointer*, not a RAM pointer. The default device impl uses `Serial.print` which handles both transparently on Arduino cores. A custom port that writes through a non-Arduino API must call the flash-aware variant (`strlen_P`, `memcpy_P`, the `rofn::to_charptr` helper in [esp32_pdi.cpp](devices/esp32/esp32_pdi.cpp)).

### 9.6 Where logs go

The default sink is the device's serial port at 115200 baud — `LoggerInterface::init()` calls `Serial.begin(115200)`. There is **no** built-in routing to:

- Telnet / SSH sessions (those have their own terminal lanes via `iTerminalInterface`)
- The file system (no rotating log file)
- The web portal (the dashboard doesn't show log lines)
- A remote syslog / HTTP sink

If you need any of those, write a custom `LoggerInterface` for your port whose methods fan out to multiple sinks (Serial + your destination). The interface contract supports it; the framework just doesn't ship the wiring.

### 9.7 Typical usage in framework code

Three patterns recur across services:

```cpp
LogBegin (in PdiStack::initialize)              // boot the logger sink

LogI("Starting WiFi service");                  // simple level
LogE("Found invalid configs.. starting factory reset!");

LogFmtI("NTP Validity : %d\n", __i_ntp.is_valid_ntptime());   // formatted
LogFmtE("OTA failed for %s -> %d", url, status);
```

There is **no** convention for module tags (`"[wifi] starting"` etc.) — services prepend their name in-line if they want one. Keep messages short; long strings cost flash.

### 9.8 Enabling logs on a running build

To turn logs on:

1. Uncomment one or more `ENABLE_LOG_*` in [devices/DeviceConfig.h](devices/DeviceConfig.h):
   ```c
   #define ENABLE_LOG_INFO
   #define ENABLE_LOG_ERROR
   // or just: #define ENABLE_LOG_ALL
   ```
2. Recompile and flash. Open Serial Monitor at 115200.

There is no runtime toggle. To selectively log only one service, the practical option is to gate that service's calls behind a custom macro (`#define LogMqtt(v) LogI(v)` and only include it in MQTT files), then leave only `ENABLE_LOG_INFO` on. The framework doesn't ship per-tag filters.

### 9.9 Gotchas

- **`LOGBEGIN` must be called or `Serial` is never started.** [`PDIStack::initialize`](src/PdiStack.cpp) does this on your behalf — don't `Serial.begin()` again in your sketch's `setup()` or you'll race the logger.
- **`Log*("constant string")` works; `Log*(my_var)` may not.** `RODT_ATTR(v)` only does the right thing when `v` is a string literal. If you want to log a `const char*` variable, use `LogFmtI("%s", my_var)` — the format string is a literal, the variable is just a `%s` argument.
- **No level-filtering at runtime.** Disabling `ENABLE_LOG_INFO` removes every `LogI` from the binary; you cannot turn one back on without recompiling.
- **`SerialServiceProvider` and the logger share the same serial port by default.** On boards with only one UART, this is fine because both call `Serial.print`. On boards where you've remapped the CLI to a second UART, the logger still goes to `Serial` — override `LoggerInterface::log_*` to route there too if you want them unified.
- **No timestamping.** Lines arrive as-written, no prefix. If you want timestamps, prepend them with `LogFmtI("[%lu] ...", millis())` — there's no global setting.

---

## 10. Transports

Transports are the **protocol implementations** that sit between the byte-level `iClientInterface` (which the device port provides) and the feature services (which deal in domain payloads). The three transports — HTTP, MQTT, SMTP — share one design: take a `iClientInterface*`, speak the wire protocol on it, hand the parsed result back to a service. They have **no device knowledge**, **no scheduling of their own**, and **no global state** other than per-instance buffers.

Implementation: [src/transports/](src/transports/). No global instances — each consumer creates an instance with an `iClientInterface*` it owns.

### 10.1 Why transports are separate from services

Three reasons the framework keeps these layers split:

1. **Reuse.** Both `WebServer` and `HTTP_CLIENT` need HTTP parsing; both `MqttServiceProvider` and `DeviceIotServiceProvider` need MQTT. A service owns *what to do*; the transport owns *how to speak the protocol*.
2. **Replaceability.** A device port that ships an SDK-native MQTT client can offer a `iClientInterface`-shaped wrapper and the framework's `MQTTClient` falls away — services don't change.
3. **Testability off-device.** Transports are pure C++ that take a stream — they can be exercised against a memory-backed `iClientInterface` mock without WiFi.

The service layer never opens its own socket: it asks `iInstanceInterface::getNewTcpClientInstance()` for a fresh `iTcpClientInterface*`, hands it to the transport, and lets the transport drive.

### 10.2 Transport summary

| Transport | Path | Class | Consumer services | Wire spec |
|---|---|---|---|---|
| HTTP | [transports/http/](src/transports/http/) | `Http_Client` | `OtaServiceProvider`, `DeviceIotServiceProvider`, `GpioServiceProvider` (HTTP-post mode), `WebServer` indirectly | HTTP/1.1 (default), 1.0 supported; placeholders for 2/3. HTTPS works through the same class — see [§10.3 HTTPS](#https--same-client-different-socket) |
| MQTT | [transports/mqtt/](src/transports/mqtt/) | `MQTTClient` (+ `mqtt_msg_*` builders) | `MqttServiceProvider`, `DeviceIotServiceProvider` | MQTT 3.1.1 |
| SMTP | [transports/smtp/](src/transports/smtp/) | `SMTPClient` | `EmailServiceProvider` | SMTP with `AUTH LOGIN`, no STARTTLS yet |

### 10.3 HTTP — `Http_Client`

Header: [HTTPClient.h](src/transports/http/HTTPClient.h). Two POD records — `http_req_t` and `http_resp_t` — carry request/response metadata; the class itself is the state machine that drives a connection through `iClientInterface`.

#### Lifecycle

```cpp
Http_Client http;
http.Begin();                                  // resets internal state
http.SetClient(__i_instance.getNewTcpClientInstance());
http.SetTimeout(HTTP_REQUEST_DURATION);        // 10000 ms by default
http.SetKeepAlive(true);
http.SetFollowRedirects(true);
http.SetRedirectLimit(3);
http.SetHttpVersion(HTTP_VERSION_1_1);
http.SetDefaultHeaders(true);                  // adds Host, User-Agent, Connection
http.AddReqHeader("X-Device", mac);

int16_t code = http.Get("http://api.example.com/v1/ping");
// or: http.Post(url, jsonBody);
// or: http.SendRequest("PATCH", url, body, len);

char *body; int16_t len;
http.GetResponse(body, len);                   // body is borrowed; do not free
char *agent;
http.GetRespHeader("Server", agent);

http.End(/*preserve_client=*/true);            // close session, optionally keep socket for next request
```

The class exposes the usual `Get` / `Post` / `SendRequest`, request-shaping setters (URL, timeout, keep-alive, headers, basic-auth, user-agent, redirect policy, HTTP version), and matching `GetResponse` / `GetRespHeader` accessors. Requests are synchronous; a negative return signals a transport-level failure (connect/read/timeout) before an HTTP status is available. Full signatures in [HTTPClient.h](src/transports/http/HTTPClient.h).

#### Defaults (from [HttpConfig.h](src/config/HttpConfig.h))

| Macro | Value | Meaning |
|---|---|---|
| `HTTP_DEFAULT_PORT` | 80 | Used if URL omits port |
| `HTTPS_DEFAULT_PORT` | 443 | Used if URL omits port for `https://` |
| `HTTP_DEFAULT_VERSION` | `HTTP_VERSION_1_1` | Request line version |
| `HTTP_DEFAULT_KEEP_ALIVE_MS` | 30000 | Connection: keep-alive timeout sent to peer |
| `HTTP_REQUEST_DURATION` | 10000 (Common.h) | Per-request total budget |
| `HTTP_CLIENT_BUF_SIZE` | 640 | Working buffer used during parse |
| `HTTP_CLIENT_MAX_READ_MS` | 1500 | Per-chunk read timeout inside the parse loop |
| `HTTP_REQUEST_RETRY` | 1 | Connect retries before giving up |

#### Status codes

`http_resp_t::status_code` carries the standard HTTP code on success; on transport failure (connect/read/timeout) the `Get`/`Post`/`SendRequest` return is a negative `int16_t`. The full enum (`HTTP_RESP_*`) is in [HttpConfig.h](src/config/HttpConfig.h).

#### HTTPS — same client, different socket

`Http_Client` doesn't know or care whether its underlying `iClientInterface*` is plaintext TCP or TLS. To make any request HTTPS, only the factory call changes:

```cpp
Http_Client http;
http.Begin();

// HTTPS instead of HTTP:
iTlsClientInterface* tls = __i_instance.getNewTlsClientInstance();
tls->setCertificateAuthorityPath(TLS_DEFAULT_OUTBOUND_CA_BUNDLE_PATH);   // optional
tls->setSNIHostname("api.example.com");                                  // recommended
http.SetClient(tls);

int16_t code = http.Get("https://api.example.com/v1/ping");
```

What you get for free: full handshake, peer-cert verification (when configured), record-level read/write, the same `GetResponse` / `GetRespHeader` parsing. What you should know:

- **The URL scheme is informational** — `Http_Client` doesn't open the socket, the client does. Passing a TLS client and a `http://...` URL still works (and travels encrypted); passing a TCP client and `https://...` connects in plaintext on the URL's port. Pair them correctly.
- **Default port follows the URL** — `https://` uses `HTTPS_DEFAULT_PORT = 443` from [HttpConfig.h](src/config/HttpConfig.h), `http://` uses `HTTP_DEFAULT_PORT = 80`.
- **`setVerifyPeer(false)` on the client** keeps the channel encrypted but skips chain validation — fine for dev / self-signed; **never** for production paths that cross an untrusted network.
- **`HTTPS_HSTS_MAX_AGE_SECONDS`** (server-side, [HttpConfig.h](src/config/HttpConfig.h)) is `0` by default — the HTTPS server doesn't emit `Strict-Transport-Security` until you set it non-zero (only do so when serving a CA-signed cert, otherwise the header hard-fails self-signed click-through).

The bundled `m_client` in `PdiStack` is already TLS-allocated when `ENABLE_TLS_SERVICE` is on (see [src/PdiStack.cpp](src/PdiStack.cpp)), so OTA, IoT, GPIO-post, and any other service that borrows that pointer automatically run over HTTPS without sketch changes.

#### Server-side HTTP / HTTPS

There is no `Http_Server` class in `transports/` — the **server** side lives at the interface layer ([`iHttpServerInterface`](src/interface/pdi/middlewares/iServerInterface.h)) with a portable default in [src/interface/pdi/impl/middlewares/HttpServerInterfaceImpl.{h,cpp}](src/interface/pdi/impl/middlewares/HttpServerInterfaceImpl.h). This split is intentional: the client is reused stand-alone; the server is plumbed through the `WebServer` ([§8](#8-web-server)) and never speaks bare HTTP.

HTTPS uses the **same** `HttpServerInterfaceImpl` — `begin(port, secure=true)` flips it into TLS mode, where each accepted client is wrapped via `__i_instance.getNewTlsServerInstance()` (so the request parser still sees decrypted bytes through the same `iClientInterface` API). Wiring details — cert/key path defaults, mTLS, HSTS — live in [§6.2.12](#6-service-providers) and [§6.2.16](#6-service-providers).

### 10.4 MQTT — `MQTTClient` + `mqtt_msg_*`

Two files split by concern:

- [Mqtt_msg.{h,cpp}](src/transports/mqtt/Mqtt_msg.h) — pure protocol encoder/decoder. Free functions like `mqtt_msg_publish(conn, topic, data, len, qos, retain, &msgId)` produce wire-format `mqtt_message_t` records from a fixed-size buffer. No I/O.
- [MqttClient.{h,cpp}](src/transports/mqtt/MqttClient.h) — `MQTTClient` class, the connection state machine: handshake, keep-alive, subscribe/publish, QoS-1/2 acknowledgement, callback dispatch.

#### Lifecycle

```cpp
MQTTClient mqtt;
mqtt.begin(client, &generalCfg, &lwtCfg);     // wires client + config tables
mqtt.OnConnected(&onMqttConnected);
mqtt.OnData(&onMqttData);                     // (args, topic, topic_len, data, len)
mqtt.OnDisconnected(&onMqttDisconnected);
mqtt.OnTimeout(&onMqttTimeout);

mqtt.InitConnection(host, MQTT_DEFAULT_PORT, /*security=*/0);
mqtt.InitClient(clientId, user, pass, /*keepAlive=*/60, /*cleanSession=*/1);
mqtt.InitLWT(willTopic, willMsg, /*qos=*/1, /*retain=*/0);

mqtt.Connect();
mqtt.Subscribe("ctrl/+/cmd", /*qos=*/1);
mqtt.Publish("sensor/temp", payload, len, /*qos=*/1, /*retain=*/0);
// ... mqtt.Disconnect() / mqtt.DeleteClient() during shutdown
```

The class covers the usual MQTT surface: connection setup (`Init*`, `Connect` / `Disconnect`), `Publish` / `Subscribe` (QoS 0/1/2), Last-Will registration, and per-event `On*` callback slots (`OnConnected`, `OnData`, `OnSubscribed`, `OnTimeout`, …). `Subscribe` returns success on enqueue, not on SUBACK — wait for `OnSubscribed` before assuming the topic is live. Full signatures in [MqttClient.h](src/transports/mqtt/MqttClient.h).

#### Callbacks and threading

All callbacks fire on whatever lane drives `MqttServiceProvider::handleMqttPublish` / `handleMqttSubScribe`. By default that is the inline scheduler. Don't block in a callback — schedule any expensive work as a follow-up via `__task_scheduler.setTimeout(..., 1, now())`, exactly as the `DeviceIotServiceProvider` does it (see [DeviceIotServiceProvider.cpp grep](src/service_provider/iot/DeviceIotServiceProvider.cpp)).

#### Defaults

| Macro | Value |
|---|---|
| `MQTT_DEFAULT_PORT` | 1883 |
| `MQTT_DEFAULT_KEEPALIVE` | 60 s |

Service-level tables (`mqtt_general_config_table`, `mqtt_lwt_config_table`, `mqtt_pubsub_config_table`) live in [MqttConfig.h](src/config/MqttConfig.h); the `MqttServiceProvider` translates them into the `Init*` calls above.

#### Direct use of `mqtt_msg_*`

Useful when you need to encode a packet without owning a connection — e.g. SSH-tunnelled MQTT, or a unit test. `mqtt_msg_init(&conn, buf, size)` binds a buffer; subsequent `mqtt_msg_connect`, `mqtt_msg_publish`, `mqtt_msg_subscribe`, etc. return a `mqtt_message_t*` whose `data`/`length` you write to whatever stream you have. `mqtt_get_total_length` and `mqtt_get_id` parse incoming frames.

### 10.5 SMTP — `SMTPClient`

Implementation: [SMTPClient.{h,cpp}](src/transports/smtp/SMTPClient.h). A blocking, command-response client built on top of `iClientInterface`. Each `sendXxx` issues one SMTP verb and waits for the expected reply code (or arbitrary expected string).

#### Lifecycle

```cpp
SMTPClient smtp;
smtp.begin(client, host, port);

smtp.sendHello(domain);                    // EHLO/HELO
smtp.sendAuthLogin(username, password);    // AUTH LOGIN (base64 user, base64 pass)
smtp.sendFrom(sender);                     // MAIL FROM:<...>
smtp.sendTo(recipient);                    // RCPT TO:<...>
smtp.sendDataCommand();                    // DATA
smtp.sendDataHeader(sender, recipient, subject);
smtp.sendDataBody(body);                   // string overloads
smtp.sendQuit();                           // QUIT
smtp.end();
```

The class exposes one `sendXxx` helper per SMTP verb (`sendHello`, `sendAuthLogin`, `sendFrom`, `sendTo`, `sendDataCommand`, `sendDataHeader`, `sendDataBody`, `sendQuit`), plus low-level primitives (`readResponse`, `waitForExpectedResponse`, `sendCommandAndGetCode`) for callers that want to drive the protocol directly. `begin` only stores parameters; the connection opens on the first send. Full signatures in [SMTPClient.h](src/transports/smtp/SMTPClient.h).

#### Defaults & status

| Macro / type | Value |
|---|---|
| `SMTP_DEFAULT_TIMEOUT` | 5000 ms (`MILLISECOND_DURATION_5000`) |
| `enum smtp_reply_code` | Standard reply codes (220, 250, 334, 354, 235, 221, …) |
| `enum smtp_command_status` | Success/failure of each helper |

#### Limitations to know

- **No STARTTLS.** SMTP is plaintext on whatever port you give it; for TLS-only providers you need a port that supports TLS at the `iClientInterface` level.
- **No multi-recipient batching.** Loop `sendTo` per recipient between `MAIL FROM` and `DATA`.
- **Blocking.** All verbs synchronously block on the wire — call from a service tick that already accepts a multi-second tail latency. `EmailServiceProvider::sendMail` schedules this off the critical path.
- **No body encoding.** The transport doesn't quoted-printable / base64 the body. If you need attachments or non-ASCII, encode at the caller.

### 10.6 Helpers (`src/helpers/`)

Two small shims live alongside the transports to keep service code terse:

- [ClientHelper.{h,cpp}](src/helpers/ClientHelper.h) — `connectToServer`, `disconnect`, `isConnected`, `sendPacket`, `readPacket`. These work on any `iClientInterface*` and add timeout discipline + chunked write/read. Anyone using a transport at the byte level (e.g. SSH tunnel) reaches for these instead of the raw interface.
- [HttpHelper.h](src/helpers/HttpHelper.h) — `getHttpStatusString(code)`, `getMimeTypeString(mimetype_t)`, `getHttpMethodString(http_method_t)`, plus the static `HTTP_SERVER_DEFAULT_STATIC_PATH = "/var/www/static/"`. Used by `Http_Client`, `WebServer`, and the web controllers.
- [StorageHelper.h](src/helpers/StorageHelper.h) — `getMimeTypeString` / `getMimeTypeExtension` for filename ↔ MIME mapping; pulled in by the FS CLI and SFTP.

### 10.7 Adding a new transport

If you want to add (say) CoAP:

1. **Create [src/transports/coap/CoAPClient.{h,cpp}](src/transports/coap/CoAPClient.h)** that takes a `iClientInterface*` (UDP-shaped, so the port must expose UDP via the same interface) and exposes a `begin/send/receive` API plus callbacks for incoming responses.
2. **Keep it free of `__database_service` and `__task_scheduler`.** Transports do not own time or persistence — they parse bytes.
3. **Build the service on top** under `src/service_provider/transport/CoAPServiceProvider.{h,cpp}` — that's the layer that owns config, scheduling, and event dispatch.
4. **Wire one new flag** (`ENABLE_COAP_SERVICE`) in [DeviceConfig.h](devices/DeviceConfig.h), guard the orchestrator wiring in [PdiStack.h](src/PdiStack.h) / [PdiStack.cpp](src/PdiStack.cpp), and you're done.

### 10.8 Gotchas

- **Transports don't own the `iClientInterface*`.** Whoever called `getNewTcpClientInstance()` must `delete` it after `End`/`Disconnect`/`end`. Forgetting leaks one socket per request — fatal on long-uptime nodes.
- **No retry loops in transports.** `HTTP_REQUEST_RETRY = 1` covers connect; everything else is a single attempt. Retries belong to the service layer (which knows whether the failure is recoverable).
- **HTTP response body is a borrowed pointer.** `GetResponse(&body, &len)` returns an internal buffer; if you need it past the next request, copy.
- **MQTT `Subscribe` returns true on enqueue, not on SUBACK.** Wait for `OnSubscribed` (or skip waiting and accept best-effort) before publishing on the freshly-subscribed topic.
- **SMTP timeouts are per-verb.** A flaky link can sum up to `n × SMTP_DEFAULT_TIMEOUT` for one email; bound the service-side budget explicitly.
- **HTTP/2 and /3 enums exist; the implementation does not.** Setting `HTTP_VERSION_2` silently degrades to 1.1. Don't ship code that depends on multiplexed requests until the impl lands.

---

## 11. Examples Walkthrough

[examples/](examples/) ships two tracks: one **end-user** example that just brings the whole framework up, and one **`Dev/` tree** with task-focused snippets demonstrating each extension surface. They are minimum-viable code on purpose — copy, adapt, ship.

Available examples:

| Example | Path | Lines | Demonstrates |
|---|---|---|---|
| `PdiStack` | [examples/PdiStack/PdiStack.ino](examples/PdiStack/PdiStack.ino) | 13 | The smallest possible sketch — `initialize()` + `serve()` |
| `TaskScheduling` | [examples/Dev/TaskScheduling/TaskScheduling.ino](examples/Dev/TaskScheduling/TaskScheduling.ino) | 112 | All three task modes (inline / cooperative / preemptive) plus update + clear |
| `AddingDatabaseTable` | [examples/Dev/AddingDatabaseTable/AddingDatabaseTable.ino](examples/Dev/AddingDatabaseTable/AddingDatabaseTable.ino) | 123 | Ad-hoc app-side persistence without going through codegen |
| `AddingController` | [examples/Dev/AddingController/AddingController.ino](examples/Dev/AddingController/AddingController.ino) | 93 | Adding a custom web route with auth middleware |
| `MqttExample` | [examples/Dev/MqttExample/MqttExample.ino](examples/Dev/MqttExample/MqttExample.ino) | 120 | Programmatic MQTT config + publish / subscribe callbacks |
| `DeviceIotExample` | [examples/Dev/DeviceIotExample/](examples/Dev/DeviceIotExample/) | 184 | Implementing `iDeviceIotInterface` to feed the IoT pipeline |

### 11.1 `PdiStack` — the everything-on sketch

```cpp
#include <PdiStack.h>

void setup() {
    PdiStack.initialize();
}

void loop() {
    PdiStack.serve();
}
```

That's the whole file. Every service in [devices/DeviceConfig.h](devices/DeviceConfig.h) that is `ENABLE_*`'d brings itself up under [`PDIStack::initialize`](src/PdiStack.cpp), and `serve()` ticks them all on every loop iteration. The sketch you flash to a brand-new device.

What you should see after flash:

1. Serial @ 115200 prints the boot banner (`Starting PDIStack ! / Release / Config`).
2. WiFi AP `pdiStack` (password `pdiStack@123`) appears in the available networks.
3. Connect, browse to `http://192.168.0.1`, log in as `pdiStack` / `pdiStack@123`.
4. The dashboard, GPIO, MQTT, OTA, Email, Storage menus are all live.

**When to start here:** every other example assumes `initialize()` ran first. Use this to validate the build before adding code.

### 11.2 `TaskScheduling` — the scheduler in three modes

This example layers four behaviours on the bare stack:

- A one-shot `timeout_task` fired 1 s after boot via `__task_scheduler.setTimeout(...)`.
- A periodic `interval_task` started at 3 s cadence via `setInterval`, with the returned id captured in `variable_time_task_id`.
- A second one-shot at 15 s that calls `updateInterval` to **change the periodic task's cadence** to 1 s.
- A third one-shot at 30 s that calls `clearInterval` to remove it.

Inside `#ifdef ENABLE_CONTEXTUAL_EXECUTION` (esp8266 and esp32 both ship the threading port now), two more tasks demonstrate the contextual lanes — each is a `while(1) { … sleep(500); }` body promoted via `scheduleUnderExecSched` with a 1 KB per-task stack:

```cpp
int cooperative_task_id = __task_scheduler.register_task(cooperative_task);
__task_scheduler.scheduleUnderExecSched(
    &__i_cooperative_scheduler, cooperative_task_id,
    TASK_MODE_COOPERATIVE, 1 * 1024);
```

Maps directly to [§4. Task Scheduler](#4-task-scheduler) — particularly §4.4 (API by use case) and §4.8 (contextual scheduling).

**Disable `ENABLE_LOG_*`** before running this example, or the framework's own log output will interleave with the demo prints and obscure the schedule.

### 11.3 `AddingDatabaseTable` — app-side persistence without codegen

The framework's normal DB flow runs through `DBTableSchema.json` and the codegen ([§5.5](#5-database-layer)). This example shows the **escape hatch**: define a `DatabaseTable<addr, T>` subclass directly in your sketch and register it via the same static-init mechanism the framework uses for its own tables.

Key lines:

```cpp
#if defined(DEVICE_ARDUINOUNO)
#define STUDENT_TABLE_ADDRESS  800
#else
#define STUDENT_TABLE_ADDRESS  2500     // addresses ≤ 2499 reserved by framework
#endif

struct student_table { student_t students[MAX_STUDENTS]; int student_count; };

class StudentTable : public DatabaseTable<STUDENT_TABLE_ADDRESS, student_table> {};
StudentTable __student_table;          // static-init self-registers (see §5.3)
```

Then in `setup` after `PdiStack.initialize()`: build a `student_table` value, call `__student_table.set(&value)`. A scheduled task every 5 s reads back via `__student_table.get(&value)` and prints it.

Three things this example teaches well:

1. **Address picking.** The comment at lines 9-14 is the canonical guidance — pick `≥ 2500` on esp\* (`≥ 800` on UNO) to stay out of the framework's pre-allocated range from [DBTableSchema.json](devices/esp32/config/DBTableSchema.json).
2. **POD struct discipline.** `student_t` is a fixed-size `_name[20]` + `uint8_t` + `enum` — no `String`, no pointer, no `pdiutil::string`. Required by the raw-NVM serialiser ([§3.4](#3-configuration-system), [§5.7](#5-database-layer)).
3. **No JSON schema edit needed.** Because the table lives in the sketch, the codegen never sees it. Trade-off: it won't appear in `srvc` listings or any other reflective tooling that walks the generated tables.

### 11.4 `AddingController` — extending the web portal

A new route `/test-route` behind `AUTH_MIDDLEWARE`. The pattern from [§8.4](#8-web-server) and [§8.10](#8-web-server) made concrete:

```cpp
class TestController : public Controller {
public:
    TestController() : Controller("test") {}

    void boot() override {
        m_route_handler->register_route(
            "/test-route",
            [&]{ this->handleTestRoute(); },
            AUTH_MIDDLEWARE);
    }

    void handleTestRoute() {
        char* _page = new char[PAGE_HTML_MAX_SIZE];
        memset(_page, 0, PAGE_HTML_MAX_SIZE);
        strcat_ro(_page, WEB_SERVER_HEADER_HTML);
        strcat_ro(_page, WEB_SERVER_MENU_CARD_PAGE_WRAP_TOP);
        concat_svg_menu_card(_page, WEB_SERVER_HOME_MENU_TITLE_DASHBOARD,
                             SVG_ICON48_PATH_DASHBOARD, WEB_SERVER_DASHBOARD_ROUTE);
        strcat_ro(_page, WEB_SERVER_MENU_CARD_PAGE_WRAP_BOTTOM);
        strcat_ro(_page, WEB_SERVER_FOOTER_HTML);
        m_web_resource->m_server->send(HTTP_RESP_OK, MIME_TYPE_TEXT_HTML, _page);
        delete[] _page;
    }
};

TestController test_controller;        // static-init self-registers (see §8.4)
```

Things to copy:

- **Self-registration via the constructor** — no manual list to update.
- **`strcat_ro(...)` for flash-string concatenation** — the framework's flash-aware version of `strcat`, preserving the `RODT_ATTR` discipline of [§12.2](#122-flash-strings--rodt_attr-and-prog_rodt_attr).
- **`AUTH_MIDDLEWARE`** is the only line that gates the route; without it, the route is public.

Things to **not** copy verbatim:

- The 5 KB `new char[PAGE_HTML_MAX_SIZE]` allocation per request. Real controllers (see [§8.7](#8-web-server)) compose pages in chunked `send(..., true)` calls so the response streams to the client without an intermediate buffer. The example's allocator-based version is illustrative; chunked is the right pattern for production.
- The hard-coded comment "PAGE_HTML_MAX_SIZE defined in framework as 5000" is **stale** — it's currently 1800. See [§8.7](#8-web-server).

### 11.5 `MqttExample` — programmatic MQTT setup

This is the pattern for any sketch that wants MQTT configured **from code**, not from the web portal — useful for fleet provisioning. Three sequential steps in `configure_mqtt()`:

1. **Read** the three MQTT tables from NVM via `__database_service.get_mqtt_*_config_table(&local)`.
2. **Overwrite** the relevant fields with `memcpy(local.host, MQTT_HOST, …)` etc.
3. **Write** the tables back with `__database_service.set_mqtt_*_config_table(&local)`.
4. **Register callbacks** with `__mqtt_service.setMqttPublishDataCallback(...)` and `setMqttSubscribeDataCallback(...)`.
5. **Trigger the service to reload** via `__task_scheduler.setTimeout([&]{ __mqtt_service.handleMqttConfigChange(); }, 10, millis())`.

The 10 ms `setTimeout` is doing real work — it defers the reload off the `setup()` stack so any pending log output flushes first. The pattern recurs across services (WiFi, OTA, IoT).

The callback signatures match [§6.2.7](#6-service-providers):
- `void publish_callback(char* payload, uint16_t length)` — fills `payload` with the data to publish.
- `void subscribe_callback(uint32_t* args, const char* topic, uint32_t tlen, const char* data, uint32_t dlen)` — called per inbound message.

The `[mac]` token in `MQTT_CLIENT_ID`, topics, and the LWT message is replaced by the framework with the device's MAC at runtime — handy for fleet uniqueness without sketch-side templating.

### 11.6 `DeviceIotExample` — implementing `iDeviceIotInterface`

The IoT service is the only one whose contract requires the **application** to implement an interface ([§13.3.5](#13-portable-interfaces)). This example has the implementation split across two files:

- [DeviceIotSensor.h](examples/Dev/DeviceIotExample/DeviceIotSensor.h) — declares `class DeviceIotSensor : public iDeviceIotInterface` and the per-sensor state (`m_sensor_samples[]`, `m_sensor_sample_index`, `m_sensor_sample_value`).
- [DeviceIotSensor.cpp](examples/Dev/DeviceIotExample/DeviceIotSensor.cpp) — implements `init()`, `sampleHook()` (gather one sample), `dataHook(pdiutil::string& payload)` (build the JSON to publish), `resetSampleHook()` (clear the sample buffer).
- [DeviceIotExample.ino](examples/Dev/DeviceIotExample/DeviceIotExample.ino) — registers the sensor:
  ```cpp
  DeviceIotSensor sensor;
  void setup() {
      PdiStack.initialize();
      __device_iot_service.initDeviceIotSensor(&sensor);
  }
  ```

Once `__device_iot_service.initDeviceIotSensor(&sensor)` is called, the service drives the schedule itself: `sampleHook` runs at the configured sample rate, `dataHook` runs at the configured publish rate, and the payload is pushed to the configured MQTT channel ([§6.2.9](#6-service-providers)).

This is the **only** example with a sketch-side `.h` and `.cpp` rather than a single `.ino` — necessary because `iDeviceIotInterface` has a constructor + multiple virtual overrides, which Arduino's `.ino` preprocessor handles poorly for non-trivial classes.

### 11.7 Suggested example order for a new contributor

1. **`PdiStack`** — confirm the toolchain works end-to-end.
2. **`TaskScheduling`** — the scheduler is the most useful primitive once you start adding behaviour.
3. **`MqttExample`** — see the get-set-callback-trigger pattern in real code; reusable for WiFi/OTA/IoT.
4. **`AddingDatabaseTable`** — if your project has app-specific persistent state.
5. **`AddingController`** — if your project needs a custom web page.
6. **`DeviceIotExample`** — only if you're publishing telemetry through the IoT pipeline.

### 11.8 Gotchas

- **Examples assume `DeviceSetup.py` has already run.** Without it, every example fails to compile with mock-device misses ([§2.2](#22-installation-flow)).
- **Most examples `#error` if their dependency flag is off.** `AddingController` requires `ENABLE_HTTP_SERVER`; `MqttExample` requires `ENABLE_MQTT_SERVICE`; `DeviceIotExample` requires `ENABLE_DEVICE_IOT`. The default `DeviceConfig.h` has all three on, but if you've trimmed for memory, re-enable before running the matching example.
- **`AddingController` references a stale `PAGE_HTML_MAX_SIZE = 5000`.** Current value is 1800 ([§8.7](#8-web-server)). The example's response will fit, but the *comment* should not be quoted as authoritative.
- **`TaskScheduling`'s contextual section needs `ENABLE_CONTEXTUAL_EXECUTION`.** Off by default even on esp8266. Uncomment the matching block in [devices/DeviceConfig.h](devices/DeviceConfig.h).
- **`AddingDatabaseTable` writes to NVM on every boot.** Repeated flash + boot cycles wear the storage. Comment out the writes once you've verified the read path.

---

## 12. Memory & Performance Notes

This section is a cross-cutting reference of the **memory model** and **what each subsystem actually costs** in flash, RAM, and CPU. None of it introduces new mechanics — it collects the constraints that have shown up in pieces through earlier sections so you can size a build before you compile it.

### 12.1 The memory budget per target

Rough usable budget after the Arduino core, lwIP, and stdlib are linked. Treat these as orders of magnitude, not exact ceilings.

| Target | Flash | RAM | NVM (EEPROM-emulated) | Realistic ceiling |
|---|---|---|---|---|
| Arduino UNO (ATmega328P) | 32 KB | 2 KB | 1 KB | Serial + Storage(EEPROM) + GPIO_BASIC + CMD; **no network** |
| ESP8266 | 1 MB | ~50 KB free heap | ~4 KB | Default full build minus SSH (key sizes + crypto blow heap); contextual execution feasible |
| ESP32 | 4 MB | ~250 KB free heap | ~4 KB | Default full build comfortably; can spare a second contextual lane |

These are why [DeviceConfig.h](devices/DeviceConfig.h) gates entire feature groups on `DEVICE_ARDUINOUNO` and why `MAX_DB_TABLES` is 5 on UNO vs. 15 on esp\*.

### 12.2 Flash strings — `RODT_ATTR` and `PROG_RODT_ATTR`

Two macros, one purpose: keep string literals out of RAM.

| Macro | Defined where | Effect on devices with flash strings (esp*) | Effect on others (mock, UNO) |
|---|---|---|---|
| `RODT_ATTR("text")` | `devices/<board>/<board>_device_config.h` | Expands to `(const char*)F("text")` — wraps the literal in the Arduino `F()` macro so it stays in flash | Falls through to the default `#define RODT_ATTR(x) x` in [DataTypeDef.h](src/utility/DataTypeDef.h) — plain literal, identical behaviour |
| `PROG_RODT_ATTR` | same | Expands to `PROGMEM` (storage qualifier on the variable) | Empty — variable lives in normal `.rodata` |
| `PROG_RODT_PTR` | same | Expands to `PGM_P` — the right pointer type to read flash strings on AVR | Empty — `const char*` works directly |

**Rule:** every string literal that ships in the binary belongs in `RODT_ATTR(...)` (when used inline) or in a `static const char foo[] PROG_RODT_ATTR = "...";` (when held in a named variable). The framework's CLI prompts, terminal output, HTTP page fragments, log messages, and every `srvc` name follow this rule — it's why the binary is dense rather than RAM-hungry.

### 12.3 Reading a flash string back — `rofn::to_charptr`

Some consumers can't accept a flash pointer directly (e.g. a function that calls `strcpy` instead of `strcpy_P`). The framework exposes `rofn::to_charptr(const void* rostr)` ([DataTypeDef.h](src/utility/DataTypeDef.h); implementation per device, e.g. [esp32_pdi.cpp](devices/esp32/esp32_pdi.cpp)) that:

1. Reads the flash string into a fresh `new char[len+1]`.
2. Returns the RAM pointer.
3. **Caller owns the memory** — must `delete[]` it.

Use this sparingly. Every call allocates from the heap; it's a one-way escape hatch, not a default.

### 12.4 The expensive features, called out

A small number of choices dominate the budget:

- **SSH** is by far the heaviest enable. If you turn it on, you've effectively committed to esp32-class memory.
- **TLS** is comparable to SSH in flash and worse than NAPT in heap — and `ENABLE_TLS_SERVICE` **cannot coexist with `ENABLE_NAPT` on esp8266** because both want too much of the same fixed heap. Pick one. Inbound HTTPS (`ENABLE_HTTPS_SERVER`) and outbound TLS clients share the same per-port BearSSL/mbedTLS code so there's no extra cost for enabling both.
  - **Per-live-session footprint matters more than the flat enable cost.** Each accepted (or outbound) TLS client triggers, *for the lifetime of that one connection*: a dedicated worker task (one stack — `TLS_TASK_STACK_SIZE` ≈ 6.5 KB, allocated from heap on FreeRTOS), the TLS I/O record buffers, plus SSL workflow state (session keys, cipher contexts, certificate chain in RAM during validation, handshake scratch). On **esp8266 / BearSSL** this drops free heap by **10-15 KB per active session** — drastic on a ~30-40 KB working budget; `TLS_IBUF_SIZE + TLS_OBUF_SIZE` (~3 KB) dominates, BearSSL engine state adds another 3-5 KB. On **esp32 / mbedTLS** the drop is **~35-50 KB per active session** — most of it is mbedTLS's internal `IN_CONTENT_LEN` + `OUT_CONTENT_LEN` record buffers (default 16 KB each in IDF), plus ~10 KB of `ssl_context`/`config`/x509/pk/RNG state. Trim `MBEDTLS_SSL_IN/OUT_CONTENT_LEN` in `sdkconfig.defaults` to drop esp32 per-session below 20 KB if you control your peer's record sizes. **All of that memory is released as soon as the client closes** — the worker task exits (FreeRTOS reaps the stack via the idle task on esp32, so reclamation may lag the close by one main-loop yield), buffers are freed, session state is torn down. Plan capacity for the worst-case number of concurrent TLS clients, not the average; an idle build looks healthy because nothing is allocated.
- **NAPT** is invisible in flash but expensive in heap because lwIP allocates the translation table. Disable on builds that don't bridge the AP to the STA.
- **HTTP server controllers** each carry their own form-validation code; the 12 of them add up. If you don't need the portal, drop `ENABLE_HTTP_SERVER` even when `ENABLE_WIFI_SERVICE` stays on.
- **Contextual execution** is cheap in flash but the per-task stacks live in RAM for the lifetime of the task. Two cooperative tasks at 1 KB each is 2 KB of RAM you can't reclaim — and turning on `ENABLE_TLS_SERVICE` adds at least one 6.5 KB cooperative task on esp8266 to host BearSSL work off the cont_t stack.
- **`pdiutil::string` and `pdiutil::vector` allocate** — even though they hide the heap, every push/append can fragment over time. Reserve up-front (`m_tasks.reserve(MAX_SCHEDULABLE_TASKS)` in [TaskScheduler.cpp](src/utility/TaskScheduler.cpp)) when the size is known.

### 12.5 Heap discipline

The framework expects to run for **weeks to months** without rebooting. The patterns that keep it stable:

1. **Don't `new` after `setup()`.** The single `m_client = __i_instance.getNewT(cp|ls)ClientInstance()` in [PdiStack.cpp](src/PdiStack.cpp) is acceptable because it happens once at static-init (the TLS branch picks the right factory at compile time via `ENABLE_TLS_SERVICE`). Anything that allocates per request, per tick, or per event will fragment.
2. **Reserve container sizes up front.** Both `__task_scheduler.m_tasks` and `__database.m_database_tables` reserve at `init` time so push_backs don't reallocate.
3. **Reuse buffers.** Web controllers reuse the `WebResourceProvider`'s 1.8 KB scratch. Transports reuse caller-owned `iClientInterface*` instances. The MQTT parser uses a static `PROTO_PARSER` ring.
4. **Hold-then-free, never hold-forever.** `CommandBase::holdOptionValue` ([§7.4](#7-command-line--terminal)) allocates a fresh buffer for the option value, but `Clear()` frees it at the end of every command iteration.
5. **No std-exception machinery.** PdiSTL's exception support is stubbed where the toolchain allows; `throw`/`catch` outside crypto/`pdistl` corners costs more than it's worth on a 50 KB heap.
6. **Check the heap before allocating big.** For code paths that genuinely *can* be skipped under memory pressure (TLS handshake, large web response composition), prefer `pdiutil::safe_new<T>(args...)` / `safe_new_array<T>(n)` from [SafeAlloc.h](src/utility/SafeAlloc.h) — they refuse the allocation (returning `nullptr`) if the free heap would dip below `PDI_SAFE_ALLOC_HEAP_MARGIN` (default 2 KB), letting the caller bail cleanly instead of crashing inside a third-party lib.

When something fragments, the symptom is `register_task` returning `-1`, `new` returning `nullptr` inside a transport, or `pdiutil::vector` failing to grow. The framework doesn't `LogE` these by default — wire your own check on `__task_scheduler.m_tasks.size()` if you suspect a leak.

### 12.6 CPU and tick budget

The main loop is `PdiStack::serve()` ([§1.6](#1-architecture-overview)). Every iteration:

| Step | Typical cost |
|---|---|
| `__web_server.handle_clients()` | <1 ms idle, 5-50 ms during a request |
| `__task_scheduler.run()` | <1 ms idle; runs at most **one** inline task per iteration (the "run-one-then-break" pattern, [§4.5](#4-task-scheduler)) |
| `__i_dvc_ctrl.yield()` | Vendor SDK time slice — 1-3 ms typical |
| `__i_dvc_ctrl.handleEvents()` | <1 ms unless a service handler is slow |
| Contextual scheduler ticks (if enabled) | Depends on per-task work |

Loop frequency ranges from **hundreds of Hz idle** down to **~10 Hz during heavy HTTP/SSH activity**. Two implications:

- **Anything that needs sub-100 ms response should not live in an inline task.** Use the `contextual` lanes or accept the jitter.
- **WiFi yields are not free.** Reducing `WIFI_CONNECTIVITY_CHECK_DURATION` below 5 s starts to compete with serve-loop work.

### 12.7 Boot-time profile

```
power-on
 ├─ static-init: every __<x> global constructed     ~10-50 ms
 ├─ PDIStack ctor: m_client = __i_instance.getNewT(cp|ls)ClientInstance() ~1 ms
setup()
 ├─ __database_service.initService                  ~50-200 ms  (NVM read of every table)
 ├─ serial init, terminal greeting                  ~10 ms
 ├─ wifi init + STA scan                            ~500-3000 ms  (the dominant boot cost)
 ├─ http server, telnet, ssh accept loops           ~50 ms
 ├─ cmd_service prompt                              ~10 ms
loop() — steady state begins
```

Total to first prompt: ~1-4 seconds depending on whether STA connection happens immediately.

### 12.8 Performance-sensitive choices

- **`RODT_ATTR` everywhere.** Skipping it doesn't break anything but it does silently move 50-200 bytes of strings to RAM per file. Always wrap.
- **`pdiutil::string` vs. `char[]`.** Use `char[]` for NVM-shaped fixed-size data (the config structs); use `pdiutil::string` for transient runtime work where size isn't known. Mixing them in one struct defeats the NVM serialisation contract.
- **`int` vs. `int32_t` in config tables.** Always use the explicit-size type — `sizeof(int)` differs between AVR and ESP and that breaks NVM portability between ports of the same build (though the framework doesn't ship NVM images across ports today).
- **Avoid `printf` family.** Use [DataTypeConversions](src/utility/DataTypeConversions.h) — the framework saves 4-8 KB by not pulling in libc's formatter.
- **SFTP throughput cap.** SCP/SFTP is 0.2-1 KB/s (see [§7.9 SFTP / SCP file transfer](#79-sftp--scp-file-transfer)). Large file movement is not a use case; firmware-update should go through OTA, not SFTP.

### 12.9 Profiling on-device

Three handles for runtime visibility:

- **`ps`** lists every scheduler task with its rolling `%CPU`, run count, and last-run interval — your best signal that a service is hogging the loop ([§4.9](#4-task-scheduler)).
- **`srvc list`** enumerates every service with its state and tracked task count; **`srvc status <name>`** drills into one — covers DB validity, WiFi connectivity, MQTT connect state.
- **`iUtilityInterface::measure_lastfn_stack()`** ([§13.3.1](#13-portable-interfaces)) — optional; a port that implements it lets you wrap critical work with `__i_dvc_ctrl.measure_lastfn_stack()` calls to estimate per-fn stack high-water marks. esp* ports don't implement it today.

### 12.10 Gotchas

- **`pdiutil::vector::reserve` is a *hint*.** Pushing past the reservation reallocates and copies. Always reserve to the *worst case*, not the *typical case*.
- **`PAGE_HTML_MAX_SIZE = 1800`** is per-`send` chunk, not per-response. Compose pages in three calls ([§8.7](#8-web-server)) — one big string overflows silently.
- **`NAPT` table growth is unbounded** by default in lwIP. If your AP carries dozens of clients, the heap will climb. The framework has no per-client throttle.
- **History file writes happen per-keystroke** during `fwrite` ([§7.9](#7-command-line--terminal)). For long files, the flash wear is real — `fwrite` is for small config edits, not log capture.
- **`__task_scheduler` runs *one* inline task per `serve()` tick.** Twenty registered tasks at 100 ms cadence each can starve each other if your serve loop runs less than 10 times per second. Profile with `ps` (or `top` for a live view).

---

## 13. Portable Interfaces

The interface layer is the **contract** between the framework and any device. Every type is an abstract C++ class with pure-virtual methods, no state, and no platform headers — it depends only on standard types, the framework's own [pdistl](src/utility/pdistl/) primitives, and other interfaces in this layer. Each one is implemented by exactly one device-side class per build (selected at compile time) and surfaced as a single global `__i_*` symbol.

### 13.1 Layout

```
src/interface/pdi/
├── iDatabaseInterface.h       NVM-backed key/blob store
├── iLoggerInterface.h         Structured log sink
├── iDeviceIotInterface.h      Application hook for the IoT service
├── drivers/                   "Bare metal" surfaces
│   ├── iGpioInterface.h           Digital/analog/blink GPIO
│   └── iWdtInterface.h            Watchdog
├── middlewares/               Higher-level building blocks
│   ├── iDeviceControlInterface.h  Composite (gpio + wdt + util + upgrade + …)
│   ├── iClientInterface.h         Generic stream client + iTcpClientInterface
│   │                              + iTlsClientInterface (ENABLE_TLS_SERVICE)
│   ├── iServerInterface.h         iTcpServerInterface + iTlsServerInterface
│   │                              + iHttpServerInterface (with HTTPS hooks)
│   ├── iNtpInterface.h            Time sync
│   ├── iPingInterface.h           Reachability check
│   └── iUpgradeInterface.h        OTA primitive
├── modules/                   Stand-alone feature surfaces
│   ├── serial/iSerialInterface.h
│   ├── storage/iStorageInterface.h        Raw byte-addressable backing store
│   ├── storage/iFileSystemInterface.h     Files & directories on top of storage
│   └── wifi/iWiFiInterface.h
├── threading/                 Optional execution-context surfaces
│   ├── iContext.h, iMutex.h, iCondvar.h, iExecution.h
│   ├── cooperative/iCooperative.h
│   └── preemptive/iPreemptive.h
└── impl/                      Default implementations reusable across ports
    ├── middlewares/HttpServerInterfaceImpl.{h,cpp}
    └── modules/storage/FileSystemInterfaceImpl.{h,c,cpp}
```

In addition, [src/utility/](src/utility/) ships three foundational interfaces that the `pdi/` headers build on — they live with the utilities because they have no device dependency at all:

| Foundation interface | Path | Role |
|---|---|---|
| `iIOInterface` / `iTerminalInterface` | [src/utility/iIOInterface.h](src/utility/iIOInterface.h) | Byte/line I/O contract — base of every stream-like interface (serial, TCP client, terminal sessions) |
| `iUtilityInterface` | [src/utility/iUtilityInterface.h](src/utility/iUtilityInterface.h) | `wait`, `millis_now`, `micros_now` (64-bit µs — powers `ps` %CPU), `random_now` (HW RNG on esp; xorshift default), `yield`, `log`, optional stack measurement |
| `iInstanceInterface` | [src/utility/iInstanceInterface.h](src/utility/iInstanceInterface.h) | Factory: new TCP client/server, get utility/filesystem |

### 13.2 Naming and discovery conventions

- All interface types are prefixed with a lowercase `i` (`iWiFiInterface`, `iClientInterface`).
- Each interface header forward-declares the **concrete** derived class with the canonical name (`class DeviceControlInterface;`) and at the bottom declares the singleton:
  ```cpp
  extern DeviceControlInterface __i_dvc_ctrl;
  ```
  This is what couples a port to the framework's expected symbol — the device's `.cpp` defines the matching variable.
- Composite interfaces use multiple inheritance to bundle smaller contracts; see [`iDeviceControlInterface`](src/interface/pdi/middlewares/iDeviceControlInterface.h) which inherits from `iGpioInterface`, `iWdtInterface`, `iUtilityInterface`, and `iUpgradeInterface`.
- Stream-like things (serial, TCP, terminal sessions) all share `iTerminalInterface` as a base, so the terminal, logger, and CLI can write to any of them uniformly.

### 13.3 Interface reference

Each row below: what the interface models, who implements it on a typical port, who consumes it from above.

#### 13.3.1 Core (always required)

| Interface | Path | Implementer | Primary consumers | Key methods |
|---|---|---|---|---|
| `iDeviceControlInterface` | [middlewares/iDeviceControlInterface.h](src/interface/pdi/middlewares/iDeviceControlInterface.h) | Device | `PDIStack`, every service via `__i_dvc_ctrl` | `initDeviceSpecificFeatures`, `resetDevice`, `restartDevice`, `eraseConfig`, `getDeviceId`, `getDeviceMac`, `isDeviceFactoryRequested`, `getTerminal`, `handleEvents` (+ inherited GPIO/WDT/utility/upgrade) |
| `iDatabaseInterface` | [iDatabaseInterface.h](src/interface/pdi/iDatabaseInterface.h) | Device | `DatabaseServiceProvider`, every config table | `beginConfigs(size)`, `cleanAllConfigs`, `isValidConfigs`, `getMaxDBSize`, plus templated typed read/write |
| `iInstanceInterface` | [src/utility/iInstanceInterface.h](src/utility/iInstanceInterface.h) | Device | Services that need fresh TCP/TLS/FS instances (MQTT pool, SSH, OTA, HTTPS) | `getNewTcpClientInstance`, `getNewTcpServerInstance`, `getNewTlsClientInstance` / `getNewTlsServerInstance` (`ENABLE_TLS_SERVICE`), `getFileSystemInstance`, `getUtilityInstance` |
| `iUtilityInterface` | [src/utility/iUtilityInterface.h](src/utility/iUtilityInterface.h) | Inherited via `iDeviceControlInterface` | Scheduler, event bus, logger, `DevFs` (`/dev/random`) | `wait`, `millis_now`, `micros_now`, `random_now`, `yield`, `log`, optional `can_measure_stack` / `measure_lastfn_stack` |
| `iIOInterface`, `iTerminalInterface` | [src/utility/iIOInterface.h](src/utility/iIOInterface.h) | Any stream (serial, TCP, etc.) | Logger, CLI, web body writers | `write`/`writeln` family (overloaded for all primitive types + `RODT_ATTR` strings), `with_timestamp`, `connect`/`disconnect` |

#### 13.3.2 Drivers

| Interface | Path | Implementer | Consumers | Key methods |
|---|---|---|---|---|
| `iGpioInterface` | [drivers/iGpioInterface.h](src/interface/pdi/drivers/iGpioInterface.h) | Device (via `DeviceControlInterface`) | `GpioServiceProvider`, `SysFs` (`/sys/class/gpio`) | `gpioMode`, `gpioWrite`, `gpioRead`, `gpioFromPinMap`, `isExceptionalGpio`, blink instance create/release |
| `iGpioBlinkerInterface` | same file | Device | GPIO service for blink mode | `setConfig`, `updateConfig`, `start`, `stop`, `isRunning` |
| `iWdtInterface` | [drivers/iWdtInterface.h](src/interface/pdi/drivers/iWdtInterface.h) | Device (folded into `DeviceControlInterface`) | Long-running services, scheduler | `enableWdt(mode)`, `disableWdt`, `feedWdt` |

#### 13.3.3 Middlewares (networking & device-level operations)

| Interface | Path | Implementer | Consumers | Notes |
|---|---|---|---|---|
| `iClientInterface` | [middlewares/iClientInterface.h](src/interface/pdi/middlewares/iClientInterface.h) | Serial, TCP, telnet/SSH sessions | Terminal sessions, transports | Adds only `setTimeout` on top of `iTerminalInterface` — the I/O surface |
| `iTcpClientInterface` | same file | Device | OTA, MQTT, SMTP, HTTP client | Adds `getLocalIp`, `getRemoteIp`, ports, `setKeepAlive`, `setNoDelay` |
| `iTlsClientInterface` | same file | Device (esp8266 via BearSSL, esp32 via mbedTLS) | Same consumers as TCP when `ENABLE_TLS_SERVICE` is on — orchestrator allocates this instead of plain TCP, see [src/PdiStack.cpp](src/PdiStack.cpp) | Extends `iTcpClientInterface`: `setCertificateAuthorityPath`, `setClientCertificatePath`, `setClientPrivateKeyPath`, `setSNIHostname`, `setVerifyPeer`, `isSecure()` |
| `iTcpServerInterface` | [middlewares/iServerInterface.h](src/interface/pdi/middlewares/iServerInterface.h) | Device | Telnet, SSH, raw TCP services | `begin(port)`, `hasClient`, `accept`, `close` |
| `iTlsServerInterface` | same file | Device | HTTPS server, future TLS-wrapped Telnet/MQTT brokers | Extends `iTcpServerInterface`: `setServerCertificatePath`, `setServerPrivateKeyPath`, `setClientCertificateAuthorityPath` (mTLS). `accept()` returns a TLS-capable client transparently |
| `iHttpServerInterface` | same file | Device (or `impl/HttpServerInterfaceImpl`) | `WebServer` | Routing (`on`, `onNotFound`), args/headers, `send(code, mime, body, chunked)`. With TLS: `begin(port, secure=true)` plus `setServerCertificatePath` / `setServerPrivateKeyPath` / `setClientCertificateAuthorityPath` |
| `iNtpInterface` | [middlewares/iNtpInterface.h](src/interface/pdi/middlewares/iNtpInterface.h) | Device | Logger timestamps, IoT, sessions | `init_ntp_time`, `is_valid_ntptime`, `get_ntp_time` |
| `iPingInterface` | [middlewares/iPingInterface.h](src/interface/pdi/middlewares/iPingInterface.h) | Device | WiFi service (internet-availability check) | `init_ping(wifi)`, `ping`, `isHostRespondingToPing` |
| `iUpgradeInterface` | [middlewares/iUpgradeInterface.h](src/interface/pdi/middlewares/iUpgradeInterface.h) | Device (folded into `DeviceControlInterface`) | `OtaServiceProvider` | `Upgrade(path, version) → upgrade_status_t` |

#### 13.3.4 Modules

| Interface | Path | Implementer | Consumers | Notes |
|---|---|---|---|---|
| `iSerialInterface` | [modules/serial/iSerialInterface.h](src/interface/pdi/modules/serial/iSerialInterface.h) | Device | `SerialServiceProvider`, logger, CLI | Derives from `iClientInterface`; serial is just another stream |
| `iStorageInterface` | [modules/storage/iStorageInterface.h](src/interface/pdi/modules/storage/iStorageInterface.h) | Device (EEPROM / flash / SD adapter) | `iFileSystemInterface`, `littlefs` | Byte-addressable: `read`, `write`, `erase`, `size` |
| `iFileSystemInterface` | [modules/storage/iFileSystemInterface.h](src/interface/pdi/modules/storage/iFileSystemInterface.h) | Device (often via [`FileSystemInterfaceImpl`](src/interface/pdi/impl/modules/storage/FileSystemInterfaceImpl.h)) | SSH/SFTP + file-oriented CLI commands | 42 virtuals: file/dir CRUD, traversal, line/offset lookup, search (`findInFile`, `getLineNumbersInFile`, offset⇄line), custom file attributes. Constructed with an `iStorageInterface&` |
| `iWiFiInterface` | [modules/wifi/iWiFiInterface.h](src/interface/pdi/modules/wifi/iWiFiInterface.h) | Device (esp8266/esp32) | `WiFiServiceProvider`, `net` CLI | STA + AP, scan (sync/async), `enableNAPT`, `setMode`/`getMode` |

#### 13.3.5 Optional helpers

| Interface | Path | Implementer | Consumers | Notes |
|---|---|---|---|---|
| `iLoggerInterface` | [iLoggerInterface.h](src/interface/pdi/iLoggerInterface.h) | Device | `LOG*` macros | `init`, `log(type, msg)`, type-specific helpers, `log_format(fmt, type, …)` |
| `iDeviceIotInterface` | [iDeviceIotInterface.h](src/interface/pdi/iDeviceIotInterface.h) | **Application** (not device) | `DeviceIotServiceProvider` | `sampleHook`, `dataHook(payload)`, `resetSampleHook` — implemented in the user's sketch to feed IoT payloads |

> Note: `iDeviceIotInterface` is the only `i*Interface` whose implementer is the *application*, not the device port. It is the framework's intentional extension point for "what to publish, on what schedule".

#### 13.3.6 Threading (only required for contextual execution)

| Interface | Path | Implementer | Consumers | Notes |
|---|---|---|---|---|
| `iContext` | [threading/iContext.h](src/interface/pdi/threading/iContext.h) | Device (CPU-specific) | Cooperative + preemptive schedulers | `save(out)`, `restore(in)` — raw register save/restore |
| `iMutex` | [threading/iMutex.h](src/interface/pdi/threading/iMutex.h) | Device | Anything sharing state across contexts | `lock`/`unlock` + `critical_lock`/`critical_unlock` (IRQ-safe) |
| `iConditionVar` | [threading/iCondvar.h](src/interface/pdi/threading/iCondvar.h) | Device | Cooperative scheduler primitives | `wait(mtx)`, `notify_one`, `notify_all` |
| `iExecutionContext` | [threading/iExecution.h](src/interface/pdi/threading/iExecution.h) | Device | Schedulers | `start`, `suspend`, `resume` |
| `iExecutive` | same file | Device | Tasks | Carries stack pointer/size + entry/arg + back-link to `task_t` |
| `iExecutionScheduler` | same file | Device | `TaskScheduler` | `schedule_task`, `mute`, `yield`, `sleep`, `run`, optional `enable_sched`/`disable_sched` and cross-scheduler hand-off |
| `iCooperative`, `iCooperativeScheduler` | [threading/cooperative/iCooperative.h](src/interface/pdi/threading/cooperative/iCooperative.h) | Device | Cooperative-mode tasks | Specialisation tags |
| `iPreemptive`, `iPreemptiveScheduler` | [threading/preemptive/iPreemptive.h](src/interface/pdi/threading/preemptive/iPreemptive.h) | Device | Preemptive-mode tasks | Specialisation tags |

The `SoftIrq` machinery in [iExecution.h](src/interface/pdi/threading/iExecution.h) lives in the interface header because it is the shared protocol between any ISR (device side) and the scheduler (utility side): the ISR sets a bit via `raise_softirq()`, the main context drains it with `fetch_softirq_bits()`. Ports that need preemption must wire their tick ISR to this protocol.

### 13.4 The `impl/` directory — shared default implementations

Not every interface is best re-written per device. [src/interface/pdi/impl/](src/interface/pdi/impl/) holds **portable default implementations** that a device can adopt as-is:

| Default impl | What it gives you | When a port should override |
|---|---|---|
| [`HttpServerInterfaceImpl`](src/interface/pdi/impl/middlewares/HttpServerInterfaceImpl.h) | A protocol-correct HTTP/1.1 server built on top of `iTcpServerInterface` + `iTcpClientInterface`. When `ENABLE_TLS_SERVICE` is on, `begin(port, secure=true)` wraps accepted connections via `__i_instance.getNewTlsServerInstance()` so the same impl serves HTTPS without a separate file | Almost never — only if the SDK provides a measurably better native HTTP server |
| [`FileSystemInterfaceImpl`](src/interface/pdi/impl/modules/storage/FileSystemInterfaceImpl.h) | LittleFS-based filesystem on top of any `iStorageInterface` | Only if the SDK exposes its own FS more efficiently |

This is how the framework keeps the porting effort small: a new device only needs raw `iTcpServerInterface` + `iStorageInterface`, and the rest of the stack (HTTP server, HTTPS server, file system) is inherited. The TLS client/server classes are **not** in `impl/` because BearSSL vs. mbedTLS are fundamentally different libraries — each port supplies its own pair from scratch.

### 13.5 Lifecycle and threading expectations

Every interface implementation must respect three contracts:

1. **Construction is cheap and side-effect-free.** Singletons are constructed before `setup()` — they must not allocate from the heap, touch hardware, or open ports in their constructor. All real work belongs in an `init*` method invoked from `PDIStack::initialize()` (or from another service's `initService`).
2. **Methods are non-blocking unless explicitly otherwise.** Anything that could take more than a few milliseconds (TCP connect, NTP fetch, OTA download) must return early or expose a state-machine; long-form work is driven by the scheduler, not held inside the interface call.
3. **No re-entrancy assumed.** The framework runs single-threaded inline by default. If the port enables contextual execution, the *implementation* (not the caller) is responsible for guarding shared state with `iMutex` — interfaces document where this matters per-method.

### 13.6 Adding a new interface

The bar for adding an interface is "is there at least one device that can implement it differently?". If yes:

1. Decide the group: `drivers/` (silicon-level), `middlewares/` (network/device), `modules/` (orthogonal feature), `threading/` (execution), or top-level (cross-cutting like database/logger).
2. Forward-declare the concrete class, declare `extern Concrete __i_<name>;` at the bottom.
3. Add a guard so no existing port has to provide it until it opts in — usually via the same `ENABLE_*` flag that gates the consuming service.
4. Provide a stub in [devices/mockdevice/](devices/mockdevice/) so the off-device build still links.
5. Document required vs optional methods in this section.

A new interface that ships with only one device implementation is a smell — collapse it into the device-specific code until a second port appears.

---

## 14. Device Layer & Porting Guide

The device layer is the **only** place where vendor SDK / Arduino-core symbols are allowed. Everything above it talks to the device through abstract `i*Interface` pointers defined in [src/interface/pdi/](src/interface/pdi/). A port is the work of providing one folder under [devices/](devices/) that implements those interfaces for a specific MCU family.

### 14.1 Anatomy of a device folder

A complete port looks like this (using esp32 as the canonical example):

```
devices/esp32/
├── esp32.h                       SDK / Arduino-core umbrella include
├── esp32_device_config.h         Per-port platform macros (RODT_ATTR / PROG_RODT_ATTR / strcat_ro
│                                 / strcpy_ro family, CRITICAL_SECTION_ENTER/EXIT, etc.)
├── esp32_pdi.h                   Header aggregator — included by src/interface/pdi.h
├── esp32_pdi.cpp                 Source aggregator — see §14.3
├── esp32_pdi.c                   (Optional) C-side aggregator for pure-C sources
├── DeviceControlInterface.{h,cpp}   Required — implements iDeviceControlInterface
├── DatabaseInterface.{h,cpp}        Required — implements iDatabaseInterface
├── LoggerInterface.{h,cpp}          Optional — only if logging is enabled
├── InstanceInterface.{h,cpp}        Required — factory for runtime instances
├── SerialInterface.{h,cpp}          If ENABLE_SERIAL_SERVICE
├── StorageInterface.{h,cpp}         If ENABLE_STORAGE_SERVICE
├── FileSystemInterface.{h,cpp}      If ENABLE_STORAGE_SERVICE
├── WiFiInterface.{h,cpp}            If ENABLE_WIFI_SERVICE
├── HttpServerInterface.{h,cpp}      If ENABLE_WIFI_SERVICE / HTTP_SERVER
├── TcpClientInterface.{h,cpp}       If ENABLE_NETWORK_SERVICE
├── TcpServerInterface.{h,cpp}       If ENABLE_NETWORK_SERVICE
├── NtpInterface.{h,cpp}             If ENABLE_NETWORK_SERVICE
├── PingInterface.{h,cpp}            If ENABLE_NETWORK_SERVICE
├── TlsClientInterface.{h,cpp}       If ENABLE_TLS_SERVICE  (esp8266 → BearSSL, esp32 → mbedTLS)
├── TlsServerInterface.{h,cpp}       If ENABLE_TLS_SERVICE
├── BearSSLCertLoader.{h,cpp}        esp8266 only: PEM/DER load helpers used by TlsClient/Server
│                                    (TlsCryptoLoader namespace)
├── MbedTLSCertLoader.{h,cpp}        esp32 only: same role, against mbedTLS
├── TlsCertProvisioner.{h,cpp}       esp32 only — if ENABLE_TLS_CERT_GENERATION
│                                    (self-signed EC/RSA cert issuance using mbedTLS)
├── ExceptionsNotifier.{h,cpp}       Optional — crash / exception capture
├── config/
│   └── DBTableSchema.json        Per-device DB schema; consumed by scripts
├── core/
│   └── EEPROM.{h,cpp}            Vendor-specific helpers (e.g. EEPROM emulator)
└── threading/                    Optional — only if the port provides cooperative
    │                              and/or preemptive execution contexts
    ├── Cooperative.{h,cpp}          iCooperativeScheduler implementation
    ├── CooperativeCondvar.{h,cpp}   iConditionVar pair for the cooperative lane
    ├── Preemptive.{h,cpp}           iPreemptiveScheduler implementation
    ├── PreemptiveMutex.{h,cpp}      iMutex implementation (IRQ-safe critical_lock pair)
    ├── XtensaContext.{h,cpp}        (esp8266) Xtensa register save/restore — iContext
    └── XtensaTimer.{h,cpp}          (esp8266) Hardware-timer ISR driving preemption
```

esp32 currently uses FreeRTOS task / mutex / semaphore primitives under the hood for its threading port (no `XtensaContext`/`XtensaTimer` files needed), while esp8266 ships the bare-metal Xtensa context save/restore + hardware-timer drive shown above.

Compare with the minimal end of the spectrum:

- [devices/mockdevice/](devices/mockdevice/) — header-only stub used when no `DEVICE_*` is defined; lets the framework compile for static analysis or off-device unit tests.
- [devices/arduinouno/](devices/arduinouno/) — no WiFi/Network/HTTP/Storage; ships only `DeviceControl`, `Database`, `Serial`, `Storage` (EEPROM-only), `FileSystem`, `Instance`, `Logger`.

### 14.2 Which interfaces are required vs optional

| Interface | Required? | Trigger flag | Notes |
|---|---|---|---|
| `iDeviceControlInterface` | **Always** | — | GPIO, reset, WDT, yield, events, terminal accessor |
| `iDatabaseInterface` | **Always** | — | NVM read/write for the config DB |
| `iInstanceInterface` | **Always** | — | Factory for fresh TCP client/server and FS handles |
| `iLoggerInterface` | Conditional | `ENABLE_LOG_*` | Skipped if no log level is enabled |
| `iSerialInterface` | Conditional | `ENABLE_SERIAL_SERVICE` | Powers the serial terminal |
| `iStorageInterface` + `iFileSystemInterface` | Conditional | `ENABLE_STORAGE_SERVICE` | LittleFS / SPIFFS / SD adapter |
| `iWiFiInterface`, `iHttpServerInterface` | Conditional | `ENABLE_WIFI_SERVICE` | WiFi STA+AP control, embedded HTTP server |
| `iTcpClientInterface`, `iTcpServerInterface` | Conditional | `ENABLE_NETWORK_SERVICE` | Raw TCP for MQTT/SMTP/OTA |
| `iNtpInterface`, `iPingInterface` | Conditional | `ENABLE_NETWORK_SERVICE` | Time sync, ICMP-based reachability |
| `iTlsClientInterface`, `iTlsServerInterface` | Conditional | `ENABLE_TLS_SERVICE` | TLS-wrapped TCP. Backend per port: BearSSL (esp8266), mbedTLS (esp32). When on, the orchestrator allocates these instead of plain TCP for `m_client`, so HTTP/MQTT/SMTP/OTA/IoT all upgrade to TLS transparently |
| `TlsCertProvisioner` (free functions, not a virtual interface) | Optional | `ENABLE_TLS_CERT_GENERATION` (esp32 only) | On-device self-signed cert/key issuance for the HTTPS server |
| `iExecution`, `iMutex`, `iCondvar`, `iContext` | Optional | `ENABLE_CONTEXTUAL_EXECUTION` | Needed for cooperative/preemptive task modes. Implicitly required by `ENABLE_TLS_SERVICE` so BearSSL/mbedTLS can run off the main loop's stack |
| `iGpioInterface`, `iWdtInterface` | Folded into `iDeviceControlInterface` | — | Implemented as part of `DeviceControl`, not separate classes today |

A port is "valid" the moment the **always** rows compile and link; every other row can be added incrementally as you turn on more `ENABLE_*` flags in [devices/DeviceConfig.h](devices/DeviceConfig.h).

### 14.3 The two aggregator files

Every port provides a pair of aggregators with a strict role split:

- **`<name>_pdi.h`** — declares which interface *headers* are visible to the rest of the framework. Each include is wrapped in the matching `ENABLE_*` guard so unused interfaces cost zero. See [devices/esp32/esp32_pdi.h](devices/esp32/esp32_pdi.h).
- **`<name>_pdi.cpp`** — `#include`s the implementation `.cpp` files (this is intentional, driven by the Arduino build layout). Every device translation unit ends up flattened into a **single object file** per build, so anything you mark `static` inside `devices/<board>/*.cpp` is per-port, not per-file — and those `.cpp` files must not be `#include`d from anywhere outside this chain.

Optionally:

- **`<name>_pdi.c`** — the same trick for C-only translation units.
- **`<name>.h`** — a small umbrella header pulling in the Arduino core / vendor SDK headers; included by every per-interface header so they don't each repeat the SDK plumbing.

### 14.4 Device selection flow

```
scripts/DeviceSetup.py -d esp8266
        │
        │  writes
        ▼
devices/DeviceSetup.h               #define DEVICE_ESP8266
        │
        │  #include'd by
        ▼
devices/DeviceConfig.h              cascades into ENABLE_* feature flags;
        │                           ALSO pulls in devices/<board>/<board>_device_config.h
        │                           via #if defined(DEVICE_*) → so RODT_ATTR, PROG_RODT_ATTR,
        │                           strcat_ro family, CRITICAL_SECTION_ENTER/EXIT
        │                           are defined before any framework header sees them
        │
        │  pulled in transitively from
        ▼
src/config/Config.h                 (everything in src/* sees the flags + platform macros)
        │
        ▼
src/interface/pdi.h                 selects <name>_pdi.h via #if defined(DEVICE_*)
        │
        ▼
devices/esp8266/esp8266_pdi.h       brings in the per-device interface headers
                                    (which transitively include esp8266.h for SDK symbols)
```

**Three** files need a matching entry when adding a device:
1. [devices/DeviceConfig.h](devices/DeviceConfig.h) — the `#if defined(DEVICE_*)` cascade at the top (to include `<board>_device_config.h`) **and** any per-device limits (`MAX_DIGITAL_GPIO_PINS`, `MAX_DB_TABLES`, etc.).
2. [src/interface/pdi.h](src/interface/pdi.h) — the `#if defined(DEVICE_*)` cascade that picks `<board>_pdi.h`.
3. [library.properties](library.properties) — add the architecture to the `architectures=` list.

### 14.5 Per-device global singletons

Each port instantiates exactly one object per interface and names it according to the framework's convention. The rest of the framework refers to these names directly — they are part of the contract.

| Symbol | Defined in (esp32 example) | Required when |
|---|---|---|
| `__i_dvc_ctrl` | [DeviceControlInterface.cpp](devices/esp32/DeviceControlInterface.cpp) | Always |
| `__i_db` | [DatabaseInterface.cpp](devices/esp32/DatabaseInterface.cpp) | Always |
| `__i_instance` | [InstanceInterface.cpp](devices/esp32/InstanceInterface.cpp) | Always |
| `__i_logger` | [LoggerInterface.cpp](devices/esp32/LoggerInterface.cpp) | Any `ENABLE_LOG_*` |
| `__i_serial` | [SerialInterface.cpp](devices/esp32/SerialInterface.cpp) | `ENABLE_SERIAL_SERVICE` |
| `__i_storage`, `__i_fs` | [StorageInterface.cpp](devices/esp32/StorageInterface.cpp), [FileSystemInterface.cpp](devices/esp32/FileSystemInterface.cpp) | `ENABLE_STORAGE_SERVICE` |
| `__i_wifi`, `__i_http_server` | [WiFiInterface.cpp](devices/esp32/WiFiInterface.cpp), [HttpServerInterface.cpp](devices/esp32/HttpServerInterface.cpp) | `ENABLE_WIFI_SERVICE` |
| `__i_ntp`, `__i_ping` | [NtpInterface.cpp](devices/esp32/NtpInterface.cpp), [PingInterface.cpp](devices/esp32/PingInterface.cpp) | `ENABLE_NETWORK_SERVICE` |
| `__i_cooperative_scheduler`, `__i_preemptive_scheduler` | [devices/esp8266/threading/](devices/esp8266/threading/), [devices/esp32/threading/](devices/esp32/threading/) | `ENABLE_CONTEXTUAL_EXECUTION` |

If your port skips a flag, also skip the symbol — it must not exist when the corresponding `ENABLE_*` is off.

### 14.6 The optional threading port

Cooperative / preemptive task modes require five additional pieces, implemented under [devices/esp8266/threading/](devices/esp8266/threading/) and [devices/esp32/threading/](devices/esp32/threading/):

- An `iExecution`-derived scheduler (one for cooperative, one for preemptive).
- An `iContext` implementation that saves/restores CPU state (e.g. `XtensaContext` on esp8266; FreeRTOS task primitives on esp32).
- An `iMutex` and `iCondvar` pair appropriate for the mode.
- A periodic tick source for the preemptive lane (e.g. `XtensaTimer` on esp8266; FreeRTOS scheduler tick on esp32).

If a port omits this layer, contextual tasks are simply unavailable — inline tasks continue to work because they share the `loop()` stack. Section [5. Task Scheduler](#4-task-scheduler) details the API and the trade-offs.

`ENABLE_TLS_SERVICE` implicitly turns this layer on — BearSSL (esp8266) and mbedTLS (esp32) handshakes overflow the default Arduino main stack, so the TLS work runs on a dedicated cooperative task sized by `TLS_TASK_STACK_SIZE`.

### 14.7 Per-device database schema

Each port carries its own [config/DBTableSchema.json](devices/esp32/config/DBTableSchema.json) describing the tables present in NVM for that board (capacity-tuned). `scripts/DeviceSetup.py` invokes `CreateDBSourceFromJson.py` against this file to emit C++ table sources under [src/database/tables/](src/database/tables/). See section [6. Database Layer] for the schema format.

### 14.8 Adding a new device — step by step

Suppose you are porting to a board called `myboard`.

1. **Create the folder and umbrella headers**
   ```
   devices/myboard/
   ├── myboard.h                  // pulls in Arduino.h / vendor SDK
   ├── myboard_device_config.h    // per-port platform macros: RODT_ATTR, PROG_RODT_ATTR,
   │                              // strcat_ro / strcpy_ro family, CRITICAL_SECTION_ENTER/EXIT
   └── config/DBTableSchema.json  // start from devices/esp32/config/DBTableSchema.json
   ```
   Wire `myboard_device_config.h` into the device-config cascade by adding a `#elif defined(DEVICE_MYBOARD)` branch in [devices/DeviceConfig.h](devices/DeviceConfig.h) alongside the existing esp8266/esp32/arduinouno includes.
2. **Implement the always-required interfaces**: `DeviceControlInterface`, `DatabaseInterface`, `InstanceInterface`. Each `.h` derives from the matching `i*Interface` in [src/interface/pdi/](src/interface/pdi/); each `.cpp` defines the global singleton with the canonical `__i_*` name.
3. **Write the two aggregators**
   - `myboard_pdi.h` — mirror [esp32_pdi.h](devices/esp32/esp32_pdi.h), keeping only the interfaces you've implemented and guarding each with the right `ENABLE_*`.
   - `myboard_pdi.cpp` — mirror [esp32_pdi.cpp](devices/esp32/esp32_pdi.cpp), `#include`ing the corresponding `.cpp` files.
4. **Register the device in the selector**
   ```cpp
   // src/interface/pdi.h
   #elif defined(DEVICE_MYBOARD)
   #include "../../devices/myboard/myboard_pdi.h"
   ```
5. **Add per-device limits** in [devices/DeviceConfig.h](devices/DeviceConfig.h) (`MAX_DIGITAL_GPIO_PINS`, `MAX_ANALOG_GPIO_PINS`, `MAX_DB_TABLES`, and gate any service flags the board cannot support, mirroring the `DEVICE_ARDUINOUNO` pattern).
6. **Run the setup script** from `scripts/`:
   ```bash
   python3 DeviceSetup.py -d myboard
   ```
   This writes `devices/DeviceSetup.h` with `#define DEVICE_MYBOARD` and generates the DB table sources.
7. **Build the bundled example** ([examples/PdiStack/](examples/PdiStack/)) for the new board to validate.
8. **Iterate on optional interfaces** (network, storage, WiFi, …), enabling the matching `ENABLE_*` in `DeviceConfig.h` one at a time and adding the corresponding `*Interface.{h,cpp}` files.
9. **(Optional) Add TLS support** by implementing `TlsClientInterface` / `TlsServerInterface` against your board's preferred SSL stack (e.g. mbedTLS via ESP-IDF, BearSSL on smaller boards) plus a cert-loader namespace (`TlsCryptoLoader`) for PEM/DER ingestion from the FS. Wire `__i_instance.getNewTlsClient/ServerInstance()` to allocate them. See [devices/esp8266/TlsClientInterface.h](devices/esp8266/TlsClientInterface.h) and [devices/esp32/TlsClientInterface.h](devices/esp32/TlsClientInterface.h) as references — both implement the same `iTlsClientInterface` contract on different SSL stacks.
10. **(Optional) Add the threading port** under `devices/myboard/threading/` if you want `ENABLE_CONTEXTUAL_EXECUTION` (or you've turned on TLS, which requires it). Provide `Cooperative` / `Preemptive` schedulers + matching `Mutex` / `Condvar`. The two existing ports — bare-metal Xtensa context on esp8266, FreeRTOS-backed on esp32 — show both ends of the implementation spectrum.

### 14.9 Validation checklist

A port is ready to merge when:

- [ ] `devices/mockdevice/` still compiles (you didn't accidentally couple `src/` to a vendor header).
- [ ] The bundled `examples/PdiStack` builds with all flags this board *can* support enabled.
- [ ] Every `__i_*` symbol expected by the build's `ENABLE_*` set is defined exactly once.
- [ ] `iUtilityInterface::micros_now()` returns monotonic 64-bit µs across the platform's native counter wrap. Reference impls: ESP8266 → `micros64()`; ESP32 → `esp_timer_get_time()`; AVR → wrap-tracking around 32-bit `micros()` (loop-context only). `ps` should show non-zero `%CPU` for any task with sub-ms callbacks after a few ticks.
- [ ] The `srvc list` CLI command lists every service your build started; `srvc stop <name>` freezes it (SIG_STOP), `srvc start <name>` resumes (SIG_CONT).
- [ ] `reboot` and factory-reset both round-trip without losing the DB.
- [ ] If `ENABLE_STORAGE_SERVICE` is on, SFTP upload/download via `scp` works.
- [ ] If `ENABLE_CONTEXTUAL_EXECUTION` is on, a sample task scheduled under `__i_cooperative_scheduler` and `__i_preemptive_scheduler` prints from both lanes without stack corruption.
- [ ] If `ENABLE_TLS_SERVICE` is on, `__i_instance.getNewTlsClientInstance()` returns a non-null instance and a manual TLS handshake against a known peer (e.g. `curl --resolve` to the device, or outbound to a public HTTPS endpoint) completes without panic.
- [ ] If `ENABLE_HTTPS_SERVER` is on, the portal answers on port 443 once `/etc/http/server.{crt,key}` are present.
- [ ] If `ENABLE_TLS_CERT_GENERATION` is on (esp32 only), `tls q=1,t=0,l=256,n=test,i=1` writes both files to the configured paths.

---

## 15. Utility Library

[src/utility/](src/utility/) is the framework's foundation: small, dependency-light primitives every layer above uses. Anything in this directory may include the abstract interfaces from [src/interface/pdi/](src/interface/pdi/) but **not** any device header — the same `.cpp` must compile under every port without conditional includes.

Most of these have appeared in passing in earlier sections. This section is the index — what's in the box, the public API, and where each piece is used.

### 15.1 Inventory

| Component | Path | Purpose |
|---|---|---|
| **Interface foundations** | `iIOInterface.h`, `iUtilityInterface.h`, `iInstanceInterface.h` | Three abstract bases the entire framework rests on |
| **Type definitions** | `DataTypeDef.h` | `task_t`, `task_state_t` (`READY`/`RUNNING`/`SLEEPING`/`STOPPED`/`ZOMBIE`), `signal_t` (`SIG_HUP`/`KILL`/`TERM`/`CONT`/`STOP`), `session_t`, `serial_event_t`, `CallBack*Fn` aliases, `cmd_term_inseq_t`, `ipaddress_t`, etc. |
| **Database engine** | `Database.{h,cpp}` | See [§5](#5-database-layer) |
| **Task scheduler** | `TaskScheduler.{h,cpp}` | See [§4](#4-task-scheduler) |
| **Command base** | `CommandBase.{h,cpp}` | See [§7](#7-command-line--terminal) |
| **Event bus** | `EventUtil.{h,cpp}` | Cross-service pub/sub |
| **String operations** | `StringOperations.{h,cpp}` | C-string helpers tuned for fixed-size NVM data |
| **Data type conversions** | `DataTypeConversions.{h,cpp}` | int/string/BCD/hex conversions without `printf` |
| **Base64** | `Base64.{h,cpp}` | Encoding + 16-byte unique key generator |
| **Regex match** | `RegexMatch.{h,cpp}` | Minimal regex engine — `.` `*` `+` `?` `^` `$` `[abc]` `[a-z]` `[^abc]` `\<char>`. No alternation or grouping. Powers the `grep` CLI |
| **Safe alloc** | `SafeAlloc.{h,cpp}` | Heap-checked `pdiutil::safe_new<T>(args...)` / `safe_new_array<T>(n)` + `safe_delete` / `safe_delete_array` (null-safe). Refuses allocations that would breach `PDI_SAFE_ALLOC_HEAP_MARGIN` (default 2 KB headroom). Used by the TLS path to bail cleanly on tight heap |
| **Queue / RingBuf / Proto** | `queue/queue.{h,cpp}`, `queue/ringbuf.{h,cpp}`, `queue/proto.{h,cpp}` | Byte queues and a length-prefixed parser |
| **Utility umbrella** | `Utility.{h,cpp}` | Single `#include <utility/Utility.h>` that pulls the lot in (conditional on `ENABLE_*`) |
| **Crypto** | `crypto/` | SHA-1/256/512, HMAC-SHA1, AES (ECB/CBC/CTR), Curve25519, Ed25519 |
| **PdiSTL** | `pdistl/` | A trimmed C++ standard library subset for memory-constrained devices |
| **Fiber** | `fiber/` | Reserved namespace, currently empty |

### 15.2 Interface foundations (recap)

The three abstract bases — `iIOInterface` / `iTerminalInterface` (stream I/O + VT100 helpers), `iUtilityInterface` (`wait`, `millis_now`, `micros_now`, `yield`, `log`), and `iInstanceInterface` (factory for TCP/TLS/FS handles) — are documented in [§13.1](#13-portable-interfaces); listed here only as a cross-reference.

### 15.3 `EventUtil` — cross-service pub/sub

[EventUtil.h](src/utility/EventUtil.h). Global `__utl_event` is the framework's synchronous publisher: services `add_event_listener(event, handler)` at boot and `execute_event(event, arg)` from state changes without taking a direct dependency on each other. Event names live in [EventConfig.h](src/config/EventConfig.h); usage patterns are in [§6.4](#6-service-providers) and [§16.9](#169-react-to-an-event).

### 15.4 `StringOperations` — fixed-size C-string toolkit

[StringOperations.h](src/utility/StringOperations.h). Bounded, `printf`-free helpers (`__strstr`, `__strtrim`, `__are_str_equals`, `__find_and_replace`, `__get_from_json`, IPv4 ↔ string, case, etc.) built for the fixed-size, possibly-not-null-terminated char arrays used by the NVM config structs — every function takes an explicit length bound (default 300).

### 15.5 `DataTypeConversions` — no-`stdio` numeric formatting

[DataTypeConversions.h](src/utility/DataTypeConversions.h). Integer / hex / BCD / string conversions (`StringToUint*`, `Int32ToString`, `BcdToUint8`, `StringToHex16`, …) with no `printf` dependency — using them instead of libc's formatter saves 4-8 KB of flash on AVR/ESP8266 builds.

### 15.6 `Base64`

[Base64.h](src/utility/Base64.h). `base64Encode(input, len, *res)` for standard Base64, plus `genUniqueKey(*key, len)` used by the web session handler for cookie tokens. **No decode helper** — call sites that need it (SMTP `AUTH LOGIN`, SSH parsing) write it inline.

### 15.7 Queues — `QUEUE`, `RINGBUF`, `PROTO`

Three layers, originally from `tuanpmt/esp_mqtt`, retained intact:

| Layer | Path | Use |
|---|---|---|
| **`RINGBUF`** | [queue/ringbuf.h](src/utility/queue/ringbuf.h) | Byte ring with `RINGBUF_Init(r, buf, size)`, `RINGBUF_Put(r, c)`, `RINGBUF_Get(r, *c)` — the substrate |
| **`QUEUE`** | [queue/queue.h](src/utility/queue/queue.h) | Length-tagged record queue built on a ring; `QUEUE_Init`, `QUEUE_Puts`, `QUEUE_Gets`, `QUEUE_IsEmpty` |
| **`PROTO_PARSER`** | [queue/proto.h](src/utility/queue/proto.h) | Length-prefixed packet parser with a completion callback — used by MQTT to assemble frames out of TCP byte streams |

These are used internally by [src/transports/mqtt/](src/transports/mqtt/) and incidentally by the serial event bus. New code should reach for `pdiutil::vector` or `pdistl::deque` unless the byte-level discipline of the ring matters.

### 15.8 Crypto — `src/utility/crypto/`

The framework ships a minimal but production-quality crypto kit. Everything is plain C-style functions over fixed-size buffers; no allocation, no state hidden in globals (except per-context structs you own).

#### Hashes

| Algorithm | Header | API shape |
|---|---|---|
| SHA-1 | [hash/sha1.h](src/utility/crypto/hash/sha1.h) | `SHA1Init/Update/Final`, one-shot `SHA1(out, in, len)` |
| SHA-256 | [hash/sha256.h](src/utility/crypto/hash/sha256.h) | `sha256_init/update/final`, one-shot `sha256(msg, len, out)` |
| SHA-512 | [hash/sha512.h](src/utility/crypto/hash/sha512.h) | `sha512_init/update/final`, one-shot `sha512(msg, len, out)` |
| HMAC-SHA1 | [hmac/hmac_sha1.h](src/utility/crypto/hmac/hmac_sha1.h) | One-shot `hmac_sha1(key, klen, data, dlen, out)` |

#### Symmetric

| Cipher | Header | Modes |
|---|---|---|
| AES-128 / AES-256 (compile-time) | [symmetric/aes/aes.h](src/utility/crypto/symmetric/aes/aes.h) | `ECB_encrypt/decrypt(ctx, buf)`, `CBC_encrypt/decrypt_buffer(ctx, buf, len)`, `CTR_xcrypt_buffer(ctx, buf, len[, resetCtx])` |

`struct AES_ctx` is initialised with `AES_init_ctx(ctx, key)` or `AES_init_ctx_iv(ctx, key, iv)`; reset the IV mid-stream via `AES_ctx_set_iv`.

#### Asymmetric

| Algorithm | Header | API highlights |
|---|---|---|
| Curve25519 (X25519 ECDH) | [asymmetric/curve25519/curve25519.h](src/utility/crypto/asymmetric/curve25519/curve25519.h) | `crypto_scalarmult_base(q, n)`, `crypto_scalarmult(q, n, p)`, `curve25519_create_keypair(pub, priv)`, `curve25519_create_keypair_with_ed25519privkey(pub, priv, ed_priv)` |
| Ed25519 (signing) | [asymmetric/ed25519/ed25519.h](src/utility/crypto/asymmetric/ed25519/ed25519.h) | `ed25519_create_seed`, `ed25519_create_keypair(pub, priv, seed)`, `ed25519_sign(sig, msg, len, pub, priv)`, `ed25519_verify(sig, msg, len, pub) → int`, `ed25519_key_exchange(shared, pub, priv)`, `ed25519_private_to_curve25519` for SSH bridging |

All asymmetric primitives are upstream from the standard portable Ed25519 reference (see [ed25519/readme.md](src/utility/crypto/asymmetric/ed25519/readme.md)). They are used by [src/service_provider/shell/ssh/](src/service_provider/shell/ssh/) for SSH host key generation, the key exchange handshake, and signing the SSH banner. AES-CTR powers the SSH transport encryption.

#### `fixedint.h`

[crypto/fixedint.h](src/utility/crypto/fixedint.h) is a small stdint shim used by the crypto primitives — only relevant if you port them to a non-C99 toolchain.

### 15.9 PdiSTL — `src/utility/pdistl/`

A trimmed subset of the C++ standard library for memory-constrained targets. Adopted from [mike-matera/ArduinoSTL](https://github.com/mike-matera/ArduinoSTL) (based on uClibc++). Lives under namespaces `pdiutil::` (containers — `string`, `vector`, `function`, `shared_ptr`, `unique_ptr`, plus the usual container set) and `pdistd::` (`min`, `max`, `swap`, `<algorithm>`). Standard C-library wrappers (`<cstring>`, `<cstdint>`, `<cmath>`, …) and the C++ ABI glue (`abi/`) are also provided so the framework builds on toolchains that don't ship libstdc++. Per-feature `__UCLIBCXX_*` toggles live in `system_configuration.h`.

**Rule of thumb:** in framework code, use `pdiutil::string` and `pdiutil::vector`, always with the explicit namespace prefix (no `using namespace`) — a future swap to the host `std::` is then mechanical.

### 15.10 The umbrella header

[Utility.h](src/utility/Utility.h) is the **one include** that pulls in: the right scheduler variant (timer vs. cooperative based on `ENABLE_TIMER_TASK_SCHEDULER`), event bus, conversions, string ops, queue, Base64, I/O, and `CommandBase` (when CMD is enabled). Anything in `src/` that does `#include <utility/Utility.h>` gets the whole foundation in one shot. New code should follow this pattern rather than reaching for individual files.

### 15.11 Gotchas

- **`fiber/` is empty.** It's a reserved namespace for future coroutine/fiber primitives. Don't try to import from it.
- **`pdiutil::string` is not `std::string`.** Some methods are missing or renamed; check the header before relying on a particular API. SBO size and allocator semantics differ from libstdc++.
- **`StringOperations` always takes a length.** Don't drop the default — the framework's NVM strings are often non-null-terminated up to their buffer size, and the default-300 cap is what keeps the helpers safe.
- **Crypto routines are constant-time only where the upstream made them so.** Ed25519 verify is, AES is table-based and timing-side-channel sensitive. Don't run on a network where timing attacks are in the threat model without auditing.
- **`Base64` has no decode.** Plan for that if your code path needs round-trip Base64.
- **`PROTO_PARSER` keeps state across calls.** Don't share one parser across two connections; allocate one per stream.
- **The umbrella header has a transitive cost.** It pulls 20+ headers. If a file *only* needs `StringOperations`, include that one directly to keep your TU lean — `Utility.h` is for files that genuinely consume the whole surface.

---

## 16. Extending the Framework

Every section so far has its own "how do I add one of these?" subsection. This section is the **cross-reference index** — pick what you're building, follow the cookbook, jump to the deep dive when you need it. The intent is that an experienced embedded developer can skim this page and ship a non-trivial extension without re-reading the whole document.

### 16.1 Decision: which extension surface fits?

| You want to… | Reach for | Section |
|---|---|---|
| Support a new MCU / board | **Add a device port** | [§16.2](#162-add-a-new-device) → [§14](#14-device-layer--porting-guide) |
| Add a missing hardware capability across all ports | **Add a portable interface** | [§16.3](#163-add-a-new-interface) → [§13.6](#13-portable-interfaces) |
| Add a new framework-level feature (logging sink, fleet manager, …) | **Add a service provider** | [§16.4](#164-add-a-new-service-provider) → [§6.5](#6-service-providers) |
| Speak a new wire protocol (CoAP, gRPC, …) | **Add a transport** | [§16.5](#165-add-a-new-transport) → [§10.7](#10-transports) |
| Persist new configuration to NVM | **Add a database table** | [§16.6](#166-add-a-new-database-table) → [§5.9](#5-database-layer) |
| Add a screen to the on-device portal | **Add a web page / controller** | [§16.7](#167-add-a-new-web-page) → [§8.10](#8-web-server) |
| Add a terminal command | **Add a CLI command** | [§16.8](#168-add-a-new-cli-command) → [§7.11](#7-command-line--terminal) |
| Hand-roll persistence in a sketch without touching codegen | **Use the DB escape hatch** | [§11.3 `AddingDatabaseTable`](#113-addingdatabasetable--app-side-persistence-without-codegen) |
| React to a service event without coupling to it | **Subscribe to the event bus** | [§16.9](#169-react-to-an-event) → [§6.4](#6-service-providers), [§15.3](#15-utility-library) |
| Run periodic / long-running work | **Use the task scheduler** | [§4](#4-task-scheduler) |
| Encrypt outbound HTTP / MQTT / SMTP / OTA without changing service code | **Turn on `ENABLE_TLS_SERVICE`** — orchestrator swaps `m_client` to a TLS-capable instance, every service that holds it auto-upgrades | [§6.2.16](#6-service-providers), [§12.4](#124-the-expensive-features-called-out) |
| Serve the on-device portal over HTTPS | **Turn on `ENABLE_HTTPS_SERVER`** + drop cert/key into `/etc/http/server.{crt,key}` | [§6.2.12](#6-service-providers), [§6.2.16](#6-service-providers) |
| Provision an HTTPS cert | **`tls` CLI** on esp32 *or* [scripts/GenTlsCerts.py](scripts/GenTlsCerts.py) off-device | [§2.3](#23-what-the-scripts-do), [§17.5](#175-frequently-asked-questions) |
| Talk to an existing service from the sketch | **Call the singleton** | [§16.10](#1610-call-an-existing-service-from-a-sketch) |

If your need straddles two of these, the **lower** layer in the stack is usually the right answer — adding a transport is preferable to adding a service that hard-codes a protocol; adding an interface is preferable to a service that touches the vendor SDK.

### 16.2 Add a new device

Full guide: [§14.8](#148-adding-a-new-device--step-by-step). The shape:

1. `devices/<board>/` folder with the umbrella header (`<board>.h` for SDK includes), the platform-macro header (`<board>_device_config.h` for `RODT_ATTR` / `strcat_ro` / `CRITICAL_SECTION_*`), and the two aggregators (`<board>_pdi.h` / `<board>_pdi.cpp`).
2. Implement the **always-required** trio (`DeviceControlInterface`, `DatabaseInterface`, `InstanceInterface`) and define their `__i_*` singletons.
3. Branch on `DEVICE_<NAME>` in [src/interface/pdi.h](src/interface/pdi.h) **and** in [devices/DeviceConfig.h](devices/DeviceConfig.h) (the second branch pulls in `<board>_device_config.h`).
4. Add per-board limits in [devices/DeviceConfig.h](devices/DeviceConfig.h).
5. `python3 scripts/DeviceSetup.py -d <board>`.
6. Build [examples/PdiStack](examples/PdiStack/) against the new board.
7. Iterate on optional interfaces (network, storage, WiFi, threading, TLS) one flag at a time.
8. Add the architecture to `library.properties`.

Validation checklist: [§14.9](#149-validation-checklist).

### 16.3 Add a new interface

Full guide: [§13.6](#13-portable-interfaces). The shape:

1. Pick the right group: `drivers/`, `middlewares/`, `modules/`, `threading/`, or top-level.
2. Write [src/interface/pdi/.../i<Name>.h](src/interface/pdi/) with pure virtuals, forward-declared concrete class, `extern Concrete __i_<name>;`.
3. Guard inclusion behind an `ENABLE_*` if your consumers are gated.
4. Add a stub in [devices/mockdevice/](devices/mockdevice/) so off-device builds link.
5. Provide a default implementation under [src/interface/pdi/impl/](src/interface/pdi/impl/) if more than one port would re-implement the same logic.

**The bar:** at least one port should genuinely implement it *differently* from another. A single-port interface is a smell — keep it concrete in the device until the second port shows up.

### 16.4 Add a new service provider

Full guide: [§6.5](#6-service-providers). The shape:

1. New `ENABLE_<NAME>_SERVICE` flag in [devices/DeviceConfig.h](devices/DeviceConfig.h).
2. New `SERVICE_<NAME>` enum value (guarded by the flag) in [src/service_provider/ServiceProvider.h](src/service_provider/ServiceProvider.h).
3. `src/service_provider/<group>/<Name>ServiceProvider.{h,cpp}` deriving from `ServiceProvider`:
   ```cpp
   <Name>ServiceProvider() : ServiceProvider(SERVICE_<NAME>, RODT_ATTR("<Name>")) {}
   bool initService(void* arg) override { /* register tasks, listeners */ return ServiceProvider::initService(arg); }
   ```
4. Conditional include in [src/PdiStack.h](src/PdiStack.h) + `initService(...)` call in [src/PdiStack.cpp](src/PdiStack.cpp), slotted per the **init order rules** ([§6.3](#6-service-providers)).
5. Override `printConfigToTerminal` / `printStatusToTerminal` so the `srvc` CLI sees you.
6. (Optional) Add a CLI command ([§16.8](#168-add-a-new-cli-command)) and/or a web controller ([§16.7](#167-add-a-new-web-page)).
7. (Optional) Persist config via a database table ([§16.6](#166-add-a-new-database-table)).

Anti-patterns: ctor side-effects, direct service-to-service coupling for fan-out (use events), allocating after `setup()`. See [§6.6](#6-service-providers).

### 16.5 Add a new transport

Full guide: [§10.7](#10-transports). The shape:

1. `src/transports/<proto>/<Proto>Client.{h,cpp}` that takes an `iClientInterface*` and exposes `begin / send / receive / end`. No `__database_service`, no `__task_scheduler` — transports parse bytes, period.
2. Build a `<Proto>ServiceProvider` *on top* — the service owns the schedule, config, and event dispatch.
3. One new `ENABLE_*` flag, wired through [PdiStack.h](src/PdiStack.h) / [PdiStack.cpp](src/PdiStack.cpp) the usual way.

The split matters: a transport written this way is reusable from sketches and from other services without dragging the framework in. See HTTP / MQTT / SMTP as references.

### 16.6 Add a new database table

Two paths, depending on whether your table belongs to **the framework** or **your application**.

**Framework table** (must round-trip through every port, surface in `srvc`, be visible to the web portal) — full guide [§5.9](#5-database-layer):

1. Define the struct in a new or existing [src/config/<X>Config.h](src/config/) with the [§3.4](#3-configuration-system) 6-part contract.
2. Pick a free address using the gap calculator.
3. Add a `defItems` entry to each `devices/<board>/config/DBTableSchema.json` that needs it.
4. Regenerate: `python3 scripts/DeviceSetup.py -d <board>`.
5. Add include + global + accessor pair in [DatabaseServiceProvider.{h,cpp}](src/service_provider/database/DatabaseServiceProvider.h) under your `ENABLE_*` flag.
6. Raise `MAX_DB_TABLES` if needed.

**Sketch-local table** (only the application code needs to see it):

- Use the escape hatch from [§11.3](#113-addingdatabasetable--app-side-persistence-without-codegen) — `class MyTable : public DatabaseTable<ADDR, my_struct> {};` directly in the `.ino`. No codegen, no schema edit. The trade-off: not visible to the framework's reflective tools.

### 16.7 Add a new web page

Full guide: [§8.10](#8-web-server). The shape:

1. Add the route constant to [src/webserver/routes/Routes.h](src/webserver/routes/Routes.h).
2. Create `MyPage.h` under [src/webserver/pages/](src/webserver/pages/) holding the body HTML as a `PROG_RODT_ATTR` const string.
3. Create `MyController` under [src/webserver/controllers/](src/webserver/controllers/) deriving from `Controller`, register the route in `boot()` (with `AUTH_MIDDLEWARE` unless deliberately public).
4. Add a value-member of the controller in [src/webserver/WebServer.h](src/webserver/WebServer.h), conditional on the appropriate flag.
5. Add a menu entry on the home page so users can navigate to it.

For sketch-side controllers (no framework edit), see [examples/Dev/AddingController/](examples/Dev/AddingController/AddingController.ino) and [§11.4](#114-addingcontroller--extending-the-web-portal).

Compose the response in three chunked `send(..., send_in_chunks=true)` calls — header / body / footer — to stay under the per-`send` 1.8 KB cap ([§8.7](#8-web-server), [§8.11](#8-web-server)).

### 16.8 Add a new CLI command

Full guide: [§7.11](#7-command-line--terminal). The shape:

1. Add `CMD_NAME_<X>` to [src/service_provider/cmd/commands/CommandCommon.h](src/service_provider/cmd/commands/CommandCommon.h).
2. Create [src/service_provider/cmd/commands/<X>Command.h](src/service_provider/cmd/commands/) — a struct derived from `CommandBase` that calls `SetCommand`, `AddOption`s in its ctor and implements `execute(cmd_term_inseq_t)` returning a `cmd_result_t`.
3. Include the header in [CommandLineServiceProvider.h](src/service_provider/cmd/CommandLineServiceProvider.h) and call `CommandBase::RegisterCommand(CMD_NAME_<X>, &<X>Command::Registrar)` from `CommandLineServiceProvider::initService`.

The new command auto-completes via TAB, gets history via ↑/↓ (storage required), and respects CTRL+C / CTRL+Z if you override `executeTermInputAction`.

Constraints to know: command name ≤ 8 chars, ≤ 3 options, option names ≤ 2 chars ([§7.12](#7-command-line--terminal)).

### 16.9 React to an event

Service-to-service coupling for fan-out should go through `__utl_event`, not direct calls:

```cpp
__utl_event.add_event_listener(EVENT_WIFI_CONNECTED, [&](void* arg) {
    // your handler — runs on the inline scheduler lane
});
```

To fire your own:

```cpp
__utl_event.execute_event(EVENT_MY_THING, &payload);   // synchronous
```

Add new event names to [src/config/EventConfig.h](src/config/EventConfig.h). The framework's own events are listed in [§6.4](#6-service-providers).

### 16.10 Call an existing service from a sketch

Every `ENABLE_*`'d service is a global, named per [§1.7](#1-architecture-overview):

```cpp
#include <PdiStack.h>

void setup() {
    PdiStack.initialize();

    // configuration
    wifi_config_table cfg;
    __database_service.get_wifi_config_table(&cfg);
    memcpy(cfg.sta_ssid, "MyNet", 6);
    __database_service.set_wifi_config_table(&cfg);

    // service control
    __wifi_service.scan_aps_and_configure_wifi_station_async(0);

    // GPIO
    __i_dvc_ctrl.gpioMode(DIGITAL_WRITE, 2);
    __i_dvc_ctrl.gpioWrite(DIGITAL_WRITE, 2, 1);

    // scheduling
    __task_scheduler.setInterval([&] {
        __i_dvc_ctrl.getTerminal()->writeln("tick");
    }, 1000, millis());

    // events
    __utl_event.add_event_listener(EVENT_WIFI_CONNECTED, [&](void*) {
        // reactive code
    });
}

void loop() { PdiStack.serve(); }
```

This is the **only** API contract you have to know for application code — every service is reachable through one named global, every singleton is reachable through one `__i_*` symbol, every operation is sync-and-cheap or scheduled-and-deferred.

### 16.11 Where extensions tend to go wrong

A quick scan of the most common ways extensions fail to land cleanly:

- **Constructor-time work.** The single most repeated mistake — covered in [§13.5](#13-portable-interfaces), [§6.6](#6-service-providers), [§12.10](#1210-gotchas). Use `init*` methods, not constructors.
- **Direct cross-service calls instead of events.** If your new service `__x_service` calls into `__y_service` from a callback, you have created a hidden ordering dependency that breaks on init-order changes. Use events for fan-out, direct calls only when the dependency is structural (e.g. `MqttServiceProvider` always needs `__database_service`).
- **Holding pointers into transient buffers.** `CommandBase` option values, web request args, HTTP response bodies are all borrowed pointers ([§7.4](#7-command-line--terminal), [§10.8](#10-transports), [§8.7](#8-web-server)). Copy or call `holdOptionValue` if you need the bytes past the next call.
- **Pulling vendor headers above the device layer.** If your service or interface file `#include <esp_wifi.h>`, the build breaks for every other port. Push the dependency down into `devices/<board>/`.
- **Forgetting the codegen step.** A new framework-level table needs a JSON schema edit *and* the script re-run *and* the `DatabaseServiceProvider.{h,cpp}` edit. Skipping any one of the three leaves an inconsistent state.
- **Ignoring the `ENABLE_*` triple-gate.** A new feature flag must guard (1) the device aggregator include, (2) the orchestrator include, (3) the orchestrator's `initService` call. Miss one and the build either fails to link or carries dead code. See [§3.3](#3-configuration-system).
- **One global per slot.** Two `XServiceProvider` instances overwrite `m_services[SERVICE_X]` silently. Two `<X>Table` globals at the same address are quietly skipped by `Database::register_table`. One per slot, always.

---

## 17. Troubleshooting & FAQ

Common problems and recurring questions. Each entry is short — deep explanations live in the sections they point to.

### 17.1 Build & flash problems

**`undefined reference to lfs_*` at link time.**
You're on an old checkout that still expects the LittleFS submodule. LittleFS is now vendored under [external/littlefs/](external/littlefs/) — pull `main` to get the in-tree sources. See [§2.4](#24-vendored-externals).

**`fatal error: DeviceSetup.h: No such file or directory`.**
You're on an old checkout that still hard-required `DeviceSetup.h`. The current header uses `#if __has_include("DeviceSetup.h")` to fall back to ESP32. Pull `main`, or run the setup script if you want a non-default target: `python3 DeviceSetup.py -d <board>`. See [§2.2](#22-installation-flow).

**Build succeeds against ESP8266 / UNO but device misbehaves at runtime.**
You installed via Library Manager (or copied a fresh repo) without running `DeviceSetup.py -d <board>` for the actual target. The ESP32 fallback in `DeviceConfig.h` produced an ESP32-shaped binary. Run the setup script for your target board. See [§2.6](#26-gotchas).

**Build succeeds, but `srvc list` lists no services and the AP never appears.**
`devices/DeviceSetup.h` still points at the wrong device — usually because you switched boards without re-running the setup script. Re-run with the new `-d <board>`, or `rm devices/DeviceSetup.h` to fall back to ESP32. See [§2.6](#26-gotchas).

**`multiple definition of __i_<x>` link error.**
Two ports defined the same singleton, or you referenced a `devices/<board>/*.cpp` from somewhere other than its aggregator. Per [§14.3 The two aggregator files](#143-the-two-aggregator-files), each device translation unit must be reached through exactly one `.cpp`-include chain.

**`fatal error: esp_wifi.h: No such file or directory` building for AVR.**
A vendor-specific header leaked above the device layer. The fix is to push that include down into `devices/<board>/`. See [§16.11](#16-extending-the-framework).

**Compile error mentioning `pdiutil::function` / `pdiutil::vector` on an obscure target.**
The host compiler is missing GCC's variadic-macro / variadic-template extensions used in [PdiSTL](src/utility/pdistl/). Switch to a GCC-based toolchain. See [§9.9](#9-logger), [§15.11](#15-utility-library).

### 17.2 Boot & runtime problems

**Device boots but `srvc list` shows nothing or factory-resets every 5 s.**
NVM is in an invalid state (corrupt checksum, mismatched schema after a struct edit). With `AUTO_FACTORY_RESET_ON_INVALID_CONFIGS` on (the default), the device auto-recovers — wait one reset cycle. If it loops, the table sizes have outgrown the address allocation; see [§5.10](#5-database-layer).

**Boot hangs after `Starting PDIStack !` with no AP.**
WiFi STA connect is timing out. `WIFI_STATION_CONNECT_ATTEMPT_TIMEOUT` defaults to 1 s ([§3.3.3](#3-configuration-system)) — short. Usually means the configured STA credentials are stale. Hold the flash button 6-7 s to factory-reset (assuming `CONFIG_CLEAR_TO_DEFAULT_ON_FACTORY_RESET`), reconnect to AP, set fresh credentials.

**`PdiStack.initialize()` runs to completion but `LogI` calls print nothing.**
No `ENABLE_LOG_*` flag is set in [devices/DeviceConfig.h](devices/DeviceConfig.h) — every `Log*` site compiled to nothing. Uncomment at least `ENABLE_LOG_INFO`. See [§9.8](#9-logger).

**Application code runs once and stops.**
Almost always missing `PdiStack.serve()` in `loop()`. Without it the inline scheduler never advances and nothing periodic fires. The minimal sketch is [examples/PdiStack/PdiStack.ino](examples/PdiStack/PdiStack.ino).

**Heap fragmentation after hours/days of uptime.**
Symptoms: `register_task` returning `-1`, `pdiutil::vector` growth failing, transports refusing new connections. You're allocating inside a hot path. The discipline is in [§12.5](#125-heap-discipline) — pre-reserve containers, hold-then-free, no `new` after `setup()`.

**`__task_scheduler.register_task` always returns `-1`.**
The slot table (`MAX_SCHEDULABLE_TASKS = 25`) is full. Either you have a legitimate need for more (raise the constant in [src/config/Common.h](src/config/Common.h)) or a service is leaking tasks (`ps` will show the population). See [§4.7](#4-task-scheduler).

**One service starves the others; serve loop feels jittery.**
The scheduler runs **one** inline task per `serve()` iteration ([§4.5](#4-task-scheduler)). If a task does too much work per call, every other task suffers. Split into a state machine or promote to a cooperative lane.

### 17.3 Network / portal problems

**Cannot connect to the `pdiStack` AP.**
Default password is `pdiStack@123` (case-sensitive). Confirm the AP is up via `srvc list` + `srvc status WiFi` over serial. If still missing, `ENABLE_WIFI_SERVICE` may be off — re-check [devices/DeviceConfig.h](devices/DeviceConfig.h).

**Cannot reach `http://192.168.0.1` from the AP.**
Default subnet is hardcoded in [WifiConfig.h](src/config/WifiConfig.h). If `ENABLE_DYNAMIC_SUBNETTING` is on, the AP may have picked a different subnet; check the IP your phone/laptop got from DHCP and use that subnet's `.1`.

**Login form keeps redirecting to `/login`.**
Cookie is being rejected or session expired (default 300 s). Check the browser actually accepts cookies for the device IP; private-mode windows often don't.

**The page is truncated mid-HTML.**
A `send()` was called without `send_in_chunks=true` and the payload exceeded `PAGE_HTML_MAX_SIZE = 1800`. Compose pages in three chunked sends (header/body/footer) — see [§8.7](#8-web-server) and the stale-comment caveat in [§11.4](#114-addingcontroller--extending-the-web-portal).

**MQTT connects but no callbacks fire.**
For inbound data, confirm `Subscribe` was actually acknowledged by waiting for the `OnSubscribed` callback before publishing on the same topic. `Subscribe` returns success on *enqueue*, not on SUBACK ([§10.8](#10-transports)).

**OTA pings the server but never updates.**
The framework only updates when **the server's `latest` version is strictly newer than the device's `FIRMWARE_VERSION`** (see [§6.2.5 OtaServiceProvider](#6-service-providers)). Bump `FIRMWARE_VERSION` in [GlobalConfig.h](src/config/GlobalConfig.h) before publishing.

**Email send times out.**
SMTP is plaintext; the framework has **no STARTTLS** ([§10.5](#10-transports)). TLS-only SMTP providers (Gmail SMTP submission, SES) won't work. Use a relay that accepts plain SMTP on the LAN, or a mailtrap-style test sink.

**HTTPS server returns connection-refused / handshake-fail.**
Cert/key not on the device FS, or paths don't match the defaults. Upload via SFTP to `/etc/http/server.crt` and `/etc/http/server.key` (paths overridable via `TLS_DEFAULT_SERVER_*_PATH` in [TlsConfig.h](src/config/TlsConfig.h)). On esp32, the on-device `tls q=1,t=0,l=256,n=<CN>,i=<IPv4>` command can generate them in place; on esp8266, use [scripts/GenTlsCerts.py](scripts/GenTlsCerts.py) off-device and upload. Watch for `ENABLE_HTTPS_SERVER_MTLS` builds — they additionally need `/etc/http/client-ca.crt`. See [§6.2.16](#6-service-providers).

**Outbound HTTPS to an unknown CA fails.**
The bundled `m_client` in `PdiStack` defaults to `setVerifyPeer(false)` so first-boot demos work without provisioned trust anchors. For production cross-network paths, edit [src/PdiStack.cpp](src/PdiStack.cpp): drop the `setVerifyPeer(false)` line and uncomment `setCertificateAuthorityPath(TLS_DEFAULT_OUTBOUND_CA_BUNDLE_PATH)`, then upload a CA bundle to `/etc/ssl/ca-bundle.crt`.

**esp8266 TLS handshake stack-overflows / fails on a 16 KB record.**
Two ceilings: (1) BearSSL ECDSA sign needs more than the 4864 B default Arduino cont_t stack — the TLS task runs on a dedicated `TLS_TASK_STACK_SIZE = 6500` B stack; lower at your own risk. (2) `TLS_IBUF_SIZE = 2048` defaults below BearSSL's spec maximum of ~17 KB. Peers that emit 16 KB records will fail the handshake — bump it (and accept the heap cost) only if you can't control the peer.

**HTTPS portal works on esp32 but esp8266 rejects every connection.**
Most likely `ENABLE_NAPT` is also on. The two cannot coexist on esp8266 — both want too much of the same fixed heap. Disable NAPT for HTTPS builds. See [§12.4](#124-the-expensive-features-called-out).

**`EVENT_WIFI_STA_GOT_IP` listener (auto cert-gen) never fires.**
`ENABLE_SERVER_TLS_CERT_GENERATION_AT_RUNTIME` is gated on both `ENABLE_TLS_CERT_GENERATION` (esp32 only) and a successful STA association. Confirm STA is actually getting an IP via `net ip`; auto-mint won't run on AP-only mode.

### 17.4 CLI / terminal problems

**Telnet / SSH login fails with the right credentials.**
The CLI uses the same login row as the web portal (`login_credential_table`). If you've changed credentials via the web, telnet/SSH inherits them. If `srvc status Auth` shows defaults, factory reset.

**SSH won't accept any client.**
You probably never generated a key pair. From the CLI:
```
ssh q=1,t=1
```
(`q=1` is `SSH_COMMAND_QUERY_KEYGEN`, `t=1` selects the algorithm — see [SshConfig.h](src/config/SshConfig.h)).

**`scp -s …` is slow.**
By design — the framework streams SFTP in small bolus chunks (≤256 B) and has overwrite overhead on flash filesystems. Expected throughput 0.2-1 KB/s ([§6.2.14](#6-service-providers), [§7.9](#7-command-line--terminal)). Use OTA, not SFTP, for firmware.

**Two telnet clients can't connect simultaneously.**
The Telnet transport currently holds a single `m_client`, so a second telnet TCP connect is refused until the first disconnects. The session layer itself supports `PDI_MAX_SESSIONS` concurrent slots — serial + telnet + ssh coexist fine, and multi-telnet just needs a client-vector refactor in [TelnetServiceProvider.cpp](src/service_provider/transport/TelnetServiceProvider.cpp) ([§7.8](#78-multi-terminal-session-lifecycle)).

**`fwrite` won't exit.**
Press **ESC** to finalize (commit + exit). See [§7.7 Built-in command inventory](#77-built-in-command-inventory).

**TAB autocomplete works but ↑/↓ history doesn't.**
History is persisted to `/.term_history` — only available when `ENABLE_STORAGE_SERVICE` is on. Autocomplete uses the in-RAM command registry and works regardless. See [§7.6](#7-command-line--terminal).

**Command name longer than 8 chars is silently truncated.**
`CMD_SIZE_MAX = 8`. Shorten the name. See [§7.12](#7-command-line--terminal).

### 17.5 Frequently asked questions

**Q. Can I run multiple `PDIStack` instances?**
No. The framework relies on a single global `PdiStack` plus one singleton per interface and per service. There's no path to multi-instance — and on the target memory budgets, no point.

**Q. Can I use `String` (Arduino) instead of `pdiutil::string`?**
In sketch code, yes. In framework code, no — `String`'s allocator differs by core and breaks the off-AVR portability the framework relies on. See [§15.9](#15-utility-library).

**Q. Why are command names so short?**
`CMD_SIZE_MAX = 8` and `CMD_OPTION_SIZE_MAX = 3` are sized for AVR-class RAM. Loosening them costs RAM proportional to `MAX_SCHEDULABLE_TASKS × CMD_OPTION_MAX × CMD_OPTION_SIZE_MAX`. The trade-off was made for UNO.

**Q. Why is SSH so heavy?**
Ed25519 keypair + Curve25519 ECDH + AES-CTR streaming + per-session protocol state. Roughly +150 KB flash and +8-16 KB heap per session ([§12.4](#124-the-expensive-features-called-out)). It's why ESP32 is the recommended target for SSH builds.

**Q. Can the device be configured without flashing — e.g. from a USB serial bootstrap?**
Yes. After the first boot, every persisted config in NVM can be changed via the web portal, the CLI, or another sketch that calls `__database_service.set_*_table`. Flash only changes *defaults* ([§3.8](#3-configuration-system)).

**Q. Where do logs go?**
Serial at 115200 only, by default. The framework doesn't ship file/telnet/SSH/syslog/HTTP routing for logs — implement a custom `LoggerInterface` if you need fan-out ([§9.6](#9-logger)).

**Q. How do I enable TLS?**
TLS ships in-tree as of the last release. Set `ENABLE_TLS_SERVICE` in [devices/DeviceConfig.h](devices/DeviceConfig.h) — esp8266 uses BearSSL, esp32 uses mbedTLS. Add `ENABLE_HTTPS_SERVER` for the inbound web portal (cert/key at `/etc/http/server.{crt,key}`), `ENABLE_HTTPS_SERVER_MTLS` for client-cert auth (`/etc/http/client-ca.crt`), `ENABLE_TLS_CERT_GENERATION` + `ENABLE_SERVER_TLS_CERT_GENERATION_AT_RUNTIME` (esp32 only) for on-boot auto-mint. Cost is significant — see [§12.4](#124-the-expensive-features-called-out). On esp8266 `ENABLE_TLS_SERVICE` cannot coexist with `ENABLE_NAPT`. Full reference: [§6.2.16](#6-service-providers).

**Q. How do I provision certs?**
Two paths. **On-device (esp32 only):** `tls q=1,t=0,l=256,n=device.local,i=192.168.1.50` from the CLI generates and stores a self-signed EC P-256 cert + key at the default `TlsConfig.h` paths. **Off-device (any board):** `python3 scripts/GenTlsCerts.py --dns device.local --ip 192.168.1.50` produces files under `certs/`, then upload via SFTP. For a stable dev CA, run with `--gen-ca` once and use `--ca-cert`/`--ca-key` for each device cert thereafter — every device that trusts that CA bundle then trusts every device cert it signs.

**Q. Is there a Wokwi / on-host simulator?**
Not bundled. `devices/mockdevice/` is a header-only stub that lets the framework *compile* off-device for static analysis, but it doesn't simulate behaviour. For interactive testing, flash to real hardware.

**Q. How do I unit-test framework code?**
The interface-based design supports it: mock the `i*Interface` your unit depends on and link against the unit's `.cpp`. The framework doesn't ship a test harness — bring your own (CppUTest, Catch2, …). PdiSTL works on host x86 with GCC.

**Q. What's the upgrade path from the old `esp8266-framework` repo?**
Migration is mostly mechanical — rename `__i_dvc_ctrl` calls, replace `String` with `pdiutil::string`, follow the [§14.8 device port guide](#148-adding-a-new-device--step-by-step) if you had a custom board.

**Q. Does the framework receive security updates?**
Crypto primitives are upstream (`ed25519`, `curve25519`, `aes`, `sha*`) — vendored copies. There's no automated CVE tracking. If you ship a product on it, mirror the upstream sources and watch their release feeds yourself.

**Q. Can I disable the auto-factory-reset behaviour?**
Yes — comment out `AUTO_FACTORY_RESET_ON_INVALID_CONFIGS` in [devices/DeviceConfig.h](devices/DeviceConfig.h). Then invalid NVM halts further service initialization until manually cleared, which is sometimes what you want during development.

**Q. Where do I report bugs / file issues?**
GitHub: <https://github.com/Suraj151/pdi-framework>.

### 17.6 When you're really stuck

The best signal-to-noise loop on an unknown problem:

1. **Enable logs.** Uncomment `ENABLE_LOG_ALL` in [devices/DeviceConfig.h](devices/DeviceConfig.h), flash, watch serial @ 115200.
2. **List services.** `srvc list` over the terminal — confirms what actually booted, with per-service state and task counts.
3. **Print a service's status.** `srvc status <name>` — state, tracked PIDs, ready to correlate against `ps`.
4. **List active tasks.** `ps` — column `%CPU` is your CPU-hog detector; the `OWN` column ties tasks back to their owning session or kernel.
5. **Watch tasks over time.** `top i=2000; n=10` (built-in refresh), or `watch c=ps; i=2000; n=10` for a scroll-preserving variant.
6. **Poke lifecycle live.** `srvc stop <name>` freezes the service (SIG_STOP its tasks), `srvc start <name>` resumes; `kill 19 <pid>` / `kill 18 <pid>` do the same at task granularity. Watch `ST` in `ps` flip `S`→`T`→`S`.
7. **Check NVM integrity.** `srvc status DB` reports DB validity; outside that, an `AUTO_FACTORY_RESET_ON_INVALID_CONFIGS` build will reset every 5 s if NVM is bad.
8. **Reboot.** `reboot` — the explicit version, since pulling power loses serial output.

If none of those localise the issue, open a GitHub issue with: device + board-package version, the relevant `ENABLE_*` flags, the serial log up to and including the failure, and the exact command sequence that reproduces.

