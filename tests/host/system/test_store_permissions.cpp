/*********************** Credential Store Permissions *************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

/etc/shadow holds every password hash and is the one file on the device that
must never widen. The store rewrites it in place whenever a password changes or
an account is removed, and a rewrite is where metadata is easiest to lose.

Author          : Suraj I.
created Date    : 16th Aug 2026
******************************************************************************/

#include <ShellHarness.h>
#include <service_provider/user/UserStoreService.h>
#include <pditest.h>

using pditest::saw;

static void makeUser(const char *name, uint16_t uid, const char *password)
{
    pditest::mountedVfs();
    pditest::seedRootAccount();

    __i_fs.beginPrivileged();
    __user_store_service.removeUser(name);
    __i_fs.endPrivileged();

    user_record_t record;
    record.m_username = name;
    record.m_uid = uid;
    record.m_gid = uid;
    record.m_home = "/";
    record.m_shell = "/bin/sh";
    __user_store_service.addUser(record, password);
}

static void forget(const char *name)
{
    __i_fs.beginPrivileged();
    __user_store_service.removeUser(name);
    __i_fs.endPrivileged();
}

static bool metaOf(const char *path, file_info_t &out)
{
    return (0 == __i_fs.getFileMeta(path, out));
}

/* ------------------------------------------------------------------ shadow */

TEST(storeperms, the_shadow_file_starts_locked_down)
{
    pditest::mountedVfs();
    pditest::seedRootAccount();

    file_info_t meta;
    ASSERT_TRUE(metaOf(USER_STORE_SHADOW_PATH, meta));
    ASSERT_EQ(meta.m_perms, 0600);
    ASSERT_EQ(meta.m_uid, USER_STORE_ROOT_UID);
}

/**
 * The rewrite that a password change performs must not hand the file to
 * whoever ran the command.
 */
TEST(storeperms, a_users_own_password_change_does_not_widen_the_shadow_file)
{
    makeUser("pwuser", 2000, "firstpw");

    pditest::Shell shell;
    shell.run("su pwuser firstpw");
    ASSERT_EQ(SessionManager::getCurrentUid(), 2000);

    ASSERT_TRUE(saw(shell.run("passwd p=firstpw n=secondpw c=secondpw"), "updated"));

    file_info_t meta;
    ASSERT_TRUE(metaOf(USER_STORE_SHADOW_PATH, meta));
    ASSERT_EQ(meta.m_perms, 0600);
    ASSERT_EQ(meta.m_uid, USER_STORE_ROOT_UID);
    ASSERT_EQ(meta.m_gid, USER_STORE_ROOT_GID);

    forget("pwuser");
}

TEST(storeperms, a_user_cannot_read_the_hashes_after_changing_their_password)
{
    makeUser("nosypw", 2001, "firstpw");

    pditest::Shell shell;
    shell.run("su nosypw firstpw");
    shell.run("passwd p=firstpw n=secondpw c=secondpw");

    pdiutil::string spill;
    int rc = __i_fs.readFile(USER_STORE_SHADOW_PATH, 128, [&spill](char *c, uint32_t n) {
        spill += pdiutil::string(c, n);
        return true;
    });

    ASSERT_EQ(rc, PDI_ERR_PERM);
    ASSERT_EQ(spill.length(), 0u);

    forget("nosypw");
}

TEST(storeperms, a_user_cannot_write_the_hashes_after_changing_their_password)
{
    makeUser("writepw", 2002, "firstpw");

    pditest::Shell shell;
    shell.run("su writepw firstpw");
    shell.run("passwd p=firstpw n=secondpw c=secondpw");

    ASSERT_EQ(__i_fs.writeFile(USER_STORE_SHADOW_PATH, "junk\n", 5, true), PDI_ERR_PERM);

    forget("writepw");
}

TEST(storeperms, the_new_password_still_works_after_the_rewrite)
{
    makeUser("stillpw", 2003, "firstpw");

    pditest::Shell shell;
    shell.run("su stillpw firstpw");
    shell.run("passwd p=firstpw n=secondpw c=secondpw");

    ASSERT_TRUE(__user_store_service.verifyPassword("stillpw", "secondpw"));
    ASSERT_FALSE(__user_store_service.verifyPassword("stillpw", "firstpw"));

    forget("stillpw");
}

TEST(storeperms, several_password_changes_in_a_row_keep_it_locked_down)
{
    makeUser("repeatpw", 2004, "pw0");

    pditest::Shell shell;
    shell.run("su repeatpw pw0");
    shell.run("passwd p=pw0 n=pw1 c=pw1");
    shell.run("passwd p=pw1 n=pw2 c=pw2");
    shell.run("passwd p=pw2 n=pw3 c=pw3");

    file_info_t meta;
    ASSERT_TRUE(metaOf(USER_STORE_SHADOW_PATH, meta));
    ASSERT_EQ(meta.m_perms, 0600);
    ASSERT_EQ(meta.m_uid, USER_STORE_ROOT_UID);
    ASSERT_TRUE(__user_store_service.verifyPassword("repeatpw", "pw3"));

    forget("repeatpw");
}

/* ------------------------------------------------------------------ passwd */

TEST(storeperms, removing_an_account_keeps_the_passwd_file_as_it_was)
{
    makeUser("goingaway", 2005, "gonepw");

    file_info_t before;
    ASSERT_TRUE(metaOf(USER_STORE_PASSWD_PATH, before));

    forget("goingaway");

    file_info_t after;
    ASSERT_TRUE(metaOf(USER_STORE_PASSWD_PATH, after));
    ASSERT_EQ(after.m_perms, before.m_perms);
    ASSERT_EQ(after.m_uid, before.m_uid);
    ASSERT_EQ(after.m_gid, before.m_gid);
}

TEST(storeperms, the_working_copy_is_not_left_readable_in_the_temp_directory)
{
    makeUser("tmppw", 2006, "firstpw");

    pditest::Shell shell;
    shell.run("su tmppw firstpw");
    shell.run("passwd p=firstpw n=secondpw c=secondpw");

    // whatever the rewrite used must not survive as a readable copy
    pdiutil::string leftover = pdiutil::string(__i_fs.getTempDirectory()) +
                               __i_fs.basename(USER_STORE_SHADOW_PATH);
    if (__i_fs.isFileExist(leftover.c_str()))
    {
        file_info_t meta;
        ASSERT_TRUE(metaOf(leftover.c_str(), meta));
        ASSERT_EQ(meta.m_perms & 0077, 0);
    }

    forget("tmppw");
}
