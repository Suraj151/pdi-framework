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

TEST(cmdusers, useradd_refuses_a_name_that_would_not_read_back_as_one_account)
{
    pditest::Shell shell;
    readyStore();

    ASSERT_TRUE(pditest::saw(shell.run("useradd u=ghost:x:0:0::/:/bin/sh p=x"), "invalid user name"));
    ASSERT_TRUE(pditest::saw(shell.run("useradd u=\"two words\" p=x"), "invalid user name"));

    user_record_t found;
    ASSERT_FALSE(__user_store_service.findUserByName("ghost", found));
    ASSERT_FALSE(__user_store_service.findUserByName("two words", found));
}

TEST(cmdusers, useradd_refuses_a_name_too_long_rather_than_cutting_it_down)
{
    pditest::Shell shell;
    readyStore();
    std::string name(USER_STORE_MAX_USERNAME_LEN + 5, 'l');
    std::string cut(USER_STORE_MAX_USERNAME_LEN - 1, 'l');

    ASSERT_TRUE(pditest::saw(shell.run(("useradd u=" + name + " p=x").c_str()), "invalid user name"));

    user_record_t found;
    ASSERT_FALSE(__user_store_service.findUserByName(cut.c_str(), found));
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

/* ------------------------------------------------------------ home dirs */

static bool inDir(const char *want)
{
    pdiutil::string pwd = SessionManager::getPWD();
    while (pwd.size() > 1 && '/' == pwd[pwd.size() - 1]) pwd.pop_back();
    return pwd == want;
}

static void dropHome(const char *name)
{
    pdiutil::string home = "/home/";
    home += name;
    __i_fs.beginPrivileged();
    __user_store_service.removeUser(name);
    if (__i_fs.isDirExist(home.c_str())) __i_fs.deleteDirectory(home.c_str());
    __i_fs.endPrivileged();
}

TEST(cmdusers, useradd_makes_a_home_owned_by_the_new_user)
{
    pditest::Shell shell;
    readyStore();
    dropHome("homer");

    shell.run("useradd u=homer p=homerpass");

    user_record_t found;
    ASSERT_TRUE(__user_store_service.findUserByName("homer", found));
    ASSERT_TRUE(found.m_home == "/home/homer");
    ASSERT_TRUE(__i_fs.isDirectory("/home/homer"));

    file_info_t meta;
    ASSERT_EQ(__i_fs.getFileMeta("/home/homer", meta), 0);
    ASSERT_EQ(meta.m_uid, found.m_uid);
    ASSERT_EQ(meta.m_gid, found.m_gid);
    ASSERT_EQ(meta.m_perms, 0755);

    ASSERT_EQ(__i_fs.getFileMeta("/home", meta), 0);
    ASSERT_EQ(meta.m_uid, USER_STORE_ROOT_UID);
    ASSERT_EQ(meta.m_perms, 0755);

    dropHome("homer");
}

TEST(cmdusers, userdel_removes_the_home_and_what_is_in_it)
{
    pditest::Shell shell;
    readyStore();
    dropHome("leaver");

    shell.run("useradd u=leaver p=leaverpass");
    ASSERT_TRUE(__i_fs.isDirectory("/home/leaver"));
    __i_fs.createFile("/home/leaver/notes", "n\n");
    __i_fs.createDirectory("/home/leaver/sub");

    shell.run("userdel u=leaver");

    user_record_t found;
    ASSERT_FALSE(__user_store_service.findUserByName("leaver", found));
    ASSERT_FALSE(__i_fs.isDirExist("/home/leaver"));
    ASSERT_TRUE(__i_fs.isDirectory("/home"));

    dropHome("leaver");
}

TEST(cmdusers, userdel_leaves_a_home_outside_home_alone)
{
    pditest::Shell shell;
    makeUser("rooted", 1512, "rootedpass");

    shell.run("userdel u=rooted");

    user_record_t found;
    ASSERT_FALSE(__user_store_service.findUserByName("rooted", found));
    ASSERT_TRUE(__i_fs.isDirectory(USER_STORE_ETC_DIR));
    ASSERT_TRUE(__i_fs.isFileExist(USER_STORE_PASSWD_PATH));
}

TEST(cmdusers, useradd_keeps_a_home_that_is_already_there)
{
    pditest::Shell shell;
    readyStore();
    dropHome("keeper2");
    __i_fs.createDirectory("/home/keeper2");
    __i_fs.createFile("/home/keeper2/old", "old\n");

    std::string out = shell.run("useradd u=keeper2 p=keeperpass");

    user_record_t found;
    ASSERT_TRUE(__user_store_service.findUserByName("keeper2", found));
    ASSERT_TRUE(saw(out, "home exists"));
    ASSERT_TRUE(__i_fs.isFileExist("/home/keeper2/old"));
    file_info_t meta;
    ASSERT_EQ(__i_fs.getFileMeta("/home/keeper2", meta), 0);
    ASSERT_EQ(meta.m_uid, USER_STORE_ROOT_UID);

    dropHome("keeper2");
}

TEST(cmdusers, useradd_takes_the_account_back_when_a_file_holds_the_home)
{
    pditest::Shell shell;
    readyStore();
    dropHome("blocked");
    __i_fs.createDirectory("/home");
    __i_fs.createFile("/home/blocked", "in the way\n");

    std::string out = shell.run("useradd u=blocked p=blockedpass");

    user_record_t found;
    ASSERT_FALSE(__user_store_service.findUserByName("blocked", found));
    ASSERT_TRUE(saw(out, "cannot create home"));
    ASSERT_TRUE(__i_fs.isFileExist("/home/blocked"));
    ASSERT_FALSE(__i_fs.isDirectory("/home/blocked"));

    __i_fs.deleteFile("/home/blocked");
}

TEST(cmdusers, useradd_gives_no_home_to_a_name_of_only_dots)
{
    pditest::Shell shell;
    readyStore();
    __i_fs.createDirectory("/home");
    __i_fs.createFile("/home/dots_marker", "m\n");

    std::string out = shell.run("useradd u=.. p=dotspass");
    shell.run("useradd u=. p=dotspass");

    user_record_t found;
    ASSERT_FALSE(__user_store_service.findUserByName("..", found));
    ASSERT_FALSE(__user_store_service.findUserByName(".", found));
    ASSERT_TRUE(saw(out, "cannot create home"));
    ASSERT_TRUE(__i_fs.isFileExist("/home/dots_marker"));
    ASSERT_TRUE(__i_fs.isFileExist(USER_STORE_PASSWD_PATH));

    __i_fs.deleteFile("/home/dots_marker");
}

TEST(cmdusers, userdel_leaves_a_home_that_belongs_to_another_name_alone)
{
    pditest::Shell shell;
    readyStore();
    __user_store_service.removeUser("thief");
    __i_fs.createDirectory("/home/victim");
    __i_fs.createFile("/home/victim/data", "victim's\n");

    user_record_t record;
    record.m_username = "thief";
    record.m_uid = 1513;
    record.m_gid = 1513;
    record.m_home = "/home/victim";
    record.m_shell = "/bin/sh";
    __user_store_service.addUser(record, "thiefpass");

    shell.run("userdel u=thief");

    user_record_t found;
    ASSERT_FALSE(__user_store_service.findUserByName("thief", found));
    ASSERT_TRUE(__i_fs.isFileExist("/home/victim/data"));

    __i_fs.deleteDirectory("/home/victim");
}

TEST(cmdusers, login_with_a_home_that_is_gone_stays_where_logout_left_it)
{
    pditest::Shell shell;
    readyStore();
    dropHome("homeless");
    shell.run("useradd u=homeless p=homelesspass");
    __i_fs.deleteDirectory("/home/homeless");

    shell.run("logout");
    shell.type("homeless\n");
    shell.type("homelesspass\n");

    user_record_t found;
    ASSERT_TRUE(__user_store_service.findUserByName("homeless", found));
    ASSERT_EQ(SessionManager::getCurrentUid(), found.m_uid);
    ASSERT_TRUE(inDir("/"));

    dropHome("homeless");
}

TEST(cmdusers, login_lands_in_the_users_home)
{
    pditest::Shell shell;
    readyStore();
    dropHome("lander");
    shell.run("useradd u=lander p=landerpass");

    shell.run("logout");
    shell.type("lander\n");
    shell.type("landerpass\n");

    user_record_t found;
    ASSERT_TRUE(__user_store_service.findUserByName("lander", found));
    ASSERT_EQ(SessionManager::getCurrentUid(), found.m_uid);
    ASSERT_TRUE(inDir("/home/lander"));

    ASSERT_TRUE(__i_fs.createFile("/home/lander/mine", "m\n") >= 0);

    dropHome("lander");
}

TEST(cmdusers, a_bare_cd_and_cd_tilde_go_to_the_users_home)
{
    pditest::Shell shell;
    readyStore();
    dropHome("walker");
    shell.run("useradd u=walker p=walkerpass");
    shell.run("su walker walkerpass");

    shell.run("cd /tmp");
    shell.run("cd");
    ASSERT_TRUE(inDir("/home/walker"));

    shell.run("cd /etc");
    shell.run("cd ~");
    ASSERT_TRUE(inDir("/home/walker"));

    dropHome("walker");
}

TEST(cmdusers, su_lands_in_the_new_users_home)
{
    pditest::Shell shell;
    readyStore();
    dropHome("stayer");
    shell.run("useradd u=stayer p=stayerpass");

    shell.run("cd /etc");
    shell.run("su stayer stayerpass");
    ASSERT_TRUE(inDir("/home/stayer"));

    dropHome("stayer");
}

TEST(cmdusers, root_still_lands_in_slash)
{
    pditest::Shell shell;
    shell.run("cd /etc");
    shell.run("cd");
    ASSERT_TRUE(inDir("/"));
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
