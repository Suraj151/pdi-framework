/************************ Directory Write Permission ***************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Creating, removing, renaming or moving an entry changes the directory that
holds it, so a user who may not write that directory may do none of them,
whatever the entry's own mode says. Reading is untouched.

Author          : Suraj I.
created Date    : 22nd Oct 2026
******************************************************************************/

#include <ShellHarness.h>
#include <service_provider/user/UserStoreService.h>
#include <service_provider/network/SyslogServiceProvider.h>
#include <pditest.h>

static const uint16_t s_uid = 2100;
static const char s_user[] = "dirperm";
static const char s_pass[] = "dirpermpw";

static void makeUser()
{
    pditest::mountedVfs();
    pditest::seedRootAccount();

    __i_fs.beginPrivileged();
    __user_store_service.removeUser(s_user);
    __i_fs.endPrivileged();

    user_record_t record;
    record.m_username = s_user;
    record.m_uid = s_uid;
    record.m_gid = s_uid;
    record.m_home = "/";
    record.m_shell = "/bin/sh";
    __user_store_service.addUser(record, s_pass);
}

static void forget()
{
    __i_fs.beginPrivileged();
    __user_store_service.removeUser(s_user);
    __i_fs.endPrivileged();
}

static void rootFile(const char *path, uint16_t perms)
{
    __i_fs.beginPrivileged();
    if (__i_fs.isFileExist(path)) __i_fs.deleteFile(path);
    __i_fs.createFile(path, "kept\n");
    __i_fs.setFilePermissions(path, perms);
    __i_fs.setFileOwner(path, USER_STORE_ROOT_UID, USER_STORE_ROOT_GID);
    __i_fs.endPrivileged();
}

static void rootDir(const char *path, uint16_t perms, uint16_t uid)
{
    __i_fs.beginPrivileged();
    if (__i_fs.isDirExist(path)) __i_fs.deleteDirectory(path);
    __i_fs.createDirectory(path);
    __i_fs.setFilePermissions(path, perms);
    __i_fs.setFileOwner(path, uid, uid);
    __i_fs.endPrivileged();
}

static void drop(const char *path)
{
    __i_fs.beginPrivileged();
    if (__i_fs.isDirExist(path)) __i_fs.deleteDirectory(path);
    else if (__i_fs.isFileExist(path)) __i_fs.deleteFile(path);
    __i_fs.endPrivileged();
}

static bool exists(const char *path)
{
    __i_fs.beginPrivileged();
    bool there = __i_fs.isFileExist(path) || __i_fs.isDirExist(path);
    __i_fs.endPrivileged();
    return there;
}

/* ---------------------------------------------------------- rename, move */

TEST(dirperm, a_user_cannot_move_the_shadow_file_out_of_etc)
{
    makeUser();
    pditest::Shell shell;
    shell.run("su dirperm dirpermpw");
    ASSERT_EQ(SessionManager::getCurrentUid(), s_uid);

    ASSERT_EQ(__i_fs.moveFile(USER_STORE_SHADOW_PATH, "/tmp/dp_shadow"), PDI_ERR_PERM);
    ASSERT_TRUE(exists(USER_STORE_SHADOW_PATH));
    ASSERT_FALSE(exists("/tmp/dp_shadow"));

    drop("/tmp/dp_shadow");
    forget();
}

TEST(dirperm, a_user_cannot_move_a_file_they_can_read_out_of_a_root_directory)
{
    makeUser();
    rootFile("/etc/dp_read", 0644);
    pditest::Shell shell;
    shell.run("su dirperm dirpermpw");

    ASSERT_EQ(__i_fs.moveFile("/etc/dp_read", "/tmp/dp_read"), PDI_ERR_PERM);
    ASSERT_TRUE(exists("/etc/dp_read"));
    ASSERT_FALSE(exists("/tmp/dp_read"));

    drop("/etc/dp_read");
    drop("/tmp/dp_read");
    forget();
}

TEST(dirperm, a_user_cannot_rename_a_file_inside_a_root_directory)
{
    makeUser();
    rootFile("/etc/dp_name", 0666);
    pditest::Shell shell;
    shell.run("su dirperm dirpermpw");

    ASSERT_EQ(__i_fs.rename("/etc/dp_name", "/etc/dp_other"), PDI_ERR_PERM);
    ASSERT_TRUE(exists("/etc/dp_name"));
    ASSERT_FALSE(exists("/etc/dp_other"));

    drop("/etc/dp_name");
    drop("/etc/dp_other");
    forget();
}

TEST(dirperm, a_user_cannot_move_their_own_file_into_a_root_directory)
{
    makeUser();
    pditest::Shell shell;
    shell.run("su dirperm dirpermpw");
    ASSERT_TRUE(__i_fs.createFile("/tmp/dp_mine", "mine\n") >= 0);

    ASSERT_EQ(__i_fs.moveFile("/tmp/dp_mine", "/etc/dp_mine"), PDI_ERR_PERM);
    ASSERT_TRUE(exists("/tmp/dp_mine"));
    ASSERT_FALSE(exists("/etc/dp_mine"));

    drop("/tmp/dp_mine");
    drop("/etc/dp_mine");
    forget();
}

/* ---------------------------------------------------------------- create */

TEST(dirperm, a_user_cannot_create_a_file_in_a_root_directory)
{
    makeUser();
    drop("/etc/dp_new");
    pditest::Shell shell;
    shell.run("su dirperm dirpermpw");

    ASSERT_EQ(__i_fs.createFile("/etc/dp_new", "x"), PDI_ERR_PERM);
    ASSERT_FALSE(exists("/etc/dp_new"));

    drop("/etc/dp_new");
    forget();
}

TEST(dirperm, a_user_cannot_make_a_directory_in_a_root_directory)
{
    makeUser();
    drop("/etc/dp_dir");
    pditest::Shell shell;
    shell.run("su dirperm dirpermpw");

    ASSERT_EQ(__i_fs.createDirectory("/etc/dp_dir"), PDI_ERR_PERM);
    ASSERT_FALSE(exists("/etc/dp_dir"));

    drop("/etc/dp_dir");
    forget();
}

TEST(dirperm, a_user_cannot_make_nested_directories_under_a_root_directory)
{
    makeUser();
    drop("/dp_deep");
    pditest::Shell shell;
    shell.run("su dirperm dirpermpw");

    ASSERT_EQ(__i_fs.createDirectory("/dp_deep/inner"), PDI_ERR_PERM);
    ASSERT_FALSE(exists("/dp_deep"));
    ASSERT_EQ(__i_fs.createDirectory("/etc/dp_deep/inner/more"), PDI_ERR_PERM);
    ASSERT_FALSE(exists("/etc/dp_deep"));

    drop("/dp_deep");
    drop("/etc/dp_deep");
    forget();
}

TEST(dirperm, a_user_makes_nested_directories_inside_a_directory_they_own)
{
    makeUser();
    rootDir("/dp_nest", 0755, s_uid);
    pditest::Shell shell;
    shell.run("su dirperm dirpermpw");

    ASSERT_EQ(__i_fs.createDirectory("/dp_nest/a/b"), PDI_OK);
    ASSERT_TRUE(exists("/dp_nest/a/b"));

    drop("/dp_nest");
    forget();
}

TEST(dirperm, a_dot_dot_out_of_an_own_directory_does_not_reach_a_root_directory)
{
    makeUser();
    rootDir("/dp_trav", 0755, s_uid);
    drop("/etc/dp_trav_f");
    drop("/etc/dp_trav_d");
    pditest::Shell shell;
    shell.run("su dirperm dirpermpw");

    ASSERT_EQ(__i_fs.createFile("/dp_trav/../etc/dp_trav_f", "x"), PDI_ERR_PERM);
    ASSERT_EQ(__i_fs.createDirectory("/dp_trav/../etc/dp_trav_d"), PDI_ERR_PERM);
    ASSERT_EQ(__i_fs.createFile("/dp_trav/missing/../../etc/dp_trav_f", "x"), PDI_ERR_PERM);
    ASSERT_FALSE(exists("/etc/dp_trav_f"));
    ASSERT_FALSE(exists("/etc/dp_trav_d"));

    drop("/etc/dp_trav_f");
    drop("/etc/dp_trav_d");
    drop("/dp_trav");
    forget();
}

/**
 * A path with no separator is taken from the root of the filesystem, whatever
 * the filesystem's own working directory says.
 */
TEST(dirperm, a_relative_name_is_checked_against_the_root_directory)
{
    makeUser();
    rootDir("/dp_relown", 0755, s_uid);
    drop("/dp_rel_f");
    pdiutil::string before = __i_fs.getPWD();
    __i_fs.setPWD("/dp_relown");
    pditest::Shell shell;
    shell.run("su dirperm dirpermpw");

    int rc = __i_fs.createFile("dp_rel_f", "x");
    __i_fs.setPWD(before.c_str());

    ASSERT_EQ(rc, PDI_ERR_PERM);
    ASSERT_FALSE(exists("/dp_rel_f"));
    ASSERT_FALSE(exists("/dp_relown/dp_rel_f"));

    drop("/dp_rel_f");
    drop("/dp_relown");
    forget();
}

TEST(dirperm, a_privileged_scope_still_creates_for_a_user_session)
{
    makeUser();
    drop("/etc/dp_priv");
    pditest::Shell shell;
    shell.run("su dirperm dirpermpw");

    __i_fs.beginPrivileged();
    int rc = __i_fs.createFile("/etc/dp_priv", "x");
    __i_fs.endPrivileged();

    ASSERT_TRUE(rc >= 0);
    ASSERT_TRUE(exists("/etc/dp_priv"));

    drop("/etc/dp_priv");
    forget();
}

TEST(dirperm, a_user_moves_and_renames_a_directory_they_own_inside_their_own)
{
    makeUser();
    rootDir("/dp_tree", 0755, s_uid);
    pditest::Shell shell;
    shell.run("su dirperm dirpermpw");

    ASSERT_EQ(__i_fs.createDirectory("/dp_tree/a"), PDI_OK);
    ASSERT_EQ(__i_fs.rename("/dp_tree/a", "/dp_tree/b"), PDI_OK);
    ASSERT_TRUE(exists("/dp_tree/b"));
    ASSERT_EQ(__i_fs.rename("/dp_tree/b", "/etc/dp_tree_b"), PDI_ERR_PERM);
    ASSERT_TRUE(exists("/dp_tree/b"));

    drop("/etc/dp_tree_b");
    drop("/dp_tree");
    forget();
}

TEST(dirperm, a_user_cannot_write_a_new_file_into_a_root_directory)
{
    makeUser();
    drop("/etc/dp_write");
    pditest::Shell shell;
    shell.run("su dirperm dirpermpw");

    ASSERT_EQ(__i_fs.writeFile("/etc/dp_write", "x", 1, false), PDI_ERR_PERM);
    ASSERT_FALSE(exists("/etc/dp_write"));

    drop("/etc/dp_write");
    forget();
}

TEST(dirperm, a_user_cannot_touch_a_new_file_into_a_root_directory)
{
    makeUser();
    drop("/etc/dp_touch");
    pditest::Shell shell;
    shell.run("su dirperm dirpermpw");

    ASSERT_EQ(__i_fs.touch("/etc/dp_touch"), PDI_ERR_PERM);
    ASSERT_FALSE(exists("/etc/dp_touch"));

    drop("/etc/dp_touch");
    forget();
}

TEST(dirperm, a_user_cannot_open_a_new_file_for_writing_in_a_root_directory)
{
    makeUser();
    drop("/etc/dp_open");
    pditest::Shell shell;
    shell.run("su dirperm dirpermpw");

    pdi_fhandle_t handle = __i_fs.openFile("/etc/dp_open", FILE_OPEN_WRITE | FILE_OPEN_CREATE);
    ASSERT_EQ((pdi_err_t)handle, PDI_ERR_PERM);
    ASSERT_FALSE(exists("/etc/dp_open"));

    drop("/etc/dp_open");
    forget();
}

TEST(dirperm, a_user_cannot_copy_a_readable_file_into_a_root_directory)
{
    makeUser();
    drop("/etc/dp_copy");
    pditest::Shell shell;
    shell.run("su dirperm dirpermpw");

    ASSERT_EQ(__i_fs.copyFile(USER_STORE_PASSWD_PATH, "/etc/dp_copy"), PDI_ERR_PERM);
    ASSERT_FALSE(exists("/etc/dp_copy"));

    drop("/etc/dp_copy");
    forget();
}

TEST(dirperm, a_user_cannot_put_a_new_crontab_in_place_of_a_moved_one)
{
    makeUser();
    drop("/etc/dp_crontab");
    pditest::Shell shell;
    shell.run("su dirperm dirpermpw");

    ASSERT_EQ(__i_fs.createFile("/etc/dp_crontab", "* * * * * reboot\n"), PDI_ERR_PERM);
    shell.run("echo hi > /etc/dp_crontab");
    ASSERT_FALSE(exists("/etc/dp_crontab"));

    drop("/etc/dp_crontab");
    forget();
}

/**
 * Creating what is already there is answered with what the backend says
 * about it, the same for a user as for root, not with a refusal.
 */
TEST(dirperm, creating_an_existing_file_answers_the_same_for_a_user_as_for_root)
{
    makeUser();
    rootFile("/etc/dp_there", 0644);

    __i_fs.beginPrivileged();
    int asroot = __i_fs.createFile("/etc/dp_there", "x");
    __i_fs.endPrivileged();

    pditest::Shell shell;
    shell.run("su dirperm dirpermpw");
    int asuser = __i_fs.createFile("/etc/dp_there", "x");

    ASSERT_NE(asroot, PDI_ERR_PERM);
    ASSERT_EQ(asuser, asroot);

    drop("/etc/dp_there");
    forget();
}

/* ---------------------------------------------------------------- delete */

TEST(dirperm, a_user_cannot_delete_a_world_writable_file_in_a_root_directory)
{
    makeUser();
    rootFile("/etc/dp_open_mode", 0666);
    pditest::Shell shell;
    shell.run("su dirperm dirpermpw");

    ASSERT_EQ(__i_fs.deleteFile("/etc/dp_open_mode"), PDI_ERR_PERM);
    ASSERT_TRUE(exists("/etc/dp_open_mode"));

    drop("/etc/dp_open_mode");
    forget();
}

TEST(dirperm, a_user_cannot_remove_a_world_writable_directory_in_a_root_directory)
{
    makeUser();
    rootDir("/etc/dp_open_dir", 0777, USER_STORE_ROOT_UID);
    pditest::Shell shell;
    shell.run("su dirperm dirpermpw");

    ASSERT_EQ(__i_fs.deleteDirectory("/etc/dp_open_dir"), PDI_ERR_PERM);
    ASSERT_TRUE(exists("/etc/dp_open_dir"));

    drop("/etc/dp_open_dir");
    forget();
}

/* ------------------------------------------------- what keeps working */

TEST(dirperm, a_user_still_reads_what_the_modes_allow)
{
    makeUser();
    rootFile("/etc/dp_readable", 0644);
    pditest::Shell shell;
    shell.run("su dirperm dirpermpw");

    pdiutil::string got;
    int rc = __i_fs.readFile("/etc/dp_readable", 64, [&got](char *c, uint32_t n) {
        got += pdiutil::string(c, n);
        return true;
    });
    ASSERT_TRUE(rc >= 0);
    ASSERT_TRUE(got == "kept\n");

    pdiutil::vector<file_info_t> items;
    int listed = __i_fs.getDirFileList("/etc", items);
    for (uint16_t i = 0; i < items.size(); i++) {
        pdiutil::safe_delete_array(items[i].m_name);
    }
    ASSERT_TRUE(listed >= 0);

    drop("/etc/dp_readable");
    forget();
}

TEST(dirperm, a_user_still_writes_an_existing_file_its_mode_lets_them)
{
    makeUser();
    rootFile("/etc/dp_shared", 0666);
    pditest::Shell shell;
    shell.run("su dirperm dirpermpw");

    ASSERT_TRUE(__i_fs.writeFile("/etc/dp_shared", "more\n", 5, true) >= 0);
    ASSERT_TRUE(__i_fs.touch("/etc/dp_shared") >= 0);

    drop("/etc/dp_shared");
    forget();
}

TEST(dirperm, a_user_creates_moves_and_deletes_inside_a_directory_they_own)
{
    makeUser();
    rootDir("/dp_own", 0755, s_uid);
    pditest::Shell shell;
    shell.run("su dirperm dirpermpw");

    ASSERT_TRUE(__i_fs.createFile("/dp_own/a", "a\n") >= 0);
    ASSERT_TRUE(__i_fs.writeFile("/dp_own/b", "b\n", 2, false) >= 0);
    ASSERT_TRUE(__i_fs.createDirectory("/dp_own/d") >= 0);
    ASSERT_EQ(__i_fs.rename("/dp_own/a", "/dp_own/c"), PDI_OK);
    ASSERT_EQ(__i_fs.copyFile("/dp_own/b", "/dp_own/e"), PDI_OK);
    ASSERT_EQ(__i_fs.moveFile("/dp_own/e", "/tmp/dp_e"), PDI_OK);
    ASSERT_EQ(__i_fs.moveFile("/tmp/dp_e", "/dp_own/e"), PDI_OK);
    ASSERT_EQ(__i_fs.deleteFile("/dp_own/c"), PDI_OK);
    ASSERT_EQ(__i_fs.deleteDirectory("/dp_own/d"), PDI_OK);
    ASSERT_FALSE(exists("/dp_own/c"));
    ASSERT_TRUE(exists("/dp_own/e"));

    drop("/tmp/dp_e");
    drop("/dp_own");
    forget();
}

TEST(dirperm, a_user_creates_and_deletes_in_tmp)
{
    makeUser();
    pditest::Shell shell;
    shell.run("su dirperm dirpermpw");

    ASSERT_TRUE(__i_fs.createFile("/tmp/dp_t", "t\n") >= 0);
    ASSERT_TRUE(__i_fs.createDirectory("/tmp/dp_td") >= 0);
    ASSERT_EQ(__i_fs.deleteFile("/tmp/dp_t"), PDI_OK);
    ASSERT_EQ(__i_fs.deleteDirectory("/tmp/dp_td"), PDI_OK);

    forget();
}

TEST(dirperm, root_still_creates_moves_and_deletes_in_root_directories)
{
    pditest::mountedVfs();
    pditest::seedRootAccount();
    drop("/etc/dp_root");
    drop("/etc/dp_root2");
    pditest::Shell shell;
    ASSERT_EQ(SessionManager::getCurrentUid(), USER_STORE_ROOT_UID);

    ASSERT_TRUE(__i_fs.createFile("/etc/dp_root", "r\n") >= 0);
    ASSERT_EQ(__i_fs.rename("/etc/dp_root", "/etc/dp_root2"), PDI_OK);
    ASSERT_EQ(__i_fs.deleteFile("/etc/dp_root2"), PDI_OK);
    ASSERT_FALSE(exists("/etc/dp_root2"));
}

/* ------------------------------------------- services after a user's line */

static bool fileSays(const char *path, const char *text)
{
    pdiutil::string got;
    __i_fs.beginPrivileged();
    __i_fs.readFile(path, 128, [&got](char *c, uint32_t n) {
        got += pdiutil::string(c, n);
        return true;
    });
    __i_fs.endPrivileged();
    return pdiutil::string::npos != got.find(text);
}

/**
 * Services run between lines, after the shell has handled whatever a user
 * typed. What they write is the system's, whoever typed last.
 */
TEST(dirperm, a_service_still_logs_after_a_user_typed_the_last_line)
{
    makeUser();
    __i_fs.beginPrivileged();
    __i_fs.createDirectory(SYSLOG_DIR);
    __i_fs.setFileOwner(SYSLOG_DIR, USER_STORE_ROOT_UID, USER_STORE_ROOT_GID);
    __i_fs.endPrivileged();
    rootFile(SYSLOG_FILE_WARNING, 0644);
    drop(SYSLOG_FILE_ERROR);

    pditest::Shell shell;
    shell.run("su dirperm dirpermpw");
    ASSERT_EQ(SessionManager::getCurrentUid(), s_uid);

    __syslog_service.initService();
    SysLogW("dirperm appended line\n");
    SysLogE("dirperm created line\n");
    __syslog_service.stopService();

    ASSERT_TRUE(fileSays(SYSLOG_FILE_WARNING, "dirperm appended line"));
    ASSERT_TRUE(fileSays(SYSLOG_FILE_ERROR, "dirperm created line"));

    forget();
}
