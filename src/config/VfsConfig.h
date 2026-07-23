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
#define VFS_MAX_MOUNTS 5
#endif

#ifndef VFS_MOUNT_PREFIX_MAX
#define VFS_MOUNT_PREFIX_MAX 15
#endif

#ifndef VFS_MOUNT_NAME_MAX
#define VFS_MOUNT_NAME_MAX 11
#endif

// Per-backend enable flags. Each backend adds a mount slot at boot, so keep
// VFS_MAX_MOUNTS ≥ 1 + (number of enabled synthetic backends). Override to
// undef in DeviceConfig.h to opt out on tight-RAM ports.
#ifndef ENABLE_PROCFS
#define ENABLE_PROCFS
#endif

#ifndef ENABLE_SYSFS
#define ENABLE_SYSFS
#endif

#ifndef ENABLE_DEVFS
#define ENABLE_DEVFS
#endif

// RAM-backed scratch filesystem. Holds real file content in the heap, so keep
// it off (undef in DeviceConfig.h) on tight-RAM ports like Arduino UNO.
#ifndef ENABLE_TMPFS
#define ENABLE_TMPFS
#endif

// Byte count an unbounded devfs node (/dev/zero, /dev/random, /dev/urandom)
// yields per read call. MCU-safe cap so `cat /dev/zero` cannot spin forever.
#ifndef DEVFS_STREAM_READ_MAX
#define DEVFS_STREAM_READ_MAX 64
#endif

#endif
