/****************************** PDI Semantic Types ****************************
This file is part of the PDI stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Central semantic typedefs mapped onto stdint fixed-width primitives.
Kept minimal and dependency-free so it can be included anywhere.

Author          : Suraj I.
Created Date    : 16th July 2026
******************************************************************************/

#ifndef _PDI_TYPES_H_
#define _PDI_TYPES_H_

#include <stdint.h>

#ifdef __cplusplus
namespace pdiutil {

    // Time
    using millis_t     = uint64_t; ///< millisecond timestamp / duration
    using epoch_time_t = int64_t;  ///< seconds since Unix epoch

    // Task scheduler
    using task_id_t       = int16_t; ///< task identity, -1 = invalid
    using task_priority_t = uint8_t; ///< task priority, 0 = lowest
    using attempts_t      = int16_t; ///< remaining attempts, -1 = unlimited, 0 = expired

    // Network
    using net_port_t = uint16_t; ///< TCP / UDP port number

} // namespace pdiutil
#endif

/**
 * PDI framework error codes.
 *
 * Convention: 0 = success, negative = error. Functions returning a count or
 * size keep that value on success and return one of these codes on failure,
 * so existing `< 0` checks stay valid.
 *
 * Codes are grouped into fixed-width bands by section, so any code is traceable
 * to its origin via PDI_ERR_SECTION(code). Generic codes (band 0) are shared by
 * every section; PDI_ERR_FAILURE keeps the historical -1 so bare `-1` returns
 * stay valid until upgraded to a specific code.
 */
#define PDI_ERR_BAND_SIZE 300

#define PDI_ERRBASE_GENERIC 0
#define PDI_ERRBASE_IO      (-300)
#define PDI_ERRBASE_NET     (-600)
#define PDI_ERRBASE_HTTP    (-900)
#define PDI_ERRBASE_MQTT    (-1200)
#define PDI_ERRBASE_SMTP    (-1500)
#define PDI_ERRBASE_STORAGE (-1800)
#define PDI_ERRBASE_SSH     (-2100)
#define PDI_ERRBASE_AUTH    (-2400)
#define PDI_ERRBASE_DB      (-2700)
#define PDI_ERRBASE_CRYPTO  (-3000)
#define PDI_ERRBASE_TASK    (-3300)
#define PDI_ERRBASE_CMD     (-3600)
#define PDI_ERRBASE_WIFI    (-3900)
// Foreign lower-framework passthrough bands (preserve exact underlying code)
#define PDI_ERRBASE_NET_LWIP    (-4200)
#define PDI_ERRBASE_FS_LITTLEFS (-4500)

// Section index of a code (0 = generic); index * PDI_ERR_BAND_SIZE is its base
#define PDI_ERR_SECTION(code) ((int16_t)((-(int32_t)(code)) / PDI_ERR_BAND_SIZE))

// Foreign lower-framework error passthrough: preserve the EXACT underlying code
// offset from a per-framework base, so both the origin and the original code
// stay traceable (decode: original = code - base). Never collapse/hide a foreign
// framework error into a single generic code — encode it here instead.
#define PDI_ERR_FROM_LWIP(e) ((int16_t)(PDI_ERRBASE_NET_LWIP + (int16_t)(e)))
#define PDI_ERR_FROM_LFS(e)  ((int16_t)(PDI_ERRBASE_FS_LITTLEFS + (int16_t)(e)))

#ifdef __cplusplus
enum pdi_err : int16_t {
#else
enum pdi_err {
#endif

    PDI_OK = 0,

    /* Generic errors (band 0), shared by every section */
    PDI_ERR_FAILURE         = -1,
    PDI_ERR_INVALID_ARG     = -2,
    PDI_ERR_NULL_PTR        = -3,
    PDI_ERR_NO_MEM          = -4,
    PDI_ERR_TIMEOUT         = -5,
    PDI_ERR_NOT_FOUND       = -6,
    PDI_ERR_NOT_SUPPORTED   = -7,
    PDI_ERR_NOT_IMPLEMENTED = -8,
    PDI_ERR_NOT_INITIALIZED = -9,
    PDI_ERR_BUSY            = -10,
    PDI_ERR_AGAIN           = -11,
    PDI_ERR_PERM            = -12,
    PDI_ERR_EXISTS          = -13,
    PDI_ERR_NO_SPACE        = -14,
    PDI_ERR_IO              = -15,
    PDI_ERR_OVERFLOW        = -16,
    PDI_ERR_UNDERFLOW       = -17,
    PDI_ERR_RANGE           = -18,
    PDI_ERR_STATE           = -19,
    PDI_ERR_CORRUPT         = -20,
    PDI_ERR_CRC             = -21,
    PDI_ERR_ABORTED         = -22,
    PDI_ERR_CLOSED          = -23,
    PDI_ERR_EOF             = -24,
    PDI_ERR_FULL            = -25,
    PDI_ERR_EMPTY           = -26,
    PDI_ERR_GENERIC_MAX     = -299,

    /* Storage / filesystem / VFS errors (band -1800..-2099) */
    STORAGE_ERROR_BAD_PATH        = PDI_ERRBASE_STORAGE - 1,
    STORAGE_ERROR_NAME_TOO_LONG   = PDI_ERRBASE_STORAGE - 2,
    STORAGE_ERROR_NO_PARENT       = PDI_ERRBASE_STORAGE - 3,
    STORAGE_ERROR_NOT_A_FILE      = PDI_ERRBASE_STORAGE - 4,
    STORAGE_ERROR_NOT_A_DIRECTORY = PDI_ERRBASE_STORAGE - 5,
    STORAGE_ERROR_READ_ONLY       = PDI_ERRBASE_STORAGE - 6,
    STORAGE_ERROR_NODE_LIMIT      = PDI_ERRBASE_STORAGE - 7,
    STORAGE_ERROR_ATTR_NOT_FOUND  = PDI_ERRBASE_STORAGE - 8,
    STORAGE_ERROR_NOT_MOUNTED     = PDI_ERRBASE_STORAGE - 9,
    STORAGE_ERROR_BACKEND         = PDI_ERRBASE_STORAGE - 10,
    STORAGE_ERROR_MAX             = PDI_ERRBASE_STORAGE - 299,

    /* Network / TCP / UDP / TLS errors (band -600..-899) */
    NET_ERROR_CONNECTION_FAILED    = PDI_ERRBASE_NET - 1,
    NET_ERROR_DNS_FAILED           = PDI_ERRBASE_NET - 2,
    NET_ERROR_TLS_HANDSHAKE_FAILED = PDI_ERRBASE_NET - 3,
    NET_ERROR_MAX                  = PDI_ERRBASE_NET - 299,

    /* HTTP transport errors (band -900..-1199) */
    HTTP_ERROR_CONNECTION_FAILED  = PDI_ERRBASE_HTTP - 1,
    HTTP_ERROR_UNEXPECTED_STATUS  = PDI_ERRBASE_HTTP - 2,
    HTTP_ERROR_MAX                = PDI_ERRBASE_HTTP - 299,

    /* MQTT band (PDI_ERRBASE_MQTT, -1200..-1499) reserved: current MQTT
       errors are generic (null/overflow/invalid-arg); add MQTT_ERROR_* here. */

    /* SSH transport errors (band -2100..-2399) */
    SSH_ERROR_MAC_FAILED          = PDI_ERRBASE_SSH - 1,
    SSH_ERROR_MAX                 = PDI_ERRBASE_SSH - 299,

    /* Task scheduler / Threading errors (band -3300..-3599) */
    TASK_ERROR_INVALID_MODE        = PDI_ERRBASE_TASK - 1,
    TASK_ERROR_CREATION_FAILED     = PDI_ERRBASE_TASK - 2,
    TASK_ERROR_MAX                 = PDI_ERRBASE_TASK - 299,

    /* lwIP passthrough (band -4200..-4499): value = PDI_ERR_FROM_LWIP(err_t),
       i.e. PDI_ERRBASE_NET_LWIP + err (lwIP err_t is 0..-16). Origin + exact
       code recoverable: err = code - PDI_ERRBASE_NET_LWIP. */
    NET_LWIP_ERROR_MAX             = PDI_ERRBASE_NET_LWIP - 299,

    /* LittleFS passthrough (band -4500..-4799): value = PDI_ERR_FROM_LFS(lfs_err),
       i.e. PDI_ERRBASE_FS_LITTLEFS + err (LittleFS LFS_ERR_* is 0..-84). Origin +
       exact code recoverable: err = code - PDI_ERRBASE_FS_LITTLEFS. */
    FS_LITTLEFS_ERROR_MAX          = PDI_ERRBASE_FS_LITTLEFS - 299
};

// Return type for status-returning functions. A plain signed integer (not the
// enum) so `return 0;` and forwarding an int-returning call stay well-formed;
// the pdi_err enumerators above convert to it implicitly.
typedef int16_t pdi_err_t;

#endif // _PDI_TYPES_H_
