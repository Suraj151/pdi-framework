/******************************* Config helper *******************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st Aug 2026
******************************************************************************/

#ifndef _CONFIG_HELPER_H_
#define _CONFIG_HELPER_H_

#include <interface/pdi.h>
#include <config/Config.h>
#include <utility/Utility.h>

/* generic config file support functions */

bool loadConfigFile(const char *path, pdiutil::vector<config_kv_t> &out);

#endif
