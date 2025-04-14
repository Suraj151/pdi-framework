/***************************** LittleFS Wrapper *******************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 6th Apr 2025
******************************************************************************/

// #pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#define LFS_NAME_MAX 24
#define LFS_NO_DEBUG
#define LFS_NO_WARN
#define LFS_NO_ERROR

#include "littlefs/lfs_util.c"
#include "littlefs/lfs.c"
