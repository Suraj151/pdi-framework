/**************************** File System Tests *******************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Runs the real LittleFS over the emulated flash the mock device provides, so the
storage stack under test is the one that ships.

Author          : Suraj I.
created Date    : 16th Aug 2026
******************************************************************************/

#include <interface/pdi.h>
#include <MountedStack.h>
#include <pditest.h>

static FileSystemInterface *mountedFs()
{
    return pditest::rootFs();
}

static void removeIfPresent(FileSystemInterface *fs, const char *path)
{
    if (fs->isFileExist(path))
    {
        fs->deleteFile(path);
    }
}

/**
 * Collect a whole file into a string through the read callback.
 */
static pdiutil::string slurp(FileSystemInterface *fs, const char *path)
{
    pdiutil::string out;
    int64_t size = fs->getFileSize(path);
    if (size <= 0)
    {
        return out;
    }

    fs->readFile(path, (uint64_t)size, [&out](char *chunk, uint32_t len) {
        for (uint32_t i = 0; i < len; i++)
        {
            out += chunk[i];
        }
        return true;
    });

    return out;
}

TEST(storage, reports_its_size)
{
    ASSERT_EQ(__i_storage.size(), (uint64_t)MOCKDEVICE_STORAGE_SIZE);
}

TEST(storage, an_erased_block_reads_as_all_ones)
{
    __i_storage.erase(0, MOCKDEVICE_STORAGE_BLOCK_SIZE);

    uint8_t buf[16];
    ASSERT_EQ(__i_storage.read(0, buf, sizeof(buf)), (int64_t)sizeof(buf));
    for (size_t i = 0; i < sizeof(buf); i++)
    {
        ASSERT_EQ(buf[i], (uint8_t)0xFF);
    }
}

TEST(storage, a_program_after_erase_stores_the_bytes)
{
    const uint8_t payload[4] = {0x01, 0x02, 0x03, 0x04};
    uint8_t readback[4] = {0};

    __i_storage.erase(0, MOCKDEVICE_STORAGE_BLOCK_SIZE);
    ASSERT_EQ(__i_storage.write(0, payload, sizeof(payload)), (int64_t)sizeof(payload));
    ASSERT_EQ(__i_storage.read(0, readback, sizeof(readback)), (int64_t)sizeof(readback));
    ASSERT_MEMEQ(readback, payload, sizeof(payload));
}

/**
 * Flash only clears bits, so rewriting without an erase yields the and of the
 * two values. Anything layered on top has to erase first.
 */
TEST(storage, a_program_without_erase_only_clears_bits)
{
    const uint8_t first[1] = {0xF0};
    const uint8_t second[1] = {0x0F};
    uint8_t readback[1] = {0};

    __i_storage.erase(0, MOCKDEVICE_STORAGE_BLOCK_SIZE);
    __i_storage.write(0, first, 1);
    __i_storage.write(0, second, 1);
    __i_storage.read(0, readback, 1);

    ASSERT_EQ(readback[0], (uint8_t)0x00);
}

TEST(storage, erase_rejects_an_unaligned_range)
{
    ASSERT_FALSE(__i_storage.erase(1, MOCKDEVICE_STORAGE_BLOCK_SIZE));
    ASSERT_FALSE(__i_storage.erase(0, 100));
}

TEST(storage, access_past_the_end_is_refused)
{
    uint8_t buf[8];
    ASSERT_EQ(__i_storage.read(MOCKDEVICE_STORAGE_SIZE, buf, sizeof(buf)), (int64_t)-1);
    ASSERT_EQ(__i_storage.write(MOCKDEVICE_STORAGE_SIZE, buf, sizeof(buf)), (int64_t)-1);
    ASSERT_FALSE(__i_storage.erase(MOCKDEVICE_STORAGE_SIZE, MOCKDEVICE_STORAGE_BLOCK_SIZE));
}

TEST(filesystem, mounts)
{
    ASSERT_NOT_NULL(mountedFs());
}

TEST(filesystem, reports_a_total_size)
{
    FileSystemInterface *fs = mountedFs();
    ASSERT_NOT_NULL(fs);
    ASSERT_GT(fs->getTotalSize(), (uint64_t)0);
}

TEST(filesystem, free_size_never_exceeds_total)
{
    FileSystemInterface *fs = mountedFs();
    ASSERT_NOT_NULL(fs);
    ASSERT_LE(fs->getFreeSize(), fs->getTotalSize());
}

TEST(filesystem, creates_and_reads_back_a_file)
{
    FileSystemInterface *fs = mountedFs();
    ASSERT_NOT_NULL(fs);

    removeIfPresent(fs, "/hello.txt");
    ASSERT_GT(fs->createFile("/hello.txt", "hello world"), 0);
    ASSERT_TRUE(fs->isFileExist("/hello.txt"));
    ASSERT_STREQ(slurp(fs, "/hello.txt").c_str(), "hello world");
}

TEST(filesystem, reports_the_file_size)
{
    FileSystemInterface *fs = mountedFs();
    ASSERT_NOT_NULL(fs);

    removeIfPresent(fs, "/sized.txt");
    fs->createFile("/sized.txt", "0123456789");
    ASSERT_EQ(fs->getFileSize("/sized.txt"), (int64_t)10);
}

TEST(filesystem, refuses_to_create_a_file_that_exists)
{
    FileSystemInterface *fs = mountedFs();
    ASSERT_NOT_NULL(fs);

    removeIfPresent(fs, "/once.txt");
    ASSERT_GT(fs->createFile("/once.txt", "first"), 0);
    ASSERT_LT(fs->createFile("/once.txt", "second"), 0);
}

TEST(filesystem, writes_over_an_existing_file)
{
    FileSystemInterface *fs = mountedFs();
    ASSERT_NOT_NULL(fs);

    removeIfPresent(fs, "/over.txt");
    fs->createFile("/over.txt", "original");
    fs->writeFile("/over.txt", "replaced", 8, false);

    ASSERT_STREQ(slurp(fs, "/over.txt").c_str(), "replaced");
}

TEST(filesystem, appends_to_an_existing_file)
{
    FileSystemInterface *fs = mountedFs();
    ASSERT_NOT_NULL(fs);

    removeIfPresent(fs, "/append.txt");
    fs->createFile("/append.txt", "head");
    fs->writeFile("/append.txt", "-tail", 5, true);

    ASSERT_STREQ(slurp(fs, "/append.txt").c_str(), "head-tail");
}

TEST(filesystem, stores_binary_content_including_nul)
{
    FileSystemInterface *fs = mountedFs();
    ASSERT_NOT_NULL(fs);

    const char payload[] = {'a', '\0', 'b', (char)0xFF, 'c'};
    removeIfPresent(fs, "/binary.bin");
    ASSERT_GT(fs->createFile("/binary.bin", payload, sizeof(payload)), 0);
    ASSERT_EQ(fs->getFileSize("/binary.bin"), (int64_t)sizeof(payload));

    pdiutil::string back = slurp(fs, "/binary.bin");
    ASSERT_EQ(back.size(), sizeof(payload));
    ASSERT_EQ(back[1], '\0');
    ASSERT_EQ((uint8_t)back[3], (uint8_t)0xFF);
}

/**
 * A block of an uploaded file can begin with a nul as readily as with any
 * other byte, so the size given to the write is the only thing that says how
 * much to store.
 */
TEST(filesystem, appends_a_block_that_begins_with_nul)
{
    FileSystemInterface *fs = mountedFs();
    ASSERT_NOT_NULL(fs);

    const char head[] = {'h', 'i'};
    const char tail[] = {'\0', '\1', 'z'};

    removeIfPresent(fs, "/nulhead.bin");
    ASSERT_GT(fs->writeFile("/nulhead.bin", head, sizeof(head), true), 0);
    ASSERT_GT(fs->writeFile("/nulhead.bin", tail, sizeof(tail), true), 0);

    ASSERT_EQ(fs->getFileSize("/nulhead.bin"), (int64_t)(sizeof(head) + sizeof(tail)));

    pdiutil::string back = slurp(fs, "/nulhead.bin");
    ASSERT_EQ(back.size(), sizeof(head) + sizeof(tail));
    ASSERT_EQ(back[2], '\0');
    ASSERT_EQ(back[4], 'z');
}

TEST(filesystem, deletes_a_file)
{
    FileSystemInterface *fs = mountedFs();
    ASSERT_NOT_NULL(fs);

    removeIfPresent(fs, "/gone.txt");
    fs->createFile("/gone.txt", "temporary");
    ASSERT_TRUE(fs->isFileExist("/gone.txt"));

    ASSERT_EQ(fs->deleteFile("/gone.txt"), PDI_OK);
    ASSERT_FALSE(fs->isFileExist("/gone.txt"));
}

TEST(filesystem, reports_a_missing_file_as_absent)
{
    FileSystemInterface *fs = mountedFs();
    ASSERT_NOT_NULL(fs);
    ASSERT_FALSE(fs->isFileExist("/definitely-not-here.txt"));
}

TEST(filesystem, renames_a_file)
{
    FileSystemInterface *fs = mountedFs();
    ASSERT_NOT_NULL(fs);

    removeIfPresent(fs, "/before.txt");
    removeIfPresent(fs, "/after.txt");
    fs->createFile("/before.txt", "content");

    ASSERT_EQ(fs->rename("/before.txt", "/after.txt"), PDI_OK);
    ASSERT_FALSE(fs->isFileExist("/before.txt"));
    ASSERT_STREQ(slurp(fs, "/after.txt").c_str(), "content");
}

TEST(filesystem, copies_a_file)
{
    FileSystemInterface *fs = mountedFs();
    ASSERT_NOT_NULL(fs);

    removeIfPresent(fs, "/source.txt");
    removeIfPresent(fs, "/copy.txt");
    fs->createFile("/source.txt", "duplicate me");

    ASSERT_EQ(fs->copyFile("/source.txt", "/copy.txt"), PDI_OK);
    ASSERT_STREQ(slurp(fs, "/source.txt").c_str(), "duplicate me");
    ASSERT_STREQ(slurp(fs, "/copy.txt").c_str(), "duplicate me");
}

TEST(filesystem, creates_and_removes_a_directory)
{
    FileSystemInterface *fs = mountedFs();
    ASSERT_NOT_NULL(fs);

    if (fs->isDirExist("/adir"))
    {
        fs->deleteDirectory("/adir");
    }

    ASSERT_EQ(fs->createDirectory("/adir"), PDI_OK);
    ASSERT_TRUE(fs->isDirExist("/adir"));
    ASSERT_TRUE(fs->isDirectory("/adir"));

    ASSERT_EQ(fs->deleteDirectory("/adir"), PDI_OK);
    ASSERT_FALSE(fs->isDirExist("/adir"));
}

TEST(filesystem, a_file_is_not_a_directory)
{
    FileSystemInterface *fs = mountedFs();
    ASSERT_NOT_NULL(fs);

    removeIfPresent(fs, "/plain.txt");
    fs->createFile("/plain.txt", "x");
    ASSERT_FALSE(fs->isDirectory("/plain.txt"));
}

TEST(filesystem, lists_directory_contents)
{
    FileSystemInterface *fs = mountedFs();
    ASSERT_NOT_NULL(fs);

    if (!fs->isDirExist("/listing"))
    {
        fs->createDirectory("/listing");
    }
    removeIfPresent(fs, "/listing/one.txt");
    removeIfPresent(fs, "/listing/two.txt");
    fs->createFile("/listing/one.txt", "1");
    fs->createFile("/listing/two.txt", "2");

    pdiutil::vector<file_info_t> items;
    int result = fs->getDirFileList("/listing", items);

    // a listing carries "." and ".." alongside the real entries
    size_t count = items.size();
    bool sawone = false;
    bool sawtwo = false;
    for (size_t i = 0; i < items.size(); i++)
    {
        if (0 == strcmp(items[i].m_name, "one.txt"))
        {
            sawone = true;
        }
        if (0 == strcmp(items[i].m_name, "two.txt"))
        {
            sawtwo = true;
        }
        // the listing hands each name over to the caller, the way the ls
        // command takes ownership of it
        pdiutil::safe_delete_array(items[i].m_name);
    }

    ASSERT_GE(result, 0);
    ASSERT_EQ(count, (size_t)4);
    ASSERT_TRUE(sawone);
    ASSERT_TRUE(sawtwo);
}

TEST(filesystem, a_listing_pattern_narrows_the_results)
{
    FileSystemInterface *fs = mountedFs();
    ASSERT_NOT_NULL(fs);

    if (!fs->isDirExist("/filtered"))
    {
        fs->createDirectory("/filtered");
    }
    removeIfPresent(fs, "/filtered/keep.txt");
    removeIfPresent(fs, "/filtered/drop.txt");
    fs->createFile("/filtered/keep.txt", "1");
    fs->createFile("/filtered/drop.txt", "2");

    pdiutil::vector<file_info_t> items;
    fs->getDirFileList("/filtered", items, "keep");
    size_t count = items.size();

    for (size_t i = 0; i < items.size(); i++)
    {
        pdiutil::safe_delete_array(items[i].m_name);
    }

    ASSERT_EQ(count, (size_t)1);
}

TEST(filesystem, a_file_written_in_a_directory_reads_back)
{
    FileSystemInterface *fs = mountedFs();
    ASSERT_NOT_NULL(fs);

    if (!fs->isDirExist("/nested"))
    {
        fs->createDirectory("/nested");
    }
    removeIfPresent(fs, "/nested/deep.txt");
    fs->createFile("/nested/deep.txt", "down here");

    ASSERT_STREQ(slurp(fs, "/nested/deep.txt").c_str(), "down here");
}

TEST(filesystem, survives_many_rewrites_of_the_same_file)
{
    FileSystemInterface *fs = mountedFs();
    ASSERT_NOT_NULL(fs);

    removeIfPresent(fs, "/churn.txt");
    fs->createFile("/churn.txt", "0");

    for (int i = 1; i <= 50; i++)
    {
        char content[16];
        __snprintf(content, sizeof(content), "value-%d", i);
        fs->writeFile("/churn.txt", content, (uint32_t)strlen(content), false);
    }

    ASSERT_STREQ(slurp(fs, "/churn.txt").c_str(), "value-50");
}

TEST(filesystem, permissions_round_trip)
{
    FileSystemInterface *fs = mountedFs();
    ASSERT_NOT_NULL(fs);

    removeIfPresent(fs, "/perm.txt");
    fs->createFile("/perm.txt", "x");
    ASSERT_GE(fs->setFilePermissions("/perm.txt", 0640), 0);

    file_info_t meta;
    ASSERT_EQ(fs->getFileMeta("/perm.txt", meta), PDI_OK);
    ASSERT_EQ(meta.m_perms, (uint16_t)0640);
}

TEST(filesystem, ownership_round_trips)
{
    FileSystemInterface *fs = mountedFs();
    ASSERT_NOT_NULL(fs);

    removeIfPresent(fs, "/owned.txt");
    fs->createFile("/owned.txt", "x");
    ASSERT_GE(fs->setFileOwner("/owned.txt", 1000, 1001), 0);

    file_info_t meta;
    ASSERT_EQ(fs->getFileMeta("/owned.txt", meta), PDI_OK);
    ASSERT_EQ(meta.m_uid, (uint16_t)1000);
    ASSERT_EQ(meta.m_gid, (uint16_t)1001);
}

TEST(filesystem, touch_creates_a_missing_file)
{
    FileSystemInterface *fs = mountedFs();
    ASSERT_NOT_NULL(fs);

    removeIfPresent(fs, "/touched.txt");
    ASSERT_EQ(fs->touch("/touched.txt"), PDI_OK);
    ASSERT_TRUE(fs->isFileExist("/touched.txt"));
}

TEST(filesystem, finds_a_string_in_a_file)
{
    FileSystemInterface *fs = mountedFs();
    ASSERT_NOT_NULL(fs);

    removeIfPresent(fs, "/search.txt");
    fs->createFile("/search.txt", "alpha beta gamma beta");

    pdiutil::vector<uint32_t> hits;
    ASSERT_GE(fs->findInFile("/search.txt", "beta", &hits), 0);
    ASSERT_EQ(hits.size(), (size_t)2);
}

/**
 * Line numbers start at zero.
 */
TEST(filesystem, reads_a_numbered_line)
{
    FileSystemInterface *fs = mountedFs();
    ASSERT_NOT_NULL(fs);

    removeIfPresent(fs, "/lines.txt");
    fs->createFile("/lines.txt", "first\nsecond\nthird\n");

    pdiutil::string line;
    ASSERT_GE(fs->readLineInFile("/lines.txt", 0, line), 0);
    ASSERT_STREQ(line.c_str(), "first");

    ASSERT_GE(fs->readLineInFile("/lines.txt", 1, line), 0);
    ASSERT_STREQ(line.c_str(), "second");

    ASSERT_GE(fs->readLineInFile("/lines.txt", 2, line), 0);
    ASSERT_STREQ(line.c_str(), "third");
}

TEST(filesystem, counts_the_lines_in_a_file)
{
    FileSystemInterface *fs = mountedFs();
    ASSERT_NOT_NULL(fs);

    removeIfPresent(fs, "/counted.txt");
    fs->createFile("/counted.txt", "a\nb\nc\n");

    pdiutil::vector<uint32_t> indices;
    ASSERT_GE(fs->getLineNumbersInFile("/counted.txt", indices), 0);
    ASSERT_GT(indices.size(), (size_t)0);
}

/**
 * LFS_NAME_MAX sizes struct lfs_info, and lfs_dir_read memsets that struct by
 * whatever size its own translation unit sees. LittleFSWrapper.c sets the value
 * before including lfs.c, and LittleFSWrapper.h sets it for everyone else, so
 * the two have to agree. littlefs refuses a name longer than its limit, which
 * makes the value it actually compiled with observable from out here.
 */
TEST(filesystem, honours_the_configured_name_limit)
{
    FileSystemInterface *fs = mountedFs();
    ASSERT_NOT_NULL(fs);

    char atlimit[FILE_NAME_MAX_SIZE + 2];
    atlimit[0] = '/';
    memset(atlimit + 1, 'a', FILE_NAME_MAX_SIZE);
    atlimit[FILE_NAME_MAX_SIZE + 1] = '\0';

    removeIfPresent(fs, atlimit);
    ASSERT_GT(fs->createFile(atlimit, "fits"), 0);
    ASSERT_TRUE(fs->isFileExist(atlimit));
    fs->deleteFile(atlimit);
}

TEST(filesystem, refuses_a_name_past_the_configured_limit)
{
    FileSystemInterface *fs = mountedFs();
    ASSERT_NOT_NULL(fs);

    char toolong[FILE_NAME_MAX_SIZE + 10];
    toolong[0] = '/';
    memset(toolong + 1, 'b', FILE_NAME_MAX_SIZE + 8);
    toolong[FILE_NAME_MAX_SIZE + 9] = '\0';

    removeIfPresent(fs, toolong);
    ASSERT_LT(fs->createFile(toolong, "too long"), 0);
}

TEST(filesystem, basename_strips_the_directory)
{
    FileSystemInterface *fs = mountedFs();
    ASSERT_NOT_NULL(fs);
    ASSERT_STREQ(fs->basename("/a/b/c.txt").c_str(), "c.txt");
}

TEST(filesystem, a_separator_is_appended_to_a_path_that_lacks_one)
{
    FileSystemInterface *fs = mountedFs();
    ASSERT_NOT_NULL(fs);

    pdiutil::string path = "/a/b";
    fs->appendFileSeparator(path);
    ASSERT_STREQ(path.c_str(), "/a/b/");
}

TEST(filesystem, a_path_that_already_ends_in_a_separator_is_left_alone)
{
    FileSystemInterface *fs = mountedFs();
    ASSERT_NOT_NULL(fs);

    pdiutil::string path = "/a/b/";
    fs->appendFileSeparator(path);
    ASSERT_STREQ(path.c_str(), "/a/b/");
}

/**
 * An empty path is the root of the storage browser: the portal strips its own
 * route off the location it was posted, and at the top of the tree nothing is
 * left. Reading the last character of it reads one byte before the buffer.
 */
TEST(filesystem, an_empty_path_becomes_the_root)
{
    FileSystemInterface *fs = mountedFs();
    ASSERT_NOT_NULL(fs);

    pdiutil::string path;
    fs->appendFileSeparator(path);
    ASSERT_STREQ(path.c_str(), "/");
}

TEST(filesystem, an_empty_buffer_becomes_the_root)
{
    FileSystemInterface *fs = mountedFs();
    ASSERT_NOT_NULL(fs);

    char path[8] = {0};
    fs->appendFileSeparator(path);
    ASSERT_STREQ(path, "/");
}

/**
 * The mime type is read from the extension of a file that exists. A path that
 * is not on the filesystem reports no type at all.
 */
TEST(filesystem, mime_type_follows_the_extension)
{
    FileSystemInterface *fs = mountedFs();
    ASSERT_NOT_NULL(fs);

    removeIfPresent(fs, "/page.html");
    removeIfPresent(fs, "/style.css");
    removeIfPresent(fs, "/data.json");
    fs->createFile("/page.html", "<html>");
    fs->createFile("/style.css", "body{}");
    fs->createFile("/data.json", "{}");

    ASSERT_EQ(fs->getFileMimeType(pdiutil::string("/page.html")), MIME_TYPE_TEXT_HTML);
    ASSERT_EQ(fs->getFileMimeType(pdiutil::string("/style.css")), MIME_TYPE_TEXT_CSS);
    ASSERT_EQ(fs->getFileMimeType(pdiutil::string("/data.json")), MIME_TYPE_APPLICATION_JSON);
}

TEST(filesystem, mime_type_of_a_missing_file_is_unknown)
{
    FileSystemInterface *fs = mountedFs();
    ASSERT_NOT_NULL(fs);

    removeIfPresent(fs, "/absent.html");
    ASSERT_EQ(fs->getFileMimeType(pdiutil::string("/absent.html")), MIME_TYPE_MAX);
}

TEST(filesystem, mime_type_of_a_file_without_an_extension_is_unknown)
{
    FileSystemInterface *fs = mountedFs();
    ASSERT_NOT_NULL(fs);

    removeIfPresent(fs, "/noext");
    fs->createFile("/noext", "x");
    ASSERT_EQ(fs->getFileMimeType(pdiutil::string("/noext")), MIME_TYPE_MAX);
}
