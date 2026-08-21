/************************ Storage Lifecycle Tests *****************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Bringing the filesystem up more than once. A mount allocates caches inside
littlefs, so a second one has to release the first rather than orphan it.

The leak itself is caught by the sanitizer rather than by an assertion: it
surfaces as a failed run instead of a failed test, which is why these cases
exist at all — nothing else in the suite calls init twice.

Author          : Suraj I.
created Date    : 17th Aug 2026
******************************************************************************/

#include <MountedStack.h>
#include <pditest.h>

TEST(storagelife, the_filesystem_survives_being_initialised_again)
{
    pditest::rootFs();

    ASSERT_EQ(__i_rootfs.init(), PDI_OK);
    ASSERT_TRUE(__i_rootfs.isDirectory(FILE_SEPARATOR));
}

TEST(storagelife, repeated_initialisation_does_not_accumulate)
{
    pditest::rootFs();

    for (uint8_t round = 0; round < 5; round++)
    {
        ASSERT_EQ(__i_rootfs.init(), PDI_OK);
    }

    ASSERT_TRUE(__i_rootfs.isDirectory(FILE_SEPARATOR));
}

TEST(storagelife, content_written_before_a_remount_is_still_there_after_it)
{
    pditest::rootFs();

    const char *path = "/remount.txt";
    if (__i_rootfs.isFileExist(path))
    {
        __i_rootfs.deleteFile(path);
    }
    ASSERT_TRUE(__i_rootfs.createFile(path, "kept across a remount") >= 0);

    ASSERT_EQ(__i_rootfs.init(), PDI_OK);

    ASSERT_TRUE(__i_rootfs.isFileExist(path));

    pdiutil::string out;
    __i_rootfs.readFile(path, 64, [&out](char *chunk, uint32_t len) {
        out += pdiutil::string(chunk, len);
        return true;
    });
    ASSERT_STREQ(out.c_str(), "kept across a remount");

    __i_rootfs.deleteFile(path);
}

TEST(storagelife, the_filesystem_still_takes_writes_after_a_remount)
{
    pditest::rootFs();
    ASSERT_EQ(__i_rootfs.init(), PDI_OK);

    const char *path = "/afterremount.txt";
    if (__i_rootfs.isFileExist(path))
    {
        __i_rootfs.deleteFile(path);
    }

    ASSERT_TRUE(__i_rootfs.createFile(path, "written after") >= 0);
    ASSERT_TRUE(__i_rootfs.isFileExist(path));
    ASSERT_EQ(__i_rootfs.getFileSize(path), 13);

    __i_rootfs.deleteFile(path);
}
