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
#include "pdi_types.h"
#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>
#include <limits.h>
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
#ifndef RODT_ATTR
#define RODT_ATTR(x) x
#endif

#ifndef PROG_RODT_ATTR
#define PROG_RODT_ATTR
#endif

#ifndef PROG_RODT_PTR
#define PROG_RODT_PTR
#endif

// Attribute to be redefined while entering critical section
#ifndef CRITICAL_SECTION_ENTER
#define CRITICAL_SECTION_ENTER
#endif

#ifndef CRITICAL_SECTION_EXIT
#define CRITICAL_SECTION_EXIT
#endif

// String operations api for read-write-modify string data (can be redefined in derived interfaces)
// _ro for each of api referes to read-only data region specific api
#ifndef strcat
#define strcat strcat
#endif

#ifndef strcat_ro
#define strcat_ro strcat
#endif

#ifndef strncat
#define strncat strncat
#endif

#ifndef strncat_ro
#define strncat_ro strncat
#endif

#ifndef strcpy
#define strcpy strcpy
#endif

#ifndef strcpy_ro
#define strcpy_ro strcpy
#endif

#ifndef strncpy
#define strncpy strncpy
#endif

#ifndef strncpy_ro
#define strncpy_ro strncpy
#endif

#ifndef strlen
#define strlen strlen
#endif

#ifndef strlen_ro
#define strlen_ro strlen
#endif

#ifndef strcmp
#define strcmp strcmp
#endif

#ifndef strcmp_ro
#define strcmp_ro strcmp
#endif

#ifndef strncmp
#define strncmp strncmp
#endif

#ifndef strncmp_ro
#define strncmp_ro strncmp
#endif

#ifndef memcpy
#define memcpy memcpy
#endif

#ifndef memcpy_ro
#define memcpy_ro memcpy
#endif


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
enum conn_status : uint8_t {
    CONN_STATUS_IDLE = 0,
    CONN_STATUS_CONNECTED,
    CONN_STATUS_CONNECTION_FAILED,
    CONN_STATUS_CONNECTION_LOST,
    CONN_STATUS_CONFIG_ERROR,
    CONN_STATUS_NOT_AVAILABLE,
    CONN_STATUS_DISCONNECTED,
    CONN_STATUS_MAX
};
typedef enum conn_status conn_status_t;

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
typedef pdiutil::function<void *(void *)> CallBackVoidPointerArgVoidPointerRetFn;
typedef pdiutil::function<bool(const uint8_t *, uint32_t)> CallBackBytesArgBoolRetFn;

/**
 * Logging types
 */
enum logger_type : uint8_t {
    INFO_LOG,
    ERROR_LOG,
    WARNING_LOG,
    SUCCESS_LOG
};
typedef enum logger_type logger_type_t;

/**
 * IP address types
 */
enum ip_addr_type : uint8_t {
    IP_ADDR_TYPE_V4 = 4,  /** IPv4 */
    IP_ADDR_TYPE_V6 = 6,  /** IPv6 */
    IP_ADDR_TYPE_ANY = 46 /** IPv4+IPv6 ("dual-stack") */
};

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

    explicit ipaddress_t(const char* str) : type(IP_ADDR_TYPE_V4) {
        ip4[0] = ip4[1] = ip4[2] = ip4[3] = 0;
        if (!str) return;

        uint8_t octets[4] = {0,0,0,0};
        uint8_t oct_i = 0;
        uint32_t cur = 0;
        bool seen_digit = false;
        bool valid = true;
        for (uint16_t k = 0; ; ++k) {
            char c = str[k];
            if (c == '.' || c == 0) {
                if (!seen_digit || cur > 255 || oct_i >= 4) { valid = false; break; }
                octets[oct_i++] = (uint8_t)cur;
                cur = 0;
                seen_digit = false;
                if (c == 0) break;
            } else if (c >= '0' && c <= '9') {
                cur = cur * 10 + (uint32_t)(c - '0');
                seen_digit = true;
            } else {
                valid = false;
                break;
            }
        }
        if (valid && oct_i == 4) {
            ip4[0] = octets[0];
            ip4[1] = octets[1];
            ip4[2] = octets[2];
            ip4[3] = octets[3];
        }
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
enum upgrade_status : int8_t {
    UPGRADE_STATUS_FAILED = -1,
    UPGRADE_STATUS_SUCCESS,
    UPGRADE_STATUS_IGNORE, // No update available
    UPGRADE_STATUS_MAX
};
typedef enum upgrade_status upgrade_status_t;

/**
 * @struct task_t
 * @brief Represents a task for the scheduler.
 *
 * Contains task details such as ID, duration, priority, and callback function.
 */

#define DEFAULT_TASK_PRIORITY 0

#ifdef ENABLE_CONTEXTUAL_EXECUTION
// forward declaration
class iExecutive;
#endif

enum TaskMode : uint8_t {
    TASK_MODE_INLINE = 0,
#ifdef ENABLE_CONTEXTUAL_EXECUTION
    TASK_MODE_COOPERATIVE,
    TASK_MODE_PREEMPTIVE,
#endif
    TASK_MODE_MAX
};
typedef enum TaskMode task_mode_t;

enum TaskPolicy : uint8_t {
    TASK_POLICY_FIFO = 0, ///< First-In-First-Out
    TASK_POLICY_ROUNDROBIN, ///< Equal time slices
    TASK_POLICY_DEADLINE, ///< Earliest deadline first
    TASK_POLICY_FAIRSHARE ///< Balance across tasks
};
typedef enum TaskPolicy task_policy_t;

enum TaskState : uint8_t {
    TASK_STATE_READY = 0,   ///< Registered, waiting for its next dispatch
    TASK_STATE_RUNNING,     ///< Currently executing on the scheduler
    TASK_STATE_SLEEPING,    ///< Waiting for its next due tick (default idle)
    TASK_STATE_STOPPED,     ///< Suspended (SIGSTOP analogue); skipped by scheduler
    TASK_STATE_ZOMBIE       ///< Callback done, awaiting reap by remove_expired_tasks
};
typedef enum TaskState task_state_t;

/**
 * POSIX-style signal numbers delivered to tasks via TaskScheduler::sendSignal.
 * Only SIG_KILL and SIG_TERM are honored today (both reap the task); SIG_STOP /
 * SIG_CONT / SIG_HUP reserved for future steps.
 */
enum Signal : uint8_t {
    SIG_NONE = 0,
    SIG_HUP  = 1,   ///< reload/hangup
    SIG_KILL = 9,   ///< uncatchable; reap immediately
    SIG_TERM = 15,  ///< default `kill`; polite reap (invokes handler if set)
    SIG_CONT = 18,  ///< resume a stopped task
    SIG_STOP = 19   ///< suspend a running task
};
typedef enum Signal signal_t;

struct task_t {
    pdiutil::task_id_t m_task_id;               ///< Task ID (-1 = invalid, acts as PID)
    pdiutil::attempts_t m_max_attempts;         ///< Maximum number of attempts (-1 = unlimited, 0 = expired)
    pdiutil::millis_t m_duration;               ///< Task duration in milliseconds
    pdiutil::millis_t m_last_millis;            ///< Last execution timestamp
    CallBackVoidArgFn m_task;                   ///< Task callback function
    pdiutil::task_priority_t m_task_priority;   ///< Task priority (default is 0)
    uint32_t m_task_exec_us;                ///< Last execution duration in µs
    task_mode_t m_task_mode;                    ///< Task mode
    task_policy_t m_task_policy;                ///< Task policy
    const char* m_name;                         ///< Human-readable name (RO ptr; nullptr = anonymous)
    uint8_t m_owner;                            ///< Owning session id (0 = kernel)
    task_state_t m_state;                       ///< Current lifecycle state
    int8_t m_nice;                              ///< POSIX-style nice value (-20..19), folded into priority
    pdiutil::millis_t m_created_ms;             ///< Timestamp of registration
    uint32_t m_run_count;                       ///< Times the callback has fired
    uint64_t m_total_exec_us;                   ///< Cumulative execution time (µs) since registration
    uint8_t m_pending_sig;                      ///< Pending signal number to consume on next tick (SIG_NONE = idle)
    #ifdef ENABLE_CONTEXTUAL_EXECUTION
    iExecutive* m_task_exec = nullptr;          ///< Task executive
    #endif

    task_t() { clear(); }
    ~task_t() { clear(); }

    void clear() {
        m_task_id = -1;
        m_max_attempts = 0;
        m_duration = 0;
        m_last_millis = 0;
        m_task = nullptr;
        m_task_priority = 0;
        m_task_exec_us = 0;
        m_task_mode = TASK_MODE_INLINE;
        m_task_policy = TASK_POLICY_FIFO;
        m_name = nullptr;
        m_owner = 0;
        m_state = TASK_STATE_READY;
        m_nice = 0;
        m_created_ms = 0;
        m_run_count = 0;
        m_total_exec_us = 0;
        m_pending_sig = SIG_NONE;
        // #ifdef ENABLE_CONTEXTUAL_EXECUTION
        // if(m_task_exec != nullptr){
        //     delete m_task_exec;
        // }
        // m_task_exec = nullptr;
        // #endif
    }

    // Copy Constructor
    task_t(const task_t &t) {
        m_task_id = t.m_task_id;
        m_max_attempts = t.m_max_attempts;
        m_duration = t.m_duration;
        m_last_millis = t.m_last_millis;
        m_task = t.m_task;
        m_task_priority = t.m_task_priority;
        m_task_exec_us = t.m_task_exec_us;
        m_task_mode = t.m_task_mode;
        m_task_policy = t.m_task_policy;
        m_name = t.m_name;
        m_owner = t.m_owner;
        m_state = t.m_state;
        m_nice = t.m_nice;
        m_created_ms = t.m_created_ms;
        m_run_count = t.m_run_count;
        m_total_exec_us = t.m_total_exec_us;
        m_pending_sig = t.m_pending_sig;
        #ifdef ENABLE_CONTEXTUAL_EXECUTION
        m_task_exec = t.m_task_exec;
        #endif
    }

    // Assignment Operator
    task_t& operator=(const task_t &t){

        if (this != &t) {

            m_task_id = t.m_task_id;
            m_max_attempts = t.m_max_attempts;
            m_duration = t.m_duration;
            m_last_millis = t.m_last_millis;
            m_task = t.m_task;
            m_task_priority = t.m_task_priority;
            m_task_exec_us = t.m_task_exec_us;
            m_task_mode = t.m_task_mode;
            m_task_policy = t.m_task_policy;
            m_name = t.m_name;
            m_owner = t.m_owner;
            m_state = t.m_state;
            m_nice = t.m_nice;
            m_created_ms = t.m_created_ms;
            m_run_count = t.m_run_count;
            m_total_exec_us = t.m_total_exec_us;
            m_pending_sig = t.m_pending_sig;
            #ifdef ENABLE_CONTEXTUAL_EXECUTION
            m_task_exec = t.m_task_exec;
            #endif
        }
        return *this;
    }
};

/**
 * Terminal types
 */
enum terminal_types : uint8_t {
    TERMINAL_TYPE_SERIAL = 0,
    TERMINAL_TYPE_TELNET,
    TERMINAL_TYPE_SSH,
    TERMINAL_TYPE_MAX
};
typedef enum terminal_types terminal_types_t;

/* 
 * enumeration for special terminal input sequences 
 */
enum cmd_term_inseq_t : uint8_t {
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

enum file_type_t : uint8_t {
    FILE_TYPE_REG = 0, ///< Regular file
    FILE_TYPE_DIR, ///< Directory
    FILE_TYPE_LINK, ///< Symbolic link
    FILE_TYPE_MAX
};

// Reserved custom-attribute IDs for PDI file metadata. Values 0 and >=
// FILE_ATTR_USER_BASE are free for callers of setFileAttr to define their own.
enum file_attr_id_t : uint8_t {
    FILE_ATTR_CTIME = 1,     ///< uint32_t seconds since Unix epoch (created)
    FILE_ATTR_MTIME = 2,     ///< uint32_t seconds since Unix epoch (modified)
    FILE_ATTR_PERMS = 3,     ///< uint16_t POSIX-style permission bit mask
    FILE_ATTR_USER_BASE = 16 ///< first attribute id available to user code
};

// Default POSIX-style permission bits assigned to newly created entries.
#define FILE_PERM_DEFAULT_FILE  0644
#define FILE_PERM_DEFAULT_DIR   0755

struct file_info_t {
    // Type of the file
    file_type_t type;

    // Size of the file, only valid for REG files.
    uint64_t size;

    // Name of the file stored as a null-terminated string. must be limited to
    // FILE_NAME_MAX_SIZE+1,
    char *name;

    // Seconds since Unix epoch when the entry was created. 0 means unknown
    // (time source was unavailable when the entry was made).
    uint32_t ctime;

    // Seconds since Unix epoch of the last content modification. 0 means unknown.
    uint32_t mtime;

    // POSIX-style permission bits (advisory only; not enforced by the FS).
    uint16_t perms;
};

#ifdef ENABLE_AUTH_SERVICE
struct user_record_t {
    user_record_t() : m_uid(0), m_gid(0) {}

    void clear() {
        m_uid = 0;
        m_gid = 0;
        m_username.clear();
        m_home.clear();
        m_shell.clear();
    }

    uint16_t m_uid;
    uint16_t m_gid;
    pdiutil::string m_username;
    pdiutil::string m_home;
    pdiutil::string m_shell;
};
#endif

class iTerminalInterface;

enum session_state_t : uint8_t {
    SESSION_STATE_FREE = 0,
    SESSION_STATE_PRELOGIN,
    SESSION_STATE_LOGIN_WAIT,
    SESSION_STATE_INTERACTIVE,
    SESSION_STATE_DEAD
};

struct session_t {
    session_t() : m_sid(0), m_state(SESSION_STATE_FREE), m_terminal(nullptr), m_loginAt(0), m_lastActivityAt(0),
                  m_cursor(0),
#ifdef ENABLE_STORAGE_SERVICE
                  m_historyIdx(-1), m_prevHistorySize(0), m_prevArgSize(0),
#endif
                  m_autoCompleteIdx(-1), m_prevCmdSize(0) {}

    void clear() {
        m_sid = 0;
        m_state = SESSION_STATE_FREE;
        m_terminal = nullptr;
        m_loginAt = 0;
        m_lastActivityAt = 0;
        m_linebuf.clear();
        m_cursor = 0;
#ifdef ENABLE_STORAGE_SERVICE
        m_historyIdx = -1;
        m_prevHistorySize = 0;
        m_origTypedPrefix.clear();
        m_prevArgSize = 0;
        m_cwd.clear();
        m_lastCwd.clear();
#endif
#ifdef ENABLE_AUTH_SERVICE
        m_isAuthorized = false;
        m_username.clear();
#endif
        m_autoCompleteIdx = -1;
        m_prevCmdSize = 0;
    }

    uint8_t m_sid;
    session_state_t m_state;
    iTerminalInterface *m_terminal;
    uint32_t m_loginAt;
    uint32_t m_lastActivityAt;

    pdiutil::string m_linebuf;
    uint16_t m_cursor;
#ifdef ENABLE_STORAGE_SERVICE
    int16_t m_historyIdx;
    int16_t m_prevHistorySize;
    pdiutil::string m_origTypedPrefix;
    int16_t m_prevArgSize;
    pdiutil::string m_cwd;
    pdiutil::string m_lastCwd;
#endif
#ifdef ENABLE_AUTH_SERVICE
    bool m_isAuthorized;
    pdiutil::string m_username;
#endif
    int16_t m_autoCompleteIdx;
    int16_t m_prevCmdSize;
};

#endif
