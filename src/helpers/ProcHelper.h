/******************************** Proc helper *********************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 27th Aug 2026
******************************************************************************/

#ifndef _PROC_HELPER_H_
#define _PROC_HELPER_H_

#include <interface/pdi.h>
#include <config/Config.h>
#include <utility/Utility.h>

/* generated filesystem reading support functions */

/**
 * Hands every line of a file to the callback in turn, stopping where it answers
 * false; false when nothing is readable.
 */
bool readProcLines(const char *path, pdiutil::function<bool(pdiutil::string &)> online);

#ifdef ENABLE_CMD_SERVICE

/**
 * Writes the process table a session may see, filtered to one owner or to all.
 */
void printProcessTable(iTerminalInterface *terminal, uint8_t filter_owner = 0xFF);

#endif

#endif
