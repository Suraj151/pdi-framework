/************************** Session Identity Tests ****************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Where a session's privilege comes from. The filesystem judges every access by
the session's own uid, so that is what a command has to read as well, and a
session the store has no record for must not inherit root by default.

Author          : Suraj I.
created Date    : 16th Aug 2026
******************************************************************************/

#include <ShellHarness.h>
#include <service_provider/user/UserStoreService.h>
#include <pditest.h>

using pditest::saw;

static void forget(const char *name)
{
    __i_fs.beginPrivileged();
    __user_store_service.removeUser(name);
    __i_fs.endPrivileged();
}

static void makeUser(const char *name, uint16_t uid, const char *password)
{
    pditest::mountedVfs();
    pditest::seedRootAccount();
    forget(name);

    user_record_t record;
    record.m_username = name;
    record.m_uid = uid;
    record.m_gid = uid;
    record.m_home = "/";
    record.m_shell = "/bin/sh";
    __user_store_service.addUser(record, password);
}

/* ------------------------------------------- one source of truth: the session */

TEST(identity, id_reports_what_the_session_carries)
{
    makeUser("idsrc", 2100, "idsrcpw");

    pditest::Shell shell;
    shell.run("su idsrc idsrcpw");

    std::string out = shell.run("id");
    ASSERT_TRUE(saw(out, "uid=2100"));
    ASSERT_TRUE(saw(out, "gid=2100"));
    ASSERT_TRUE(saw(out, "idsrc"));
    ASSERT_EQ(SessionManager::getCurrentUid(), 2100);

    forget("idsrc");
}

TEST(identity, groups_reports_what_the_session_carries)
{
    makeUser("grpsrc", 2101, "grpsrcpw");

    pditest::Shell shell;
    shell.run("su grpsrc grpsrcpw");
    ASSERT_TRUE(saw(shell.run("groups"), "2101"));

    forget("grpsrc");
}

/**
 * A record edited under a live session must not change what that session may
 * do. The uid is settled at login, the way it is on a real system, and the
 * filesystem would not have followed a mid-session change anyway.
 */
TEST(identity, promoting_a_record_does_not_promote_a_live_session)
{
    makeUser("promoted", 2102, "promotedpw");

    pditest::Shell shell;
    shell.run("su promoted promotedpw");
    ASSERT_EQ(SessionManager::getCurrentUid(), 2102);
    forget("probeacct");

    __i_fs.beginPrivileged();
    __user_store_service.removeUser("promoted");
    user_record_t asroot;
    asroot.m_username = "promoted";
    asroot.m_uid = USER_STORE_ROOT_UID;
    asroot.m_gid = USER_STORE_ROOT_GID;
    asroot.m_home = "/";
    asroot.m_shell = "/bin/sh";
    __user_store_service.addUser(asroot, "promotedpw");
    __i_fs.endPrivileged();

    // the store now calls this user root; the session must not agree
    ASSERT_EQ(SessionManager::getCurrentUid(), 2102);
    ASSERT_TRUE(saw(shell.run("useradd u=probeacct p=x"), "root required"));
    ASSERT_TRUE(saw(shell.run("id"), "uid=2102"));

    user_record_t made;
    ASSERT_FALSE(__user_store_service.findUserByName("probeacct", made));

    forget("promoted");
}

TEST(identity, removing_a_record_does_not_demote_a_live_session)
{
    makeUser("staying", USER_STORE_ROOT_UID, "stayingpw");

    pditest::Shell shell;
    shell.run("su staying stayingpw");
    ASSERT_EQ(SessionManager::getCurrentUid(), USER_STORE_ROOT_UID);

    forget("staying");

    // the filesystem still treats this session as root, so the commands must too
    ASSERT_EQ(SessionManager::getCurrentUid(), USER_STORE_ROOT_UID);
    forget("afterremoval");
    ASSERT_FALSE(saw(shell.run("useradd u=afterremoval p=x"), "root required"));

    user_record_t made;
    ASSERT_TRUE(__user_store_service.findUserByName("afterremoval", made));
    forget("afterremoval");
}

TEST(identity, a_plain_user_is_refused_the_root_only_commands)
{
    makeUser("plainacct", 2103, "plainpw");

    pditest::Shell shell;
    shell.run("su plainacct plainpw");

    // srvc guards the same way, but it resolves the service name first, so it
    // needs a service the build actually linked and is covered with the rest of
    // the process commands
    ASSERT_TRUE(saw(shell.run("useradd u=nope p=x"), "root required"));
    ASSERT_TRUE(saw(shell.run("userdel u=plainacct"), "root required"));

    user_record_t still;
    ASSERT_TRUE(__user_store_service.findUserByName("plainacct", still));

    forget("plainacct");
}

/* --------------------------------- a session with no record is not privileged */

TEST(identity, authenticating_without_a_record_does_not_grant_root)
{
    makeUser("orphan", 2104, "orphanpw");

    // the store is in use and this account can still authenticate, but its
    // record is gone: the shape a truncated passwd file leaves behind
    __i_fs.beginPrivileged();
    __i_fs.deleteFile(USER_STORE_PASSWD_PATH);
    __i_fs.endPrivileged();

    pditest::Shell shell;
    shell.run("su orphan orphanpw");

    ASSERT_NE(SessionManager::getCurrentUid(), USER_STORE_ROOT_UID);
    ASSERT_EQ(SessionManager::getCurrentUid(), USER_STORE_NOBODY_UID);
    ASSERT_TRUE(saw(shell.run("id"), "uid=65534"));

    // and it must not be able to reach what root owns
    __i_fs.beginPrivileged();
    __i_fs.createFile("/rootonly.txt", "secret");
    __i_fs.setFileOwner("/rootonly.txt", USER_STORE_ROOT_UID, USER_STORE_ROOT_GID);
    __i_fs.setFilePermissions("/rootonly.txt", 0600);
    __i_fs.endPrivileged();

    ASSERT_FALSE(saw(shell.run("cat /rootonly.txt"), "secret"));
    ASSERT_TRUE(saw(shell.run("useradd u=nope2 p=x"), "root required"));

    __i_fs.beginPrivileged();
    __i_fs.deleteFile("/rootonly.txt");
    __i_fs.endPrivileged();
}

/**
 * Before the store exists there is only the configured account, and it is the
 * administrator. That path has to keep working or a fresh device cannot be set
 * up at all.
 */
TEST(identity, the_login_table_path_still_grants_root)
{
    pditest::mountedVfs();

    __i_fs.beginPrivileged();
    __i_fs.deleteFile(USER_STORE_SHADOW_PATH);
    __i_fs.deleteFile(USER_STORE_PASSWD_PATH);
    __i_fs.endPrivileged();

    pditest::Shell shell;
    __auth_service.setVerifiedUsername("pdiStack");
    __auth_service.setAuthorized(true);

    ASSERT_EQ(SessionManager::getCurrentUid(), USER_STORE_ROOT_UID);
    ASSERT_EQ(SessionManager::getCurrentGid(), USER_STORE_ROOT_GID);
}

/* -------------------------------------------------- the store keeps an admin */

TEST(identity, boot_restores_an_administrator_when_the_store_has_none)
{
    pditest::mountedVfs();

    __i_fs.beginPrivileged();
    __i_fs.deleteFile(USER_STORE_PASSWD_PATH);
    __i_fs.deleteFile(USER_STORE_SHADOW_PATH);
    __i_fs.endPrivileged();

    __database_service.initService();
    __user_store_service.initService();

    user_record_t root;
    ASSERT_TRUE(__user_store_service.findUserByUid(USER_STORE_ROOT_UID, root));
    ASSERT_TRUE(root.m_username.length() > 0);
}

/**
 * A store that exists belongs to the administrator, whatever state they left it
 * in. Boot must not invent an account to repair it — a passwd file with no root
 * account is an undefined state, and the way out is the factory reset on the
 * device's flash key, not a surprise login appearing by itself.
 */
TEST(identity, boot_does_not_invent_an_administrator_for_a_store_that_has_none)
{
    pditest::mountedVfs();
    pditest::seedRootAccount();

    __i_fs.beginPrivileged();
    __user_store_service.removeUser("pdiStack");
    __i_fs.endPrivileged();

    // a store with entries but no root account at all
    user_record_t plain;
    plain.m_username = "plainonly";
    plain.m_uid = 2300;
    plain.m_gid = 2300;
    plain.m_home = "/";
    plain.m_shell = "/bin/sh";
    __user_store_service.addUser(plain, "plainonlypw");

    user_record_t none;
    ASSERT_FALSE(__user_store_service.findUserByUid(USER_STORE_ROOT_UID, none));

    __database_service.initService();
    __user_store_service.initService();

    ASSERT_FALSE(__user_store_service.findUserByUid(USER_STORE_ROOT_UID, none));
    ASSERT_FALSE(__user_store_service.findUserByName("pdiStack", none));

    forget("plainonly");
}

TEST(identity, boot_leaves_an_existing_administrator_alone)
{
    pditest::mountedVfs();
    pditest::seedRootAccount();

    // an operator who replaced the shipped account with their own
    __i_fs.beginPrivileged();
    __user_store_service.removeUser("pdiStack");
    __i_fs.endPrivileged();

    user_record_t mine;
    mine.m_username = "operator";
    mine.m_uid = USER_STORE_ROOT_UID;
    mine.m_gid = USER_STORE_ROOT_GID;
    mine.m_home = "/";
    mine.m_shell = "/bin/sh";
    __user_store_service.addUser(mine, "operatorpw");

    __user_store_service.initService();

    // the shipped account must not come back
    user_record_t shipped;
    ASSERT_FALSE(__user_store_service.findUserByName("pdiStack", shipped));
    ASSERT_TRUE(__user_store_service.findUserByName("operator", shipped));

    forget("operator");
}
