# 11. Examples Walkthrough

[examples/](../examples/) has two tracks: one end-user sketch that brings the whole framework up, and a `Dev/` tree of task-focused snippets, one per extension surface. They are deliberately minimal — copy, adapt, ship.

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

The example composes its page into a heap buffer to keep the code short. Production controllers stream instead, sending header, body and footer as three chunked calls, which is what [§8.7](web-server.md#87-composing-a-response) describes and what the shipped controllers do.

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

## 11.10 Suggested order

Start with `PdiStack` to confirm the toolchain, and `WiFiSetup` to put the board on your network. Then `TaskScheduling`, because the scheduler is the primitive you reach for as soon as you add behaviour. `AddingCommand` and `Logging` are short and pay off immediately — one gives you a way to drive your own code from the shell, the other a way to see what it did. `ScriptAndCron` follows naturally: once there is something to run, that is how you make the device run it on its own. Then `MqttExample` for the read-modify-write-then-reload pattern, which transfers directly to WiFi, OTA and IoT. After that, take whichever of `AddingDatabaseTable`, `AddingController` and `DeviceIotExample` matches what you are building.

Two practical notes. Run `DeviceSetup.py` before compiling any of them if you are not building for the default port. And most examples stop the build with an `#error` when their feature flag is off — the stock config has them all on, so this only bites after you have trimmed the config for memory.
