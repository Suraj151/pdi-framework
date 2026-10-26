/************************** FatFs Unicode Tests ********************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Covers the code conversions FatFs is built with: each one answers exactly what
the vendored fatfs/ffunicode.c answers, for every character it can be asked.

Author          : Suraj I.
created Date    : 24th Oct 2026
******************************************************************************/

#include <config/Config.h>

#ifdef ENABLE_FAT_FS

#include <pditest.h>

extern "C" {
#define ff_uni2oem vendored_uni2oem
#define ff_oem2uni vendored_oem2uni
#define ff_wtoupper vendored_wtoupper
#include <external/fatfs/ffunicode.c>
#undef ff_uni2oem
#undef ff_oem2uni
#undef ff_wtoupper

WCHAR ff_uni2oem(DWORD uni, WORD cp);
WCHAR ff_oem2uni(WCHAR oem, WORD cp);
DWORD ff_wtoupper(DWORD uni);
}

#if FF_USE_LFN != 0 && FF_CODE_PAGE == 437

TEST(fatfs_unicode, upper_case_matches_the_vendored_table_for_every_character)
{
    uint32_t differ = 0;
    for (DWORD c = 0; c <= 0x10000; c++)
    {
        differ += (ff_wtoupper(c) != vendored_wtoupper(c)) ? 1 : 0;
    }
    ASSERT_EQ((int)differ, 0);
    ASSERT_EQ((int)ff_wtoupper('a'), (int)'A');
    ASSERT_EQ((int)ff_wtoupper(0x00E9), 0x00C9);
}

TEST(fatfs_unicode, code_page_437_matches_the_vendored_table_both_ways)
{
    uint32_t differ = 0;
    for (DWORD c = 0; c <= 0x10000; c++)
    {
        differ += (ff_uni2oem(c, 437) != vendored_uni2oem(c, 437)) ? 1 : 0;
        differ += (ff_uni2oem(c, 850) != vendored_uni2oem(c, 850)) ? 1 : 0;
        differ += (ff_oem2uni((WCHAR)c, 437) != vendored_oem2uni((WCHAR)c, 437)) ? 1 : 0;
        differ += (ff_oem2uni((WCHAR)c, 850) != vendored_oem2uni((WCHAR)c, 850)) ? 1 : 0;
    }
    ASSERT_EQ((int)differ, 0);
    ASSERT_EQ((int)ff_uni2oem(0x00E9, 437), 0x82);
    ASSERT_EQ((int)ff_oem2uni(0x82, 437), 0x00E9);
}

#endif

#endif
