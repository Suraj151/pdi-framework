# 16. Extending the Framework

Every section above has its own "how do I add one of these" part. This one is the index: find what you're building, follow the short version, and jump to the deep dive when you need it.

## 16.1 Which surface fits

| You want to | Reach for | Detail |
|---|---|---|
| support a new board | a device port | [§14](porting.md#14-device-layer--porting-guide) |
| add a hardware capability across all ports | a portable interface | [§13.6](interfaces.md#136-adding-an-interface) |
| add a framework-level feature | a service provider | [§6.5](services.md#65-writing-a-new-service) |
| speak a new wire protocol | a transport | [§10.5](transports.md#105-adding-a-transport) |
| persist new configuration | a database table | [§5.11](database.md#511-adding-a-table) |
| add a screen to the portal | a controller and a page | [§8.10](web-server.md#810-adding-a-page) |
| add a terminal command | a command class | [§7.12](shell.md#712-adding-a-command) |
| persist something only your sketch cares about | the database escape hatch | [§11.3](examples.md#113-addingdatabasetable) |
| react to another service without coupling to it | the event bus | [§6.4](services.md#64-the-event-bus) |
| run periodic or long work | the scheduler | [§4](scheduler.md#4-task-scheduler) |
| encrypt a connection going out | turn on TLS and use the secure address — HTTP reads it from the URL, MQTT from the broker setting | [§6.2.18](services.md#6218-tls-no-provider-transport-hookup--cert-provisioning) |
| serve the portal over HTTPS | turn on HTTPS and drop a cert and key on the filesystem | [§8.7.1](web-server.md#871-https-wiring-and-certificates) |
| call a service from a sketch | the global | [§16.9](#169-calling-a-service-from-a-sketch) |

When a need straddles two rows, the lower layer is usually right. A transport beats a service that hard-codes a protocol; an interface beats a service that reaches for a vendor header.

## 16.2 A new device

Create the folder with its SDK umbrella header, its platform-macro header and the two aggregators. Implement device control, database and the instance factory, defining their globals. Branch on the new `DEVICE_*` in both the device config and the interface selector. Add the per-board limits. Run the setup script, build the bundled example, then add optional interfaces one flag at a time. Finally add the architecture to `library.properties`. Full walk-through in [§14.8](porting.md#148-porting-step-by-step), checklist in [§14.9](porting.md#149-before-you-call-it-done).

## 16.3 A new interface

Pick the group, write the header with pure virtuals plus a forward-declared concrete class and its `extern` singleton, guard it behind the flag that gates its consumers, and add a host-port implementation so off-device builds still link. If more than one port would end up writing the same logic, put a default implementation under `impl/` instead.

The bar is that two ports would genuinely implement it differently. A single implementation belongs in the device folder instead.

## 16.4 A new service

Add the flag, add the `service_t` value behind it, and write the provider:

```cpp
<Name>ServiceProvider() : ServiceProvider(SERVICE_<NAME>, RODT_ATTR("<Name>")) {}

bool initService(void* arg) override {
    // load config, register tasks through the base wrappers, add listeners
    return ServiceProvider::initService(arg);
}
```

Include it in the orchestrator and call its `initService` in the right slot for the init order. Implement the two print hooks so `service` can show it. Add a command, a page or a table if it earns them.

## 16.5 A new transport

Write the client under `src/transports/<proto>/`, taking an `iClientInterface*` and exposing begin, send, receive and end. Keep the database and the scheduler out of it. Put the service on top, wire one flag through the orchestrator, done.

That split is what makes a transport reusable from a sketch, or from a second service, without dragging the framework along. HTTP, MQTT and SMTP are the references.

## 16.6 A new table

Two paths, depending on who needs to see it.

A framework table — one that has to round-trip on every port and show up in tooling — means defining the struct in a config header, picking an id nothing else uses, adding a schema entry for each board that carries it, regenerating, and adding the accessor pair to the database service. [§5.11](database.md#511-adding-a-table) has the detail.

A sketch-local table skips all of that: declare a `DatabaseTable<ADDR, my_struct>` subclass in the `.ino` and let static-init register it, as in [§11.3](examples.md#113-addingdatabasetable). It never passes through the generator, so framework tooling will not see it.

## 16.7 A new page

Add the route constant, add a page header holding the body HTML in program memory, write the controller and register the route in `boot()` — with the auth middleware unless the page is deliberately public — then add it as a member of the web server and put a menu entry on the home page. Compose the response as header, body and footer in three chunked sends.

A sketch can add a controller the same way without touching the framework; [§11.4](examples.md#114-addingcontroller) shows it.

## 16.8 A new command

Add the name constant, write the command struct with its options and `execute`, then include and register it in the CLI service. Completion, history and Ctrl+C come for free. Keep the name shorter than `CMD_SIZE_MAX` and the options within `CMD_OPTION_MAX` ([§7.12](shell.md#712-adding-a-command)).

## 16.9 Calling a service from a sketch

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

## 16.10 What to watch for

A handful of habits separate an extension that lands cleanly from one that fights the framework.

Do the work in `init*`, not in a constructor. Globals are constructed before `setup()`, when the device interfaces do not exist yet.

Use events for fan-out and direct calls only for structural dependencies. A service calling another from a callback creates an ordering dependency that breaks the next time init order changes.

Copy borrowed pointers. Command option values, request arguments and HTTP response bodies all point into buffers that are about to be reused.

Keep vendor headers below the device layer. One `#include <esp_wifi.h>` in a service breaks the build for every other port.

Remember that a feature flag gates three things — the device aggregator include, the orchestrator include, and the orchestrator's init call. Miss one and you get either a link error or dead code in the binary.

And keep one global per slot. A second service instance overwrites the first in the service table, and a second table at the same address is quietly skipped at registration.

## 16.11 A new driver

A driver is a class implementing `iDriverInterface` plus one registration that tells the serial service what it drives:

```cpp
class Bme280 : public iDriverInterface {
public:
    pdi_err_t probe(driver_device_t &dev, iSerialInterface *bus) override {
        if (SERIAL_IFACE_I2C != serialFamily(bus->type())) return PDI_ERR_NOT_SUPPORTED;
        iI2cSerialInterface *i2c = static_cast<iI2cSerialInterface *>(bus);
        uint8_t id = 0;
        i2c->select(dev.m_target);
        if (1 != i2c->readRegister(0xD0, &id, 1) || 0x60 != id) return PDI_ERR_NOT_FOUND;
        return PDI_OK;
    }
    void remove(driver_device_t &dev) override {}
};

static const uint8_t s_bme280_addresses[] = {0x76, 0x77};
static iDriverInterface *createBme280() { return pdiutil::safe_new<Bme280>(); }

void registerBme280() {
    DeviceManager::registerDriver(driver_prop_t(RODT_ATTR(DRIVER_NAME_BME280), SERIAL_IFACE_I2C,
        s_bme280_addresses, sizeof(s_bme280_addresses), nullptr, createBme280));
}
```

The registry keeps the name pointer, so the name is a define in `DriverConfig.h` handed over with `RODT_ATTR` from inside a function and stays in flash. Call the registration from `registerBuiltInDrivers()`, which each port calls from its `initDeviceSpecificFeatures()` before the services start.

The addresses are what a scan probes on I2C; a usb driver lists none and passes a match function that looks at `dev.m_class`, `m_vid` or `m_pid` instead. `probe` must confirm the chip and answer an error for anything else — the next driver then gets its turn. The driver object exists only while a device is bound to it, created by the factory when one turns up and deleted when it goes, so a driver with nothing bound costs its registry entry and nothing more. Check `bus->type()` before casting to a protocol's interface: some builds have no RTTI, so `dynamic_cast` is not available.

A driver that gives the user something to read overrides `nodeName()` with a path under `/dev` and `readNode()` with what a read returns, and the node appears while the device is bound. A driver that is a kind of device commands look for answers its class from `driverClass()` — a microphone derives from `iSoundDriverInterface`, says `DRIVER_CLASS_SOUND_CAPTURE` and gives its `rate()`, and `arecord` finds it with `DeviceManager::bound()` without knowing which chip it is. A memory derives from `iNvmemDriverInterface`, says `DRIVER_CLASS_NVMEM` and gives its `size()`, `readAt()` and `writeAt()`, and its device directory gains an `eeprom` file that reads and writes through them. A disk derives from `iBlockDriverInterface`, says `DRIVER_CLASS_BLOCK` and gives its `blocks()`, `readBlocks()` and `writeBlocks()`; its `nodeName()` becomes a sized, `0600` disk under `/dev` and it appears in `/sys/block` and `lsblk`.

An SPI driver talks to its device inside a transaction: `begin(cs, clock, mode)` takes the bus at the clock the device needs, held to the bus's top clock, and pulls the chip select low, `exchange()` clocks bytes out and in at once, and `end()` lets the chip select go. A port holds the bus from `begin()` to `end()` against other tasks, so a driver's commands and data are never split by another device's.

A driver whose device stops answering on its bus while it works — an eeprom storing a page — overrides `busy()` to say so for that time, and a scan then takes the device as still there instead of letting it go.

A driver for something that can be taken out on a bus no scan can see — a card on SPI — registers with its last `driver_prop_t` argument `true` and overrides `stillThere()` with a quick question to the device. Each scan then asks a bound one whether it is still there and lets it go when it is not, and offers a missing one to the driver again, so the device comes and goes the way a detected one does.

A port's bus reports the pins it runs on by default from `defaultPins()`, and an I2C bus takes a bus line's pins and clock through `configure(sda, scl, clock)`, refusing a pin it cannot use. A port's `isExceptionalGpio()` asks `isDriverGpio()` on the device control interface, which answers from the pins the lines in `/etc/devices.conf` hold, so a new port gets the same pin handling by making that one call.
