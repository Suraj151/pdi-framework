# 1. Architecture Overview

The stack is layered, and the layering is enforced by what each layer is allowed to include. The bottom half describes what a device must be able to *do* — pure abstract interfaces. The top half describes what the product *offers* — services, transports, the web server, the shell. A single device-selection macro and a set of `ENABLE_*` flags bolt the two halves together at compile time.

## 1.1 Layered model

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

Dependencies only ever point downward. A service never includes a device header; it holds an `i*Interface` pointer and calls through it. The one place device code is reached from above is [src/interface/pdi.h](../src/interface/pdi.h), which builds the include path for exactly one board aggregator out of `PDI_DEVICE` — the port's directory name, set on the compiler command line or written into `devices/DeviceSetup.h` by `scripts/DeviceSetup.py`.

| Layer | Role | Sees |
|---|---|---|
| Application | your sketch: `initialize()` once, `serve()` in the loop | orchestrator |
| Orchestrator | the `PdiStack` global that conditionally starts every enabled service | services, interfaces |
| Services | one `ServiceProvider` subclass per feature, self-registering in a global table | utilities, interfaces |
| Utilities | device-agnostic primitives — scheduler, event bus, database engine, crypto, STL subset | interfaces only |
| Interfaces | pure abstract ports, grouped by role into drivers, middlewares, modules and threading | nothing |
| Devices | the concrete per-MCU adapters | vendor SDK / Arduino core |

## 1.2 The `ServiceProvider` contract

Every feature service derives from `ServiceProvider` and puts itself into a global table indexed by a `service_t` enum:

```cpp
ServiceProvider(service_t st, const char *_svc_name)
    : m_service_t(st), m_service_name(_svc_name), m_service_routine_task_id(-1) {
    m_services[st] = this;          // self-registration
}
```

The base class fixes the lifecycle. `initService` is the required override, and callers never reach for it directly — `startService` wraps it and records what it returned, which is the state both `PDIStack::initialize` and the `service` command read. `stopService` tears down and records its own; an override calls the base to clear the tracked tasks. `getServiceDependencies` names what the service `Requires`, `printConfigToTerminal` and `printStatusToTerminal` supply what `service status` prints, and a static `getService(st)` lets one service find another without include cycles.

Because the shape is uniform, `service` can enumerate, start, stop and inspect anything without knowing what it is.

Start reading at [src/service_provider/ServiceProvider.h](../src/service_provider/ServiceProvider.h).

## 1.3 The ports

Interfaces are grouped by role, not by feature:

| Group | Examples | Implemented by | Used by |
|---|---|---|---|
| drivers | `iGpioInterface`, `iWdtInterface` | device adapter | GPIO service, core |
| middlewares | `iClientInterface`, `iServerInterface`, `iNtpInterface`, `iPingInterface`, `iDeviceControlInterface`, `iUpgradeInterface` | device adapter | OTA, MQTT, email, HTTP, IoT |
| modules | `iSerialInterface`, `iStorageInterface`, `iFileSystemInterface`, `iWiFiInterface`, `iHttpServerInterface` | device adapter | serial, storage, WiFi |
| threading | `iExecution`, `iMutex`, `iCondvar`, `iContext` | device adapter, where the board can | scheduler in contextual mode |
| top level | `iDatabaseInterface`, `iDeviceIotInterface` | device adapter / framework | database, IoT |

A port is usable once it provides `iDeviceControlInterface`, `iDatabaseInterface`, `iSerialInterface` (when the serial service is on), plus whichever groups the feature flags it claims to support actually need.

## 1.4 Compile-time composition

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

## 1.5 Runtime lifecycle

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

Two scheduling lanes live side by side. Inline tasks advance on every `serve()` tick and suit short periodic work. Contextual tasks run on their own stacks and the loop merely yields to them; they exist on boards whose port supplies `iExecution` and a matching scheduler. [§4](scheduler.md#4-task-scheduler) covers picking between them.

## 1.6 Global instances and naming

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
