/*********************** Navigation Command Tests *****************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Where the shell thinks it is, what it can see mounted, and the metadata it can
change on what it finds there.

Author          : Suraj I.
created Date    : 16th Aug 2026
******************************************************************************/

#include <ShellHarness.h>
#include <pditest.h>

using pditest::saw;

static void tidy(const char *path)
{
    if (__i_fs.isFileExist(path))
    {
        __i_fs.deleteFile(path);
    }
}

/* ------------------------------------------------------------------ cd, pwd */

TEST(cmdmeta, pwd_starts_at_the_root)
{
    pditest::Shell shell;
    ASSERT_TRUE(saw(shell.run("pwd"), "/"));
}

TEST(cmdmeta, cd_moves_into_a_directory)
{
    pditest::Shell shell;
    __i_fs.createDirectory("/w_cd");

    shell.run("cd /w_cd");
    ASSERT_TRUE(saw(shell.run("pwd"), "/w_cd"));

    shell.run("cd /");
    __i_fs.deleteDirectory("/w_cd");
}

TEST(cmdmeta, cd_into_a_missing_directory_leaves_us_where_we_were)
{
    pditest::Shell shell;
    __i_fs.createDirectory("/w_cdstay");
    shell.run("cd /w_cdstay");

    shell.run("cd /nosuchplace");
    ASSERT_TRUE(saw(shell.run("pwd"), "/w_cdstay"));

    shell.run("cd /");
    __i_fs.deleteDirectory("/w_cdstay");
}

TEST(cmdmeta, cd_to_a_relative_name_appends_to_the_current_directory)
{
    pditest::Shell shell;
    __i_fs.createDirectory("/w_rel");
    __i_fs.createDirectory("/w_rel/inner");

    shell.run("cd /w_rel");
    shell.run("cd inner");
    ASSERT_TRUE(saw(shell.run("pwd"), "/w_rel/inner"));

    shell.run("cd /");
    __i_fs.deleteDirectory("/w_rel/inner");
    __i_fs.deleteDirectory("/w_rel");
}

TEST(cmdmeta, cd_to_dot_dot_goes_back_up)
{
    pditest::Shell shell;
    __i_fs.createDirectory("/w_up");
    __i_fs.createDirectory("/w_up/deep");

    shell.run("cd /w_up/deep");
    shell.run("cd ..");
    ASSERT_TRUE(saw(shell.run("pwd"), "/w_up"));

    shell.run("cd /");
    __i_fs.deleteDirectory("/w_up/deep");
    __i_fs.deleteDirectory("/w_up");
}

TEST(cmdmeta, a_relative_file_resolves_against_the_current_directory)
{
    pditest::Shell shell;
    __i_fs.createDirectory("/w_resolve");
    shell.run("cd /w_resolve");

    shell.run("echo local file > here.txt");
    ASSERT_TRUE(__i_fs.isFileExist("/w_resolve/here.txt"));

    shell.run("cd /");
    tidy("/w_resolve/here.txt");
    __i_fs.deleteDirectory("/w_resolve");
}

/* --------------------------------------------------------------- df, mount */

TEST(cmdmeta, df_reports_every_mount_with_its_sizes)
{
    pditest::Shell shell;
    std::string out = shell.run("df");

    ASSERT_TRUE(saw(out, "rootfs"));
    ASSERT_TRUE(saw(out, "procfs"));
    ASSERT_TRUE(saw(out, "sysfs"));
    ASSERT_TRUE(saw(out, "devfs"));
    ASSERT_TRUE(saw(out, "tmpfs"));
    ASSERT_TRUE(saw(out, "TOTAL"));
    ASSERT_TRUE(saw(out, "FREE"));
}

TEST(cmdmeta, mount_lists_the_prefixes_and_types)
{
    pditest::Shell shell;
    std::string out = shell.run("mount");

    ASSERT_TRUE(saw(out, "/proc"));
    ASSERT_TRUE(saw(out, "/sys"));
    ASSERT_TRUE(saw(out, "/dev"));
    ASSERT_TRUE(saw(out, "/tmp"));
    ASSERT_TRUE(saw(out, "littlefs"));
}

TEST(cmdmeta, df_shows_the_root_filesystem_as_the_largest)
{
    pditest::Shell shell;
    ASSERT_TRUE(saw(shell.run("df"), "1048576"));
}

/* ------------------------------------------------------- chmod, chown, umask */

TEST(cmdmeta, chmod_changes_the_mode_bits)
{
    pditest::Shell shell;
    tidy("/w_chmod.txt");
    __i_fs.createFile("/w_chmod.txt", "moded");

    shell.run("chmod 640 /w_chmod.txt");

    file_info_t meta;
    ASSERT_EQ(__i_fs.getFileMeta("/w_chmod.txt", meta), (pdi_err_t)0);
    ASSERT_EQ(meta.m_perms, 0640);

    tidy("/w_chmod.txt");
}

TEST(cmdmeta, chmod_shows_the_new_mode_in_a_listing)
{
    pditest::Shell shell;
    __i_fs.createDirectory("/w_chmodls");
    __i_fs.createFile("/w_chmodls/f.txt", "x");
    shell.run("cd /w_chmodls");

    shell.run("chmod 400 f.txt");
    ASSERT_TRUE(saw(shell.run("ls"), "-r--"));

    shell.run("cd /");
    tidy("/w_chmodls/f.txt");
    __i_fs.deleteDirectory("/w_chmodls");
}

TEST(cmdmeta, chown_changes_the_owner)
{
    pditest::Shell shell;
    tidy("/w_chown.txt");
    __i_fs.createFile("/w_chown.txt", "owned");

    shell.run("chown 7 /w_chown.txt");

    file_info_t meta;
    ASSERT_EQ(__i_fs.getFileMeta("/w_chown.txt", meta), (pdi_err_t)0);
    ASSERT_EQ(meta.m_uid, 7);
    ASSERT_EQ(meta.m_gid, 7);

    tidy("/w_chown.txt");
}

TEST(cmdmeta, chown_takes_a_separate_group)
{
    pditest::Shell shell;
    tidy("/w_chowng.txt");
    __i_fs.createFile("/w_chowng.txt", "owned");

    shell.run("chown 7:9 /w_chowng.txt");

    file_info_t meta;
    ASSERT_EQ(__i_fs.getFileMeta("/w_chowng.txt", meta), (pdi_err_t)0);
    ASSERT_EQ(meta.m_uid, 7);
    ASSERT_EQ(meta.m_gid, 9);

    tidy("/w_chowng.txt");
}

TEST(cmdmeta, chown_is_refused_to_a_user_that_is_not_root)
{
    pditest::Shell shell(7, 7);
    tidy("/w_chownno.txt");
    __i_fs.createFile("/w_chownno.txt", "owned");
    __i_fs.setFileOwner("/w_chownno.txt", 7, 7);

    shell.run("chown 8 /w_chownno.txt");

    file_info_t meta;
    __i_fs.getFileMeta("/w_chownno.txt", meta);
    ASSERT_EQ(meta.m_uid, 7);

    tidy("/w_chownno.txt");
}

TEST(cmdmeta, umask_prints_the_current_value)
{
    pditest::Shell shell;
    ASSERT_TRUE(saw(shell.run("umask"), "0"));
}

TEST(cmdmeta, umask_takes_a_new_value_and_keeps_it)
{
    pditest::Shell shell;

    shell.run("umask 077");
    ASSERT_TRUE(saw(shell.run("umask"), "077"));
    ASSERT_EQ(SessionManager::getCurrentUmask(), 0077);
}

TEST(cmdmeta, the_umask_takes_bits_off_a_new_file)
{
    pditest::Shell shell;
    __i_fs.createDirectory("/w_umask");
    shell.run("cd /w_umask");
    shell.run("umask 077");

    shell.run("touch masked.txt");

    file_info_t meta;
    ASSERT_EQ(__i_fs.getFileMeta("/w_umask/masked.txt", meta), (pdi_err_t)0);
    ASSERT_EQ(meta.m_perms & 0077, 0);

    shell.run("umask 022");
    shell.run("cd /");
    tidy("/w_umask/masked.txt");
    __i_fs.deleteDirectory("/w_umask");
}

TEST(cmdmeta, one_session_umask_does_not_leak_into_the_next)
{
    {
        pditest::Shell first;
        first.run("umask 077");
        ASSERT_EQ(SessionManager::getCurrentUmask(), 0077);
    }

    pditest::Shell second;
    ASSERT_NE(SessionManager::getCurrentUmask(), 0077);
}
