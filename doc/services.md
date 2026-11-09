# 6. Service Providers

A service provider is the framework's unit of feature. Each one lives under [src/service_provider/](../src/service_provider/), derives from `ServiceProvider`, and is gated by exactly one `ENABLE_*` flag. This section is the per-service reference.

## 6.1 The common shape

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

A task registered through these runs as the system: no session is current while it runs, so it reads and writes as root rather than as whoever typed last on a terminal, and the session that was current is put back afterwards. A task registered straight on the scheduler, as a command does for work it runs in the background, keeps whatever session is current — a command that must act as its user sets that itself.

The tracked list is fixed-size — six slots by default. `stopService` in the base reaps every one of them, so an override is only needed when you also have sockets or buffers to release; call the base at the end when you do write one. If you ever register a task straight on the scheduler, hand its id to `trackServiceTask(id)` so it is not orphaned on stop.

Persisted configuration always goes through the database service accessors, never `__i_db`. Cross-service reactions go through `__utl_event`, never direct calls — that is what keeps the dependency graph one-directional.

The base also offers `signalAllServiceTasks(sig)`, `countServiceTasks(...)` and the task-id iterators; those are what the `service` command renders.

A new service is runtime-toggleable for free. The base reads `enabled` from `/etc/x/x.conf` before anything starts, and `startService` itself refuses when the service is disabled, so no caller has to remember the check. Override `getServiceConfigPath` only if the feature genuinely cannot use the derived path. Override `isEssentialService()` to return true only if the device genuinely cannot be recovered without the service; that puts it beyond the reach of `service disable` *and* of a hand-edited conf file.

## 6.2 Service reference

Ordered the way the orchestrator starts them.

### 6.2.1 `TimeServiceProvider` — `__time_service`

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

### 6.2.2 `DatabaseServiceProvider` — `__database_service`

Always present; everything else may need persisted config. It mounts NVM, boots every registered table, optionally polls config validity every five seconds, and listens for the factory-reset event. Fully covered in [§5](database.md#5-database-layer).

### 6.2.3 `DeviceFactoryReset` — `__factory_reset`

Always present. Polls the flash button for a six-to-seven second hold and fires `EVENT_FACTORY_RESET`. Other services listen for that event to drop their own caches before the reboot. The web portal exposes the same action as a form.

### 6.2.4 `SerialServiceProvider` — `__serial_service`

Opens the serial port at the configured baud and hooks its input handler into the device event pump. It supplies the default terminal for the CLI, and exposes the JSON apply/append hooks that Device-IoT uses to move sensor payloads across the serial link.

**It also finds the devices on its interfaces and binds drivers to them.** Every serial interface a port makes — UART, I2C, SPI, I2S, a USB host port — is an `iSerialInterface`, so a bus is a stream the shell can use like any other as well as something devices sit on. Every `DEVICE_SCAN_INTERVAL_MS` (2 s) the service looks on each interface the port made:

| Interface | How a device is found |
|---|---|
| I2C | only the addresses a compiled-in driver answers to are probed, never the whole bus, and only while a line in `/etc/devices.conf` uses the bus |
| USB host | the port reports what is attached, with its vendor, product and class |
| SPI, I2S, UART | nothing to detect: a device there is declared in `/etc/devices.conf` |

A device that turns up is offered to each driver that could drive it, and the driver **confirms the chip** — an id register, a JEDEC id — before it takes it, since two different chips can answer at one I2C address. A device no driver takes is still listed, without a driver. A device that goes away is let go of the same way. Each one raises `EVENT_DEVICE_ADDED` or `EVENT_DEVICE_REMOVED` on the event bus and writes one line to the system log.

A scan never holds the board up for long. A declared device whose probe takes longer than `DEVICE_SCAN_PROBE_MAX_MS` (100 ms) is left out of every later scan, bound or not; the answer that slow probe gave still counts, so a device it found gone is let go and one it found is kept. The system log says so once, `DEV: i2c0 0-0050 probe took 1080 ms, left to udevadm trigger`, and `udevadm info` shows `manual` after its state. `udevadm trigger` probes it again and puts it back in the scan when the probe is quick. A device left out that is unplugged is noticed on its next read or write. Where the port drives I2C in software, a bus whose lines do not both rest high, with nothing pulling them up, is not clocked by a probe at all, so an unplugged chip costs nothing; where the bus is a hardware peripheral, a probe is bounded by `I2C_BUS_TIMEOUT_MS` (50 ms).

`/etc/devices.conf` declares what cannot be detected, one device per line, `#` for comments:

```
# interface  target   driver    settings
i2c0         bus      i2c       sda=25 scl=15 clock=400000
i2c0         0x76     bme280
i2c0         0x50     at24      chip=24c128 size=16384 page=64
spi0         bus      spi       sck=6 miso=1 mosi=7 clock=8000000
spi0         cs=10    mmc_spi
i2s0         rx       inmp441   sck=26 ws=27 sd=39 rate=16000
```

The target is a number (`0x` for hex) on I2C, USB and UART, `cs=<pin>` on SPI, `rx` or `tx` on I2S, and `bus` on a line that sets up the bus itself rather than declaring a device — its driver is the bus's own name. After the driver come up to `DRIVER_PARAMS_MAX` (8) `key=value` settings, which the driver reads for what it cannot find out for itself — the pins a bus is wired to, a sample rate, the size of a memory. The settings a driver reads are whole numbers in decimal or `0x` hex; one it does not read, such as `chip=`, stays in the file as a label. A line that does not read is logged and skipped, and the rest still load. The file is taken in at boot and again whenever its size or modification time changes, judged without reading it; `udevadm trigger` reads it whatever it looks like, for an edit of the same length on a board whose clock is not set. A declared device that is not there is listed as `absent`, and one whose named driver will not take it as `failed`.

**Pins.** A bus whose pins the framework hands out — I2C, I2S and SPI — runs on the pins its line names, or on the board's default pins ([Supported Boards](../README.md#supported-boards)) when it names none. Those pins are **held** for as long as a line in `/etc/devices.conf` uses the bus, whether or not the device is there or bound: the GPIO service, `/sys/class/gpio` and the portal's GPIO pages leave a held pin alone and drive it again once the line is gone, so the driver always wins over a GPIO setting. Without a line, a default pin is an ordinary GPIO and the bus is never started. Each kind of bus takes its pins in its own way:

| Bus | Line | Pins it holds |
|---|---|---|
| I2S | `i2s0 rx inmp441 sck=26 ws=27 sd=39` | the three it names |
| I2S | `i2s0 rx inmp441` | the board's default SCK, WS, SD |
| I2C | `i2c0 bus i2c sda=25 scl=15 [clock=<hz>]` | the two it names, for every device on `i2c0` |
| I2C | `i2c0 0x50 at24 …` with no bus line | the board's default SDA, SCL |
| SPI | `spi0 bus spi sck=6 miso=1 mosi=7 [clock=<hz>]` | the three it names, for every device on `spi0` |
| SPI | `spi0 cs=10 mmc_spi` | the bus's pins and its own chip select |

A bus line names all of its pins or none, and `clock` is optional: `I2C_BUS_CLOCK_HZ` (100 kHz) for I2C, and for SPI the top clock `SPI_BUS_CLOCK_HZ` (20 MHz), below which each device picks its own. A bus line alone is enough to have an I2C bus looked on for chips that can be detected. A line whose own pins repeat — an SPI chip select on one of the bus's pins — is refused with `PDI_ERR_BUSY`. A board whose gpio matrix reaches any pin takes any pin that can drive a line; a board that drives the bus in software takes any GPIO but the ones kept for flash and console. Bus lines are taken first, then device lines in file order, and a line whose pins a line on another bus already holds is refused. A device line refused that way is listed `failed` with `PDI_ERR_BUSY` and gets its turn again once the pins are free; a bus line that is refused leaves its bus on the default pins, logged as `DEV: <bus> bus refused: <code>`:

| Error | Why a bus line was refused |
|---|---|
| `PDI_ERR_INVALID_ARG` | it names some of its pins without the rest, the same pin twice, or a pin that cannot carry its line |
| `PDI_ERR_NOT_FOUND` | the board has no such interface |
| `PDI_ERR_NOT_SUPPORTED` | the bus takes no bus line, or the board's pins for it are fixed in hardware |
| `PDI_ERR_BUSY` | a line on another bus already holds one of its pins |
| `PDI_ERR_EXISTS` | an earlier bus line already set up the same bus |

**Sound.** An I2S microphone is declared with its three pins, or none for the board's defaults: `sck` to the mic's SCK, `ws` to WS, `sd` to SD, with L/R tied to ground so it sends on the left channel. `rate` is optional, `SOUND_DEFAULT_RATE` (16000) without it. The `inmp441` driver starts the bus and listens for up to `SOUND_PROBE_WAIT_MS` (300 ms), long enough for the mic to wake, and takes the device only when the samples change — a data pin with nothing on it reads flat, and is listed `failed` with the bus handed back. A bound mic appears as `/dev/snd/pcmC0D0c`, 16 bit mono samples, and `arecord -d <seconds> <file>` records it into a WAV file ([§7.8](shell.md#78-built-in-command-inventory)). The I2S bus is behind `ENABLE_I2S_BUS`, on where the port declares `DEVICE_SUPPORTS_I2S`.

A declared device its driver refuses is listed `failed`, and the system log names the driver's reason as a `DEV: ... refused by <driver>: <code>` line. The same codes come back from `arecord` as `capture device failed : <code>`:

| Error | What failed |
|---|---|
| `PDI_ERR_INVALID_ARG` | the declaration names some of the three pins but not all, or the port cannot use one |
| `PDI_ERR_NO_MEM` | no memory left to start the bus |
| `PDI_ERR_NOT_FOUND` | the bus runs but the data pin reads flat — nothing is sending on it |
| `PDI_ERR_NOT_SUPPORTED` | the bus cannot run in the direction asked for |
| `PDI_ERR_NOT_INITIALIZED` | the bus was read before it was started |
| `PDI_ERR_BUSY` | a pin is held by a line on another bus or taken by another peripheral, or no I2S controller is free |
| `PDI_ERR_IO`, `PDI_ERR_STATE` | the port's I2S driver failed, or was in the wrong state for the call |

**Memory.** An I2C eeprom of the 24c family is declared with the `at24` driver and two settings, both required:

| Setting | Meaning |
|---|---|
| `size` | bytes the chip holds, a power of two up to 512 KB |
| `page` | bytes the chip stores in one write, at most `I2C_TRANSFER_MAX` (64) and no more than `size` |
| `chip` | optional, the chip's name as a label — `24c02`, `24c128`, … |

An eeprom carries no id to read back, so it is bound by declaration only: the driver takes it when it answers at its address and gives back its first byte. A chip of up to 2 KB takes a one byte word address and the bits above it in its bus address, so a 512 byte chip at `0x50` also answers at `0x51`; a larger chip takes a two byte word address. A bound eeprom has an `eeprom` file in its device directory under `/sys/bus/i2c`, `0600`, the size of the chip. Reading it gives the memory from the offset asked for; writing it stores from the start, and an edit at an offset stores from there. Writes go a page at a time, never across a page edge, and each waits up to `AT24_WRITE_TIMEOUT_MS` (25 ms) for the chip to finish storing before the next. The file is read and written `I2C_TRANSFER_MAX` (64) bytes at a time with the system given its turn between each, so reading or writing the whole chip never holds the processor long enough to trip a watchdog. While a read or write is under way the driver counts as busy: a scan does not take the chip's silence for it going away, a change to `/etc/devices.conf` is taken in once it ends, and a stop of the serial service waits up to `DEVICE_BUSY_WAIT_MS` (1 s) for it. A chip taken away part way ends the read or write with its code. The I2C bus is behind `ENABLE_I2C_BUS`, on where the port declares `DEVICE_SUPPORTS_I2C`, and runs at `I2C_BUS_CLOCK_HZ` (100 kHz).

Wire the chip's SDA and SCL to the bus's pins — the bus line's, or the board's defaults. The bus needs pull-ups on both lines, which most eeprom modules carry.

A declared eeprom its driver refuses is listed `failed` with the same `DEV: ... refused by at24: <code>` line, and a read or write of its `eeprom` file fails with one of the same codes:

| Error | What failed |
|---|---|
| `PDI_ERR_INVALID_ARG` | `size` or `page` is missing from the declaration |
| `PDI_ERR_NULL_PTR` | a read or write was given no buffer |
| `PDI_ERR_TIMEOUT` | the chip did not answer again after storing a page — it took too long, or was taken away while storing — or the bus timed out |
| `PDI_ERR_NOT_FOUND` | nothing answers at the address — the chip is not wired, not powered or was taken away |
| `PDI_ERR_NOT_SUPPORTED` | the bus is not I2C, or a write tried to append to the `eeprom` file |
| `PDI_ERR_BUSY` | a bus line is held low, or a line on another bus holds the bus's pins |
| `PDI_ERR_NO_SPACE` | a write starts at or past the end of the chip |
| `PDI_ERR_IO` | the chip answered its address but refused the data |
| `PDI_ERR_RANGE` | `size` or `page` is one no eeprom has |

**Disks.** An SD card on the SPI bus is declared by the chip select it is wired to, `spi0 cs=10 mmc_spi`, with the card's CS on that pin and its CK, DI and DO on the bus's SCK, MOSI and MISO. A card carries no id the bus could find on its own, so it is bound by declaration only. The `mmc_spi` driver speaks the card's SPI mode itself: it wakes the card with its select high at `SD_INIT_CLOCK_HZ` (400 kHz), resets it, tells an older card from a newer one, turns the card's CRC checking on the way linux does so every command and block carries a checked CRC, tells a standard capacity card from a high capacity one once the card reports its power-up done (a card still powering up is asked again, up to `SD_READY_TIMEOUT_MS`), reads its size — a size register that does not fit the card's capacity class is refused, not bound, and offered again on the next scan — and from then on runs it at `SD_CLOCK_HZ`, held to the bus's top clock: 20 MHz by default, lowered by a port whose hardware bus does not read a card reader on jumper wires cleanly that fast — down to 4 MHz on one that gave corrupt blocks at 8 MHz. A card wired for a faster or slower clock sets it with the bus line's `clock=`. A bound card is a disk:

| Where | What |
|---|---|
| `/dev/mmcblk0` | the whole card, `0600`, the size of the card; read from any offset, written at any offset, a block it changes in part read first so the rest is kept |
| `/dev/mmcblk0p1` … | each partition of a card that carries a dos partition table, numbered by its slot in the table as linux numbers them, so deleting one leaves the others their numbers; `0600`, read and written the same way and never past the partition's end |
| `/sys/block/mmcblk0/size` | the card's size in 512 byte sectors |
| `/sys/block/mmcblk0/mmcblk0p1/{start,size}` | where each partition starts and how long it is, in 512 byte sectors |
| `lsblk` | one row for the card and one per partition, with the name, the size in bytes, `disk` or `part`, and the filesystem |
| `blkid` | one line per node that carries something: `/dev/mmcblk0: PTTYPE="dos"` for a partition table, `/dev/mmcblk0p1: TYPE="vfat"` for a filesystem; `blkid <node>` names only that one |

What a card carries is read from its first blocks, the way linux blkid reads it: a partition table, or a filesystem across the whole card, and the filesystem in each partition. A filesystem is named by its signature — `vfat`, `exfat`, `ntfs`, `ext2`, `ext3`, `ext4`, `littlefs` — whether or not the framework can mount it, so a card in a format it cannot read is still named for what it is. A gpt table is named `gpt`, with no partitions listed. A first block that ends in the boot signature is a dos table, even with no partitions in it, the way linux reads an empty label; a FAT filesystem needs that signature as well as its own name. What was read is kept until a write reaches the card, then read again; a first block that reads as all zeros is read again on every look. A card that is bound logs what it carries, `DEV: mmcblk0: dos p1 p2`, or `DEV: mmcblk0: no partition table`.

**Mounting a card.** Root mounts the filesystem a card or one of its partitions carries on a directory that already exists, the way linux does: `mkdir /mnt` then `mount /dev/mmcblk0p1 /mnt`. Which filesystem it is comes from the card, as `blkid` names it: a littlefs volume with `ENABLE_LFS_DISK`, and a FAT volume with `ENABLE_FAT_FS`. Files under `/mnt` are then the card's files, read and written by every file command; `mount` lists it as `vfat` or `littlefs`, and `umount /mnt` detaches it once no file on it is open.

FAT keeps no owner or mode, so every entry on a FAT volume shows root's `0755`, less the write bits when the entry is read-only; `chmod` sets or clears read-only and `chown` is refused. Each file keeps the time it was last written, to two seconds, as the local wall time the way FAT has always kept it, so a computer the card is moved to shows the same time the board does. A littlefs volume keeps the owner, mode and times of every entry the way the root filesystem does, so `chmod`, `chown` and `ls` work on it as they do on `/`. It is read and written a sector at a time in 4 KB blocks and leaves wear levelling to the card, and it takes its caches, about 1.5 KB, from the heap only while it is mounted. Up to `FAT_VOLUMES_MAX` (2) FAT volumes and `LFS_DISK_VOLUMES_MAX` (1) littlefs volume are mounted at once, each in a mount slot of its own. A mount that fails prints `mount: <device> : <code>`:

| Error | Meaning |
|---|---|
| `PDI_ERR_INVALID_ARG` | the directory's path is longer than `VFS_MOUNT_PREFIX_MAX` (15) |
| `PDI_ERR_NOT_FOUND` | the device is not a disk or partition node |
| `PDI_ERR_NOT_SUPPORTED` | the device is a whole disk that carries a partition table (mount the partition, e.g. `/dev/mmcblk0p1`), or carries a filesystem `blkid` names that is neither FAT nor littlefs |
| `PDI_ERR_BUSY` | the device is mounted already |
| `PDI_ERR_PERM` | not root |
| `PDI_ERR_EXISTS` | something is mounted on the directory already |
| `PDI_ERR_FULL` | `FAT_VOLUMES_MAX` FAT volumes, or `LFS_DISK_VOLUMES_MAX` littlefs volumes, are mounted already |
| `PDI_ERR_NO_MEM` | the heap could not hold a littlefs volume |
| `STORAGE_ERROR_NOT_A_DIRECTORY` | the directory does not exist |
| `PDI_ERR_FROM_FAT(<result>)` | FatFs refused it, its own `FRESULT` kept in the `PDI_ERRBASE_FS_FATFS` band; `PDI_ERR_FROM_FAT(FR_NO_FILESYSTEM)` is a device with no FAT filesystem on it |
| `PDI_ERR_FROM_LFS(<code>)` | littlefs refused it, its own error kept in the `PDI_ERRBASE_FS_LITTLEFS` band; `PDI_ERR_FROM_LFS(LFS_ERR_CORRUPT)` is a littlefs volume that cannot be read |

A read or write on a mounted card that the card itself fails gives the card's code, `PDI_ERR_NOT_FOUND` for a card that was taken out. A card bound again while it is mounted, by a changed `/etc/devices.conf` or a card swapped in, may not be the card the mount read its tables from, so every read and write of that mount answers `PDI_ERR_NOT_FOUND` without touching the card until it is unmounted and mounted again.

**Preparing a card.** Three commands clear a card, partition it and make a filesystem on it, the way their linux namesakes do. Each is for root only, anyone else being refused with `PDI_ERR_PERM`, and each refuses a card a volume is mounted from with `PDI_ERR_BUSY`. A failure prints `<command>: <device> : <code>`.

- `wipefs /dev/mmcblk0` lists the signatures a disk or partition carries, one `DEVICE OFFSET TYPE` row each, and changes nothing. `wipefs -a /dev/mmcblk0` erases them, saying what each held: `/dev/mmcblk0: 8 bytes were erased at offset 0x00000052 (vfat): 46 41 54 33 32 20 20 20`. A signature is the name a filesystem keeps in its first blocks — `vfat`, `exfat`, `ntfs`, `ext2` to `ext4`, and both copies of a littlefs superblock — and the boot signature a `dos` or `gpt` table ends with. Only those bytes go, as on linux, so the card then reads as blank, with no table and no filesystem, and nothing else on it is touched.
- `sfdisk /dev/mmcblk0` writes a dos partition table from a script on its input: `echo ",,c" | sfdisk /dev/mmcblk0` makes one FAT32 partition across the card, and `sfdisk /dev/mmcblk0 < /layout.txt` takes one from a file. Each line is `start, size, type, bootable`, separated by commas or by spaces, and any field left empty takes its default: `start` the first 1 MiB boundary after the partition before; `size` the rest of the card, as does `+`, in sectors or with `K`, `M`, `G` or `T` (`KiB` and so on too); `type` a hex id such as `c` or `83`, or `L`, `S`, `E` or `U`, `83` when empty; `bootable` is `*`. Blank lines and lines starting with `#` are skipped. Nothing is written until every line has been taken, so a wrong line leaves the card as it was and prints `sfdisk: line <n> : <code>`: `PDI_ERR_RANGE` for a partition outside the card or over the one before, `PDI_ERR_INVALID_ARG` for a field it cannot read, `PDI_ERR_FULL` for a fifth partition. A card that has a dos table already keeps its disk identifier and boot code; one that has none gets a new random identifier. A partition node is refused with `PDI_ERR_INVALID_ARG`, and a script with no partitions in it with `sfdisk: no partitions given`.
- `sfdisk -l` lists every disk, or `sfdisk -l /dev/mmcblk0` one, the way `fdisk -l` does: the size, the label type and identifier, and one row per partition with its start, end, sectors, size, id and type name. `sfdisk --delete /dev/mmcblk0 2` empties slot 2, leaving the other partitions their numbers, and `sfdisk --delete /dev/mmcblk0` empties every slot, leaving an empty dos label; a slot that is not in use is `PDI_ERR_NOT_FOUND`.
- `mkfs -t vfat /dev/mmcblk0p1` makes a FAT filesystem with two FATs the way `mkfs.vfat` does: `FAT32` from 512 MiB up, and below that `FAT12` or `FAT16` as the size calls for; `mkfs -t littlefs /dev/mmcblk0p1` makes an empty littlefs volume. It says `this may take some time` before it starts. A whole card that carries a partition table is refused with `PDI_ERR_EXISTS`, as `mkfs.vfat` refuses one without `-I`, so `wipefs -a` clears it first. Neither changes the partition's type id, as on linux. Another type is `PDI_ERR_NOT_SUPPORTED`, and what the filesystem library refuses comes back in its own band.

sfdisk writes dos tables only, with up to four primary partitions; gpt tables, extended and logical partitions, and sfdisk's own `-d` dump format are not supported.

**Mounting at boot.** `/etc/fstab` lists what to mount, one line per filesystem in the linux layout, `<device> <dir> <type> <options> <dump> <pass>`: `/dev/mmcblk0p1 /mnt vfat defaults,nofail 0 0`. The device is a node under `/dev`; `UUID=` and `LABEL=` are not supported and give `PDI_ERR_NOT_FOUND`. The type is `vfat`, `littlefs` or `auto`; a type that is not the one `blkid` names on the card gives `PDI_ERR_NOT_SUPPORTED`. Of the options, `noauto` leaves the line out and `nofail` keeps its failure quiet; any other option, and the dump and pass fields, are taken and ignored. Each time a disk is bound — at boot, and when a card is put in — every line whose device is not mounted on its directory yet is mounted, and a line that fails is logged as `DEV: <device> not mounted from fstab: <code>`. Root runs the same pass with `mount -a`, which prints `mount: <device> : <code>` for each line that fails. A disk that goes — the card taken out, its `devices.conf` line removed, or that line rewritten with a gap between writes — takes its mounts with it, as `umount -l` does on linux: the mount point leaves the table at once, a session working inside it is moved to `/`, and a file still open there fails its reads and writes until it is closed. When the disk is bound again, what `/etc/fstab` lists for it is mounted again; a mount made by hand is made again by hand.

The card is read and written in 512 byte blocks, several in one transfer when the filesystem asks for a run of them, with the system given its turn between blocks, so a long read or write never trips a watchdog. Writing the disk node replaces what is on the card, filesystem and all. One card is handled at a time, at `/dev/mmcblk0`. SPI has no line that says a card is in the slot, so each scan asks a bound card for its status, the way linux polls a card with no detect pin: a card that does not answer three times running is let go, with an `EVENT_DEVICE_REMOVED` and its `/dev` and `/sys/block` entries gone, and a read or write meanwhile fails with `PDI_ERR_NOT_FOUND`. While the slot is empty the card is listed `absent` and offered to `mmc_spi` again on each scan, its refusal logged once rather than every scan, so a card put back is bound on the next scan with no `udevadm trigger`. A slot that never answers the reset as an idle card holds no card, whatever its floating data line happens to read, and is listed `absent` too. A card that answers but is refused for another reason — a card half way in — is listed `failed` and offered again on each scan the same way, so it is bound once it answers properly. A driver that takes longer than `DEVICE_SCAN_PROBE_MAX_MS` to answer leaves the card out of the scan until `udevadm trigger`; writing the refusal to the log is not counted. A declared card its driver refuses, and a read or write of `/dev/mmcblk0` that fails, give one of these:

| Error | What failed |
|---|---|
| `PDI_ERR_INVALID_ARG` | the chip select is a pin that cannot drive a line |
| `PDI_ERR_TIMEOUT` | the card never left its idle state or never reported its power-up done within `SD_READY_TIMEOUT_MS` (1 s), sent no data within `SD_READ_TIMEOUT_MS` (100 ms), or was still storing a block after `SD_WRITE_TIMEOUT_MS` (500 ms) |
| `PDI_ERR_NOT_FOUND` | no card answers behind the chip select — not wired, no card in the slot, or taken out |
| `PDI_ERR_NOT_SUPPORTED` | the bus is not SPI, the card is an MMC, does not take the board's voltage or is over 2 TB, or a write tried to append to the disk |
| `PDI_ERR_BUSY` | the bus could not be started, or the chip select is one of the bus's pins or held by another line |
| `PDI_ERR_NO_SPACE` | a read or write past the end of the card |
| `PDI_ERR_IO` | the card refused a command, answered a read or write with an error, or gave a size register that does not fit its capacity class, which the system log gives as `SD: ...` |
| `PDI_ERR_CRC` | a block's CRC did not match, read from the card — the log gives both, `SD: block CRC from card …, computed …` — or as the card checked a write. A card that does not know how to turn CRC on is driven without checking its data CRC, and the log says so. |

Devices show up under `/sys/bus` ([§6.2.12](#6212-storage-interface-init-no-provider)) and through `udevadm` ([§7.8](shell.md#78-built-in-command-inventory)). The whole of this is behind `ENABLE_DRIVER_CORE`, on by default on a board whose port declares `DEVICE_SUPPORTS_DRIVER_CORE`, and switched off automatically on any other board or without the serial service; with it off nothing here is built.

### 6.2.5 `WiFiServiceProvider` — `__wifi_service`

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

### 6.2.6 `OtaServiceProvider` — `__ota_service`

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

Requests carry HTTP basic auth and a `pdistack` user agent, and run over TLS when the TLS service is on. With Device-IoT on, the auth is the device key ([§6.2.10](#6210-deviceiotserviceprovider--__device_iot_service)); without it, `ota:<device-mac>`. Where they go comes from the host URL alone — its scheme or an explicit `:port` picks the port. Which of the three upgrade strategies applies is a compile-time choice.

With no update server set, each check has nothing to ask. That is written to the error log once, and again only after a server has been set and taken away, so a board left unconfigured does not fill its log with the same line every interval.

A build can also be flashed without any update server. `collectLocalImages()` lists the firmware images sitting on the filesystem and `flashFromFile()` writes one of them, checking the image magic byte before it commits and reporting through the same `upgrade_status_t` as a server-driven update. That is what backs the **Flash From Storage** form on the OTA page: upload a binary through the storage browser, pick it from the list, confirm, and the device flashes and restarts. Both entry points need the storage service.

### 6.2.7 `GpioServiceProvider` — `__gpio_service`

Loads the GPIO table, ticks pin modes and values, and refreshes the table every five minutes. Modes are off, digital write, digital read, digital blink, analog write and analog read.

From the shell, GPIO is driven as files under `/sys` — `echo 1 > /sys/class/gpio/5/value`. The portal adds a GPIO page plus an events submenu where a pin condition can be routed to email or an HTTP endpoint.

With a GPIO server url set, the service posts its pins there on the configured frequency, authorised the way OTA is: the device key with Device-IoT on, `gpio:<device-mac>` without it.

### 6.2.8 `MqttServiceProvider` — `__mqtt_service`

Opens the broker connection with a last-will message, subscribes to whatever the pub/sub table lists, and schedules the publish driver. Applications inject and receive payloads by registering publish and subscribe callbacks. The portal has general, LWT and pub/sub forms.

### 6.2.9 `EmailServiceProvider` — `__email_service`

Loads the email table and, if periodic mail is enabled, schedules the send. The GPIO service uses it for event alerts. The portal page includes a test button.

A mail is sent over plain SMTP: the board connects, greets with the sending domain, logs in with `AUTH LOGIN` only when a username is set — so a relay that takes mail without a login works with the username left empty — then gives the sender, the recipient and the message. A mail is not attempted until the server, the domain, the sender and the recipient are all set. `sendMail(body, done)` returns at once — false when the mail is refused, as it is while another mail is still going out — and the mail then goes out over a connection of its own, one step per scheduler tick, so the board keeps answering while the server replies; each step waits up to 5 s for its reply, and `done(sent)` is called with whether the server took the mail. A mail that is not sent says why in the system log, naming the step the server refused and the code it answered: `Email: not sent, the server refused the recipient: 550`, `... the login: 535`, `Email: not sent, no ready answer from <host>:<port>` when no server answers ready, and `Email: not sent, the mail server, domain, sender or recipient is not set`.

### 6.2.10 `DeviceIotServiceProvider` — `__device_iot_service`

Off by default; uncomment `ENABLE_DEVICE_IOT` to build it. The service handles registration and channel setup, and the application supplies the sensor.

```
  portal: host + duid, Request OTP
   └─ GET <otp url>      →  { otp, status }         shown on the page; entering it on
                                                     the server registers this device
  config request (repeats until configured)
   ├─ GET <config url>   →  { did, channelhost, channelport, channelsecure,
   │                          channelread, channelwrite, datarate,
   │                          samplerate, keepalive, reconfig, … }
   ├─ configureMQTT()    →  writes those values into the MQTT tables
   └─ sample loop        →  sampleHook() × samplerate  ──▶ dataHook(payload) ──▶ publish
```

Both URLs are templates that substitute the MAC and the device unique id at request time.

**The device proves who it is with its own key.** The first OTP request generates a random 32-byte key, kept as hex in the IoT table, which is sealed. The OTP the server answers with is bound to that key: once the user enters it on the server, the server accepts that device by its key and by nothing else. Every request to the server then carries `key:<key>` as its basic auth — the OTP and config requests, the GPIO event posts and the OTA checks and downloads. Before a key exists the requests carry `mac:<device-mac>` instead, and without `ENABLE_DEVICE_IOT` the GPIO and OTA services keep their own credentials.

The key belongs to one server and one device unique id. Changing the host or the unique id — from the portal, `iot sethost` / `iot setid`, or by writing the table — drops it, and the next OTP request generates a new one, so the device is registered again. A factory reset drops it with the rest of the table.

The MQTT identity: the client id is a base64 of `mac:<device-mac>` rather than the raw MAC, the username is the device unique id, and the password is the device key. The connection runs over TLS when the config answers `channelsecure` true. The last-will topic is the read channel with a payload carrying the device id.

Because `configureMQTT()` writes the MQTT tables, Device-IoT's server-supplied broker settings replace whatever the portal had for MQTT when both are in use.

The application registers its sensor by calling `initDeviceIotSensor(...)` with an object implementing init, sample, data and reset hooks. The shell offers `iot setid`, `iot getid`, `iot sethost` and `iot gethost`. [§11.6](examples.md#116-deviceiotexample) walks through the example sketch.

### 6.2.11 `AuthServiceProvider` — `__auth_service`

A session-aware delegator rather than a store. `isAuthorized(user, pass)` verifies against `/etc/shadow` through the user store when that file exists, and against the bootstrap credential row otherwise. `setAuthorized` and `getUsername` read and write the *current* session, so there is no global "logged in" bit — three terminals can be at three different privilege levels at once.

### 6.2.12 Storage (interface init, no provider)

Storage has no service class. The filesystem is used directly by SSH and SFTP, the user store, every file command, and application code.

`__i_fs` is a dispatcher: an `iFileSystemInterface` that routes each call to a mounted backend by longest-prefix match.

```
  path ─▶ VfsDispatcher ─┬─ /       rootfs   LittleFS, the real flash filesystem
                         ├─ /proc   procfs   read-only, generated on each read
                         ├─ /sys    sysfs    peripherals as read/write files
                         ├─ /dev    devfs    byte-stream nodes
                         └─ /tmp    tmpfs    RAM-backed, lost on reboot
```

The flash filesystem moves busy blocks to fresh ones as they wear, and one write can set off a move that reads and rewrites the flash for seconds. While littlefs works the flash back to back for more than `LFS_BUSY_YIELD_MS` (50 ms), it gives the system its turn, so the watchdog is fed and the network served through the longest move; work shorter than that is never paused.

Mounting happens during `initialize()`, and the table is five slots by default — exactly what those five backends need. `mount` shows the table at runtime and `df` reports usage per mount. On a RAM-tight port, `/tmp` is the first thing to drop, since it holds file content in the heap.

Each prefix is named once, in [src/config/VfsConfig.h](../src/config/VfsConfig.h): `PROC_MOUNT_PREFIX`, `SYS_MOUNT_PREFIX`, `DEV_MOUNT_PREFIX` and `TMP_MOUNT_PREFIX` beside the `ENABLE_` flag for the backend that answers there, with the root at `FILE_SEPARATOR`. Code that reaches a synthetic node writes `PROC_MOUNT_PREFIX "/mounts"` rather than the path in full, so moving a mount is one edit.

**procfs** nodes are all `0444` and root-owned; writes fail, and a redirect into one says `cannot write <path>` rather than appearing to succeed. `/proc/uptime` gives seconds since boot in the Linux two-number layout, `/proc/version` gives the release and config version, `/proc/meminfo` the free heap, the largest block it can still hand out and how many allocations the heap check has refused since boot, `/proc/mounts` a line per mount, `/proc/stat` the scheduler's cumulative counters, and `/proc/cpuinfo` the processor in the Linux layout — a block per core with its number, model name and clock in MHz, then the cores, the program flash size and the platform beneath the stack, `unknown` for what the port cannot report. Every running task has a directory of its own — `/proc/<pid>/{stat,cmdline,status}` — and `/proc/net/{route,dev,tcp}` describes the network: the routes each interface offers, the traffic each has carried, and every TCP endpoint the stack holds, listening and connected alike. Everything that reads files works on them — `cat`, `head`, `wc`, `grep`, `hexdump`.

`/proc/net/route` carries one row per route rather than one per interface, in the `route -n` columns with a `Flags` field. An interface that holds a netmask contributes the network it reaches directly — destination `ip & netmask`, no gateway, flag `U` — and one that holds a gateway contributes the default route as well, `0.0.0.0/0.0.0.0` through it, flag `UG`. An access point therefore appears once, for its own subnet, and an interface that is up but has no address yet appears not at all, because it has nowhere to route.

`/proc/net/dev` and `/proc/net/tcp` each fill in only as far as the port can answer. Counters come from the link, and a link that cannot count is left out rather than listed with zeroes that would read as an idle interface — a prebuilt network stack often has its statistics module compiled out. Endpoints come from the stack, and a port that cannot enumerate one leaves the columns empty rather than reporting a device with nothing listening. Both are port capabilities, so a link or stack that can answer fills the node in without anything above changing.

Columns are fixed width rather than tab separated, here and in every command that prints a table: a tab only reaches the next eight-column stop, so one long field would push the rest of the row out of line on a terminal that does no reflowing of its own.

`ps`, `top`, `mount` and `df` read through these nodes rather than reaching into the scheduler or the mount table, so what a command shows and what a file says cannot disagree. `df` takes its list of mounts from `/proc/mounts` and asks the backend behind each prefix for its sizes.

**sysfs** carries GPIO, the network interfaces, and the devices the serial service found:

| Node | Mode | Content |
|---|---|---|
| `/sys/class/gpio/<pin>/value` | `0666` | current reading, or the value to drive |
| `/sys/class/gpio/<pin>/mode` | `0666` | `0` off · `1` digital write · `2` digital read · `3` blink · `4` analog write · `5` analog read |
| `/sys/class/net/<iface>/address` | `0444` | the interface's hardware address |
| `/sys/class/net/<iface>/operstate` | `0444` | `up` or `down` |
| `/sys/class/net/<iface>/{ip,netmask,gateway}` | `0444` | its current addressing |
| `/sys/class/net/<iface>/{ssid,rssi}` | `0444` | the association, on a station only |
| `/sys/bus/<bus>/devices/<id>/{driver,state,target}` | `0444` | the driver bound, `absent` · `present` · `bound` · `failed`, and where the device sits |
| `/sys/bus/usb/devices/<id>/{idVendor,idProduct,bDeviceClass}` | `0444` | what a usb device's descriptors say it is |
| `/sys/bus/i2c/devices/<id>/eeprom` | `0600` | the memory of a bound eeprom, read from any offset and written from the start |
| `/sys/block/<disk>/size` | `0444` | a bound disk's size in 512 byte sectors |
| `/sys/block/<disk>/<disk>p<n>/{start,size}` | `0444` | where a partition starts and its length, in 512 byte sectors |
| `/sys/bus/<bus>/drivers/<name>` | `0555` | one directory per compiled-in driver for that bus |
| `/sys/bus/<bus>/rescan` | `0200` | write anything to look for devices at once |

A bus appears under `/sys/bus` only when the port made an interface of that kind — `uart`, `i2c`, `spi`, `can`, `i2s`, `usb`. Devices are named the way linux names them: `0-0076` for address `0x76` on the first I2C bus, `1-2` for usb device 2, `spi0.5` for chip select 5.

The radio presents itself as `wlan0` and `ap0`. A link type registers into a table the same way a filesystem registers into the mount table, so Ethernet or PPP appears here by registering rather than by anything under `/sys` changing.

Reads return what the GPIO service last sampled; writes update its config copy, persist it and re-apply the modes. Set mode first, then value — `echo 3 > /sys/class/gpio/4/mode` followed by `echo 500 > /sys/class/gpio/4/value` blinks pin 4 at half-second intervals.

**Pin numbers.** `<pin>` is the framework's pin number, not the chip's GPIO number: each port maps it onto a GPIO that is safe to drive, leaving out the pins the board keeps for its flash, console and boot. A pin is off (`mode` `0`) until a mode is set, so a value written before the mode is stored but drives nothing.

| `<pin>` | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 | 11 | 12 | 13 | 14 | 15 |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| esp8266 (NodeMCU label) | D0 · 16 | D1 · 5 | D2 · 4 | D3 · 0 | D4 · 2 | D5 · 14 | D6 · 12 | D7 · 13 | D8 · 15 | A0 | | | | | | |
| esp32 (esp32dev GPIO) | 4 | 5 | 13 | 14 | 16 | 17 | 18 | 19 | 21 | 22 | 23 | 25 | 32 | 33 | 34 | 35 |

On a NodeMCU, pin 4 is D4, the on-board LED: `echo 3 > /sys/class/gpio/4/mode` then `echo 500 > /sys/class/gpio/4/value` blinks it. Pins 12–15 on the esp32 are its analog inputs (34 and 35 are input only). Other esp32 variants map their own set; the map is `gpioFromPinMap()` in `devices/<port>/DeviceControlInterface.cpp`. Use `echo` with redirection for these; the `fedit` editor works through a temp file and so belongs to real files only.

**devfs** exposes the usual byte streams, all `0666`:

| Node | Reading it | Writing to it |
|---|---|---|
| `/dev/null` | nothing, immediate EOF | discarded |
| `/dev/zero` | zero bytes | discarded |
| `/dev/random`, `/dev/urandom` | random bytes from the device RNG | discarded |
| `/dev/snd/pcmC0D0c` | 16 bit mono samples from a bound microphone, `0444`, present only while one is bound ([§6.2.4](#624-serialserviceprovider--__serial_service)) | refused |
| `/dev/mmcblk0` | the whole of a bound SD card, `0600`, the size of the card, read from any offset, present only while one is bound | stored at the offset, from the start for `>`; `>>` refused |
| `/dev/mmcblk0p<n>` | one partition of a bound card, `0600`, the size of the partition | stored at the offset within the partition, never past its end |

The endless ones are capped per read call — 64 bytes by default — so `cat /dev/zero` finishes instead of pinning the CPU until the watchdog fires. The RNG behind them is hardware on the ESP ports and a micros-seeded xorshift elsewhere, which is fine for filling buffers and not for keys.

**tmpfs** is a real read/write filesystem that happens to live in the heap, so the entire command surface works against it — `mkdir`, `touch`, redirection, `cat`, `cp`, `mv`, `rm`, `chmod`, `chown`. Files carry the creating session's uid and gid and the usual umask treatment, so permissions behave exactly as they do on flash. Budgets are a total byte count, a node count and a path length; `df` reports against the byte budget. A write the heap cannot hold is refused with `PDI_ERR_NO_MEM` and leaves the file as it was, so a shell redirect into `/tmp` reports `cannot write` instead of a success that stored nothing. Files are opened through handles as on flash — opened, read, written, positioned and closed, with a handle following its file through a rename — so a redirect creates or empties its file when it opens it, even for a command that prints nothing. A path that is not there is refused as `STORAGE_ERROR_NO_ENTRY` (`No such file or directory`), a directory where a file is wanted as `STORAGE_ERROR_NOT_A_FILE`, and a file where a directory is wanted as `STORAGE_ERROR_NOT_A_DIRECTORY`.

**Permissions** are stored as file attributes and enforced in the dispatcher. Every entry carries type, size, name, ctime, mtime, mode, uid and gid, stamped at creation from the current session with `0644` for files and `0755` for directories, masked by that session's umask.

```
  write family    (write, edit, delete, touch)        ─▶ W bits on the path
  read family     (read, list, search)                ─▶ R bits on the path
  create          (a path that does not exist yet)    ─▶ W bits on the directory holding it
  delete / rename / move                              ─▶ W bits on every directory losing or gaining the name
  attribute + permission changes                      ─▶ owner or root
  ownership changes                                   ─▶ root only
  metadata queries                                    ─▶ ungated
        │
        └─ root bypasses all of it; entries with no recorded owner read as root-owned
```

Adding, removing or renaming an entry changes the directory that holds it, so those need write permission on that directory whatever the entry's own mode is. A file anyone may read in a root-owned directory such as `/etc` can still be copied, but not moved or removed. Creating a path that already exists is answered by the filesystem, the same for every user. Search permission on the directories along a path is not checked; a file is protected by its own mode.

There is a narrow setuid analogue: a privileged scope that suspends those checks between a begin and an end call. The user store brackets exactly one thing in it — reading and writing `/etc/shadow` on behalf of a `su`, `login` or `passwd` running as a non-root session. The system log writes its files in the same scope, so a line raised inside a user's command still reaches `/var/log`.

Each session acts as its own user. A shell line runs as the session that typed it; an SSH connection, shell, scp and SFTP alike, runs as the account that logged in over it, whichever terminal was used last.

Copying, renaming or moving across mounts streams the file chunk by chunk into the destination backend, through one open file where the destination keeps open files, else creating on the first chunk and appending after that; a failure part-way removes the partial destination, and a `mv` is that copy followed by deleting the source. Same-backend operations go straight to the backend's own call. Directories don't cross mounts, and an existing destination is refused.

### 6.2.13 `WebServer` — `__web_server`

Started with the HTTP server interface and ticked from every pass of `serve()`. With HTTPS on, `initService` sets the certificate, key and — under mTLS — client CA paths, then binds 443 with TLS enabled and also opens a plain listener on 80 that answers every request with a redirect to the matching `https://` URL, so a browser that arrives on `http://` is carried up to the secure portal; without HTTPS it binds 80 directly. It has its own router, middleware chain, controllers and session handling, all covered in [§8](web-server.md#8-web-server).

### 6.2.14 `TelnetServiceProvider` — `__telnet_service`

Binds port 23, accepts clients into a pool of `TELNET_MAX_SESSIONS` slots, and hands each stream to the CLI as a terminal. Everything after that is the shell. A slot whose client has gone is reclaimed before the next accept, an idle session is closed after `TELNET_SHELL_IDLE_MS` while one running a command is left alone, and a client arriving when every slot is taken is told so and closed after `TELNET_POOL_FULL_GRACE_MS` rather than left waiting on a prompt that will not come. The sizes live in [src/config/TelnetConfig.h](../src/config/TelnetConfig.h).

### 6.2.15 `SSHServer` — `__sshserver_service`

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

Both authentication methods are on by default and each can be switched off in `/etc/ssh/ssh.conf`, which is created with defaults on first boot. When an attempt fails the server advertises exactly the methods still permitted. [§7.10.1](shell.md#7101-ssh-authentication) has the operational detail.

Two sessions are served concurrently by default, which is what graphical SFTP clients need — they hold a browse connection open and open a second one to move a file. The SFTP subsystem covers path resolution, stat, directory listing, open/read/write, mkdir, rmdir, remove and rename, which is enough for interactive `sftp`, for editing a remote file in FileZilla or WinSCP, and for `scp -s`.

### 6.2.16 `CommandLineServiceProvider` — `__cmd_service`

Registers every command and owns the binding between terminals and sessions. Attaching a terminal creates or finds its session and draws the login prompt; each input tick looks the session up from the terminal and makes it current before dispatching. Every in-flight command remembers which session owns it, so one session's half-finished prompt is never handed to another.

Up to `PDI_MAX_SESSIONS` sessions run at once across serial, telnet and SSH, each with its own line buffer, cursor, history position, working directory, umask and identity — and telnet and SSH each hold a pool of their own, so several remote shells of the same kind can be open together. The table is sized from those pools: `PDI_MAX_SESSIONS` defaults to one console plus `TELNET_MAX_SESSIONS` plus `SSH_MAX_SESSIONS`, and a board that names a smaller number in its device config fails the build rather than leaving one transport unable to claim the slots its own pool promises. [§7](shell.md#7-command-line--terminal) is the full CLI reference.

The same service runs scripts. One line of a script goes through the same parse-and-dispatch step a typed line does, so `source` and `/etc/rc.local` get the whole grammar without a second one; what the service wraps around a typed line — history and the prompt — happens once for the line you typed rather than once per script line, while finished commands are released after every line either way, so a long script does not hold every command it ever ran. Where a script has got to is held by the session, not by the call stack, which is what lets a line that stops for input hand the terminal back and be picked up afterwards. `/etc/rc.local` runs at the end of `initialize()`, before the login prompt is drawn. See [§7.15](shell.md#715-shell-scripts-and-etcrclocal).

### 6.2.17 `CronServiceProvider` — `__cron_service`

Starts after the cmd service, because a job is a command line. It keeps no clock and no table of its own: it listens on `EVENT_TIME_MINUTE`, and on each minute the clock calls valid it streams `/etc/crontab` a line at a time, matching each row against the instant the event carried and running whatever is due. Nothing is held between minutes, so an edit to the table takes effect on the next one and a board carries no memory cost for a feature it is not using.

It registers **no scheduler task**, only a listener — which is why it keeps that listener's id and gives it back in `stopService()`. A service that listens rather than scheduling would otherwise keep reacting after being stopped, and gain a second listener on every restart ([§6.4](#64-the-event-bus)).

At init it seeds `/etc/crontab` with its column names when none is there, `0600 root:root`, so there is always a file to `fedit`. Jobs run as root, so a table anyone could write would be a way to schedule root commands as anyone.

Full reference, including the field grammar, in [§7.16](shell.md#716-scheduled-jobs-and-etccrontab).

### 6.2.18 TLS (no provider; transport hookup + cert provisioning)

TLS has no service class either — it lives at the interface and port level. `iInstanceInterface` hands out the one outbound client the services share, built on first call and kept: `getSharedTcpClientInstance()` and, in a TLS build, `getSharedTlsClientInstance()`.

Most callers ask for neither by name. `getSharedClientForScheme()` takes the protocol you are speaking — `http` or `https`, `mqtt` or `mqtts`, `smtp` or `smtps` — and gives back the right client for it. The address already says which one it needs, so whoever holds the address makes the choice: the HTTP client looks at each URL as it goes, even when a redirect changes it half way.

**A secure address only works on a board that can do TLS, in a build where it is switched on.** Anywhere else there is no secure client to give back. An HTTP request then fails with an error instead of going out unencrypted; MQTT drops back to a plain connection so a broker that was already working keeps working. Both write a line to the log either way, so you can see which happened. Plain `http` and `mqtt` keep working everywhere with a network.

A service that keeps a connection open for a long time holds its own client instead of sharing, because the shared one gets swapped whenever someone else asks for a different protocol. MQTT builds its own from the broker setting: if you have put the broker's certificate at `/etc/ssl/ca-bundle.crt` it checks the broker really is who it claims to be, and if you have not, the traffic is still encrypted but nobody has checked who is on the other end. The log says which of the two you got. **Email is plain only** — the SMTP path has no TLS support.

Each port brings its own TLS backend, named alongside `ENABLE_TLS_SERVICE` in [§3.3.1](configuration.md#331-service-flags). Nothing above the interface knows which one it has. Certificates and keys are read from the filesystem at runtime, defaulting to `/etc/http/server.crt`, `/etc/http/server.key`, `/etc/http/client-ca.crt` and `/etc/ssl/ca-bundle.crt`.

Handshakes need more stack than a main context typically has, so enabling TLS also enables contextual execution and runs the engine on its own cooperative task with a stack sized by `TLS_TASK_STACK_SIZE`.

The bundled outbound client is created with peer verification off so that an encrypted-but-unverified connection works immediately. For production, point it at the CA bundle path and drop that line.

**Certificates come from one of two places, and which one is a port capability.** A port declaring `DEVICE_SUPPORTS_TLS_CERT_GENERATION` can issue its own: the on-device provisioner writes self-signed EC or RSA certs with the SANs you ask for, `ensureServerCert` reissues only when the stored certificate's SANs no longer cover what was asked for, and the `tls` command exposes the same issuer from the shell. With runtime generation enabled the mDNS service drives it, so the certificate covers the address and `<hostname>.local` together ([§6.2.21](#6221-mdnsserviceprovider--__mdns_service)).

**Without that capability there is no `tls` command and nothing issues a certificate on the device.** Generate the material off-device with `scripts/GenTlsCerts.py`, which does the same job with OpenSSL, and upload the result to the paths above over SFTP ([§7.10](shell.md#710-sftp-and-scp-file-transfer)). Outbound verification needs the same treatment — a CA bundle at `/etc/ssl/ca-bundle.crt` is something you put there.

**TLS is also the flag most likely to need room made for it.** A live session costs more than any other feature here, per session rather than per build, and on a device whose heap is already committed the handshake is what fails first. Expect to turn other services off to fit it — the per-session figures and what to trade against them are in [§12.3](memory-performance.md#123-the-expensive-features).

### 6.2.19 `UserStoreService` — `__user_store_service`

The user directory, in two files:

```
  /etc/passwd   0644   username:x:uid:gid:home:shell        world-readable — id and groups need it
  /etc/shadow   0600   username:hexhash:hexsalt             SHA-256 of salt‖password, 8-byte salt
```

The API is what you'd expect — look up by name or uid, add a user (writing both files, rolling the passwd row back if the shadow write fails), remove a user, verify a password with a constant-time compare, set a password.

On first start, if `/etc/passwd` is absent, it seeds root from the bootstrap credential row and hashes that password into shadow. The seeding is idempotent, and every boot re-stamps shadow as `0600`, so a file that turns up readable is corrected rather than trusted.

Verifying or changing a password has to read shadow on behalf of a non-root session, so those two methods — and only those two — bracket the access in the privileged scope described in [§6.2.12](#6212-storage-interface-init-no-provider). `useradd` gives each user their own group by setting gid to uid.

It initialises after the filesystem and before the CLI, and the auth service prefers it whenever shadow exists. That preference is the switch that turns the framework from single-credential auth into a multi-user system.

### 6.2.20 `SessionManager`

Not a service — a static registry holding one `session_t` per attached terminal, three by default.

A session carries its own line buffer and cursor, history position, autocomplete state, working directory, umask, and — when auth is on — authorisation flag, username, uid and gid. The line buffer grows from the heap as keys arrive; a key it has no room for while the heap is short is not taken and not echoed, and the line keeps what it already holds.

```
  terminal attaches      ─▶ session created, login prompt drawn
  input arrives          ─▶ session found by terminal, made current, command dispatched
  login succeeds         ─▶ uid, gid cached on the session; umask reset to default
  every filesystem call  ─▶ dispatcher reads that cached uid/gid — one dereference,
                            never a scan of /etc/passwd per file operation
  connection closes      ─▶ session detached
```

SSH attaches its session as soon as user auth succeeds, so authorisation state is anchored to the channel rather than to whichever command runs first.

### 6.2.21 `MdnsServiceProvider` — `__mdns_service`

The responder from [§2.4.2](build.md#242-mdns-and-dns-sd), running as an ordinary service on the stack's raw UDP API. It derives the hostname from the MAC, writes `/etc/hostname`, joins the multicast group when the station gets an IP, and advertises whichever servers this build is running. Responses bundle PTR, SRV, TXT and A so a single query gets everything it needs. `service status MDNS` shows what it is announcing.

Holding both the address and the name also makes it the right owner of HTTPS certificate provisioning: with `ENABLE_SERVER_TLS_CERT_GENERATION_AT_RUNTIME` it schedules `ensureServerCert` for `<hostname>.local` plus the IP, one queued job at a time, dropped again if the service stops ([§6.2.18](#6218-tls-no-provider-transport-hookup--cert-provisioning)).

## 6.3 Init order

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

## 6.4 The event bus

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

That is the whole list — [src/config/EventConfig.h](../src/config/EventConfig.h) is the one place event names are declared, and `EVENT_NAME_MAX` closes it. Not everything that fans out uses the bus: GPIO alerting does not, because a pin crossing its threshold has exactly one destination configured for it, so the GPIO service holds its own HTTP client and calls email and MQTT directly rather than publishing for nobody in particular.

Subscribe with `__utl_event.add_event_listener(name, [&](void* e){ … })`, publish with `__utl_event.execute_event(name, ptr)`.

**Subscribing returns an id, and that is how a listener is taken back.** `add_event_listener` hands back an `int16_t`, `EVENT_LISTENER_ID_INVALID` if the table was full; `remove_event_listener(id)` releases it. A service that listens rather than scheduling a task needs this: the base `stopService()` reaps tracked *tasks*, so without releasing its listener a service would keep reacting after it was stopped, and would gain a second listener on every restart. Keep the id and give it back in `stopService()`, the way cron does.

Removal **marks the slot rather than erasing it**, and a later subscription takes the slot over. That is deliberate: `execute_event` calls each handler through the table entry, so erasing during a dispatch would destroy the callable while it is running — which is exactly what a job that stops its own service would do.

## 6.5 Writing a new service

Say you want a metrics service.

1. Add `ENABLE_METRICS_SERVICE` to the device config and a `service_t` value behind the same flag.
2. Add the persisted struct ([§3.4](configuration.md#34-the-shape-of-a-service-config)) and its table ([§5.11](database.md#511-adding-a-table)).
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
