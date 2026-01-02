/******************************** Type Definitions ****************************
This file is part of the PDI stack.

This is free software. You can redistribute it and/or modify it but without any
warranty.

The DataTypeDef file provides type definitions, utility structures, and enums
used throughout the PDI stack. It includes definitions for connection statuses,
logging types, IP address handling, task management, and callback functions.

Author          : Suraj I.
Created Date    : 1st June 2019
******************************************************************************/

#ifndef _DATA_TYPE_DEFS_H_
#define _DATA_TYPE_DEFS_H_

#include "../../devices/DeviceConfig.h"
#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>
#include <utility/pdistl/functional>
#include <utility/pdistl/string>
#include <utility/pdistl/vector>
#include <utility/pdistl/cstdio>
#include <utility/pdistl/cstring>
#include <utility/pdistl/cmath>

// Namespace for utility types and functions
namespace pdiutil {

    // String type alias
    using string = pdistd::string;

    // Vector type alias
    template <class T, class A = pdistd::allocator<T>> 
    using vector = pdistd::vector<T, A>;

    // Conversion to string
    using pdistd::to_string;

    // Function type alias
    template<typename _Res, typename... _ArgTypes>
    using function = pdistd::function<_Res, _ArgTypes...>;

} // namespace pdiutil

// Attribute for read-only data (can be redefined in derived interfaces)
#define RODT_ATTR(x) x
#define PROG_RODT_ATTR
#define PROG_RODT_PTR

// String operations api for read-write-modify string data (can be redefined in derived interfaces)
// _ro for each of api referes to read-only data region specific api
#define strcat strcat
#define strcat_ro strcat
#define strncat strncat
#define strncat_ro strncat
#define strcpy strcpy
#define strcpy_ro strcpy
#define strncpy strncpy
#define strncpy_ro strncpy
#define strlen strlen
#define strlen_ro strlen
#define strcmp strcmp
#define strcmp_ro strcmp
#define strncmp strncmp
#define strncmp_ro strncmp
#define memcpy memcpy
#define memcpy_ro memcpy

// define weak functions, so when the device doesn't define them,
// the linker just sets their address to 0
namespace rofn{

    // pdiutil::string get_string_ro(const void *rostr) __attribute__((weak));
    extern char* to_charptr(const void *rostr);

    struct ROPTR{

        ROPTR() : ptr(nullptr), autodelete(true) {}

        ROPTR(const void *p, bool auto_del = true) : ptr(nullptr), autodelete(auto_del) {
            ptr = rofn::to_charptr(p);
        }

        ROPTR(const ROPTR &obj) : ptr(nullptr), autodelete(obj.autodelete) {
            if( nullptr != obj.ptr ){
                ((ROPTR &)obj).setAutoDelete(false); // Prevent deletion of the original pointer
                ptr = obj.ptr;
            }
        }

        ROPTR(ROPTR &&obj) : ptr(obj.ptr), autodelete(obj.autodelete) {
            if( nullptr != obj.ptr ){
                obj.setAutoDelete(false); // Prevent deletion of the original pointer
            }
        }

        ROPTR& operator=(const ROPTR &obj) {
            if (this != &obj) {

                if( nullptr != ptr )
                    delete[] ptr;

                autodelete = obj.autodelete;
                if( nullptr != obj.ptr ){
                    ((ROPTR &)obj).setAutoDelete(false); // Prevent deletion of the original pointer
                    ptr = obj.ptr;
                }
            }
            return *this;
        }

        ~ROPTR() {
            if (ptr && autodelete) {
                delete[] ptr; // Free the memory allocated for the non read-only string
            }
            ptr = nullptr;
        }

        void setAutoDelete(bool auto_del=true) { autodelete = auto_del; } ///< Modify autodelete
        operator char*() const { return ptr; } ///< Implicit conversion to char*
        private :
        char *ptr = nullptr; ///< Pointer to the non read-only string
        bool autodelete = true; ///< Flag to indicate if the pointer should be deleted
    };    
} // namespace rofunctions
#define ROPTR_WRAP(x) rofn::ROPTR(RODT_ATTR(x))
#define CHARPTR_WRAP(x) (char*)rofn::ROPTR(RODT_ATTR(x))
#define CHARPTR_WRAP_RO(x) (char*)rofn::ROPTR(x)


// redefine these in derived interface
#define LOGBEGIN

#define LogI(v) // info log
#define LogE(v) // error log
#define LogW(v) // warning log
#define LogS(v) // success log

#define LogFmtI(f, args...)
#define LogFmtE(f, args...)
#define LogFmtW(f, args...)
#define LogFmtS(f, args...)


/* Connection status enums */
typedef enum {
    CONN_STATUS_IDLE = 0,
    CONN_STATUS_CONNECTED,
    CONN_STATUS_CONNECTION_FAILED,
    CONN_STATUS_CONNECTION_LOST,
    CONN_STATUS_CONFIG_ERROR,
    CONN_STATUS_NOT_AVAILABLE,
    CONN_STATUS_DISCONNECTED,
    CONN_STATUS_MAX
} conn_status_t;

// GPIO and WiFi-related typedefs
typedef uint8_t gpio_id_t;
typedef int64_t gpio_val_t;
typedef uint8_t pdi_wifi_mode_t;
typedef uint8_t sleep_mode_t;
typedef conn_status_t wifi_status_t;
typedef uint8_t follow_redirects_t;

/**
 * Callback function typedefs
 */
typedef pdiutil::function<void(int)> CallBackIntArgFn;
typedef pdiutil::function<void(void)> CallBackVoidArgFn;
typedef pdiutil::function<void(void *)> CallBackVoidPointerArgFn;

/**
 * Logging types
 */
typedef enum {
    INFO_LOG,
    ERROR_LOG,
    WARNING_LOG,
    SUCCESS_LOG
} logger_type_t;

/**
 * IP address types
 */
typedef enum {
    IP_ADDR_TYPE_V4 = 4,  /** IPv4 */
    IP_ADDR_TYPE_V6 = 6,  /** IPv6 */
    IP_ADDR_TYPE_ANY = 46 /** IPv4+IPv6 ("dual-stack") */
} ip_addr_type;

// IPv4 address constants and macros
/** 255.255.255.255 */
#define IP4_ADDRESS_NONE        ((uint32_t)0xffffffffUL)
/** 127.0.0.1 */
#define IP4_ADDRESS_LOOPBACK    ((uint32_t)0x7f000001UL)
/** 0.0.0.0 */
#define IP4_ADDRESS_ANY         ((uint32_t)0x00000000UL)
/** 255.255.255.255 */
#define IP4_ADDRESS_BROADCAST   ((uint32_t)0xffffffffUL)
/** get byte from 32bit ipaddr */
#define IP4_ADDRESS_BYTE(ipaddr, idx) (((const uint8_t *)(&ipaddr))[idx])

/** Check the byte order */
#define IS_BIG_ENDIAN() ({ uint32_t n = 0x11223344; uint8_t *p = (uint8_t *)&n; (*p == 0x11); })

/**
 * @struct ipaddress_t
 * @brief Represents an IP address.
 *
 * Provides constructors and operators for handling IPv4 addresses.
 */
struct ipaddress_t {
    uint8_t ip4[4]; ///< IPv4 address bytes
    ip_addr_type type; ///< IP address type (IPv4/IPv6)

    ipaddress_t() : type(IP_ADDR_TYPE_V4) {
        ip4[0] = 0;
        ip4[1] = 0;
        ip4[2] = 0;
        ip4[3] = 0;
    }

    ipaddress_t(uint32_t _ip4) : type(IP_ADDR_TYPE_V4) {
        ip4[0] = IP4_ADDRESS_BYTE(_ip4, 0);
        ip4[1] = IP4_ADDRESS_BYTE(_ip4, 1);
        ip4[2] = IP4_ADDRESS_BYTE(_ip4, 2);
        ip4[3] = IP4_ADDRESS_BYTE(_ip4, 3);
    }

    ipaddress_t(uint8_t first_octet, uint8_t second_octet, uint8_t third_octet, uint8_t fourth_octet) : type(IP_ADDR_TYPE_V4) {
        ip4[0] = first_octet;
        ip4[1] = second_octet;
        ip4[2] = third_octet;
        ip4[3] = fourth_octet;
    }

    operator pdiutil::string() {
        return (pdiutil::to_string(ip4[0]) + "." + pdiutil::to_string(ip4[1]) + "." + pdiutil::to_string(ip4[2]) + "." + pdiutil::to_string(ip4[3]));
    }

    operator uint32_t() {
        if (IS_BIG_ENDIAN()) {
            return (((uint32_t)ip4[0] << 24) | ((uint32_t)ip4[1] << 16) | ((uint32_t)ip4[2] << 8) | ((uint32_t)ip4[3]));
        } else {
            return (((uint32_t)ip4[3] << 24) | ((uint32_t)ip4[2] << 16) | ((uint32_t)ip4[1] << 8) | ((uint32_t)ip4[0]));
        }
    }

    uint8_t operator[](uint8_t index) {
        return index < 4 ? ip4[index] : 0;
    }

    bool isSet() {
        return (((uint32_t)*this) != IP4_ADDRESS_NONE) && (((uint32_t)*this) != IP4_ADDRESS_ANY);
    }
};

/**
 * @struct wifi_station_info_t
 * @brief Represents WiFi station information.
 *
 * Contains the BSSID (MAC address) and IPv4 address of a connected station.
 */
struct wifi_station_info_t {
    uint8_t bssid[6]; ///< MAC address of the access point or connected station
    uint32_t ip4; ///< IPv4 address

    wifi_station_info_t(uint8_t *_bssid, uint32_t _ip4) {
        memcpy(bssid, _bssid, 6);
        ip4 = _ip4;
    }
};

/**
 * Upgrade status enums
 */
typedef enum {
    UPGRADE_STATUS_FAILED = -1,
    UPGRADE_STATUS_SUCCESS,
    UPGRADE_STATUS_IGNORE, // No update available
    UPGRADE_STATUS_MAX
} upgrade_status_t;

/**
 * @struct task_t
 * @brief Represents a task for the scheduler.
 *
 * Contains task details such as ID, duration, priority, and callback function.
 */

#define DEFAULT_TASK_PRIORITY 0

typedef enum TaskMode { 
    TASK_MODE_INLINE = 0, 
    TASK_MODE_CONCURRENT 
} task_mode_t;

typedef enum TaskPolicy { 
    TASK_POLICY_FIFO = 0, ///< First-In-First-Out 
    TASK_POLICY_ROUNDROBIN, ///< Equal time slices 
    TASK_POLICY_DEADLINE, ///< Earliest deadline first 
    TASK_POLICY_FAIRSHARE ///< Balance across tasks 
} task_policy_t;

struct task_t {
    int _task_id; ///< Task ID
    int _max_attempts; ///< Maximum number of attempts
    uint64_t _duration; ///< Task duration in milliseconds
    uint64_t _last_millis; ///< Last execution timestamp
    CallBackVoidArgFn _task; ///< Task callback function
    int _task_priority; ///< Task priority (default is 0)
    uint64_t _task_exec_millis; ///< Task execution timestamp
    task_mode_t _task_mode; ///< Task mode
    task_policy_t _task_policy; ///< Task policy

    task_t() { clear(); }
    ~task_t() { clear(); }

    void clear() {
        _task_id = -1;
        _max_attempts = 0;
        _duration = 0;
        _last_millis = 0;
        _task = nullptr;
        _task_priority = 0;
        _task_exec_millis = 0;
        _task_mode = TASK_MODE_INLINE;
        _task_policy = TASK_POLICY_FIFO;
    }

    task_t(const task_t &t) {
        _task_id = t._task_id;
        _max_attempts = t._max_attempts;
        _duration = t._duration;
        _last_millis = t._last_millis;
        _task = t._task;
        _task_priority = t._task_priority;
        _task_exec_millis = t._task_exec_millis;
        _task_mode = t._task_mode;
        _task_policy = t._task_policy;
    }
};

/**
 * Terminal types
 */
typedef enum terminal_types {
    TERMINAL_TYPE_SERIAL = 0,
    TERMINAL_TYPE_TELNET,
    TERMINAL_TYPE_SSH,
    TERMINAL_TYPE_MAX
} terminal_types_t;

/* 
 * enumeration for special terminal input sequences 
 */
enum cmd_term_inseq_t {
    CMD_TERM_INSEQ_NONE = 0,
    CMD_TERM_INSEQ_ENTER,
    CMD_TERM_INSEQ_BACKSPACE_CHAR,
    CMD_TERM_INSEQ_DELETE_CHAR,
    CMD_TERM_INSEQ_DELETE,
    CMD_TERM_INSEQ_CTRL_C,
    CMD_TERM_INSEQ_CTRL_Z,
    CMD_TERM_INSEQ_ESC,
    CMD_TERM_INSEQ_UP_ARROW,
    CMD_TERM_INSEQ_DOWN_ARROW,
    CMD_TERM_INSEQ_RIGHT_ARROW,
    CMD_TERM_INSEQ_LEFT_ARROW,
    CMD_TERM_INSEQ_HOME,
    CMD_TERM_INSEQ_END,
    CMD_TERM_INSEQ_PAGE_UP,
    CMD_TERM_INSEQ_PAGE_DOWN,
    CMD_TERM_INSEQ_TAB,
    CMD_TERM_INSEQ_MAX
};

/* escape sequence start chars */
#define TERMINAL_ESCAPE_SEQ "\033["

/**
 * File info structure & definitions
 */
#define FILE_SEPARATOR "/"
#define FILE_NAME_MAX_SIZE 24

enum file_type_t {
    FILE_TYPE_REG = 0, ///< Regular file
    FILE_TYPE_DIR, ///< Directory
    FILE_TYPE_LINK, ///< Symbolic link
    FILE_TYPE_MAX
};

struct file_info_t {
    // Type of the file
    file_type_t type;

    // Size of the file, only valid for REG files.
    uint64_t size;

    // Name of the file stored as a null-terminated string. must be limited to
    // FILE_NAME_MAX_SIZE+1,
    char *name;
};

#endif
