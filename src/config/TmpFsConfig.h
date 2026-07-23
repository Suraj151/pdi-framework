/***************************** TmpFS Config page ******************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 23rd July 2026
******************************************************************************/
#ifndef _TMPFS_CONFIG_H_
#define _TMPFS_CONFIG_H_

#include "Common.h"

// Total content bytes a single tmpfs instance may hold across all its files.
#ifndef TMPFS_MAX_BYTES
#define TMPFS_MAX_BYTES 4096
#endif

// Maximum number of nodes (files + directories) in a single tmpfs instance.
#ifndef TMPFS_MAX_NODES
#define TMPFS_MAX_NODES 24
#endif

// Longest mount-relative path a tmpfs node may have.
#ifndef TMPFS_MAX_PATH
#define TMPFS_MAX_PATH 63
#endif

#endif
