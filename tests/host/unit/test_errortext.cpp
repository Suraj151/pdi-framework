/****************************** Error Text Tests ******************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 5th Nov 2026
******************************************************************************/

#include <pditest.h>
#include <utility/ErrorText.h>

TEST(errortext, names_the_codes_a_file_command_meets)
{
    ASSERT_STREQ(ErrorToText(PDI_ERR_EXISTS), "File exists");
    ASSERT_STREQ(ErrorToText(PDI_ERR_PERM), "Permission denied");
    ASSERT_STREQ(ErrorToText(PDI_ERR_NOT_FOUND), "Not found");
    ASSERT_STREQ(ErrorToText(PDI_ERR_NO_SPACE), "No space left on device");
    ASSERT_STREQ(ErrorToText(STORAGE_ERROR_NOT_A_FILE), "Is a directory");
    ASSERT_STREQ(ErrorToText(STORAGE_ERROR_NOT_A_DIRECTORY), "Not a directory");
    ASSERT_STREQ(ErrorToText(STORAGE_ERROR_NOT_EMPTY), "Directory not empty");
    ASSERT_STREQ(ErrorToText(STORAGE_ERROR_NAME_TOO_LONG), "File name too long");
    ASSERT_STREQ(ErrorToText(STORAGE_ERROR_READ_ONLY), "Read-only file system");
}

TEST(errortext, names_the_littlefs_and_fat_codes_the_same_way)
{
    ASSERT_STREQ(ErrorToText(PDI_ERR_FROM_LFS(-2)), "No such file or directory");
    ASSERT_STREQ(ErrorToText(PDI_ERR_FROM_LFS(-21)), "Is a directory");
    ASSERT_STREQ(ErrorToText(PDI_ERR_FROM_LFS(-39)), "Directory not empty");
    ASSERT_STREQ(ErrorToText(PDI_ERR_FROM_LFS(-28)), "No space left on device");
    ASSERT_STREQ(ErrorToText(PDI_ERR_FROM_FAT(4)), "No such file or directory");
    ASSERT_STREQ(ErrorToText(PDI_ERR_FROM_FAT(8)), "File exists");
    ASSERT_STREQ(ErrorToText(PDI_ERR_FROM_FAT(10)), "Write protected");
}

TEST(errortext, a_code_without_words_gives_none)
{
    ASSERT_TRUE(nullptr == ErrorToText(PDI_OK));
    ASSERT_TRUE(nullptr == ErrorToText(1));
    ASSERT_TRUE(nullptr == ErrorToText(-32768));
    ASSERT_TRUE(nullptr == ErrorToText(INT32_MIN));
}
