# 3. Configuration System

Configuration is layered and additive. Which services *exist*, how big each table is, what the defaults are and which interfaces a port supplies are decided when you build. What a service *does* once it exists — including whether it runs at all — is settled at runtime, from text files under `/etc` on any board that has a filesystem. See §3.10.

## 3.1 Three tiers

| Tier | Lives in | Owns | Written by |
|---|---|---|---|
| Device | `devices/DeviceSetup.h` (generated), `devices/DeviceConfig.h`, and each port's `device_config.h` | the `PDI_DEVICE` selector, the `ENABLE_*` flags, per-board limits, and platform macros such as `RODT_ATTR` and `CRITICAL_SECTION_ENTER/EXIT` | integrator picking board and features; porter describing the board |
| Common | `src/config/Common.h`, `src/config/GlobalConfig.h` | cross-cutting constants and the always-present `global_config` table | framework author, integrator |
| Service | one `*Config.h` per service under `src/config/` | per-service knobs plus the struct that gets persisted to NVM | service author |

Everything funnels through [src/config/Config.h](../src/config/Config.h), which conditionally includes the service-tier headers according to the flags the device tier settled on. Every translation unit in the framework reaches configuration through that one header.

## 3.2 How the pieces include each other

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

## 3.3 Device-tier flags

Every flag acts as a triple gate: which interface the device exposes, which service the orchestrator includes, and which config header `Config.h` pulls in.

### 3.3.1 Service flags

| Flag | On by default | Brings in | Needs | Cost |
|---|---|---|---|---|
| `ENABLE_SERIAL_SERVICE` | all boards | serial service + `SerialConfig.h` | `iSerialInterface` | low |
| `ENABLE_DRIVER_CORE` | boards declaring `DEVICE_SUPPORTS_DRIVER_CORE` | devices found on the serial interfaces, their drivers, `/etc/devices.conf`, `/sys/bus`, `udevadm` | the serial service | low |
| `ENABLE_I2S_BUS` | boards declaring `DEVICE_SUPPORTS_I2S` | the I2S bus, the `inmp441` microphone driver, `/dev/snd`, and `arecord` with storage | the driver core | low |
| `ENABLE_I2C_BUS` | boards declaring `DEVICE_SUPPORTS_I2C` | the I2C bus and the `at24` eeprom driver with its `eeprom` file under `/sys/bus/i2c` | the driver core | low |
| `ENABLE_SPI_BUS` | boards declaring `DEVICE_SUPPORTS_SPI` | the SPI bus and the `mmc_spi` SD card driver with its `/dev/mmcblk0` disk | the driver core | low |
| `ENABLE_FAT_FS` | with the driver core and storage | mounting the FAT volume a disk or partition carries, `mount <device> <dir>`, `mkfs -t vfat`, and `FAT_VOLUMES_MAX` (2) more mount slots | the driver core, storage | medium; a mounted volume holds one sector of cache, an open file on it a little more |
| `ENABLE_LFS_DISK` | with the driver core and storage | mounting and making a littlefs volume on a disk or partition, `mkfs -t littlefs`, and `LFS_DISK_VOLUMES_MAX` (1) more mount slot | the driver core, storage | low until mounted; a mounted volume takes about 1.5 KB of heap, an open file on it 512 bytes more |
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
| `ENABLE_TLS_SERVICE` | off | TLS client and server + `TlsConfig.h`; turns on contextual execution, since TLS runs on its own cooperative task | whichever TLS backend the port supplies | high — see [§12.3](memory-performance.md#123-the-expensive-features) |
| `ENABLE_TLS_CERT_GENERATION` | off | the `tls` command and the on-device issuer | TLS service, and a port declaring `DEVICE_SUPPORTS_TLS_CERT_GENERATION` | medium |
| `ENABLE_SERVER_TLS_CERT_GENERATION_AT_RUNTIME` | with cert generation | the mDNS service mints a self-signed cert covering the address and `<host>.local` once the station gets an IP | cert generation, mDNS | one-shot |
| `ENABLE_SCRIPT_RUNNER` | yes | `source`, control flow and `/etc/rc.local` | cmd + storage | low |
| `ENABLE_CONTEXTUAL_EXECUTION` | off | cooperative and preemptive lanes | the threading interfaces | high (per-task stacks) |

### 3.3.2 Behaviour flags

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

### 3.3.3 Per-device limits

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
| `TCP_UNACKED_MAX` | bytes a TCP or TLS connection may have sent and not yet had acknowledged before a write waits | the network stack's send buffer, `TCP_SND_BUF`; lower to spare RAM |
| `TCP_WRITE_CHUNK_MAX` | most bytes one write hands a connection | one TCP segment: 1436 where the network stack's MSS is 1436, 536 where it may be 536 |
| `SSH_SFTP_READ_MAX` | most file bytes one SFTP read reply carries | 2 KB, 16 KB where RAM allows |
| `SSH_SFTP_READ_MIN` | fewest file bytes a read reply is cut to when the heap is short | 256 |
| `SSH_RX_PACKET_MAX` | most channel data one packet from an SSH client may carry, told to the client when a channel opens; every packet is held whole and its MAC checked | 4 KB (the least common clients send in), 16 KB where RAM allows, 32 KB on a host |
| `SSH_TX_QUEUE_MAX` | encrypted output one session holds for its connection before whatever writes it waits | 8 KB, 4 KB where RAM is tight, 32 KB where RAM allows |
| `SSH_RX_STALL_MS`, `SSH_TX_STALL_MS` | how long a packet still arriving, or output the connection cannot take, is waited for before the session is closed with its reason logged | 30 s each |
| `HTTP_UPLOAD_WRITE_BLOCK_SIZE`, `HTTP_UPLOAD_WRITE_BLOCK_MIN` | upload bytes gathered before one write to the staging file, and the smallest gathering buffer taken when the heap is short | 2048, 256 |
| `MQTT_PACKET_MAX` | largest packet the MQTT client builds for one publish | 1 KB, 8 KB where RAM allows |
| `MQTT_SEND_QUEUE_MAX` | most packet bytes waiting in the MQTT send queue at once; past it a publish is refused, nothing queued is dropped | 2 KB, 16 KB where RAM allows |
| `MQTT_INFLIGHT_MAX` | qos 1 publishes sent and not yet acknowledged before the client waits for an answer | 4, 8 where RAM allows |

## 3.4 The shape of a service config

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

The struct carries no size constant of its own. A table declares `sizeof(Table)` when it registers, and the database engine reserves the slot from that — framing, growth room and the seal a credential table needs are all its business, not the config header's. [§5](database.md#5-database-layer) covers how a record is laid out and how the total is checked against the medium at build time.

## 3.5 What each config file carries

| File | Persisted table | Headline knobs |
|---|---|---|
| Common | — | `USER`, `PASSPHRASE`, task count, time durations, HTTP request budget |
| GlobalConfig | `global_config_table` | config version, firmware version, release, launch time |
| WifiConfig | `wifi_config_table` | buffer sizes, default IPs, modification policy |
| ServerConfig | `login_credential_table` | session name, cookie max age, web session slots and token size, absolute session cap, login attempt limit and lockout |
| HttpConfig | — | client buffer size, request limits, HTTPS port, HSTS age |
| TlsConfig | — | per-session record buffers, TLS task stack and poll interval, default cert/key paths |
| OtaConfig | `ota_config_table` | update server url, version, check cadence, local image extension and magic byte |
| MqttConfig | general / LWT / pub-sub tables | broker, last will, publish and subscribe slots, client tick interval and timeouts in milliseconds |
| GpioConfig | `gpio_config_table` | pin map, modes, event conditions and channels |
| EmailConfig | `email_config_table` | SMTP host, port, auth, default subject |
| DeviceIotConfig | `device_iot_config_table` | config and OTP URLs, channel keys, device key size, sampling bounds |
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

## 3.6 What depends on what

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

## 3.7 Common build shapes

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

## 3.8 Defaults are not current values

Editing `DEFAULT_SSID` does not change the SSID a running device uses. Defaults are consulted exactly twice: on first boot, when NVM is empty or the checksum fails, and on factory reset when the framework is configured to write defaults back rather than zeroing.

To change a live value, go through the portal, the CLI (`net connsta`, `iot sethost`, and so on), or the database service directly and persist the table. Defaults govern the fallback state; the database holds the current one.

## 3.9 Conventions worth keeping

Branch on `ENABLE_*` with `#ifdef`, never with a runtime `if` — the unreached branch still needs symbols the link cannot provide. Keep config structs POD and fixed-size; a pointer or a `pdiutil::string` inside one cannot be serialised to NVM. Never include one service config from another; if two need the same value, it belongs in `Common.h`. And gate code on the capability (`ENABLE_WIFI_SERVICE`) rather than on the board (`DEVICE_<BOARD>`) — boards come and go, the capability is what the code actually depends on.

## 3.10 The `/etc` config surface

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

### Which features have a settings file, and turning one on

Four features keep their settings in `/etc`:

| Feature | File | Gate | Carries |
|---|---|---|---|
| WiFi | `/etc/wifi/wifi.conf` | `ENABLE_WIFI_CONFIG_FILE` | station and AP credentials, addresses, and the `sta_enable` / `ap_enable` gates |
| MQTT | `/etc/mqtt/mqtt.conf` | `ENABLE_MQTT_CONFIG_FILE` | broker host, port, whether the broker is reached over TLS, client id, credentials, keepalive, clean session, and the last will |
| OTA | `/etc/ota/ota.conf` | `ENABLE_OTA_CONFIG_FILE` | update server url, as `http://host:port` |
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

### Runtime service enable

Which services run is a runtime decision, and it lives in **one** file rather than in each feature's:

```
# /etc/service.conf — one line per service
mqtt no
email yes
```

The key is the service's own name, lowercased. That keeps the whole enable surface to a single filesystem entry however many services a build has, and leaves a feature's own conf holding only its settings.

Not everything under `/etc` is a config file. `/etc/passwd` and `/etc/shadow` are the user store's, `/etc/ssh` holds host keys beside `ssh.conf`, `/etc/hosts` is the resolver's, and `/etc/rc.local` is a script rather than a table of keys — it is read line by line and run, not parsed for settings ([§7.15](shell.md#715-shell-scripts-and-etcrclocal)).

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

### Two views of one setting

A conf file and the web portal are two windows onto the same value, so both are held to the same rule. Each of the four settings files above is `0600 root:root`, and the portal pages that change one require a root session — a non-root login is answered with a "root privilege required" page instead. Editing `/etc/mqtt/mqtt.conf` and saving the MQTT form are the same privilege; see [8.5](web-server.md#85-middleware).

Saving from the portal writes the record store *and* the file, and a service reads the file back at init, so an edit made either way survives a restart of that service.
