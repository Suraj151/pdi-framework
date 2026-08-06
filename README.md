# PDI Framework — Portable Device Interface Stack

One C++ codebase that runs on an ESP32, an ESP8266 or an Arduino UNO. Application and service code is written against interfaces — `iWiFiInterface`, `iFileSystemInterface`, `iTcpServerInterface`, etc — and each board ships an adapter that implements them. Nothing above the adapter layer knows which chip it is sitting on.

What comes out of the box is closer to a small system than to a sketch template: a WiFi captive portal and web UI, an HTTP/HTTPS server, MQTT, OTA, an SSH server with SFTP and scp, Telnet, SMTP, a virtual filesystem with users and permissions, a task scheduler with three execution models, and a Linux-flavoured shell sitting on top of all of it.

<p align="center">
  <img width="500" src="https://github.com/Suraj151/pdi-framework/blob/master/doc/pdi-framework.jpg">
</p>

## What it can do

**Portability is the whole point.** Services depend on abstract interfaces, not on vendor SDKs. Supporting a new board means writing an adapter for the interfaces that board can actually offer; the services, the portal and the shell come along unchanged. Anything a board can't do is switched off at compile time rather than stubbed at runtime.

**Services.** WiFi with captive portal, HTTP/HTTPS web portal, MQTT client, OTA updates, SSH server, Telnet server, SFTP subsystem, SMTP client, GPIO control (locally and over MQTT/HTTP), an NVM-backed configuration database, TLS via BearSSL or mbedTLS, ESPNOW mesh, authentication, and a device-IoT hook for your own cloud. Each one is a `ServiceProvider` with the same lifecycle, and `srvc list / status / start / stop / restart` drives them at runtime the way systemd drives units.

**A real shell.** The same forty-plus commands are reachable over serial, Telnet and SSH: `ls`, `cat`, `grep`, `head`, `tail`, `wc`, `hexdump`, `df`, `mount`, `chmod`, `chown`, `umask`, `ps`, `top`, `kill`, `renice`, `net`, `host`, `ping`, `date`, `useradd`, `passwd`, `watch` and the rest. Login, history, tab completion, in-place file editing and Ctrl+C all behave the way muscle memory expects.

**A filesystem with users.** Several backends mount into one tree and are routed by longest prefix: LittleFS at the root, a read-only `/proc` of live system nodes, a writable `/sys` where GPIO pins are files (`echo 1 > /sys/class/gpio/5/value`), a `/dev` with `null`/`zero`/`random`, and a RAM-backed `/tmp`. Permissions, ownership and per-session umask are enforced in the VFS layer, so `/etc/passwd` and `/etc/shadow` mean what they say and two logged-in users genuinely see different access.

**Scheduling that scales down and up.** Tasks run inline, cooperatively, or preemptively on a hardware tick, with priorities, POSIX nice values and per-task signals. On ESP32 an external relocatable ELF can be loaded from the filesystem and launched as a background process — `elfload <path>` returns a pid you can `ps` and `kill`, no reflash involved.

**Found on the network without help.** A from-scratch mDNS/DNS-SD responder built straight on lwIP UDP advertises `pdi-<mac>.local` and the services it is listening on, so the device answers to a name and shows up in `avahi-browse -a`. Name lookups walk IP literal, then `/etc/hosts`, then DNS.

**Configured from a browser.** Session-based login, one settings page per service, GPIO control, a storage browser, and MQTT and email testers — all served from flash-resident page fragments.

## Quick Start

1. **Install** from the **Arduino Library Manager** (search "pdi-framework"). Builds for ESP32 by default.
   For ESP8266 / Arduino UNO, run: `python3 scripts/DeviceSetup.py -d <board>`.
2. Open **File → Examples → pdi-framework → PdiStack**, then compile and upload.

   Note : ESP32 require 1.4MB+ app size so make sure you will select suitable partition scheme that fits required app size.
4. On your phone or laptop, look for the WiFi network **`pdiStack`** — password **`pdiStack@123`**.
5. Browse to **http://192.168.0.1** and log in as **`pdiStack` / `pdiStack@123`**.

That's it — the device is now running a web portal, a remote shell and file transfer.

Prefer a remote shell? `ssh pdiStack@<device-ip>` or `telnet <device-ip>` (default ports 22 and 23).
Copying files? `scp -s file pdiStack@<device-ip>:/path` or `sftp -P 22 pdiStack@<device-ip>`.

Manual clone paths, the autogen script, board-package versions and git-ignored files are covered in [§2 Build & Toolchain](#2-build--toolchain).

### Supported Boards

| Device | Arduino board version |
|---|---|
| esp32 | 3.3.3 |
| esp8266 | 3.1.2 |
| arduinouno | 1.8.6 |

Not every board exposes every capability. An Arduino UNO has no WiFi, so the web server, MQTT and OTA services are compiled out on that port.

## What's Inside

**Services** — WiFi · HTTP/S server · MQTT · OTA · SSH · Telnet · SFTP · SMTP · GPIO · Serial · Terminal · Database · TLS · ESPNOW · Auth · Device-IoT.
Per-service reference in [§6 Service Providers](#6-service-providers).

**Utilities** — task scheduler, event bus, queues, string helpers, data converters, crypto, PdiSTL, factory reset.
Full inventory in [§15 Utility Library](#15-utility-library).

**Storage** — one VFS tree over LittleFS, `/proc`, `/sys`, `/dev` and `/tmp`, with POSIX permissions and multi-user access control.
Details in [§6.2.11 Storage](#6211-storage-interface-init-no-provider).

**CLI** — 40+ built-in commands, listed in [§7.7 Built-in command inventory](#77-built-in-command-inventory).

**Extras** — captive portal, GPIO events over MQTT/HTTP/email, NAT on the ESP8266 lwIP port ([§2.4.1](#241-nat-and-mesh)), mesh over ESPNOW.

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

The **[Detailed Documentation](#detailed-documentation)** below is the in-tree reference for contributors and porters. Jump to whatever you're working on:

- **[1. Architecture Overview](#1-architecture-overview)** — layered model, ports and adapters, runtime lifecycle.
- **[2. Build & Toolchain](#2-build--toolchain)** — install, switch board, autogen scripts, vendored externals.
- **[3. Configuration System](#3-configuration-system)** — every `ENABLE_*` flag and what depends on it.
- **[4. Task Scheduler](#4-task-scheduler)** — three modes, four policies, and how to choose.
- **[5. Database Layer](#5-database-layer)** — schema model, address-based store, JSON codegen.
- **[6. Service Providers](#6-service-providers)** — per-service init flow, CLI and web surface, events.
- **[7. Command Line / Terminal](#7-command-line--terminal)** — full CLI reference and how to add a command.
- **[8. Web Server](#8-web-server)** — request lifecycle, routes, views, adding a page.
- **[9. Logger](#9-logger)** — levels, macros, and the zero-cost-when-disabled pattern.
- **[10. Transports](#10-transports)** — HTTP, MQTT and SMTP client internals.
- **[11. Examples Walkthrough](#11-examples-walkthrough)** — what each bundled sketch demonstrates.
- **[12. Memory & Performance Notes](#12-memory--performance-notes)** — flash and RAM cost per feature.
- **[13. Portable Interfaces](#13-portable-interfaces)** — the ports every device adapter implements.
- **[14. Device Layer & Porting Guide](#14-device-layer--porting-guide)** — how to add a new board.
- **[15. Utility Library](#15-utility-library)** — event bus, string ops, embedded STL, crypto.
- **[16. Extending the Framework](#16-extending-the-framework)** — adding services, commands, pages.
- **[17. Troubleshooting & FAQ](#17-troubleshooting--faq)** — common issues and fixes.

# Detailed Documentation

Each section stands on its own, and points at the file worth opening when you want the code.

---
## 1. Architecture Overview

The stack is layered, and the layering is enforced by what each layer is allowed to include. The bottom half describes what a device must be able to *do* — pure abstract interfaces. The top half describes what the product *offers* — services, transports, the web server, the shell. A single device-selection macro and a set of `ENABLE_*` flags bolt the two halves together at compile time.

### 1.1 Layered model

```
                       ┌────────────────────────────────────────┐
   Application sketch  │  your .ino — initialize() + serve()    │
                       └───────────────────┬────────────────────┘
                                           │ uses
                       ┌───────────────────▼────────────────────┐
   Orchestrator        │  PDIStack — one global, wires it all   │
                       └───────────────────┬────────────────────┘
                                           │ wires up
   ┌───────────────────────────────────────┼───────────────────────────────────────┐
   ▼                                       ▼                                       ▼
┌──────────────────┐         ┌──────────────────────────┐          ┌───────────────────────┐
│  Services        │         │  Web server / CLI / SSH  │          │  Transports           │
│  one per feature │◄────────┤                          │◄─────────┤  HTTP · MQTT · SMTP   │
└────────┬─────────┘         └──────────────┬───────────┘          └──────────┬────────────┘
         │                                  │                                 │
         │            all of them consume only what is below                  │
         ▼                                  ▼                                 ▼
                       ┌──────────────────────────────────────────────────┐
   Utilities           │  scheduler · event bus · database engine ·       │
                       │  string ops · crypto · embedded STL · queues     │
                       └────────────────────────┬─────────────────────────┘
                                                │ + abstract contracts
                                                ▼
                       ┌──────────────────────────────────────────────────┐
   Interfaces          │  iDeviceControl · iClient · iServer · iWiFi ·    │
   (the "ports")       │  iFileSystem · iSerial · iGpio · iDatabase ·     │
                       │  iNtp · iPing · iWdt · threading primitives      │
                       └────────────────────────┬─────────────────────────┘
                                                │ implemented by
                                                ▼
                       ┌──────────────────────────────────────────────────┐
   Devices             │  esp32 · esp8266 · arduinouno · mockdevice       │
   (the "adapters")    │  concrete implementations + one aggregator each  │
                       └──────────────────────────────────────────────────┘
```

Dependencies only ever point downward. A service never includes a device header; it holds an `i*Interface` pointer and calls through it. The one place device code is reached from above is [src/interface/pdi.h](src/interface/pdi.h), which picks exactly one board aggregator based on the `DEVICE_*` macro that `scripts/DeviceSetup.py` writes into `devices/DeviceSetup.h`.

| Layer | Role | Sees |
|---|---|---|
| Application | your sketch: `initialize()` once, `serve()` in the loop | orchestrator |
| Orchestrator | the `PdiStack` global that conditionally starts every enabled service | services, interfaces |
| Services | one `ServiceProvider` subclass per feature, self-registering in a global table | utilities, interfaces |
| Utilities | device-agnostic primitives — scheduler, event bus, database engine, crypto, STL subset | interfaces only |
| Interfaces | pure abstract ports, grouped by role into drivers, middlewares, modules and threading | nothing |
| Devices | the concrete per-MCU adapters | vendor SDK / Arduino core |

### 1.2 The `ServiceProvider` contract

Every feature service derives from `ServiceProvider` and puts itself into a global table indexed by a `service_t` enum:

```cpp
ServiceProvider(service_t st, const char *_svc_name)
    : m_service_t(st), m_service_name(_svc_name), m_service_routine_task_id(-1) {
    m_services[st] = this;          // self-registration
}
```

The base class fixes the lifecycle. `initService` is the required override and runs once from `PDIStack::initialize`. `stopService` tears down. `printConfigToTerminal` and `printStatusToTerminal` are what the `srvc` command prints. A static `getService(st)` lets one service find another without include cycles. Because the shape is uniform, `srvc` can enumerate, start, stop and inspect anything without knowing what it is.

Start reading at [src/service_provider/ServiceProvider.h](src/service_provider/ServiceProvider.h).

### 1.3 The ports

Interfaces are grouped by role, not by feature:

| Group | Examples | Implemented by | Used by |
|---|---|---|---|
| drivers | `iGpioInterface`, `iWdtInterface` | device adapter | GPIO service, core |
| middlewares | `iClientInterface`, `iServerInterface`, `iNtpInterface`, `iPingInterface`, `iDeviceControlInterface`, `iUpgradeInterface` | device adapter | OTA, MQTT, email, HTTP, IoT |
| modules | `iSerialInterface`, `iStorageInterface`, `iFileSystemInterface`, `iWiFiInterface`, `iHttpServerInterface` | device adapter | serial, storage, WiFi |
| threading | `iExecution`, `iMutex`, `iCondvar`, `iContext` | device adapter, where the board can | scheduler in contextual mode |
| top level | `iDatabaseInterface`, `iDeviceIotInterface` | device adapter / framework | database, IoT |

A port is usable once it provides `iDeviceControlInterface`, `iDatabaseInterface`, `iSerialInterface` (when the serial service is on), plus whichever groups the feature flags it claims to support actually need.

### 1.4 Compile-time composition

There is no runtime plugin system. Every optional capability is chosen by an `ENABLE_*` macro in `devices/DeviceConfig.h`, and the macros cascade — turning on networking is what makes MQTT, the HTTP server and OTA meaningful for that board, and SSH only appears when storage is also present:

```c
#ifdef ENABLE_NETWORK_SERVICE
  #define ENABLE_MQTT_SERVICE
  #define ENABLE_WIFI_SERVICE
  #if defined(ENABLE_STORAGE_SERVICE)
    #define ENABLE_SSH_SERVICE
  #endif
#endif
```

One flag then drives three things in lockstep:

```
   ENABLE_SSH_SERVICE
        │
        ├──▶ device aggregator includes the interfaces SSH needs
        ├──▶ orchestrator includes SSHServiceProvider
        └──▶ initialize() calls its initService()
```

The triple guard is deliberate. Comment out one `#define` and the feature leaves the binary completely — no flash, no RAM, no static-init cost, no dangling stub.

### 1.5 Runtime lifecycle

```
boot
 │
 ├─ static init of the PdiStack global
 │     ├─ event bus starts
 │     ├─ scheduler gets its utility interface
 │     └─ (WiFi builds) a TCP or TLS client instance is created
 │
setup()
 │
 ├─ PdiStack.initialize()
 │     ├─ device brings up its own features
 │     ├─ terminal acquired and handed to the ServiceProvider base
 │     ├─ database service starts            (always)
 │     └─ every enabled service starts       (conditional)
 │
loop()
 │
 └─ PdiStack.serve()
       ├─ web server handles pending clients
       ├─ scheduler runs inline tasks
       ├─ device yield + event dispatch
       └─ contextual lanes get a slice       (cooperative, then preemptive)
```

Two scheduling lanes live side by side. Inline tasks advance on every `serve()` tick and suit short periodic work. Contextual tasks run on their own stacks and the loop merely yields to them; they exist on boards whose port supplies `iExecution` and a matching scheduler. [§4](#4-task-scheduler) covers picking between them.

### 1.6 Global instances and naming

A small set of well-known globals, all prefixed `__`, so any of them can be found with one grep:

| Symbol | From | What it is |
|---|---|---|
| `__i_dvc_ctrl` | device | the one `iDeviceControlInterface` for this build |
| `__i_db`, `__i_fs`, `__i_wifi`, `__i_http_server`, `__i_ntp`, `__i_ping`, `__i_serial` | device | interface singletons, present only when the matching flag is on |
| `__task_scheduler`, `__utl_event` | utilities | scheduler and event bus |
| `__database_service`, `__wifi_service`, `__mqtt_service`, `__ota_service`, … | services | one per `ServiceProvider` subclass |
| `__i_cooperative_scheduler`, `__i_preemptive_scheduler` | threading port | the contextual lanes |
| `PdiStack` | orchestrator | the single application-facing facade |

Services reach each other by global symbol when the dependency is fixed, and through `ServiceProvider::getService(st)` when the lookup has to stay generic — which is exactly what `srvc` does.

---
## 2. Build & Toolchain

The build target is the Arduino IDE / arduino-cli toolchain. The code compiles clean against `-std=c++14` or newer with GCC's variadic-macro extension, and nothing else is assumed — PlatformIO or a hand-rolled `make` works as long as you reproduce the usual Arduino-core defines.

### 2.1 Board packages

| Board | Arduino board package version |
|---|---|
| Arduino UNO | 1.8.6 |
| ESP8266 | 3.1.2 |
| ESP32 | 3.3.3 |

These are the versions the adapters in `devices/` are written against. Vendor headers move between core releases, so a newer board package can need adapter changes.

### 2.2 Installing

Two routes: the Library Manager, or a git clone if you intend to work on the framework itself.

**Library Manager — builds for ESP32 with no extra steps.**

1. Install the board package at the version above through the Boards Manager.
2. Tools → Manage Libraries → search `pdi-framework` → install.
3. File → Examples → pdi-framework → PdiStack, pick an ESP32 board, compile, flash.

   <table>
     <tr>
       <td width="50%"><img src="https://github.com/Suraj151/pdi-framework/blob/master/doc/library-install-arduino.png" width="100%"></td>
       <td width="50%"><img src="https://github.com/Suraj151/pdi-framework/blob/master/doc/open-example-arduino.png" width="100%"></td>
     </tr>
   </table>

A fresh install already carries ESP32-shaped placeholder database headers, and `devices/DeviceConfig.h` falls back to `DEVICE_ESP32` when no generated setup header is present — hence the zero-step ESP32 build.

**For ESP8266 or Arduino UNO,** generate the per-device files first:

```
cd <your-Arduino-libraries-path>/pdi-framework/scripts
python3 DeviceSetup.py -d esp8266        # or arduinouno
```

That writes `devices/DeviceSetup.h` with the right `DEVICE_<NAME>` and regenerates the database table headers for the target. Changing boards later is the same one-liner with a different name.

**Git clone, for contributors.**

1. Install the board package as above.
2. Clone into your Arduino `libraries/` directory:
   ```
   cd ~/Arduino/libraries
   git clone https://github.com/Suraj151/pdi-framework.git
   cd pdi-framework
   ```
   Linux and macOS use `~/Arduino/libraries/`. On Windows it is either `Documents\Arduino\libraries\` for a per-user install, or `%USERPROFILE%\AppData\Local\Arduino15\packages\<vendor>\hardware\<arch>\<ver>\libraries\` for a cross-arch one.
3. Run `python3 scripts/DeviceSetup.py -d <board>` if you are not targeting ESP32. Open a terminal at the sketchbook location, as in the images below.

   <table>
     <tr>
       <td width="50%"><img src="https://github.com/Suraj151/pdi-framework/blob/master/doc/sketch-location-arduino.png" width="100%"></td>
       <td width="50%"><img src="https://github.com/Suraj151/pdi-framework/blob/master/doc/script-devicesetup-arduino.png" width="100%"></td>
     </tr>
   </table>

4. Open the example, select the board, compile, flash.

   <table>
     <tr>
       <td width="50%"><img src="https://github.com/Suraj151/pdi-framework/blob/master/doc/esp32-tool-arduino.png" width="100%"></td>
       <td width="50%"><img src="https://github.com/Suraj151/pdi-framework/blob/master/doc/open-example-arduino.png" width="100%"></td>
     </tr>
   </table>

There are no git submodules to initialise — LittleFS is vendored in-tree.

### 2.3 What the scripts do

```
  DeviceSetup.py -d <board>
        │
        ├──▶ devices/DeviceSetup.h        #define DEVICE_<NAME>
        │
        └──▶ CreateDBSourceFromJson.py
                   │  reads the board's DBTableSchema.json
                   └──▶ JsonToCpp.py ──▶ src/database/tables/*.h
```

| Script | Run it when |
|---|---|
| `DeviceSetup.py` | switching target device — it chains the database codegen for you |
| `CreateDBSourceFromJson.py` | changing the schema of the active device |
| `JsonToCpp.py` | never directly; it is the codegen engine |
| `Util.py` | never directly; shared template and path helpers |
| `GenTlsCerts.py` | provisioning HTTPS certificates off-device, or signing an ESP32 cert with a stable dev CA |

Generated headers are passed through `clang-format --style=Microsoft` when the formatter is on `PATH`, so they read like hand-written code. Without it they are written unformatted and compile the same.

### 2.4 Vendored externals

Two bodies of external code live directly in the repo. [external/littlefs/](external/littlefs/) is the filesystem used by the storage interface on ESP8266 and ESP32, and by everything downstream of it — SFTP, shell history, the file commands. AVR builds have no storage and never reach it. [lwip/](lwip/) is a customised lwIP 1.4 used by the legacy NAPT path described below.

#### 2.4.1 NAT and mesh

Both are radio-level capabilities layered onto WiFi rather than regular services.

**NAT on ESP8266** rewrites IP-header fields on packets in transit so that clients joining the device's access point reach the upstream network the station link is connected to. From ESP8266 core 2.6.x onward this runs on lwIP v2 (IPv4), selected in the IDE's Tools menu, and is the path in normal use. The older NAPT implementation uses the vendored lwIP 1.4: rename the core's `tools/sdk/lwip` aside, drop this repo's `lwip/` in its place, and pick the "lwIP 1.4 compile from source" variant. Which one is active is a compile-time choice. On the service side, `ENABLE_NAPT` makes the WiFi service schedule a one-shot NAPT enable once the station link is up.

**Mesh over ESPNOW** wraps Espressif's peer-to-peer link-layer protocol into a small API so applications can build broadcasts and hop-distance topologies without touching the driver. It shares the radio with station mode and is configured from the application. Paired with `ENABLE_DYNAMIC_SUBNETTING` and `ENABLE_INTERNET_BASED_CONNECTIONS` on the WiFi service, it gives each node a notion of how many hops it sits from the hub.

#### 2.4.2 mDNS and DNS-SD

The responder is written from scratch on raw lwIP UDP — `udp_*` plus `igmp_joingroup` on ESP8266, the same wrapped in `LOCK_TCPIP_CORE` on ESP32 — and runs as an ordinary service. No Arduino mDNS library is involved.

```
  EVENT_WIFI_STA_GOT_IP
        │
        ├─ hostname pdi-<last-3-mac-bytes> written to /etc/hostname
        ├─ join 224.0.0.251:5353
        └─ answer queries as they arrive        (callback-driven, nothing to pump)
                │
                ├─ A                    →  hostname → address
                ├─ PTR _services…       →  which service types exist
                ├─ PTR <type>           →  instances of a type
                └─ SRV / TXT            →  port + metadata, bundled with A
```

It advertises what the build is actually listening on: `_http._tcp` or `_https._tcp`, `_ssh._tcp`, `_sftp-ssh._tcp`, `_telnet._tcp`. Outbound clients such as MQTT and OTA listen for nothing, so nothing is advertised for them. `cat /etc/hostname` shows the name, `ping pdi-<xxxxxx>.local` proves it resolves, and `srvc status MDNS` lists the address and the advertised set. Service types, TTLs and the multicast group live in [src/config/MdnsConfig.h](src/config/MdnsConfig.h).

### 2.5 How the ESP32 default works

Three things line up so that a first build needs no scripts:

```
  devices/DeviceConfig.h
        │
        ├─ #if __has_include("DeviceSetup.h")  →  use the generated macro
        └─ #else                               →  #define DEVICE_ESP32
                │
                ├─ per-port config cascade ends in esp32_device_config.h
                └─ checked-in placeholder table headers are ESP32-shaped
```

Running `DeviceSetup.py` for another board overrides all three: the generated `DeviceSetup.h` wins over the fallback and fresh table headers replace the placeholders. To come back to ESP32, either re-run the script with `-d esp32` or delete `devices/DeviceSetup.h` and let the fallback take over again.

Because the fallback is silent, a build flashed onto an ESP8266 or an UNO without running the script compiles happily with ESP32 table addresses and feature flags. Run the script whenever you leave the ESP32 default, and again whenever you come back — `git checkout src/database/tables/` restores the placeholders if the generated ones are still lying around.

#### 2.5.1 Per-port capability flags

Board-specific answers live in `<board>_device_config.h`, not in the central config, which keeps the selection logic board-agnostic:

| Macro | Set by | Effect |
|---|---|---|
| `DEVICE_SUPPORTS_TLS` | esp8266, esp32 | lets `ENABLE_TLS_SERVICE` take effect; ports without it get the flag undefined automatically |
| `DEVICE_SUPPORTS_CONTEXTUAL_EXECUTION` | esp8266, esp32 | same shape, for the cooperative and preemptive lanes |
| `DEVICE_SUPPORTS_TLS_CERT_GENERATION` | esp32 | gates on-device certificate generation |
| `MAX_DIGITAL_GPIO_PINS`, `MAX_ANALOG_GPIO_PINS`, `MAX_DB_TABLES` | every port | per-board limits |
| `ENABLE_NETWORK_SERVICE`, `ENABLE_AUTH_SERVICE`, `ENABLE_STORAGE_SERVICE`, `ENABLE_GPIO_BASIC_ONLY` | every port | per-board defaults — AVR omits network, auth and storage; the ESP ports enable them |

The contract is simple: genuinely per-board facts go in the per-port header, and the central `DeviceConfig.h` carries only cross-board feature flags and the auto-undef chains. A new port sets its `DEVICE_SUPPORTS_*` macros and the optional services fall in line by themselves.

---
## 3. Configuration System

Configuration is layered, compile-time and additive. There is no `.ini` file, no runtime parser and no over-the-air feature toggle. Which services exist, how big each table is, what the defaults are and which interfaces a port supplies are all decided when you build.

### 3.1 Three tiers

| Tier | Lives in | Owns | Written by |
|---|---|---|---|
| Device | `devices/DeviceSetup.h` (generated), `devices/DeviceConfig.h`, and each port's `<board>_device_config.h` | the `DEVICE_*` selector, the `ENABLE_*` flags, per-board limits, and platform macros such as `RODT_ATTR` and `CRITICAL_SECTION_ENTER/EXIT` | integrator picking board and features; porter describing the board |
| Common | `src/config/Common.h`, `src/config/GlobalConfig.h` | cross-cutting constants and the always-present `global_config` table | framework author, integrator |
| Service | one `*Config.h` per service under `src/config/` | per-service knobs plus the struct that gets persisted to NVM | service author |

Everything funnels through [src/config/Config.h](src/config/Config.h), which conditionally includes the service-tier headers according to the flags the device tier settled on. Every translation unit in the framework reaches configuration through that one header.

### 3.2 How the pieces include each other

```
  DeviceSetup.h            generated:  #define DEVICE_<NAME>
        │
        ▼
  DeviceConfig.h           DEVICE_* ──▶ ENABLE_* cascade
        │                  + pulls in <board>_device_config.h for platform macros
        ▼
  Common.h                 shared defaults other configs reference
        │
        ▼
  GlobalConfig.h           the always-loaded global_config table
        │
        ▼
  Config.h ──── conditional includes ────┐
                                          │
   WifiConfig · ServerConfig · HttpConfig · StorageConfig · OtaConfig · …
                     one per ENABLE_*-gated service
```

Two consequences worth internalising. A config header for a disabled service is never included, so its struct does not exist in the build and the NVM address it would have taken is reclaimed. And a config header may include `Common.h` for shared defaults but never another service config — coupling two services through configuration is exactly the thing the layout prevents.

### 3.3 Device-tier flags

Every flag acts as a triple gate: which interface the device exposes, which service the orchestrator includes, and which config header `Config.h` pulls in.

#### 3.3.1 Service flags

| Flag | On by default | Brings in | Needs | Cost |
|---|---|---|---|---|
| `ENABLE_SERIAL_SERVICE` | all boards | serial service + `SerialConfig.h` | `iSerialInterface` | low |
| `ENABLE_STORAGE_SERVICE` | yes, except UNO | filesystem + `StorageConfig.h` | `iStorageInterface`, `iFileSystemInterface` | medium |
| `ENABLE_GPIO_SERVICE` | yes | GPIO service + `GpioConfig.h` | `iGpioInterface` | low; `ENABLE_GPIO_BASIC_ONLY` trims it further |
| `ENABLE_CMD_SERVICE` | yes | the CLI | a terminal source | low |
| `ENABLE_AUTH_SERVICE` | non-UNO | auth service + the credential table | storage | low |
| `ENABLE_NETWORK_SERVICE` | non-UNO | umbrella for everything below | TCP client/server, NTP, ping, WiFi | — |
| `ENABLE_WIFI_SERVICE` | with network | WiFi service + `WifiConfig.h` | `iWiFiInterface` | medium |
| `ENABLE_HTTP_SERVER` | with network | web portal + `ServerConfig.h`, `HttpConfig.h` | `iHttpServerInterface` | medium |
| `ENABLE_HTTPS_SERVER` | off | serves the portal on 443 with certs from the filesystem | `ENABLE_TLS_SERVICE` | high |
| `ENABLE_HTTP_CLIENT` | with network | outbound HTTP | TCP client | low |
| `ENABLE_MQTT_SERVICE` | with network | MQTT service + `MqttConfig.h` | TCP client | medium |
| `ENABLE_OTA_SERVICE` | with network | OTA service + `OtaConfig.h` | TCP client, `iUpgradeInterface` | low |
| `ENABLE_EMAIL_SERVICE` | with network | email service + `EmailConfig.h` | TCP client | low |
| `ENABLE_TELNET_SERVICE` | with network | telnet server | TCP server | low |
| `ENABLE_SSH_SERVICE` | with network and storage | SSH server, SFTP, scp | TCP server, filesystem | high |
| `ENABLE_DEVICE_IOT` | with network | IoT service + `DeviceIotConfig.h` | TCP client | low |
| `ENABLE_TLS_SERVICE` | off | TLS client and server + `TlsConfig.h`; turns on contextual execution, since TLS runs on its own cooperative task | BearSSL on esp8266, mbedTLS on esp32 | high — see [§12.3](#123-the-expensive-features) |
| `ENABLE_TLS_CERT_GENERATION` | off, esp32 | the `tls` command and the on-device issuer | TLS service, esp32 | medium |
| `ENABLE_SERVER_TLS_CERT_GENERATION_AT_RUNTIME` | off | mints a self-signed cert on first boot once the station gets an IP | cert generation | one-shot |
| `ENABLE_CONTEXTUAL_EXECUTION` | off | cooperative and preemptive lanes | the threading interfaces | high (per-task stacks) |
| `ENABLE_TIMER_TASK_SCHEDULER` | off | timer-backed scheduler variant | a device timer | depends |

#### 3.3.2 Behaviour flags

| Flag | Effect |
|---|---|
| `ENABLE_GPIO_BASIC_ONLY` | digital-only GPIO, used on UNO |
| `ENABLE_DYNAMIC_SUBNETTING` | AP subnet and gateway chosen at runtime instead of statically |
| `ENABLE_NAPT` | AP clients reach the station's network; costs heap |
| `IGNORE_FREE_RELAY_CONNECTIONS` | skip already-connected SSIDs during scan, avoiding mesh loops |
| `ALLOW_WIFI_CONFIG_MODIFICATION`, `ALLOW_WIFI_SSID_PASSKEY_CONFIG_MODIFICATION_ONLY` | how much of the WiFi form is editable |
| `ALLOW_MQTT_CONFIG_MODIFICATION`, `ALLOW_OTA_CONFIG_MODIFICATION` | same for the MQTT and OTA forms |
| `AUTO_FACTORY_RESET_ON_INVALID_CONFIGS` | bad checksum at boot resets to defaults instead of halting |
| `CONFIG_CLEAR_TO_DEFAULT_ON_FACTORY_RESET` | factory reset writes the default structs back rather than zeroing |
| `ENABLE_CONSOLE_LOG_ALL` / `_INFO` / `_WARNING` / `_ERROR` / `_SUCCESS` | per-level console gates; with none set, every log call compiles away |
| `ENABLE_SYSLOG_SERVICE` | also persist log lines under `/var/log`; needs storage |

#### 3.3.3 Per-device limits

| Macro | Purpose | Typical values |
|---|---|---|
| `MAX_DIGITAL_GPIO_PINS` | size of the GPIO config table | 14 UNO · 12 esp32 WROOM/S2/S3/C6 · 8 esp32 C3/H2 · 9 esp8266 |
| `MAX_ANALOG_GPIO_PINS` | same, analog side | 5 UNO · 4 esp32 · 1 esp8266 |
| `MAX_DB_TABLES` | upper bound on registered tables | 5 UNO · 15 esp |
| `MAX_SCHEDULABLE_TASKS` | inline scheduler slots | 25 |
| `MAX_FACTORY_RESET_CALLBACKS` | reset hooks | same as the task count |
| `WIFI_STATION_CONNECT_ATTEMPT_TIMEOUT` | station connect budget, seconds | 1 |
| `WIFI_CONNECTIVITY_CHECK_DURATION` | link recheck interval, ms | 5000 |
| `INTERNET_CONNECTIVITY_CHECK_DURATION` | internet recheck cadence | same |

### 3.4 The shape of a service config

Every per-service config follows the same six-part shape, which is what lets the database layer and the web forms treat them uniformly. WiFi is the canonical example:

```cpp
#define WIFI_CONFIGS_BUF_SIZE 30
#define DEFAULT_SSID         USER          // 1. defaults
#define DEFAULT_PASSPHRASE   PASSPHRASE
const uint8_t DEFAULT_AP_LOCAL_IP[] = {192, 168, 0, 1};

#define ALLOW_WIFI_CONFIG_MODIFICATION    // 2. policy switches

typedef struct { ... } __status_wifi_t;   // 3. runtime status, never persisted
extern __status_wifi_t __status_wifi;

struct wifi_configs {                     // 4. the persisted struct
  wifi_configs() { clear(); }             //    default-constructible
  void clear() { ... }                    //    resettable
  char sta_ssid[WIFI_CONFIGS_BUF_SIZE];   //    fixed-size members only
};

const int wifi_config_size = sizeof(wifi_configs) + 5;  // 5. size on NVM
using wifi_config_table = wifi_configs;                 // 6. canonical alias
```

The `+ 5` is the per-table framing on NVM — table id and checksum. Boot sums those sizes across every registered table and checks the total against what the database interface reports as available.

### 3.5 What each config file carries

| File | Persisted table | Headline knobs |
|---|---|---|
| Common | — | `USER`, `PASSPHRASE`, task count, time durations, HTTP request budget |
| GlobalConfig | `global_config_table` | config version, firmware version, release, launch time |
| WifiConfig | `wifi_config_table` | buffer sizes, default IPs, modification policy |
| ServerConfig | `login_credential_table` | session name, cookie max age |
| HttpConfig | — | client buffer size, request limits, HTTPS port, HSTS age |
| TlsConfig | — | per-session record buffers, TLS task stack and poll interval, default cert/key paths |
| OtaConfig | `ota_config_table` | host, port, version, check cadence |
| MqttConfig | general / LWT / pub-sub tables | broker, last will, publish and subscribe slots |
| GpioConfig | `gpio_config_table` | pin map, modes, event conditions and channels |
| EmailConfig | `email_config_table` | SMTP host, port, auth, default subject |
| DeviceIotConfig | `device_iot_config_table` | config and OTP URLs, channel keys, sampling bounds |
| SerialConfig | — | mode, baud, interface selection |
| StorageConfig | — | mount point, path limits |
| SshConfig | — | key algorithms, RSA key bits, session count, auth policy, host-key and config paths |
| NetworkConfig | — | network-wide timeouts |
| EventConfig | — | the event enum and channel registry |

The always-on tables — global config, plus credentials and WiFi when those services are on — are the minimum NVM footprint. Everything else arrives with its flag.

### 3.6 What depends on what

```
  NETWORK ─┬─ WIFI ─── HTTP_SERVER ─┬─ HTTPS_SERVER ── TLS_SERVICE ── CONTEXTUAL_EXECUTION
           │                        └─ (needs STORAGE for certs)
           ├─ MQTT · OTA · EMAIL · DEVICE_IOT        (TCP client)
           ├─ TELNET                                 (TCP server, pairs with CMD)
           └─ SSH ─── STORAGE                        (host keys, SFTP)

  STORAGE ─┬─ AUTH                                   (credentials survive reboot)
           └─ SYSLOG ─── (+NETWORK) ─── SYSLOG_FORWARD

  TLS_CERT_GENERATION ── TLS_SERVICE + esp32
  DEVICE_IOT ── the application implements iDeviceIotInterface and passes it to initService
```

Most of these are enforced structurally — the dependent flags are physically nested inside `#ifdef ENABLE_NETWORK_SERVICE` and friends in `DeviceConfig.h`. Two pairings are worth remembering because nothing stops them at compile time: TLS and NAPT both want more heap than an ESP8266 has, and enabling TLS implies contextual execution because the TLS engine runs on its own cooperative task.

### 3.7 Common build shapes

| Goal | Keep | Drop |
|---|---|---|
| Smallest possible (UNO class) | serial, CLI, basic GPIO | everything else |
| Offline gateway | add storage and auth | everything network |
| Headless networked node | add network, WiFi, MQTT, OTA, NTP | web server, SSH, email |
| Full portal | the stock `DeviceConfig.h` | — |
| Diagnostics build | stock plus console logging, optionally syslog | — |
| Concurrency demo | stock plus contextual execution | — |
| HTTPS portal | stock plus TLS and HTTPS (esp32: also cert generation) | NAPT on esp8266 |
| HTTPS with client certs | as above plus mTLS | — |

### 3.8 Defaults are not current values

Editing `DEFAULT_SSID` does not change the SSID a running device uses. Defaults are consulted exactly twice: on first boot, when NVM is empty or the checksum fails, and on factory reset when the framework is configured to write defaults back rather than zeroing.

To change a live value, go through the portal, the CLI (`net connsta`, `iot sethost`, and so on), or the database service directly and persist the table. Defaults govern the fallback state; the database holds the current one.

### 3.9 Conventions worth keeping

Branch on `ENABLE_*` with `#ifdef`, never with a runtime `if` — the unreached branch still needs symbols the link cannot provide. Keep config structs POD and fixed-size; a pointer or a `pdiutil::string` inside one cannot be serialised to NVM. Never include one service config from another; if two need the same value, it belongs in `Common.h`. And gate code on the capability (`ENABLE_WIFI_SERVICE`) rather than on the board (`DEVICE_ESP32`) — the board changes, the capability is what the code actually depends on.

---
## 4. Task Scheduler

The scheduler is the pacemaker. NTP refresh, MQTT reconnect, OTA polling, GPIO blinking, session expiry, watchdog feeding, log heartbeats — all of it is a task. Every board gets the inline scheduler for free; a port that implements the threading interfaces adds two more lanes on top.

Implementation is [src/utility/TaskScheduler.h](src/utility/TaskScheduler.h), and the global is `__task_scheduler`.

### 4.1 Three modes

| Mode | Runs on | Stack | Needs port support | Right when |
|---|---|---|---|---|
| inline | the main loop's stack, inside `run()` | the loop's own | no | the task is short and never blocks — this covers nearly everything |
| cooperative | its own stack, advancing only when it yields or sleeps | one you size | yes | you want to write it as `while(1){ step(); sleep(N); }` and you trust it to yield |
| preemptive | its own stack, interrupted by a timer ISR | one you size | yes | the task may block or run long, and you can't trust it to yield |

All three coexist. Each pass of `serve()` ticks the lanes in order:

```
  serve()
    ├─ __task_scheduler.run()                     inline: at most one task, then out
    ├─ device yield + event dispatch
    ├─ cooperative_scheduler.tick_from_loop()     one slice
    └─ preemptive_scheduler.yield()               ISR already drives it; this is courtesy
```

`tick_from_loop()` is the main-loop entry point for a lane. Cooperative schedulers forward it to `run()`; preemptive ones ignore it because a hardware timer is already driving them. A port that needs a different main-loop hook overrides only that one call.

### 4.2 Policies

Independently of mode, each task carries a policy that decides who goes first when several inline tasks come due on the same tick:

| Policy | Selection rule |
|---|---|
| FIFO (default) | priority, biased by a logarithmic lateness boost |
| round robin | equal slices, favouring whoever ran least recently |
| deadline | earliest deadline first, with double the lateness weight |
| fair share | favours whoever has consumed the least cumulative CPU |

All four share the same skeleton: a base term of `effective_priority × 100`, plus `policy_boost × min(floor(log2(ms_late + 1)), cap)`. The logarithmic aging term is what guarantees a ceiling — no low-priority task waits more than about five seconds before its accumulated score overtakes a high-priority one. Policies apply to the inline lane; contextual lanes schedule themselves.

### 4.3 What a task record holds

A task is a POD stored by value in a vector reserved to `MAX_SCHEDULABLE_TASKS`, so steady-state scheduling causes no heap churn.

| Group | Fields | Why |
|---|---|---|
| identity | id, name, owner | pid, a read-only name pointer (often in flash), and the owning session — 0 means kernel |
| callback | the function | what actually runs |
| schedule | duration, last run, remaining attempts, priority, nice, policy, mode | everything scoring needs |
| lifecycle | state, pending signal | ready / running / sleeping / stopped / zombie, plus a queued signal |
| observability | created-at, run count, last and total exec time in µs | what `ps` and `top` render, including %CPU |
| contextual | the lane executive | present only in contextual builds |

Exec time is sampled in microseconds around each callback, so a task that finishes in well under a millisecond still registers a non-zero cost rather than rounding to zero.

### 4.4 The API, by what you want to do

Every registration call takes a trailing name and owner. Fill them in — that is what makes a task visible in `ps` and reachable by `pkill NAME`. Services normally don't call these directly; they go through the `ServiceProvider` wrappers, which supply the service name automatically.

**Registering and cancelling**

| Want to | Call | Note |
|---|---|---|
| run once after N ms | `setTimeout(fn, dur, now, prio, name, owner)` | auto-removed after it fires |
| run every N ms | `setInterval(fn, dur, now, prio, name, owner)` | runs until cancelled |
| reschedule a one-shot | `updateTimeout(id, fn, dur, now)` | replaces callback and duration in place |
| reschedule a periodic | `updateInterval(id, …)` | falls back to registering fresh if the id is gone |
| spell everything out | `register_task(fn, dur, prio, last, maxAtt, name, owner)` | the lowest-level entry point |
| cancel either kind | `clearTimeout(id)` / `clearInterval(id)` | marks for removal on the next sweep |
| look one up | `get_task(id)` | to change policy or mode after the fact |
| force a re-sort now | `rebaseAndRestartPrioTasks()` | the current sweep breaks after one task |

`register_task` returns `-1` when the slot table is full — check it before assuming the task exists.

**Metadata, signals, rendering**

| Want to | Call |
|---|---|
| name a task after the fact | `setTaskName(id, name)` — the pointer must outlive the task |
| attach an owning session | `setTaskOwner(id, sid)` |
| change nice, -20..19 | `setTaskNice(id, nice)`, then rebase for an immediate re-sort |
| queue a signal on one task | `sendSignal(id, sig)` |
| signal every task with a name | `sendSignalByName(name, sig, requester, is_root)` — this is what `pkill` and `srvc stop` use |
| print the `ps` view | `printPsToTerminal(terminal, filter_owner)` |
| promote a task to a lane | `scheduleUnderExecSched(sched, id, mode, stack)` |

### 4.5 What happens on a tick

```
  run() ──▶ handle_tasks()
             │
             1. now = millis
             2. score every task, sort an index table (the vector never moves,
                so task ids stay stable), 3 ms tolerance on due-times
             3. for each task in score order:
                  │
                  ├─ contextual task?  deliver its pending signal to the lane
                  │                    (STOP suspends, CONT resumes, KILL ends),
                  │                    reap it if finished, then move on
                  │
                  ├─ consume the pending signal under one critical section
                  │     KILL / TERM ─▶ zombie, callback dropped
                  │     STOP        ─▶ stopped, skipped until CONT
                  │     CONT        ─▶ back to sleeping
                  │
                  ├─ due?  sample µs, run the callback, sample µs again,
                  │        update exec time, total time, run count, state
                  │
                  ├─ advance last-run by whole intervals (catch-up capped at 3)
                  └─ yield to the platform
             4. after one task, if a rebase was requested, break out —
                the next tick re-sorts from scratch
             5. drop everything whose attempts hit zero
```

Point 4 is why the scheduler keeps jitter low without preemption: each pass of `serve()` runs at most one inline task before handing control back to the platform, the web server and the contextual lanes. It also means a freshly registered high-priority task is picked up on the very next tick.

### 4.6 Picking a mode

```
  Does the task work for more than a few ms,
  or call a blocking SDK API?
   │
   ├─ No ──▶ inline. No port work, no stack to size. Default answer.
   │
   └─ Yes ─▶ Would you naturally write it as while(1){ step(); sleep(N); } ?
              │
              ├─ Yes ──▶ cooperative. Cheap switches, you control the yields,
              │          and peers depend on you actually yielding.
              │
              └─ No, it may block unpredictably ──▶ preemptive. A timer ISR
                         forces the switch. Costlier, and it needs the port to
                         supply context switching and a tick ISR.
```

On a port without the contextual interfaces, inline plus a hand-written state machine is the answer; promoting a task to a lane that isn't there does nothing.

### 4.7 Behaviour worth knowing before you lean on it

Cancelling a task marks it rather than erasing it — the vector entry is dropped in the sweep at the end of `handle_tasks`, not the moment you call `clearInterval`. Don't dereference an id in between.

Tasks are stored by value, so a lambda capturing `this` or a chunk of state is fine, but those captures live until the removal sweep runs.

The scheduler is not re-entrant. Never call `run()` from inside a task; that is what the contextual lanes are for.

A freshly registered interval stamps its last-run time on first consideration, so it fires one full interval later rather than immediately. Catch-up after a stall is capped at three rounds, so a scheduler frozen through an OTA or a deep sleep resumes fresh instead of firing a burst of back-runs — re-register if you need exact counts.

The slot table is `MAX_SCHEDULABLE_TASKS`, 25 by default. Raising it raises every service's worst-case footprint and the factory-reset hook count along with it.

Sorting tolerates 3 ms of clock jitter per comparison. A `millis_now()` that wanders further will visibly reshuffle task order between runs.

### 4.8 Promoting a task to a lane

With contextual execution on, registration and promotion are two steps — you register normally, get an id, then hand that id to a lane:

```cpp
int tid = __task_scheduler.register_task([&](){
    uint32_t i = 0;
    while (1) {
        __i_dvc_ctrl.getTerminal()->writeln((uint32_t)i++);
        __i_cooperative_scheduler.sleep(500);  // yields to peers
    }
});
__task_scheduler.scheduleUnderExecSched(
    &__i_cooperative_scheduler,
    tid,
    TASK_MODE_COOPERATIVE,
    1 * 1024                  // per-task stack, in bytes
);
```

Stack sizing is on you unless the port can measure it. A KiB for cooperative and two for preemptive is a reasonable starting point on ESP8266; raise it if the task touches `pdiutil::string`, parses JSON, or goes through TLS.

Across lanes: `sleep(ms)` and `yield()` are called from inside a cooperative task (the inline lane's `sleep` is deliberately a no-op), a preemptive task can yield voluntarily even though the ISR will preempt it anyway, and any data shared between lanes needs `iMutex` or `iConditionVar`. The inline lane needs neither.

### 4.9 Watching and steering it from the shell

`ps` prints every registered task — pid, owner, state (`R` running, `S` sleeping, `T` stopped, `Z` zombie), priority, nice, policy, rolling %CPU, run count, interval and name — and `ps <sid>` filters by owner. `top` re-renders the same view on a scheduler tick and stops on Ctrl+C. `watch` wraps any other command on an interval.

Signals mirror POSIX: `HUP=1`, `KILL=9`, `TERM=15`, `CONT=18`, `STOP=19`. They are queued on the task and consumed at the top of the next scheduler pass.

```
  kill    [sig] <pid>     one task, by pid          default TERM
  pkill   [sig] <name>    every task with a name    default TERM
  killall [sig] <name>    same, but default KILL
  renice  <nice> <pid>    -20..19, re-sorts on the next tick
```

Root may signal any task; anyone else only tasks owned by their own session. All of it compiles out when auth is off.

Services are driven the same way, one level up:

```
  srvc list             every service: state and task count
  srvc status <name>    that service's state and the pids it owns
  srvc start <name>     CONT every task the service owns
  srvc stop <name>      STOP every task the service owns   (a freeze, not a teardown)
  srvc restart <name>   stop, then start
```

Start, stop and restart need root. Ownership is tracked per service rather than per task name, so renaming a task never breaks service control.

---
## 5. Database Layer

This is where framework configuration goes to survive a reboot. It is not a key-value store, not relational, and not a filesystem — it is a fixed-address, fixed-size, type-safe table store sized for the few kilobytes of EEPROM-emulated flash a small MCU can spare. Every persisted struct from [§3](#3-configuration-system) reaches NVM through it.

It follows the same three tiers as everything else:

```
  service   DatabaseServiceProvider     get_*_table / set_*_table, factory-reset wiring
      │                                 ← the only surface other services touch
  engine    Database + DatabaseTable    registry, address checks, typed get/set/clear
      │                                 ← owns no bytes, just knows who lives where
  port      iDatabaseInterface          raw read, write and checksum at an address
```

The per-table classes are generated from a JSON schema and live in `src/database/tables/`. Nothing there is hand-edited.

### 5.1 Mental model

NVM is one flat byte space owned by the port:

```
0          5                                                                  end
├──────────┼──────────┬──────────┬──────────┬──────────┬──────────┬──── ... ──┤
│ reserved │ Global   │ Login    │ WiFi     │ OTA      │ Gpio     │           │
│  prefix  │  @5      │  @50     │  @150    │  @300    │  @500    │  unused   │
└──────────┴──────────┴──────────┴──────────┴──────────┴──────────┴──── ... ──┘
```

A table occupies `[address, address + sizeof(T) + 5)`, where the five bytes are the framing the port writes — a table id and a checksum. Addresses are fixed in the generated header. The engine's only check at registration time is whether a table overlaps its neighbour or runs past the end of the region. There is no allocator, no relocation, no defragmentation, and that is the whole point.

### 5.2 The port

The device side opens, erases and validates the NVM region and reports its size. It also supplies the three template methods that actually move typed bytes — `saveConfig<T>`, `loadConfig<T>`, `clearConfig<T>`. Templates cannot be virtual, so the base interface documents them as a contract and every port defines them inline in its own header. The singleton is `__i_db`.

### 5.3 The engine

Tables register themselves before `main()` runs. Each generated table inherits an abstract layer whose constructor pushes the instance into a static array, so by the time the database service starts, it already knows every table that exists in this build.

```cpp
class WiFiTable : public DatabaseTable<WIFI_CONFIG_TABLE_ADDRESS, wifi_config_table> {};
extern WiFiTable __wifi_table;
```

The template supplies `boot()` — which registers the address and size with the engine — plus typed `get`, `set` and `clear` that forward to the port.

The engine itself, the `__database` singleton, keeps the registry and enforces three rules when a table registers: the address must sit past the previous table's tail with a two-byte gap, the new tail must fit inside the region, and the table count must stay within `MAX_DB_TABLES`. A table that fails any of them is skipped, which shows up as a table quietly returning defaults. If that happens after a schema edit, the address map is where to look.

### 5.4 The service

`DatabaseServiceProvider` is what the rest of the framework calls. It defines the table globals, each behind the matching `ENABLE_*` flag; it owns the boot sequence; and it exposes one typed `get_*_table` / `set_*_table` pair per persisted struct.

```cpp
__i_db.beginConfigs(__i_db.getMaxDBSize());       // port mounts NVM
__database.init_database(__i_db.getMaxDBSize());  // every table boots and registers

if (AUTO_FACTORY_RESET_ON_INVALID_CONFIGS) {
    __task_scheduler.setInterval([&]{
        if (!__i_db.isValidConfigs()) { clear_default_tables(); __factory_reset.factory_reset(); }
    }, 5000, now);
}
if (CONFIG_CLEAR_TO_DEFAULT_ON_FACTORY_RESET) {
    __utl_event.add_event_listener(EVENT_FACTORY_RESET, [&]{ clear_default_tables(); });
}
```

Services go through those accessors rather than touching `__i_db` or a table global directly. That keeps the dependency one-way: a service depends on the database service, never on the port or on the generated layout.

### 5.5 Where the tables come from

```
  devices/<board>/config/DBTableSchema.json      ← you edit this
        │
        │   DeviceSetup.py -d <board>
        │     └─ CreateDBSourceFromJson.py
        │          └─ JsonToCpp.py
        ▼
  src/database/tables/<TableName>.h              ← generated, never edited
```

Each `defItems` entry names the C++ class, the backing struct alias, the address macro and its value, and the global the framework links against. A generated table is a body-less subclass of the template — all the behaviour lives in the template, so copying an existing entry is genuinely all it takes.

The default map on the ESP ports:

| Table | Address | Backing struct | Present when |
|---|---|---|---|
| Global | 5 | `global_config_table` | always |
| Login | 50 | `login_credential_table` | auth or web server |
| WiFi | 150 | `wifi_config_table` | WiFi |
| OTA | 300 | `ota_config_table` | OTA |
| GPIO | 500 | `gpio_config_table` | GPIO |
| MQTT general | 700 | `mqtt_general_config_table` | MQTT |
| MQTT LWT | 1500 | `mqtt_lwt_config_table` | MQTT |
| MQTT pub/sub | 1700 | `mqtt_pubsub_config_table` | MQTT |
| Email | 2000 | `email_config_table` | email |
| Device IoT | 2600 | `device_iot_config_table` | IoT |

The gaps are growth room. Adding a field to the WiFi struct is free until it reaches `300 - 150 - 5 = 145` bytes; past that, OTA has to move and existing devices need to be reset.

### 5.6 Boot, end to end

```
  static init        every table global is constructed
                     └─ pushes itself into the pre-registration array

  initialize()       __database_service.initService()
                       ├─ port mounts NVM
                       ├─ engine reserves its registry
                       ├─ every pre-registered table boot()s
                       │     └─ address checked, then accepted
                       ├─ optional 5 s validity watchdog → reset on corruption
                       └─ optional listener: factory reset → clear to defaults
```

After that, any service can ask for a table and trust what comes back.

### 5.7 Read and write semantics

Reads are whole-struct — put a `T` on the stack, read into it, change what you need, write it back. Writes land immediately; there is no journal and no commit phase, so on a flash-backed port every `set` is a flash write. Batch your changes rather than writing per field.

Structs are `memcpy`'d raw, which is what makes serialisation free and also means an NVM image belongs to the toolchain that wrote it. Nothing about the layout is portable across ABIs.

The engine is single-threaded. If contextual execution is on and two lanes touch the same table, guard it with a mutex.

### 5.8 Factory reset

Two switches shape the behaviour. `AUTO_FACTORY_RESET_ON_INVALID_CONFIGS` runs a five-second validity check and, on a bad checksum, clears the tables and fires the reset — which is how a device recovers from corrupted flash instead of sitting there bricked. `CONFIG_CLEAR_TO_DEFAULT_ON_FACTORY_RESET` makes the reset event write default structs back rather than leaving zeros.

The reset event is public, so any service can hook it to drop its own caches.

### 5.9 Adding a table

Say you want a metrics service to persist a host and a port.

1. Define the struct in `src/config/MetricsConfig.h` following the [§3.4](#34-the-shape-of-a-service-config) shape — POD, fixed-size members, a `clear()` the constructor calls:
   ```cpp
   struct metrics_configs { char host[40]; uint16_t port; };
   const int metrics_config_size = sizeof(metrics_configs) + 5;
   using metrics_config_table = metrics_configs;
   ```
2. Pick an address past the last table's tail: `prev_addr + prev_size + 5 < yours`.
3. Add a `defItems` entry to every board schema that should carry it:
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
4. Regenerate with `python3 scripts/DeviceSetup.py -d <board>`.
5. Add the include, the global and the accessor pair to the database service, guarded by your feature flag, mirroring any existing table.
6. Raise `MAX_DB_TABLES` if you have run out of slots.

Keep schema addresses strictly ascending. The overlap check compares against the most recently registered table, so a table declared out of order slips past it.

Changing a struct changes the on-flash layout, which invalidates the checksum on every device already in the field — they come up and reset to defaults. Bump `CONFIG_VERSION` when you do that, so it reads as a decision rather than an accident.

---
## 6. Service Providers

A service provider is the framework's unit of feature. Each one lives under [src/service_provider/](src/service_provider/), derives from `ServiceProvider`, and is gated by exactly one `ENABLE_*` flag. This section is the per-service reference.

### 6.1 The common shape

```cpp
class XServiceProvider : public ServiceProvider {
public:
    XServiceProvider() : ServiceProvider(SERVICE_X, RODT_ATTR("X")) {}
    bool initService(void* arg = nullptr) override;   // called once by the orchestrator
    bool stopService()                    override;   // optional; base reaps tracked tasks
    void printConfigToTerminal(iTerminalInterface*) override;
    void printStatusToTerminal(iTerminalInterface*) override;
};
extern XServiceProvider __x_service;
```

The global is `__<name>_service`. The constructor hands the base a `service_t` value and a flash-resident name, and the base registers it so `srvc` can find it without knowing what it is.

Background work goes through the base's scheduling wrappers rather than the scheduler directly:

```
  serviceSetInterval(fn, dur, now, prio)      periodic
  serviceSetTimeout (fn, dur, now, prio)      one-shot
  serviceUpdateInterval(id, …)                reschedule in place
        │
        └─▶ same call on the scheduler, with the service's name and owner filled in,
            and the returned id recorded in m_service_task_ids[]
                  │
                  └─▶ that list is what makes `ps` show a sensible name,
                      `pkill <Service>` work, and `srvc stop` freeze the right tasks
```

The tracked list is fixed-size — six slots by default. `stopService` in the base reaps every one of them, so an override is only needed when you also have sockets or buffers to release; call the base at the end when you do write one. If you ever register a task straight on the scheduler, hand its id to `trackServiceTask(id)` so it is not orphaned on stop.

Persisted configuration always goes through the database service accessors, never `__i_db`. Cross-service reactions go through `__utl_event`, never direct calls — that is what keeps the dependency graph one-directional.

The base also offers `signalAllServiceTasks(sig)`, `countServiceTasks(...)` and the task-id iterators; those are what the `srvc` command renders.

### 6.2 Service reference

Ordered the way the orchestrator starts them.

#### 6.2.1 `DatabaseServiceProvider` — `__database_service`

Always present; everything else may need persisted config. It mounts NVM, boots every registered table, optionally polls config validity every five seconds, and listens for the factory-reset event. Fully covered in [§5](#5-database-layer).

#### 6.2.2 `DeviceFactoryReset` — `__factory_reset`

Always present. Polls the flash button for a six-to-seven second hold and fires `EVENT_FACTORY_RESET`. Other services listen for that event to drop their own caches before the reboot. The web portal exposes the same action as a form.

#### 6.2.3 `SerialServiceProvider` — `__serial_service`

Opens the serial port at the configured baud and hooks its input handler into the device event pump. It supplies the default terminal for the CLI, and exposes the JSON apply/append hooks that Device-IoT uses to move sensor payloads across the serial link.

#### 6.2.4 `WiFiServiceProvider` — `__wifi_service`

Configures the access point from the WiFi table, starts a station scan, and keeps a connectivity check running every five seconds. NAPT, when enabled, is switched on by a one-shot scheduled after the station link comes up.

The shell surface is `net ip`, `net scansta` and `net connsta,<ssid>,<pass>`; the portal has a WiFi page whose editability is governed by `ALLOW_WIFI_CONFIG_MODIFICATION`.

It is the framework's main event source:

```
  station connected ──▶ EVENT_WIFI_STA_CONNECTED
  DHCP / static IP  ──▶ EVENT_WIFI_STA_GOT_IP     ─┬─▶ mDNS announces
                                                   ├─▶ TLS cert provisioner mints
                                                   └─▶ services needing a stable address
  link lost         ──▶ EVENT_WIFI_STA_DISCONNECTED ──▶ MQTT, OTA, IoT stand down
  AP client in/out  ──▶ EVENT_WIFI_AP_STA(DIS)CONNECTED
  internet poller   ──▶ EVENT_WIFI_INTERNET_UP / _DOWN
```

#### 6.2.5 `OtaServiceProvider` — `__ota_service`

Polls for a firmware update on the interval stored in the OTA table:

```
  GET /api/fordevice/ota-version?mac_id=…&duid=…      →  { "latest": <ver> }
        │  newer than what's running?
        ▼
  GET /api/fordevice/ota-bin?…&version=<latest>       →  bytes
        │
        ├─ stream straight into the updater                  (default)
        ├─ or download to a temp file first                  (needs storage)
        └─ or hand the URL to the SDK's own updater          (fallback)
```

Requests carry HTTP basic auth and a `pdistack` user agent, and run over TLS when the TLS service is on. Which of the three upgrade strategies applies is a compile-time choice.

#### 6.2.6 `GpioServiceProvider` — `__gpio_service`

Loads the GPIO table, ticks pin modes and values, and refreshes the table every five minutes. Modes are off, digital write, digital read, digital blink, analog write and analog read.

From the shell, GPIO is driven as files under `/sys` — `echo 1 > /sys/class/gpio/5/value`. The portal adds a GPIO page plus an events submenu where a pin condition can be routed to email or an HTTP endpoint.

#### 6.2.7 `MqttServiceProvider` — `__mqtt_service`

Opens the broker connection with a last-will message, subscribes to whatever the pub/sub table lists, and schedules the publish driver. Applications inject and receive payloads by registering publish and subscribe callbacks. The portal has general, LWT and pub/sub forms.

#### 6.2.8 `EmailServiceProvider` — `__email_service`

Loads the email table and, if periodic mail is enabled, schedules the send. The GPIO service uses it for event alerts. The portal page includes a test button.

#### 6.2.9 `DeviceIotServiceProvider` — `__device_iot_service`

Off by default; uncomment `ENABLE_DEVICE_IOT` to build it. The service handles registration and channel setup, and the application supplies the sensor.

```
  boot
   ├─ GET <otp url>      →  { otp, status }
   ├─ GET <config url>   →  { did, token, channelhost, channelport,
   │                          channelread, channelwrite, datarate,
   │                          samplerate, keepalive, reconfig, … }
   ├─ configureMQTT()    →  writes those values into the MQTT tables
   └─ sample loop        →  sampleHook() × samplerate  ──▶ dataHook(payload) ──▶ publish
```

Both URLs are templates that substitute the MAC and the device unique id at request time. The MQTT identity it builds is worth knowing: the client id is a base64 of `mac:<device-mac>` rather than the raw MAC, the username is the device unique id from the IoT table, and the password is the token the server returned. The last-will topic is the read channel with a payload carrying the device id.

Because `configureMQTT()` writes the MQTT tables, Device-IoT's server-supplied broker settings replace whatever the portal had for MQTT when both are in use.

The application registers its sensor by calling `initDeviceIotSensor(...)` with an object implementing init, sample, data and reset hooks. The shell offers `iot setid`, `iot getid`, `iot sethost` and `iot gethost`. [§11.6](#116-deviceiotexample) walks through the example sketch.

#### 6.2.10 `AuthServiceProvider` — `__auth_service`

A session-aware delegator rather than a store. `isAuthorized(user, pass)` verifies against `/etc/shadow` through the user store when that file exists, and against the bootstrap credential row otherwise. `setAuthorized` and `getUsername` read and write the *current* session, so there is no global "logged in" bit — three terminals can be at three different privilege levels at once.

#### 6.2.11 Storage (interface init, no provider)

Storage has no service class. The filesystem is used directly by SSH and SFTP, the user store, every file command, and application code.

`__i_fs` is a dispatcher: an `iFileSystemInterface` that routes each call to a mounted backend by longest-prefix match.

```
  path ─▶ VfsDispatcher ─┬─ /       rootfs   LittleFS, the real flash filesystem
                         ├─ /proc   procfs   read-only, generated on each read
                         ├─ /sys    sysfs    peripherals as read/write files
                         ├─ /dev    devfs    byte-stream nodes
                         └─ /tmp    tmpfs    RAM-backed, lost on reboot
```

Mounting happens during `initialize()`, and the table is five slots by default — exactly what those five backends need. `mount` shows the table at runtime and `df` reports usage per mount. On a RAM-tight port, `/tmp` is the first thing to drop, since it holds file content in the heap.

**procfs** nodes are all `0444` and root-owned; writes fail. `/proc/uptime` gives seconds since boot in the Linux two-number layout, `/proc/version` gives the release and config version. Everything that reads files works on them — `cat`, `head`, `wc`, `grep`, `hexdump`.

**sysfs** is where GPIO lives:

| Node | Mode | Content |
|---|---|---|
| `/sys/class/gpio/<pin>/value` | `0666` | current reading, or the value to drive |
| `/sys/class/gpio/<pin>/mode` | `0666` | `0` off · `1` digital write · `2` digital read · `3` blink · `4` analog write · `5` analog read |

Reads return what the GPIO service last sampled; writes update its config copy, persist it and re-apply the modes. Set mode first, then value — `echo 3 > /sys/class/gpio/4/mode` followed by `echo 500 > /sys/class/gpio/4/value` blinks pin 4 at half-second intervals. Use `echo` with redirection for these; the `fedit` editor works through a temp file and so belongs to real files only.

**devfs** exposes the usual byte streams, all `0666`:

| Node | Reading it | Writing to it |
|---|---|---|
| `/dev/null` | nothing, immediate EOF | discarded |
| `/dev/zero` | zero bytes | discarded |
| `/dev/random`, `/dev/urandom` | random bytes from the device RNG | discarded |

The endless ones are capped per read call — 64 bytes by default — so `cat /dev/zero` finishes instead of pinning the CPU until the watchdog fires. The RNG behind them is hardware on the ESP ports and a micros-seeded xorshift elsewhere, which is fine for filling buffers and not for keys.

**tmpfs** is a real read/write filesystem that happens to live in the heap, so the entire command surface works against it — `mkdir`, `touch`, redirection, `cat`, `cp`, `mv`, `rm`, `chmod`, `chown`. Files carry the creating session's uid and gid and the usual umask treatment, so permissions behave exactly as they do on flash. Budgets are a total byte count, a node count and a path length; `df` reports against the byte budget.

**Permissions** are stored as file attributes and enforced in the dispatcher. Every entry carries type, size, name, ctime, mtime, mode, uid and gid, stamped at creation from the current session with `0644` for files and `0755` for directories, masked by that session's umask.

```
  write family    (write, edit, delete, touch)        ─▶ W bits on the path
  read family     (read, list, search)                ─▶ R bits on the path
  attribute + permission changes                      ─▶ owner or root
  ownership changes                                   ─▶ root only
  create / rename / metadata queries                  ─▶ ungated
        │
        └─ root bypasses all of it; entries with no recorded owner read as root-owned
```

There is a narrow setuid analogue: a privileged scope that suspends those checks between a begin and an end call. The user store brackets exactly one thing in it — reading and writing `/etc/shadow` on behalf of a `su`, `login` or `passwd` running as a non-root session.

Copying, renaming or moving across mounts streams the file chunk by chunk into the destination backend, creating on the first chunk and appending after that; a failure part-way removes the partial destination, and a `mv` is that copy followed by deleting the source. Same-backend operations go straight to the backend's own call. Directories don't cross mounts, and an existing destination is refused.

#### 6.2.12 `WebServer` — `__web_server`

Started with the HTTP server interface and ticked from every pass of `serve()`. With HTTPS on, `initService` sets the certificate, key and — under mTLS — client CA paths, then binds 443 with TLS enabled; otherwise it binds 80. It has its own router, middleware chain, controllers and session handling, all covered in [§8](#8-web-server).

#### 6.2.13 `TelnetServiceProvider` — `__telnet_service`

Binds port 23, accepts a client, and hands its stream to the CLI as a terminal. Everything after that is the shell.

#### 6.2.14 `SSHServer` — `__sshserver_service`

The most expensive service in the framework, and the most capable: a full SSH server with an SFTP subsystem, written on the framework's own crypto.

```
  version exchange
        │
  KEXINIT ──▶ pick host-key algorithm from what we actually hold
        │     curve25519-sha256 · aes128-ctr · hmac-sha2-256 or hmac-sha1
        │
  KEXDH ────▶ sign the exchange hash with the host key
        │
  NEWKEYS ──▶ (RFC 8308 server-sig-algs sent here, so modern clients offer RSA keys)
        │
  USERAUTH ─┬─ publickey  ─▶ blob matched against ~/.ssh/authorized_keys, then
        │   │                signature verified over the session id
        │   └─ password   ─▶ verified against /etc/shadow
        │
  CHANNEL ──┬─ shell      ─▶ a terminal handed to the CLI
            └─ subsystem  ─▶ SFTP
```

Host keys live in `/etc/ssh` alongside `sshconfig`, leaving `~/.ssh` to the user's own client keys. The Ed25519 host key is created on service start if it is missing, which takes milliseconds. RSA is generated only when asked for with `sshkgen t=2,f=b`, because 2048-bit keygen on these parts is measured in minutes.

Both authentication methods are on by default and each can be switched off in `/etc/ssh/sshconfig`, which is created with defaults on first boot. When an attempt fails the server advertises exactly the methods still permitted. [§7.9.1](#791-ssh-authentication) has the operational detail.

Two sessions are served concurrently by default, which is what graphical SFTP clients need — they hold a browse connection open and open a second one to move a file. The SFTP subsystem covers path resolution, stat, directory listing, open/read/write, mkdir, rmdir, remove and rename, which is enough for interactive `sftp`, for editing a remote file in FileZilla or WinSCP, and for `scp -s`.

#### 6.2.15 `CommandLineServiceProvider` — `__cmd_service`

Registers every command and owns the binding between terminals and sessions. Attaching a terminal creates or finds its session and draws the login prompt; each input tick looks the session up from the terminal and makes it current before dispatching. Every in-flight command remembers which session owns it, so one session's half-finished prompt is never handed to another.

Up to three sessions run at once across serial, telnet and SSH, each with its own line buffer, cursor, history position, working directory, umask and identity. [§7](#7-command-line--terminal) is the full CLI reference.

#### 6.2.16 TLS (no provider; transport hookup + cert provisioning)

TLS has no service class either — it lives at the interface and port level, and anything that asks the instance factory for a client gets a TLS one when the flag is on. That single substitution is what puts OTA, MQTT, email, IoT and GPIO posting on TLS without any of them knowing.

BearSSL backs the ESP8266 port and mbedTLS the ESP32 one. Certificates and keys are read from the filesystem at runtime, defaulting to `/etc/http/server.crt`, `/etc/http/server.key`, `/etc/http/client-ca.crt` and `/etc/ssl/ca-bundle.crt`.

Handshakes need more stack than the ESP8266 main context has, so enabling TLS also enables contextual execution and runs the engine on its own cooperative task with a stack sized by `TLS_TASK_STACK_SIZE`.

The bundled outbound client is created with peer verification off so that an encrypted-but-unverified connection works immediately. For production, point it at the CA bundle path and drop that line.

Certificates come from one of two places. On ESP32, the on-device provisioner issues self-signed EC or RSA certs with the SANs you ask for, and `ensureServerCert` creates one only when it is missing — wired to the got-IP event when runtime generation is enabled. Everywhere else, `scripts/GenTlsCerts.py` does the same job with OpenSSL and you upload the result over SFTP.

#### 6.2.17 `UserStoreService` — `__user_store_service`

The user directory, in two files:

```
  /etc/passwd   0644   username:x:uid:gid:home:shell        world-readable — id and groups need it
  /etc/shadow   0600   username:hexhash:hexsalt             SHA-256 of salt‖password, 8-byte salt
```

The API is what you'd expect — look up by name or uid, add a user (writing both files, rolling the passwd row back if the shadow write fails), remove a user, verify a password with a constant-time compare, set a password.

On first start, if `/etc/passwd` is absent, it seeds root from the bootstrap credential row and hashes that password into shadow. The seeding is idempotent, and every boot re-stamps shadow as `0600` in case an older build left it readable.

Verifying or changing a password has to read shadow on behalf of a non-root session, so those two methods — and only those two — bracket the access in the privileged scope described in [§6.2.11](#6211-storage-interface-init-no-provider). `useradd` gives each user their own group by setting gid to uid.

It initialises after the filesystem and before the CLI, and the auth service prefers it whenever shadow exists. That preference is the switch that turns the framework from single-credential auth into a multi-user system.

#### 6.2.18 `SessionManager`

Not a service — a static registry holding one `session_t` per attached terminal, three by default.

A session carries its own line buffer and cursor, history position, autocomplete state, working directory, umask, and — when auth is on — authorisation flag, username, uid and gid.

```
  terminal attaches      ─▶ session created, login prompt drawn
  input arrives          ─▶ session found by terminal, made current, command dispatched
  login succeeds         ─▶ uid, gid cached on the session; umask reset to default
  every filesystem call  ─▶ dispatcher reads that cached uid/gid — one dereference,
                            never a scan of /etc/passwd per file operation
  connection closes      ─▶ session detached
```

SSH attaches its session as soon as user auth succeeds, so authorisation state is anchored to the channel rather than to whichever command runs first.

#### 6.2.19 `MdnsServiceProvider` — `__mdns_service`

The responder from [§2.4.2](#242-mdns-and-dns-sd), running as an ordinary service on raw lwIP UDP. It derives the hostname from the MAC, writes `/etc/hostname`, joins the multicast group when the station gets an IP, and advertises whichever servers this build is running. Responses bundle PTR, SRV, TXT and A so a single query gets everything it needs. `srvc status MDNS` shows what it is announcing.

### 6.3 Init order

The orchestrator starts services in a deliberate order:

```
   1  database        every other service may need persisted config
   2  serial          gives boot messages somewhere to go
   3  wifi            brings the network up
   4  ota             may pre-empt the rest of startup if an update is queued
   5  gpio            can raise alerts once http and email exist
   6  mqtt            may publish boot status
   7  email           used by gpio events
   8  factory reset   last of the always-on set, so every listener is registered
   9  device iot      needs MQTT up
  10  auth            gates everything the CLI and portal expose
  11  storage         filesystem up — SSH depends on it
  12  web server      needs auth and storage
  13  telnet, ssh     need network, storage and the CLI
  14  cmd             last, so `srvc` sees a complete list
```

Two things follow from that. A service started later may call into one started earlier; the reverse is not defined. And a service that needs another one in its *constructor* is relying on static-init order — move the lookup into `initService`.

### 6.4 The event bus

Direct calls are reserved for dependencies that are known to already exist. Anything fan-out goes through `__utl_event`:

| Event | Fired by | Typical listeners |
|---|---|---|
| `EVENT_FACTORY_RESET` | factory reset | database clears tables, IoT drops cache, GPIO resets pins |
| `EVENT_WIFI_STA_CONNECTED` / `_DISCONNECTED` | WiFi | MQTT reconnects, OTA and IoT stand down |
| `EVENT_WIFI_STA_GOT_IP` | WiFi, once the address latches | mDNS, cert provisioner, anything needing a stable address |
| `EVENT_WIFI_AP_STACONNECTED` / `_STADISCONNECTED` | WiFi | captive-portal flows, per-client tracking |
| `EVENT_WIFI_INTERNET_UP` / `_DOWN` | connectivity poller | OTA, IoT, email |
| `EVENT_GPIO_TRIGGER` | GPIO event detector | email, MQTT, HTTP post |
| `EVENT_SERIAL_AVAILABLE` | serial bridge | sketch hooks |
| `EVENT_OTA_*` | OTA | logger, portal status |

Subscribe with `__utl_event.add_event_listener(name, [&](void* e){ … })`, publish with `__utl_event.fire(name, ptr)`.

### 6.5 Writing a new service

Say you want a metrics service.

1. Add `ENABLE_METRICS_SERVICE` to the device config and a `service_t` value behind the same flag.
2. Add the persisted struct ([§3.4](#34-the-shape-of-a-service-config)) and its table ([§5.9](#59-adding-a-table)).
3. Write the provider, scheduling through the base wrappers so the task is named, owned and tracked:
   ```cpp
   MetricsServiceProvider() : ServiceProvider(SERVICE_METRICS, RODT_ATTR("Metrics")) {}

   bool initService(void* arg) override {
       __database_service.get_metrics_config_table(&m_cfg);
       this->serviceSetInterval([&]{ this->tick(); },
                                m_cfg.interval_ms, __i_dvc_ctrl.millis_now());
       return ServiceProvider::initService(arg);
   }
   ```
   That alone makes it visible in `ps`, killable with `pkill Metrics`, and controllable with `srvc stop Metrics`.
4. Include it and call its `initService` from the orchestrator, in the right slot per [§6.3](#63-init-order).
5. Implement the two print hooks so `srvc` has something to show, and add a command or a web page if it deserves one.
6. React to other services through events rather than by calling them.

Two habits keep services well-behaved. Do the real work in `initService`, not in the constructor — at construction time the device globals do not exist yet. And prefer value members or static buffers to heap allocation, since these devices stay up for months at a time.

---
## 7. Command Line / Terminal

The shell is the universal control plane. The same commands are reachable over serial, over telnet on port 23, and over SSH on port 22, with login, history, tab completion, in-place editing and file transfer. One implementation covers all three channels because every source presents itself as an `iTerminalInterface`, and the CLI binds one per session.

Start reading at [src/service_provider/cmd/](src/service_provider/cmd/); the parser lives in `src/utility/CommandBase.h`.

### 7.1 Layered model

```
   serial port        telnet session        ssh channel
        │                   │                    │
        └───────────────────┴────────────────────┘
                            │  iTerminalInterface*
                            ▼
              CommandLineServiceProvider
                ├─ binds a session per terminal
                ├─ line editing, history, completion
                └─ dispatch
                            │
                            ▼
                    CommandBase registry
                ├─ every command self-registers by name
                ├─ parses  cmd opt=val[,opt=val]  or positional args
                └─ supports held options, multi-tick commands, abort
                            │
                            ▼
                   services, scheduler, filesystem
```

### 7.2 The terminal contract

Anything that wants to feed the CLI implements `iTerminalInterface` — byte-level reads and writes overloaded for every primitive type, plus the terminal affordances. Three implementations ship: the serial port, the client object a TCP server hands back for telnet, and the SSH channel wrapper.

When a telnet or SSH client connects, its service calls `useTerminal(client)`, which attaches a fresh session and draws a login prompt on that client alone. Serial holds its slot from boot. Nothing leaks between sessions.

### 7.3 Input sequences

Line editing happens in-process, so the CLI recognises control sequences byte by byte:

| Sequence | Action |
|---|---|
| Enter | submit the line |
| Backspace, Delete | edit |
| ←, → | move within the line |
| ↑, ↓ | walk history (needs storage) |
| Home, End | jump to line start or end |
| Page Up, Page Down | scroll long output |
| Tab | complete, cycling through matching command names |
| Esc | cancel the line; inside `fedit`, open the save/cancel/delete menu |
| Ctrl+C, Ctrl+Z | abort the running command |

A long-running command receives these mid-execution by overriding `executeTermInputAction`.

### 7.4 The command contract

Every command is a `CommandBase`. The base owns parsing; you write `execute()`.

```cpp
struct TempCommand : public CommandBase {
    TempCommand() {
        Clear();
        SetCommand(CMD_NAME_TEMP);      // name, at most 8 characters
        AddOption(CMD_OPTION_NAME_T);   // up to 3 options, names up to 3 characters
    }
    const char* getUsage() const override {
        return RODT_ATTR("temp [t=C|F]  read the temperature sensor (default Celsius)");
    }
    bool needauth() override { return true; }
    cmd_result_t execute(cmd_term_inseq_t) override { … }
};
```

`getUsage()` is the single source of truth for that command's help. It is what `help` prints, and what the dispatcher prints automatically whenever the command returns an argument error — so no error branch ever needs to repeat a usage string.

Parsing works like this:

```
   "gpio p=4,m=3,v=500"
      │     └──┬──┘
      │        └─ split on the option separator (',' by default; ';' or ' ' if the
      │           command asks for one), then match each key to a declared option
      └─ command name, up to the first space

   "chmod 0644 /etc/passwd"
      └─ a command that opted into positional args gets them in declaration order
```

Inside `execute`, options come back by name:

```cpp
auto pin = RetrieveOption(CMD_OPTION_NAME_P);
if (pin == nullptr) return CMD_RESULT_ARGS_MISSING;
```

A command that cannot finish in one tick — `watch`, `fedit`, an interactive prompt — returns `CMD_RESULT_INCOMPLETE`. The dispatcher keeps the instance alive, counts the iteration, and re-enters `execute` on the next input.

One thing to know when you do that: a parsed option value points into the live receive buffer, which is gone by the next keystroke. `holdOptionValue("c")` copies the bytes into storage the option owns, freed when the command clears. Any value that has to survive into the next tick needs it.

### 7.5 Result codes

| Result | Meaning | Dispatcher's response |
|---|---|---|
| `OK` | done | blank line, options cleared |
| `INCOMPLETE` | keep me alive | nothing cleared; wait for more input |
| `ARGS_ERROR`, `ARGS_MISSING`, `INVALID_OPTION` | bad usage | error line plus the command's usage string |
| `NOT_FOUND`, `INVALID` | no such command, or unparsable | error line |
| `NEED_AUTH`, `WRONG_CREDENTIAL` | login required or failed | back to the login flow |
| `ABORTED` | Ctrl+C or Ctrl+Z | error line, iteration stops |
| `FAILED` | the command's own failure | error line |
| `TERMINAL_*` | terminal-side states | handled by the service |

### 7.6 The dispatcher

The service owns the list of in-flight commands and the history file path. Everything else — receive buffer, cursor, history position, completion index, working directory — lives on the session. Each input tick resolves the session from the terminal, makes it current, and then runs the command.

History is persisted only when storage is available, in a file capped at 25 lines. Completion walks the command registry for names matching the typed prefix and cycles on repeated Tab, and works with or without storage because the registry is in RAM.

### 7.7 Built-in command inventory

| Command | Options | Brief |
|---|---|---|
| ls [\<dir>] | | List with mode, owner, group, mtime and size. No arg lists the current directory; relative paths join it. Owner and group show names, falling back to numbers. e.g. **ls**, **ls /proc** |
| mkdir \<dir> | | Create a directory, `0755` masked by umask, owned by the session. e.g. **mkdir /home/scripts** |
| touch \<file> | | Create empty at `0644` masked by umask, or bump mtime if it exists. e.g. **touch /home/notes.txt** |
| mv \<src> \<dst> | | Move or rename, across mounts if needed. e.g. **mv /home/a.txt /home/b.txt** |
| cp \<src> \<dst> | | Copy a file, across mounts if needed. e.g. **cp /home/a.txt /home/b.txt** |
| pwd | | Print the working directory. |
| rm \<path> | | Remove a file or directory; needs write permission. e.g. **rm /home/notes.txt** |
| cat \<file> | | Print a file; needs read permission. e.g. **cat /proc/uptime** |
| echo \<text> [> \<file>] | | Print text, or write it to a file with `>` — a single write, so it works on synthetic nodes too. e.g. **echo 1 > /sys/class/gpio/5/value** |
| fedit \<file> | | Scrolling in-place line editor. A status bar shows the path; ←/→/Home/End/Backspace edit the active line, ↑/↓ move through the file, Enter splits at the cursor. Esc opens the menu: **!w** save, **!c** cancel, **!d** delete line. Edits stream to a temp copy and commit on save. e.g. **fedit /home/notes.txt** |
| head \<file> [N] | | First N lines, default 10, in constant memory. |
| tail \<file> [N] | | Last N lines, default 10, in constant memory. |
| wc \<file> | | Lines, words, bytes. |
| df | | One row per mount: total, used, free. |
| mount | | The mount table: prefix, type, backend. |
| chmod \<octal> \<path> | | Set permission bits; owner or root. e.g. **chmod 0644 /etc/passwd** |
| chown \<uid>[:\<gid>] \<path> | | Change owner, root only; gid defaults to uid. e.g. **chown 1001 /home/alice** |
| umask [\<octal>] | | Show or set this session's umask, default `0022`. |
| hexdump \<file> | | Offset, sixteen hex bytes, ASCII. |
| grep \<pattern> \<path> | | Search a file or directory tree, printing `path:line:col:content`. Regex subset: `.` `*` `+` `?` `^` `$` `[abc]` `[a-z]` `[^abc]` and escapes. e.g. **grep ^ERROR /home/log.txt** |
| cls | | Clear the screen. |
| cd \<dir> | | Change directory; `~` and `-` work. |
| login | u=, p= | Interactive login, or inline with both options. |
| logout | | End the session — serial returns to the prompt, telnet and SSH close. |
| whoami | | The session's username. |
| id | | `uid=N(name) gid=N`. |
| who | | Active sessions across all channels: user, tty, sid, login time, idle. |
| groups | | The current user's primary group. |
| su u=\<user> p=\<pass> | u, p | Switch user in this session, prompting when arguments are omitted. On success the identity, uid, gid and home directory all follow. |
| passwd p=\<curr> n=\<new> c=\<confirm> | p, n, c | Change your own password; prompts in three echo-suppressed phases when arguments are omitted. |
| useradd u=\<user> p=\<pass> | u, p | Root only. Next free uid, gid equal to uid, home `/`. Writes both user files. |
| userdel u=\<user> | u | Root only. Removes from both files; refuses root and self. |
| srvc list \| status \| start \| stop \| restart | positional | Service supervisor. `list` shows state per service, `status <name>` adds tracked pids and that service's own detail, and start/stop/restart signal every task it owns. Root for the last three. e.g. **srvc status MDNS** |
| ps [\<sid>] | | Scheduler tasks with owner, state, %CPU, run count, interval and name; optional owner filter. |
| top | i=, n=, u= | The `ps` view on a repeating tick — interval, iteration bound, owner filter. Ctrl+C stops it. |
| kill [\<sig>] \<pid> | | Signal a task: 9 KILL, 15 TERM, 18 CONT, 19 STOP. One argument is a pid, two are signal then pid. |
| pkill [\<sig>] \<name> | | Same, matched by name across every task carrying it. |
| killall [\<sig>] \<name> | | Same as `pkill`, defaulting to KILL. |
| renice \<nice> \<pid> | | Change nice, -20..19, and re-sort immediately. |
| sshkgen t=\<algo>[,f=\<dir>] | t, f | Generate an SSH key pair. `t=1` Ed25519, `t=2` or `3` RSA. Without `f` it prompts for the directory — **a]** `~/.ssh` for client keys, **b]** `/etc/ssh` for host keys, the default — and an explicit path also works. Ed25519 is instant; RSA takes about a minute on ESP32 and six on ESP8266. e.g. **sshkgen t=1**, **sshkgen t=2,f=b** |
| net \<option> | ip, scansta, connsta | Network state and control. e.g. **net connsta,\<ssid>,\<password>** |
| host \<name> | | Resolve a name: IP literal, then `/etc/hosts`, then DNS. |
| ping \<host> [count] | | ICMP echo, default four packets and at most ten, streaming each reply and finishing with a loss and rtt summary. |
| date [-u] [-n] [-s \<epoch>] [+\<fmt>] | | Show or set the clock. `-u` for UTC, `+fmt` for a custom format, `-s` to set, `-n` to force an NTP resync. |
| tdctl | | Clock status: local and universal time, zone, sync state, server. |
| reboot | | Reboot. |
| watch | c=, i=, n= | Run a command repeatedly. Options are separated by `;` so the inner command may contain commas. e.g. **watch c=net ip; i=3000; n=10** |
| iot \<option> | setid, getid, sethost, gethost | Device unique id and IoT host. |
| help | | Every registered command with its usage line. Works before login. |
| uptime | | `up Xd Yh Zm Ws`. |
| tls q=1,t=,l=,n=,i= | | On-device certificate generation, ESP32 with cert generation enabled. e.g. **tls q=1,t=0,l=256,n=device.local,i=192.168.1.50** |
| elfload \<path> | | ESP32 only. Load an ELF from the filesystem and run it as a background task, returning its pid. See [§7.12](#712-dynamic-app-loading-esp32). |

Path arguments behave the POSIX way everywhere: a leading `/` is absolute, anything else resolves against the session's working directory, and `cd` also takes `~` and `-`.

On argument style: commands with at most two arguments take them positionally, which is both shorter to type and what muscle memory expects — `chmod 0644 /etc/passwd`, `renice -5 10`. Where a leading argument is optional, the count disambiguates: one argument is the target, two are signal then target. Named `x=y` options are kept for commands with many optional slots, for anything that takes a password, and for patterns where the optional argument sits in the middle.

### 7.8 Multi-terminal session lifecycle

Three sessions can run at once, one per channel, with fully independent state. There is a single dispatcher and an array of session slots; each slot holds its own line buffer, cursor, history and completion position, working directory, umask, and identity.

```
  boot ──▶ filesystem up
        ──▶ user store bootstraps /etc/passwd and /etc/shadow if absent
        ──▶ serial terminal attached, login prompt drawn

  telnet connect ──▶ accept ──▶ useTerminal(client)
                                  ├─ session attached to the next free slot
                                  └─ login prompt on that client only
                  ──▶ each tick: find session by terminal, make current, dispatch
                  ──▶ disconnect: session detached, slot freed

  ssh connect    ──▶ user auth succeeds ──▶ session attached and marked authorised
                  ──▶ channel opens     ──▶ useTerminal, prompt
                  ──▶ each tick: same session-scoped path
                  ──▶ close: session detached
```

Two invariants carry the whole design. First, the current session is switched once per tick, at the top of input handling, and the static terminal pointer moves with it — so prompt drawing, new command instances and the auth delegators all see the right session without anyone passing it around. Second, every in-flight command records which session created it, and the lookups that find waiting commands filter on that. A telnet login prompt waiting for a username cannot be fed by SSH keystrokes.

Long-running commands capture their terminal and owner when they start, so their output keeps flowing to the right session no matter what the other sessions are doing.

### 7.9 SFTP and SCP file transfer

The SSH service opens an SFTP subsystem on demand, and the same handlers serve `scp -s` for single files, interactive `sftp`, and graphical clients like FileZilla and WinSCP — including editing a remote file in place, which works because the session pool serves the second connection those clients open.

```
scp -s <local-file>  pdiStack@<device-ip>:<remote-path>     # upload
scp -s pdiStack@<device-ip>:<remote-path>  <local-file>     # download
sftp -P 22 pdiStack@<device-ip>                             # interactive
```

| Operation | What you type | Purpose |
|---|---|---|
| REALPATH | `pwd`, `cd` | resolve `.`, `..` and relative paths |
| STAT, LSTAT, FSTAT | `ls`, `stat` | size, type, attributes |
| OPENDIR, READDIR, CLOSE | `ls` | directory listing, 16 entries per response |
| OPEN, READ, WRITE, CLOSE | `get`, `put` | file transfer |
| MKDIR, RMDIR | `mkdir`, `rmdir` | directories |
| REMOVE | `rm` | delete |
| RENAME | `rename` | rename without overwriting |

Data records arrive small — the SSH crypto window caps them at 256 bytes — and stream straight to the filesystem, so a transfer never needs a whole-file buffer. Combined with flash write costs when overwriting, throughput lands between roughly 0.2 and 1 KB/s, which is why firmware goes over OTA rather than SFTP.

Directory listings are read once when the directory opens and paginated across responses, released when it closes. One handle is tracked per session, which is what an interactive client uses. Idle SFTP sessions are reaped after a minute so a suspended client cannot hold a pool slot; interactive shell sessions are never idle-reaped, because a person may sit at a prompt for as long as they like.

#### 7.9.1 SSH authentication

Password and public key are both accepted, and `/etc/ssh/sshconfig` decides which are offered:

```
# /etc/ssh/sshconfig
PasswordAuthentication yes
PubkeyAuthentication   yes
```

Passwords are checked against `/etc/shadow` — the same credentials as serial and telnet. Public keys are Ed25519 or 2048-bit RSA, listed one per line in the device's `~/.ssh/authorized_keys` in the standard OpenSSH format, which is exactly what you copy out of your own `id_ed25519.pub`:

```
fedit ~/.ssh/authorized_keys        # paste the line, then !w
```

Set either option to `no` to switch that method off. With passwords off, only a holder of an authorised private key gets in:

```
ssh -i ~/.ssh/id_ed25519 pdiStack@<device-ip>
```

The device's own host keys are separate, in `/etc/ssh`. Ed25519 lives in `/etc/ssh/ed25519` with its `.pub` and `.seed`, and is created automatically the first time the SSH service starts. RSA lives in `/etc/ssh/rsa` and is generated only when you ask for it with `sshkgen t=2,f=b`, since 2048-bit keygen costs about a minute on ESP32 and six on ESP8266. Either way the key goes onto the wire in standard SSH format during the handshake. `authorized_keys` is one file under the device home directory, shared by all users.

### 7.10 Background commands and Ctrl+C

`watch` and `top` run as scheduler tasks behind a shell session, and they are the template for any long-running command:

```
  1  parse arguments, holdOptionValue() anything that must outlive this tick
  2  register a scheduler task under the command's name, so it shows up in ps
  3  set m_runinbackground and return INCOMPLETE, so the instance stays alive
  4  override stopRunningInBackground() to remove that task and clear the flag
```

Ctrl+C handling is generic: the dispatcher walks its command list and calls `stopRunningInBackground()` on every backgrounded command owned by the current session. A new background command inherits that behaviour with no wiring in the shell. `ps` shows what is currently running, under the command's own name.

### 7.11 Adding a command

Say you want `temp`.

1. Add the name — `#define CMD_NAME_TEMP "temp"` — keeping it within eight characters.
2. Write the command:
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
           m_terminal->writeln(readSensor(celsius));
           return CMD_RESULT_OK;
       }
       static void* Registrar(void*) { static TempCommand cmd; return &cmd; }
   };
   ```
3. Include the header in the CLI service and register the name against the registrar, next to its siblings.

That is the whole job. Tab completion, history, help, argument-error usage printing and Ctrl+C all come from the base.

A few limits shape command design. Names are capped at eight characters and options at three, each name up to three characters, so a verb that wants more either splits into sub-commands or takes positional arguments. Option values cannot contain the separator, an `=`, or spaces, since there is no quoting — pick a separator that doesn't collide with your payload, which is why `watch` separates on `;`. And `needauth()` is the only permission gate, so put it on anything that changes state.

### 7.12 Dynamic app loading (esp32)

With `ENABLE_PROGRAM_EXEC`, `elfload` reads a relocatable ELF off the filesystem, resolves its external symbols against the running firmware, and launches its `main()` as a background preemptive task:

```
elfload /apps/hello.app.elf
```

The command returns to the prompt immediately with a pid. The app runs concurrently and its output arrives asynchronously; `ps` lists it and `kill <pid>` ends it. It also ends when `main()` returns, and the image is freed on either path. Because it is a task rather than a scheduler entry, stop and continue don't apply to it.

The path goes through the normal VFS, so apps arrive by SFTP or HTTP upload like any other file. The feature is ESP32-only, needs storage, and brings contextual execution with it.

The program must be a small position-independent ELF built against the loader — not an ESP-IDF firmware image — calling only symbols the firmware exports; the common libc entries are already there. Build one from Espressif's template with the IDF environment active:

```bash
idf.py create-project-from-example "espressif/elf_loader=*:build_elf_file_example"
cd build_elf_file_example        # edit main/main.c, keeping int main(int argc, char *argv[])
idf.py set-target esp32          # plain esp32, matching the loader config
idf.py elf                       # → build/hello_world.app.elf
```

What makes the output loadable is two lines in the project's top-level `CMakeLists.txt`: `include(elf_loader)` and `project_elf(<name>)`. Upload the result and run it.

---
## 8. Web Server

The web server is the on-device admin portal. With `ENABLE_HTTP_SERVER` on, the device serves HTTP/1.1 on its access-point address — `192.168.0.1` by default — so a phone or laptop can log in, configure WiFi, drive GPIO, watch a dashboard, upload files and manage MQTT, OTA, email and IoT without a cable. Adding `ENABLE_HTTPS_SERVER` moves the same portal to 443 over TLS; no controller, page or route changes, only a different `begin()` call and a certificate on disk.

It is the largest subsystem in the framework, but it is built from a handful of pieces: an orchestrator, a route registry, a middleware check, a resource provider that owns the response buffer, one controller per feature, and HTML fragments in flash.

Start reading at [src/webserver/](src/webserver/).

### 8.1 Layered model

```
   iHttpServerInterface           bytes in and out, route registration
        │  on(uri, cb) · arg() · header() · send()
        ▼
   HttpServer                     owns one controller per feature, each flag-gated
        │  boots every controller once
        ▼
   Controllers                    home · login · dashboard · wifi · ota · gpio
        │                         mqtt · email · iot · storage
        ├─ register_route(uri, fn, middleware, redirect) ──▶ RouteHandler
        │                                                       │
        │                                                   Middleware
        │                                                    ├─ none
        │                                                    └─ auth ─▶ session cookie check
        │
        └─ build the response ──▶ WebResourceProvider
                                    ├─ the page buffer and the server pointer
                                    ├─ shortcuts into the database service
                                    └─ HTML fragments in flash: header · page · footer
```

A request enters through the port, which matches the URI to a registered lambda. The lambda runs its middleware; on a pass the controller method runs, pulls config from the database service, composes HTML from the fragments and sends it. On a fail the middleware sends a redirect to `/login` and the controller never runs.

### 8.2 The orchestrator

`HttpServer` is a service provider that owns its controllers as value members rather than as singletons. Home, dashboard and login are always present; the rest appear with their feature flag — OTA, WiFi, GPIO, MQTT, email, IoT and storage.

`initService` stashes the server pointer, hands it to the resource provider, and walks the controller registry calling `boot()` on each. Every controller put itself into that registry when it was constructed, so registration needs no wiring.

The last thing `initService` does is bind the listener, and that is the only place TLS shows up:

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

Nothing else in the subsystem is conditional on TLS. The per-tick `handle_clients()` is a single call into the port; all the work happens inside the route lambdas.

### 8.3 Routes

URIs are constants in one header, so a typo is a compile error rather than a dead link.

| Route | Purpose | Gate |
|---|---|---|
| `/` | menu landing | — |
| `/login` | form and its POST | — |
| `/logout` | drop the session | auth |
| `/login-config` | change username and password | auth |
| `/dashboard`, `/listen-dashboard` | live device summary and its updates | auth |
| `/wifi-config` | station and AP form | auth |
| `/ota-config` | host, port, version | auth |
| `/email-config` | SMTP credentials | auth |
| `/gpio-manage`, `/gpio-server`, `/gpio-config`, `/gpio-write`, `/gpio-event` | GPIO panel and its subforms | auth |
| `/gpio-monitor`, `/listen-monitor` | live analog graphs | auth |
| `/mqtt-manage`, `/mqtt-general-config`, `/mqtt-lwt-config`, `/mqtt-pubsub-config` | MQTT forms | auth |
| `/device-register-config` | IoT registration | auth |
| `/storage`, `/storage-fileupload`, `/storage-filelist`, `/storage-filedel` | file browser and upload | auth |

Registering one takes a URI, a callback, an optional middleware level and an optional redirect target. There is also a hook for the not-found handler, which the home controller owns.

### 8.4 Controllers

A controller inherits the base — which registers it — holds pointers to the resource provider and the route registry, and implements `boot()`:

```cpp
class LoginController : public Controller {
public:
    LoginController() : Controller("LoginController") {}

    void boot() override {
        m_route_handler->register_route(WEB_SERVER_LOGIN_ROUTE,
                                        [&] { this->handleLoginRoute(); });
        m_route_handler->register_route(WEB_SERVER_LOGOUT_ROUTE,
                                        [&] { this->handleLogoutRoute(); });
        m_route_handler->register_route(WEB_SERVER_LOGIN_CONFIG_ROUTE,
                                        [&] { this->handleLoginConfigRoute(); },
                                        AUTH_MIDDLEWARE);
    }
};
```

Ten controllers ship, one per feature area, each owning the routes in its own prefix. The GPIO and MQTT ones are by far the largest, because they carry the most form state.

Since every route ends up in one global registry, URI constants have to be unique — two controllers claiming the same path means the second one wins.

### 8.5 Middleware

The auth level asks the session handler whether the request carries a live session; if it doesn't, the request gets a redirect to the configured target and the route never runs. A route registered without a level is public.

Middleware lives on the session handler rather than as a separate chain, so a new level means extending the enum and adding a branch — routes then opt in by naming it.

### 8.6 Sessions

Sessions are cookie-based. The cookie name and its max age come from the credential table — `pdi_session` and five minutes by default. Logging in derives a token and sets the cookie; each guarded request parses the incoming `Cookie` header against the stored credentials; logging out and every redirect send an expired cookie back.

There is no server-side session table. The token is derived at login rather than rotated, and logout invalidates it on the client.

### 8.7 Composing a response

The resource provider holds the server pointer for the current request, the page buffer, and shortcuts into the database service so controllers never include database headers.

Pages are raw HTML strings kept in program memory and sent in three chunks:

```cpp
m_web_resource->m_server->send(HTTP_RESP_OK, MIME_TYPE_HTML, headerHtml, /*chunked=*/true);
m_web_resource->m_server->send(HTTP_RESP_OK, MIME_TYPE_HTML, bodyHtml,   /*chunked=*/true);
m_web_resource->m_server->send(HTTP_RESP_OK, MIME_TYPE_HTML, footerHtml, /*chunked=*/true);
```

The header and footer are shared by every page; the middle is whichever page is being served. Sending in chunks is what lets a page exceed the 1800-byte single-send buffer, which is why every composed page uses it.

Three helpers carry the dynamic markup: a page builder that turns C structs into form inputs, tables and option lists; a set of tag and attribute constants that keep allocations down; and an inline SVG icon set.

#### 8.7.1 HTTPS wiring and certificates

The same server implementation runs in TLS mode, with responsibility split like this:

```
  HttpServer::initService     sets cert, key, optional client CA, then begin(443, secure)
        │
  iHttpServerInterface        adds the TLS setters and the secure flag; they are no-ops
        │                     in a non-TLS build, so existing ports compile untouched
        ▼
  HttpServerInterfaceImpl     listener switches to the TLS server instance; each accepted
        │                     client still looks like an ordinary client to the parser
        ▼
  port TLS backend            BearSSL on esp8266, mbedTLS on esp32 — loads PEM from the
                              filesystem at the configured paths
```

| Path | Default | Purpose |
|---|---|---|
| server certificate | `/etc/http/server.crt` | PEM, may be a chain |
| server key | `/etc/http/server.key` | PEM, EC or RSA |
| client CA | `/etc/http/client-ca.crt` | only under mTLS |

Upload those over SFTP after first boot and reboot; the listener picks them up on the next start. The directory is created for you.

Certificates come either from the on-device `tls` command on ESP32 — optionally minted automatically the first time the device gets an IP — or from `scripts/GenTlsCerts.py` off-device.

One header is worth a decision rather than a default: `Strict-Transport-Security` is sent only when its max-age is non-zero, and it ships as zero. Turn it on once you have a CA-signed certificate. With a self-signed one, the browser will pin HTTPS and refuse the click-through until the pin expires.

### 8.8 Request lifecycle

```
  client request
      │
  handleClient()                parse method, URI, headers, args
      │
  route lambda                  the one Controller::boot() registered for this URI
      │
  middleware                    auth? no live session → 301 to /login, stop here
      │
  controller method             read args, load config, on POST validate and persist,
      │                         then send header + page + footer
      ▼
  send(code, mime, body, chunked)  ──▶  bytes back to the client
```

### 8.9 Three routes worth tracing

**`/wifi-config` POST.** Auth passes, the controller reads the station and AP arguments, loads the current WiFi table so untouched fields survive, applies the new values, saves, and renders the success page. The actual reconnect is scheduled a tick later — the response has to flush before the radio drops out from under it.

**`/listen-monitor`.** The browser polls; each request reads the analog pin and returns a small JSON object with the pin and its value. That is the whole live-graph mechanism.

**`/storage-fileupload`.** The controller reads the multipart body through the server's chunked API and writes it to the filesystem as it arrives, so upload size is bounded by flash rather than by RAM.

### 8.10 Adding a page

For a `/metrics` page:

1. Add the route constant next to the others.
2. Add `MetricsPage.h` with the body HTML as a program-memory constant.
3. Write the controller:
   ```cpp
   class MetricsController : public Controller {
   public:
       MetricsController() : Controller("MetricsController") {}
       void boot() override {
           m_route_handler->register_route(WEB_SERVER_METRICS_ROUTE,
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
4. Add it as a value member of `HttpServer`, behind a flag if it is optional, and include its header. Nothing else — the base constructor registers it.
5. Add a menu entry so people can reach it.

Two habits keep the portal responsive. Pass page strings straight to `send()` rather than copying them into RAM; it reads flash directly. And keep controller methods short — `handle_clients()` runs on every pass of the main loop, so a route that needs real work should schedule it and return immediately rather than blocking the loop while it runs.

---
## 9. Logger

The logger is deliberately the smallest interesting subsystem in the framework: four levels, a printf-style formatter, and one shared object. It splits along a single axis — console lines go to the serial terminal, syslog lines go to the terminal *and* a file, and optionally to a remote collector. The interesting part is how it disappears. When a level's gate is off, every call site for that level compiles to nothing at all.

Start reading at [src/interface/pdi/impl/log/LogManager.h](src/interface/pdi/impl/log/LogManager.h).

### 9.1 One logger

`__log_manager` is the single logger for the whole stack on every port. Its contract is two methods — initialise with an I/O interface, and log a level with a format string. It holds exactly one pointer: the terminal it writes to.

The device port does not implement a logger. Its only job is to expose its serial terminal, which the stack wires in during boot:

```cpp
__log_manager.init(__i_dvc_ctrl.getTerminal());
```

That happens inside `initialize()`, so there is no lifecycle call to remember.

For a syslog line, the manager formats it, echoes it to the console, and hands it to whatever sink is registered — the syslog service, which owns the file and the network. The manager itself never touches either. With no sink registered, a syslog line is simply a console line.

### 9.2 Levels

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

### 9.3 Flags

| Flag | Effect |
|---|---|
| `ENABLE_CONSOLE_LOG_INFO` / `_ERROR` / `_WARNING` / `_SUCCESS` | enables that level's console macro |
| `ENABLE_CONSOLE_LOG_ALL` | all four |
| `ENABLE_SYSLOG_SERVICE` | the file sink behind `SysLog*`; needs storage, and is defined inside that guard so it is simply absent on ports without a filesystem |
| `ENABLE_SYSLOG_FORWARD` | also ships each line to a collector; needs syslog and network, and stays idle until a host is configured |

With every console gate off and syslog off, the build is silent: nothing on serial, nothing on flash. When syslog is off, `SysLog*` degrades to the matching console macro, so call sites never need an `#ifdef`.

### 9.4 How a level disappears

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

### 9.5 Flash strings and the formatter

Every macro wraps its format string in `RODT_ATTR`, which keeps the literal in flash rather than RAM — a few hundred call sites then cost flash they were going to occupy anyway and no RAM at all. The manager copies the format into a short-lived buffer before walking it.

The formatter is the framework's own and understands `%d`, `%u`, `%x`, `%c`, `%f` and `%s`. There are no width, precision or length modifiers, so cast a 64-bit value down before logging it. Since every message goes through the formatter, a literal percent sign needs escaping — write `%%`, or pass the text as a `%s` argument.

Console output streams to the terminal character by character with no line buffer, which is what keeps it usable on the RAM-starved boards.

### 9.6 Where syslog goes

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

Remote forwarding builds the datagram directly on the UDP interface, with no vendor syslog library: `<PRI>timestamp hostname pdi: message`, where PRI comes from the level under the `local0` facility, the timestamp comes from NTP once synced, and the hostname is the device address. The collector host is resolved through the name resolver once the station has an IP, and `srvc status syslog` shows the target, whether it resolved, and the socket state. File writing and forwarding are independent — files work with no network at all, and forwarding rides on top when there is one.

Two things follow from the design. Levels are compile-time, so turning `LogI` back on means a rebuild. And the logger shares the serial terminal with the shell, so on a single-UART board their output interleaves — which is exactly what you want while watching a device boot.

---
## 10. Transports

A transport speaks a wire protocol on a byte stream. It sits between the `iClientInterface` the device port provides and the service that deals in payloads. All three — HTTP, MQTT, SMTP — share the same shape: take a client, speak the protocol on it, hand the parsed result back. None of them knows about the device, owns a schedule, or keeps global state beyond its own buffers.

```
   service          knows what to do, when to retry, what to persist
      │  hands over an iClientInterface* it owns
      ▼
   transport        knows how to speak the protocol; parses bytes, nothing else
      │
      ▼
   iClientInterface  plaintext TCP or TLS — the transport can't tell
```

That split buys three things. The same HTTP parsing serves both the client and the portal. A port that ships an SDK-native MQTT client can present it as an `iClientInterface` and the framework's own client falls away without any service noticing. And because a transport is plain C++ over a stream, it can be exercised against a memory-backed mock with no radio involved.

Live in [src/transports/](src/transports/). There are no globals — each consumer creates an instance.

| Transport | Class | Used by | Speaks |
|---|---|---|---|
| HTTP | `Http_Client` | OTA, IoT, GPIO posting, and the portal indirectly | HTTP/1.1, and 1.0 |
| MQTT | `MQTTClient` plus the message builders | MQTT service, IoT service | MQTT 3.1.1 |
| SMTP | `SMTPClient` | email service | SMTP with AUTH LOGIN |

### 10.1 HTTP

Two POD records carry request and response metadata; the class is the state machine between them.

```cpp
Http_Client http;
http.Begin();
http.SetClient(__i_instance.getNewTcpClientInstance());
http.SetTimeout(HTTP_REQUEST_DURATION);   // 10 s
http.SetKeepAlive(true);
http.SetFollowRedirects(true);
http.SetDefaultHeaders(true);             // Host, User-Agent, Connection
http.AddReqHeader("X-Device", mac);

int16_t code = http.Get("http://api.example.com/v1/ping");

char *body; int16_t len;
http.GetResponse(body, len);              // borrowed buffer — copy if you need it later
http.End(/*preserve_client=*/true);
```

Requests are synchronous. A negative return means the transport failed before there was an HTTP status to report — connect, read or timeout.

| Setting | Default | Meaning |
|---|---|---|
| port | 80, or 443 for `https://` | used when the URL omits one |
| version | HTTP/1.1 | request line |
| keep-alive | 30 s | what is advertised to the peer |
| request budget | 10 s | whole request |
| working buffer | 640 B | used during parsing |
| per-chunk read | 1.5 s | inside the parse loop |
| connect retries | 1 | everything above this belongs to the service |

**HTTPS is the same client on a different socket.** The class never opens the connection, so making a request encrypted is a change of factory call and nothing else:

```cpp
iTlsClientInterface* tls = __i_instance.getNewTlsClientInstance();
tls->setCertificateAuthorityPath(TLS_DEFAULT_OUTBOUND_CA_BUNDLE_PATH);
tls->setSNIHostname("api.example.com");
http.SetClient(tls);

int16_t code = http.Get("https://api.example.com/v1/ping");
```

Handshake, verification, record framing and the same response parsing all come along. Worth knowing: the URL scheme is informational. A TLS client with an `http://` URL still travels encrypted; a TCP client with an `https://` URL connects in plaintext on port 443. Pair them deliberately. `setVerifyPeer(false)` keeps the channel encrypted while skipping chain validation, which is right for a self-signed development box and wrong for anything crossing a network you don't own.

When TLS is enabled, the client bundled into `PdiStack` is already the TLS one, so OTA, IoT and GPIO posting go over HTTPS without a line of sketch code.

There is no HTTP *server* class here. The server side lives at the interface layer with a portable default implementation, plumbed through the web server, and `begin(port, secure)` is what flips it into TLS ([§8.7.1](#871-https-wiring-and-certificates)).

### 10.2 MQTT

Split in two by concern: a pure encoder and decoder that turns operations into wire records with no I/O at all, and the client class that owns the connection state machine — handshake, keep-alive, subscriptions, QoS acknowledgement, callback dispatch.

```cpp
MQTTClient mqtt;
mqtt.begin(client, &generalCfg, &lwtCfg);
mqtt.OnConnected(&onMqttConnected);
mqtt.OnData(&onMqttData);              // (args, topic, topic_len, data, len)
mqtt.OnDisconnected(&onMqttDisconnected);

mqtt.InitConnection(host, MQTT_DEFAULT_PORT, /*security=*/0);
mqtt.InitClient(clientId, user, pass, /*keepAlive=*/60, /*cleanSession=*/1);
mqtt.InitLWT(willTopic, willMsg, /*qos=*/1, /*retain=*/0);

mqtt.Connect();
mqtt.Subscribe("ctrl/+/cmd", /*qos=*/1);
mqtt.Publish("sensor/temp", payload, len, /*qos=*/1, /*retain=*/0);
```

`Subscribe` reports success when the packet is enqueued, not when the broker acknowledges it — wait for the subscribed callback before publishing on a topic you just asked for.

Callbacks fire on whichever lane drives the MQTT service, which by default is the inline scheduler. Don't block inside one; schedule the expensive part as a follow-up tick, the way the IoT service does.

The encoder is usable on its own when you need a packet without owning a connection — bind a buffer, build a connect or publish record, and write its bytes to whatever stream you have.

Defaults are port 1883 and a 60-second keep-alive. The service translates its three config tables into the `Init*` calls above.

### 10.3 SMTP

A blocking, command-response client. Each helper issues one verb and waits for the reply code it expects.

```cpp
SMTPClient smtp;
smtp.begin(client, host, port);        // stores parameters; connects on first send

smtp.sendHello(domain);
smtp.sendAuthLogin(username, password);
smtp.sendFrom(sender);
smtp.sendTo(recipient);                // once per recipient
smtp.sendDataCommand();
smtp.sendDataHeader(sender, recipient, subject);
smtp.sendDataBody(body);
smtp.sendQuit();
smtp.end();
```

Lower-level primitives are exposed too — read a response, wait for an expected one, send a command and get its code — for callers that want to drive the protocol themselves.

Every verb blocks on the wire with a five-second timeout, so a flaky link can add up across a single message. That is why the email service schedules sending off the critical path rather than calling it from a request handler. The body goes out as given, so anything that needs encoding gets encoded by the caller.

### 10.4 Helpers

Three small shims sit alongside the transports so service code stays terse. The client helper wraps connect, disconnect, send and read on any client with timeout discipline and chunked I/O — anyone working at the byte level, like the SSH tunnel, uses these rather than the raw interface. The HTTP helper maps status codes, MIME types and methods to strings. The storage helper maps filenames to MIME types for the file commands and SFTP.

### 10.5 Adding a transport

For CoAP, say:

1. Write the client under `src/transports/coap/`, taking an `iClientInterface*` and exposing begin, send, receive and a callback for responses.
2. Keep it free of the database and the scheduler. A transport parses bytes; it does not own time or persistence.
3. Put the service on top, under `src/service_provider/transport/`. That is where config, scheduling and events live.
4. Add one flag, guard the orchestrator wiring with it, and you are done.

Two rules apply to every transport. The client instance belongs to whoever created it — the transport will not delete it, and a leaked socket per request is fatal on a node that stays up for months. And retries belong to the service, which is the layer that knows whether a failure is worth repeating; transports make one attempt.

---
## 11. Examples Walkthrough

[examples/](examples/) has two tracks: one end-user sketch that brings the whole framework up, and a `Dev/` tree of task-focused snippets, one per extension surface. They are deliberately minimal — copy, adapt, ship.

| Example | Demonstrates |
|---|---|
| `PdiStack` | the smallest possible sketch: initialise, then serve |
| `TaskScheduling` | all three task modes, plus rescheduling and cancelling |
| `AddingDatabaseTable` | app-side persistence without touching the codegen |
| `AddingController` | a custom web route behind auth |
| `MqttExample` | configuring MQTT from code and wiring callbacks |
| `DeviceIotExample` | implementing the IoT sensor interface |

### 11.1 `PdiStack`

```cpp
#include <PdiStack.h>

void setup() { PdiStack.initialize(); }
void loop()  { PdiStack.serve(); }
```

That is the entire file. Every service the device config enables brings itself up inside `initialize()`, and `serve()` ticks all of them. This is what you flash to a fresh board.

After flashing you should see the boot banner on serial at 115200, the `pdiStack` access point appear, and the portal at `http://192.168.0.1` accept the default login with every menu live.

Start here. Every other example assumes `initialize()` already ran.

### 11.2 `TaskScheduling`

Four behaviours layered on the bare stack: a one-shot a second after boot, a periodic task at three seconds, a second one-shot at fifteen seconds that changes the periodic task's cadence to one second, and a third at thirty that cancels it. That is the whole registration API exercised in one file.

Behind the contextual-execution flag, two more tasks show the other lanes — each a `while(1) { … sleep(500); }` body promoted onto its own stack:

```cpp
int cooperative_task_id = __task_scheduler.register_task(cooperative_task);
__task_scheduler.scheduleUnderExecSched(&__i_cooperative_scheduler,
                                        cooperative_task_id,
                                        TASK_MODE_COOPERATIVE, 1 * 1024);
```

Turn the console log gates off before running this one, or the framework's own output interleaves with the demo prints and the schedule becomes hard to read.

### 11.3 `AddingDatabaseTable`

The normal database flow goes through the JSON schema and the generator. This example takes the escape hatch: declare a table subclass directly in the sketch and let the same static-init mechanism register it.

```cpp
#if defined(DEVICE_ARDUINOUNO)
#define STUDENT_TABLE_ADDRESS  800
#else
#define STUDENT_TABLE_ADDRESS  2500     // the framework owns everything below this
#endif

struct student_table { student_t students[MAX_STUDENTS]; int student_count; };

class StudentTable : public DatabaseTable<STUDENT_TABLE_ADDRESS, student_table> {};
StudentTable __student_table;           // registers itself at static-init
```

After `initialize()`, the sketch builds a value and calls `set`, and a five-second task reads it back with `get` and prints it.

Three things it teaches. Pick an address above the framework's range — 2500 on the ESP ports, 800 on UNO. Keep the struct POD, fixed-size arrays and scalars only, because it is written to NVM as raw bytes. And accept the trade: a table declared in the sketch never passes through the generator, so tooling that walks generated tables will not see it.

### 11.4 `AddingController`

A `/test-route` behind auth, which is [§8.10](#810-adding-a-page) made concrete:

```cpp
class TestController : public Controller {
public:
    TestController() : Controller("test") {}

    void boot() override {
        m_route_handler->register_route("/test-route",
                                        [&]{ this->handleTestRoute(); },
                                        AUTH_MIDDLEWARE);
    }

    void handleTestRoute() { … }
};

TestController test_controller;        // registers itself at static-init
```

Copy the self-registration, the `strcat_ro` flash-aware string building, and the middleware argument — that one parameter is the whole difference between a private page and a public one.

The example composes its page into a heap buffer to keep the code short. Production controllers stream instead, sending header, body and footer as three chunked calls, which is what [§8.7](#87-composing-a-response) describes and what the shipped controllers do.

### 11.5 `MqttExample`

The pattern for configuring MQTT from code rather than the portal, which is what fleet provisioning needs:

```
  read the three MQTT tables from NVM
        ▼
  overwrite the fields you care about
        ▼
  write them back
        ▼
  register publish and subscribe callbacks
        ▼
  schedule handleMqttConfigChange() ~10 ms out, so setup() finishes first
```

That last deferral is doing real work: it lets the current call stack unwind and pending output flush before the service reconnects. The same shape recurs in WiFi, OTA and IoT.

The publish callback fills a buffer with what to send; the subscribe callback receives topic and payload per inbound message. A `[mac]` token in the client id, the topics or the will message is substituted with the device's MAC at runtime, which gives fleet-wide uniqueness without templating in the sketch.

### 11.6 `DeviceIotExample`

The IoT service is the one contract where the application implements the interface rather than consuming it. The example splits it across a header and a source file — declaring the sensor class and its sample buffer, then implementing init, the sample hook, the data hook that builds the JSON payload, and the reset hook — with the sketch doing only this:

```cpp
DeviceIotSensor sensor;

void setup() {
    PdiStack.initialize();
    __device_iot_service.initDeviceIotSensor(&sensor);
}
```

From there the service drives everything: sampling at the configured rate, building a payload at the publish rate, and pushing it to the channel the server handed out.

It is the only example with a separate `.h` and `.cpp` rather than a single sketch file, because the Arduino preprocessor handles a class with virtual overrides poorly when it lives inline in an `.ino`.

### 11.7 Suggested order

Start with `PdiStack` to confirm the toolchain. Then `TaskScheduling`, because the scheduler is the primitive you reach for as soon as you add behaviour. Then `MqttExample` for the read-modify-write-then-reload pattern, which transfers directly to WiFi, OTA and IoT. After that, take whichever of `AddingDatabaseTable`, `AddingController` and `DeviceIotExample` matches what you are building.

Two practical notes. Run `DeviceSetup.py` before compiling any of them if you are not on the ESP32 default. And most examples stop the build with an `#error` when their feature flag is off — the stock config has them all on, so this only bites after you have trimmed the config for memory.

---
## 12. Memory & Performance Notes

Nothing new here — this collects the constraints that appear piecemeal in earlier sections, so you can size a build before compiling it.

### 12.1 Budget per target

Roughly what is left after the Arduino core, lwIP and the standard library are linked. Orders of magnitude, not guarantees.

| Target | Flash | RAM | NVM | What fits |
|---|---|---|---|---|
| Arduino UNO | 32 KB | 2 KB | 1 KB | serial, EEPROM storage, basic GPIO, shell — no network |
| ESP8266 | 1 MB | ~50 KB free heap | ~4 KB | the full build short of SSH; contextual execution is comfortable |
| ESP32 | 4 MB | ~250 KB free heap | ~4 KB | the full build, with room for a second contextual lane |

That table is why entire feature groups are gated on the board in the device config, and why the table limit is 5 on UNO against 15 on the ESP ports.

### 12.2 Keeping strings out of RAM

Two macros, one purpose:

| Macro | On the ESP ports | Elsewhere |
|---|---|---|
| `RODT_ATTR("text")` | wraps the literal so it stays in flash | plain literal, same behaviour |
| `PROG_RODT_ATTR` | a storage qualifier that puts the variable in flash | empty; the variable is already read-only data |
| `PROG_RODT_PTR` | the right pointer type for reading flash on AVR | a plain `const char*` |

The rule is simple: a literal used inline goes in `RODT_ATTR(...)`, and one held in a named variable is declared `static const char foo[] PROG_RODT_ATTR = "…"`. Every prompt, page fragment, log message and service name in the framework follows it, which is why the binary is dense rather than RAM-hungry.

When a consumer genuinely cannot take a flash pointer, `rofn::to_charptr()` copies the string into a fresh heap buffer and hands it over — and the caller owns it from there. It is an escape hatch, not a default; every call is an allocation.

### 12.3 The expensive features

A handful of choices dominate the budget.

**SSH** is the heaviest single flag. Turning it on effectively commits you to ESP32-class memory.

**TLS** costs about what SSH does in flash, and more than NAPT in heap. On ESP8266 the two cannot coexist — both want more of the same fixed heap than exists. Inbound HTTPS and outbound TLS share the same backend, so enabling both costs nothing extra.

What matters with TLS is per-session, not per-build:

```
   one live TLS session holds
        ├─ a worker task stack        ~6.5 KB
        ├─ record buffers             in + out
        └─ engine state               keys, cipher contexts, cert chain during validation

   esp8266 / BearSSL   10-15 KB per session   on a 30-40 KB working budget
   esp32   / mbedTLS   35-50 KB per session   mostly the 16 KB record buffers
                                              trim them in sdkconfig to halve this
```

Every byte of that comes back when the client disconnects — the worker exits, buffers are freed, state is torn down, though on FreeRTOS the stack is reclaimed by the idle task a moment later. Size for the worst-case number of *concurrent* sessions; an idle build always looks healthy because none of it is allocated yet.

**NAPT** is invisible in flash and expensive in heap, because lwIP holds the translation table. Leave it off unless the device is bridging its AP to the station link.

**The portal's controllers** each carry their own form-validation code, and there are a dozen. If you don't need the portal, drop the HTTP server even while keeping WiFi.

**Contextual execution** is cheap in flash and costs RAM for the lifetime of every task, since each one owns its stack. Two cooperative tasks at a kilobyte each is two kilobytes you don't get back until they exit.

**The containers allocate.** `pdiutil::string` and `pdiutil::vector` hide the heap but still use it, and repeated growth fragments. Reserve up front wherever the size is known, the way the scheduler reserves its task table.

### 12.4 Heap discipline

The framework is built to run for weeks or months between reboots, which shapes a few habits.

Don't allocate after `setup()`. Anything that allocates per request, per tick or per event will fragment eventually. Reserve container sizes at init. Reuse buffers — the portal reuses one scratch buffer, transports reuse a caller-owned client, the MQTT parser uses a static ring. Where a buffer must be held across ticks, hold and then free, the way a held command option is released when the command clears.

For the paths that genuinely can be skipped under pressure — a TLS handshake, a large page composition — use `pdiutil::safe_new<T>()` and `safe_new_array<T>(n)`. They refuse the allocation and return null when the free heap would fall below the configured margin, which lets the caller back out cleanly instead of failing somewhere deep inside a library.

When fragmentation does set in, the symptoms are recognisable: `register_task` returning -1, a transport getting null from an allocation, or a vector failing to grow.

### 12.5 CPU and tick budget

| Step of `serve()` | Typical cost |
|---|---|
| web server client handling | under 1 ms idle, 5-50 ms during a request |
| scheduler | under 1 ms idle; at most one inline task per pass |
| device yield | the vendor SDK's slice, 1-3 ms |
| event dispatch | under 1 ms unless a handler is slow |
| contextual ticks | whatever those tasks do |

Loop frequency runs from hundreds of hertz when idle down to around ten during heavy HTTP or SSH traffic. So anything needing sub-100 ms response belongs on a contextual lane rather than in an inline task, and shortening the WiFi connectivity check below five seconds starts competing with the loop's real work.

### 12.6 Boot profile

```
  power-on
    ├─ static init of every global                 10-50 ms
    ├─ the shared client is constructed             ~1 ms
  setup()
    ├─ database: read every table from NVM        50-200 ms
    ├─ serial and terminal greeting                 ~10 ms
    ├─ WiFi init and station scan               500-3000 ms   ← dominates
    ├─ http, telnet, ssh listeners                  ~50 ms
    └─ shell prompt                                 ~10 ms
  loop()
```

One to four seconds to first prompt, depending mostly on how quickly the station associates.

### 12.7 Choices that pay off

Wrap every literal in `RODT_ATTR` — skipping it breaks nothing and quietly moves a few hundred bytes per file into RAM.

Use `char[]` for anything NVM-shaped and `pdiutil::string` for transient work; mixing them inside one config struct breaks the serialisation contract.

Use explicit-width integer types in config structs. `sizeof(int)` differs between AVR and the ESP parts, and an NVM layout that depends on it is not portable.

Stay away from the `printf` family; the framework's own conversions save four to eight kilobytes by never linking libc's formatter.

Move firmware over OTA rather than SFTP — file transfer runs at 0.2 to 1 KB/s by design ([§7.9](#79-sftp-and-scp-file-transfer)).

### 12.8 Looking at a running device

`ps` gives every scheduler task with its rolling CPU share, run count and interval — the fastest way to spot a service hogging the loop. `srvc list` shows service state and task counts, and `srvc status <name>` drills into one, covering database validity, WiFi state, MQTT connection and the rest. A port that implements the optional stack-measurement hook additionally lets you bracket critical work and read back a high-water mark.

Two behaviours to keep in mind while reading those numbers. The scheduler runs one inline task per pass, so twenty tasks at a 100 ms cadence will pace each other if the loop is turning over ten times a second. And a page send is capped per chunk rather than per response, which is why composed pages go out in three calls.

---
## 13. Portable Interfaces

The interface layer is the contract between the framework and a device. Every type here is abstract: pure virtual methods, no state, no platform headers, depending only on standard types and other interfaces. Each is implemented by exactly one device-side class per build and reached through a single `__i_*` global.

### 13.1 Layout

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

### 13.2 Conventions

Interface types start with a lowercase `i`. Each header forward-declares the concrete class the port will define and, at the bottom, declares the singleton:

```cpp
extern DeviceControlInterface __i_dvc_ctrl;
```

That declaration is the coupling — the port's source file defines the matching variable, and everything above finds it by name.

Composites are built by multiple inheritance rather than by aggregation, which is why device control alone gives you GPIO, watchdog, utility and upgrade. And everything stream-like shares the terminal base, which is what lets the logger, the shell and the web writer target any of them without caring which.

### 13.3 Reference

#### 13.3.1 Core, always required

| Interface | Implemented by | Used by | Key methods |
|---|---|---|---|
| `iDeviceControlInterface` | device | the orchestrator and every service | device init, reset, restart, erase config, device id and MAC, factory-request check, `getTerminal`, `handleEvents`, plus everything it inherits |
| `iDatabaseInterface` | device | the database service and every table | begin, clean, validate, report size, and the typed read/write templates |
| `iInstanceInterface` | device | anything needing a fresh connection | new TCP client and server, new UDP socket, new TLS client and server, filesystem and utility handles |
| `iUtilityInterface` | inherited through device control | scheduler, event bus, logger, `/dev/random` | `wait`, `millis_now`, `micros_now`, `random_now`, `yield`, `log`, optional stack measurement |
| `iIOInterface`, `iTerminalInterface` | any stream | logger, shell, web writers | the write family overloaded for every primitive, timestamps, connect and disconnect |

#### 13.3.2 Drivers

| Interface | Used by | Key methods |
|---|---|---|
| `iGpioInterface` | GPIO service, `/sys/class/gpio` | mode, write, read, pin mapping, exceptional-pin check, blinker create and release |
| `iGpioBlinkerInterface` | GPIO service in blink mode | configure, update, start, stop, running |
| `iWdtInterface` | long-running services, the scheduler | enable, disable, feed |

#### 13.3.3 Middlewares

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

#### 13.3.4 Modules

| Interface | Used by | Notes |
|---|---|---|
| `iSerialInterface` | serial service, logger, shell | derives from the client interface — serial is just another stream |
| `iStorageInterface` | the filesystem, LittleFS | byte-addressable read, write, erase, size |
| `iFileSystemInterface` | SSH, SFTP, every file command | file and directory CRUD, traversal, line and offset lookup, search, custom attributes |
| `iWiFiInterface` | WiFi service, `net` | station and AP, sync and async scan, NAPT, mode |

#### 13.3.5 Optional

| Interface | Implemented by | Notes |
|---|---|---|
| `iLoggerInterface` | the framework, not the device | the device supplies only its terminal; `LogManager` is the sole implementation |
| `iDeviceIotInterface` | the application | sample, data and reset hooks — the one interface whose implementer is your sketch |

That last one is the framework's deliberate extension point for "what to publish, and how often".

#### 13.3.6 Threading, only for contextual execution

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

### 13.4 Shared default implementations

Not every interface is worth rewriting per device. Two portable defaults ship under `impl/`, and both are what a new port should reach for first.

The HTTP server implementation is a protocol-correct HTTP/1.1 server built on nothing but the TCP server and client interfaces — and with TLS enabled, the same file serves HTTPS by wrapping accepted connections. The filesystem implementation is LittleFS on top of any storage interface.

So a new device needs to supply raw TCP and raw storage, and inherits the HTTP server, the HTTPS server and the whole filesystem for free. The TLS classes are deliberately not here, because BearSSL and mbedTLS are different enough that each port supplies its own pair.

### 13.5 What an implementation must promise

Construction is cheap and free of side effects. The singletons are constructed before `setup()` runs, so a constructor must not allocate, touch hardware, or open a port — that work belongs in an init method the orchestrator calls.

Methods do not block unless they say so. Anything that might take more than a few milliseconds — a TCP connect, an NTP fetch, an OTA download — either returns early or exposes a state machine, and the scheduler drives the long form.

Re-entrancy is not assumed. The framework is single-threaded by default; when a port enables contextual execution, the implementation rather than the caller is responsible for guarding shared state.

### 13.6 Adding an interface

The bar is whether at least two devices could implement it differently. If they could:

1. Pick the group — drivers for silicon, middlewares for network and device operations, modules for orthogonal features, threading for execution, top level for cross-cutting concerns.
2. Forward-declare the concrete class and declare the `extern` singleton at the bottom.
3. Guard it with the same flag that gates the service consuming it, so no existing port has to provide anything until it opts in.
4. Add a stub to the mock device so the off-device build still links.
5. Write it up here.

An interface with exactly one implementation is usually a sign the abstraction is premature — keep it in device-specific code until a second port needs it.

---
## 14. Device Layer & Porting Guide

The device layer is the only place vendor SDK and Arduino-core symbols are allowed. Everything above it talks through abstract interfaces. A port is one folder under [devices/](devices/) implementing those interfaces for one MCU family.

### 14.1 What a port contains

```
  devices/esp32/
    esp32.h                     umbrella include for the SDK and core
    esp32_device_config.h       platform macros: flash strings, critical sections
    esp32_pdi.h                 header aggregator — what the framework sees
    esp32_pdi.cpp               source aggregator — see below

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
    TlsClient / TlsServer       with TLS — BearSSL on esp8266, mbedTLS on esp32
    cert loader                 per backend, loads PEM and DER off the filesystem
    TlsCertProvisioner          esp32, with on-device cert generation
    ExceptionsNotifier          optional crash capture

    config/DBTableSchema.json   this board's table layout
    core/                       vendor helpers, e.g. an EEPROM emulator
    threading/                  optional: the cooperative and preemptive lanes
```

The two ends of the spectrum are worth looking at. The mock device is header-only stubs, used when no board is selected so the framework still compiles for analysis or off-device tests. The Arduino UNO port has no network, storage-beyond-EEPROM, or web server at all — device control, database, serial, storage, filesystem and the instance factory, and nothing more.

For threading, ESP32 builds on FreeRTOS primitives while ESP8266 ships bare-metal Xtensa context switching driven by a hardware timer. Both satisfy the same interfaces.

### 14.2 Required versus optional

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
| TLS client and server | with TLS | when on, the orchestrator hands these out instead of plain TCP, and every outbound service upgrades transparently |
| cert provisioner | with on-device cert generation | esp32; free functions rather than a virtual interface |
| threading family | with contextual execution | also required by TLS, which runs off the main stack |
| GPIO and watchdog | always, folded in | implemented as part of device control rather than as separate classes |

A port is valid the moment the always rows compile and link. Everything else arrives as you turn flags on.

### 14.3 The two aggregators

Every port supplies a pair, with a strict split:

```
  <name>_pdi.h     which interface headers the framework can see
                   each include wrapped in its ENABLE_* guard, so unused interfaces cost nothing

  <name>_pdi.cpp   #includes the implementation .cpp files
                   the Arduino build flattens the port into one object file, which means
                   anything marked static there is per-port, not per-file — and those .cpp
                   files must never be included from outside this chain
```

There is an optional C-side aggregator for pure-C translation units, and the umbrella header exists so each per-interface header can pull in the SDK once rather than repeating the plumbing.

### 14.4 How a board gets selected

```
  DeviceSetup.py -d esp8266
        │  writes
        ▼
  devices/DeviceSetup.h        #define DEVICE_ESP8266
        │  included by
        ▼
  devices/DeviceConfig.h       cascades into ENABLE_* flags, and pulls in
        │                      esp8266_device_config.h so the platform macros exist
        │                      before any framework header is parsed
        ▼
  src/config/Config.h          now everything under src/ sees flags and macros
        ▼
  src/interface/pdi.h          picks esp8266_pdi.h
        ▼
  the port's interface headers  which transitively pull in the SDK
```

Adding a board touches exactly three files outside its own folder: the device config cascade, the interface selector, and the architecture list in `library.properties`.

### 14.5 The singletons a port must define

Each port instantiates exactly one object per interface, under the name the framework expects — those names are part of the contract.

| Symbol | Required when |
|---|---|
| `__i_dvc_ctrl`, `__i_db`, `__i_instance` | always |
| `__i_serial` | serial service |
| `__i_storage`, `__i_fs` | storage |
| `__i_wifi`, `__i_http_server` | WiFi |
| `__i_ntp`, `__i_ping` | networking |
| `__i_cooperative_scheduler`, `__i_preemptive_scheduler` | contextual execution |

If the port skips a flag it must also skip the symbol — the name should not exist when its feature is off.

### 14.6 The optional threading port

Four pieces make the contextual lanes work: a scheduler for each lane, a context implementation that saves and restores CPU state, a matching mutex and condition variable, and a periodic tick source to drive preemption.

Leave the layer out and contextual tasks are simply unavailable — inline tasks keep working, because they run on the loop's own stack. [§4](#4-task-scheduler) covers the trade-offs.

Enabling TLS turns this layer on implicitly, because both SSL backends need more stack than the Arduino main context has; the handshake runs on a dedicated cooperative task instead.

### 14.7 Per-board database schema

Each port carries its own table schema describing what lives in NVM on that board, tuned to its capacity. The setup script turns it into C++ table sources. Format is in [§5.5](#55-where-the-tables-come-from).

### 14.8 Porting, step by step

Say the board is `myboard`.

1. **Create the folder** with three files to start: the SDK umbrella header, the platform-macro header, and a copy of an existing board's table schema.
   ```
   devices/myboard/
     myboard.h
     myboard_device_config.h
     config/DBTableSchema.json
   ```
   Add a branch for it in the device-config cascade so those macros are picked up.
2. **Implement the three required interfaces** — device control, database, instance factory — each deriving from its abstract counterpart, each defining its `__i_*` global.
3. **Write the two aggregators**, mirroring an existing board's pair and keeping only what you have implemented.
4. **Register the board** in the interface selector:
   ```cpp
   #elif defined(DEVICE_MYBOARD)
   #include "../../devices/myboard/myboard_pdi.h"
   ```
5. **Add the per-board limits** — pin counts, table count — and switch off any service the board cannot support, the way the UNO port does.
6. **Generate the setup files**: `python3 DeviceSetup.py -d myboard`.
7. **Build the bundled example** for the new board. That is the first real validation.
8. **Add optional interfaces one flag at a time**, rebuilding as you go.
9. **Add TLS** if the board can carry it, by implementing the client and server against whichever SSL stack it has and wiring the factory to hand them out. The two existing ports implement the same contract on different libraries, so either is a usable reference.
10. **Add the threading port** if you want the contextual lanes, or if you turned on TLS. The bare-metal and the RTOS-backed implementations bracket the range of what this can look like.

### 14.9 Before you call it done

- The mock device still compiles — proof that nothing under `src/` picked up a vendor header.
- The bundled example builds with every flag the board can support.
- Every `__i_*` symbol the flag set implies is defined exactly once.
- Microsecond time is monotonic across the platform's counter wrap, and `ps` shows non-zero CPU share for tasks with sub-millisecond callbacks after a few ticks.
- `srvc list` shows every service the build started, and stop and start actually freeze and resume them.
- Reboot and factory reset both round-trip without losing the database.
- With storage on, upload and download over SFTP work.
- With contextual execution on, a task on each lane runs and prints without corrupting a stack.
- With TLS on, the factory returns a live instance and a handshake against a known peer completes.
- With HTTPS on, the portal answers on 443 once the certificate and key are on the filesystem.
- With on-device cert generation on, the `tls` command writes both files where the config says.

---
## 15. Utility Library

[src/utility/](src/utility/) is the foundation: small, dependency-light primitives every layer above uses. Anything here may include the abstract interfaces, but never a device header — the same source file compiles under every port with no conditionals.

Most of these have already appeared in passing. This is the index.

| Component | What it is |
|---|---|
| interface foundations | the three abstract bases the framework rests on: stream I/O, utility, instance factory |
| type definitions | `task_t`, task states and signals, `session_t`, callback aliases, input sequences, addresses |
| database engine | [§5](#5-database-layer) |
| task scheduler | [§4](#4-task-scheduler) |
| command base | [§7](#7-command-line--terminal) |
| event bus | cross-service publish and subscribe |
| string operations | bounded C-string helpers for fixed-size NVM data |
| data conversions | integer, hex and BCD conversions with no `printf` |
| Base64 | encode, decode, and a unique-key generator |
| regex | a minimal engine behind `grep` |
| safe alloc | heap-checked allocation that refuses to breach a margin |
| queues | a byte ring, a record queue, and a length-prefixed parser |
| crypto | hashes, HMAC, AES, Curve25519, Ed25519, RSA |
| PdiSTL | a trimmed standard library for constrained targets |
| umbrella header | one include that pulls the whole foundation in |

### 15.1 Event bus

`__utl_event` is a synchronous publisher. Services add listeners at boot and fire events from state changes, without any of them taking a direct dependency on another. Event names are centralised, and the usage patterns are in [§6.4](#64-the-event-bus).

### 15.2 String operations

Bounded, `printf`-free helpers — substring search, trim, compare, find-and-replace, a small JSON field extractor, IPv4 conversions, case folding. Every one takes an explicit length bound, defaulting to 300, because the NVM config strings are fixed-size arrays that are often not null-terminated. Keep the bound.

### 15.3 Data conversions

Integer, hex and BCD conversions in both directions with no `stdio` dependency. Using these instead of libc's formatter is worth four to eight kilobytes of flash on the smaller targets.

### 15.4 Base64

Encode and decode, plus a unique-key generator that the web session handler uses for cookie tokens and the SSH keygen uses for seeds.

### 15.5 Queues

Three layers, useful when byte-level discipline matters:

```
  RINGBUF        a byte ring — put, get, init over a caller-supplied buffer
      │
  QUEUE          length-tagged records on top of the ring
      │
  PROTO_PARSER   length-prefixed packets with a completion callback
```

MQTT uses the parser to reassemble frames out of a TCP byte stream, which is what they exist for. New code that just needs a container should reach for `pdiutil::vector` instead. One parser belongs to one stream — it carries state across calls.

### 15.6 Crypto

A small, production-quality kit. Everything is plain functions over fixed-size buffers, with no allocation and no hidden global state beyond contexts you own.

| Kind | What's there |
|---|---|
| hashes | SHA-1, SHA-256, SHA-512, each with streaming and one-shot forms |
| HMAC | HMAC-SHA1, HMAC-SHA256 |
| symmetric | AES-128 and AES-256 in ECB, CBC and CTR |
| key agreement | Curve25519, including the bridge from an Ed25519 private key |
| signing | Ed25519, and RSA with a portable big-integer layer and PKCS#1 v1.5 |

The Curve25519 and Ed25519 code comes from the standard portable reference. The RSA and big-integer layer is self-contained and device-agnostic — the caller injects the RNG and a watchdog-yield hook, which is what makes on-device keygen survivable. SSH uses all of it: host key generation, the key exchange, host-key signing, public-key authentication, and AES-CTR for transport encryption.

One property worth knowing before you rely on it: constant-time behaviour holds only where the upstream implementation provides it. Ed25519 verification is constant-time; the table-based AES and the big-integer path are not hardened against timing observation.

### 15.7 PdiSTL

A trimmed subset of the C++ standard library for targets that have no libstdc++, adopted from ArduinoSTL by way of uClibc++. Containers live under `pdiutil::` — `string`, `vector`, `function`, the smart pointers and the usual set — and algorithms under `pdistd::`. The C-library wrappers and the ABI glue come along so the framework builds on toolchains that ship neither.

In framework code, write `pdiutil::string` and `pdiutil::vector` with the namespace spelled out, never a `using namespace`. It keeps a later swap to the host standard library mechanical.

`pdiutil::string` is not `std::string` — some methods are missing or named differently, and the small-buffer and allocator behaviour differ. Check the header before assuming an API is there.

### 15.8 The umbrella header

`utility/Utility.h` is the single include that pulls in the scheduler variant the config selected, the event bus, conversions, string operations, the queues, Base64, I/O and the command base. It is the right include for a file that genuinely consumes the whole foundation, and the wrong one for a file that needs a single helper — it carries twenty-odd headers with it.

---
## 16. Extending the Framework

Every section above has its own "how do I add one of these" part. This one is the index: find what you're building, follow the short version, and jump to the deep dive when you need it.

### 16.1 Which surface fits

| You want to | Reach for | Detail |
|---|---|---|
| support a new board | a device port | [§14](#14-device-layer--porting-guide) |
| add a hardware capability across all ports | a portable interface | [§13.6](#136-adding-an-interface) |
| add a framework-level feature | a service provider | [§6.5](#65-writing-a-new-service) |
| speak a new wire protocol | a transport | [§10.5](#105-adding-a-transport) |
| persist new configuration | a database table | [§5.9](#59-adding-a-table) |
| add a screen to the portal | a controller and a page | [§8.10](#810-adding-a-page) |
| add a terminal command | a command class | [§7.11](#711-adding-a-command) |
| persist something only your sketch cares about | the database escape hatch | [§11.3](#113-addingdatabasetable) |
| react to another service without coupling to it | the event bus | [§6.4](#64-the-event-bus) |
| run periodic or long work | the scheduler | [§4](#4-task-scheduler) |
| encrypt everything outbound | turn on TLS — the orchestrator swaps the shared client and every service follows | [§6.2.16](#6216-tls-no-provider-transport-hookup--cert-provisioning) |
| serve the portal over HTTPS | turn on HTTPS and drop a cert and key on the filesystem | [§8.7.1](#871-https-wiring-and-certificates) |
| call a service from a sketch | the global | [§16.9](#169-calling-a-service-from-a-sketch) |

When a need straddles two rows, the lower layer is usually right. A transport beats a service that hard-codes a protocol; an interface beats a service that reaches for a vendor header.

### 16.2 A new device

Create the folder with its SDK umbrella header, its platform-macro header and the two aggregators. Implement device control, database and the instance factory, defining their globals. Branch on the new `DEVICE_*` in both the device config and the interface selector. Add the per-board limits. Run the setup script, build the bundled example, then add optional interfaces one flag at a time. Finally add the architecture to `library.properties`. Full walk-through in [§14.8](#148-porting-step-by-step), checklist in [§14.9](#149-before-you-call-it-done).

### 16.3 A new interface

Pick the group, write the header with pure virtuals plus a forward-declared concrete class and its `extern` singleton, guard it behind the flag that gates its consumers, and add a mock stub so off-device builds still link. If more than one port would end up writing the same logic, put a default implementation under `impl/` instead.

The bar is that two ports would genuinely implement it differently. One implementation means it belongs in the device folder for now.

### 16.4 A new service

Add the flag, add the `service_t` value behind it, and write the provider:

```cpp
<Name>ServiceProvider() : ServiceProvider(SERVICE_<NAME>, RODT_ATTR("<Name>")) {}

bool initService(void* arg) override {
    // load config, register tasks through the base wrappers, add listeners
    return ServiceProvider::initService(arg);
}
```

Include it in the orchestrator and call its `initService` in the right slot for the init order. Implement the two print hooks so `srvc` can show it. Add a command, a page or a table if it earns them.

### 16.5 A new transport

Write the client under `src/transports/<proto>/`, taking an `iClientInterface*` and exposing begin, send, receive and end. Keep the database and the scheduler out of it. Put the service on top, wire one flag through the orchestrator, done.

That split is what makes a transport reusable from a sketch, or from a second service, without dragging the framework along. HTTP, MQTT and SMTP are the references.

### 16.6 A new table

Two paths, depending on who needs to see it.

A framework table — one that has to round-trip on every port and show up in tooling — means defining the struct in a config header, picking a free address, adding a schema entry for each board that carries it, regenerating, and adding the accessor pair to the database service. [§5.9](#59-adding-a-table) has the detail.

A sketch-local table skips all of that: declare a `DatabaseTable<ADDR, my_struct>` subclass in the `.ino` and let static-init register it, as in [§11.3](#113-addingdatabasetable). It never passes through the generator, so framework tooling will not see it.

### 16.7 A new page

Add the route constant, add a page header holding the body HTML in program memory, write the controller and register the route in `boot()` — with the auth middleware unless the page is deliberately public — then add it as a member of the web server and put a menu entry on the home page. Compose the response as header, body and footer in three chunked sends.

A sketch can add a controller the same way without touching the framework; [§11.4](#114-addingcontroller) shows it.

### 16.8 A new command

Add the name constant, write the command struct with its options and `execute`, then include and register it in the CLI service. Completion, history and Ctrl+C come for free. Keep the name within eight characters and the options within three ([§7.11](#711-adding-a-command)).

### 16.9 Calling a service from a sketch

Every enabled service is a global, and that is the whole API surface an application needs:

```cpp
#include <PdiStack.h>

void setup() {
    PdiStack.initialize();

    wifi_config_table cfg;
    __database_service.get_wifi_config_table(&cfg);
    memcpy(cfg.sta_ssid, "MyNet", 6);
    __database_service.set_wifi_config_table(&cfg);

    __wifi_service.scan_aps_and_configure_wifi_station_async(0);

    __i_dvc_ctrl.gpioMode(DIGITAL_WRITE, 2);
    __i_dvc_ctrl.gpioWrite(DIGITAL_WRITE, 2, 1);

    __task_scheduler.setInterval([&] {
        __i_dvc_ctrl.getTerminal()->writeln("tick");
    }, 1000, millis());

    __utl_event.add_event_listener(EVENT_WIFI_CONNECTED, [&](void*) {
        // reactive code
    });
}

void loop() { PdiStack.serve(); }
```

Reacting to an event uses the same bus the framework does — add a listener by name, fire your own with a payload, and put new event names in the central header.

### 16.10 What to watch for

A handful of habits separate an extension that lands cleanly from one that fights the framework.

Do the work in `init*`, not in a constructor. Globals are constructed before `setup()`, when the device interfaces do not exist yet.

Use events for fan-out and direct calls only for structural dependencies. A service calling another from a callback creates an ordering dependency that breaks the next time init order changes.

Copy borrowed pointers. Command option values, request arguments and HTTP response bodies all point into buffers that are about to be reused.

Keep vendor headers below the device layer. One `#include <esp_wifi.h>` in a service breaks the build for every other port.

Remember that a feature flag gates three things — the device aggregator include, the orchestrator include, and the orchestrator's init call. Miss one and you get either a link error or dead code in the binary.

And keep one global per slot. A second service instance overwrites the first in the service table, and a second table at the same address is quietly skipped at registration.

---
## 17. Troubleshooting & FAQ

Short entries; the explanations live in the sections they point at.

### 17.1 Build and flash

**The build succeeds for ESP8266 or UNO but the device misbehaves.**
The setup script was never run for that target, so the ESP32 fallback produced an ESP32-shaped binary — right code, wrong table addresses and flags. Run `python3 DeviceSetup.py -d <board>` and rebuild ([§2.5](#25-how-the-esp32-default-works)).

**The build succeeds but `srvc list` is empty and no access point appears.**
Same cause seen from the other end: `devices/DeviceSetup.h` still names the previous board. Re-run the script, or delete the file to fall back to ESP32.

**`multiple definition of __i_<x>`.**
Either two ports define the same singleton, or a device `.cpp` was included from outside its aggregator chain. Every device translation unit must be reached through exactly one aggregator ([§14.3](#143-the-two-aggregators)).

**`fatal error: esp_wifi.h: No such file or directory` while building for AVR.**
A vendor header leaked above the device layer. Push the include down into the port ([§16.10](#1610-what-to-watch-for)).

**Compile errors inside `pdiutil::function` or `pdiutil::vector` on an unusual target.**
The toolchain is missing the GCC extensions PdiSTL relies on. Use a GCC-based toolchain.

### 17.2 Boot and runtime

**The device factory-resets every five seconds.**
NVM is invalid — a corrupt checksum, or a struct that changed shape since the last flash. With auto-reset on, one cycle recovers it. If it loops, a table has outgrown its address slot ([§5.9](#59-adding-a-table)).

**Boot stops after the banner and no access point appears.**
The station connect is timing out against stale credentials. Hold the flash button for six or seven seconds to factory-reset, join the access point, and set fresh ones.

**`initialize()` completes but no log output appears.**
No console log gate is set, so every call site compiled away. Enable at least the info gate ([§9.3](#93-flags)).

**The application runs once and stops.**
`PdiStack.serve()` is missing from `loop()`. Without it the scheduler never advances.

**`register_task` always returns -1.**
The slot table is full at 25. Either raise the limit or find the service leaking tasks — `ps` shows the population ([§4.7](#47-behaviour-worth-knowing-before-you-lean-on-it)).

**One service starves the others and the loop feels jittery.**
The scheduler runs one inline task per pass, so a task doing too much per call delays everything behind it. Split it into a state machine or move it to a cooperative lane ([§4.5](#45-what-happens-on-a-tick)).

**Free heap drifts down over days.**
Something is allocating in a hot path. The habits that prevent it are in [§12.4](#124-heap-discipline).

### 17.3 Network and portal

**Cannot join the `pdiStack` access point.**
The password is `pdiStack@123`, case-sensitive. Confirm the radio is up with `srvc status WiFi` over serial.

**Cannot reach `http://192.168.0.1`.**
With dynamic subnetting on, the access point may have chosen a different subnet — use the `.1` of whatever your client was given.

**The login form keeps bouncing back to itself.**
The cookie is being rejected or the five-minute session expired. Private browsing windows often refuse cookies for a bare IP.

**A page arrives truncated mid-HTML.**
A `send()` went out without chunking and exceeded the per-send buffer. Compose in header, body and footer ([§8.7](#87-composing-a-response)).

**MQTT connects but no callbacks fire.**
Wait for the subscribed callback before publishing on that topic — `Subscribe` reports enqueue, not acknowledgement ([§10.2](#102-mqtt)).

**OTA polls the server but never updates.**
The device only updates when the server's version is strictly newer than the firmware version compiled into it. Bump the firmware version before publishing ([§6.2.5](#625-otaserviceprovider--__ota_service)).

**Email sends time out.**
SMTP goes out in plaintext, so providers that require TLS on submission will not complete. Use a relay that accepts plain SMTP, or a test sink ([§10.3](#103-smtp)).

**HTTPS refuses connections or fails the handshake.**
The certificate and key are missing from the filesystem or sitting at different paths than configured. Upload them to `/etc/http/server.crt` and `/etc/http/server.key`; under mTLS the client CA is needed too ([§8.7.1](#871-https-wiring-and-certificates)).

**Outbound HTTPS to an unknown CA fails.**
The bundled client ships with peer verification off so first-boot demos work. For production, point it at `/etc/ssl/ca-bundle.crt` and drop the `setVerifyPeer(false)` line.

**ESP8266 TLS handshakes fail on large records.**
Two ceilings meet here. The handshake runs on a dedicated task stack sized by `TLS_TASK_STACK_SIZE` because the default main stack is too small for an ECDSA sign, and the inbound record buffer defaults well below the 16 KB a peer may emit. Raise the buffer — and pay the heap — only when you cannot control the peer.

**HTTPS works on ESP32 and refuses everything on ESP8266.**
NAPT is almost certainly on as well. The two cannot share that heap ([§12.3](#123-the-expensive-features)).

**The auto cert-generation listener never fires.**
It hangs off the station getting an IP, so it does not run in AP-only mode. Check with `net ip`.

### 17.4 Shell

**Telnet or SSH rejects the right credentials.**
The shell shares the portal's credential row. If they were changed through the portal, the shell inherits them.

**SSH accepts no clients.**
The Ed25519 host key is created when the service starts, so this means the write failed — a full or unmounted filesystem. Check `df`, then regenerate:
```
sshkgen t=1,f=b
```
`t=1` is Ed25519 and is instant; `f=b` is the `/etc/ssh` host-key directory.

**`scp -s` is slow.**
Transfers stream in small chunks and pay flash-write cost on overwrite, which lands around 0.2 to 1 KB/s. Firmware belongs on OTA ([§7.9](#79-sftp-and-scp-file-transfer)).

**`fedit` will not exit.**
Press Esc for the menu, then `!w` to save and leave or `!c` to discard.

**Tab completion works but the arrow keys don't recall history.**
History is a file, so it needs storage; completion reads the in-RAM registry and works either way ([§7.6](#76-the-dispatcher)).

### 17.5 Questions that come up

**Can I run two `PdiStack` instances?**
No. One global stack, one singleton per interface and per service — and on these memory budgets there would be no gain.

**Can I use Arduino `String` instead of `pdiutil::string`?**
In your sketch, yes. In framework code, no — its allocator differs per core, which is exactly the portability the framework is built to avoid depending on.

**Why are command names so short?**
Eight characters for a name and three for an option are sized for AVR-class RAM, and the cost of loosening them multiplies across every command slot.

**Why is SSH so heavy?**
Host keys, key exchange, streaming AES and per-session protocol state add roughly 150 KB of flash and 8 to 16 KB of heap per session. That is why ESP32 is the recommended target for SSH builds ([§12.3](#123-the-expensive-features)).

**Can the device be reconfigured without reflashing?**
Yes. Everything persisted in NVM is reachable from the portal, the shell, or a sketch. Flashing only changes defaults ([§3.8](#38-defaults-are-not-current-values)).

**Where do logs go?**
Console lines go to serial at 115200. With syslog on, those lines also land in `/var/log/syslog.<level>`, and with forwarding on they go to a collector as well ([§9.6](#96-where-syslog-goes)).

**How do I turn on TLS?**
Set `ENABLE_TLS_SERVICE` — BearSSL on ESP8266, mbedTLS on ESP32. Add `ENABLE_HTTPS_SERVER` for the portal, `ENABLE_HTTPS_SERVER_MTLS` for client certificates, and on ESP32 the cert-generation flags for on-boot minting. It is the most expensive flag in the framework ([§12.3](#123-the-expensive-features)), and on ESP8266 it cannot share the board with NAPT.

**How do I provision certificates?**
On ESP32, `tls q=1,t=0,l=256,n=device.local,i=192.168.1.50` writes a self-signed EC pair straight to the configured paths. Anywhere else, `python3 scripts/GenTlsCerts.py --dns device.local --ip 192.168.1.50` produces them off-device for upload. Run the script once with `--gen-ca` and reuse that CA for every device, and a client that trusts it trusts your whole fleet.

**Is there a simulator?**
The mock device lets the framework compile off-device for analysis; it does not simulate behaviour. Interactive testing means real hardware.

**How do I unit-test framework code?**
Mock the interfaces the unit depends on and link against its source. Bring your own harness — PdiSTL builds on host x86 with GCC.

**Where do I report issues?**
GitHub: <https://github.com/Suraj151/pdi-framework>.

### 17.6 When you're properly stuck

```
  1  enable console logging, flash, watch serial at 115200
  2  srvc list              what actually booted
  3  srvc status <name>     that service's state and its pids
  4  ps                     %CPU finds the hog; OWN ties tasks to sessions
  5  top i=2000; n=10       the same view over time
  6  srvc stop / start      freeze and resume, and watch the state column flip
  7  srvc status DB         NVM validity
  8  reboot                 explicitly, so you keep the serial output
```

If none of that localises it, open an issue with the board and package version, the flags you have set, the serial log through the failure, and the exact sequence that reproduces it.
