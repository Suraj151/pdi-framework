/********************************** SysFS **************************************
This file is part of the PDI Stack.

This is free software. You can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
Created Date    : 23rd July 2026
******************************************************************************/

#include <config/Config.h>

#ifdef ENABLE_SYSFS

#include "SysFs.h"
#include <interface/pdi.h>
#include <utility/DataTypeConversions.h>
#include <config/GpioConfig.h>

#ifdef ENABLE_NETWORK_SERVICE
#include <interface/pdi/impl/modules/netif/NetifRegistry.h>
#endif

#ifdef ENABLE_GPIO_SERVICE
#include <service_provider/device/GpioServiceProvider.h>
#include <service_provider/database/DatabaseServiceProvider.h>
#endif

#ifdef ENABLE_DRIVER_CORE
#include <service_provider/transport/SerialServiceProvider.h>
#include <interface/pdi/modules/driver/iNvmemDriverInterface.h>
#include <interface/pdi/modules/driver/iBlockDriverInterface.h>
#endif

namespace {

class SysFsNullStorage : public iStorageInterface {
public:
    int64_t read(uint64_t, void*, uint64_t) override { return PDI_ERR_NOT_SUPPORTED; }
    int64_t write(uint64_t, const void*, uint64_t) override { return PDI_ERR_NOT_SUPPORTED; }
    bool erase(uint64_t, uint64_t) override { return false; }
    uint64_t size() const override { return 0; }
};

SysFsNullStorage s_sys_null_storage;

const uint8_t s_sys_leaf_count = 2;

/**
 * The name of one leaf under a pin directory, copied out of read only memory.
 */
pdiutil::string gpioLeafName(uint8_t leaf) {
    return (0 == leaf) ? pdiutil::string(CHARPTR_WRAP(SYSFS_GPIO_VALUE))
                       : pdiutil::string(CHARPTR_WRAP(SYSFS_GPIO_MODE));
}

#ifdef ENABLE_NETWORK_SERVICE
const uint8_t s_sys_net_leaf_count = 7;

/**
 * The name of one leaf under a network interface directory, copied out of read only
 * memory.
 */
pdiutil::string netLeafName(uint8_t leaf) {
    switch (leaf) {
        case 0:  return pdiutil::string(CHARPTR_WRAP(SYSFS_NET_ADDRESS));
        case 1:  return pdiutil::string(CHARPTR_WRAP(SYSFS_NET_OPERSTATE));
        case 2:  return pdiutil::string(CHARPTR_WRAP(SYSFS_NET_IP));
        case 3:  return pdiutil::string(CHARPTR_WRAP(SYSFS_NET_NETMASK));
        case 4:  return pdiutil::string(CHARPTR_WRAP(SYSFS_NET_GATEWAY));
        case 5:  return pdiutil::string(CHARPTR_WRAP(SYSFS_NET_SSID));
        default: return pdiutil::string(CHARPTR_WRAP(SYSFS_NET_RSSI));
    }
}

// a station reports the last two; an access point has no association to
// describe, so its directory stops before them
const uint8_t s_sys_net_common_leaf_count = 5;
#endif

#ifdef ENABLE_DRIVER_CORE

const uint8_t s_sys_dev_common_leaf_count = 3;
const uint8_t s_sys_dev_usb_leaf_count = 6;
const uint8_t s_sys_block_start_leaf = 0x10;

/**
 * Whether the port made an interface of this kind.
 */
bool busFamilyPresent(serial_iface_t family) {
    for (uint8_t i = 0; i < SERIAL_IFACE_MAX; ++i) {
        if (nullptr != iSerialInterface::instances[i] &&
            family == serialFamily((serial_iface_t)i)) {
            return true;
        }
    }
    return false;
}

/**
 * How many leaves a device's directory holds.
 */
uint8_t devLeafCount(const driver_device_t &dev) {
    return (SERIAL_IFACE_USB_HOST == serialFamily(dev.m_bus)) ? s_sys_dev_usb_leaf_count
                                                             : s_sys_dev_common_leaf_count;
}

/**
 * The memory the device's bound driver presents, nullptr when the device has
 * no driver or its driver presents no memory.
 */
iNvmemDriverInterface* nvmemOf(const driver_device_t *dev) {
    if (nullptr == dev || DEVICE_STATE_BOUND != dev->m_state || nullptr == dev->m_instance ||
        DRIVER_CLASS_NVMEM != dev->m_instance->driverClass()) {
        return nullptr;
    }
    return static_cast<iNvmemDriverInterface*>(dev->m_instance);
}

#endif

}

#ifdef ENABLE_NETWORK_SERVICE
/**
 * How many leaves an interface's directory holds. Only a station has an
 * association to describe, so an access point's stops before ssid and rssi.
 */
static uint8_t netLeafCount(uint8_t index) {
    iNetifInterface* netif = __netif_registry.at(index);
    if (nullptr == netif) return 0;

    netif_info_t info;
    if (!netif->getInfo(info)) return s_sys_net_common_leaf_count;

    return (NETIF_KIND_WIFI_STA == info.m_kind) ? s_sys_net_leaf_count
                                                : s_sys_net_common_leaf_count;
}
#endif

SysFs __i_sysfs;

SysFs::SysFs() : SynthFs(s_sys_null_storage, SYS_MOUNT_PREFIX) {}

/**
 * Whether the pin exists on this board and is safe to drive.
 */
bool SysFs::isValidPin(uint8_t pin) const {
#ifdef ENABLE_GPIO_SERVICE
    return pin < MAX_GPIO_PINS && !__i_dvc_ctrl.isExceptionalGpio(pin);
#else
    (void)pin;
    return false;
#endif
}

/**
 * Which node the path names, plus the pin or interface index behind it and
 * which of that node's leaves it is.
 */
SysFs::NodeKind SysFs::classify(const char* path, int16_t& index_out,
                                uint8_t& leaf_out) const {
    const char* p = normalizePath(path);
    index_out = -1;
    leaf_out = 0;

    if (*p == '\0') return SYS_ROOT;
#ifdef ENABLE_DRIVER_CORE
    pdiutil::string busdir = CHARPTR_WRAP(SYSFS_BUS_DIR);
    if (matchSegment(p, busdir.c_str())) return classifyBus(p, index_out, leaf_out);
    pdiutil::string blockdir = CHARPTR_WRAP(SYSFS_BLOCK_DIR);
    if (matchSegment(p, blockdir.c_str())) return classifyBlock(p, index_out, leaf_out);
#endif
    pdiutil::string classdir = CHARPTR_WRAP(SYSFS_CLASS_DIR);
    if (!matchSegment(p, classdir.c_str())) return SYS_INVALID;
    if (*p == '\0') return SYS_CLASS;
    if (!nextSegment(p)) return SYS_INVALID;

    if (matchSegment(p, "gpio")) {
        if (*p == '\0') return SYS_GPIODIR;
        if (!nextSegment(p)) return SYS_INVALID;

        int32_t pin = numberSegment(p);
        if (pin < 0 || !isValidPin((uint8_t)pin)) return SYS_INVALID;
        index_out = (int16_t)pin;
        if (*p == '\0') return SYS_PIN;
        if (!nextSegment(p)) return SYS_INVALID;

        for (uint8_t i = 0; i < s_sys_leaf_count; ++i) {
            const char* cursor = p;
            pdiutil::string name = gpioLeafName(i);
            if (matchSegment(cursor, name.c_str()) && *cursor == '\0') {
                leaf_out = i;
                return (0 == i) ? SYS_VALUE : SYS_MODE;
            }
        }

        return SYS_INVALID;
    }

#ifdef ENABLE_NETWORK_SERVICE
    if (matchSegment(p, "net")) {
        if (*p == '\0') return SYS_NETDIR;
        if (!nextSegment(p)) return SYS_INVALID;

        int16_t found = -1;
        for (uint8_t i = 0; i < __netif_registry.count(); ++i) {
            iNetifInterface* netif = __netif_registry.at(i);
            if (nullptr == netif) continue;
            const char* cursor = p;
            if (matchSegment(cursor, netif->name()) && (*cursor == '\0' || *cursor == PATH_SEPARATOR_CHAR)) {
                found = (int16_t)i;
                p = cursor;
                break;
            }
        }

        if (found < 0) return SYS_INVALID;
        index_out = found;
        if (*p == '\0') return SYS_NETIF;
        if (!nextSegment(p)) return SYS_INVALID;

        for (uint8_t i = 0; i < netLeafCount((uint8_t)found); ++i) {
            const char* cursor = p;
            pdiutil::string name = netLeafName(i);
            if (matchSegment(cursor, name.c_str()) && *cursor == '\0') {
                leaf_out = i;
                return SYS_NETATTR;
            }
        }

        return SYS_INVALID;
    }
#endif

    return SYS_INVALID;
}

#ifdef ENABLE_DRIVER_CORE
/**
 * Which node under /sys/bus the rest of the path names: a bus, its devices and
 * drivers, a device leaf, or rescan.
 */
SysFs::NodeKind SysFs::classifyBus(const char* p, int16_t& index_out,
                                   uint8_t& leaf_out) const {
    if (*p == '\0') return SYS_BUS;
    if (!nextSegment(p)) return SYS_INVALID;

    int16_t family = -1;
    for (uint8_t i = 0; i < SERIAL_IFACE_MAX; ++i) {
        serial_iface_t iface = (serial_iface_t)i;
        pdiutil::string name;
        if (iface != serialFamily(iface) || !serialInterfaceName(iface, name)) continue;
        const char* cursor = p;
        if (matchSegment(cursor, name.c_str()) && busFamilyPresent(iface)) {
            family = (int16_t)iface;
            p = cursor;
            break;
        }
    }

    if (family < 0) return SYS_INVALID;
    index_out = family;
    if (*p == '\0') return SYS_BUSDIR;
    if (!nextSegment(p)) return SYS_INVALID;

    pdiutil::string rescan = CHARPTR_WRAP(DEVICE_SYSFS_RESCAN);
    if (matchSegment(p, rescan.c_str())) {
        return (*p == '\0') ? SYS_BUSRESCAN : SYS_INVALID;
    }

    pdiutil::string drivers = CHARPTR_WRAP(DEVICE_SYSFS_DRIVERS);
    if (matchSegment(p, drivers.c_str())) {
        if (*p == '\0') return SYS_BUSDRVS;
        if (!nextSegment(p)) return SYS_INVALID;

        pdiutil::vector<driver_prop_t>& registry = DeviceManager::drivers();
        for (uint16_t d = 0; d < registry.size(); ++d) {
            if ((serial_iface_t)family != registry[d].m_family) continue;
            const char* cursor = p;
            pdiutil::string name = CHARPTR_WRAP_RO(registry[d].m_name);
            if (matchSegment(cursor, name.c_str()) && *cursor == '\0') {
                index_out = (int16_t)d;
                return SYS_BUSDRV;
            }
        }
        return SYS_INVALID;
    }

    pdiutil::string devices = CHARPTR_WRAP(DEVICE_SYSFS_DEVICES);
    if (!matchSegment(p, devices.c_str())) return SYS_INVALID;
    if (*p == '\0') return SYS_BUSDEVS;
    if (!nextSegment(p)) return SYS_INVALID;

    DeviceManager* mgr = __serial_service.devices();
    if (nullptr == mgr) return SYS_INVALID;

    int16_t found = -1;
    pdiutil::string id;
    for (uint16_t i = 0; i < mgr->count(); ++i) {
        const driver_device_t* dev = mgr->at(i);
        if ((serial_iface_t)family != serialFamily(dev->m_bus) || DEVICE_STATE_ABSENT == dev->m_state) continue;
        if (!DeviceManager::deviceId(*dev, id)) continue;
        const char* cursor = p;
        if (matchSegment(cursor, id.c_str())) {
            found = (int16_t)i;
            p = cursor;
            break;
        }
    }

    if (found < 0) return SYS_INVALID;
    index_out = found;
    if (*p == '\0') return SYS_BUSDEV;
    if (!nextSegment(p)) return SYS_INVALID;

    uint8_t leaves = devLeafCount(*mgr->at((uint16_t)found));
    for (uint8_t i = 0; i < leaves; ++i) {
        const char* cursor = p;
        pdiutil::string name = deviceAttrName(i);
        if (matchSegment(cursor, name.c_str()) && *cursor == '\0') {
            leaf_out = i;
            return SYS_BUSDEVATTR;
        }
    }

    pdiutil::string eeprom = CHARPTR_WRAP(DEVICE_ATTR_EEPROM);
    if (matchSegment(p, eeprom.c_str()) && *p == '\0' && nullptr != nvmemOf(mgr->at((uint16_t)found))) {
        return SYS_BUSDEVNVMEM;
    }

    return SYS_INVALID;
}

/**
 * What one leaf of a known device currently reads.
 */
pdiutil::string SysFs::renderBusAttr(uint16_t device, uint8_t leaf) {
    pdiutil::string out;
    DeviceManager* mgr = __serial_service.devices();
    const driver_device_t* dev = (nullptr == mgr) ? nullptr : mgr->at(device);
    if (nullptr == dev) return out;

    char buf[12] = {0};

    switch (leaf) {
        case 0:
            out = dev->m_driver;
            break;
        case 1:
            out = driverStateName(dev->m_state);
            break;
        case 2:
            Uint32ToString(dev->m_target, buf, sizeof(buf));
            out = buf;
            break;
        case 3:
        case 4: {
            uint16_t value = (3 == leaf) ? dev->m_vid : dev->m_pid;
            Uint32ToHexString(value, buf, sizeof(buf), false, 4);
            out += buf;
            break;
        }
        default:
            Uint32ToHexString(dev->m_class, buf, sizeof(buf), false, 2);
            out += buf;
            break;
    }

    out += TERMINAL_NEW_LINE;
    return out;
}

/**
 * Which node under /sys/block the rest of the path names.
 */
SysFs::NodeKind SysFs::classifyBlock(const char* p, int16_t& index_out, uint8_t& leaf_out) const {
    if (*p == '\0') return SYS_BLOCK;
    if (!nextSegment(p)) return SYS_INVALID;

    DeviceManager* mgr = __serial_service.devices();
    if (nullptr == mgr) return SYS_INVALID;

    pdiutil::string node;
    pdiutil::string size = CHARPTR_WRAP(SYSFS_BLOCK_SIZE);
    pdiutil::string start = CHARPTR_WRAP(SYSFS_BLOCK_START);
    for (uint16_t i = 0; i < mgr->count(); ++i) {
        const driver_device_t* dev = mgr->at(i);
        iBlockDriverInterface* disk = iBlockDriverInterface::diskOf(dev);
        if (nullptr == disk || !disk->nodeName(node)) continue;
        const char* cursor = p;
        if (!matchSegment(cursor, node.c_str())) continue;
        index_out = (int16_t)i;
        leaf_out = 0;
        if (*cursor == '\0') return SYS_BLOCKDEV;
        if (!nextSegment(cursor)) return SYS_INVALID;

        const char* leaf = cursor;
        if (matchSegment(leaf, size.c_str()) && *leaf == '\0') return SYS_BLOCKSIZE;

        for (uint8_t n = 1; n <= disk->partitionCount(); ++n) {
            if (nullptr == disk->partition(n)) continue;
            pdiutil::string name = blockPartitionName(node.c_str(), n);
            const char* part = cursor;
            if (!matchSegment(part, name.c_str())) continue;
            leaf_out = n;
            if (*part == '\0') return SYS_BLOCKPART;
            if (!nextSegment(part)) return SYS_INVALID;
            const char* attr = part;
            if (matchSegment(attr, size.c_str()) && *attr == '\0') return SYS_BLOCKPARTATTR;
            attr = part;
            if (matchSegment(attr, start.c_str()) && *attr == '\0') {
                leaf_out = (uint8_t)(n | s_sys_block_start_leaf);
                return SYS_BLOCKPARTATTR;
            }
            return SYS_INVALID;
        }
        return SYS_INVALID;
    }
    return SYS_INVALID;
}

/**
 * What a disk or partition leaf under /sys/block currently reads.
 */
pdiutil::string SysFs::renderBlockAttr(uint16_t device, uint8_t leaf) {
    DeviceManager* mgr = __serial_service.devices();
    iBlockDriverInterface* disk = (nullptr == mgr) ? nullptr : iBlockDriverInterface::diskOf(mgr->at(device));
    if (nullptr == disk) return pdiutil::string();

    uint64_t value = ((uint64_t)disk->blocks() * disk->blockSize()) / 512;
    uint8_t number = (uint8_t)(leaf & ~s_sys_block_start_leaf);
    if (0 != number) {
        const block_partition_t* part = disk->partition(number);
        if (nullptr == part) return pdiutil::string();
        value = (0 != (leaf & s_sys_block_start_leaf)) ? part->m_start : part->m_blocks;
    }

    char buf[12] = {0};
    Uint32ToString((uint32_t)value, buf, sizeof(buf));
    pdiutil::string out(buf);
    out += TERMINAL_NEW_LINE;
    return out;
}

/**
 * The memory behind a device's eeprom file, nullptr when the path names none.
 */
iNvmemDriverInterface* SysFs::nvmemAt(const char* path) const {
    int16_t index;
    uint8_t leaf;
    if (SYS_BUSDEVNVMEM != classify(path, index, leaf)) return nullptr;
    DeviceManager* mgr = __serial_service.devices();
    return (nullptr == mgr) ? nullptr : nvmemOf(mgr->at((uint16_t)index));
}

/**
 * Delivers device memory from the offset a transfer at a time, yielding between and
 * finding the device again each time.
 */
int SysFs::readNvmem(const char* path, uint64_t size,
                     pdiutil::function<bool(char*, uint32_t)> readbackfn,
                     uint64_t offset, const char* readUntilMatchStr, bool* didmatchfound) {
    int16_t index;
    uint8_t leaf;
    DeviceManager* mgr = __serial_service.devices();
    if (SYS_BUSDEVNVMEM != classify(path, index, leaf) || nullptr == mgr) return PDI_ERR_NOT_FOUND;

    const driver_device_t* dev = mgr->at((uint16_t)index);
    serial_iface_t bus = dev->m_bus;
    uint32_t target = dev->m_target;
    iNvmemDriverInterface* nvmem = nvmemOf(dev);
    if (nullptr == nvmem) return PDI_ERR_NOT_FOUND;

    uint32_t total = nvmem->size();
    if (offset >= total) return 0;

    bool matching = (nullptr != readUntilMatchStr && '\0' != readUntilMatchStr[0]);
    uint32_t matchlen = matching ? (uint32_t)strlen(readUntilMatchStr) : 0;
    uint32_t piece = (size > 0 && size < I2C_TRANSFER_MAX) ? (uint32_t)size : I2C_TRANSFER_MAX;
    uint8_t buf[I2C_TRANSFER_MAX];
    uint32_t at = (uint32_t)offset;
    uint32_t done = 0;

    while (at < total) {
        mgr = __serial_service.devices();
        nvmem = (nullptr == mgr) ? nullptr : nvmemOf(mgr->find(bus, target));
        if (nullptr == nvmem) return PDI_ERR_NOT_FOUND;

        int32_t got = nvmem->readAt(at, buf, I2C_TRANSFER_MAX);
        if (got < 0) return (int)got;
        if (0 == got) break;

        uint32_t give = (uint32_t)got;
        bool matched = false;
        if (matching) {
            int32_t found = __strstr((const char*)buf, give, readUntilMatchStr, matchlen, 0);
            if (found >= 0) {
                give = (uint32_t)found;
                matched = true;
            }
        }

        for (uint32_t sent = 0; sent < give;) {
            uint32_t n = (give - sent < piece) ? give - sent : piece;
            if (!readbackfn((char*)buf + sent, n)) {
                return (int)(done + sent + n);
            }
            sent += n;
        }
        done += give;
        at += (uint32_t)got;

        if (matched) {
            if (nullptr != didmatchfound) *didmatchfound = true;
            break;
        }

        __i_dvc_ctrl.yield();
    }

    return (int)done;
}

int SysFs::readFile(const char* path, uint64_t size,
                    pdiutil::function<bool(char*, uint32_t)> readbackfn,
                    uint64_t offset, const char* readUntilMatchStr, bool* didmatchfound) {
    if (nullptr != path && readbackfn && nullptr != nvmemAt(path)) {
        return readNvmem(path, size, readbackfn, offset, readUntilMatchStr, didmatchfound);
    }
    return SynthFs::readFile(path, size, readbackfn, offset, readUntilMatchStr, didmatchfound);
}

/**
 * Stores bytes in device memory from the offset a transfer at a time, yielding
 * between and finding the device again each time.
 */
int SysFs::editFile(const char* path, uint64_t offset, const char* content, uint32_t size) {
    int16_t index;
    uint8_t leaf;
    DeviceManager* mgr = __serial_service.devices();
    if (SYS_BUSDEVNVMEM != classify(path, index, leaf) || nullptr == mgr) return PDI_ERR_NOT_SUPPORTED;

    const driver_device_t* dev = mgr->at((uint16_t)index);
    serial_iface_t bus = dev->m_bus;
    uint32_t target = dev->m_target;
    iNvmemDriverInterface* nvmem = nvmemOf(dev);
    if (nullptr == nvmem) return PDI_ERR_NOT_SUPPORTED;
    if (nullptr == content) return PDI_ERR_NULL_PTR;
    if (offset >= nvmem->size()) return PDI_ERR_NO_SPACE;

    uint32_t done = 0;
    while (done < size) {
        mgr = __serial_service.devices();
        nvmem = (nullptr == mgr) ? nullptr : nvmemOf(mgr->find(bus, target));
        if (nullptr == nvmem) return PDI_ERR_NOT_FOUND;

        uint32_t piece = (size - done < I2C_TRANSFER_MAX) ? size - done : I2C_TRANSFER_MAX;
        int32_t stored = nvmem->writeAt((uint32_t)offset + done, (const uint8_t*)content + done, piece);
        if (stored < 0) return (0 == done || PDI_ERR_NO_SPACE != stored) ? (int)stored : (int)done;
        if (0 == stored) break;
        done += (uint32_t)stored;
        if ((uint32_t)stored < piece) break;

        __i_dvc_ctrl.yield();
    }

    return (int)done;
}

/**
 * How long a file node reads, a device's memory by its size.
 */
int64_t SysFs::sizeOf(const char* path) {
    iNvmemDriverInterface* nvmem = nvmemAt(path);
    return (nullptr != nvmem) ? (int64_t)nvmem->size() : SynthFs::sizeOf(path);
}
#endif

SynthFs::synth_node_t SysFs::resolve(const char* path) {
    int16_t index;
    uint8_t leaf;

    switch (classify(path, index, leaf)) {
        case SYS_ROOT:
        case SYS_CLASS:
        case SYS_GPIODIR:
        case SYS_PIN:
        case SYS_NETDIR:
        case SYS_NETIF:
        case SYS_BUS:
        case SYS_BUSDIR:
        case SYS_BUSDEVS:
        case SYS_BUSDEV:
        case SYS_BUSDRVS:
        case SYS_BUSDRV:
        case SYS_BLOCK:
        case SYS_BLOCKDEV:
        case SYS_BLOCKPART:
            return SYNTH_DIR;
        case SYS_BLOCKSIZE:
        case SYS_BLOCKPARTATTR:
            return SYNTH_FILE;
        case SYS_VALUE:
        case SYS_MODE:
        case SYS_NETATTR:
        case SYS_BUSDEVATTR:
        case SYS_BUSDEVNVMEM:
        case SYS_BUSRESCAN:
            return SYNTH_FILE;
        default:
            return SYNTH_NONE;
    }
}

/**
 * The permission bits a node carries. Read-only trees keep the default.
 */
uint16_t SysFs::permsFor(const char* path, synth_node_t kind) {
    if (SYNTH_DIR == kind) return 0555;

    int16_t index;
    uint8_t leaf;
    NodeKind k = classify(path, index, leaf);
    if (SYS_BUSRESCAN == k) return 0200;
    if (SYS_BUSDEVNVMEM == k) return 0600;
    return (SYS_NETATTR == k || SYS_BUSDEVATTR == k || SYS_BLOCKSIZE == k || SYS_BLOCKPARTATTR == k) ? 0444 : 0666;
}

pdiutil::string SysFs::render(const char* path) {
    int16_t index;
    uint8_t leaf;
    NodeKind k = classify(path, index, leaf);

#ifdef ENABLE_NETWORK_SERVICE
    if (SYS_NETATTR == k) {
        return renderNetAttr((uint8_t)index, leaf);
    }
#endif

#ifdef ENABLE_DRIVER_CORE
    if (SYS_BUSDEVATTR == k) {
        return renderBusAttr((uint16_t)index, leaf);
    }
    if (SYS_BLOCKSIZE == k || SYS_BLOCKPARTATTR == k) {
        return renderBlockAttr((uint16_t)index, leaf);
    }
#endif

#ifdef ENABLE_GPIO_SERVICE
    if (k != SYS_VALUE && k != SYS_MODE) return pdiutil::string();

    uint32_t v = (k == SYS_VALUE)
                     ? (uint32_t)__gpio_service.m_gpio_config_copy.gpio_readings[index]
                     : (uint32_t)__gpio_service.m_gpio_config_copy.gpio_mode[index];

    char buf[12];
    Uint32ToString(v, buf, sizeof(buf));
    pdiutil::string out(buf);
    out += TERMINAL_NEW_LINE;
    return out;
#else
    return pdiutil::string();
#endif
}

int SysFs::writeFile(const char* path, const char* content, uint32_t size, bool append) {
#ifdef ENABLE_DRIVER_CORE
    {
        int16_t family;
        uint8_t leaf;
        if (SYS_BUSRESCAN == classify(path, family, leaf)) {
            DeviceManager* mgr = __serial_service.devices();
            if (nullptr == mgr) return PDI_ERR_NOT_INITIALIZED;
            mgr->trigger();
            return (int)size;
        }
        if (nullptr != nvmemAt(path)) {
            return append ? PDI_ERR_NOT_SUPPORTED : editFile(path, 0, content, size);
        }
    }
#endif
#ifdef ENABLE_GPIO_SERVICE
    int16_t pin;
    uint8_t leaf;
    NodeKind k = classify(path, pin, leaf);
    if (!content) return PDI_ERR_NULL_PTR;
    if (k != SYS_VALUE && k != SYS_MODE) return STORAGE_ERROR_READ_ONLY;

    uint16_t val = StringToUint16(content, (uint8_t)(size > 255 ? 255 : size));

    if (k == SYS_MODE) {
        if (val >= GPIO_MODE_MAX) return PDI_ERR_RANGE;
        __gpio_service.m_gpio_config_copy.gpio_mode[pin] = (uint8_t)val;
    } else {
        __gpio_service.m_gpio_config_copy.gpio_readings[pin] = val;
    }

    __database_service.set_gpio_config_table(&__gpio_service.m_gpio_config_copy);
    __gpio_service.handleGpioModes(GPIO_WRITE_CONFIG);
    return (int)size;
#else
    (void)path; (void)content; (void)size; (void)append;
    return PDI_ERR_NOT_SUPPORTED;
#endif
}

int SysFs::listChildren(const char* path, pdiutil::vector<file_info_t>& items) {
    int16_t index;
    uint8_t leaf;
    NodeKind k = classify(path, index, leaf);

    if (k == SYS_ROOT) {
#ifdef ENABLE_DRIVER_CORE
        pdiutil::string busdir = CHARPTR_WRAP(SYSFS_BUS_DIR);
        addEntry(items, busdir.c_str(), FILE_TYPE_DIR, 0555);
        pdiutil::string blockdir = CHARPTR_WRAP(SYSFS_BLOCK_DIR);
        addEntry(items, blockdir.c_str(), FILE_TYPE_DIR, 0555);
#endif
        pdiutil::string classdir = CHARPTR_WRAP(SYSFS_CLASS_DIR);
        addEntry(items, classdir.c_str(), FILE_TYPE_DIR, 0555);
#ifdef ENABLE_DRIVER_CORE
    } else if (k == SYS_BUS) {
        for (uint8_t i = 0; i < SERIAL_IFACE_MAX; ++i) {
            serial_iface_t iface = (serial_iface_t)i;
            pdiutil::string name;
            if (iface == serialFamily(iface) && serialInterfaceName(iface, name) && busFamilyPresent(iface)) {
                addEntry(items, name.c_str(), FILE_TYPE_DIR, 0555);
            }
        }
    } else if (k == SYS_BUSDIR) {
        pdiutil::string devices = CHARPTR_WRAP(DEVICE_SYSFS_DEVICES);
        pdiutil::string drivers = CHARPTR_WRAP(DEVICE_SYSFS_DRIVERS);
        pdiutil::string rescan = CHARPTR_WRAP(DEVICE_SYSFS_RESCAN);
        addEntry(items, devices.c_str(), FILE_TYPE_DIR, 0555);
        addEntry(items, drivers.c_str(), FILE_TYPE_DIR, 0555);
        addEntry(items, rescan.c_str(), FILE_TYPE_REG, 0200);
    } else if (k == SYS_BUSDEVS) {
        DeviceManager* mgr = __serial_service.devices();
        pdiutil::string id;
        for (uint16_t i = 0; nullptr != mgr && i < mgr->count(); ++i) {
            const driver_device_t* dev = mgr->at(i);
            if ((serial_iface_t)index != serialFamily(dev->m_bus) || DEVICE_STATE_ABSENT == dev->m_state) continue;
            if (DeviceManager::deviceId(*dev, id)) {
                addEntry(items, id.c_str(), FILE_TYPE_DIR, 0555);
            }
        }
    } else if (k == SYS_BUSDEV) {
        DeviceManager* mgr = __serial_service.devices();
        const driver_device_t* dev = (nullptr == mgr) ? nullptr : mgr->at((uint16_t)index);
        for (uint8_t i = 0; nullptr != dev && i < devLeafCount(*dev); ++i) {
            pdiutil::string name = deviceAttrName(i);
            addEntry(items, name.c_str(), FILE_TYPE_REG, 0444);
        }
        iNvmemDriverInterface* nvmem = nvmemOf(dev);
        if (nullptr != nvmem) {
            pdiutil::string eeprom = CHARPTR_WRAP(DEVICE_ATTR_EEPROM);
            addEntry(items, eeprom.c_str(), FILE_TYPE_REG, 0600, (int64_t)nvmem->size());
        }
    } else if (k == SYS_BLOCK) {
        DeviceManager* mgr = __serial_service.devices();
        pdiutil::string node;
        for (uint16_t i = 0; nullptr != mgr && i < mgr->count(); ++i) {
            const driver_device_t* dev = mgr->at(i);
            if (nullptr != iBlockDriverInterface::diskOf(dev) && dev->m_instance->nodeName(node)) {
                addEntry(items, node.c_str(), FILE_TYPE_DIR, 0555);
            }
        }
    } else if (k == SYS_BLOCKDEV) {
        pdiutil::string size = CHARPTR_WRAP(SYSFS_BLOCK_SIZE);
        addEntry(items, size.c_str(), FILE_TYPE_REG, 0444);
        DeviceManager* mgr = __serial_service.devices();
        iBlockDriverInterface* disk = (nullptr == mgr) ? nullptr : iBlockDriverInterface::diskOf(mgr->at((uint16_t)index));
        pdiutil::string node;
        for (uint8_t n = 1; nullptr != disk && disk->nodeName(node) && n <= disk->partitionCount(); ++n) {
            if (nullptr == disk->partition(n)) continue;
            addEntry(items, blockPartitionName(node.c_str(), n).c_str(), FILE_TYPE_DIR, 0555);
        }
    } else if (k == SYS_BLOCKPART) {
        pdiutil::string size = CHARPTR_WRAP(SYSFS_BLOCK_SIZE);
        pdiutil::string start = CHARPTR_WRAP(SYSFS_BLOCK_START);
        addEntry(items, size.c_str(), FILE_TYPE_REG, 0444);
        addEntry(items, start.c_str(), FILE_TYPE_REG, 0444);
    } else if (k == SYS_BUSDRVS) {
        pdiutil::vector<driver_prop_t>& registry = DeviceManager::drivers();
        for (uint16_t d = 0; d < registry.size(); ++d) {
            if ((serial_iface_t)index == registry[d].m_family) {
                pdiutil::string name = CHARPTR_WRAP_RO(registry[d].m_name);
                addEntry(items, name.c_str(), FILE_TYPE_DIR, 0555);
            }
        }
    } else if (k == SYS_BUSDRV) {
#endif
    } else if (k == SYS_CLASS) {
        addEntry(items, "gpio", FILE_TYPE_DIR, 0555);
#ifdef ENABLE_NETWORK_SERVICE
        addEntry(items, "net", FILE_TYPE_DIR, 0555);
#endif
#ifdef ENABLE_NETWORK_SERVICE
    } else if (k == SYS_NETDIR) {
        for (uint8_t i = 0; i < __netif_registry.count(); ++i) {
            iNetifInterface* netif = __netif_registry.at(i);
            if (nullptr == netif) continue;
            addEntry(items, netif->name(), FILE_TYPE_DIR, 0555);
        }
    } else if (k == SYS_NETIF) {
        for (uint8_t i = 0; i < netLeafCount((uint8_t)index); ++i) {
            pdiutil::string name = netLeafName(i);
            addEntry(items, name.c_str(), FILE_TYPE_REG, 0444);
        }
#endif
    } else if (k == SYS_GPIODIR) {
        char numbuf[6];
        for (uint8_t i = 0; i < MAX_GPIO_PINS; ++i) {
            if (!isValidPin(i)) continue;
            Uint32ToString(i, numbuf, sizeof(numbuf));
            addEntry(items, numbuf, FILE_TYPE_DIR, 0555);
        }
    } else if (k == SYS_PIN) {
        for (uint8_t i = 0; i < s_sys_leaf_count; ++i) {
            pdiutil::string name = gpioLeafName(i);
            addEntry(items, name.c_str(), FILE_TYPE_REG, 0666);
        }
    } else {
        return STORAGE_ERROR_NOT_A_DIRECTORY;
    }

    return (int)items.size();
}

#ifdef ENABLE_NETWORK_SERVICE
/**
 * What one leaf of a registered network interface currently reads.
 */
pdiutil::string SysFs::renderNetAttr(uint8_t index, uint8_t leaf) {
    iNetifInterface* netif = __netif_registry.at(index);
    if (nullptr == netif) return pdiutil::string();

    netif_info_t info;
    if (!netif->getInfo(info)) return pdiutil::string();

    pdiutil::string out;

    switch (leaf) {
        case 0:
            out = pdiutil::string(info.m_mac);
            break;
        case 1:
            out = info.m_up ? CHARPTR_WRAP(SYSFS_NET_STATE_UP) : CHARPTR_WRAP(SYSFS_NET_STATE_DOWN);
            break;
        case 2:
            out = pdiutil::string(info.m_ip);
            break;
        case 3:
            out = pdiutil::string(info.m_netmask);
            break;
        case 4:
            out = pdiutil::string(info.m_gateway);
            break;
        case 5:
            out = info.m_ssid;
            break;
        default: {
            char buf[12];
            Int32ToString(info.m_rssi, buf, sizeof(buf), 0);
            out = pdiutil::string(buf);
            break;
        }
    }

    // a leaf that has nothing to say still reads as a line, so a reader can
    // tell an empty value from a node that is not there
    out += TERMINAL_NEW_LINE;
    return out;
}
#endif

#endif
