/**************************** ELF Loader Config *******************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 30th July 2026
******************************************************************************/

#ifndef _ESP32_ELF_LOADER_CONFIG_H_
#define _ESP32_ELF_LOADER_CONFIG_H_

/**
 * Plain ESP32 (Xtensa, no PSRAM): load ELF sections into internal RAM and run
 * .text through the D/IRAM bus mirror. PSRAM and MMU remap paths stay off.
 */
#define CONFIG_ELF_LOADER_BUS_ADDRESS_MIRROR 1
#define CONFIG_ELF_FILE_SYSTEM_BASE_PATH     "/storage"
#define CONFIG_ELF_LOADER_NUMBER_SYMBOLS     8

/**
 * Symbol tables the loaded app resolves its externals against: libc
 * (printf/memcpy/sleep...), esp-idf helpers, and the customer table reserved
 * for PDI-exported functions.
 */
#define CONFIG_ELF_LOADER_LIBC_SYMBOLS
#define CONFIG_ELF_LOADER_ESPIDF_SYMBOLS
#define CONFIG_ELF_LOADER_CUSTOMER_SYMBOLS

#endif // _ESP32_ELF_LOADER_CONFIG_H_
