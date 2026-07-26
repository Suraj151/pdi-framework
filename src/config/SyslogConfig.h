/******************************* Syslog Config *******************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 25th July 2026
******************************************************************************/
#ifndef _SYSLOG_CONFIG_H_
#define _SYSLOG_CONFIG_H_

#include "Common.h"

/**
 * syslog (file logging) configuration
 */

#ifndef SYSLOG_DIR
#define SYSLOG_DIR "/var/log"
#endif

// per-type file paths (files split by log level; the current logger carries no
// service/unit identity, so type is the only axis for now)
#ifndef SYSLOG_FILE_INFO
#define SYSLOG_FILE_INFO SYSLOG_DIR "/syslog.info"
#endif
#ifndef SYSLOG_FILE_ERROR
#define SYSLOG_FILE_ERROR SYSLOG_DIR "/syslog.error"
#endif
#ifndef SYSLOG_FILE_WARNING
#define SYSLOG_FILE_WARNING SYSLOG_DIR "/syslog.warning"
#endif
#ifndef SYSLOG_FILE_SUCCESS
#define SYSLOG_FILE_SUCCESS SYSLOG_DIR "/syslog.success"
#endif

// truncate-and-restart once a file crosses this many bytes
#ifndef SYSLOG_FILE_MAX_SIZE
#define SYSLOG_FILE_MAX_SIZE 8192
#endif

// max assembled length of a single syslog line (longer lines are truncated)
#ifndef SYSLOG_LINE_MAX
#define SYSLOG_LINE_MAX 200
#endif

#endif
