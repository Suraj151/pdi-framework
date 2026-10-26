/***************************** Storage Config page ****************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st June 2025
******************************************************************************/
#ifndef _STORAGE_CONFIG_H_
#define _STORAGE_CONFIG_H_

#include "Common.h"

/**
 * Storage configurations
 */

#ifndef DD_BLOCK_MAX
#define DD_BLOCK_MAX 4096                   /* largest block dd copies at once */
#endif

#define DD_BLOCK_DEFAULT 512                /* block dd copies when bs= is not given */

#ifndef LFS_BUSY_YIELD_MS
#define LFS_BUSY_YIELD_MS 50                /* longest the filesystem works the flash without giving the system its turn */
#endif
#ifndef LFS_DISK_VOLUMES_MAX
#define LFS_DISK_VOLUMES_MAX 1              /* littlefs volumes mounted from disks at once, each taking a mount slot */
#endif
#define LFS_DISK_BLOCK_SIZE 4096            /* bytes in one littlefs block on a disk */
#define LFS_DISK_IO_SIZE 512                /* littlefs reads and programs a disk one sector at a time */
#define LFS_DISK_LOOKAHEAD_SIZE 64          /* bytes of the free block map littlefs keeps for a disk */
#ifndef FSTAB_PATH
#define FSTAB_PATH "/etc/fstab"             /* the filesystems mounted when their disk comes and by mount -a */
#endif
#define FSTAB_TYPE_AUTO "auto"
#define FSTAB_OPTION_NOAUTO "noauto"
#define FSTAB_OPTION_NOFAIL "nofail"
#define FSTAB_OPTION_SEPARATOR ','
#define DD_NUMBER_DIGITS_MAX 9              /* digits a dd number may have */

#define DD_OPERAND_IF             "if"
#define DD_OPERAND_OF             "of"
#define DD_OPERAND_BS             "bs"
#define DD_OPERAND_COUNT          "count"
#define DD_OPERAND_SKIP           "skip"
#define DD_OPERAND_SEEK           "seek"


// enum StorageType {
//     STORAGE_TYPE_FLASH,
//     STORAGE_TYPE_SPIFFS,
//     STORAGE_TYPE_LITTLEFS,
//     STORAGE_TYPE_MAX
// };

// enum StorageAccessMode {
//     STORAGE_ACCESS_MODE_R,
//     STORAGE_ACCESS_MODE_W,
//     STORAGE_ACCESS_MODE_X,
//     STORAGE_ACCESS_MODE_MAX
// };


#endif
