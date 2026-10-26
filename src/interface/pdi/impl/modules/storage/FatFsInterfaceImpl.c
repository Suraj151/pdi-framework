/************************** FatFs Interface Impl *******************************
This file is part of the PDI Stack.

This is free software. You can redistribute it and/or modify it but without any
warranty.

Builds the vendored FatFs where the device does not already link it.

Author          : Suraj I.
Created Date    : 21st Oct 2026
******************************************************************************/

#include "../../../../../../devices/DeviceConfig.h"

#if defined(ENABLE_FAT_FS) && !defined(DEVICE_PROVIDES_FATFS)

#include "../../../../../../external/FatFsWrapper.c"

#endif
