# 5. Database Layer

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

## 5.1 Mental model

A table is identified by an **id that never changes and is never reused**. Nothing declares a byte address. The engine lays the records out itself:

```
  0        5            17                        212                        end
  ├────────┼────────────┼─────────────────────────┼──────── ... ──────┬───────┤
  │reserved│ superblock │ directory[MAX_TABLES]   │ records, packed   │ free  │
  └────────┴────────────┴─────────────────────────┴──────── ... ──────┴───────┘
```

The superblock carries a magic, the format revision, the firmware version, the launch year and a checksum. The directory is fixed at `MAX_DB_TABLES` entries — so adding a table never shifts a record — and each entry holds the table id, the version that wrote it, the offset, the slot capacity, the bytes actually written, a checksum and a flag byte.

Because placement is derived, a struct that grows does not push a neighbour into it. On the next boot the engine notices the geometry no longer matches, repacks by id, and carries every payload across.

## 5.2 Two tiers

A device with storage runs on a container file and keeps the eeprom as the defaults behind it:

| tier | medium | role |
|---|---|---|
| live | `/etc/database/pdi.db`, `0600 root` in a `0700 root` directory | everything the device is running |
| defaults | eeprom | what it falls back to when the container is wiped or reset |

`resolve_tiers()` prefers the container and falls back to running directly on the eeprom when there is no storage service or the container cannot be opened. A container that never existed, or one that was wiped, is rebuilt from the eeprom defaults on the next boot.

The eeprom is opened only for the length of a defaults copy, so on a device running the container tier it holds no RAM at all — a few KB that stays available to everything else.

A single container file is used rather than one file per table: LittleFS runs a 4096-byte block and a 64-byte inline limit, so most tables would each claim a whole block. One container costs one block, and every write is an in-place `editFile()` at its offset, which keeps the mode and owner the file was created with.

## 5.3 The ports

`iDbStoreInterface` is a flat byte device — `read`, `write`, `flush`, `capacity`, plus `init`/`deinit` for opening and releasing the medium. Two implementations ship: `EepromDbStore`, which compares bytes before staging them so an unchanged record never dirties the medium, and `FsDbStore`, which keeps the container at its full length so a read never runs past the end of the file.

Underneath the eeprom store, `iDatabaseInterface` is what a board port provides: `readByte`, `writeByte`, `commitConfigs`, `beginConfigs`, `endConfigs` and the region size. The singleton is `__i_db`.

## 5.4 The engine

Tables register themselves before `main()` runs. Each generated table inherits an abstract layer whose constructor pushes the instance into a static array, so by the time the database service starts it already knows every table in this build.

```cpp
class WiFiTable : public DatabaseTable<WIFI_TABLE_ID, wifi_config_table, 1, true> {};
extern WiFiTable __wifi_table;
```

The template arguments are the id, the backing struct, the struct's layout version and whether the record holds a credential. The template supplies `boot()` — which registers id, size, version and secrecy — plus typed `get`, `set` and `clear` that go through `DbLayout`.

`__database` is the registry and enforces two rules: a table must carry a non-zero id nobody else has taken, and the count must stay within `MAX_DB_TABLES`. `DbLayout` then decides whether they fit the medium. A table that fails either shows up as a `SysLogE` at boot and a table that refuses to persist.

A table registering itself a second time keeps its entry rather than colliding with it, so a service that initialises more than once — a restart, a factory reset — comes back with its tables intact. Only a *different* table reaching for an id already held is a conflict.

Whether the records fit at all is settled at build time — `src/database/core/DatabaseLayout.h` sums every record, seals included, and fails the build against both the eeprom and the container.

## 5.5 The service

`DatabaseServiceProvider` is what the rest of the framework calls. It defines the table globals, each behind the matching `ENABLE_*` flag; it resolves the tiers at boot; and it exposes one typed `get_*_table` / `set_*_table` pair per persisted struct, plus `save_defaults()` and `restore_defaults()`.

Services go through those accessors rather than touching a store or a table global directly. That keeps the dependency one-way: a service depends on the database service, never on the port or the generated layout.

## 5.6 Where the tables come from

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

## 5.7 Read and write semantics

Reads are whole-struct — put a default-constructed `T` on the stack, read into it, change what you need, write it back. Passing a default-constructed struct matters: a record written by a shorter, older version of the struct only covers the bytes it had, and everything past that keeps the default it came in with.

That is the growth rule, and it is the reason **new fields go at the end of a struct**. An appended field needs no migration code at all. Anything else — a field changing meaning or moving — bumps the table's version and overrides `migrate()`. A struct that *shrank* returns the record to its defaults, since the stored bytes no longer describe it.

Writes land immediately; there is no journal. Structs are `memcpy`'d raw, so an image belongs to the toolchain that wrote it. The engine is single-threaded; if contextual execution is on and two lanes touch the same table, guard it with a mutex.

## 5.8 Sealed records

Sealing is a device capability. `ENABLE_DB_SEALING` resolves against `DEVICE_SUPPORTS_DB_SEALING`, so a board that cannot spare the RAM the ciphers need — the AES key schedule and the HMAC state come to roughly 600 bytes of stack — builds without them, and every record carries a checksum instead. On a board that declares it, a table marked secret has its record sealed on the way to the medium: a fresh 16-byte nonce, AES-128-CTR ciphertext, then a truncated HMAC-SHA256 tag over the nonce and the ciphertext. The tag is checked before anything is decrypted, and before anything reaches the caller's buffer, so a tampered record cannot land on top of the defaults it is holding.

The key is 16 bytes at the top of the eeprom, above the capacity the store reports, created from the hardware RNG the first time a device boots. Nothing in the shell or over SFTP can reach the eeprom, so copying `pdi.db` off the device yields ciphertext for every credential. This does not defend against someone dumping the flash itself.

Plain records carry a checksum instead — enough to catch a rotted byte, and there is no point paying for a tag on a record with nothing to hide.

Nothing in the read or write path allocates, and the whole engine costs one 75-byte object plus at most 70 bytes of stack, measured on the tightest port. A repack moves only the records whose offset actually changes, so appending a table or growing the last one asks for no memory at all.

## 5.9 Factory reset and defaults

`db save` takes what the device is running now as the defaults the eeprom holds, which is how a provisioned device is made to come back to its provisioned state rather than a blank one. A factory reset restores those defaults; on a device with no storage tier it clears every record, which reads back as the struct defaults.

Two switches shape the behaviour. `AUTO_FACTORY_RESET_ON_INVALID_CONFIGS` runs a five-second check and fires the reset when the database will not mount. `CONFIG_CLEAR_TO_DEFAULT_ON_FACTORY_RESET` hooks the reset event to put the defaults back. The reset event is public, so any service can hook it to drop its own caches.

## 5.10 From the terminal

```
  db status     which medium is live, how much of it the records take
  db list       one line per record, sealed ones marked
  db verify     read every record past its checksum or tag
  db save       take what is running now as the defaults
  db restore    put those defaults back in use (root only)
```

`db` never prints a record's contents, so a sealed credential stays sealed. `db restore` also rewrites each feature's `/etc` config file ([§3.10](configuration.md#310-the-etc-config-surface)) from its restored record, so the file cannot put the old values straight back.

## 5.11 Adding a table

Say you want a metrics service to persist a host and a port.

1. Define the struct in `src/config/MetricsConfig.h` following the [§3.4](configuration.md#34-the-shape-of-a-service-config) shape — POD, fixed-size members, a `clear()` the constructor calls:
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
