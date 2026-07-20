/****************************** VFS Config page *******************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 20th July 2026
******************************************************************************/
#ifndef _VFS_CONFIG_H_
#define _VFS_CONFIG_H_

#include "Common.h"

#ifndef VFS_MAX_MOUNTS
#define VFS_MAX_MOUNTS 3
#endif

#ifndef VFS_MOUNT_PREFIX_MAX
#define VFS_MOUNT_PREFIX_MAX 15
#endif

#ifndef VFS_MOUNT_NAME_MAX
#define VFS_MOUNT_NAME_MAX 11
#endif

#endif
