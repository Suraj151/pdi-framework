/*************************** User Command Tests *******************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Who the session says it is, and the commands that change that.

Author          : Suraj I.
created Date    : 16th Aug 2026
******************************************************************************/

#include <ShellHarness.h>
#include <service_provider/user/UserStoreService.h>
#include <pditest.h>

using pditest::saw;

/**
 * The store needs /etc, which only the boot path creates.
 */
static void readyStore()
{
    pditest::mountedVfs();
    if (!__i_fs.isDirectory(USER_STORE_ETC_DIR))
    {
        __i_fs.createDirectory(USER_STORE_ETC_DIR);
    }
}

static void makeUser(const char *name, uint16_t uid, const char *password)
{
    readyStore();
    __user_store_service.removeUser(name);

    user_record_t record;
    record.m_username = name;
    record.m_uid = uid;
    record.m_gid = uid;
    record.m_home = "/";
    record.m_shell = "/bin/sh";
    __user_store_service.addUser(record, password);
}

/* ------------------------------------------------------------------ identity */

TEST(cmdusers, whoami_names_the_logged_in_user)
{
    pditest::Shell shell;
    ASSERT_TRUE(saw(shell.run("whoami"), "pdiStack"));
}

TEST(cmdusers, id_prints_the_uid_and_gid)
{
    pditest::Shell shell;
    std::string out = shell.run("id");

    ASSERT_TRUE(saw(out, "uid=0"));
    ASSERT_TRUE(saw(out, "gid=0"));
}

/**
 * id and groups read the account record, not the session's own uid, so a
 * switched user is what changes what they report.
 */
TEST(cmdusers, id_follows_a_switched_user)
{
    makeUser("idtest", 1500, "idtestpass");
    pditest::Shell shell;
    shell.run("su idtest idtestpass");

    std::string out = shell.run("id");
    ASSERT_TRUE(saw(out, "uid=1500"));
    ASSERT_TRUE(saw(out, "idtest"));

    __user_store_service.removeUser("idtest");
}

TEST(cmdusers, groups_prints_the_primary_group)
{
    makeUser("grptest", 1501, "grptestpass");
    pditest::Shell shell;
    shell.run("su grptest grptestpass");

    ASSERT_TRUE(saw(shell.run("groups"), "1501"));

    __user_store_service.removeUser("grptest");
}

TEST(cmdusers, who_lists_the_open_session)
{
    pditest::Shell shell;
    std::string out = shell.run("who");

    ASSERT_TRUE(saw(out, "pdiStack"));
    ASSERT_TRUE(saw(out, "USER"));
}

TEST(cmdusers, who_shows_a_second_session_too)
{
    pditest::Shell first;
    pditest::Shell second;

    std::string out = second.run("who");
    uint8_t rows = 0;
    for (size_t at = out.find("pdiStack"); at != std::string::npos; at = out.find("pdiStack", at + 1))
    {
        rows++;
    }

    ASSERT_TRUE(rows >= 2);
}

/* -------------------------------------------------------- useradd, userdel */

TEST(cmdusers, useradd_creates_an_account_that_can_be_found)
{
    pditest::Shell shell;
    readyStore();
    __user_store_service.removeUser("tess");

    shell.run("useradd u=tess p=tesspass");

    user_record_t found;
    ASSERT_TRUE(__user_store_service.findUserByName("tess", found));
    ASSERT_TRUE(__user_store_service.verifyPassword("tess", "tesspass"));

    __user_store_service.removeUser("tess");
}

TEST(cmdusers, useradd_is_refused_to_a_user_that_is_not_root)
{
    makeUser("ordinary", 1502, "ordinarypass");
    pditest::Shell shell;
    shell.run("su ordinary ordinarypass");
    __user_store_service.removeUser("sneaky");

    shell.run("useradd u=sneaky p=sneakypass");

    user_record_t found;
    ASSERT_FALSE(__user_store_service.findUserByName("sneaky", found));

    __user_store_service.removeUser("ordinary");
}

TEST(cmdusers, userdel_removes_an_account)
{
    pditest::Shell shell;
    makeUser("gone", 1503, "gonepass");

    shell.run("userdel u=gone");

    user_record_t found;
    ASSERT_FALSE(__user_store_service.findUserByName("gone", found));
}

TEST(cmdusers, userdel_is_refused_to_a_user_that_is_not_root)
{
    makeUser("keeper", 1504, "keeperpass");
    makeUser("meddler", 1505, "meddlerpass");
    pditest::Shell shell;
    shell.run("su meddler meddlerpass");

    shell.run("userdel u=keeper");

    user_record_t found;
    ASSERT_TRUE(__user_store_service.findUserByName("keeper", found));

    __user_store_service.removeUser("keeper");
    __user_store_service.removeUser("meddler");
}

/* ------------------------------------------------------------------- su */

TEST(cmdusers, su_switches_the_session_to_another_user)
{
    makeUser("switched", 1506, "switchedpass");
    pditest::Shell shell;

    shell.run("su switched switchedpass");

    ASSERT_TRUE(saw(shell.run("whoami"), "switched"));
    ASSERT_TRUE(saw(shell.run("id"), "uid=1506"));

    __user_store_service.removeUser("switched");
}

TEST(cmdusers, su_with_a_wrong_password_changes_nothing)
{
    makeUser("guarded", 1507, "guardedpass");
    pditest::Shell shell;

    shell.run("su guarded notthepassword");

    ASSERT_TRUE(saw(shell.run("whoami"), "pdiStack"));
    ASSERT_TRUE(saw(shell.run("id"), "uid=0"));
    ASSERT_EQ(SessionManager::getCurrentUid(), 0);

    __user_store_service.removeUser("guarded");
}

TEST(cmdusers, su_to_an_unknown_user_changes_nothing)
{
    pditest::Shell shell;

    shell.run("su nobodyhere somepass");
    ASSERT_TRUE(saw(shell.run("whoami"), "pdiStack"));
}

TEST(cmdusers, a_switched_session_loses_root_privilege)
{
    makeUser("plain", 1508, "plainpass");
    pditest::Shell shell;

    // owned and locked down while the session is still root, since changing
    // ownership is itself a root-only operation
    __i_fs.deleteFile("/w_superm.txt");
    __i_fs.createFile("/w_superm.txt", "root only");
    __i_fs.setFileOwner("/w_superm.txt", 0, 0);
    __i_fs.setFilePermissions("/w_superm.txt", 0600);
    ASSERT_TRUE(saw(shell.run("cat /w_superm.txt"), "root only"));

    shell.run("su plain plainpass");
    ASSERT_FALSE(saw(shell.run("cat /w_superm.txt"), "root only"));

    __i_fs.deleteFile("/w_superm.txt");
    __user_store_service.removeUser("plain");
}

/* ----------------------------------------------------------------- passwd */

TEST(cmdusers, passwd_changes_the_password_of_the_current_user)
{
    makeUser("changer", 1509, "oldpass");
    pditest::Shell shell;
    shell.run("su changer oldpass");

    shell.run("passwd p=oldpass n=newpass c=newpass");

    ASSERT_TRUE(__user_store_service.verifyPassword("changer", "newpass"));
    ASSERT_FALSE(__user_store_service.verifyPassword("changer", "oldpass"));

    __user_store_service.removeUser("changer");
}

TEST(cmdusers, passwd_needs_the_current_password_to_be_right)
{
    makeUser("careful", 1510, "realpass");
    pditest::Shell shell;
    shell.run("su careful realpass");

    shell.run("passwd p=wrongpass n=newpass c=newpass");

    ASSERT_TRUE(__user_store_service.verifyPassword("careful", "realpass"));
    ASSERT_FALSE(__user_store_service.verifyPassword("careful", "newpass"));

    __user_store_service.removeUser("careful");
}

TEST(cmdusers, passwd_needs_the_confirmation_to_match)
{
    makeUser("typo", 1511, "typopass");
    pditest::Shell shell;
    shell.run("su typo typopass");

    shell.run("passwd p=typopass n=firstnew c=secondnew");

    ASSERT_TRUE(__user_store_service.verifyPassword("typo", "typopass"));
    ASSERT_FALSE(__user_store_service.verifyPassword("typo", "firstnew"));

    __user_store_service.removeUser("typo");
}

/* ----------------------------------------------------------------- logout */

TEST(cmdusers, logout_drops_the_authorisation)
{
    pditest::Shell shell;
    ASSERT_TRUE(__auth_service.getAuthorized());

    shell.run("logout");
    ASSERT_FALSE(__auth_service.getAuthorized());
}
