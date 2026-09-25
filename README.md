# PDI Framework — Portable Device Interface Stack

**A C++ framework for ESP32, ESP8266 and Arduino boards that boots your device into a small operating system.**

PDI is a set of libraries tied together — WiFi, storage, networking, scheduling, security, logging — and on top of them runs an operating system your board starts into. Flash one sketch and the device comes up with a web portal, an SSH server, MQTT, over-the-air updates, a filesystem with real users and file permissions, and a Linux-style shell you can log into over serial, Telnet or SSH.

**Every device inherits what its own hardware can give.** The operating system is written once, against capabilities rather than chips. A board with a radio gets the networking services; one without simply has them left out of its build, and not a line above them changes. The same goes for storage, for TLS, for a second execution lane — each arrives because a port said it could carry it. Adding a new board means describing what that board can do, not porting the system to it.

**Small on the device, big in what it gives you.** It fits in the flash of an ordinary microcontroller and still hands you the things you would otherwise build yourself every time: remote login, file transfer, scheduled jobs, firmware updates over WiFi, a configuration UI, and an API to your own hardware. You write normal Arduino code on top, and the same sketch moves from one board to another.

**Stability comes from staying inside the board's limits.** Enable the services the hardware can carry — flash and RAM are the budget — and the device keeps running without resets or dropped connections. That is what the test suite checks: 1300+ unit tests, and 425+ feature tests driven on real hardware — two different boards at once, each over serial, Telnet and SSH at the same time.

## Quick Start

1. **Install** from the **Arduino Library Manager** (search "pdi-framework"). Builds for ESP32 by default.
   For ESP8266 / Arduino UNO, run: `python3 scripts/DeviceSetup.py -d <board>`.
2. Open **File → Examples → pdi-framework → PdiStack**, then compile and upload. That sketch is the whole application:

   ```cpp
   #include <PdiStack.h>

   void setup() {
    PdiStack.initialize();
   }

   void loop() {
    PdiStack.serve();
   }
   ```

   `initialize()` mounts the filesystem tree and starts every service the build enables, each through its own `startService()`. `serve()` is the loop body: it services web clients, runs the inline task scheduler, yields to the device controller and drains pending device events. Everything described on this page runs from those two calls — your own code goes alongside them, not instead of them.

   Note : ESP32 require 1.4MB+ app size so make sure you will select suitable partition scheme that fits required app size.
3. **Over the serial cable.** Open the serial monitor at **115200** and log in as **`pdiStack` / `pdiStack@123`**. You get the full shell — the same one every other route reaches.
4. **Over the device's own WiFi.** Look for the network **`pdiStack`**, password **`pdiStack@123`**, then browse to **http://192.168.0.1** and log in with the same credentials for the web portal.
5. **Over your own network.** Point the device at your router from the portal or with `net` commands, then reach it by IP or by name — it advertises itself over mDNS as **`pdi-<xxxxxx>.local`**, so any machine on the same router can use that instead of hunting for the address:
   ```
   ssh pdiStack@pdi-<xxxxxx>.local        # or telnet, port 23
   ```
6. **Copying files.** `scp -s file pdiStack@<host>:/path` or `sftp -P 22 pdiStack@<host>`.

That's it — the device is now running a web portal, a remote shell and file transfer, and the shell behaves the same whether you reached it over serial, ssh or telnet.

Manual clone paths, the autogen script, board-package versions and git-ignored files are covered in [§2 Build & Toolchain](#2-build--toolchain).

### Supported Boards

| Device | Arduino board version |
|---|---|
| esp32 | 3.3.3 |
| esp8266 | 3.1.2 |
| arduinouno | 1.8.6 |

Not every board exposes every capability. An Arduino UNO has no WiFi, so the web server, MQTT and OTA services are compiled out on that port.

## What You Can Do With It

**Log in to your board like a computer.** Open a serial monitor, or `ssh` in, or `telnet` in — you get the same login prompt and the same 57 commands either way. `ls`, `cat`, `grep`, `ps`, `top`, `kill`, `df`, `chmod`, `ping`, `wget` and the rest all behave the way you expect, with tab completion, command history, an in-place file editor and Ctrl+C. Commands pipe into each other and redirect to files, so `ps | grep ssh` and `cat /proc/meminfo > /tmp/mem.txt` do exactly what they look like.

**Run it from a browser.** The device serves its own web portal: a live dashboard, a settings page for each service, GPIO switches, a file browser, MQTT and email testers, and firmware upload. Users sign in with their own accounts and see only what their permissions allow — the same rules the shell uses. It works on a phone and follows your light or dark theme.

**A real filesystem, with real users.** Files live on flash and survive reboots. `/proc` shows live memory, running tasks and network state. `/sys` exposes GPIO pins and network interfaces as files. `/tmp` lives in RAM. Every file has an owner and permission bits, so two people logged in at once genuinely see different things — `/etc/shadow` is readable by root and nobody else, and it means it.

**Control your hardware from anywhere.** Pins are files, so `echo 1 > /sys/class/gpio/5/value` turns one on from any terminal. The portal gives you switches for them. MQTT and HTTP reach them from across the internet, and a pin crossing a threshold can fire off an email or call a web endpoint on its own.

**Get it on the network, and find it again.** A captive portal appears on first boot so you can pick your WiFi from a phone with no code. After that the device announces itself as `pdi-<id>.local`, so you reach it by name instead of hunting for an IP address — it shows up in your router, in `avahi-browse`, and in Finder.

**Update it without plugging it in.** Push new firmware over WiFi from the portal, or from a file already on the device. No cable, no reflash, no losing your settings.

**Give it jobs to do.** Schedule a function to run every few seconds, once after a delay, or continuously on its own stack. Write shell scripts with `if`, `while`, `for` and variables and run them with `source`. Put ordinary cron lines in `/etc/crontab` for anything that should happen at a particular time. `/etc/rc.local` runs at boot, so a device can bring itself back up exactly as you left it.

**Settings you can read, not reflash.** WiFi, MQTT, OTA and email keep their configuration in plain text files under `/etc` that you can open, edit, copy off over SFTP and compare between two devices. Change a setting and restart the service — no rebuild.

**Secrets stay secret.** Stored passwords and keys are encrypted on the device, so a copy of its storage taken off it reveals nothing. SSH uses real host keys, HTTPS is available where the board can support it, and every log line can be shipped to a remote collector as well as kept on disk.

**Try the whole thing on your laptop.** The same code builds as a normal program on your computer — you can `ssh` into it, copy files to it and open its portal in a browser, all without a board plugged in. That is how the test suite checks every change before hardware is involved.

**Make it yours.** Add your own shell command, your own web page, your own settings table, or your own service, and the framework treats it exactly like the built-in ones. The bundled examples show each of these in a sketch you can copy.

## What's Inside

**19 services** — Time · Cron · WiFi · Web server · MQTT · OTA · SSH · Telnet · Email · GPIO · Serial · Shell · Database · User store · Login · mDNS · Syslog · Factory reset · Device-IoT.

You manage them the way you would on Linux: `service list` shows what is running, and `service start`, `stop`, `restart`, `enable` and `disable` do what they say. Stopping the SSH service really gives up port 22 rather than parking it — and the system will not let you stop the service carrying the session you are typing into.
Per-service reference in [§6 Service Providers](#6-service-providers).

**Things the services are built on**, rather than services themselves — TLS/HTTPS · SFTP and scp · link-layer mesh.

**57 shell commands** — the full list is in [§7.8 Built-in command inventory](#78-built-in-command-inventory).

**Storage** — one filesystem tree covering flash, `/proc`, `/sys`, `/dev` and `/tmp`, with Unix-style permissions and multi-user access control.
Details in [§6.2.12](#6212-storage-interface-init-no-provider).

**Building blocks** — task scheduler, event bus, queues, string and conversion helpers, crypto, and a compact C++ standard library for boards that ship without one.
Full inventory in [§15 Utility Library](#15-utility-library).

**Extras** — captive portal, GPIO events over MQTT/HTTP/email, and NAT plus peer-to-peer mesh networking where the radio supports them ([§2.4.1](#241-nat-and-mesh)).

## A Peek at the Terminal and Web UI

<table>
  <tr>
    <td width="50%"><img src="https://github.com/Suraj151/pdi-framework/blob/master/doc/terminal-boot.png" width="100%"></td>
    <td width="50%"><img src="https://github.com/Suraj151/pdi-framework/blob/master/doc/terminal-ls-1.png" width="100%"></td>
  </tr>
  <tr>
    <td width="50%"><img src="https://github.com/Suraj151/pdi-framework/blob/master/doc/terminal-ps-1.png" width="100%"></td>
    <td width="50%"><img src="https://github.com/Suraj151/pdi-framework/blob/master/doc/terminal-service.png" width="100%"></td>
  </tr>
  <tr>
    <td width="50%"><img src="https://github.com/Suraj151/pdi-framework/blob/master/doc/terminal-usradd-1.png" width="100%"></td>
    <td width="50%"><img src="https://github.com/Suraj151/pdi-framework/blob/master/doc/terminal-ssh.png" width="100%"></td>
  </tr>
  <tr>
    <td width="50%"><img src="https://github.com/Suraj151/pdi-framework/blob/master/doc/terminal-net-1.png" width="100%"></td>
    <td width="50%"><img src="https://github.com/Suraj151/pdi-framework/blob/master/doc/terminal-help-1.png" width="100%"></td>
  </tr>
  <tr>
    <td width="50%"><img src="https://github.com/Suraj151/pdi-framework/blob/master/doc/portal_home_menu.png" width="100%"></td>
    <td width="50%"><img src="https://github.com/Suraj151/pdi-framework/blob/master/doc/gpio-control-menu.png" width="100%"></td>
  </tr>
  <tr>
    <td width="50%"><img src="https://github.com/Suraj151/pdi-framework/blob/master/doc/mqtt-submenu.png" width="100%"></td>
    <td width="50%"><img src="https://github.com/Suraj151/pdi-framework/blob/master/doc/storage-home.png" width="100%"></td>
  </tr>
  <tr>
    <td ><img src="https://github.com/Suraj151/pdi-framework/blob/master/doc/dashboard.png" width="100%"></td>
  </tr>
</table>

## Want to Dig Deeper?

The **[Detailed Documentation](#detailed-documentation)** below is the in-tree reference for contributors and porters. Jump to whatever you're working on:

- **[1. Architecture Overview](#1-architecture-overview)** — layered model, ports and adapters, runtime lifecycle.
- **[2. Build & Toolchain](#2-build--toolchain)** — install, switch board, autogen scripts, vendored externals.
- **[3. Configuration System](#3-configuration-system)** — every `ENABLE_*` flag and what depends on it.
- **[4. Task Scheduler](#4-task-scheduler)** — three modes, four policies, and how to choose.
- **[5. Database Layer](#5-database-layer)** — record store, storage and eeprom tiers, sealed records, JSON codegen.
- **[6. Service Providers](#6-service-providers)** — per-service init flow, CLI and web surface, events.
- **[7. Command Line / Terminal](#7-command-line--terminal)** — full CLI reference, variables, scripting, and how to add a command.
- **[8. Web Server](#8-web-server)** — request lifecycle, routes, views, adding a page.
- **[9. Logger](#9-logger)** — levels, macros, and the zero-cost-when-disabled pattern.
- **[10. Transports](#10-transports)** — HTTP, MQTT and SMTP client internals.
- **[11. Examples Walkthrough](#11-examples-walkthrough)** — what each bundled sketch demonstrates.
- **[12. Memory & Performance Notes](#12-memory--performance-notes)** — flash and RAM cost per feature.
- **[13. Portable Interfaces](#13-portable-interfaces)** — the ports every device adapter implements.
- **[14. Device Layer & Porting Guide](#14-device-layer--porting-guide)** — how to add a new board.
- **[15. Utility Library](#15-utility-library)** — event bus, string ops, embedded STL, crypto.
- **[16. Extending the Framework](#16-extending-the-framework)** — adding services, commands, pages.
- **[17. Test Suite](#17-test-suite)** — the four tiers, running them, adding a test.
- **[18. Troubleshooting & FAQ](#18-troubleshooting--faq)** — common issues and fixes.

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
   Devices             │  one folder per supported board, plus a host port │
   (the "adapters")    │  concrete implementations + one aggregator each  │
                       └──────────────────────────────────────────────────┘
```

Dependencies only ever point downward. A service never includes a device header; it holds an `i*Interface` pointer and calls through it. The one place device code is reached from above is [src/interface/pdi.h](src/interface/pdi.h), which builds the include path for exactly one board aggregator out of `PDI_DEVICE` — the port's directory name, set on the compiler command line or written into `devices/DeviceSetup.h` by `scripts/DeviceSetup.py`.

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

The base class fixes the lifecycle. `initService` is the required override, and callers never reach for it directly — `startService` wraps it and records what it returned, which is the state both `PDIStack::initialize` and the `service` command read. `stopService` tears down and records its own; an override calls the base to clear the tracked tasks. `getServiceDependencies` names what the service `Requires`, `printConfigToTerminal` and `printStatusToTerminal` supply what `service status` prints, and a static `getService(st)` lets one service find another without include cycles.

Because the shape is uniform, `service` can enumerate, start, stop and inspect anything without knowing what it is.

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
 │     └─ scheduler gets its utility interface
 │
setup()
 │
 ├─ PdiStack.initialize()
 │     ├─ device brings up its own features
 │     ├─ terminal acquired and handed to the ServiceProvider base
 │     ├─ time service starts                (always, every port)
 │     ├─ database service starts            (always)
 │     ├─ every enabled service starts       (conditional)
 │     ├─ /etc/rc.local runs, if present     (script runner)
 │     └─ cron starts watching the minute      (cron service)
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
| `__i_db`, `__i_fs`, `__i_storage`, `__i_wifi`, `__i_http_server`, `__i_ntp`, `__i_ping`, `__i_instance` | device | interface singletons, present only when the matching flag is on |
| `__task_scheduler`, `__utl_event` | utilities | scheduler and event bus |
| `__database_service`, `__wifi_service`, `__mqtt_service`, `__ota_service`, … | services | one per `ServiceProvider` subclass |
| `__i_cooperative_scheduler`, `__i_preemptive_scheduler` | threading port | the contextual lanes |
| `PdiStack` | orchestrator | the single application-facing facade |

Services reach each other by global symbol when the dependency is fixed, and through `ServiceProvider::getService(st)` when the lookup has to stay generic — which is exactly what `service` does.

---
## 2. Build & Toolchain

The build target is the Arduino IDE / arduino-cli toolchain. Shared code holds at the `gnu++11` floor, because that is the oldest standard any supported core compiles with; other ports build at `gnu++17` and `gnu++2a`. `scripts/SyntaxSweep.py` compiles every file against all three, so a newer construct cannot quietly slip into a shared header and strand the oldest port. Beyond that and GCC's variadic-macro extension nothing is assumed, so PlatformIO or a hand-rolled `make` works as long as you reproduce the usual Arduino-core defines.

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

A fresh install already carries ESP32-shaped placeholder database headers, and `devices/DeviceConfig.h` falls back to `PDI_DEVICE esp32` when no generated setup header is present — hence the zero-step ESP32 build.

**For ESP8266 or Arduino UNO,** generate the per-device files first:

```
cd <your-Arduino-libraries-path>/pdi-framework/scripts
python3 DeviceSetup.py -d esp8266        # or arduinouno
```

That writes `devices/DeviceSetup.h` with the right `PDI_DEVICE` and regenerates the database table headers for the target. Changing boards later is the same one-liner with a different name.

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

**PlatformIO.** `library.json` carries the build rules and the bundled-library dependencies, so a project needs nothing beyond the port selector:

```ini
[env:esp8266]
platform = espressif8266
board = nodemcuv2
framework = arduino
lib_deps = https://github.com/Suraj151/pdi-framework.git
build_flags = -DPDI_DEVICE=esp8266

[env:esp32]
platform = https://github.com/pioarduino/platform-espressif32/releases/download/stable/platform-espressif32.zip
board = esp32dev
framework = arduino
board_build.partitions = min_spiffs.csv
lib_deps = https://github.com/Suraj151/pdi-framework.git
build_flags = -DPDI_DEVICE=esp32
```

`-DPDI_DEVICE=<port>` selects the port on the command line and takes precedence over `devices/DeviceSetup.h`, so no script run is needed to change boards. The database table headers still come from `DeviceSetup.py` when you need a target other than the ESP32 defaults.

Two settings the ESP32 environment cannot do without. `board_build.partitions = min_spiffs.csv` gives each app slot 1.9 MB and keeps both OTA slots, where the `esp32dev` default of 1.31 MB cannot hold the image. And the platform line points at the `pioarduino` fork because the official `espressif32` platform pins Arduino-ESP32 2.x, while the framework builds against the 3.x line.

⚠ Do not set `lib_ldf_mode = deep+`. In that mode the dependency finder stops resolving `Networking` into the bundled `WiFi` library, and the ESP32 build fails on a missing `Network.h`. The default mode resolves it correctly.

### 2.3 What the scripts do

```
  DeviceSetup.py -d <board>
        │
        ├──▶ devices/DeviceSetup.h        #define PDI_DEVICE <board>
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
| `GenTlsCerts.py` | provisioning HTTPS certificates off-device, or signing a device cert with a stable dev CA |

Generated headers are passed through `clang-format --style=Microsoft` when the formatter is on `PATH`, so they read like hand-written code. Without it they are written unformatted and compile the same.

### 2.4 Vendored externals

[external/littlefs/](external/littlefs/) is the filesystem used by the storage interface on devices that have storage, and by everything downstream of it — SFTP, shell history, the file commands. Builds for a device without storage never reach it.

#### 2.4.1 NAT and mesh

Both are radio-level capabilities layered onto WiFi rather than regular services.

**NAT**, where the device supports it, rewrites IP-header fields on packets in transit so that clients joining the device's access point reach the upstream network the station link is connected to. It needs the IPv4 network stack the board's core supplies, in the variant that includes translation — on cores that offer the choice it is a Tools-menu setting. On the service side, `ENABLE_NAPT` makes the WiFi service schedule a one-shot NAPT enable once the station link is up.

**Mesh**, where the radio offers a connectionless peer-to-peer link layer, wraps that into a small API so applications can build broadcasts and hop-distance topologies without touching the driver. It shares the radio with station mode and is configured from the application. Paired with `ENABLE_DYNAMIC_SUBNETTING` and `ENABLE_INTERNET_BASED_CONNECTIONS` on the WiFi service, it gives each node a notion of how many hops it sits from the hub.

#### 2.4.2 mDNS and DNS-SD

The responder is written from scratch on the network stack's raw UDP API — the datagram calls plus a multicast group join, taken under the stack's core lock where the port needs one — and runs as an ordinary service. No vendor mDNS library is involved.

```
  EVENT_WIFI_STA_GOT_IP
        │
        ├─ hostname pdi-<last-3-mac-bytes> written to /etc/hostname
        ├─ join 224.0.0.251:5353
        ├─ queue HTTPS cert provisioning        (where the port can generate certs)
        └─ answer queries as they arrive        (callback-driven, nothing to pump)
                │
                ├─ A                    →  hostname → address
                ├─ PTR _services…       →  which service types exist
                ├─ PTR <type>           →  instances of a type
                └─ SRV / TXT            →  port + metadata, bundled with A
```

It advertises what the build is actually listening on: `_http._tcp` or `_https._tcp`, `_ssh._tcp`, `_sftp-ssh._tcp`, `_telnet._tcp`. Outbound clients such as MQTT and OTA listen for nothing, so nothing is advertised for them. `cat /etc/hostname` shows the name, `ping pdi-<xxxxxx>.local` proves it resolves, and `service status MDNS` lists the address and the advertised set. Service types, TTLs and the multicast group live in [src/config/MdnsConfig.h](src/config/MdnsConfig.h).

With runtime certificate generation on, the responder is also what provisions the HTTPS certificate. It is the one place holding both the address and the name, so the certificate it asks for carries `<hostname>.local` as a DNS subject alt name alongside the IP, and `https://pdi-<xxxxxx>.local/` matches it. Key generation wants several kB of stack and runs for seconds, so the responder queues it on the scheduler instead of doing the work in the event callback.

### 2.5 How the default port works

Three things line up so that a first build needs no scripts:

```
  devices/DeviceConfig.h
        │
        ├─ #if __has_include("DeviceSetup.h")  →  use the generated macro
        └─ #else                               →  fall back to the default port
                │
                ├─ per-port config cascade ends in that port's device_config.h
                └─ checked-in placeholder table headers match it
```

The default is whichever port `DeviceConfig.h` names in that `#else`, and it ships as `esp32` — the one place in the documentation where a particular board matters, because it decides whether you need to run anything at all before your first build.

Running `DeviceSetup.py` for another board overrides all three: the generated `DeviceSetup.h` wins over the fallback, and fresh table headers replace the placeholders. To come back, either re-run the script for the default port or delete `devices/DeviceSetup.h` and let the fallback take over again.

`DeviceSetup.h` is not tracked, so a fresh clone always starts on the fallback. A build that only needs a different port, and not a different table set, can say so on the compiler command line instead — `-DPDI_DEVICE=<board>` takes precedence over both the generated header and the fallback, and leaves the working tree untouched. See [§14.4](#144-how-a-board-gets-selected).

Because the fallback is silent, a build flashed onto any other board without running the script compiles happily with the default port's table set and feature flags — which is the one failure here that looks like a hardware fault rather than a build mistake. Run the script whenever you leave the default port, and again whenever you come back — `git checkout src/database/tables/` restores the placeholders if the generated ones are still lying around.

#### 2.5.1 Per-port capability flags

Board-specific answers live in each port's `device_config.h`, not in the central config, which keeps the selection logic board-agnostic:

| Macro | Set by | Effect |
|---|---|---|
| `DEVICE_SUPPORTS_TLS` | a port with an SSL backend | lets `ENABLE_TLS_SERVICE` take effect; ports without it get the flag undefined automatically |
| `DEVICE_SUPPORTS_CONTEXTUAL_EXECUTION` | a port with the threading interfaces | same shape, for the cooperative and preemptive lanes |
| `DEVICE_SUPPORTS_TLS_CERT_GENERATION` | a port with the RAM and the RNG for it | gates on-device certificate generation |
| `DEVICE_SUPPORTS_NTP` | a port that can reach the network | the port can reach a time server, so the clock can call itself valid; a port without it still keeps time, counted from boot |
| `MAX_DIGITAL_GPIO_PINS`, `MAX_ANALOG_GPIO_PINS`, `MAX_DB_TABLES` | every port | per-board limits |
| `ENABLE_NETWORK_SERVICE`, `ENABLE_AUTH_SERVICE`, `ENABLE_STORAGE_SERVICE`, `ENABLE_GPIO_BASIC_ONLY` | every port | per-board defaults — a port with no radio and no filesystem omits network, auth and storage; one with both enables them |

The contract is simple: genuinely per-board facts go in the per-port header, and the central `DeviceConfig.h` carries the cross-board feature flags, the auto-undef chains, and settings that are a property of the deployment rather than the board — the timezone (`TZ`, `TZ_SEC`, `DST_MN`) and `NTP_SERVER1` among them, so anything wanting local time does not have to reach into the NTP interface for it. A new port sets its `DEVICE_SUPPORTS_*` macros and the optional services fall in line by themselves.

---
## 3. Configuration System

Configuration is layered and additive. Which services *exist*, how big each table is, what the defaults are and which interfaces a port supplies are decided when you build. What a service *does* once it exists — including whether it runs at all — is settled at runtime, from text files under `/etc` on any board that has a filesystem. See §3.10.

### 3.1 Three tiers

| Tier | Lives in | Owns | Written by |
|---|---|---|---|
| Device | `devices/DeviceSetup.h` (generated), `devices/DeviceConfig.h`, and each port's `device_config.h` | the `PDI_DEVICE` selector, the `ENABLE_*` flags, per-board limits, and platform macros such as `RODT_ATTR` and `CRITICAL_SECTION_ENTER/EXIT` | integrator picking board and features; porter describing the board |
| Common | `src/config/Common.h`, `src/config/GlobalConfig.h` | cross-cutting constants and the always-present `global_config` table | framework author, integrator |
| Service | one `*Config.h` per service under `src/config/` | per-service knobs plus the struct that gets persisted to NVM | service author |

Everything funnels through [src/config/Config.h](src/config/Config.h), which conditionally includes the service-tier headers according to the flags the device tier settled on. Every translation unit in the framework reaches configuration through that one header.

### 3.2 How the pieces include each other

```
  DeviceSetup.h            generated:  #define PDI_DEVICE <name>
        │
        ▼
  DeviceConfig.h           PDI_DEVICE ──▶ ENABLE_* cascade
        │                  + pulls in <port>/device_config.h for platform macros
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
| `ENABLE_STORAGE_SERVICE` | where the port has storage | filesystem + `StorageConfig.h` | `iStorageInterface`, `iFileSystemInterface` | medium |
| `ENABLE_GPIO_SERVICE` | yes | GPIO service + `GpioConfig.h` | `iGpioInterface` | low; `ENABLE_GPIO_BASIC_ONLY` trims it further |
| `ENABLE_CMD_SERVICE` | yes | the CLI | a terminal source | low |
| `ENABLE_AUTH_SERVICE` | with storage | auth service + the credential table | storage | low |
| `ENABLE_NETWORK_SERVICE` | where the port has a radio or a link | umbrella for everything below | TCP client/server, NTP, ping, WiFi | — |
| `ENABLE_WIFI_SERVICE` | with network | WiFi service + `WifiConfig.h` | `iWiFiInterface` | medium |
| `ENABLE_HTTP_SERVER` | with network | web portal + `ServerConfig.h`, `HttpConfig.h` | `iHttpServerInterface` | medium |
| `ENABLE_HTTPS_SERVER` | off | serves the portal on 443 with certs from the filesystem, and redirects plain HTTP on 80 to it | turns on `ENABLE_TLS_SERVICE` | high |
| `ENABLE_HTTP_CLIENT` | with network | outbound HTTP | TCP client | low |
| `ENABLE_MQTT_SERVICE` | with network | MQTT service + `MqttConfig.h` | TCP client, or the TLS client when the broker is configured secure | medium |
| `ENABLE_OTA_SERVICE` | with network | OTA service + `OtaConfig.h` | TCP client, `iUpgradeInterface` | low |
| `ENABLE_EMAIL_SERVICE` | with network | email service + `EmailConfig.h` | TCP client | low |
| `ENABLE_TELNET_SERVICE` | with network | telnet server | TCP server | low |
| `ENABLE_SSH_SERVICE` | with network and storage | SSH server, SFTP, scp | TCP server, filesystem | high |
| `ENABLE_DEVICE_IOT` | with network | IoT service + `DeviceIotConfig.h` | TCP client | low |
| `ENABLE_TLS_SERVICE` | off | TLS client and server + `TlsConfig.h`; turns on contextual execution, since TLS runs on its own cooperative task | whichever TLS backend the port supplies | high — see [§12.3](#123-the-expensive-features) |
| `ENABLE_TLS_CERT_GENERATION` | off | the `tls` command and the on-device issuer | TLS service, and a port declaring `DEVICE_SUPPORTS_TLS_CERT_GENERATION` | medium |
| `ENABLE_SERVER_TLS_CERT_GENERATION_AT_RUNTIME` | with cert generation | the mDNS service mints a self-signed cert covering the address and `<host>.local` once the station gets an IP | cert generation, mDNS | one-shot |
| `ENABLE_SCRIPT_RUNNER` | yes | `source`, control flow and `/etc/rc.local` | cmd + storage | low |
| `ENABLE_CONTEXTUAL_EXECUTION` | off | cooperative and preemptive lanes | the threading interfaces | high (per-task stacks) |

#### 3.3.2 Behaviour flags

| Flag | Effect |
|---|---|
| `ENABLE_GPIO_BASIC_ONLY` | digital-only GPIO, for a port with no analog path |
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
| `MAX_DIGITAL_GPIO_PINS` | size of the GPIO config table | 8-14, whatever the board exposes |
| `MAX_ANALOG_GPIO_PINS` | same, analog side | 1-5, whatever the board exposes |
| `MAX_DB_TABLES` | upper bound on registered tables | 5 where NVM is a kilobyte, 15 where it is a few |
| `MAX_SCHEDULABLE_TASKS` | inline scheduler slots | 25, trimmed to 8 on a port with kilobytes of RAM |
| `MAX_FACTORY_RESET_CALLBACKS` | reset hooks | same as the task count |
| `WIFI_STATION_CONNECT_ATTEMPT_TIMEOUT` | station connect budget, seconds | 1 |
| `WIFI_CONNECTIVITY_CHECK_DURATION` | link recheck interval, ms | 5000 |
| `INTERNET_CONNECTIVITY_CHECK_DURATION` | internet recheck cadence | same |

### 3.4 The shape of a service config

Every per-service config follows the same five-part shape, which is what lets the database layer and the web forms treat them uniformly. WiFi is the canonical example:

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

using wifi_config_table = wifi_configs;   // 5. canonical alias
```

The struct carries no size constant of its own. A table declares `sizeof(Table)` when it registers, and the database engine reserves the slot from that — framing, growth room and the seal a credential table needs are all its business, not the config header's. [§5](#5-database-layer) covers how a record is laid out and how the total is checked against the medium at build time.

### 3.5 What each config file carries

| File | Persisted table | Headline knobs |
|---|---|---|
| Common | — | `USER`, `PASSPHRASE`, task count, time durations, HTTP request budget |
| GlobalConfig | `global_config_table` | config version, firmware version, release, launch time |
| WifiConfig | `wifi_config_table` | buffer sizes, default IPs, modification policy |
| ServerConfig | `login_credential_table` | session name, cookie max age, web session slots and token size, absolute session cap, login attempt limit and lockout |
| HttpConfig | — | client buffer size, request limits, HTTPS port, HSTS age |
| TlsConfig | — | per-session record buffers, TLS task stack and poll interval, default cert/key paths |
| OtaConfig | `ota_config_table` | host, port, version, check cadence, local image extension and magic byte |
| MqttConfig | general / LWT / pub-sub tables | broker, last will, publish and subscribe slots |
| GpioConfig | `gpio_config_table` | pin map, modes, event conditions and channels |
| EmailConfig | `email_config_table` | SMTP host, port, auth, default subject |
| DeviceIotConfig | `device_iot_config_table` | config and OTP URLs, channel keys, sampling bounds |
| SerialConfig | — | mode, baud, interface selection |
| StorageConfig | — | mount point, path limits |
| SshConfig | — | key algorithms, RSA key bits, session pool size and pool-full grace, auth policy, host-key and config paths |
| TelnetConfig | — | port, idle timeout, session pool size and pool-full grace |
| SessionConfig | — | session table size, pipe capacity, file-stream buffer, tail hold-back |
| VfsConfig | — | mount slots, prefix and name limits, per-backend enable flags, mount prefixes, devfs read cap |
| TmpFsConfig | — | the RAM filesystem's byte, node and path budgets |
| UserStoreConfig | — | `/etc/passwd` and `/etc/shadow` paths, field and record limits |
| SyslogConfig | — | syslog directory and the four level files, rotation size |
| MdnsConfig | — | advertised service types, TTLs, multicast group |
| NetworkConfig | — | network-wide timeouts |
| EventConfig | — | the event enum and channel registry |

The always-on tables — global config, plus credentials and WiFi when those services are on — are the minimum NVM footprint. Everything else arrives with its flag.

### 3.6 What depends on what

```
  NETWORK ─┬─ WIFI ─── HTTP_SERVER ─┬─ HTTPS_SERVER ── TLS_SERVICE ── CONTEXTUAL_EXECUTION
           │                        └─ (needs STORAGE for certs)
           ├─ MQTT · OTA · EMAIL · DEVICE_IOT        (TCP client; all but EMAIL
           │                                          use the TLS one for a
           │                                          secure address when
           │                                          TLS_SERVICE is on)
           ├─ TELNET                                 (TCP server, pairs with CMD)
           └─ SSH ─── STORAGE                        (host keys, SFTP)

  STORAGE ─┬─ AUTH                                   (credentials survive reboot)
           └─ SYSLOG ─── (+NETWORK) ─── SYSLOG_FORWARD

  TLS_CERT_GENERATION ── TLS_SERVICE + a port declaring cert generation
  DEVICE_IOT ── the application implements iDeviceIotInterface and passes it to initService
```

Most of these are enforced structurally — the dependent flags are physically nested inside `#ifdef ENABLE_NETWORK_SERVICE` and friends in `DeviceConfig.h`. Two pairings are worth remembering because nothing stops them at compile time: TLS and NAPT both want a large share of the same heap, so a device without room for both will fail at runtime rather than at build time, and enabling TLS implies contextual execution because the TLS engine runs on its own cooperative task.

### 3.7 Common build shapes

| Goal | Keep | Drop |
|---|---|---|
| Smallest possible | serial, CLI, basic GPIO | everything else |
| Offline gateway | add storage and auth | everything network |
| Headless networked node | add network, WiFi, MQTT, OTA, NTP | web server, SSH, email |
| Full portal | the stock `DeviceConfig.h` | — |
| Diagnostics build | stock plus console logging, optionally syslog | — |
| Concurrency demo | stock plus contextual execution | — |
| HTTPS portal | stock plus TLS and HTTPS, and cert generation where the port has it | NAT, which competes for the same heap |
| HTTPS with client certs | as above plus mTLS | — |

### 3.8 Defaults are not current values

Editing `DEFAULT_SSID` does not change the SSID a running device uses. Defaults are consulted exactly twice: on first boot, when NVM is empty or the checksum fails, and on factory reset when the framework is configured to write defaults back rather than zeroing.

To change a live value, go through the portal, the CLI (`net connsta`, `iot sethost`, and so on), or the database service directly and persist the table. Defaults govern the fallback state; the database holds the current one.

### 3.9 Conventions worth keeping

Branch on `ENABLE_*` with `#ifdef`, never with a runtime `if` — the unreached branch still needs symbols the link cannot provide. Keep config structs POD and fixed-size; a pointer or a `pdiutil::string` inside one cannot be serialised to NVM. Never include one service config from another; if two need the same value, it belongs in `Common.h`. And gate code on the capability (`ENABLE_WIFI_SERVICE`) rather than on the board (`DEVICE_<BOARD>`) — boards come and go, the capability is what the code actually depends on.

### 3.10 The `/etc` config surface

On a board with a filesystem, a feature keeps its settings in a plain text file at `/etc/<feature>/<feature>.conf`, one `key value` per line, `#` for comments. It is readable with `cat`, editable with `fedit`, copyable off the device over SFTP, and diffable between two devices — configuration becomes portable in the Linux sense rather than something you rebuild for.

`src/helpers/ConfigHelper.h` is the whole API:

| Call | What it does |
|---|---|
| `buildConfigPath(name, out)` | the path a feature's settings live at, in one place |
| `loadConfigFile(path, out)` | every option as key/value pairs |
| `getConfigValue(path, key, out)` | one option, without holding the rest |
| `setConfigValue(path, key, value)` | persist one option, leaving comments, ordering and other options untouched |
| `saveConfigFile(path, kvs, header, mode)` | write the whole file from pairs |
| `ensureConfigFile(path, defaults, header, mode)` | create the directory and the file with defaults, only if missing |
| `writeConfigValues(path, kvs, header, mode)` | bring the file to these options, creating it when absent and otherwise rewriting only what changed |
| `appendConfigValue` / `findConfigValue` | build and read a set of pairs without knowing how a line is spelled |
| `takeConfigText` / `takeConfigAddress` / `takeConfigBool` / `takeConfigNumber` | take one option into a record field, leaving it alone if the file does not carry it or the value will not read |
| `configAddressAsValue` / `configBoolAsValue` / `configNumberAsValue` | render a field the way a file carries it |
| `configValueAsBool` | the `yes`/`no` rule, in one place |

The `take*` calls are where a malformed file is made harmless. Each returns whether it applied anything and otherwise leaves the field exactly as it arrived, so a typo costs that one option rather than the record. They validate by round-tripping through the matching renderer — `port smtp` and `port 70000` both fail to read back as themselves and are ignored, and so is `sta_gateway not.an.address`.

A feature turns its record into pairs and back in `src/helpers/FeatureConfigFiles.h`, four functions per feature: render to pairs, take from pairs, write the file, and sync file and record store at service init. `ConfigHelper` stays generic — if a signature mentions only keys, values and paths it belongs there; if it names a feature's table it belongs in `FeatureConfigFiles`.

`setConfigValue` rewrites through a working copy and renames over the original, so a half-written file never replaces a good one — and it restores the original's permissions and owner afterwards, because a file created on this VFS is stamped with the *writer's* identity. A `0600 root:root` config stays that way.

Values are written with CRLF, the same line ending `fedit` writes and `putln()` sends. `cat` streams file bytes straight to the terminal, so a file with bare newlines would staircase down a raw console.

**Precedence.** Where a filesystem exists, a key present in the conf file wins. A key that is absent leaves whatever the record store or the compiled default supplied, and a value that cannot be read keeps the default rather than silently becoming zero or `false`. On a board with no filesystem the whole layer compiles out — the record store is the fallback and the boot-critical minimum, and such a board keeps working with no `/etc` at all.

#### Which features have a settings file, and turning one on

Four features keep their settings in `/etc`:

| Feature | File | Gate | Carries |
|---|---|---|---|
| WiFi | `/etc/wifi/wifi.conf` | `ENABLE_WIFI_CONFIG_FILE` | station and AP credentials, addresses, and the `sta_enable` / `ap_enable` gates |
| MQTT | `/etc/mqtt/mqtt.conf` | `ENABLE_MQTT_CONFIG_FILE` | broker host, port, whether the broker is reached over TLS, client id, credentials, keepalive, clean session, and the last will |
| OTA | `/etc/ota/ota.conf` | `ENABLE_OTA_CONFIG_FILE` | update server host and port |
| Email | `/etc/email/email.conf` | `ENABLE_EMAIL_CONFIG_FILE` | SMTP host, port, credentials, sender, recipient and subject |

Each gate is declared in that feature's own header under `src/config/`, inside `#ifdef ENABLE_STORAGE_SERVICE`, and ships commented out:

```cpp
/* src/config/MqttConfig.h */
#ifdef ENABLE_STORAGE_SERVICE
// #define ENABLE_MQTT_CONFIG_FILE
#endif
```

Uncomment one and that feature starts keeping a file, seeded from whatever the record store currently holds so a configured device keeps working. Leave it commented and the record store stays the only source, and a conf an earlier build left behind is inert text. Every file costs two LittleFS entries, the directory and the file itself, and LittleFS charges a whole block per entry — so a board opts in where it has the room rather than paying for the surface it does not use.

**What earns a file.** A *setting you configure*, not *state you operate*. MQTT's publish and subscribe topics are rewritten at run time by the portal and the IoT API, so they stay in the record store and get no keys; the same reasoning keeps GPIO and device-IoT out of `/etc` altogether. A key here is one an administrator sets and the device then honours.

#### Runtime service enable

Which services run is a runtime decision, and it lives in **one** file rather than in each feature's:

```
# /etc/service.conf — one line per service
mqtt no
email yes
```

The key is the service's own name, lowercased. That keeps the whole enable surface to a single filesystem entry however many services a build has, and leaves a feature's own conf holding only its settings.

Not everything under `/etc` is a config file. `/etc/passwd` and `/etc/shadow` are the user store's, `/etc/ssh` holds host keys beside `ssh.conf`, `/etc/hosts` is the resolver's, and `/etc/rc.local` is a script rather than a table of keys — it is read line by line and run, not parsed for settings ([§7.15](#715-shell-scripts-and-etcrclocal)).

A service name is therefore one word. It is what you type at `service`, whose arguments split on spaces, and it is the key in a file whose lines split at the first space — so a name with a space in it is a name you can read in `service list` and neither type nor persist.

```
service list                # SERVICE  STATE  ENABLED  TASKS  R/S/Z
service status MQTT         # state, enabled, and the file its settings come from
service disable MQTT        # persists; takes effect on the next boot
service enable MQTT         # persists; starts on the next boot
service stop MQTT           # tears it down immediately, leaving the conf files alone
service restart WiFi        # stop and start in place, without rebooting the device
```

Everything but `list` and `status` is root-only. A service the device cannot be recovered without — the database, the command line, the serial console, auth, the user store and factory reset — reports `enabled yes` and refuses to be turned off, from the command *and* from a hand-edited `/etc/service.conf`. On a board with one console there is no equivalent of walking to the machine.

#### Two views of one setting

A conf file and the web portal are two windows onto the same value, so both are held to the same rule. Each of the four settings files above is `0600 root:root`, and the portal pages that change one require a root session — a non-root login is answered with a "root privilege required" page instead. Editing `/etc/mqtt/mqtt.conf` and saving the MQTT form are the same privilege; see [8.5](#85-middleware).

Saving from the portal writes the record store *and* the file, and a service reads the file back at init, so an edit made either way survives a restart of that service.

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

Independently of mode, each task carries a priority, a nice value and a policy. Together they decide who goes first when several inline tasks come due on the same tick.

Selection runs in three stages, and the first one that separates two tasks decides between them:

```
  1. due?          a task whose interval has elapsed beats one whose has not
  2. vruntime      of two equally due tasks, the one that has taken less service
                   — and policy and priority both feed the weight that charges it
  3. policy score  only when both of the above tie
```

**Due-ness is a gate, not a term.** Nothing about priority lets a task run before its interval has elapsed, so a 300-second task cannot sit at the head of the queue keeping a 1 ms listener off the CPU.

**Virtual runtime is what priority actually buys.** Each task carries `m_vruntime`, and after every run it is charged the time that run took, divided by the task's weight:

```
  vruntime += exec_µs × TASK_WEIGHT_NOMINAL / task.m_weight
```

A heavier task is charged less for the same microseconds, so it comes back round sooner. Priority therefore changes the *rate* a task accrues debt, never granting it a standing advantage — which is what makes starvation unreachable rather than merely unlikely. Period-awareness falls out for free: a 10 ms task is charged a hundred times as often as a 1 s one, with no rule about periods anywhere.

The weight is held on the task and recomputed only when priority, nice or policy changes — `setTaskPriority`, `setTaskNice` and `setTaskPolicy` own that, so no caller can move one of the three without the weight following. Nothing recomputes it per run.

`taskWeight()` maps effective priority — `priority - nice`, clamped to 0..`MAX_TASK_PRIORITY` — onto a geometric ladder, computed rather than tabulated so no table lands in RAM on a small board:

```
  TASK_WEIGHT_LEVELS   = MAX_TASK_PRIORITY + 1        one step per priority
  step ratio           = (LEVELS + TASK_WEIGHT_SPAN) / LEVELS
```

Both derive from `MAX_TASK_PRIORITY`, so raising the ceiling adds steps without stretching the ladder: `(1 + k/n)^n` tends to `e^k`, which makes `TASK_WEIGHT_SPAN` the natural log of the whole ladder's span whatever the level count. At 99 that is a ratio of 109/100 and a span of 9 → 69,531, about 7,700×.

**One step per priority means every `renice` registers.** The named rungs land at `LOW` 27, `MEDIUM` 939, `HIGH` 8,068 and `MAX` 69,531 — so a task at `MEDIUM` is charged about 35× less than one at the `LOW` default, and a `renice` of 1 moves any task about 9%.

A newly registered task starts at the scheduler's `m_min_vruntime` rather than at zero, so it cannot arrive owing nothing and monopolise the CPU until it catches up. That floor only ever moves forward.

**Policy selects the weight**, the same way a scheduling class does rather than by adding a term to a score. That is what keeps it effective: vruntime is an accumulator, so two tasks are almost never exactly level on it, and anything that only broke ties would never be consulted.

| Policy | Weight | Effect |
|---|---|---|
| FIFO (default) | the priority ladder | priority sets the rate the task accrues debt |
| fair share | the priority ladder | the same rate; differs from FIFO only in the tie-break below |
| round robin | **nominal, ignoring priority** | every round-robin task accrues at one rate, so peers get equal slices whatever their priority |
| deadline | the ladder × `TASK_WEIGHT_DEADLINE_BOOST` | charged that much less, so it comes back round that much sooner |

Round robin not consulting the ladder is the point of it — a round-robin task at `LOW` and one at `HIGH` get the same share. Use it where several tasks must progress together and priority would otherwise separate them.

Only when due-ness **and** vruntime leave two tasks level does the blended score break the tie, on lateness, recentness or cumulative CPU according to the policy. Policies apply to the inline lane; contextual lanes schedule themselves.

Priorities are larger-is-more-important, in the range 0..`MAX_TASK_PRIORITY` (99), with `HIGH_TASK_PRIORITY` 74, `MEDIUM_TASK_PRIORITY` 49 and `LOW_TASK_PRIORITY` 10 as the named rungs. `DEFAULT_TASK_PRIORITY` is `LOW`. Nice runs the other way, POSIX-style, and is subtracted.

Deadline working through the weight is what makes it period-independent: a 10 ms task gets the same proportional lift as a 1 s one, because nothing in the charge depends on how far the task is from its deadline.

### 4.3 What a task record holds

A task is a POD stored by value in a vector reserved to `MAX_SCHEDULABLE_TASKS`, so steady-state scheduling causes no heap churn.

| Group | Fields | Why |
|---|---|---|
| identity | id, name, owner | pid, a read-only name pointer (often in flash), and the owning session — 0 means kernel. Ids count up and start again at `MAX_TASK_ID`, stepping over any still in use, so a caller holding the id of a task that has ended cannot reach whatever registered next |
| callback | the function | what actually runs |
| schedule | duration, last run, remaining attempts, priority, nice, policy, mode, virtual runtime, weight | everything selection needs. Weight is derived from priority, nice and policy, and held rather than recomputed each run |
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
| change the priority | `setTaskPriority(id, prio)` — clamped to `MAX_TASK_PRIORITY` |
| change the policy | `setTaskPolicy(id, policy)` — see [§4.2](#42-policies) |
| queue a signal on one task | `sendSignal(id, sig)` |
| signal every task with a name | `sendSignalByName(name, sig, requester, is_root)` — this is what `pkill` and `killall` use |
| promote a task to a lane | `scheduleUnderExecSched(sched, id, mode, stack)` |

### 4.5 What happens on a tick

```
  run() ──▶ handle_tasks()
             │
             1. now = millis
             2. order every task — due first, then least virtual runtime,
                then policy score — into an index table (the vector never
                moves, so task ids stay stable)
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
                  │        update exec time, total time, run count, state,
                  │        and charge the run against the task's weight
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

Due-ness itself is an exact comparison against `millis_now()`. The 3 ms tolerance applies only in the deepest tie-break, where two tasks are equally due, level on virtual runtime and level on score, so clock jitter no longer reshuffles ordinary task order.

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

Stack sizing is on you unless the port can measure it. A KiB for cooperative and two for preemptive is a reasonable starting point on a constrained device; raise it if the task touches `pdiutil::string`, parses JSON, or goes through TLS.

Across lanes: `sleep(ms)` and `yield()` are called from inside a cooperative task (the inline lane's `sleep` is deliberately a no-op), a preemptive task can yield voluntarily even though the ISR will preempt it anyway, and any data shared between lanes needs `iMutex` or `iConditionVar`. The inline lane needs neither.

### 4.9 Watching and steering it from the shell

`ps` prints every registered task — pid, owner, state (`R` running, `S` sleeping, `T` stopped, `Z` zombie), priority, nice, policy, rolling %CPU, run count, interval and name — and `ps <sid>` filters by owner. `top` re-renders the same view on a scheduler tick and stops on Ctrl+C. `watch` wraps any other command on an interval.

Both read `/proc` rather than reaching into the scheduler, so what a command prints and what a file says cannot drift apart. A build with no filesystem keeps `ps` and `top` by falling back to the scheduler directly, through the same renderer.

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
  service list             every service: state, enabled, and task count
  service status <name>    that service's state, config file and the pids it owns
  service start <name>     bring it up from cold
  service stop <name>      real teardown: listener, connections and every tracked task
  service restart <name>   stop, then start
  service enable <name>    persist "<name> yes" in /etc/service.conf
  service disable <name>   persist "<name> no"
```

All of these except `list` and `status` need root. Ownership is tracked per service rather than per task name, so renaming a task never breaks service control. None of the verbs signal a task: `stop` releases what the service holds, so `service stop SSH` gives up port 22 and closes its sessions.

`stop` and `restart` refuse two services: an essential one, and the service carrying the session the command is typed on — otherwise `service stop SSH` over SSH would drop the connection mid-command. The essential set is time, database, serial, auth, user store, factory reset and cmd — a board must not be able to talk itself out of a console, a clock or an identity. `start` and `restart` refuse while a service the target `Requires` is inactive, and name it.

Each service's own start and stop record its state: `active` once its init reports success, `failed` when that init says it did not come up, `inactive` before the first start and after a stop. A service that merely holds a task does not count as running. `enable`/`disable` decide what happens on the next boot and are the pair that survives a restart — see [§3.10](#310-the-etc-config-surface).

---
## 5. Database Layer

This is where framework configuration goes to survive a reboot. It is not a key-value store, not relational, and not a filesystem — it is a small record store sized for the few kilobytes a small MCU can spare, holding the critical state a device cannot be reconfigured without: credentials, WiFi, GPIO, OTA, MQTT, email, IoT. Everything else is human-readable config under `/etc/<feature>/<feature>.conf`.

It follows the same three tiers as everything else:

```
  service   DatabaseServiceProvider     get_*_table / set_*_table, tier and factory-reset wiring
      │                                 ← the only surface other services touch
  engine    Database + DatabaseTable     registry of what exists and how big it is
            DbLayout                     superblock, directory, records, sealing
      │                                 ← decides where a record lives
  port      iDbStoreInterface            flat byte device: read, write, flush, capacity
            iDatabaseInterface           the eeprom underneath one of those stores
```

The per-table classes are generated from a JSON schema and live in `src/database/tables/`. Nothing there is hand-edited.

### 5.1 Mental model

A table is identified by an **id that never changes and is never reused**. Nothing declares a byte address. The engine lays the records out itself:

```
  0        5            17                        212                        end
  ├────────┼────────────┼─────────────────────────┼──────── ... ──────┬───────┤
  │reserved│ superblock │ directory[MAX_TABLES]   │ records, packed   │ free  │
  └────────┴────────────┴─────────────────────────┴──────── ... ──────┴───────┘
```

The superblock carries a magic, the format revision, the firmware version, the launch year and a checksum. The directory is fixed at `MAX_DB_TABLES` entries — so adding a table never shifts a record — and each entry holds the table id, the version that wrote it, the offset, the slot capacity, the bytes actually written, a checksum and a flag byte.

Because placement is derived, a struct that grows does not push a neighbour into it. On the next boot the engine notices the geometry no longer matches, repacks by id, and carries every payload across.

### 5.2 Two tiers

A device with storage runs on a container file and keeps the eeprom as the defaults behind it:

| tier | medium | role |
|---|---|---|
| live | `/etc/database/pdi.db`, `0600 root` in a `0700 root` directory | everything the device is running |
| defaults | eeprom | what it falls back to when the container is wiped or reset |

`resolve_tiers()` prefers the container and falls back to running directly on the eeprom when there is no storage service or the container cannot be opened. A container that never existed, or one that was wiped, is rebuilt from the eeprom defaults on the next boot.

The eeprom is opened only for the length of a defaults copy, so on a device running the container tier it holds no RAM at all — a few KB that stays available to everything else.

A single container file is used rather than one file per table: LittleFS runs a 4096-byte block and a 64-byte inline limit, so most tables would each claim a whole block. One container costs one block, and every write is an in-place `editFile()` at its offset, which keeps the mode and owner the file was created with.

### 5.3 The ports

`iDbStoreInterface` is a flat byte device — `read`, `write`, `flush`, `capacity`, plus `init`/`deinit` for opening and releasing the medium. Two implementations ship: `EepromDbStore`, which compares bytes before staging them so an unchanged record never dirties the medium, and `FsDbStore`, which keeps the container at its full length so a read never runs past the end of the file.

Underneath the eeprom store, `iDatabaseInterface` is what a board port provides: `readByte`, `writeByte`, `commitConfigs`, `beginConfigs`, `endConfigs` and the region size. The singleton is `__i_db`.

### 5.4 The engine

Tables register themselves before `main()` runs. Each generated table inherits an abstract layer whose constructor pushes the instance into a static array, so by the time the database service starts it already knows every table in this build.

```cpp
class WiFiTable : public DatabaseTable<WIFI_TABLE_ID, wifi_config_table, 1, true> {};
extern WiFiTable __wifi_table;
```

The template arguments are the id, the backing struct, the struct's layout version and whether the record holds a credential. The template supplies `boot()` — which registers id, size, version and secrecy — plus typed `get`, `set` and `clear` that go through `DbLayout`.

`__database` is the registry and enforces two rules: a table must carry a non-zero id nobody else has taken, and the count must stay within `MAX_DB_TABLES`. `DbLayout` then decides whether they fit the medium. A table that fails either shows up as a `SysLogE` at boot and a table that refuses to persist.

A table registering itself a second time keeps its entry rather than colliding with it, so a service that initialises more than once — a restart, a factory reset — comes back with its tables intact. Only a *different* table reaching for an id already held is a conflict.

Whether the records fit at all is settled at build time — `src/database/core/DatabaseLayout.h` sums every record, seals included, and fails the build against both the eeprom and the container.

### 5.5 The service

`DatabaseServiceProvider` is what the rest of the framework calls. It defines the table globals, each behind the matching `ENABLE_*` flag; it resolves the tiers at boot; and it exposes one typed `get_*_table` / `set_*_table` pair per persisted struct, plus `save_defaults()` and `restore_defaults()`.

Services go through those accessors rather than touching a store or a table global directly. That keeps the dependency one-way: a service depends on the database service, never on the port or the generated layout.

### 5.6 Where the tables come from

```
  devices/<board>/config/DBTableSchema.json      ← you edit this
        │
        │   DeviceSetup.py -d <board>
        │     └─ CreateDBSourceFromJson.py
        │          └─ JsonToCpp.py
        ▼
  src/database/tables/<TableName>.h              ← generated, never edited
```

Each `defItems` entry names the C++ class, the backing struct alias, the id macro and its value, the struct version, whether the record is sealed, and the global the framework links against. A generated table is a body-less subclass of the template.

| Table | Id | Backing struct | Sealed | Present when |
|---|---|---|---|---|
| Login | 1 | `login_credential_table` | yes | auth or web server |
| WiFi | 2 | `wifi_config_table` | yes | WiFi |
| OTA | 3 | `ota_config_table` | no | OTA |
| GPIO | 4 | `gpio_config_table` | no | GPIO |
| MQTT general | 5 | `mqtt_general_config_table` | yes | MQTT |
| MQTT LWT | 6 | `mqtt_lwt_config_table` | no | MQTT |
| MQTT pub/sub | 7 | `mqtt_pubsub_config_table` | no | MQTT |
| Email | 8 | `email_config_table` | yes | email |
| Device IoT | 9 | `device_iot_config_table` | yes | IoT |

Global configuration is not a table. The firmware version and launch year live in the superblock, reached through `get_global_config_table` / `set_global_config_table`.

### 5.7 Read and write semantics

Reads are whole-struct — put a default-constructed `T` on the stack, read into it, change what you need, write it back. Passing a default-constructed struct matters: a record written by a shorter, older version of the struct only covers the bytes it had, and everything past that keeps the default it came in with.

That is the growth rule, and it is the reason **new fields go at the end of a struct**. An appended field needs no migration code at all. Anything else — a field changing meaning or moving — bumps the table's version and overrides `migrate()`. A struct that *shrank* returns the record to its defaults, since the stored bytes no longer describe it.

Writes land immediately; there is no journal. Structs are `memcpy`'d raw, so an image belongs to the toolchain that wrote it. The engine is single-threaded; if contextual execution is on and two lanes touch the same table, guard it with a mutex.

### 5.8 Sealed records

Sealing is a device capability. `ENABLE_DB_SEALING` resolves against `DEVICE_SUPPORTS_DB_SEALING`, so a board that cannot spare the RAM the ciphers need — the AES key schedule and the HMAC state come to roughly 600 bytes of stack — builds without them, and every record carries a checksum instead. On a board that declares it, a table marked secret has its record sealed on the way to the medium: a fresh 16-byte nonce, AES-128-CTR ciphertext, then a truncated HMAC-SHA256 tag over the nonce and the ciphertext. The tag is checked before anything is decrypted, and before anything reaches the caller's buffer, so a tampered record cannot land on top of the defaults it is holding.

The key is 16 bytes at the top of the eeprom, above the capacity the store reports, created from the hardware RNG the first time a device boots. Nothing in the shell or over SFTP can reach the eeprom, so copying `pdi.db` off the device yields ciphertext for every credential. This does not defend against someone dumping the flash itself.

Plain records carry a checksum instead — enough to catch a rotted byte, and there is no point paying for a tag on a record with nothing to hide.

Nothing in the read or write path allocates, and the whole engine costs one 75-byte object plus at most 70 bytes of stack, measured on the tightest port. A repack moves only the records whose offset actually changes, so appending a table or growing the last one asks for no memory at all.

### 5.9 Factory reset and defaults

`db save` takes what the device is running now as the defaults the eeprom holds, which is how a provisioned device is made to come back to its provisioned state rather than a blank one. A factory reset restores those defaults; on a device with no storage tier it clears every record, which reads back as the struct defaults.

Two switches shape the behaviour. `AUTO_FACTORY_RESET_ON_INVALID_CONFIGS` runs a five-second check and fires the reset when the database will not mount. `CONFIG_CLEAR_TO_DEFAULT_ON_FACTORY_RESET` hooks the reset event to put the defaults back. The reset event is public, so any service can hook it to drop its own caches.

### 5.10 From the terminal

```
  db status     which medium is live, how much of it the records take
  db list       one line per record, sealed ones marked
  db verify     read every record past its checksum or tag
  db save       take what is running now as the defaults
  db restore    put those defaults back in use
```

`db` never prints a record's contents, so a sealed credential stays sealed.

### 5.11 Adding a table

Say you want a metrics service to persist a host and a port.

1. Define the struct in `src/config/MetricsConfig.h` following the [§3.4](#34-the-shape-of-a-service-config) shape — POD, fixed-size members, a `clear()` the constructor calls:
   ```cpp
   struct metrics_configs { char host[40]; uint16_t port; };
   using metrics_config_table = metrics_configs;
   ```
2. Take the next unused id. Ids are permanent — never reuse one a removed table had.
3. Add a `defItems` entry to every board schema that should carry it:
   ```json
   {
     "defItemName": "MetricsTable",
     "defItemDesc": "Metrics table handles metrics service configs",
     "defItemArg": "metrics_config_table",
     "defItemIdKey": "METRICS_TABLE_ID",
     "defItemIdValue": "10",
     "defItemVersion": "1",
     "defItemSecret": "false",
     "defItemExtVar": "__metrics_table"
   }
   ```
4. Regenerate with `python3 scripts/DeviceSetup.py -d <board>`.
5. Add the include, the global and the accessor pair to the database service, guarded by your feature flag, mirroring any existing table.
6. Add its size to `src/database/core/DatabaseLayout.h` so the build keeps checking that everything fits.
7. Raise `MAX_DB_TABLES` if you have run out of slots.

Adding the table does not disturb any other record — the directory has a fixed number of slots and the engine repacks by id on the next boot.

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

The global is `__<name>_service`. The constructor hands the base a `service_t` value and a flash-resident name, and the base registers it so `service` can find it without knowing what it is.

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
                      `pkill <Service>` work, and `service stop` reap the right tasks
```

The tracked list is fixed-size — six slots by default. `stopService` in the base reaps every one of them, so an override is only needed when you also have sockets or buffers to release; call the base at the end when you do write one. If you ever register a task straight on the scheduler, hand its id to `trackServiceTask(id)` so it is not orphaned on stop.

Persisted configuration always goes through the database service accessors, never `__i_db`. Cross-service reactions go through `__utl_event`, never direct calls — that is what keeps the dependency graph one-directional.

The base also offers `signalAllServiceTasks(sig)`, `countServiceTasks(...)` and the task-id iterators; those are what the `service` command renders.

A new service is runtime-toggleable for free. The base reads `enabled` from `/etc/x/x.conf` before anything starts, and `startService` itself refuses when the service is disabled, so no caller has to remember the check. Override `getServiceConfigPath` only if the feature genuinely cannot use the derived path. Override `isEssentialService()` to return true only if the device genuinely cannot be recovered without the service; that puts it beyond the reach of `service disable` *and* of a hand-edited conf file.

### 6.2 Service reference

Ordered the way the orchestrator starts them.

#### 6.2.1 `TimeServiceProvider` — `__time_service`

Always present, on every port, and **essential** — `service stop`, `restart` and `disable` all refuse it, because everything that reads the clock would quietly start reading one that had stopped. It is the single place that decides what time it is, so two readers can never disagree about which second they are in.

It ticks every `TIME_SERVICE_TICK_MS` (10 ms), reads the clock, and announces the boundaries it has crossed:

| Event | Fired when | Payload |
|---|---|---|
| `EVENT_TIME_SYNC` | validity changes in either direction | `datetime_t*` |
| `EVENT_TIME_SECOND` | the second rolls over | `datetime_t*` |
| `EVENT_TIME_MINUTE` | the minute rolls over | `datetime_t*` |

The payload is one `datetime_t` broken down once per tick: `m_epoch` is UTC, every calendar field below it is local, and `m_valid` says whether that calendar means anything. ⚠ It is the service's own instance, reused every tick — read it inside the callback, never keep the pointer.

**It is not gated on having a network.** Where the port reaches an NTP server the time is wall clock and is called valid. Where it does not, the millisecond counter still gives a clock that only moves forward, and that time is simply never called valid — so a board with no network still gets its second and minute ticks, and only the calendar *meaning* is withheld. `DEVICE_SUPPORTS_NTP` is what distinguishes the two.

Uptime is accumulated rather than divided out of `millis_now()`, which is 32-bit and wraps at about 49 days.

A minute is announced once however many the clock crossed, so an NTP correction of an hour cannot fire an hour of scheduled work. On a validity flip the second and minute markers reset — a clock that has just jumped says nothing about which boundary comes next.

Read it in code with `__time_service.now()` and `__time_service.isTimeValid()`.

#### 6.2.2 `DatabaseServiceProvider` — `__database_service`

Always present; everything else may need persisted config. It mounts NVM, boots every registered table, optionally polls config validity every five seconds, and listens for the factory-reset event. Fully covered in [§5](#5-database-layer).

#### 6.2.3 `DeviceFactoryReset` — `__factory_reset`

Always present. Polls the flash button for a six-to-seven second hold and fires `EVENT_FACTORY_RESET`. Other services listen for that event to drop their own caches before the reboot. The web portal exposes the same action as a form.

#### 6.2.4 `SerialServiceProvider` — `__serial_service`

Opens the serial port at the configured baud and hooks its input handler into the device event pump. It supplies the default terminal for the CLI, and exposes the JSON apply/append hooks that Device-IoT uses to move sensor payloads across the serial link.

#### 6.2.5 `WiFiServiceProvider` — `__wifi_service`

Configures the access point from the WiFi table, starts a station scan, and keeps a connectivity check running every five seconds. NAPT, when enabled, is switched on by a one-shot scheduled after the station link comes up. It also puts `wlan0` and `ap0` into the netif registry that backs `/proc/net` and `/sys/class/net`, once in the service's life rather than on every start, so a restart does not attempt them again.

The shell surface is `net ip`, `net scansta` and `net connsta,<ssid>,<pass>`; the portal has a WiFi page whose editability is governed by `ALLOW_WIFI_CONFIG_MODIFICATION`.

It is the framework's main event source:

```
  station connected ──▶ EVENT_WIFI_STA_CONNECTED
  DHCP / static IP  ──▶ EVENT_WIFI_STA_GOT_IP     ─┬─▶ mDNS announces, then queues cert provisioning
                                                   └─▶ services needing a stable address
  link lost         ──▶ EVENT_WIFI_STA_DISCONNECTED ──▶ MQTT, OTA, IoT stand down
  AP client in/out  ──▶ EVENT_WIFI_AP_STA(DIS)CONNECTED
  internet poller   ──▶ EVENT_WIFI_INTERNET_UP / _DOWN
```

#### 6.2.6 `OtaServiceProvider` — `__ota_service`

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

A build can also be flashed without any update server. `collectLocalImages()` lists the firmware images sitting on the filesystem and `flashFromFile()` writes one of them, checking the image magic byte before it commits and reporting through the same `upgrade_status_t` as a server-driven update. That is what backs the **Flash From Storage** form on the OTA page: upload a binary through the storage browser, pick it from the list, confirm, and the device flashes and restarts. Both entry points need the storage service.

#### 6.2.7 `GpioServiceProvider` — `__gpio_service`

Loads the GPIO table, ticks pin modes and values, and refreshes the table every five minutes. Modes are off, digital write, digital read, digital blink, analog write and analog read.

From the shell, GPIO is driven as files under `/sys` — `echo 1 > /sys/class/gpio/5/value`. The portal adds a GPIO page plus an events submenu where a pin condition can be routed to email or an HTTP endpoint.

#### 6.2.8 `MqttServiceProvider` — `__mqtt_service`

Opens the broker connection with a last-will message, subscribes to whatever the pub/sub table lists, and schedules the publish driver. Applications inject and receive payloads by registering publish and subscribe callbacks. The portal has general, LWT and pub/sub forms.

#### 6.2.9 `EmailServiceProvider` — `__email_service`

Loads the email table and, if periodic mail is enabled, schedules the send. The GPIO service uses it for event alerts. The portal page includes a test button.

#### 6.2.10 `DeviceIotServiceProvider` — `__device_iot_service`

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

#### 6.2.11 `AuthServiceProvider` — `__auth_service`

A session-aware delegator rather than a store. `isAuthorized(user, pass)` verifies against `/etc/shadow` through the user store when that file exists, and against the bootstrap credential row otherwise. `setAuthorized` and `getUsername` read and write the *current* session, so there is no global "logged in" bit — three terminals can be at three different privilege levels at once.

#### 6.2.12 Storage (interface init, no provider)

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

Each prefix is named once, in [src/config/VfsConfig.h](src/config/VfsConfig.h): `PROC_MOUNT_PREFIX`, `SYS_MOUNT_PREFIX`, `DEV_MOUNT_PREFIX` and `TMP_MOUNT_PREFIX` beside the `ENABLE_` flag for the backend that answers there, with the root at `FILE_SEPARATOR`. Code that reaches a synthetic node writes `PROC_MOUNT_PREFIX "/mounts"` rather than the path in full, so moving a mount is one edit.

**procfs** nodes are all `0444` and root-owned; writes fail, and a redirect into one says `cannot write <path>` rather than appearing to succeed. `/proc/uptime` gives seconds since boot in the Linux two-number layout, `/proc/version` gives the release and config version, `/proc/meminfo` the free heap and the largest block it can still hand out, `/proc/mounts` a line per mount, and `/proc/stat` the scheduler's cumulative counters. Every running task has a directory of its own — `/proc/<pid>/{stat,cmdline,status}` — and `/proc/net/{route,dev,tcp}` describes the network: the routes each interface offers, the traffic each has carried, and every TCP endpoint the stack holds, listening and connected alike. Everything that reads files works on them — `cat`, `head`, `wc`, `grep`, `hexdump`.

`/proc/net/route` carries one row per route rather than one per interface, in the `route -n` columns with a `Flags` field. An interface that holds a netmask contributes the network it reaches directly — destination `ip & netmask`, no gateway, flag `U` — and one that holds a gateway contributes the default route as well, `0.0.0.0/0.0.0.0` through it, flag `UG`. An access point therefore appears once, for its own subnet, and an interface that is up but has no address yet appears not at all, because it has nowhere to route.

`/proc/net/dev` and `/proc/net/tcp` each fill in only as far as the port can answer. Counters come from the link, and a link that cannot count is left out rather than listed with zeroes that would read as an idle interface — a prebuilt network stack often has its statistics module compiled out. Endpoints come from the stack, and a port that cannot enumerate one leaves the columns empty rather than reporting a device with nothing listening. Both are port capabilities, so a link or stack that can answer fills the node in without anything above changing.

Columns are fixed width rather than tab separated, here and in every command that prints a table: a tab only reaches the next eight-column stop, so one long field would push the rest of the row out of line on a terminal that does no reflowing of its own.

`ps`, `top`, `mount` and `df` read through these nodes rather than reaching into the scheduler or the mount table, so what a command shows and what a file says cannot disagree. `df` takes its list of mounts from `/proc/mounts` and asks the backend behind each prefix for its sizes.

**sysfs** carries GPIO and the network interfaces:

| Node | Mode | Content |
|---|---|---|
| `/sys/class/gpio/<pin>/value` | `0666` | current reading, or the value to drive |
| `/sys/class/gpio/<pin>/mode` | `0666` | `0` off · `1` digital write · `2` digital read · `3` blink · `4` analog write · `5` analog read |
| `/sys/class/net/<iface>/address` | `0444` | the interface's hardware address |
| `/sys/class/net/<iface>/operstate` | `0444` | `up` or `down` |
| `/sys/class/net/<iface>/{ip,netmask,gateway}` | `0444` | its current addressing |
| `/sys/class/net/<iface>/{ssid,rssi}` | `0444` | the association, on a station only |

The radio presents itself as `wlan0` and `ap0`. A link type registers into a table the same way a filesystem registers into the mount table, so Ethernet or PPP appears here by registering rather than by anything under `/sys` changing.

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

#### 6.2.13 `WebServer` — `__web_server`

Started with the HTTP server interface and ticked from every pass of `serve()`. With HTTPS on, `initService` sets the certificate, key and — under mTLS — client CA paths, then binds 443 with TLS enabled and also opens a plain listener on 80 that answers every request with a redirect to the matching `https://` URL, so a browser that arrives on `http://` is carried up to the secure portal; without HTTPS it binds 80 directly. It has its own router, middleware chain, controllers and session handling, all covered in [§8](#8-web-server).

#### 6.2.14 `TelnetServiceProvider` — `__telnet_service`

Binds port 23, accepts clients into a pool of `TELNET_MAX_SESSIONS` slots, and hands each stream to the CLI as a terminal. Everything after that is the shell. A slot whose client has gone is reclaimed before the next accept, an idle session is closed after `TELNET_SHELL_IDLE_MS` while one running a command is left alone, and a client arriving when every slot is taken is told so and closed after `TELNET_POOL_FULL_GRACE_MS` rather than left waiting on a prompt that will not come. The sizes live in [src/config/TelnetConfig.h](src/config/TelnetConfig.h).

#### 6.2.15 `SSHServer` — `__sshserver_service`

The most expensive service in the framework, and the most capable: a full SSH server with an SFTP subsystem, written on the framework's own crypto.

```
  version exchange
        │
  KEXINIT ──▶ pick host-key algorithm from the keys the device holds
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

Host keys live in `/etc/ssh` alongside `ssh.conf`, leaving `~/.ssh` to the user's own client keys. The Ed25519 host key is created on service start if it is missing, which takes milliseconds. RSA is generated only when asked for with `sshkgen t=2,f=b`, because 2048-bit keygen on these parts is measured in minutes.

Both authentication methods are on by default and each can be switched off in `/etc/ssh/ssh.conf`, which is created with defaults on first boot. When an attempt fails the server advertises exactly the methods still permitted. [§7.10.1](#7101-ssh-authentication) has the operational detail.

Two sessions are served concurrently by default, which is what graphical SFTP clients need — they hold a browse connection open and open a second one to move a file. The SFTP subsystem covers path resolution, stat, directory listing, open/read/write, mkdir, rmdir, remove and rename, which is enough for interactive `sftp`, for editing a remote file in FileZilla or WinSCP, and for `scp -s`.

#### 6.2.16 `CommandLineServiceProvider` — `__cmd_service`

Registers every command and owns the binding between terminals and sessions. Attaching a terminal creates or finds its session and draws the login prompt; each input tick looks the session up from the terminal and makes it current before dispatching. Every in-flight command remembers which session owns it, so one session's half-finished prompt is never handed to another.

Up to `PDI_MAX_SESSIONS` sessions run at once across serial, telnet and SSH, each with its own line buffer, cursor, history position, working directory, umask and identity — and telnet and SSH each hold a pool of their own, so several remote shells of the same kind can be open together. The table is sized from those pools: `PDI_MAX_SESSIONS` defaults to one console plus `TELNET_MAX_SESSIONS` plus `SSH_MAX_SESSIONS`, and a board that names a smaller number in its device config fails the build rather than leaving one transport unable to claim the slots its own pool promises. [§7](#7-command-line--terminal) is the full CLI reference.

The same service runs scripts. One line of a script goes through the same parse-and-dispatch step a typed line does, so `source` and `/etc/rc.local` get the whole grammar without a second one; what the service wraps around a typed line — history and the prompt — happens once for the line you typed rather than once per script line, while finished commands are released after every line either way, so a long script does not hold every command it ever ran. Where a script has got to is held by the session, not by the call stack, which is what lets a line that stops for input hand the terminal back and be picked up afterwards. `/etc/rc.local` runs at the end of `initialize()`, before the login prompt is drawn. See [§7.15](#715-shell-scripts-and-etcrclocal).

#### 6.2.17 `CronServiceProvider` — `__cron_service`

Starts after the cmd service, because a job is a command line. It keeps no clock and no table of its own: it listens on `EVENT_TIME_MINUTE`, and on each minute the clock calls valid it streams `/etc/crontab` a line at a time, matching each row against the instant the event carried and running whatever is due. Nothing is held between minutes, so an edit to the table takes effect on the next one and a board carries no memory cost for a feature it is not using.

It registers **no scheduler task**, only a listener — which is why it keeps that listener's id and gives it back in `stopService()`. A service that listens rather than scheduling would otherwise keep reacting after being stopped, and gain a second listener on every restart ([§6.4](#64-the-event-bus)).

At init it seeds `/etc/crontab` with its column names when none is there, `0600 root:root`, so there is always a file to `fedit`. Jobs run as root, so a table anyone could write would be a way to schedule root commands as anyone.

Full reference, including the field grammar, in [§7.16](#716-scheduled-jobs-and-etccrontab).

#### 6.2.18 TLS (no provider; transport hookup + cert provisioning)

TLS has no service class either — it lives at the interface and port level. `iInstanceInterface` hands out the one outbound client the services share, built on first call and kept: `getSharedTcpClientInstance()` and, in a TLS build, `getSharedTlsClientInstance()`.

Most callers ask for neither by name. `getSharedClientForScheme()` takes the protocol you are speaking — `http` or `https`, `mqtt` or `mqtts`, `smtp` or `smtps` — and gives back the right client for it. The address already says which one it needs, so whoever holds the address makes the choice: the HTTP client looks at each URL as it goes, even when a redirect changes it half way.

**A secure address only works on a board that can do TLS, in a build where it is switched on.** Anywhere else there is no secure client to give back. An HTTP request then fails with an error instead of going out unencrypted; MQTT drops back to a plain connection so a broker that was already working keeps working. Both write a line to the log either way, so you can see which happened. Plain `http` and `mqtt` keep working everywhere with a network.

A service that keeps a connection open for a long time holds its own client instead of sharing, because the shared one gets swapped whenever someone else asks for a different protocol. MQTT builds its own from the broker setting: if you have put the broker's certificate at `/etc/ssl/ca-bundle.crt` it checks the broker really is who it claims to be, and if you have not, the traffic is still encrypted but nobody has checked who is on the other end. The log says which of the two you got. **Email is plain only** — the SMTP path has no TLS support.

Each port brings its own TLS backend, named alongside `ENABLE_TLS_SERVICE` in [§3.3.1](#331-service-flags). Nothing above the interface knows which one it has. Certificates and keys are read from the filesystem at runtime, defaulting to `/etc/http/server.crt`, `/etc/http/server.key`, `/etc/http/client-ca.crt` and `/etc/ssl/ca-bundle.crt`.

Handshakes need more stack than a main context typically has, so enabling TLS also enables contextual execution and runs the engine on its own cooperative task with a stack sized by `TLS_TASK_STACK_SIZE`.

The bundled outbound client is created with peer verification off so that an encrypted-but-unverified connection works immediately. For production, point it at the CA bundle path and drop that line.

**Certificates come from one of two places, and which one is a port capability.** A port declaring `DEVICE_SUPPORTS_TLS_CERT_GENERATION` can issue its own: the on-device provisioner writes self-signed EC or RSA certs with the SANs you ask for, `ensureServerCert` reissues only when the stored certificate's SANs no longer cover what was asked for, and the `tls` command exposes the same issuer from the shell. With runtime generation enabled the mDNS service drives it, so the certificate covers the address and `<hostname>.local` together ([§6.2.21](#6221-mdnsserviceprovider--__mdns_service)).

**Without that capability there is no `tls` command and nothing issues a certificate on the device.** Generate the material off-device with `scripts/GenTlsCerts.py`, which does the same job with OpenSSL, and upload the result to the paths above over SFTP ([§7.10](#710-sftp-and-scp-file-transfer)). Outbound verification needs the same treatment — a CA bundle at `/etc/ssl/ca-bundle.crt` is something you put there.

**TLS is also the flag most likely to need room made for it.** A live session costs more than any other feature here, per session rather than per build, and on a device whose heap is already committed the handshake is what fails first. Expect to turn other services off to fit it — the per-session figures and what to trade against them are in [§12.3](#123-the-expensive-features).

#### 6.2.19 `UserStoreService` — `__user_store_service`

The user directory, in two files:

```
  /etc/passwd   0644   username:x:uid:gid:home:shell        world-readable — id and groups need it
  /etc/shadow   0600   username:hexhash:hexsalt             SHA-256 of salt‖password, 8-byte salt
```

The API is what you'd expect — look up by name or uid, add a user (writing both files, rolling the passwd row back if the shadow write fails), remove a user, verify a password with a constant-time compare, set a password.

On first start, if `/etc/passwd` is absent, it seeds root from the bootstrap credential row and hashes that password into shadow. The seeding is idempotent, and every boot re-stamps shadow as `0600`, so a file that turns up readable is corrected rather than trusted.

Verifying or changing a password has to read shadow on behalf of a non-root session, so those two methods — and only those two — bracket the access in the privileged scope described in [§6.2.12](#6212-storage-interface-init-no-provider). `useradd` gives each user their own group by setting gid to uid.

It initialises after the filesystem and before the CLI, and the auth service prefers it whenever shadow exists. That preference is the switch that turns the framework from single-credential auth into a multi-user system.

#### 6.2.20 `SessionManager`

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

#### 6.2.21 `MdnsServiceProvider` — `__mdns_service`

The responder from [§2.4.2](#242-mdns-and-dns-sd), running as an ordinary service on the stack's raw UDP API. It derives the hostname from the MAC, writes `/etc/hostname`, joins the multicast group when the station gets an IP, and advertises whichever servers this build is running. Responses bundle PTR, SRV, TXT and A so a single query gets everything it needs. `service status MDNS` shows what it is announcing.

Holding both the address and the name also makes it the right owner of HTTPS certificate provisioning: with `ENABLE_SERVER_TLS_CERT_GENERATION_AT_RUNTIME` it schedules `ensureServerCert` for `<hostname>.local` plus the IP, one queued job at a time, dropped again if the service stops ([§6.2.18](#6218-tls-no-provider-transport-hookup--cert-provisioning)).

### 6.3 Init order

The orchestrator starts services in a deliberate order:

```
   1  time            the clock, before anything that stamps a line with it
   2  database        every other service may need persisted config
   3  serial          gives boot messages somewhere to go
   4  wifi            brings the network up
   5  ota             may pre-empt the rest of startup if an update is queued
   6  gpio            can raise alerts once http and email exist
   7  mqtt            may publish boot status
   8  email           used by gpio events
   9  factory reset   last of the always-on set, so every listener is registered
  10  device iot      needs MQTT up
  11  auth            gates everything the CLI and portal expose
  12  storage         filesystem up — SSH depends on it
  13  web server      needs auth and storage
  14  telnet, ssh     need network, storage and the CLI
  15  cmd             last, so `service` sees a complete list
  16  cron            after cmd, because a job is a command line
```

Two things follow from that. A service started later may call into one started earlier; the reverse is not defined. And a service that needs another one in its *constructor* is relying on static-init order — move the lookup into `initService`.

### 6.4 The event bus

Direct calls are reserved for dependencies that are known to already exist. Anything fan-out goes through `__utl_event`:

| Event | Fired by | Typical listeners |
|---|---|---|
| `EVENT_FACTORY_RESET` | factory reset | database clears tables, IoT drops cache, GPIO resets pins |
| `EVENT_WIFI_STA_CONNECTED` / `_DISCONNECTED` | WiFi | MQTT reconnects, OTA and IoT stand down |
| `EVENT_WIFI_STA_GOT_IP` | WiFi, once the address latches | mDNS, which also queues cert provisioning; anything needing a stable address |
| `EVENT_WIFI_AP_STACONNECTED` / `_STADISCONNECTED` | WiFi | captive-portal flows, per-client tracking |
| `EVENT_WIFI_INTERNET_UP` / `_DOWN` | connectivity poller | OTA, IoT, email |
| `EVENT_SERIAL_AVAILABLE` | serial bridge | sketch hooks |
| `EVENT_TIME_SYNC` | time service, when validity changes | anything that must recheck a stored timestamp |
| `EVENT_TIME_SECOND` / `_MINUTE` | time service, on the boundary | cron, and anything scheduling against a wall clock |

That is the whole list — [src/config/EventConfig.h](src/config/EventConfig.h) is the one place event names are declared, and `EVENT_NAME_MAX` closes it. Not everything that fans out uses the bus: GPIO alerting does not, because a pin crossing its threshold has exactly one destination configured for it, so the GPIO service holds its own HTTP client and calls email and MQTT directly rather than publishing for nobody in particular.

Subscribe with `__utl_event.add_event_listener(name, [&](void* e){ … })`, publish with `__utl_event.execute_event(name, ptr)`.

**Subscribing returns an id, and that is how a listener is taken back.** `add_event_listener` hands back an `int16_t`, `EVENT_LISTENER_ID_INVALID` if the table was full; `remove_event_listener(id)` releases it. A service that listens rather than scheduling a task needs this: the base `stopService()` reaps tracked *tasks*, so without releasing its listener a service would keep reacting after it was stopped, and would gain a second listener on every restart. Keep the id and give it back in `stopService()`, the way cron does.

Removal **marks the slot rather than erasing it**, and a later subscription takes the slot over. That is deliberate: `execute_event` calls each handler through the table entry, so erasing during a dispatch would destroy the callable while it is running — which is exactly what a job that stops its own service would do.

### 6.5 Writing a new service

Say you want a metrics service.

1. Add `ENABLE_METRICS_SERVICE` to the device config and a `service_t` value behind the same flag.
2. Add the persisted struct ([§3.4](#34-the-shape-of-a-service-config)) and its table ([§5.11](#511-adding-a-table)).
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
   That alone makes it visible in `ps`, killable with `pkill Metrics`, and controllable with `service stop Metrics`.
4. Include it and call its `initService` from the orchestrator, in the right slot per [§6.3](#63-init-order).
5. Implement the two print hooks so `service` has something to show, and add a command or a web page if it deserves one.
6. React to other services through events rather than by calling them.

Two habits keep services well-behaved. Do the real work in `initService`, not in the constructor — at construction time the device globals do not exist yet. And prefer value members or static buffers to heap allocation, since these devices stay up for months at a time.

---
## 7. Command Line / Terminal

The shell is the universal control plane. The same commands are reachable over serial, over telnet on port 23, and over SSH on port 22, with login, history, tab completion, pipes and redirection, variables, scripting, in-place editing and file transfer. One implementation covers every channel because each source presents itself as an `iTerminalInterface`, and the CLI binds one session per terminal.

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

Anything that wants to feed the CLI implements `iTerminalInterface` — byte-level reads and writes overloaded for every primitive type, plus the terminal affordances. Four implementations ship: the serial port, the client object a TCP server hands back for telnet, the SSH channel wrapper, and `__i_null_terminal`.

The null terminal swallows everything written to it and never has anything to read. It exists because a session is bound to a terminal, so work that must run under a session with nobody watching — a cron job, say — would otherwise have to borrow the console's terminal, and would be refused whenever somebody was logged in on it. Attaching to the sink instead makes such work independent of who is signed in.

When a telnet or SSH client connects, its service calls `useTerminal(client)`, which attaches a fresh session and draws a login prompt on that client alone. Serial holds its slot from boot. Nothing leaks between sessions.

### 7.3 Input sequences

Line editing happens in-process, so the CLI recognises control sequences byte by byte:

| Sequence | Action |
|---|---|
| Enter | submit the line — `\r`, `\n`, or the two in sequence all count as one |
| Backspace, Delete | edit |
| ←, → | move within the line |
| ↑, ↓ | walk history (needs storage) |
| Home, End | jump to line start or end |
| Page Up, Page Down | scroll long output |
| Tab | complete the word under the cursor; again to list the candidates |
| Esc | cancel the line; inside `fedit`, open the save/cancel/delete menu |
| Ctrl+C, Ctrl+Z | abort the running command |

Clients disagree about what Enter is: a raw serial terminal sends `\n`, telnet sends CR LF or CR NUL. Every ending is accepted, and a pair is taken as the single key press it represents rather than as a key press followed by an empty line — the two halves may arrive in separate reads, so which ending was taken is remembered on the session.

Only real characters reach the line. Control bytes the editor has no meaning for, and the stray `0xff` a board can emit as it comes out of reset, are dropped rather than typed — so a line that starts arriving mid-reset is still the line you meant.

A long-running command receives these mid-execution by overriding `executeTermInputAction`.

**Completion.** Tab completes the word the cursor is in, and the word is found through the same tokenizer the parser uses, so the rest of the line is left exactly as typed and completion works mid-line as well as at the end. The first press types as much as every candidate agrees on; a single match is finished off and followed by a space. When nothing more can be typed, a second press lists the candidates in columns below and redraws the prompt and the line.

What is offered depends on where the word sits. A word in command position — the start of the line, or after `|`, `&&` or `;` — completes against the command names. A redirect target completes against paths. Anything else is an argument, and the command decides what its arguments name by overriding `completionFor`:

```cpp
/* argument 0 is the subcommand, everything after it names a service */
cmd_complete_t completionFor(uint8_t argindex) const override {
    return 0 == argindex ? CMD_COMPLETE_NONE : CMD_COMPLETE_SERVICE;
}
```

Commands that say nothing complete paths, which suits most of them. A path is completed one component at a time: the word splits at its last `/`, the leading part names the directory to read, and a directory match gains a trailing `/` so the next component can be typed straight away. On a board with no filesystem there is no grammar to read the line with, so completion there offers command names and nothing else.

An empty line completes nothing. Every command would match, and a console has nothing to page a screenful of them with.

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
      │        └─ split on the option separator (',' by default, ' ' for a command
      │           that reads positionally) outside any quotes, then match each key
      │           to a declared option. Never separate on ';' — that is the shell's
      │           own command separator, and a command using it loses every option
      │           after the first
      └─ command name, up to the first space

   "chmod 0644 /etc/passwd"
      └─ a command that opted into positional args gets them in declaration order
```

Inside `execute`, options come back by name:

```cpp
auto pin = RetrieveOption(CMD_OPTION_NAME_P);
if (pin == nullptr) return CMD_ERROR_ARGS_MISSING;
```

A command that cannot finish in one tick — `watch`, `fedit`, an interactive prompt — returns `CMD_ERROR_AGAIN`. The dispatcher keeps the instance alive, counts the iteration, and re-enters `execute` on the next input.

One thing to know when you do that: a parsed option value points into the live receive buffer, which is gone by the next keystroke. `holdOptionValue("c")` copies the bytes into storage the option owns, freed when the command clears. Any value that has to survive into the next tick needs it.

### 7.5 Result codes

A command returns a `pdi_err_t`, from the command band declared in [src/utility/pdi_types.h](src/utility/pdi_types.h). Everything in the framework uses the same type, so a code from storage or the network can travel back out through a command unchanged.

| Result | Meaning | Dispatcher's response |
|---|---|---|
| `PDI_OK` | done | blank line, options cleared |
| `CMD_ERROR_AGAIN` | keep me alive | nothing cleared; wait for more input |
| `CMD_ERROR_INVAL`, `CMD_ERROR_ARGS_MISSING`, `CMD_ERROR_OPT` | bad argument, missing argument, unrecognised option | error line plus the command's usage string |
| `CMD_ERROR_NOENT`, `CMD_ERROR_INVALID` | no such command, or unparsable | error line |
| `CMD_ERROR_PERM`, `CMD_ERROR_ACCES` | login required, or the credential given was wrong | back to the login flow |
| `CMD_ERROR_CANCELED`, `CMD_ERROR_INTR` | Ctrl+C or Ctrl+Z, and terminal abort | error line, iteration stops |
| `CMD_ERROR_FAILED` | the command ran and did not succeed | error line |
| `CMD_RESULT_FALSE` | the command was asked a question and the answer is no | nothing printed; only `$?` carries it |
| `CMD_ERROR_NOTTY`, `CMD_ERROR_HOLD_BUFFER`, `CMD_ERROR_UNSET` | no terminal, the terminal is holding the line buffer, no result recorded yet | handled by the service |

### 7.6 The dispatcher

The service owns the list of in-flight commands and the history file path. Everything else — receive buffer, cursor, history position, completion index, working directory — lives on the session. Each input tick resolves the session from the terminal, makes it current, and then runs the command.

History is persisted only when storage is available, in a file capped at 25 lines. Completion walks the command registry for names matching the typed prefix and cycles on repeated Tab, and works with or without storage because the registry is in RAM.

### 7.7 Pipes and redirection

A command does not know where its output is going, and a command reading input does not know where it came from. That is the whole trick: the shell puts a stream in front of each end, and the command writes and reads as it always did.

```
   ps | grep ssh > /tmp/found.txt

   ┌──────────┐   stdout     ┌──────────┐   stdout     ┌───────────────┐
   │   ps     │ ───────────▶ │  grep    │ ───────────▶ │ /tmp/found.txt│
   └──────────┘   a pipe     └──────────┘   a file     └───────────────┘
                             ▲   stdin
                             └── the pipe the stage before it filled
```

Each session carries a small descriptor table — `stdin`, `stdout`, `stderr` — that normally points at the terminal. The shell repoints an entry for the length of one line and puts it back afterwards, so nothing leaks into the next command:

```
                        fd 0 · stdin        fd 1 · stdout

   plain command        the terminal        the terminal

   cat /etc/passwd  ┬─  cat: the terminal   cat: the pipe
             | wc   ┴─  wc : the pipe       wc : the terminal
```

| Operator | Meaning |
|---|---|
| `>` | send output to a file, replacing it |
| `>>` | send output to a file, appending |
| `\|` | send output to the next command |
| `<` | read input from a file |
| `;` | run the next command whatever this one returned |
| `&&` | run the next command only if this one succeeded |
| `\|\|` | run the next command only if this one failed |

`cat`, `head`, `tail`, `wc` and `grep` read the input descriptor when no filename is given, which is what makes them useful on the right of a pipe. Everything else keeps writing to `stdout` without a line of its own changing.

Two limits are worth knowing. A pipe is a fixed buffer of `PDI_PIPE_CAPACITY` bytes (1024 by default), the way a real one is; a stage that outruns it marks the pipe overflowed instead of growing until the heap is gone. And a write the filesystem refuses — a redirect into read-only `/proc`, or a full disk — is reported as `cannot write <path>` rather than passing silently, because the answer only arrives when the last block is committed.

`;`, `&&` and `||` join whole pipelines rather than single commands, so `echo counted | wc && echo done` runs the pipeline first and the right-hand side on its result. `$?` carries that result, which is what a following `&&` reads and what `if` and `while` branch on ([§7.15](#715-shell-scripts-and-etcrclocal)). An operator with nothing after it is a syntax error, and one inside quotes is text.

### 7.8 Built-in command inventory

`if`, `else`, `fi`, `while`, `for` and `done` are not in this table. They are read by the script runner rather than dispatched as commands, so they work in a file and not at the prompt — see [§7.15](#715-shell-scripts-and-etcrclocal).

| Command | Options | Brief |
|---|---|---|
| ls [\<dir>] | | List with mode, owner, group, mtime and size. No arg lists the current directory; relative paths join it. Owner and group show names, falling back to numbers. e.g. **ls**, **ls /proc** |
| mkdir \<dir> | | Create a directory, `0755` masked by umask, owned by the session. e.g. **mkdir /scripts** |
| touch \<file> | | Create empty at `0644` masked by umask, or bump mtime if it exists. e.g. **touch /notes.txt** |
| mv \<src> \<dst> | | Move or rename, across mounts if needed. e.g. **mv /a.txt /b.txt** |
| cp \<src> \<dst> | | Copy a file, across mounts if needed. e.g. **cp /a.txt /b.txt** |
| pwd | | Print the working directory. |
| rm \<path> | | Remove a file or directory; needs write permission. e.g. **rm /notes.txt** |
| cat [\<file>] | | Print a file, or the piped input when no file is named; needs read permission. e.g. **cat /proc/uptime** |
| echo \<text> | | Print text. Like every command it can be redirected or piped ([§7.7](#77-pipes-and-redirection)), which is how a file is written from the shell. e.g. **echo 1 > /sys/class/gpio/5/value**, **echo second line >> /notes.txt** |
| fedit \<file> | | Scrolling in-place line editor. A status bar shows the path; ←/→/Home/End/Backspace edit the active line, ↑/↓ move through the file, Enter splits at the cursor. Esc opens the menu: **!w** save, **!c** cancel, **!d** delete line. Edits stream to a temp copy and commit on save. e.g. **fedit /notes.txt** |
| head [\<file>] [N] | | First N lines, default 10, in constant memory. Reads the piped input when no file is named. |
| tail [\<file>] [N] | | Last N lines, default 10, in constant memory. Reads the piped input when no file is named. |
| wc [\<file>] | | Lines, words, bytes, of a file or of the piped input. |
| df | | One row per mount: total, used, free. The mount list comes from `/proc/mounts` and the sizes from the backend behind each prefix. |
| mount | | The mount table: prefix, type, backend. |
| chmod \<octal> \<path> | | Set permission bits; owner or root. e.g. **chmod 0644 /etc/passwd** |
| chown \<uid>[:\<gid>] \<path> | | Change owner, root only; gid defaults to uid. e.g. **chown 1001 /alice** |
| umask [\<octal>] | | Show or set this session's umask, default `0022`. |
| hexdump \<file> | | Offset, sixteen hex bytes, ASCII. |
| grep \<pattern> [\<path>] | | Search a file or directory tree, or the piped input, printing `path:line:col:content`. Regex subset: `.` `*` `+` `?` `^` `$` `[abc]` `[a-z]` `[^abc]` and escapes. e.g. **grep ^ERROR /log.txt**, **ps \| grep ssh** |
| cls | | Clear the screen. |
| cd \<dir> | | Change directory; `~` and `-` work. |
| login | u=, p= | Interactive login, or inline with both options. |
| logout | | End the session — serial returns to the prompt, telnet and SSH close. The working directory and the session's variables reset either way, so the next login starts clean on a terminal that outlives the login. |
| whoami | | The session's username. |
| id | | `uid=N(name) gid=N`. |
| who | | Active sessions across all channels: user, tty, sid, login time, idle. |
| groups | | The current user's primary group. |
| su [\<user> [\<pass>]] | | Switch user in this session, prompting when arguments are omitted. On success the identity, uid, gid and home directory all follow, and the session's environment starts clean. |
| passwd p=\<curr> n=\<new> c=\<confirm> | p, n, c | Change your own password; prompts in three echo-suppressed phases when arguments are omitted. |
| useradd u=\<user> p=\<pass> | u, p | Root only. Next free uid, gid equal to uid, home `/`. Writes both user files. |
| userdel u=\<user> | u | Root only. Removes from both files; refuses root and self. |
| service list \| status \| start \| stop \| restart \| enable \| disable | positional | Service control. `list` shows state, enabled and task count per service; `status <name>` adds tracked pids and that service's own detail; `start`/`stop`/`restart` bring a service up from cold and tear it down for real; `enable`/`disable` persist `enabled` for the next boot. Root for all but the first two. e.g. **service status MDNS** |
| ps [\<sid>] | | Tasks with owner, state, %CPU, run count, interval and name, read from `/proc`; optional owner filter. |
| top | i=, n=, u= | The `ps` view on a repeating tick — interval, iteration bound, owner filter. Ctrl+C stops it. |
| kill [\<sig>] \<pid> | | Signal a task: 9 KILL, 15 TERM, 18 CONT, 19 STOP. One argument is a pid, two are signal then pid. |
| pkill [\<sig>] \<name> | | Same, matched by name across every task carrying it. |
| killall [\<sig>] \<name> | | Same as `pkill`, defaulting to KILL. |
| renice \<nice> \<pid> | | Change nice, -20..19, and re-sort immediately. |
| sshkgen t=\<algo>[,f=\<dir>] | t, f | Generate an SSH key pair. `t=1` Ed25519, `t=2` or `3` RSA. Without `f` it prompts for the directory — **a]** `~/.ssh` for client keys, **b]** `/etc/ssh` for host keys, the default — and an explicit path also works. Ed25519 is instant; RSA keygen runs into minutes and how many depends on the device. e.g. **sshkgen t=1**, **sshkgen t=2,f=b** |
| net \<option> | ip, scansta, connsta | Network state and control. e.g. **net connsta,\<ssid>,\<password>** |
| host \<name> | | Resolve a name: IP literal, then `/etc/hosts`, then DNS. |
| ping \<host> [count] | | ICMP echo, default four packets and at most ten, streaming each reply and finishing with a loss and rtt summary. |
| wget [\<path>] \<url> | | Download an `http` or `https` url to a file, redrawing a transfer bar as it goes. The path may name a file or a directory; left out, the file lands in the working directory under the name the url ends with. An existing file is replaced. The transfer is refused before anything is fetched when the name is longer than the filesystem allows or the body will not fit in the free space, and a failed transfer leaves no partial file. `https` needs the TLS service ([§6.2.18](#6218-tls-no-provider-transport-hookup--cert-provisioning)). e.g. **wget https://host/app.bin**, **wget /app.bin https://host/app.bin** |
| date [-u] [-n] [-s \<epoch>] [+\<fmt>] | | Show or set the clock. `-u` for UTC, `+fmt` for a custom format, `-s` to set, `-n` to force an NTP resync. |
| tdctl | | Clock status: local and universal time, zone, sync state, server. |
| reboot | | Reboot. |
| watch | c=, i=, n= | Run a command repeatedly. Options are comma separated, as every other command's are, so `;` stays the shell's own command separator. e.g. **watch c=net ip,i=3000,n=10**. Quote the inner command when it carries a comma of its own — **watch c="login u=a,p=b",i=3000** — and the quotes bound the value rather than ending it |
| db status \| list \| verify \| save \| restore | positional | Inspect the config record store: which medium is live and how full it is, one line per record, a checksum pass over all of them, and saving or restoring the defaults tier. Never prints a record's contents. See [§5.10](#510-from-the-terminal). |
| iot \<option> | setid, getid, sethost, gethost | Device unique id and IoT host. |
| env | | Every variable this session can see, one `NAME=value` per line, each from the tier that wins. See [§7.14](#714-environment-variables). |
| export \<name>=\<value> | | Set a variable for this session. Names the session answers for itself are refused. e.g. **export GREETING=hello** |
| unset \<name> | | Drop a variable from this session, leaving `/.env` alone. |
| source \<file> | | Run each line of a file in this session, so a `cd` or an `export` it performs is still in force afterwards. See [§7.15](#715-shell-scripts-and-etcrclocal). |
| test \<a> \<op> \<b> | | Answer a question through `$?` and print nothing either way: `=` `!=` on strings, `-eq -ne -lt -le -gt -ge` on numbers, `-z` `-n` on emptiness, `-e` `-f` `-d` on paths. e.g. **test $MODE = debug && echo debugging** |
| crontab | | List the scheduled jobs `/etc/crontab` holds, as the device parsed them, and what is due this minute. See [§7.16](#716-scheduled-jobs-and-etccrontab). |
| help | | Every registered command with its usage line. Works before login. |
| uptime | | `up Xd Yh Zm Ws`. |
| tls q=1,t=,l=,n=,i= | | On-device certificate generation. Present only where the port declares `DEVICE_SUPPORTS_TLS_CERT_GENERATION` and the flag is on; elsewhere certificates are generated off-device and uploaded ([§6.2.18](#6218-tls-no-provider-transport-hookup--cert-provisioning)). e.g. **tls q=1,t=0,l=256,n=device.local,i=192.168.1.50** |
| exec \<path> | | Load a program image from the filesystem and run it as a background task, returning its pid. Present only where the port registers a loader; a board without one simply has no `exec`. See [§7.13](#713-dynamic-app-loading). |

Path arguments behave the POSIX way everywhere: a leading `/` is absolute, anything else resolves against the session's working directory, and `cd` also takes `~` and `-`.

On argument style: commands with at most two arguments take them positionally, which is both shorter to type and what muscle memory expects — `chmod 0644 /etc/passwd`, `renice -5 10`. Where a leading argument is optional, the count disambiguates: one argument is the target, two are signal then target. Named `x=y` options are kept for commands with many optional slots, for anything that takes a password, and for patterns where the optional argument sits in the middle.

### 7.9 Multi-terminal session lifecycle

Several sessions run at once with fully independent state. There is a single dispatcher and an array of session slots; each slot holds its own line buffer, cursor, history and completion position, working directory, umask, identity, descriptor table, environment variables and position in any script it is running. The serial console holds one slot from boot, and telnet and SSH each keep a pool of their own, so two people over telnet and four over SSH are all just slots in the same table.

```
  boot ──▶ filesystem up
        ──▶ user store bootstraps /etc/passwd and /etc/shadow if absent
        ──▶ serial terminal attached, login prompt drawn

  telnet connect ──▶ free pool slot? ──▶ accept ──▶ useTerminal(client)
                  │                                  ├─ session attached to the next free slot
                  │                                  └─ login prompt on that client only
                  │  pool full ──▶ short grace, then "no telnet session available"
                  ──▶ each tick: find session by terminal, make current, dispatch
                  ──▶ disconnect: session detached, both slots freed

  ssh connect    ──▶ user auth succeeds ──▶ session attached and marked authorised
                  ──▶ channel opens     ──▶ useTerminal, prompt
                  ──▶ each tick: same session-scoped path
                  ──▶ close: session detached
```

The table and the pools are sized together — see the note in [§6.2.16](#6216-commandlineserviceprovider--__cmd_service). A pool larger than the table it draws from would leave one transport unable to claim the slots it promises, so a device config that names too few sessions fails the build rather than finding out at runtime.

Two invariants carry the whole design. First, the current session is switched once per tick, at the top of input handling, and the static terminal pointer moves with it — so prompt drawing, new command instances and the auth delegators all see the right session without anyone passing it around. Second, every in-flight command records which session created it, and the lookups that find waiting commands filter on that. A telnet login prompt waiting for a username cannot be fed by SSH keystrokes.

Long-running commands capture their terminal and owner when they start, so their output keeps flowing to the right session no matter what the other sessions are doing.

### 7.10 SFTP and SCP file transfer

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

#### 7.10.1 SSH authentication

Password and public key are both accepted, and `/etc/ssh/ssh.conf` decides which are offered:

```
# /etc/ssh/ssh.conf
PasswordAuthentication yes
PubkeyAuthentication   yes
```

Passwords are checked against `/etc/shadow` — the same credentials as serial and telnet. Public keys are Ed25519 or 2048-bit RSA, listed one per line in the device's `~/.ssh/authorized_keys` in the standard OpenSSH format, which is exactly what you copy out of your own `id_ed25519.pub`:

```
fedit ~/.ssh/authorized_keys        # paste the line, then !w
```

Set either option to `no` to switch that method off — `yes`/`on`/`1` and `no`/`off`/`0` all read the way you would expect, and a word that means neither leaves the option at its default rather than silently turning it off. Whether the SSH service starts at all is a separate question, answered by `/etc/service.conf` ([§3.10](#310-the-etc-config-surface)). With passwords off, only a holder of an authorised private key gets in:

```
ssh -i ~/.ssh/id_ed25519 pdiStack@<device-ip>
```

The device's own host keys are separate, in `/etc/ssh`. Ed25519 lives in `/etc/ssh/ed25519` with its `.pub` and `.seed`, and is created automatically the first time the SSH service starts. RSA lives in `/etc/ssh/rsa` and is generated only when you ask for it with `sshkgen t=2,f=b`, since 2048-bit keygen costs minutes and how many depends on the device. Either way the key goes onto the wire in standard SSH format during the handshake. `authorized_keys` is one file under the device home directory, shared by all users.

### 7.11 Background commands and Ctrl+C

`watch` and `top` run as scheduler tasks behind a shell session, and they are the template for any long-running command:

```
  1  parse arguments, holdOptionValue() anything that must outlive this tick
  2  register a scheduler task under the command's name, so it shows up in ps
  3  set m_runinbackground and return INCOMPLETE, so the instance stays alive
  4  override stopRunningInBackground() to remove that task and clear the flag
```

Ctrl+C handling is generic: the dispatcher walks its command list and calls `stopRunningInBackground()` on every backgrounded command owned by the current session. A new background command inherits that behaviour with no wiring in the shell. `ps` shows what is currently running, under the command's own name.

### 7.12 Adding a command

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
       static void RegisterCommand() {
           CommandBase::RegisterCommand(CMD_NAME_TEMP, [](void *arg)->void *{
               return pdiutil::safe_new<TempCommand>();
           });
       }
       const char* getUsage() const override {
           return RODT_ATTR("temp [t=C|F]  read the temperature sensor (default Celsius)");
       }
       bool needauth() override { return true; }
       pdi_err_t execute(cmd_term_inseq_t terminputaction) {
           CommandOption *unit = RetrieveOption(CMD_OPTION_NAME_T);
           bool celsius = !(unit && unit->optionval && unit->optionval[0] == 'F');
           m_terminal->writeln(readSensor(celsius));
           return PDI_OK;
       }
   };
   ```
3. Include the header in the CLI service and call `TempCommand::RegisterCommand()` in its constructor, next to its siblings.

That is the whole job. Tab completion, history, help, argument-error usage printing and Ctrl+C all come from the base.

**Two argument shapes.** `AddOption` declares named `x=y` slots, as above. A command that takes its arguments positionally calls `setAcceptArgsOptions(true)` instead and reads `m_options[0]`, `[1]` and so on; `setCmdOptionSeparator(CMD_OPTION_SEPERATOR_SPACE)` makes those the space-separated words, which is how `su <user> <pass>` and `test <a> = <b>` read theirs. ⚠ Without `setAcceptArgsOptions(true)` a positional argument is never captured and every slot looks empty.

**A command that answers a question** returns `CMD_RESULT_FALSE` for "no" rather than an error code. The dispatcher prints nothing for it, so a false result can sit inside a script or an `&&` chain without an error line, while `$?` still carries it — `test` is the one that does this today ([§7.15](#715-shell-scripts-and-etcrclocal)).

A few limits shape command design. Names are capped at eight characters and options at three, each name up to three characters, so a verb that wants more either splits into sub-commands or takes positional arguments. Options are comma separated, and a value that needs to carry a comma, an `=` or a space is **quoted** — single or double, as a shell quotes — so `watch c="login u=a,p=b",i=3000` passes the whole inner command through as one value. The quotes bound the value and are removed before the command sees it. And `needauth()` is the only permission gate, so put it on anything that changes state.

Parsing stays index-based rather than building an argv: the parser hands each command the offsets and lengths of its options within the line it already holds, and quote removal compacts in place inside that span. On a device measured in kilobytes a second copy of every argument is a cost with nothing to show for it, so the index model is the standard here and argv is deliberately not built.

### 7.13 Dynamic app loading

With `ENABLE_PROGRAM_EXEC`, `exec` reads a program image off the filesystem, hands it to whatever loader the port registered, and launches it as a background preemptive task:

```
exec /apps/hello.app.elf
```

The command knows nothing about image formats. A port declares `DEVICE_SUPPORTS_PROGRAM_EXEC` and registers an `iProgramLoaderInterface`, which owns what an image is and how it is relocated — so a board with a different architecture needs no change on the command side, and a board with no loader simply has no `exec`. The loader that exists today works over a relocatable ELF image.

The command returns to the prompt immediately with a pid. The app runs concurrently and its output arrives asynchronously; `ps` lists it and `kill <pid>` ends it. It also ends when `main()` returns, and the image is freed on either path. Because it is a task rather than a scheduler entry, stop and continue don't apply to it.

The path goes through the normal VFS, so apps arrive by SFTP or HTTP upload like any other file. The feature needs storage and brings contextual execution with it.

The program must be a small position-independent ELF built against the loader — not a firmware image — calling only symbols the firmware exports; the common libc entries are already there. How you build one depends on the SDK behind the port, and the loader ships with a template project for it:

```bash
# with the port's SDK environment active
<sdk> create-project-from-example "<loader template>"
cd <project>                     # edit main.c, keeping int main(int argc, char *argv[])
<sdk> set-target <board>         # must match the loader's own target
<sdk> elf                        # → build/<name>.app.elf
```

Two lines in the project's build file are what make the output loadable rather than a normal binary: `include(elf_loader)` and `project_elf(<name>)`. Upload the result and run it. A port without a loader simply has no `exec` command, and nothing else changes.

### 7.14 Environment variables

A variable can come from three places, and a lookup takes the first one that carries it:

```
  $NAME
    │
    ├─ 1. derived     PWD, USER, UID, HOSTNAME — answered from the session itself
    ├─ 2. session     set with export, gone when the session ends
    └─ 3. base file   /.env, shared by every session and kept across reboots
```

`env` prints all three merged, `export NAME=value` sets one for this session, and `unset NAME` drops it again. A session variable shadows the base file, so dropping it uncovers the file's value rather than deleting it. The base file is edited like any other config — `fedit /.env` — and is created with `HOME` on first boot.

**The first tier is resolved, never stored.** `PWD`, `USER`, `UID` and `HOSTNAME` are facts the session already knows, so they are read at lookup time and assigning them is refused:

```
pdiStack@12580169:(/): export PWD=/nope
the session answers for that name, it cannot be set
```

That is what keeps them honest — a stored `PWD` would be wrong the moment anyone ran `cd`, and two sources of truth for one fact is the thing worth avoiding, not the extra lookup.

The shell expands `$NAME` wherever it expands `$?`, with the same quoting rules: a single-quoted run is literal, a double-quoted one still expands, and a name the environment does not carry expands to nothing rather than erroring.

```
pdiStack@12580169:(/): export GREETING=hello
pdiStack@12580169:(/): echo $USER at $HOSTNAME
pdiStack at pdi-bff549
pdiStack@12580169:(/): echo pre$GREETING-post
prehello-post
pdiStack@12580169:(/): echo '$GREETING'
$GREETING
```

A name is a letter or underscore followed by letters, digits or underscores. A session holds `ENV_SESSION_MAX` variables, eight by default; names and values are bounded by `ENV_NAME_MAX` and `ENV_VALUE_MAX`. The base file needs storage, so a board without a filesystem keeps the first two tiers and simply has none.

The session tier ends with the login. `logout` and `su` both clear it, which matters most on the serial console: telnet and SSH close their channel and release the session with it, while the serial terminal outlives every login on it, so without an explicit reset one user's variables would follow the next one in. Only the session tier is dropped — the base file is untouched, so clearing uncovers `/.env` rather than deleting from it.

### 7.15 Shell scripts and `/etc/rc.local`

A script is a file of shell lines, and there is no second grammar. Each line goes through the same executor a typed line goes through, so pipes, redirection, `&&` and `||`, quoting, `$NAME` and `$?` all mean inside a script exactly what they mean at the prompt. The runner reads only two things for itself: a blank line, and a `#` comment.

```
pdiStack@12580169:(/): source /demo.sh
```

**`source` runs in the session that called it**, not in a subshell — the framework has none. A `cd` or an `export` a script performs is still in force when it returns, which is the whole reason the POSIX name fits:

```
# /setup.sh
export MODE=debug
cd /var/log
```
```
pdiStack@12580169:(/): source /setup.sh
pdiStack@12580169:(/var/log): echo $MODE
debug
```

A script that fails a line keeps going, the way `rc.local` does; use `&&` for a line that must gate the next. Scripts can call scripts, bounded by `SCRIPT_MAX_DEPTH`, so a file that sources itself stops rather than recursing until the heap gives out.

#### Branching and repeating

`if`/`else`/`fi`, `while`/`done` and `for`/`done` are handled by the runner as a line-level state machine, not by the line parser — control flow spans lines, and the parser deliberately understands one line at a time. There is no `then` and no `do`: the runner is line based, so a keyword on a line of its own would be noise.

A close has to match what it closes. `fi` ends an `if`, `done` ends a `while` or a `for`, and a mismatched or stray one fails the script rather than quietly ending a block it does not belong to — a `for` closed by `fi` would otherwise run a single pass and read as a loop that simply had one word in its list.

`if` and `while` branch on **a command's result**, which is what `$?` already carries for `&&`:

```
# /boot-checks.sh
if test -f /etc/wifi/wifi.conf
echo wifi settings present
else
echo running on stored defaults
fi

export TRIES=a
while test $TRIES != c
echo attempt
if test $TRIES = a
export TRIES=b
else
export TRIES=c
fi
done
```

`test` is what makes that useful. Every other command reports success or a *fault* — `grep` returns success whether or not it matched — so without `test` a condition could only ask whether a command malfunctioned, never what a value is. It answers through `$?` and prints nothing either way, so a false condition inside a loop leaves no output:

| Form | True when |
|---|---|
| `test <a> = <b>`, `test <a> != <b>` | the strings match, or differ |
| `test <a> -eq -ne -lt -le -gt -ge <b>` | the numbers compare so; a non-number is refused rather than answered |
| `test -z <s>`, `test -n <s>` | the string is empty, or is not |
| `test -e <path>`, `-f`, `-d` | the path exists, is a plain file, or is a directory |

`for` walks a list of words instead of a condition, binding each one to a name in turn:

```
# /rotate.sh
for LEVEL in error warning info
echo clearing $LEVEL
rm /var/log/syslog.$LEVEL
done
```

The list is words, and quoting decides what a word is: `for X in "one two" three` is two passes, while an unquoted `$NAME` holding blanks is as many words as it reads as. A name the environment does not carry expands to nothing and contributes no word, so a list that goes empty simply runs the body no times. The name is bound for real, so it is still set to the last word after `done`; `for` refuses a name the session answers for itself, such as `PWD`.

A loop is capped at `SCRIPT_LOOP_MAX` passes, so a condition that never goes false fails the script instead of holding the session — at boot there is nobody to interrupt one. A `for` over a fixed list ends itself, but the cap still applies, because the list is read again on each pass and a body that adds to its own list would otherwise never finish. Nesting is bounded by `SCRIPT_BLOCK_MAX`, and a block left unclosed at the end of a file is reported rather than passing quietly.

#### A line that asks for something

A command inside a script can stop for input — `su` and `passwd` both do. Where the script was typed by someone, the terminal is handed back so they can answer, and the script carries on at the next line afterwards. Where nobody is watching it, waiting would hold the session forever and the line after would be read as the answer, so the waiting command is cancelled and the script stops. Boot and scheduled scripts take the second path, which is what keeps `rc.local` from wedging a boot.

Where a script has got to lives on the session, so two transports can each be part-way through one without touching the other, and a script pending when someone logs out or runs `su` is abandoned rather than resumed under the next login.

#### `/etc/rc.local`

If `/etc/rc.local` exists it runs at the end of boot, after every service has started and before the login prompt appears. It is not created for you.

It names the identity it runs under, in its **leading comment block**:

```
# rc.local - runs at the end of boot
# UID 0
echo bringing up the workshop config
source /etc/site.sh
```

| The header says | What happens |
|---|---|
| no `UID` line | runs as root |
| `# UID <n>` and the user exists | runs as that user, with their home as the working directory |
| `# UID <n>` and no such user | **the file does not run at all** |

An identity that cannot be resolved is never quietly downgraded to root — a script asking to run as somebody specific and silently getting more privilege than it asked for is the wrong failure. A `UID` line below the first command is documentation, not policy, so a comment further down cannot change who the script runs as.

The session `rc.local` runs in is built for it and given back afterwards. That is deliberate: reusing the console's session would leave a logged-in root shell sitting on the serial port at boot. It also means `export` inside `rc.local` does not reach your login — put anything that must outlive it in `/.env`.

The whole of this section is behind `ENABLE_SCRIPT_RUNNER`, on by default. Comment it out and `source`, control flow and `rc.local` go with it, including the per-session state they use; the rest of the shell is unaffected.

### 7.16 Scheduled jobs and `/etc/crontab`

`rc.local` covers "run this once at boot". `/etc/crontab` covers "run this every so often" — one job per line, five time fields and the command:

```
# min hour dom mon dow  command
*/5  *    *   *   *     echo tick >> /tmp/heartbeat
0    3    *   *   *     source /etc/nightly.sh
30   4    *   *   0     db backup
```

Each field takes the standard grammar: `*`, a number, a range `10-20`, a step `*/5` or `10-20/2`, and comma-separated lists of any of those. Day-of-week runs 0–6 from Sunday. Month and day-of-month names (`JAN`, `MON`) and `@daily`-style macros are not read — `rc.local` is the `@reboot` equivalent.

| field | range |
|---|---|
| minute | 0–59 |
| hour | 0–23 |
| day of month | 1–31 |
| month | 1–12 |
| day of week | 0–6, Sunday 0 |

**A spec that does not parse matches nothing**, rather than reading as zero and firing on the hour. That distinction matters because the two look identical from outside: a job that never runs and a job that has not come round yet both do nothing.

**The table is read from the file every minute and nothing is kept between runs.** An edit takes effect on the next minute with no reload step, and a board carries no memory cost for jobs it is not running. The cost is one pass over the file per minute, which is why `CRON_MAX_ENTRIES` (8 by default) bounds it — a line is found by scanning from the start of the file, so a long table costs more than its length suggests.

**Jobs only run while the clock is trustworthy.** A board that cannot reach a time server still counts forward from boot but never calls that time valid, and a calendar cannot be matched against a count from boot, so nothing fires. `crontab` says so rather than leaving you to wonder.

Each job runs in a session built for it, under root, and given back afterwards. **That session is attached to the null terminal, not the console**, which is what lets a job run at all while somebody is logged in — a job must never borrow a live session, and waiting for the console to be free would mean never running on a board anyone uses. Job output therefore goes where the job sends it: to a file if it redirects, and nowhere otherwise, the way `cron` has always behaved. A job that stops for input is interrupted rather than left holding the terminal.

`rc.local` keeps the console instead, so its output is still visible at boot — it runs before anyone can be logged in, so there is no session to collide with.

`crontab` lists the table as the device parsed it, which is what makes it worth more than `cat /etc/crontab` — a row missing a field, or a typo in a spec, simply does not appear:

```
$ crontab
*/5 * * * * echo tick >> /tmp/heartbeat
0 3 * * * source /etc/nightly.sh

due this minute:
  none
```

Editing is `fedit /etc/crontab`. The whole of this is behind `ENABLE_CRON_SERVICE`, on by default and switched off automatically without the cmd and storage services.

---
## 8. Web Server

The web server is the on-device admin portal. With `ENABLE_HTTP_SERVER` on, the device serves HTTP/1.1 on its access-point address — `192.168.0.1` by default — so a phone or laptop can log in, configure WiFi, drive GPIO, watch a dashboard, upload files and manage MQTT, OTA, email and IoT without a cable. Adding `ENABLE_HTTPS_SERVER` serves the same portal on 443 over TLS and turns `ENABLE_TLS_SERVICE` on for it; port 80 stays open only to redirect `http://` callers up to `https://`, so a typed-in plain address still lands on the secure portal. No controller, page or route changes, only a different `begin()` call and a certificate on disk.

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
        │                                                    ├─ auth ─▶ session lookup, else 301
        │                                                    └─ api  ─▶ session lookup, else 401
        │                                                       └─ POST ─▶ CSRF token check
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

When it binds 443, the secure `begin()` also stands up a bare TCP listener on 80 whose only job is to answer each request with a redirect to the `https://` URL, so the plain port is never left dark for a browser that omits the scheme. Nothing else in the subsystem is conditional on TLS. The per-tick `handle_clients()` is a single call into the port; all the work happens inside the route lambdas.

### 8.3 Routes

URIs are constants in one header, so a typo is a compile error rather than a dead link.

| Route | Purpose | Gate |
|---|---|---|
| `/` | menu landing | — |
| `/login` | form and its POST | — |
| `/logout` | drop the session | — |
| `/login-config` | change your own password | auth |
| `/dashboard` | live device summary | auth |
| `/listen-dashboard` | one aggregated dashboard payload | api |
| `/wifi-config` | station and AP form | auth |
| `/ota-config` | host, port, version, and flashing an image from storage | auth |
| `/email-config` | SMTP credentials | auth |
| `/gpio-manage`, `/gpio-server`, `/gpio-config`, `/gpio-write`, `/gpio-event` | GPIO panel and its subforms | auth |
| `/gpio-monitor` | live analog graphs | auth |
| `/listen-monitor` | graph and pin updates | api |
| `/mqtt-manage`, `/mqtt-general-config`, `/mqtt-lwt-config`, `/mqtt-pubsub-config` | MQTT forms | auth |
| `/device-register-config` | IoT registration | auth |
| `/storage`, `/storage-fileupload`, `/storage-filelist`, `/storage-filedel` | file browser, upload and delete | auth |

The file browser lists each entry the way `ls -l` does — a permission string such as `drwxr-xr-x`, then owner and group resolved to names through the user store, then size. Every operation runs as the signed-in user, so a listing, a delete or an upload succeeds exactly where the same account would succeed from the shell, and an upload lands owned by whoever uploaded it. A refusal is reported as a denied permission rather than a generic failure.

The dashboard gathers what the other subsystems already know into one card grid, refreshed from a single `/listen-dashboard` payload every three seconds. Link state sits in the page heading as symbols: wifi arcs lit by signal strength with the RSSI beside them, and a globe that dims when the internet probe last failed. Storage and heap are rings — the storage ring is the used share of the root filesystem, the memory ring is how much of the free heap is one contiguous block, which is the number that matters when an allocation fails on a device still reporting plenty free. Below them sit uptime with the four busiest tasks in the columns of `ps` — pid, name, state letter, lifetime CPU share — then every authenticated session with the transport it arrived on, so a serial login, an SSH session and a portal login appear side by side with their login and idle times. Configured pins render as tiles carrying mode and current state (`DOUT · HIGH`, `DIN · LOW`, `AOUT · 180`), and an analog pin adds a rolling trace against a labelled axis, held on the client so the device only ever sends the present reading. Each card is fed by its own feature flag, so a build without storage, GPIO or the auth service simply serves fewer of them.

Rows that repeat are arrays rather than objects, which keeps a full payload — radio, filesystem, heap, tasks, sessions, pins and attached stations — around 400 bytes, and the response string is reserved up front so it never grows in steps while being built.

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

Four levels. A route registered without one is public. The auth level asks the session handler whether the request carries a live session and, if not, redirects to the configured target so the route never runs. The API level makes the same check but answers `401`, which is what the polling endpoints want — they are consumed by script, not navigated to.

The root level is the auth level plus one question: is this session root's? If not, the middleware answers with a single shared "root privilege required" page carrying a link home, and the handler never runs. That is what keeps the portal and the filesystem agreeing — a settings file is `0600 root:root`, so the page that edits it is root's too.

```cpp
register_route(WEB_SERVER_MQTT_GENERAL_CONFIG_ROUTE,
               [&]() { this->handleMqttGeneralConfigRoute(); },
               ROOT_AUTH_MIDDLEWARE);
```

Because the refusal is a *standard* page rather than the controller's own, the middleware can render it, which is why restricting a page costs one enum value and no handler code. Gate the pages that change something, never the menu that reaches them: the GPIO menu stays on the auth level so a non-root user can still get to the pin monitor, and each configuration card behind it refuses on its own. Watching is not configuring.

All guarded levels then apply one more rule: a POST must carry the CSRF token of its own session, or it is refused — `403` on an API route, a redirect on a page. Keeping that check in the middleware rather than in each handler means a newly added form is covered by default; it only has to render the field.

Forms get the field from a single shared helper, `concat_csrf_input_html_tag()`, which sits with the other markup builders and writes its markup from flash-resident constants. Pages that submit through script read the same value with `get_csrf_token()`.

Middleware lives on the session handler rather than as a separate chain, so a new level means extending the enum and adding a branch — routes then opt in by naming it.

### 8.6 Sessions

Sessions live on the server. `WebSessionManager` owns a small fixed table of slots — two by default, `WEB_MAX_SESSIONS` — and the browser only ever holds an opaque token.

Logging in verifies the credentials through the auth service, takes a slot, and draws two independent random values from the hardware RNG: the session token that goes into the cookie, and a CSRF token handed to every form that session renders.

```
Set-Cookie: pdi_session=<32 hex>; Max-Age=300; Path=/; HttpOnly; SameSite=Strict[; Secure]
```

The cookie name and max age come from the credential table — `pdi_session` and five minutes. `Secure` is appended when the request itself arrived over TLS, rather than when the build has TLS compiled in, because a browser silently drops a `Secure` cookie delivered over plain HTTP. Each guarded request parses the `Cookie` header by exact name, matches the token with a constant-time compare, and enforces both an idle timeout (the cookie max age) and an absolute cap (`WEB_SESSION_ABSOLUTE_MAX_AGE`, eight hours). Logging out, or changing a password, releases the slot, so a captured cookie stops working at that moment rather than at expiry.

Both ends of that window have to slide together. The server refreshes its idle stamp on every request, but the cookie carries a fixed expiry from the moment it was written, so a session that is still in use is handed a fresh cookie once it passes half its life — otherwise the browser drops it mid-session and the user is bounced to the login form while the session is still live. The reissue is rate-limited to the half-life rather than sent on every response, so a page polling every few seconds does not pay for the header each time.

A slot is reclaimed when its session expires, and any request sweeps the whole table rather than only the slot whose token matched. Without that, a client that simply walks away leaves its slot held until the next login happens to notice — which on the default two-slot table costs half the capacity.

A web session *is* a `session_t` — the same structure the terminals use, extended with the two tokens. That means a validated request can be published through `SessionManager` for the life of the handler, which is what makes the VFS resolve file operations as the logged-in web user: uid, gid and umask are the session's, not root's. The route wrapper detaches it again once the handler returns, so a web session is never current while terminal work runs.

Failed logins are counted; past `WEB_LOGIN_MAX_ATTEMPTS` the login route refuses attempts for `WEB_LOGIN_LOCKOUT_DURATION`. The counter is device-wide rather than per-client, because the server interface exposes no client address.

### 8.7 Composing a response

The resource provider holds the server pointer for the current request, the page buffer, and shortcuts into the database service so controllers never include database headers.

Pages are raw HTML strings kept in program memory and sent as header, body and footer:

```cpp
m_web_resource->m_server->send(HTTP_RESP_OK, MIME_TYPE_HTML, headerHtml, /*chunked=*/true);
m_web_resource->m_server->send(HTTP_RESP_OK, MIME_TYPE_HTML, bodyHtml,   /*chunked=*/true);
m_web_resource->m_server->send(HTTP_RESP_OK, MIME_TYPE_HTML, footerHtml, /*chunked=*/true);
```

The header and footer are shared by every page; the middle is whichever page is being served. Sending in chunks is what lets a page exceed the 1800-byte single-send buffer, which is why every composed page uses it.

Three helpers carry the dynamic markup: a page builder that turns C structs into form inputs, tables and option lists; a set of tag and attribute constants that keep allocations down; and an inline SVG icon set.

The shared header carries the styling for every page, so the look is defined once. Colours are CSS custom properties with a light and a dark set: the dark set applies through `prefers-color-scheme` until the visitor picks a side with the toggle in the top corner, after which a `data-theme` attribute on the root element wins and the choice is remembered in `localStorage`. Success, error and warning share that treatment — a flash message names a status class and takes its text colour from a token, over a translucent tint that composites against whichever card colour is in force, so one rule reads correctly in both schemes. Layout is sized for a phone first and widens with the viewport, with controls given larger hit targets on coarse pointers; pages that need the room, such as the file browser, open a wider container. Because the header is larger than one chunk, it is emitted as several flash-resident fragments, each under the single-send buffer.

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
  port TLS backend            whichever the port supplies — loads PEM from the
                              filesystem at the configured paths
```

| Path | Default | Purpose |
|---|---|---|
| server certificate | `/etc/http/server.crt` | PEM, may be a chain |
| server key | `/etc/http/server.key` | PEM, EC or RSA |
| client CA | `/etc/http/client-ca.crt` | only under mTLS |

Upload those over SFTP after first boot and reboot; the listener picks them up on the next start. The directory is created for you.

Where the port supports on-device generation, the certificates come from the `tls` command, or are minted automatically by the mDNS service once the station has an address, covering both the IP and `<hostname>.local`. Where it does not, they come from `scripts/GenTlsCerts.py` off-device and the upload above is the only way they arrive — there is no `tls` command to fall back on.

**Serving HTTPS costs a live TLS session on top of whatever else is running.** On a device with heap to spare that is a flag change; on one already near its limit, expect to compile other services out to make room, and read [§12.3](#123-the-expensive-features) before assuming it fits.

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
      │                         POST? no matching CSRF token → refused, stop here
      │                         on a pass the session becomes current
      │
  controller method             read args, load config, on POST validate and persist,
      │                         then send header + page + footer
      │
  release                       the session stops being current
      ▼
  send(code, mime, body, chunked)  ──▶  bytes back to the client
```

### 8.9 Three routes worth tracing

**`/wifi-config` POST.** Auth passes, the controller reads the station and AP arguments, loads the current WiFi table so untouched fields survive, applies the new values, saves, and renders the success page. The actual reconnect is scheduled a tick later — the response has to flush before the radio drops out from under it.

**`/listen-monitor`.** The browser polls; each request reads the analog pin and returns a small JSON object with the pin and its value. It carries the API gate, so an expired session gets a `401` and the page sends the browser back to the login form. That is the whole live-graph mechanism.

**`/storage-fileupload`.** The body is streamed to a staging file under `/temp` as it arrives, so upload size is bounded by flash rather than by RAM; the directory is created on demand if it is missing. Parsing finishes before the handler runs, so the staged file is written before the session is known and lands owned by root. Once the CSRF check passes, the controller moves it to the requested directory — picking a free name rather than overwriting — and assigns it to the logged-in user, which is what lets that user manage the file afterwards. A move or delete that the user's permissions do not allow is reported back on the page rather than silently dropped.

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

If the page carries a form that posts back, call `concat_csrf_input_html_tag()` while composing it. The middleware refuses an authenticated POST without a valid token, so a form that omits the field will submit and be turned away.

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

Remote forwarding builds the datagram directly on the UDP interface, with no vendor syslog library: `<PRI>timestamp hostname pdi: message`, where PRI comes from the level under the `local0` facility, the timestamp comes from NTP once synced, and the hostname is the device address. The collector host is resolved through the name resolver once the station has an IP, and `service status syslog` shows the target, whether it resolved, and the socket state. File writing and forwarding are independent — files work with no network at all, and forwarding rides on top when there is one.

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

That split buys three things. The same HTTP parsing serves both the client and the portal. A port that ships an SDK-native MQTT client can present it as an `iClientInterface` and the framework's own client falls away without any service noticing. And because a transport is plain C++ over a stream, it can be exercised on a host with no radio involved.

Live in [src/transports/](src/transports/). There are no globals — each consumer creates an instance.

| Transport | Class | Used by | Speaks |
|---|---|---|---|
| HTTP | `Http_Client` | OTA, IoT, GPIO posting, `wget`, and the portal indirectly | HTTP/1.1, and 1.0 |
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

Handshake, verification, record framing and the same response parsing all come along. `setVerifyPeer(false)` keeps the channel encrypted while skipping chain validation, which is right for a self-signed development box and wrong for anything crossing a network you don't own.

**The URL picks the client.** Each request reads its URL first and takes the client that URL needs, changing the one it holds only if it is the wrong sort. An `http://` URL goes over the plain client, an `https://` URL over the TLS one — and that includes a redirect that switches from one to the other half way, which you could not have known about when you started the request. Handing in your own client with `SetClient()` still works, and it is left alone as long as it matches.

**`https://` needs a board that can do TLS and a build with it switched on.** Without both there is no secure client to use, and the request fails with an error rather than going out unencrypted. `http://` works on any build with a network.

Because the choice is made per request, OTA, device-IoT and GPIO posting all share one client and none of them has to pick anything, and turning TLS on no longer sends a plain `http://` address into a handshake it cannot answer.

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
| `AddingCommand` | your own shell command, reachable over serial, Telnet and SSH |
| `Logging` | console logging against logging to a file under `/var/log` |
| `ScriptAndCron` | writing a script from the sketch, running it, scheduling it in `/etc/crontab` |
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
#define STUDENT_TABLE_ID  10            // 1..9 belong to the framework tables

struct student_table { student_t students[MAX_STUDENTS]; int student_count; };

class StudentTable : public DatabaseTable<STUDENT_TABLE_ID, student_table> {};
StudentTable __student_table;           // registers itself at static-init
```

After `initialize()`, the sketch builds a value and calls `set`, and a five-second task reads it back with `get` and prints it.

Three things it teaches. Take an id nothing else holds and keep it — the engine finds the record by id, and reusing one a removed table had hands the new table the old one's bytes. Keep the struct POD, fixed-size arrays and scalars only, because it is written as raw bytes. And accept the trade: a table declared in the sketch never passes through the generator, so `DatabaseLayout.h` will not include it in the build-time size check.

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

### 11.7 `AddingCommand`

A command is a struct deriving from `CommandBase` with a name, a usage line and an `execute`. Registering it is one static call, and the sketch makes that call from `setup()` after `initialize()` — the shell resolves commands by name when they run, so nothing has to be compiled into the framework.

What comes for free once it is registered: the command answers over serial, Telnet and SSH alike, tab completion offers it, `help` lists its usage line, and overriding `needauth()` puts it behind a login. Names are capped at seven characters.

### 11.8 `Logging`

`Log*` writes to the console. `SysLog*` writes to the console **and** appends to `/var/log/syslog.<level>`, which you can `cat` later. The example shows both, plus the case that decides between them: a path that discards data logs with `SysLogE`, because a console-only warning vanishes entirely on a board whose console log is off.

Which is the trap the example leads with — console logging ships switched off. Uncomment `ENABLE_CONSOLE_LOG_ALL` in `devices/DeviceConfig.h` or the sketch appears to do nothing at all.

### 11.9 `ScriptAndCron`

The sketch writes a shell script to `/hello.sh`, runs it straight away, and appends a row to `/etc/crontab` so the framework keeps running it every minute by itself. It is the feature seen from the application side rather than from the shell.

The part worth reading closely is which call to use. `ScriptRunner::runScheduledScript()` runs a file with nobody watching, as the user its `# UID` header names — the same call `initialize()` uses for `/etc/rc.local`. `ScriptRunner::runDetachedLine()` does the same for a single line, with the uid passed in. The two that look right and are not: `ScriptRunner::run()` wants the session of whoever asked for the script, and `__cmd_service.executeCommand()` wants a terminal **and** a session. A sketch has neither, so both answer `CMD_ERROR_NOTTY`.

Installation is keyed off the script file already existing, and the crontab row is **appended** rather than the table being rewritten — so a row you add or delete yourself survives a reboot. ⚠ Cron needs a real clock; on a board that never reaches the internet the row sits in the table and never fires, while running the script from the sketch still works.

### 11.10 Suggested order

Start with `PdiStack` to confirm the toolchain. Then `TaskScheduling`, because the scheduler is the primitive you reach for as soon as you add behaviour. `AddingCommand` and `Logging` are short and pay off immediately — one gives you a way to drive your own code from the shell, the other a way to see what it did. `ScriptAndCron` follows naturally: once there is something to run, that is how you make the device run it on its own. Then `MqttExample` for the read-modify-write-then-reload pattern, which transfers directly to WiFi, OTA and IoT. After that, take whichever of `AddingDatabaseTable`, `AddingController` and `DeviceIotExample` matches what you are building.

Two practical notes. Run `DeviceSetup.py` before compiling any of them if you are not building for the default port. And most examples stop the build with an `#error` when their feature flag is off — the stock config has them all on, so this only bites after you have trimmed the config for memory.

---
## 12. Memory & Performance Notes

The constraints scattered through the preceding sections, collected in one place so you can size a build before compiling it.

### 12.1 Budget per target

Roughly what is left after the board's core, its network stack and the standard library are linked. Boards are grouped by the budget they bring, since that is what actually decides the answer. Orders of magnitude, not guarantees.

| Class of board | Flash | RAM | NVM | What fits |
|---|---|---|---|---|
| kilobytes of RAM, no radio | ~32 KB | ~2 KB | ~1 KB | serial, EEPROM storage, basic GPIO, shell — no network |
| tens of kilobytes, with a radio | ~1 MB | ~50 KB free heap | ~4 KB | the full build short of SSH; contextual execution is comfortable |
| hundreds of kilobytes, with a radio | ~4 MB | ~250 KB free heap | ~4 KB | the full build, with room for a second contextual lane |

That table is why entire feature groups are gated on declared capability rather than assumed, and why the table limit is 5 where NVM is a kilobyte against 15 where it is a few.

### 12.2 Keeping strings out of RAM

Two macros, one purpose:

| Macro | On the ESP ports | Elsewhere |
|---|---|---|
| `RODT_ATTR("text")` | wraps the literal so it stays in flash | plain literal, same behaviour |
| `PROG_RODT_ATTR` | a storage qualifier that puts the variable in flash | empty; the variable is already read-only data |
| `PROG_RODT_PTR` | the right pointer type where program memory is a separate address space | a plain `const char*` |

The rule is simple: a literal used inline goes in `RODT_ATTR(...)`, and one held in a named variable is declared `static const char foo[] PROG_RODT_ATTR = "…"`. Every prompt, page fragment, log message and service name in the framework follows it, which is why the binary is dense rather than RAM-hungry.

When a consumer genuinely cannot take a flash pointer, `rofn::to_charptr()` copies the string into a fresh heap buffer and hands it over — and the caller owns it from there. It is an escape hatch, not a default; every call is an allocation.

### 12.3 The expensive features

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

**Sessions and their pools.** Every session slot is a fixed record in the table, so `PDI_MAX_SESSIONS` is a straight multiplier on static RAM — and because the table is sized from the telnet and SSH pools, growing a pool grows the table with it. A live session then costs a little heap on top for its line buffer and paths. On a tight port, shrink the pools rather than the table. The script runner adds a little to each slot too — the variables the session holds, and where it has got to in any script it is running — which is part of why it is behind a flag you can drop.

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

Use explicit-width integer types in config structs. `sizeof(int)` differs between 8-bit and 32-bit ports, and an NVM layout that depends on it is not portable.

Stay away from the `printf` family; the framework's own conversions save four to eight kilobytes by never linking libc's formatter.

Move firmware over OTA rather than SFTP — file transfer runs at 0.2 to 1 KB/s by design ([§7.10](#710-sftp-and-scp-file-transfer)).

### 12.8 Looking at a running device

`ps` gives every scheduler task with its rolling CPU share, run count and interval — the fastest way to spot a service hogging the loop. `service list` shows service state and task counts, and `service status <name>` drills into one, covering database validity, WiFi state, MQTT connection and the rest. A port that implements the optional stack-measurement hook also lets you bracket critical work and read back a high-water mark.

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
| `iInstanceInterface` | device | anything needing a connection | new TCP client and server, new UDP socket, new TLS client and server, the **shared** outbound TCP and TLS clients, filesystem and utility handles |
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
| `iNetifInterface` | `/sys/class/net`, `/proc/net` | one network link describing itself: name, kind, hardware address, addressing, link state. Traffic counters are optional — a link that cannot count says so |
| `iNetStackInterface` | `/proc/net/tcp` | the stack behind the links, walked for its TCP endpoints. Endpoints belong to the stack rather than to any one interface, so they are asked for here and not through a netif. Optional: a port that cannot enumerate its stack registers none |

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

Not every interface is worth rewriting per device. The portable defaults ship under `impl/`, and they are what a new port should reach for first.

The HTTP server implementation is a protocol-correct HTTP/1.1 server built on nothing but the TCP server and client interfaces — and with TLS enabled, the same file serves HTTPS by wrapping accepted connections. The filesystem side is larger: LittleFS on top of any storage interface, the dispatcher that routes one tree across several backends, and the generated filesystems behind `/proc`, `/sys`, `/dev` and `/tmp`. None of them are per-board.

So a new device needs to supply raw TCP and raw storage, and inherits the HTTP server, the HTTPS server, the whole filesystem and every synthetic mount for free. The TLS classes are deliberately not here, because SSL backends differ enough from one another that each port supplies its own pair.

### 13.5 What an implementation must promise

Construction is cheap and free of side effects. The singletons are constructed before `setup()` runs, so a constructor must not allocate, touch hardware, or open a port — that work belongs in an init method the orchestrator calls.

Methods do not block unless they say so. Anything that might take more than a few milliseconds — a TCP connect, an NTP fetch, an OTA download — either returns early or exposes a state machine, and the scheduler drives the long form.

Re-entrancy is not assumed. The framework is single-threaded by default; when a port enables contextual execution, the implementation rather than the caller is responsible for guarding shared state.

### 13.6 Adding an interface

The bar is whether at least two devices could implement it differently. If they could:

1. Pick the group — drivers for silicon, middlewares for network and device operations, modules for orthogonal features, threading for execution, top level for cross-cutting concerns.
2. Forward-declare the concrete class and declare the `extern` singleton at the bottom.
3. Guard it with the same flag that gates the service consuming it, so no existing port has to provide anything until it opts in.
4. Add an implementation to the host port so the off-device build still links.
5. Write it up here.

An interface with exactly one implementation is usually a sign the abstraction is premature — keep it in device-specific code until a second port needs it.

---
## 14. Device Layer & Porting Guide

The device layer is the only place vendor SDK and Arduino-core symbols are allowed. Everything above it talks through abstract interfaces. A port is one folder under [devices/](devices/) implementing those interfaces for one MCU family.

### 14.1 What a port contains

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
| TLS client and server | with TLS | handed out when something asks for a secure address, so a build with TLS still reaches plain ones |
| cert provisioner | with on-device cert generation | free functions rather than a virtual interface |
| threading family | with contextual execution | also required by TLS, which runs off the main stack |
| GPIO and watchdog | always, folded in | implemented as part of device control rather than as separate classes |

A port is valid the moment the always rows compile and link. Everything else arrives as you turn flags on.

### 14.3 The two aggregators

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

### 14.4 How a board gets selected

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

### 14.5 The singletons a port must define

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

### 14.6 The optional threading port

Four pieces make the contextual lanes work: a scheduler for each lane, a context implementation that saves and restores CPU state, a matching mutex and condition variable, and a periodic tick source to drive preemption.

Leave the layer out and contextual tasks are simply unavailable — inline tasks keep working, because they run on the loop's own stack. [§4](#4-task-scheduler) covers the trade-offs.

Enabling TLS turns this layer on implicitly, because both SSL backends need more stack than the Arduino main context has; the handshake runs on a dedicated cooperative task instead.

### 14.7 Per-board database schema

Each port carries its own table schema describing what lives in NVM on that board, tuned to its capacity. The setup script turns it into C++ table sources. Format is in [§5.6](#56-where-the-tables-come-from).

### 14.8 Porting, step by step

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

### 14.9 Before you call it done

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

`__utl_event` is a synchronous publisher. Services add listeners at boot and fire events from state changes, without any of them taking a direct dependency on another. Subscribing returns an id and `remove_event_listener(id)` gives it back, which is what lets a service that listens rather than scheduling a task be stopped and restarted without leaving listeners behind. Event names are centralised, and the usage patterns are in [§6.4](#64-the-event-bus).

### 15.2 String operations

Bounded, `printf`-free helpers — substring search, trim, compare, find-and-replace, a small JSON field extractor, IPv4 conversions, case folding. Every one takes an explicit length bound, defaulting to 300, because the NVM config strings are fixed-size arrays that are often not null-terminated. Keep the bound.

### 15.3 Data conversions

Integer, hex and BCD conversions in both directions with no `stdio` dependency. Using these instead of libc's formatter is worth four to eight kilobytes of flash on the smaller targets.

### 15.4 Base64

Encode and decode, plus a unique-key generator used for curve25519 private keys, SSH keygen seeds and SFTP file handles. Web session and CSRF tokens do not come from here — they are drawn from the platform random source behind `random_now()`, which is the hardware RNG on both ESP ports.

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

The Curve25519 and Ed25519 code comes from the standard portable reference. The RSA and big-integer layer is self-contained and device-agnostic, and the caller injects the RNG.

Every long asymmetric operation yields through one shared hook, `crypto_set_yield_hook` — the RSA ladder, the Ed25519 scalar multiplications and the Curve25519 ladder all call it from their inner loops. Install it around the operation and clear it afterwards, including on an early return, or the next caller inherits it. That is what carries on-device keygen and a handshake through without disabling the watchdog — on some ports disabling it stops the hardware feed rather than suspending it, which defeats the purpose entirely. SSH leans on all of it: host key generation, the key exchange, host-key signing, public-key authentication, and AES-CTR for transport encryption.

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
| persist new configuration | a database table | [§5.11](#511-adding-a-table) |
| add a screen to the portal | a controller and a page | [§8.10](#810-adding-a-page) |
| add a terminal command | a command class | [§7.12](#712-adding-a-command) |
| persist something only your sketch cares about | the database escape hatch | [§11.3](#113-addingdatabasetable) |
| react to another service without coupling to it | the event bus | [§6.4](#64-the-event-bus) |
| run periodic or long work | the scheduler | [§4](#4-task-scheduler) |
| encrypt a connection going out | turn on TLS and use the secure address — HTTP reads it from the URL, MQTT from the broker setting | [§6.2.18](#6218-tls-no-provider-transport-hookup--cert-provisioning) |
| serve the portal over HTTPS | turn on HTTPS and drop a cert and key on the filesystem | [§8.7.1](#871-https-wiring-and-certificates) |
| call a service from a sketch | the global | [§16.9](#169-calling-a-service-from-a-sketch) |

When a need straddles two rows, the lower layer is usually right. A transport beats a service that hard-codes a protocol; an interface beats a service that reaches for a vendor header.

### 16.2 A new device

Create the folder with its SDK umbrella header, its platform-macro header and the two aggregators. Implement device control, database and the instance factory, defining their globals. Branch on the new `DEVICE_*` in both the device config and the interface selector. Add the per-board limits. Run the setup script, build the bundled example, then add optional interfaces one flag at a time. Finally add the architecture to `library.properties`. Full walk-through in [§14.8](#148-porting-step-by-step), checklist in [§14.9](#149-before-you-call-it-done).

### 16.3 A new interface

Pick the group, write the header with pure virtuals plus a forward-declared concrete class and its `extern` singleton, guard it behind the flag that gates its consumers, and add a host-port implementation so off-device builds still link. If more than one port would end up writing the same logic, put a default implementation under `impl/` instead.

The bar is that two ports would genuinely implement it differently. A single implementation belongs in the device folder instead.

### 16.4 A new service

Add the flag, add the `service_t` value behind it, and write the provider:

```cpp
<Name>ServiceProvider() : ServiceProvider(SERVICE_<NAME>, RODT_ATTR("<Name>")) {}

bool initService(void* arg) override {
    // load config, register tasks through the base wrappers, add listeners
    return ServiceProvider::initService(arg);
}
```

Include it in the orchestrator and call its `initService` in the right slot for the init order. Implement the two print hooks so `service` can show it. Add a command, a page or a table if it earns them.

### 16.5 A new transport

Write the client under `src/transports/<proto>/`, taking an `iClientInterface*` and exposing begin, send, receive and end. Keep the database and the scheduler out of it. Put the service on top, wire one flag through the orchestrator, done.

That split is what makes a transport reusable from a sketch, or from a second service, without dragging the framework along. HTTP, MQTT and SMTP are the references.

### 16.6 A new table

Two paths, depending on who needs to see it.

A framework table — one that has to round-trip on every port and show up in tooling — means defining the struct in a config header, picking an id nothing else uses, adding a schema entry for each board that carries it, regenerating, and adding the accessor pair to the database service. [§5.11](#511-adding-a-table) has the detail.

A sketch-local table skips all of that: declare a `DatabaseTable<ADDR, my_struct>` subclass in the `.ino` and let static-init register it, as in [§11.3](#113-addingdatabasetable). It never passes through the generator, so framework tooling will not see it.

### 16.7 A new page

Add the route constant, add a page header holding the body HTML in program memory, write the controller and register the route in `boot()` — with the auth middleware unless the page is deliberately public — then add it as a member of the web server and put a menu entry on the home page. Compose the response as header, body and footer in three chunked sends.

A sketch can add a controller the same way without touching the framework; [§11.4](#114-addingcontroller) shows it.

### 16.8 A new command

Add the name constant, write the command struct with its options and `execute`, then include and register it in the CLI service. Completion, history and Ctrl+C come for free. Keep the name within eight characters and the options within three ([§7.12](#712-adding-a-command)).

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

    __utl_event.add_event_listener(EVENT_WIFI_STA_GOT_IP, [&](void*) {
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
## 17. Test Suite

Everything runs from one command:

```
python3 tests/run_tests.py
```

Exit status is zero only when every selected test passed. `tests/README.md` is the full reference;
this section is the shape of it.

### 17.1 Four tiers

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

The unit tier asserts on functions and runs under Address and UB sanitizers, so an out-of-bounds
read fails the run rather than passing quietly. The middle two drive the framework the way a user
does — a shell, an ssh channel, the web portal, an MQTT broker. The fuzz tier feeds the ssh wire,
sftp, http, shell, database-record and config parsers whatever a stranger could send before they
have authenticated.

With several `--device` targets, `--mode interleave` opens all of them at once and gives each test
the next transport in rotation. It is roughly three times faster than running the suite per
transport, and it is the only mode where the transports contend for the board, which is where
session and pool defects show up.

### 17.2 How the host build works

`src/` contains no Arduino or vendor SDK includes; every SDK dependency lives under `devices/`. The
test build compiles the real framework sources against the host port in `devices/posix/` with
`MOCK_DEVICE_TEST` defined on the command line. That gate is the only thing selecting the host
port — `devices/DeviceSetup.h`, which records the board you build firmware for, is never read or
written by a test run, so testing never disturbs your build.

`scripts/SyntaxSweep.py` compiles every translation unit on its own, across the C++ standards the
boards use and across feature-flag profiles with services switched off. It is what catches a file
that only ever compiled because something else happened to include a header first, and a guard that
was never exercised because no board turns that combination on.

### 17.3 Feature suites

`tests/suite/features/` holds one file per area: terminal, shell grammar, filesystem, generated
filesystems, users, sessions, processes, networking, name resolution, portal, ssh, sftp, scp,
timeouts, mDNS and MQTT. Each is written against a `Target` rather than a transport, so one source
runs against the host process and against a board over any of the three transports.

Two rules keep the results honest. A capability difference is found by **attempting** the thing and
skipping with the reason, never by branching on the transport's name. And a test that needs a peer
service brings its own — the suite ships a small MQTT broker and a multicast DNS client, each bound
to a port of its own, because pointing a test at a broker already running on the developer's machine
tests the machine rather than the board.

State that outlives a run, such as the hosts file or the MQTT config in the device database, is read
first and written back afterwards, so a run leaves the board as it found it.

### 17.4 Adding a test

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

---
## 18. Troubleshooting & FAQ

Short entries; the explanations live in the sections they point at.

### 18.1 Build and flash

**The build succeeds for a board but the device misbehaves.**
The setup script was never run for that target, so the default-port fallback produced a binary shaped for the wrong board — right code, wrong table set and flags. Run `python3 DeviceSetup.py -d <board>` and rebuild ([§2.5](#25-how-the-default-port-works)).

**The build succeeds but `service list` is empty and no access point appears.**
Same cause seen from the other end: `devices/DeviceSetup.h` still names the previous board. Re-run the script, or delete the file to fall back to the default port.

**`multiple definition of __i_<x>`.**
Either two ports define the same singleton, or a device `.cpp` was included from outside its aggregator chain. Every device translation unit must be reached through exactly one aggregator ([§14.3](#143-the-two-aggregators)).

**A vendor SDK header is not found while building for a board that does not have it.**
The header leaked above the device layer — the giveaway is that it builds for one port and not another. Push the include down into the port ([§16.10](#1610-what-to-watch-for)).

**Compile errors inside `pdiutil::function` or `pdiutil::vector` on an unusual target.**
The toolchain is missing the GCC extensions PdiSTL relies on. Use a GCC-based toolchain.

### 18.2 Boot and runtime

**The device factory-resets every five seconds.**
NVM is invalid — a corrupt checksum, or a struct that changed shape since the last flash. With auto-reset on, one cycle recovers it. If it loops, a table no longer fits the space reserved for it ([§5.11](#511-adding-a-table)).

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

### 18.3 Network and portal

**Cannot join the `pdiStack` access point.**
The password is `pdiStack@123`, case-sensitive. Confirm the radio is up with `service status WiFi` over serial.

**Cannot reach `http://192.168.0.1`.**
With dynamic subnetting on, the access point may have chosen a different subnet — use the `.1` of whatever your client was given.

**The login form keeps bouncing back to itself.**
The cookie is being rejected or the five-minute idle window expired. Private browsing windows often refuse cookies for a bare IP. After several wrong passwords the login route also holds attempts off for a lockout period — `passwd` over serial or SSH still works meanwhile.

**A form submits and lands back on the login page without saving.**
Its CSRF token no longer matches the session, which is what happens when a page is left open across a logout, a password change or a session expiry. Reload the page and submit it again. A form that does this every time is missing `concat_csrf_input_html_tag()` ([§8.10](#810-adding-a-page)).

**A page arrives truncated mid-HTML.**
A `send()` went out without chunking and exceeded the per-send buffer. Compose in header, body and footer ([§8.7](#87-composing-a-response)).

**MQTT connects but no callbacks fire.**
Wait for the subscribed callback before publishing on that topic — `Subscribe` reports enqueue, not acknowledgement ([§10.2](#102-mqtt)).

**OTA polls the server but never updates.**
The device only updates when the server's version is strictly newer than the firmware version compiled into it. Bump the firmware version before publishing ([§6.2.6](#626-otaserviceprovider--__ota_service)).

**Email sends time out.**
SMTP goes out in plaintext, so providers that require TLS on submission will not complete. Use a relay that accepts plain SMTP, or a test sink ([§10.3](#103-smtp)).

**HTTPS refuses connections or fails the handshake.**
The certificate and key are missing from the filesystem or sitting at different paths than configured. Upload them to `/etc/http/server.crt` and `/etc/http/server.key`; under mTLS the client CA is needed too ([§8.7.1](#871-https-wiring-and-certificates)).

**Outbound HTTPS to an unknown CA fails.**
The bundled client ships with peer verification off so first-boot demos work. For production, point it at `/etc/ssl/ca-bundle.crt` and drop the `setVerifyPeer(false)` line.

**TLS handshakes fail on large records.**
Two ceilings meet here. The handshake runs on a dedicated task stack sized by `TLS_TASK_STACK_SIZE` because the default main stack is too small for an ECDSA sign, and the inbound record buffer defaults well below the 16 KB a peer may emit. Raise the buffer — and pay the heap — only when you cannot control the peer.

**HTTPS works on a roomier board and refuses everything on a tighter one.**
NAPT is almost certainly on as well. The two cannot share that heap ([§12.3](#123-the-expensive-features)).

**The auto cert-generation listener never fires.**
It hangs off the station getting an IP, so it does not run in AP-only mode. Check with `net ip`.

### 18.4 Shell

**Telnet or SSH rejects the right credentials.**
The shell shares the portal's credential row. If they were changed through the portal, the shell inherits them.

**SSH accepts no clients.**
The Ed25519 host key is created when the service starts, so this means the write failed — a full or unmounted filesystem. Check `df`, then regenerate:
```
sshkgen t=1,f=b
```
`t=1` is Ed25519 and is instant; `f=b` is the `/etc/ssh` host-key directory.

**`scp -s` is slow.**
Transfers stream in small chunks and pay flash-write cost on overwrite, which lands around 0.2 to 1 KB/s. Firmware belongs on OTA ([§7.10](#710-sftp-and-scp-file-transfer)).

**`fedit` will not exit.**
Press Esc for the menu, then `!w` to save and leave or `!c` to discard.

**Tab completion works but the arrow keys don't recall history.**
History is a file, so it needs storage; completion reads the in-RAM registry and works either way ([§7.6](#76-the-dispatcher)).

**A second telnet client just hangs.**
It won't any more: telnet keeps a pool, and a client arriving when every slot is taken is told `no telnet session available, try again later` and closed. If you see that message, either a session is idle and about to be reaped, or the pool is genuinely smaller than the number of people trying to log in — raise `TELNET_MAX_SESSIONS`, and the session table grows with it ([§7.9](#79-multi-terminal-session-lifecycle)).

**`echo something > /proc/uptime` prints `cannot write`.**
That is correct — `/proc` is generated and read-only. The message appears when the last block is written, so a redirect that fails halfway still tells you ([§7.7](#77-pipes-and-redirection)).

### 18.5 Questions that come up

**Can I run two `PdiStack` instances?**
No. One global stack, one singleton per interface and per service — and on these memory budgets there would be no gain.

**Can I use Arduino `String` instead of `pdiutil::string`?**
In your sketch, yes. In framework code, no — its allocator differs per core, which is exactly the portability the framework is built to avoid depending on.

**Why are command names so short?**
Eight characters for a name and three for an option are sized for the tightest board the framework supports, and the cost of loosening them multiplies across every command slot.

**Why is SSH so heavy?**
Host keys, key exchange, streaming AES and per-session protocol state add roughly 150 KB of flash and 8 to 16 KB of heap per session. That is why SSH wants a board with heap to spare ([§12.3](#123-the-expensive-features)).

**Can the device be reconfigured without reflashing?**
Yes. Everything persisted in NVM is reachable from the portal, the shell, or a sketch. Flashing only changes defaults ([§3.8](#38-defaults-are-not-current-values)).

**Where do logs go?**
Console lines go to serial at 115200. With syslog on, those lines also land in `/var/log/syslog.<level>`, and with forwarding on they go to a collector as well ([§9.6](#96-where-syslog-goes)).

**How do I turn on TLS?**
Set `ENABLE_TLS_SERVICE`; the port supplies whichever SSL backend it has. Add `ENABLE_HTTPS_SERVER` for the portal, `ENABLE_HTTPS_SERVER_MTLS` for client certificates, and the cert-generation flags for on-boot minting where the port declares support. It is the most expensive flag in the framework ([§12.3](#123-the-expensive-features)), and on a tight board it cannot share the heap with NAPT.

**How do I provision certificates?**
Where the port can generate them, `tls q=1,t=0,l=256,n=device.local,i=192.168.1.50` writes a self-signed EC pair straight to the configured paths. Otherwise, `python3 scripts/GenTlsCerts.py --dns device.local --ip 192.168.1.50` produces them off-device for upload. Run the script once with `--gen-ca` and reuse that CA for every device, and a client that trusts it trusts your whole fleet.

For a development box none of that is needed, where the port can mint its own: with `ENABLE_SERVER_TLS_CERT_GENERATION_AT_RUNTIME` the mDNS service mints one covering the address and `<hostname>.local` as soon as the station has an IP, and reissues it if either changes.

**Is there a simulator?**
The host port runs the whole framework as a normal process on your computer, so most behaviour can be exercised without a board — the feature suites drive it over a shell, ssh and the portal. What it cannot stand in for is timing, radio and flash wear, so anything that depends on those still needs real hardware.

**How do I unit-test framework code?**
`python3 tests/run_tests.py` — see [§17](#17-test-suite). The framework compiles against the host port on host x86, so a unit test links the real source with no board attached.

**Where do I report issues?**
GitHub: <https://github.com/Suraj151/pdi-framework>.

### 18.6 When you're properly stuck

```
  1  enable console logging, flash, watch serial at 115200
  2  service list              what actually booted
  3  service status <name>     that service's state and its pids
  4  ps                     %CPU finds the hog; OWN ties tasks to sessions
  5  top i=2000,n=10        the same view over time
  6  service stop / start      freeze and resume, and watch the state column flip
  7  service status DB         NVM validity
  8  reboot                 explicitly, so you keep the serial output
```

If none of that localises it, open an issue with the board and package version, the flags you have set, the serial log through the failure, and the exact sequence that reproduces it.
