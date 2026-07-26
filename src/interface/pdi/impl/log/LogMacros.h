/****************************** Log Macros ***********************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 25th July 2026
******************************************************************************/

/*
 * Single home for every logging macro, shared by all device ports and the
 * whole service layer. Lives in the interface impl layer alongside LogManager
 * so both the device layer and the service layer can pull it in.
 *
 *   Log*            console (serial) only — gated by ENABLE_CONSOLE_LOG_*. One
 *                   variadic form: LogI("msg") or LogI("x=%d", n).
 *   SysLog*         console (gated) AND /var/log/syslog.<type> file — one
 *                   variadic form: SysLogI("msg") or SysLogI("x=%d", n). Active
 *                   when ENABLE_SYSLOG_SERVICE is defined, else degrades to the
 *                   console formatter.
 *
 * Both go through the formatter (printf rule: a no-args message must not
 * contain an unescaped '%').
 *
 * Requires the empty default macros (DataTypeDef.h) and RODT_ATTR (device
 * config) to be visible first. Pulled in by the service layer via
 * ServiceProvider.h and by the device layer via the device base header.
 *
 * No include guard on purpose — the `#if defined(LogX)` / `#undef` dance below
 * is idempotent, so re-including it after the console gates are known simply
 * upgrades the no-op defaults to the real macros.
 */

#include <interface/pdi/iLoggerInterface.h>
#include <interface/pdi/impl/log/LogManager.h>

/* -------------------------- console logging ----------------------------- */

#if defined(LogI) && ( defined(ENABLE_CONSOLE_LOG_INFO) || defined(ENABLE_CONSOLE_LOG_ALL) )
#undef LogI
#define LogI(f, args...) __log_manager.log(INFO_LOG, RODT_ATTR(f), ##args)
#endif

#if defined(LogE) && ( defined(ENABLE_CONSOLE_LOG_ERROR) || defined(ENABLE_CONSOLE_LOG_ALL) )
#undef LogE
#define LogE(f, args...) __log_manager.log(ERROR_LOG, RODT_ATTR(f), ##args)
#endif

#if defined(LogW) && ( defined(ENABLE_CONSOLE_LOG_WARNING) || defined(ENABLE_CONSOLE_LOG_ALL) )
#undef LogW
#define LogW(f, args...) __log_manager.log(WARNING_LOG, RODT_ATTR(f), ##args)
#endif

#if defined(LogS) && ( defined(ENABLE_CONSOLE_LOG_SUCCESS) || defined(ENABLE_CONSOLE_LOG_ALL) )
#undef LogS
#define LogS(f, args...) __log_manager.log(SUCCESS_LOG, RODT_ATTR(f), ##args)
#endif

/* -------------------------- syslog (file) ------------------------------- */

#ifdef SysLogI
#undef SysLogI
#undef SysLogE
#undef SysLogW
#undef SysLogS
#endif

#ifdef ENABLE_SYSLOG_SERVICE

// single variadic form — plain (no args) and formatted both route here
#define SysLogI(f, args...) __log_manager.syslog(INFO_LOG,    RODT_ATTR(f), ##args)
#define SysLogE(f, args...) __log_manager.syslog(ERROR_LOG,   RODT_ATTR(f), ##args)
#define SysLogW(f, args...) __log_manager.syslog(WARNING_LOG, RODT_ATTR(f), ##args)
#define SysLogS(f, args...) __log_manager.syslog(SUCCESS_LOG, RODT_ATTR(f), ##args)

#else

#define SysLogI(f, args...) LogI(f, ##args)
#define SysLogE(f, args...) LogE(f, ##args)
#define SysLogW(f, args...) LogW(f, ##args)
#define SysLogS(f, args...) LogS(f, ##args)

#endif
