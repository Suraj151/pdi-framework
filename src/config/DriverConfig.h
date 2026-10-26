/****************************** Driver configurations *************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 19th Oct 2026
******************************************************************************/

#ifndef _DRIVER_CONFIG_H_
#define _DRIVER_CONFIG_H_

#include "Common.h"
#include "SerialConfig.h"

#ifndef DEVICE_SCAN_INTERVAL_MS
#define DEVICE_SCAN_INTERVAL_MS 2000        /* how often the serial service looks for devices that came or went */
#endif

#ifndef DEVICES_CONF_PATH
#define DEVICES_CONF_PATH "/etc/devices.conf"
#endif

#define DEVICES_CONF_COMMENT_CHAR LINE_COMMENT_CHAR

#ifndef DRIVER_NAME_MAX
#define DRIVER_NAME_MAX 16                 /* longest driver name a declaration may give */
#endif

#ifndef DRIVER_PARAMS_MAX
#define DRIVER_PARAMS_MAX 8                /* key=value settings one declaration may carry */
#endif

#ifndef SOUND_DEFAULT_RATE
#define SOUND_DEFAULT_RATE 16000            /* samples per second when a declaration gives no rate */
#endif

#ifndef SOUND_CAPTURE_CHUNK
#define SOUND_CAPTURE_CHUNK 512             /* bytes a recording takes from the capture device each pass */
#endif

#ifndef SOUND_CAPTURE_INTERVAL_MS
#define SOUND_CAPTURE_INTERVAL_MS 5         /* how often a recording takes what the capture device holds */
#endif

#ifndef SOUND_CAPTURE_SYNC_BYTES
#define SOUND_CAPTURE_SYNC_BYTES 4096       /* bytes a recording takes between saving them to storage */
#endif

#ifndef SOUND_CAPTURE_SPACE_RESERVE
#define SOUND_CAPTURE_SPACE_RESERVE 8192    /* free space a recording leaves for its header rewrite and the filesystem's own records */
#endif

#ifndef SOUND_PROBE_WAIT_MS
#define SOUND_PROBE_WAIT_MS 300             /* how long a probe listens for a mic past its start up */
#endif

#define SOUND_WAV_HEADER_SIZE 44            /* bytes of the riff header ahead of the samples */

#ifndef I2C_BUS_CLOCK_HZ
#define I2C_BUS_CLOCK_HZ 100000             /* clock the i2c bus runs at */
#endif

#ifndef I2C_BUS_TIMEOUT_MS
#define I2C_BUS_TIMEOUT_MS 50               /* how long one i2c transaction may take */
#endif

#define I2C_TRANSFER_MAX 64                 /* data bytes a driver puts in one i2c transaction */

#ifndef AT24_WRITE_TIMEOUT_MS
#define AT24_WRITE_TIMEOUT_MS 25            /* how long an eeprom may take to finish storing a page */
#endif

#ifndef SPI_BUS_CLOCK_HZ
#define SPI_BUS_CLOCK_HZ 20000000           /* fastest clock the spi bus runs a device at */
#endif

#define SD_INIT_CLOCK_HZ 400000             /* clock a card is started at, the most the sd spec allows before it is ready */

#ifndef SD_CLOCK_HZ
#define SD_CLOCK_HZ 20000000                /* clock a ready card is read and written at, capped by the bus */
#endif

#define SD_BLOCK_SIZE 512                   /* bytes in one block of a card */

#ifndef SD_READY_TIMEOUT_MS
#define SD_READY_TIMEOUT_MS 1000            /* how long a card may take to leave its idle state */
#endif

#ifndef SD_READ_TIMEOUT_MS
#define SD_READ_TIMEOUT_MS 100              /* how long a card may take to start sending a block */
#endif

#ifndef SD_WRITE_TIMEOUT_MS
#define SD_WRITE_TIMEOUT_MS 500             /* how long a card may take to store a block */
#endif

#define AT24_ONE_BYTE_ADDRESS_MAX 2048      /* largest eeprom addressed by one byte plus block bits in its bus address */
#define AT24_SIZE_MAX 524288                /* largest eeprom the block bits of a bus address can reach */

#define SOUND_WAV_RIFF            "RIFF"
#define SOUND_WAV_WAVE            "WAVE"
#define SOUND_WAV_FMT             "fmt "
#define SOUND_WAV_DATA            "data"

#define DRIVER_PARAM_SCK          "sck"
#define DRIVER_PARAM_WS           "ws"
#define DRIVER_PARAM_SD           "sd"
#define DRIVER_PARAM_RATE         "rate"
#define DRIVER_PARAM_SIZE         "size"
#define DRIVER_PARAM_PAGE         "page"
#define DRIVER_PARAM_SDA          "sda"
#define DRIVER_PARAM_SCL          "scl"
#define DRIVER_PARAM_CLOCK        "clock"
#define DRIVER_PARAM_MISO         "miso"
#define DRIVER_PARAM_MOSI         "mosi"

#ifndef DEVICE_BUSY_WAIT_MS
#define DEVICE_BUSY_WAIT_MS 1000            /* how long a stop of the serial service waits for a driver busy with its device */
#endif

#ifndef DEVICE_SCAN_PROBE_MAX_MS
#define DEVICE_SCAN_PROBE_MAX_MS 100        /* longest probe a device may take and stay in the scan; slower ones wait for udevadm trigger */
#endif

#define DRIVER_PINS_MAX           4        /* pins one line can hold on its bus */
#define DRIVER_GPIO_HOLD_MAX      64       /* hardware pins the held pin set can name */

#define DEVICE_TARGET_BUS         0xFFFFFFFFUL /* the target of a line that sets up its bus rather than declaring a device */

#define DRIVER_NAME_INMP441       "inmp441"
#define DRIVER_NAME_AT24          "at24"
#define DRIVER_NAME_MMC_SPI       "mmc_spi"

#define DEVICE_NODE_PCM_CAPTURE   "snd/pcmC0D0c"
#define DEVICE_NODE_MMC_BLOCK     "mmcblk0"
#define DEVICE_TYPE_DISK          "disk"
#define DEVICE_TYPE_PART          "part"

#define DEVICE_CONF_CHIP_SELECT   "cs="
#define DEVICE_CONF_CAPTURE       "rx"
#define DEVICE_CONF_PLAYBACK      "tx"
#define DEVICE_CONF_BUS           "bus"

#define BLOCK_PARTITIONS_MAX 4              /* partitions a dos partition table holds */
#define BLOCK_ALIGN_SECTORS 2048            /* where a partition starts when none is given, 1 MiB */
#define BLOCK_SIGNATURES_MAX 4              /* signatures one block can carry that wipefs erases */

#define BLOCK_FS_NAME_VFAT        "vfat"
#define BLOCK_FS_NAME_EXFAT       "exfat"
#define BLOCK_FS_NAME_NTFS        "ntfs"
#define BLOCK_FS_NAME_EXT2        "ext2"
#define BLOCK_FS_NAME_EXT3        "ext3"
#define BLOCK_FS_NAME_EXT4        "ext4"
#define BLOCK_FS_NAME_LITTLEFS    "littlefs"
#define BLOCK_FS_NAME_FAT         "fat"
#define BLOCK_FS_NAME_MSDOS       "msdos"

#define BLOCK_TABLE_NAME_DOS      "dos"
#define BLOCK_TABLE_NAME_GPT      "gpt"

#define BLKID_TAG_TYPE            "TYPE="
#define BLKID_TAG_PTTYPE          "PTTYPE="

#define DEVICE_STATE_NAME_ABSENT  "absent"
#define DEVICE_STATE_NAME_PRESENT "present"
#define DEVICE_STATE_NAME_BOUND   "bound"
#define DEVICE_STATE_NAME_FAILED  "failed"
#define DEVICE_STATE_NOTE_MANUAL  " manual"

#define DEVICE_ATTR_DRIVER        "driver"
#define DEVICE_ATTR_STATE         "state"
#define DEVICE_ATTR_TARGET        "target"
#define DEVICE_ATTR_VENDOR        "idVendor"
#define DEVICE_ATTR_PRODUCT       "idProduct"
#define DEVICE_ATTR_CLASS         "bDeviceClass"
#define DEVICE_ATTR_EEPROM        "eeprom"

#define DEVICE_SYSFS_DEVICES      "devices"
#define DEVICE_SYSFS_DRIVERS      "drivers"
#define DEVICE_SYSFS_RESCAN       "rescan"

#define UDEVADM_VERB_INFO         "info"
#define UDEVADM_VERB_TRIGGER      "trigger"
#define UDEVADM_VERB_MONITOR      "monitor"
#define UDEVADM_EVENT_ADD         "add "
#define UDEVADM_EVENT_REMOVE      "remove "

class iDriverInterface;

/**
* where a device stands with its driver
*/
enum driver_device_state_t : uint8_t {
  DEVICE_STATE_ABSENT = 0,
  DEVICE_STATE_PRESENT,
  DEVICE_STATE_BOUND,
  DEVICE_STATE_FAILED
};

/**
* what kind of device a driver presents
*/
enum driver_class_t : uint8_t {
  DRIVER_CLASS_NONE = 0,
  DRIVER_CLASS_SOUND_CAPTURE,
  DRIVER_CLASS_NVMEM,
  DRIVER_CLASS_BLOCK
};

/**
* the filesystem a block device or partition carries, as its signature says
*/
enum block_fs_type_t : uint8_t {
  BLOCK_FS_NONE = 0,
  BLOCK_FS_VFAT,
  BLOCK_FS_EXFAT,
  BLOCK_FS_NTFS,
  BLOCK_FS_EXT2,
  BLOCK_FS_EXT3,
  BLOCK_FS_EXT4,
  BLOCK_FS_LITTLEFS
};

/**
* the partition table a block device carries
*/
enum block_table_type_t : uint8_t {
  BLOCK_TABLE_NONE = 0,
  BLOCK_TABLE_DOS,
  BLOCK_TABLE_GPT
};

/**
* one partition of a block device, in blocks of the device
*/
struct block_partition_t {
  block_partition_t() : m_start(0), m_blocks(0), m_id(0), m_fs(BLOCK_FS_NONE), m_boot(false) {}

  uint32_t m_start;
  uint32_t m_blocks;
  uint8_t m_id;
  uint8_t m_fs;
  bool m_boot;
};

/**
 * one signature that names what a block device carries: where it sits, how many
 * bytes, and the name blkid gives it
 */
struct block_signature_t {
  block_signature_t() : m_block(0), m_offset(0), m_size(0), m_name(nullptr) {}

  uint32_t m_block;
  uint16_t m_offset;
  uint8_t m_size;
  const char *m_name;
};

/**
 * The name a filesystem type goes by, as linux blkid gives it, empty for none.
 */
inline pdiutil::string blockFsTypeName(uint8_t type) {
  switch( type ){
    case BLOCK_FS_VFAT:     return pdiutil::string(CHARPTR_WRAP(BLOCK_FS_NAME_VFAT));
    case BLOCK_FS_EXFAT:    return pdiutil::string(CHARPTR_WRAP(BLOCK_FS_NAME_EXFAT));
    case BLOCK_FS_NTFS:     return pdiutil::string(CHARPTR_WRAP(BLOCK_FS_NAME_NTFS));
    case BLOCK_FS_EXT2:     return pdiutil::string(CHARPTR_WRAP(BLOCK_FS_NAME_EXT2));
    case BLOCK_FS_EXT3:     return pdiutil::string(CHARPTR_WRAP(BLOCK_FS_NAME_EXT3));
    case BLOCK_FS_EXT4:     return pdiutil::string(CHARPTR_WRAP(BLOCK_FS_NAME_EXT4));
    case BLOCK_FS_LITTLEFS: return pdiutil::string(CHARPTR_WRAP(BLOCK_FS_NAME_LITTLEFS));
    default:                return pdiutil::string();
  }
}

/**
 * The name a partition table type goes by, as linux blkid gives it, empty for none.
 */
inline pdiutil::string blockTableName(uint8_t type) {
  switch( type ){
    case BLOCK_TABLE_DOS: return pdiutil::string(CHARPTR_WRAP(BLOCK_TABLE_NAME_DOS));
    case BLOCK_TABLE_GPT: return pdiutil::string(CHARPTR_WRAP(BLOCK_TABLE_NAME_GPT));
    default:              return pdiutil::string();
  }
}

/**
* one device the serial service knows of, found on an interface or declared
*/
struct driver_device_t {
  driver_device_t() : m_bus(SERIAL_IFACE_MAX), m_target(0), m_vid(0), m_pid(0), m_class(0),
                      m_state(DEVICE_STATE_ABSENT), m_declared(false), m_seen(false), m_pin_clash(false),
                      m_unscanned(false), m_result(0), m_instance(nullptr) {}

  serial_iface_t m_bus;
  uint32_t m_target;
  uint16_t m_vid;
  uint16_t m_pid;
  uint8_t m_class;
  uint8_t m_state;
  bool m_declared;
  bool m_seen;
  bool m_pin_clash;
  bool m_unscanned;
  int16_t m_result;
  pdiutil::string m_driver;
  pdiutil::string m_params;
  iDriverInterface *m_instance;
};

typedef bool (*DriverMatchFn)(const driver_device_t &dev);
typedef iDriverInterface *(*DriverCreateFn)();

/**
 * what a compiled in driver tells the serial service about itself; the name is a
 * RODT_ATTR read only pointer
 */
struct driver_prop_t {
  driver_prop_t(const char *name = nullptr, serial_iface_t family = SERIAL_IFACE_MAX,
                const uint8_t *addresses = nullptr, uint8_t count = 0,
                DriverMatchFn match = nullptr, DriverCreateFn create = nullptr, bool rescan = false)
    : m_name(name), m_family(family), m_addresses(addresses), m_address_count(count),
      m_match(match), m_create(create), m_rescan(rescan) {}

  const char *m_name;
  serial_iface_t m_family;
  const uint8_t *m_addresses;
  uint8_t m_address_count;
  DriverMatchFn m_match;
  DriverCreateFn m_create;
  bool m_rescan;                    /* on a bus no scan can see: asked each scan whether its device is still there, and offered the device again while it is missing */
};

/**
 * The word a device's state reads as, copied out of read only memory.
 */
inline pdiutil::string driverStateName(uint8_t state) {
  switch( state ){
    case DEVICE_STATE_PRESENT: return pdiutil::string(CHARPTR_WRAP(DEVICE_STATE_NAME_PRESENT));
    case DEVICE_STATE_BOUND:   return pdiutil::string(CHARPTR_WRAP(DEVICE_STATE_NAME_BOUND));
    case DEVICE_STATE_FAILED:  return pdiutil::string(CHARPTR_WRAP(DEVICE_STATE_NAME_FAILED));
    default:                   return pdiutil::string(CHARPTR_WRAP(DEVICE_STATE_NAME_ABSENT));
  }
}

/**
 * The name of one leaf in a device's /sys/bus directory, copied out of read only
 * memory.
 */
inline pdiutil::string deviceAttrName(uint8_t leaf) {
  switch( leaf ){
    case 0:  return pdiutil::string(CHARPTR_WRAP(DEVICE_ATTR_DRIVER));
    case 1:  return pdiutil::string(CHARPTR_WRAP(DEVICE_ATTR_STATE));
    case 2:  return pdiutil::string(CHARPTR_WRAP(DEVICE_ATTR_TARGET));
    case 3:  return pdiutil::string(CHARPTR_WRAP(DEVICE_ATTR_VENDOR));
    case 4:  return pdiutil::string(CHARPTR_WRAP(DEVICE_ATTR_PRODUCT));
    default: return pdiutil::string(CHARPTR_WRAP(DEVICE_ATTR_CLASS));
  }
}

#endif
