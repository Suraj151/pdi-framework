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

#ifdef ENABLE_GPIO_SERVICE
#include <service_provider/device/GpioServiceProvider.h>
#include <service_provider/database/DatabaseServiceProvider.h>
#endif

namespace {

class SysFsNullStorage : public iStorageInterface {
public:
    int64_t read(uint64_t, void*, uint64_t) override { return -1; }
    int64_t write(uint64_t, const void*, uint64_t) override { return -1; }
    bool erase(uint64_t, uint64_t) override { return false; }
    uint64_t size() const override { return 0; }
};

SysFsNullStorage s_sys_null_storage;

// Leaf node names under a pin directory. Plain literals at namespace scope
// (RODT_ATTR is only legal inside a function body on esp8266).
const char* const s_sys_leaf_names[] = { "value", "mode" };
const uint8_t s_sys_leaf_count = sizeof(s_sys_leaf_names) / sizeof(s_sys_leaf_names[0]);

bool segEquals(const char*& p, const char* lit) {
    uint32_t n = strlen(lit);
    if (strncmp(p, lit, n) != 0) return false;
    char after = p[n];
    if (after != '\0' && after != '/') return false;
    p += n;
    return true;
}

int16_t parseSeg(const char*& p) {
    if (*p < '0' || *p > '9') return -1;
    int16_t v = 0;
    while (*p >= '0' && *p <= '9') {
        v = (int16_t)(v * 10 + (*p - '0'));
        p++;
        if (v > 1000) return -1;
    }
    return v;
}

}

SysFs __i_sysfs;

SysFs::SysFs() : iFileSystemInterface(s_sys_null_storage) {}

const char* SysFs::normalizePath(const char* path) const {
    if (!path) return "";
    while (*path == '/') path++;
    return path;
}

bool SysFs::isValidPin(uint8_t pin) const {
#ifdef ENABLE_GPIO_SERVICE
    return pin < MAX_GPIO_PINS && !__i_dvc_ctrl.isExceptionalGpio(pin);
#else
    (void)pin;
    return false;
#endif
}

SysFs::NodeKind SysFs::classify(const char* path, int16_t& pin_out) const {
    const char* p = normalizePath(path);
    pin_out = -1;

    if (*p == '\0') return SYS_ROOT;
    if (!segEquals(p, "class")) return SYS_INVALID;
    if (*p == '\0') return SYS_CLASS;
    p++; // consume '/'
    if (!segEquals(p, "gpio")) return SYS_INVALID;
    if (*p == '\0') return SYS_GPIODIR;
    p++; // consume '/'

    int16_t pin = parseSeg(p);
    if (pin < 0 || !isValidPin((uint8_t)pin)) return SYS_INVALID;
    pin_out = pin;
    if (*p == '\0') return SYS_PIN;
    if (*p != '/') return SYS_INVALID;
    p++; // consume '/'

    if (strcmp(p, "value") == 0) return SYS_VALUE;
    if (strcmp(p, "mode") == 0) return SYS_MODE;
    return SYS_INVALID;
}

pdiutil::string SysFs::basename(const char* path) {
    if (!path) return pdiutil::string();
    const char* last = path;
    for (const char* p = path; *p; ++p) {
        if (*p == '/') last = p + 1;
    }
    return pdiutil::string(last);
}

pdiutil::string SysFs::generateContent(const char* path) {
#ifdef ENABLE_GPIO_SERVICE
    int16_t pin;
    NodeKind k = classify(path, pin);
    if (k != SYS_VALUE && k != SYS_MODE) return pdiutil::string();

    uint32_t v = (k == SYS_VALUE)
                     ? (uint32_t)__gpio_service.m_gpio_config_copy.gpio_readings[pin]
                     : (uint32_t)__gpio_service.m_gpio_config_copy.gpio_mode[pin];

    char buf[12];
    Uint32ToString(v, buf, sizeof(buf));
    pdiutil::string out(buf);
    out += "\n";
    return out;
#else
    (void)path;
    return pdiutil::string();
#endif
}

int SysFs::writeFile(const char* path, const char* content, uint32_t size, bool append) {
#ifdef ENABLE_GPIO_SERVICE
    int16_t pin;
    NodeKind k = classify(path, pin);
    if ((k != SYS_VALUE && k != SYS_MODE) || !content) return -1;

    uint16_t val = StringToUint16(content, (uint8_t)(size > 255 ? 255 : size));

    if (k == SYS_MODE) {
        if (val >= GPIO_MODE_MAX) return -1;
        __gpio_service.m_gpio_config_copy.gpio_mode[pin] = (uint8_t)val;
    } else {
        __gpio_service.m_gpio_config_copy.gpio_readings[pin] = val;
    }

    __database_service.set_gpio_config_table(&__gpio_service.m_gpio_config_copy);
    __gpio_service.handleGpioModes(GPIO_WRITE_CONFIG);
    return (int)size;
#else
    (void)path; (void)content; (void)size; (void)append;
    return -1;
#endif
}

int SysFs::readFile(const char* path, uint64_t size, pdiutil::function<bool(char*, uint32_t)> readbackfn, uint64_t offset, const char* readUntilMatchStr, bool* didmatchfound) {
    if (!path || !readbackfn) return -1;
    pdiutil::string content = generateContent(path);
    if (content.empty()) return -1;
    if (offset >= content.length()) return 0;

    // `size` is the per-iteration chunk limit, not a total cap — loop the
    // callback until the whole content is delivered or it returns false.
    uint32_t total = content.length() - (uint32_t)offset;
    uint32_t chunk = (size > 0 && size < total) ? (uint32_t)size : total;
    uint32_t done = 0;
    while (done < total) {
        uint32_t n = total - done;
        if (n > chunk) n = chunk;
        if (!readbackfn((char*)content.c_str() + offset + done, n)) break;
        done += n;
    }
    return (int)done;
}

int64_t SysFs::getFileSize(const char* path) {
    pdiutil::string content = generateContent(path);
    return content.empty() ? -1 : (int64_t)content.length();
}

bool SysFs::isFileExist(const char* path) {
    int16_t pin;
    NodeKind k = classify(path, pin);
    return (k == SYS_VALUE || k == SYS_MODE);
}

bool SysFs::isDirExist(const char* path) {
    int16_t pin;
    NodeKind k = classify(path, pin);
    return (k == SYS_ROOT || k == SYS_CLASS || k == SYS_GPIODIR || k == SYS_PIN);
}

bool SysFs::isDirectory(const char* path) {
    return isDirExist(path);
}

int SysFs::getDirFileList(const char* path, pdiutil::vector<file_info_t>& items, const char* pattern) {
    int16_t pin;
    NodeKind k = classify(path, pin);

    auto addEntry = [&](const char* name, file_type_t type, uint16_t perms) {
        file_info_t info;
        memset(&info, 0, sizeof(info));
        info.m_type = type;
        info.m_perms = perms;
        info.m_uid = 0;
        info.m_gid = 0;
        info.m_ctime = 0;
        info.m_mtime = 0;
        // Callers (ls) delete[] m_name — allocate a heap copy so the free path
        // is safe.
        uint32_t nlen = strlen(name);
        info.m_name = new char[nlen + 1];
        memcpy(info.m_name, name, nlen);
        info.m_name[nlen] = '\0';
        items.push_back(info);
    };

    if (k == SYS_ROOT) {
        addEntry("class", FILE_TYPE_DIR, 0555);
    } else if (k == SYS_CLASS) {
        addEntry("gpio", FILE_TYPE_DIR, 0555);
    } else if (k == SYS_GPIODIR) {
        char numbuf[6];
        for (uint8_t i = 0; i < MAX_GPIO_PINS; ++i) {
            if (!isValidPin(i)) continue;
            Uint32ToString(i, numbuf, sizeof(numbuf));
            addEntry(numbuf, FILE_TYPE_DIR, 0555);
        }
    } else if (k == SYS_PIN) {
        for (uint8_t i = 0; i < s_sys_leaf_count; ++i) {
            addEntry(s_sys_leaf_names[i], FILE_TYPE_REG, 0666);
        }
    } else {
        return -1;
    }
    return (int)items.size();
}

int SysFs::getFileAttr(const char* path, uint8_t type, void* buffer, uint32_t size) {
    if (!buffer || size == 0) return -1;
    int16_t pin;
    NodeKind k = classify(path, pin);
    if (k == SYS_INVALID) return -1;

    bool isdir = (k == SYS_ROOT || k == SYS_CLASS || k == SYS_GPIODIR || k == SYS_PIN);

    if (type == FILE_ATTR_PERMS && size >= sizeof(uint16_t)) {
        *(uint16_t*)buffer = isdir ? 0555 : 0666;
        return sizeof(uint16_t);
    }
    if (type == FILE_ATTR_UID && size >= sizeof(uint16_t)) {
        *(uint16_t*)buffer = 0;
        return sizeof(uint16_t);
    }
    if (type == FILE_ATTR_GID && size >= sizeof(uint16_t)) {
        *(uint16_t*)buffer = 0;
        return sizeof(uint16_t);
    }
    return -1;
}

int SysFs::getFileMeta(const char* path, file_info_t& out) {
    int16_t pin;
    NodeKind k = classify(path, pin);
    // Per iFileSystemInterface contract, m_name is left untouched — some callers
    // assume m_name (when set) is heap-owned and will delete[] it.

    if (k == SYS_ROOT || k == SYS_CLASS || k == SYS_GPIODIR || k == SYS_PIN) {
        out.m_type  = FILE_TYPE_DIR;
        out.m_size  = 0;
        out.m_perms = 0555;
        out.m_uid   = 0;
        out.m_gid   = 0;
        out.m_ctime = 0;
        out.m_mtime = 0;
        return 0;
    }

    if (k == SYS_VALUE || k == SYS_MODE) {
        out.m_type  = FILE_TYPE_REG;
        out.m_size  = getFileSize(path);
        out.m_perms = 0666;
        out.m_uid   = 0;
        out.m_gid   = 0;
        out.m_ctime = 0;
        out.m_mtime = 0;
        return 0;
    }

    return -1;
}

#endif
