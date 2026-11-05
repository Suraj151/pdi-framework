/**************************** Config File Tests *******************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Covers the /etc config surface: reading one option out of a file, persisting one
option without disturbing the rest of it, and keeping the file's permissions and
ownership across the rewrite.

Author          : Suraj I.
created Date    : 30th Aug 2026
******************************************************************************/

#include <MountedStack.h>
#include <pditest.h>

#include <helpers/ConfigHelper.h>

#ifdef ENABLE_STORAGE_SERVICE

namespace
{

const char *CONF_PATH = "/tmp/feature.conf";

void writeConf(VfsDispatcher *fs, const char *content)
{
    if (fs->isFileExist(CONF_PATH))
    {
        fs->deleteFile(CONF_PATH);
    }
    fs->createFile(CONF_PATH, content);
}

pdiutil::string slurpConf(VfsDispatcher *fs)
{
    pdiutil::string out;
    fs->readFile(CONF_PATH, 64, [&](char *data, uint32_t size) -> bool {
        out += pdiutil::string(data, size);
        return true;
    });
    return out;
}

} // namespace

namespace
{

const char *LONG_CONF_PATH = "/feature_long.conf";

/**
 * A config file whose middle line is longer than the heap will hold whole while
 * capped to a 2 KB block, between two ordinary options.
 */
pdiutil::string writeLongConf(VfsDispatcher *fs)
{
    pdiutil::string content = "port 2222\r\nnote ";
    content.append(6000, 'x');
    content += "\r\nname in memory\r\n";
    if (fs->isFileExist(LONG_CONF_PATH))
    {
        fs->deleteFile(LONG_CONF_PATH);
    }
    fs->createFile(LONG_CONF_PATH, content.c_str());
    return content;
}

pdiutil::string slurpLongConf(VfsDispatcher *fs)
{
    pdiutil::string out;
    fs->readFile(LONG_CONF_PATH, 256, [&](char *data, uint32_t size) -> bool {
        out.append(data, size);
        return true;
    });
    return out;
}

} // namespace

TEST(config, a_rewrite_that_cannot_hold_a_line_whole_leaves_the_file_as_it_was)
{
    VfsDispatcher *fs = pditest::mountedVfs();
    pdiutil::string before = writeLongConf(fs);

    pdiutil::enable_heap_check();
    __i_dvc_ctrl.capMaxFreeBlock(2048);
    bool set = setConfigValue(LONG_CONF_PATH, "port", "8022");
    __i_dvc_ctrl.capMaxFreeBlock(0);

    ASSERT_FALSE(set);
    ASSERT_TRUE(slurpLongConf(fs) == before);

    fs->deleteFile(LONG_CONF_PATH);
}

TEST(config, a_line_the_heap_cannot_hold_whole_is_not_read_as_an_option)
{
    VfsDispatcher *fs = pditest::mountedVfs();
    writeLongConf(fs);

    pdiutil::vector<config_kv_t> kvs;
    pdiutil::enable_heap_check();
    __i_dvc_ctrl.capMaxFreeBlock(2048);
    bool loaded = loadConfigFile(LONG_CONF_PATH, kvs);
    __i_dvc_ctrl.capMaxFreeBlock(0);

    ASSERT_TRUE(loaded);
    bool sawnote = false, sawname = false;
    for (size_t i = 0; i < kvs.size(); i++)
    {
        sawnote = sawnote || kvs[i].m_key == "note";
        sawname = sawname || kvs[i].m_key == "name";
    }
    ASSERT_FALSE(sawnote);
    ASSERT_TRUE(sawname);

    fs->deleteFile(LONG_CONF_PATH);
}

TEST(config, a_saved_file_is_written_whole_or_left_as_it_was)
{
    VfsDispatcher *fs = pditest::mountedVfs();
    writeConf(fs, "port 2222\r\n");

    pdiutil::vector<config_kv_t> kvs;
    config_kv_t kv;
    kv.m_key = "port";
    kv.m_value = "8022";
    kvs.push_back(kv);
    ASSERT_TRUE(saveConfigFile(CONF_PATH, kvs, nullptr, 0));
    ASSERT_STREQ(slurpConf(fs).c_str(), "port 8022\r\n");
    ASSERT_FALSE(fs->isFileExist((pdiutil::string(CONF_PATH) + CONFIG_TEMP_SUFFIX).c_str()));

    fs->deleteFile(CONF_PATH);
}

/* ------------------------------------------------------------------ read */

TEST(config, one_option_reads_without_the_rest_of_the_file)
{
    VfsDispatcher *fs = pditest::mountedVfs();
    writeConf(fs, "# a comment\r\nport 2222\r\nname in memory\r\n");

    pdiutil::string value;
    ASSERT_TRUE(getConfigValue(CONF_PATH, "name", value));
    ASSERT_STREQ(value.c_str(), "in memory");

    fs->deleteFile(CONF_PATH);
}

TEST(config, an_option_that_is_absent_is_reported_absent)
{
    VfsDispatcher *fs = pditest::mountedVfs();
    writeConf(fs, "port 2222\r\n");

    pdiutil::string value;
    ASSERT_FALSE(getConfigValue(CONF_PATH, "name", value));

    fs->deleteFile(CONF_PATH);
}

TEST(config, an_option_named_in_a_comment_is_not_the_option)
{
    VfsDispatcher *fs = pditest::mountedVfs();
    writeConf(fs, "# port 9999\r\nport 2222\r\n");

    pdiutil::string value;
    ASSERT_TRUE(getConfigValue(CONF_PATH, "port", value));
    ASSERT_STREQ(value.c_str(), "2222");

    fs->deleteFile(CONF_PATH);
}

TEST(config, an_option_given_with_no_value_is_still_present)
{
    VfsDispatcher *fs = pditest::mountedVfs();
    writeConf(fs, "port\r\n");

    pdiutil::string value;
    ASSERT_TRUE(getConfigValue(CONF_PATH, "port", value));
    ASSERT_TRUE(value.empty());

    fs->deleteFile(CONF_PATH);
}

/* ----------------------------------------------------------------- write */

TEST(config, setting_an_option_replaces_only_that_line)
{
    VfsDispatcher *fs = pditest::mountedVfs();
    writeConf(fs, "# a comment\r\nport 2222\r\nname in memory\r\n");

    ASSERT_TRUE(setConfigValue(CONF_PATH, "port", "8022"));
    ASSERT_STREQ(slurpConf(fs).c_str(), "# a comment\r\nport 8022\r\nname in memory\r\n");

    fs->deleteFile(CONF_PATH);
}

TEST(config, setting_an_option_that_is_absent_appends_it)
{
    VfsDispatcher *fs = pditest::mountedVfs();
    writeConf(fs, "port 2222\r\n");

    ASSERT_TRUE(setConfigValue(CONF_PATH, "enabled", "no"));
    ASSERT_STREQ(slurpConf(fs).c_str(), "port 2222\r\nenabled no\r\n");

    fs->deleteFile(CONF_PATH);
}

TEST(config, setting_an_option_on_a_missing_file_creates_it)
{
    VfsDispatcher *fs = pditest::mountedVfs();
    if (fs->isFileExist(CONF_PATH))
    {
        fs->deleteFile(CONF_PATH);
    }

    ASSERT_TRUE(setConfigValue(CONF_PATH, "enabled", "yes"));
    ASSERT_TRUE(fs->isFileExist(CONF_PATH));
    ASSERT_STREQ(slurpConf(fs).c_str(), "enabled yes\r\n");

    fs->deleteFile(CONF_PATH);
}

TEST(config, a_repeated_option_is_left_with_one_line)
{
    VfsDispatcher *fs = pditest::mountedVfs();
    writeConf(fs, "port 1111\r\nname keep\r\nport 2222\r\n");

    ASSERT_TRUE(setConfigValue(CONF_PATH, "port", "8022"));
    ASSERT_STREQ(slurpConf(fs).c_str(), "port 8022\r\nname keep\r\n");

    fs->deleteFile(CONF_PATH);
}

TEST(config, a_file_written_with_bare_newlines_is_rewritten_for_a_terminal)
{
    VfsDispatcher *fs = pditest::mountedVfs();
    writeConf(fs, "# a comment\nport 2222\n");

    ASSERT_TRUE(setConfigValue(CONF_PATH, "port", "8022"));
    ASSERT_STREQ(slurpConf(fs).c_str(), "# a comment\r\nport 8022\r\n");

    fs->deleteFile(CONF_PATH);
}

TEST(config, a_rewritten_file_keeps_its_permissions_and_owner)
{
    VfsDispatcher *fs = pditest::mountedVfs();
    writeConf(fs, "port 2222\r\n");

    fs->beginPrivileged();
    fs->setFilePermissions(CONF_PATH, 0600);
    fs->setFileOwner(CONF_PATH, 41, 42);
    fs->endPrivileged();

    ASSERT_TRUE(setConfigValue(CONF_PATH, "port", "8022"));

    file_info_t meta;
    fs->beginPrivileged();
    ASSERT_EQ(fs->getFileMeta(CONF_PATH, meta), (pdi_err_t)PDI_OK);
    fs->endPrivileged();

    ASSERT_EQ((uint32_t)meta.m_perms, 0600u);
    ASSERT_EQ((uint32_t)meta.m_uid, 41u);
    ASSERT_EQ((uint32_t)meta.m_gid, 42u);

    fs->beginPrivileged();
    fs->deleteFile(CONF_PATH);
    fs->endPrivileged();
}

TEST(config, a_rewrite_leaves_no_working_copy_behind)
{
    VfsDispatcher *fs = pditest::mountedVfs();
    writeConf(fs, "port 2222\r\n");

    pdiutil::string temppath = pdiutil::string(CONF_PATH) + CONFIG_TEMP_SUFFIX;
    ASSERT_TRUE(setConfigValue(CONF_PATH, "port", "8022"));
    ASSERT_FALSE(fs->isFileExist(temppath.c_str()));

    fs->deleteFile(CONF_PATH);
}

/* ---------------------------------------------------------------- create */

TEST(config, a_whole_file_writes_from_pairs)
{
    VfsDispatcher *fs = pditest::mountedVfs();
    if (fs->isFileExist(CONF_PATH))
    {
        fs->deleteFile(CONF_PATH);
    }

    pdiutil::vector<config_kv_t> kvs;
    config_kv_t kv;
    kv.m_key = "port";
    kv.m_value = "2222";
    kvs.push_back(kv);
    kv.m_key = "enabled";
    kv.m_value = "yes";
    kvs.push_back(kv);

    ASSERT_TRUE(saveConfigFile(CONF_PATH, kvs, "# header\r\n"));
    ASSERT_STREQ(slurpConf(fs).c_str(), "# header\r\nport 2222\r\nenabled yes\r\n");

    fs->deleteFile(CONF_PATH);
}

TEST(config, defaults_are_written_only_when_the_file_is_missing)
{
    VfsDispatcher *fs = pditest::mountedVfs();
    writeConf(fs, "port 8022\r\n");

    pdiutil::vector<config_kv_t> kvs;
    config_kv_t kv;
    kv.m_key = "port";
    kv.m_value = "2222";
    kvs.push_back(kv);

    ASSERT_TRUE(ensureConfigFile(CONF_PATH, kvs));
    ASSERT_STREQ(slurpConf(fs).c_str(), "port 8022\r\n");

    fs->deleteFile(CONF_PATH);
}

TEST(config, defaults_create_the_directory_they_need)
{
    VfsDispatcher *fs = pditest::mountedVfs();
    const char *path = "/tmp/feature/feature.conf";
    if (fs->isFileExist(path))
    {
        fs->deleteFile(path);
    }

    pdiutil::vector<config_kv_t> kvs;
    config_kv_t kv;
    kv.m_key = "enabled";
    kv.m_value = "yes";
    kvs.push_back(kv);

    ASSERT_TRUE(ensureConfigFile(path, kvs));
    ASSERT_TRUE(fs->isDirExist("/tmp/feature"));
    ASSERT_TRUE(fs->isFileExist(path));

    fs->deleteFile(path);
    fs->deleteDirectory("/tmp/feature");
}

/* ------------------------------------------------------------------ bool */

TEST(config, the_words_for_yes_all_read_true)
{
    ASSERT_TRUE(configValueAsBool("yes", false));
    ASSERT_TRUE(configValueAsBool("YES", false));
    ASSERT_TRUE(configValueAsBool("true", false));
    ASSERT_TRUE(configValueAsBool("On", false));
    ASSERT_TRUE(configValueAsBool("1", false));
}

TEST(config, the_words_for_no_all_read_false)
{
    ASSERT_FALSE(configValueAsBool("no", true));
    ASSERT_FALSE(configValueAsBool("NO", true));
    ASSERT_FALSE(configValueAsBool("false", true));
    ASSERT_FALSE(configValueAsBool("Off", true));
    ASSERT_FALSE(configValueAsBool("0", true));
}

TEST(config, a_word_that_means_nothing_keeps_the_default)
{
    ASSERT_TRUE(configValueAsBool("maybe", true));
    ASSERT_FALSE(configValueAsBool("maybe", false));
    ASSERT_TRUE(configValueAsBool("", true));
    ASSERT_FALSE(configValueAsBool("", false));
}

TEST(config, a_word_that_merely_starts_with_yes_is_not_a_yes)
{
    ASSERT_FALSE(configValueAsBool("yess", false));
    ASSERT_TRUE(configValueAsBool("noo", true));
}

#endif
