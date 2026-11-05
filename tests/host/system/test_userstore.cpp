/**************************** User Store Tests ********************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Exercises the account store the way login, su and passwd reach it — records in
/etc/passwd and salted hashes in /etc/shadow, both on the real filesystem.

Author          : Suraj I.
created Date    : 16th Aug 2026
******************************************************************************/

#include <interface/pdi.h>
#include <service_provider/user/UserStoreService.h>
#include <MountedStack.h>
#include <pditest.h>

/**
 * The service creates /etc on its way through bootstrapFromLoginTable, which
 * needs the database. addUser does not create it on its own, so the directory
 * is put in place here the same way boot would.
 */
static UserStoreService *store()
{
    VfsDispatcher *fs = pditest::mountedVfs();

    if (!fs->isDirectory(USER_STORE_ETC_DIR))
    {
        fs->createDirectory(USER_STORE_ETC_DIR);
    }

    return &__user_store_service;
}

static user_record_t makeRecord(const char *name, uint16_t uid, uint16_t gid)
{
    user_record_t record;
    record.m_username = name;
    record.m_uid = uid;
    record.m_gid = gid;
    record.m_home = "/";
    record.m_shell = "/bin/sh";
    return record;
}

/**
 * Leave no account behind, so a suite rerun starts from the same place.
 */
static void forget(const char *name)
{
    __user_store_service.removeUser(name);
}

static pdiutil::string slurp(const char *path)
{
    pdiutil::string out;
    __i_fs.readFile(path, 128, [&out](char *chunk, uint32_t len) {
        for (uint32_t i = 0; i < len; i++)
        {
            out += chunk[i];
        }
        return true;
    });
    return out;
}

TEST(userstore, an_added_user_is_found_by_name)
{
    UserStoreService *users = store();
    forget("alice");

    ASSERT_TRUE(users->addUser(makeRecord("alice", 1001, 1001), "alicepass"));

    user_record_t found;
    ASSERT_TRUE(users->findUserByName("alice", found));
    ASSERT_STREQ(found.m_username.c_str(), "alice");
    ASSERT_EQ(found.m_uid, 1001);
    ASSERT_EQ(found.m_gid, 1001);

    forget("alice");
}

TEST(userstore, an_added_user_is_found_by_uid)
{
    UserStoreService *users = store();
    forget("bob");
    users->addUser(makeRecord("bob", 1002, 1002), "bobpass");

    user_record_t found;
    ASSERT_TRUE(users->findUserByUid(1002, found));
    ASSERT_STREQ(found.m_username.c_str(), "bob");

    forget("bob");
}

TEST(userstore, the_home_and_shell_survive_the_round_trip)
{
    UserStoreService *users = store();
    forget("carol");

    user_record_t record = makeRecord("carol", 1003, 1003);
    record.m_home = "/var";
    record.m_shell = "/bin/pdish";
    ASSERT_TRUE(users->addUser(record, "carolpass"));

    user_record_t found;
    ASSERT_TRUE(users->findUserByName("carol", found));
    ASSERT_STREQ(found.m_home.c_str(), "/var");
    ASSERT_STREQ(found.m_shell.c_str(), "/bin/pdish");

    forget("carol");
}

TEST(userstore, an_unknown_user_is_not_found)
{
    UserStoreService *users = store();
    user_record_t found;

    ASSERT_FALSE(users->findUserByName("nobody", found));
}

TEST(userstore, an_unknown_uid_is_not_found)
{
    UserStoreService *users = store();
    user_record_t found;

    ASSERT_FALSE(users->findUserByUid(60000, found));
}

TEST(userstore, a_user_without_a_name_is_refused)
{
    UserStoreService *users = store();
    user_record_t record = makeRecord("", 1004, 1004);

    ASSERT_FALSE(users->addUser(record, "somepass"));
}

TEST(userstore, a_name_carrying_the_field_separator_cannot_forge_a_record)
{
    UserStoreService *users = store();
    forget("ghost");

    ASSERT_FALSE(users->addUser(makeRecord("ghost:x:0:0::/:/bin/sh", 1007, 1007), "ghostpass"));

    user_record_t found;
    ASSERT_FALSE(users->findUserByName("ghost", found));
    ASSERT_FALSE(slurp(USER_STORE_PASSWD_PATH).find("ghost") != pdiutil::string::npos);
}

TEST(userstore, a_name_outside_the_portable_set_is_refused)
{
    UserStoreService *users = store();
    const char *names[] = {"two words", "line\nbreak", "quo\"te", "back\\slash", "-dash", "tab\there", "semi;colon"};

    for (const char *name : names)
    {
        ASSERT_FALSE(users->addUser(makeRecord(name, 1008, 1008), "somepass"));
        user_record_t found;
        ASSERT_FALSE(users->findUserByName(name, found));
    }
}

TEST(userstore, a_name_too_long_for_its_record_is_refused)
{
    UserStoreService *users = store();
    pdiutil::string name(USER_STORE_MAX_USERNAME_LEN, 'n');

    ASSERT_FALSE(users->addUser(makeRecord(name.c_str(), 1009, 1009), "somepass"));

    pdiutil::string fits(USER_STORE_MAX_USERNAME_LEN - 1, 'n');
    forget(fits.c_str());
    ASSERT_TRUE(users->addUser(makeRecord(fits.c_str(), 1009, 1009), "somepass"));
    forget(fits.c_str());
}

TEST(userstore, a_portable_name_with_digits_dots_and_dashes_is_accepted)
{
    UserStoreService *users = store();
    const char *names[] = {"svc-1", "a.b_c", "_daemon", "User9"};

    for (const char *name : names)
    {
        forget(name);
        ASSERT_TRUE(users->addUser(makeRecord(name, 1010, 1010), "somepass"));
        user_record_t found;
        ASSERT_TRUE(users->findUserByName(name, found));
        forget(name);
    }
}

TEST(userstore, adding_the_same_name_twice_is_refused)
{
    UserStoreService *users = store();
    forget("dave");
    ASSERT_TRUE(users->addUser(makeRecord("dave", 1005, 1005), "davepass"));

    ASSERT_FALSE(users->addUser(makeRecord("dave", 1006, 1006), "otherpass"));

    user_record_t found;
    ASSERT_TRUE(users->findUserByName("dave", found));
    ASSERT_EQ(found.m_uid, 1005);

    forget("dave");
}

TEST(userstore, the_password_verifies)
{
    UserStoreService *users = store();
    forget("erin");
    users->addUser(makeRecord("erin", 1007, 1007), "erinpass");

    ASSERT_TRUE(users->verifyPassword("erin", "erinpass"));

    forget("erin");
}

TEST(userstore, a_wrong_password_is_rejected)
{
    UserStoreService *users = store();
    forget("frank");
    users->addUser(makeRecord("frank", 1008, 1008), "frankpass");

    ASSERT_FALSE(users->verifyPassword("frank", "frankpas"));
    ASSERT_FALSE(users->verifyPassword("frank", "frankpassx"));
    ASSERT_FALSE(users->verifyPassword("frank", ""));

    forget("frank");
}

TEST(userstore, an_unknown_user_never_verifies)
{
    UserStoreService *users = store();
    ASSERT_FALSE(users->verifyPassword("ghost", "anything"));
}

TEST(userstore, a_changed_password_replaces_the_old_one)
{
    UserStoreService *users = store();
    forget("grace");
    users->addUser(makeRecord("grace", 1009, 1009), "firstpass");

    ASSERT_TRUE(users->setPassword("grace", "secondpass"));
    ASSERT_TRUE(users->verifyPassword("grace", "secondpass"));
    ASSERT_FALSE(users->verifyPassword("grace", "firstpass"));

    forget("grace");
}

TEST(userstore, a_removed_user_is_gone_from_both_files)
{
    UserStoreService *users = store();
    forget("henry");
    users->addUser(makeRecord("henry", 1010, 1010), "henrypass");

    ASSERT_TRUE(users->removeUser("henry"));

    user_record_t found;
    ASSERT_FALSE(users->findUserByName("henry", found));
    ASSERT_FALSE(users->verifyPassword("henry", "henrypass"));
    ASSERT_TRUE(slurp(USER_STORE_SHADOW_PATH).find("henry") == pdiutil::string::npos);
}

TEST(userstore, removing_a_user_that_is_not_there_reports_failure)
{
    UserStoreService *users = store();
    ASSERT_FALSE(users->removeUser("neverexisted"));
}

TEST(userstore, one_removal_leaves_the_other_accounts_alone)
{
    UserStoreService *users = store();
    forget("ivy");
    forget("jack");
    users->addUser(makeRecord("ivy", 1011, 1011), "ivypass");
    users->addUser(makeRecord("jack", 1012, 1012), "jackpass");

    ASSERT_TRUE(users->removeUser("ivy"));

    user_record_t found;
    ASSERT_FALSE(users->findUserByName("ivy", found));
    ASSERT_TRUE(users->findUserByName("jack", found));
    ASSERT_TRUE(users->verifyPassword("jack", "jackpass"));

    forget("jack");
}

TEST(userstore, the_record_is_written_in_the_passwd_format)
{
    UserStoreService *users = store();
    forget("kate");
    users->addUser(makeRecord("kate", 1013, 1014), "katepass");

    pdiutil::string passwd = slurp(USER_STORE_PASSWD_PATH);
    ASSERT_TRUE(passwd.find("kate:") != pdiutil::string::npos);
    ASSERT_TRUE(passwd.find(":1013:1014:") != pdiutil::string::npos);

    forget("kate");
}

TEST(userstore, the_plaintext_never_reaches_the_disk)
{
    UserStoreService *users = store();
    forget("liam");
    users->addUser(makeRecord("liam", 1015, 1015), "liamsecretpw");

    ASSERT_TRUE(slurp(USER_STORE_PASSWD_PATH).find("liamsecretpw") == pdiutil::string::npos);
    ASSERT_TRUE(slurp(USER_STORE_SHADOW_PATH).find("liamsecretpw") == pdiutil::string::npos);

    forget("liam");
}

TEST(userstore, the_same_password_hashes_differently_for_two_users)
{
    UserStoreService *users = store();
    forget("mia");
    forget("noah");
    users->addUser(makeRecord("mia", 1016, 1016), "identical");
    users->addUser(makeRecord("noah", 1017, 1017), "identical");

    pdiutil::string shadow = slurp(USER_STORE_SHADOW_PATH);
    pdiutil::string::size_type mialine = shadow.find("mia:");
    pdiutil::string::size_type noahline = shadow.find("noah:");
    ASSERT_TRUE(mialine != pdiutil::string::npos);
    ASSERT_TRUE(noahline != pdiutil::string::npos);

    pdiutil::string miahash = shadow.substr(mialine, shadow.find('\n', mialine) - mialine);
    pdiutil::string noahhash = shadow.substr(noahline, shadow.find('\n', noahline) - noahline);
    miahash.erase(0, miahash.find(':'));
    noahhash.erase(0, noahhash.find(':'));

    ASSERT_STRNE(miahash.c_str(), noahhash.c_str());
    ASSERT_TRUE(users->verifyPassword("mia", "identical"));
    ASSERT_TRUE(users->verifyPassword("noah", "identical"));

    forget("mia");
    forget("noah");
}

TEST(userstore, an_owning_id_resolves_to_its_name)
{
    UserStoreService *users = store();
    forget("olive");
    users->addUser(makeRecord("olive", 1018, 1018), "olivepass");

    pdiutil::string owner;
    pdiutil::string group;
    users->resolveOwnerNames(1018, 1018, owner, group);

    ASSERT_STREQ(owner.c_str(), "olive");
    ASSERT_STREQ(group.c_str(), "olive");

    forget("olive");
}

TEST(userstore, an_id_with_no_record_resolves_to_its_number)
{
    UserStoreService *users = store();
    pdiutil::string owner;
    pdiutil::string group;

    users->resolveOwnerNames(4242, 4242, owner, group);

    ASSERT_STREQ(owner.c_str(), "4242");
    ASSERT_STREQ(group.c_str(), "4242");
}

TEST(userstore, a_differing_group_resolves_on_its_own)
{
    UserStoreService *users = store();
    forget("peggy");
    forget("quinn");
    users->addUser(makeRecord("peggy", 1019, 1019), "peggypass");
    users->addUser(makeRecord("quinn", 1020, 1020), "quinnpass");

    pdiutil::string owner;
    pdiutil::string group;
    users->resolveOwnerNames(1019, 1020, owner, group);

    ASSERT_STREQ(owner.c_str(), "peggy");
    ASSERT_STREQ(group.c_str(), "quinn");

    forget("peggy");
    forget("quinn");
}

namespace
{

/**
 * Appends a line to a store file longer than the heap will hold whole while
 * capped to a 2 KB block, and hands back the file as it then stands.
 */
pdiutil::string appendLongLine(const char *path)
{
    pdiutil::string line = "zlong:";
    line.append(6000, 'x');
    line += '\n';
    __i_fs.beginPrivileged();
    __i_fs.writeFile(path, line.c_str(), line.size(), true);
    __i_fs.endPrivileged();
    return slurp(path);
}

void dropLongLine(const char *path)
{
    __i_fs.beginPrivileged();
    pdiutil::string content = slurp(path);
    pdiutil::string::size_type at = content.find("zlong:");
    if (pdiutil::string::npos != at)
    {
        pdiutil::string::size_type end = content.find('\n', at);
        content.erase(at, (pdiutil::string::npos == end) ? pdiutil::string::npos : end - at + 1);
        file_info_t meta;
        bool havemeta = (PDI_OK == __i_fs.getFileMeta(path, meta));
        __i_fs.deleteFile(path);
        __i_fs.createFile(path, content.c_str());
        if (havemeta)
        {
            __i_fs.setFilePermissions(path, meta.m_perms);
        }
    }
    __i_fs.endPrivileged();
}

} // namespace

TEST(userstore, a_password_change_that_cannot_copy_a_line_whole_leaves_the_store_as_it_was)
{
    UserStoreService *users = store();
    forget("ursula");
    ASSERT_TRUE(users->addUser(makeRecord("ursula", 1031, 1031), "firstpass"));

    __i_fs.beginPrivileged();
    pdiutil::string before = appendLongLine(USER_STORE_SHADOW_PATH);
    pdiutil::enable_heap_check();
    __i_dvc_ctrl.capMaxFreeBlock(2048);
    bool changed = users->setPassword("ursula", "secondpass");
    __i_dvc_ctrl.capMaxFreeBlock(0);
    pdiutil::string after = slurp(USER_STORE_SHADOW_PATH);
    __i_fs.endPrivileged();

    ASSERT_FALSE(changed);
    ASSERT_TRUE(before == after);
    ASSERT_TRUE(users->verifyPassword("ursula", "firstpass"));

    dropLongLine(USER_STORE_SHADOW_PATH);
    forget("ursula");
}

TEST(userstore, a_removal_that_cannot_copy_a_line_whole_leaves_the_store_as_it_was)
{
    UserStoreService *users = store();
    forget("victor");
    ASSERT_TRUE(users->addUser(makeRecord("victor", 1032, 1032), "victorpass"));

    pdiutil::string before = appendLongLine(USER_STORE_PASSWD_PATH);
    pdiutil::enable_heap_check();
    __i_dvc_ctrl.capMaxFreeBlock(2048);
    bool removed = users->removeUser("victor");
    __i_dvc_ctrl.capMaxFreeBlock(0);

    ASSERT_FALSE(removed);
    ASSERT_TRUE(before == slurp(USER_STORE_PASSWD_PATH));

    dropLongLine(USER_STORE_PASSWD_PATH);
    forget("victor");
}

TEST(userstore, a_password_change_keeps_every_other_record_and_replaces_its_own_in_place)
{
    UserStoreService *users = store();
    forget("wendy");
    forget("xavier");
    ASSERT_TRUE(users->addUser(makeRecord("wendy", 1033, 1033), "wendypass"));
    ASSERT_TRUE(users->addUser(makeRecord("xavier", 1034, 1034), "xavierpass"));

    ASSERT_TRUE(users->setPassword("wendy", "wendynew"));

    ASSERT_TRUE(users->verifyPassword("wendy", "wendynew"));
    ASSERT_FALSE(users->verifyPassword("wendy", "wendypass"));
    ASSERT_TRUE(users->verifyPassword("xavier", "xavierpass"));
    __i_fs.beginPrivileged();
    pdiutil::string shadow = slurp(USER_STORE_SHADOW_PATH);
    __i_fs.endPrivileged();
    ASSERT_TRUE(shadow.find("wendy:") == shadow.rfind("wendy:"));

    forget("wendy");
    forget("xavier");
}
