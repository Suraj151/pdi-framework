/*************************** Session Config page ******************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 18th July 2026
******************************************************************************/
#ifndef _SESSION_CONFIG_H_
#define _SESSION_CONFIG_H_

#include "Common.h"
#include "TelnetConfig.h"
#include "SshConfig.h"

/**
 * Slots the remote transports can ask for. A pool larger than the table it
 * draws from starves whichever transport arrives second, so the two are sized
 * together rather than by coincidence.
 */
#ifdef ENABLE_TELNET_SERVICE
#define PDI_TELNET_SESSION_DEMAND TELNET_MAX_SESSIONS
#else
#define PDI_TELNET_SESSION_DEMAND 0
#endif

#ifdef ENABLE_SSH_SERVICE
#define PDI_SSH_SESSION_DEMAND SSH_MAX_SESSIONS
#else
#define PDI_SSH_SESSION_DEMAND 0
#endif

/**
 * One for the console the board always has, plus every slot a remote transport
 * can fill at once. A board with no network keeps a spare for the login that
 * arrives while a stale session is still being reclaimed.
 */
#define PDI_SESSION_DEMAND (1 + PDI_TELNET_SESSION_DEMAND + PDI_SSH_SESSION_DEMAND)

#ifndef PDI_MAX_SESSIONS
#if PDI_SESSION_DEMAND < 2
#define PDI_MAX_SESSIONS 2
#else
#define PDI_MAX_SESSIONS PDI_SESSION_DEMAND
#endif
#endif

#if PDI_MAX_SESSIONS < PDI_SESSION_DEMAND
#error "PDI_MAX_SESSIONS is smaller than the transports can fill; raise it or shrink TELNET_MAX_SESSIONS / SSH_MAX_SESSIONS"
#endif

/**
 * Bytes a pipe carries between two stages. A pipe is fixed capacity the way a
 * linux one is; a stage that outruns it marks the pipe overflowed rather than
 * growing without bound.
 */
#ifndef PDI_PIPE_CAPACITY
#define PDI_PIPE_CAPACITY 1024
#endif

#ifndef CMD_INTERRUPT_POLL_MS
#define CMD_INTERRUPT_POLL_MS 50 /* how often a command in a long read looks at its terminal for ctrl+c */
#endif

#ifndef CMD_TYPEAHEAD_MAX
#define CMD_TYPEAHEAD_MAX 128 /* keys a session keeps for the shell while a command looks for ctrl+c; more are dropped */
#endif

#define CMD_INTERRUPT_CHAR 0x03 /* the byte ctrl+c sends */

#define CMD_MSG_ROOT_REQUIRED "root required" /* what a command prints when only root may run it */

#define UNAME_SYSTEM_NAME "PDIStack" /* what uname prints as the system name */
#define UNAME_UNKNOWN "unknown" /* what uname prints for a field the device cannot report */
#define UNAME_FIELD_SYSTEM 0x01 /* uname -s */
#define UNAME_FIELD_HOST 0x02 /* uname -n */
#define UNAME_FIELD_RELEASE 0x04 /* uname -r */
#define UNAME_FIELD_VERSION 0x08 /* uname -v */
#define UNAME_FIELD_MACHINE 0x10 /* uname -m */
#define UNAME_FIELD_ALL 0x1F /* uname -a */

#ifndef SHELL_CAPTURE_MAX
#define SHELL_CAPTURE_MAX 256 /* bytes a $(...) may answer with; a longer answer fails the line rather than being cut */
#endif

#ifndef SHELL_CAPTURE_DEPTH
#define SHELL_CAPTURE_DEPTH 2 /* $(...) that may open inside another $(...) */
#endif

#ifndef SHELL_ARITH_DEPTH
#define SHELL_ARITH_DEPTH 8 /* parentheses that may nest inside one $((...)) */
#endif

/**
 * Bytes a file stream gathers before it touches storage. Writing through per
 * byte would cost one open and close per byte on the filesystem.
 */
#ifndef PDI_FILE_STREAM_BUFFER
#define PDI_FILE_STREAM_BUFFER 128
#endif

/**
 * Lines tail will hold back while a stream drains. A stream cannot be rewound,
 * so asking for more than this would buy nothing but the memory to hold it.
 */
#ifndef PDI_STREAM_TAIL_LINES_MAX
#define PDI_STREAM_TAIL_LINES_MAX 32
#endif

#ifndef ENV_FILE_PATH
#define ENV_FILE_PATH "/.env"
#endif

#ifndef ENV_NAME_MAX
#define ENV_NAME_MAX 32
#endif

#ifndef ENV_VALUE_MAX
#define ENV_VALUE_MAX 128
#endif

#ifndef ENV_SESSION_MAX
#define ENV_SESSION_MAX 8
#endif

#define ENV_KEY_HOME "HOME"
#define ENV_KEY_PWD "PWD"
#define ENV_KEY_USER "USER"
#define ENV_KEY_UID "UID"
#define ENV_KEY_HOSTNAME "HOSTNAME"

#ifndef RC_LOCAL_FILE_PATH
#define RC_LOCAL_FILE_PATH "/etc/rc.local"
#endif

/* nested scripts open at once, the top level one and what it sources */
#ifndef SCRIPT_MAX_DEPTH
#define SCRIPT_MAX_DEPTH 3
#endif

#define SCRIPT_COMMENT_CHAR LINE_COMMENT_CHAR
#define SCRIPT_UID_DIRECTIVE "UID"

#ifndef SCRIPT_BLOCK_MAX
#define SCRIPT_BLOCK_MAX 8
#endif

#ifndef SCRIPT_LOOP_MAX
#define SCRIPT_LOOP_MAX 1000
#endif

#define SCRIPT_KEYWORD_IF "if"
#define SCRIPT_KEYWORD_ELSE "else"
#define SCRIPT_KEYWORD_FI "fi"
#define SCRIPT_KEYWORD_WHILE "while"
#define SCRIPT_KEYWORD_DONE "done"
#define SCRIPT_KEYWORD_FOR "for"
#define SCRIPT_KEYWORD_IN "in"
#define SCRIPT_KEYWORD_ELIF "elif"
#define SCRIPT_KEYWORD_BREAK "break"
#define SCRIPT_KEYWORD_CONTINUE "continue"
#define SCRIPT_KEYWORD_EXIT "exit"

#endif
