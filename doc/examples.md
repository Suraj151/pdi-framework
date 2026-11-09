# 11. Examples Walkthrough

[examples/](../examples/) has three tracks: one end-user sketch that brings the whole framework up, a `Dev/` tree of task-focused snippets, one per feature or extension surface, and a `Projects/` tree of small devices that put several features together the way a real one would. They are deliberately minimal — copy, adapt, ship.

| Example | Demonstrates |
|---|---|
| `PdiStack` | the smallest possible sketch: initialise, then serve |
| `WiFiSetup` | joining your own WiFi from the sketch, then printing the address the router gave |
| `TaskScheduling` | all three task modes, plus rescheduling and cancelling |
| `AddingCommand` | your own shell command, reachable over serial, Telnet and SSH |
| `Logging` | console logging against logging to a file under `/var/log` |
| `ScriptAndCron` | writing a script from the sketch, running it, scheduling it in `/etc/crontab` |
| `AddingDatabaseTable` | app-side persistence without touching the codegen |
| `AddingController` | a custom web route behind auth |
| `MqttExample` | configuring MQTT from code and wiring callbacks |
| `DeviceIotExample` | implementing the IoT sensor interface |
| `GpioControl` | pins as files: a blinking output, a digital and an analog input, read from the sketch |
| `FilesAndProc` | directories, files and `/proc` from code, and a log that never outgrows its cap |
| `SdCard` | an SD card reader declared once and mounted at `/mnt` on every boot |
| `EmailAlert` | the mail settings from code, a mail at boot and one when an input goes high |
| `Projects/SensorLogger` | a reading on a schedule, kept in a capped CSV, shown by a command, published over MQTT |
| `Projects/RemoteRelay` | one output switched from the shell, a portal page and MQTT, kept across restarts |
| `Projects/SelfMonitor` | the board checks its own free memory and flash every five minutes and logs a warning past a limit |

## 11.1 `PdiStack`

```cpp
#include <PdiStack.h>

void setup() { PdiStack.initialize(); }
void loop()  { PdiStack.serve(); }
```

That is the entire file. Every service the device config enables brings itself up inside `initialize()`, and `serve()` ticks all of them. This is what you flash to a fresh board.

After flashing you should see the boot banner on serial at 115200, the `pdiStack` access point appear, and the portal at `http://192.168.0.1` accept the default login with every menu live.

Start here. Every other example assumes `initialize()` already ran.

## 11.1.1 `WiFiSetup`

The first thing most boards need after `PdiStack`: to be on your own network, so you can reach them by address. The sketch reads the WiFi settings table, writes your network's name and password into it only when they differ from what is stored, and restarts the WiFi service — the same steps `net connsta,<ssid>,<password>` takes from the shell. A one-shot task then looks for the station address every second and prints it on the serial console once the router has given one.

Put your network in `MY_WIFI_SSID` and `MY_WIFI_PASSWORD` before uploading. The settings are kept, so later boots join on their own, and the comparison means a reboot does not rewrite the flash.

## 11.2 `TaskScheduling`

Four behaviours layered on the bare stack: a one-shot a second after boot, a periodic task at three seconds, a second one-shot at fifteen seconds that changes the periodic task's cadence to one second, and a third at thirty that cancels it. That is the whole registration API exercised in one file.

Behind the contextual-execution flag, two more tasks show the other lanes — each a `while(1) { … sleep(500); }` body promoted onto its own stack:

```cpp
int cooperative_task_id = __task_scheduler.register_task(cooperative_task);
__task_scheduler.scheduleUnderExecSched(&__i_cooperative_scheduler,
                                        cooperative_task_id,
                                        TASK_MODE_COOPERATIVE, 1 * 1024);
```

The framework's console log ships switched off, so the serial monitor shows only the demo's own prints. If you have turned the console log gates on, turn them off again for this one, or the framework's output interleaves with the demo and the schedule becomes hard to read.

## 11.3 `AddingDatabaseTable`

The normal database flow goes through the JSON schema and the generator. This example takes the escape hatch: declare a table subclass directly in the sketch and let the same static-init mechanism register it.

```cpp
#define STUDENT_TABLE_ID  10            // 1..9 belong to the framework tables

struct student_table { student_t students[MAX_STUDENTS]; int student_count; };

class StudentTable : public DatabaseTable<STUDENT_TABLE_ID, student_table> {};
StudentTable __student_table;           // registers itself at static-init
```

After `initialize()`, the sketch builds a value and calls `set`, and a five-second task reads it back with `get` and prints it.

Three things it teaches. Take an id nothing else holds and keep it — the engine finds the record by id, and reusing one a removed table had hands the new table the old one's bytes. Keep the struct POD, fixed-size arrays and scalars only, because it is written as raw bytes. And accept the trade: a table declared in the sketch never passes through the generator, so `DatabaseLayout.h` will not include it in the build-time size check.

## 11.4 `AddingController`

A `/test-route` behind auth, which is [§8.10](web-server.md#810-adding-a-page) made concrete:

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

The page is sent in chunks, the way the shipped controllers send theirs ([§8.7](web-server.md#87-composing-a-response)): `BEGIN_SEND_IN_CHUNK` starts the response, each piece is added to the `PAGE_HTML_MAX_SIZE` (1800 byte) buffer and flushed with `CONTINUE_SEND_IN_CHUNK`, and `END_SENDING_CHUNK` ends it. The framework's header and footer alone take most of that buffer, so a whole page built in one buffer fits only until a little more is added — one more menu card is enough to write past its end and reset the board. Sent in chunks, each piece only has to fit on its own, and the page can grow as far as you like.

## 11.5 `MqttExample`

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

## 11.6 `DeviceIotExample`

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

## 11.7 `AddingCommand`

A command is a struct deriving from `CommandBase` with a name, a usage line and an `execute`. Registering it is one static call, and the sketch makes that call from `setup()` after `initialize()` — the shell resolves commands by name when they run, so nothing has to be compiled into the framework.

What comes for free once it is registered: the command answers over serial, Telnet and SSH alike, tab completion offers it, `help` lists its usage line, and overriding `needauth()` puts it behind a login. The constructor declares the words it takes with `setArgs(flags, operands, keyvalue)` — `hello` takes no flags and one name — and the shell then refuses an unknown `-x` (`invalid option : -x`) and a word past the last operand (`extra operand : <word>`) with the usage line, before `execute` runs; `hello "John Smith"` passes the quoted name as one word ([§7.4](shell.md#74-the-command-contract)). Names are capped at `CMD_SIZE_MAX` less one, 15 characters.

## 11.8 `Logging`

`Log*` writes to the console. `SysLog*` writes to the console **and** appends to `/var/log/syslog.<level>`, which you can `cat` later. The example shows both, plus the case that decides between them: a path that discards data logs with `SysLogE`, because a console-only warning vanishes entirely on a board whose console log is off.

Which is the trap the example leads with — console logging ships switched off. Uncomment `ENABLE_CONSOLE_LOG_ALL` in `devices/DeviceConfig.h` or the sketch appears to do nothing at all.

## 11.9 `ScriptAndCron`

The sketch writes a shell script to `/hello.sh`, runs it straight away, and appends a row to `/etc/crontab` so the framework keeps running it every minute by itself. It is the feature seen from the application side rather than from the shell.

The part worth reading closely is which call to use. `ScriptRunner::runScheduledScript()` runs a file with nobody watching, as the user its `# UID` header names — the same call `initialize()` uses for `/etc/rc.local`. `ScriptRunner::runDetachedLine()` does the same for a single line, with the uid passed in. The two that look right and are not: `ScriptRunner::run()` wants the session of whoever asked for the script, and `__cmd_service.executeCommand()` wants a terminal **and** a session. A sketch has neither, so both answer `CMD_ERROR_NOTTY`.

Installation is keyed off the script file already existing, and the crontab row is **appended** rather than the table being rewritten — so a row you add or delete yourself survives a reboot. ⚠ Cron needs a real clock; on a board that never reaches the internet the row sits in the table and never fires, while running the script from the sketch still works.

## 11.10 `GpioControl`

The pins the way the shell sees them: a directory per pin under `/sys/class/gpio`, with a `mode` file and a `value` file. The sketch writes them with `__i_fs.writeFile()` exactly as `echo 3 > /sys/class/gpio/4/mode` does, reads them back with `readFile()`, and prints on the serial console when an input changes. Pin 4 blinks every 500 ms (the on-board LED on a NodeMCU), pin 5 is a digital input, and the board's first analog pin — `MAX_DIGITAL_GPIO_PINS`, so A0 on an esp8266 and pin 12 on an esp32 — an analog one. The numbers are the framework's pins, not GPIO numbers ([pin numbers](services.md#6212-storage-interface-init-no-provider)).

The lesson is in the note at the top: **writing a pin's `mode` or `value` also saves the pin settings to flash**, which is why a pin comes back the same after a restart, and also why a sketch must never toggle a pin by writing its value in a loop. Hand the repetition to the GPIO service — the blink mode does the toggling itself — and write only when the setting changes, which `set_pin()` checks by reading the value first. Reading an input costs nothing: the service samples the pins on its own and `readFile()` only returns what it last saw.

## 11.11 `FilesAndProc`

The filesystem from the application side. On boot the sketch makes `/data`, writes `notes.txt` once and appends a line on every boot, prints its size and content, reads `/proc/uptime` and the `MemFree` and `MemMaxBlock` lines of `/proc/meminfo`, and from then on adds one line to `/data/heap.log` a minute.

`readFile()` hands a file over in pieces through a callback, so `read_text()` appends each piece and stops at a limit; the same helper reads a `/proc` file, which is made when it is read and never touches the flash. The heap log shows the rule for anything that keeps growing: once it reaches `HEAP_LOG_MAX` (2 KB) it is renamed to `heap.log.1`, replacing the one before, and a fresh one starts — the two together never hold more than twice the cap. The flash is small and wears with every write, so cap every log and write it no more often than you need.

## 11.12 `SdCard`

An SD card the linux way: the sketch adds `spi0 cs=5 mmc_spi` to `/etc/devices.conf` and `/dev/mmcblk0p1 /mnt vfat defaults,nofail 0 0` to `/etc/fstab`, each only when the file does not have it yet, then asks for a rescan with `udevadm trigger`. From then on the framework binds the reader and mounts the card at `/mnt` on its own — at boot, and whenever the card is put back in the slot ([disks](services.md#624-serialserviceprovider--__serial_service)). The sketch waits for `/mnt` to show in `/proc/mounts` and writes `hello.txt` on the card.

The reader's CS goes to GPIO 5 (D1 on a NodeMCU) and its clock and data lines to the board's SPI pins, listed per board in the README's [default bus pins](../README.md#supported-boards); the card needs a FAT filesystem on its first partition, as cards come. Declaring a device is a setting, not an action, so the sketch never mounts anything itself: after the first boot the two lines are there, the card comes up during boot, and the sketch's only remaining job is to use it. A file written before the board has the time carries the FAT epoch, 1 January 1980.

## 11.13 `EmailAlert`

The mail settings written from code — the same ones the portal's email page shows — and a mail sent twice over: once when the board has joined the network, and again when a digital input goes from low to high, at most once a minute. The settings are written only when one of them differs from what is stored.

Two things to know before relying on it. The mail goes over plain SMTP with no TLS, so a server that only takes encrypted mail refuses it; the board logs in only when `MAIL_USERNAME` is set, so a relay that takes mail without a login works with both left empty, and a mail that is not sent names the step the server refused in `/var/log/syslog.error`. And `__email_service.sendMail(body, done)` returns at once: it is refused while another mail is still going out, and when it is taken the mail goes out one step at a time while the board keeps running, then `done(sent)` says whether the server took it. So the boot mail is only marked done from `done`, and tried again every 30 seconds until then.

## 11.14 `Projects/SensorLogger`

A sensor logger made of the pieces above. A task reads the first analog pin every ten seconds; each reading is added as a row to `/data/sensor.csv`, which starts with an `uptime_s,value` header and rotates to `sensor.csv.1` at 8 KB; a `sensor` command prints the latest reading, the range since boot and the number of samples, over serial, Telnet and SSH alike; and when `MQTT_HOST` names a broker, the latest reading goes out as JSON on `pdistack_<mac>/sensor` every `MQTT_PUBLISH_FREQ` seconds.

Its MQTT setup writes every field a connection needs — host, port, TLS on or off, client id and keepalive — because a board keeps whatever it had stored before, and the client does not start at all without a client id or with a keepalive of 5 seconds or less. `[mac]` in the client id and the topic is replaced with the board's mac. Leave `MQTT_HOST` empty and MQTT is left as it is.

## 11.15 `Projects/RemoteRelay`

One output — a relay, or the on-board LED to try it — switched three ways: `relay on` and `relay off` in the shell, a `/relay` page on the portal behind the login with on and off buttons, and `on` or `off` published to `pdistack_<mac>/relay` when `MQTT_HOST` names a broker. However it is switched, the pin's value is saved with the GPIO settings, so the relay comes back the way it was after a restart.

The page carries the two lessons of a route that changes something. It changes the relay **only on a POST from its own form**, which carries the session's csrf token through `concat_csrf_input_html_tag()`; the framework rejects a POST without a valid token before the handler runs, but it cannot protect a GET, so a link that switched the relay would let any other page switch it through a logged-in browser. And it is **sent in chunks**: the framework's header and footer alone take most of the `PAGE_HTML_MAX_SIZE` (1800 byte) buffer, so the page opens with `BEGIN_SEND_IN_CHUNK`, flushes the buffer with `CONTINUE_SEND_IN_CHUNK` after each piece, and ends with `END_SENDING_CHUNK`, the way the framework's own controllers do. The whole page is over 5 KB; built in one buffer it would overrun it.

## 11.16 `Projects/SelfMonitor`

A board that watches itself, written in the shell rather than in C++. The sketch writes `/etc/selfmon.sh` and a crontab row, `*/5 * * * * source /etc/selfmon.sh`, once — both keyed off the script already being there, so a limit or a schedule you change on the board survives a reboot. The script reads the free memory from `free -b` and the free flash from `df`, and past `SELFMON_FREE_MIN` (8 KB) or `SELFMON_DISK_MIN` (64 KB) writes a warning with `logger -p warning`, which lands in `/var/log/syslog.warning` and goes out to a remote syslog collector when one is set. Run it by hand with `source /etc/selfmon.sh`; cron runs it every fifth minute once the clock is valid.

It shows one shell habit worth knowing: a `for` list does not run `$(...)` itself, so each answer is kept in a variable first (`export SM_L=$(free -b | grep Mem)`) and the variable is walked a word at a time. The script text sits in flash (`PROG_RODT_ATTR`) and is copied into memory only while it is written, so it costs the sketch no RAM afterwards.

## 11.17 Suggested order

Start with `PdiStack` to confirm the toolchain, and `WiFiSetup` to put the board on your network. Then `TaskScheduling`, because the scheduler is the primitive you reach for as soon as you add behaviour. `AddingCommand` and `Logging` are short and pay off immediately — one gives you a way to drive your own code from the shell, the other a way to see what it did. `ScriptAndCron` follows naturally: once there is something to run, that is how you make the device run it on its own. Then `MqttExample` for the read-modify-write-then-reload pattern, which transfers directly to WiFi, OTA and IoT. `GpioControl` and `FilesAndProc` cover the two things nearly every device touches, pins and files, and `SdCard` and `EmailAlert` add storage and a way to be told. After that, take whichever of `AddingDatabaseTable`, `AddingController` and `DeviceIotExample` matches what you are building — and read `Projects/SensorLogger`, `Projects/RemoteRelay` and `Projects/SelfMonitor` to see the pieces put together into a device.

Two practical notes. Run `DeviceSetup.py` before compiling any of them if you are not building for the default port. And most examples stop the build with an `#error` when their feature flag is off — the stock config has them all on, so this only bites after you have trimmed the config for memory.
