/***************************** FatFs Config page ******************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Holds plain defines only, because the FatFs C sources read it as well.

Author          : Suraj I.
created Date    : 21st Oct 2026
******************************************************************************/
#ifndef _FATFS_CONFIG_H_
#define _FATFS_CONFIG_H_

/* FAT volumes mounted at once, each taking a mount slot */
#ifndef FAT_VOLUMES_MAX
#define FAT_VOLUMES_MAX 2
#endif

/* permission bits every entry on a FAT volume shows, which keeps no owner or mode */
#define FAT_PERM_DEFAULT 0755

/* the write bits a read-only entry loses */
#define FAT_PERM_WRITE_BITS 0222

/* sectors from which mkfs makes FAT32, 512 MiB */
#define FAT_FORMAT_FAT32_SECTORS 1048576

/* the volume number and colon FatFs puts before every path */
#define FAT_DRIVE_PREFIX_SIZE 2

#endif
