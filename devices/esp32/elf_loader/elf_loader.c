/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "elf_loader.h"

/* Include all source (.c) files in elf_loader */

#define TAG TAG_esp_elf_symbol
#include "esp_elf_symbol.c"
#undef TAG

#define TAG TAG_esp_elf
#include "esp_elf.c"
#undef TAG

#include "esp_elf_adapter.c"

#include "esp_all_symbol.c"

#if defined(CONFIG_IDF_TARGET_ARCH_XTENSA) || defined(__XTENSA__) || defined(__xtensa__)
#define TAG TAG_esp_elf_xtensa
#include "esp_elf_xtensa.c"
#undef TAG
#elif defined(CONFIG_IDF_TARGET_ARCH_RISCV) || defined(__RISCV__) || defined(__riscv__) || defined(__riscv)
#define TAG TAG_esp_elf_riscv
#include "esp_elf_riscv.c"
#undef TAG
#endif

#if defined(CONFIG_IDF_TARGET_ESP32S2) && defined(CONFIG_ELF_LOADER_LOAD_PSRAM)
#include "esp_elf_esp32s2.c"
#endif

#if defined(CONFIG_ELF_DYNAMIC_LOAD_SHARED_OBJECT)
#define TAG TAG_dlfcn
#include "dlfcn.c"
#undef TAG

#define TAG TAG_dlmod
#include "dlmod.c"
#undef TAG
#endif
