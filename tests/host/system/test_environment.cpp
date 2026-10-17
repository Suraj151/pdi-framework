/***************************** Environment Tests ******************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Covers the three tiers a variable can come from and which one wins: facts the
session answers for itself, variables set for this session only, and the base
file every session shares.

Author          : Suraj I.
created Date    : 5th Sep 2026
******************************************************************************/

#include <MountedStack.h>
#include <ShellHarness.h>
#include <pditest.h>

#include <service_provider/session/Environment.h>
#include <service_provider/session/SessionManager.h>
#include <helpers/ConfigHelper.h>

#ifdef ENABLE_CMD_SERVICE

namespace
{

void clearBaseEnv()
{
#ifdef ENABLE_STORAGE_SERVICE
    VfsDispatcher *fs = pditest::mountedVfs();
    fs->beginPrivileged();
    if (fs->isFileExist(ENV_FILE_PATH))
    {
        fs->deleteFile(ENV_FILE_PATH);
    }
    fs->endPrivileged();
#endif
}

std::string envValue(const char *key)
{
    pdiutil::string out;
    Environment::get(key, out);
    return std::string(out.c_str());
}

} // namespace

TEST(environment, a_name_must_look_like_a_variable)
{
    ASSERT_TRUE(Environment::isValidName("PATH"));
    ASSERT_TRUE(Environment::isValidName("_hidden"));
    ASSERT_TRUE(Environment::isValidName("a1_B2"));

    ASSERT_TRUE(!Environment::isValidName("1leading"));
    ASSERT_TRUE(!Environment::isValidName("has space"));
    ASSERT_TRUE(!Environment::isValidName("has-dash"));
    ASSERT_TRUE(!Environment::isValidName(""));
    ASSERT_TRUE(!Environment::isValidName(nullptr));
}

TEST(environment, a_session_variable_reads_back)
{
    pditest::Shell sh;
    clearBaseEnv();

    ASSERT_EQ((int)Environment::set("GREETING", "hello"), (int)PDI_OK);

    pdiutil::string value;
    ASSERT_TRUE(Environment::get("GREETING", value));
    ASSERT_STREQ(value.c_str(), "hello");
}

TEST(environment, setting_a_variable_again_replaces_it)
{
    pditest::Shell sh;
    clearBaseEnv();

    Environment::set("GREETING", "hello");
    ASSERT_EQ((int)Environment::set("GREETING", "goodbye"), (int)PDI_OK);

    pdiutil::string value;
    Environment::get("GREETING", value);
    ASSERT_STREQ(value.c_str(), "goodbye");
}

TEST(environment, an_empty_value_is_a_value)
{
    pditest::Shell sh;
    clearBaseEnv();

    ASSERT_EQ((int)Environment::set("EMPTY", ""), (int)PDI_OK);

    pdiutil::string value;
    ASSERT_TRUE(Environment::get("EMPTY", value));
    ASSERT_TRUE(value.empty());
}

TEST(environment, unset_removes_only_what_it_names)
{
    pditest::Shell sh;
    clearBaseEnv();

    Environment::set("ONE", "1");
    Environment::set("TWO", "2");

    ASSERT_TRUE(Environment::unset("ONE"));
    ASSERT_TRUE(!Environment::unset("ONE"));

    pdiutil::string value;
    ASSERT_TRUE(!Environment::get("ONE", value));
    ASSERT_TRUE(Environment::get("TWO", value));
}

TEST(environment, a_name_that_was_never_set_is_absent)
{
    pditest::Shell sh;
    clearBaseEnv();

    pdiutil::string value;
    ASSERT_TRUE(!Environment::get("NOTHING_HERE", value));
    ASSERT_TRUE(value.empty());
}

TEST(environment, the_session_answers_for_its_own_facts)
{
    pditest::Shell sh;

    pdiutil::string value;
    ASSERT_TRUE(Environment::get("PWD", value));
    ASSERT_STREQ(value.c_str(), SessionManager::getPWD().c_str());
}

TEST(environment, a_derived_value_follows_the_session_rather_than_going_stale)
{
    pditest::Shell sh;

    pdiutil::string before;
    Environment::get("PWD", before);

    sh.run("mkdir /envdir");
    sh.run("cd /envdir");

    pdiutil::string after;
    Environment::get("PWD", after);

    ASSERT_TRUE(!(before == after));
    ASSERT_STREQ(after.c_str(), SessionManager::getPWD().c_str());
}

TEST(environment, a_derived_name_cannot_be_assigned)
{
    pditest::Shell sh;

    ASSERT_EQ((int)Environment::set("PWD", "/somewhere"), (int)CMD_ERROR_PERM);
    ASSERT_EQ((int)Environment::set("USER", "someone"), (int)CMD_ERROR_PERM);

    pdiutil::string value;
    Environment::get("PWD", value);
    ASSERT_STREQ(value.c_str(), SessionManager::getPWD().c_str());
}

TEST(environment, a_malformed_name_is_refused)
{
    pditest::Shell sh;

    ASSERT_EQ((int)Environment::set("has space", "x"), (int)CMD_ERROR_INVAL);
    ASSERT_EQ((int)Environment::set("1leading", "x"), (int)CMD_ERROR_INVAL);
}

TEST(environment, the_session_holds_a_bounded_number_of_variables)
{
    pditest::Shell sh;
    clearBaseEnv();

    for (uint8_t i = 0; i < ENV_SESSION_MAX; i++)
    {
        pdiutil::string name = "VAR";
        name += (char)('0' + i);
        ASSERT_EQ((int)Environment::set(name.c_str(), "x"), (int)PDI_OK);
    }

    ASSERT_EQ((int)Environment::set("ONE_TOO_MANY", "x"), (int)CMD_ERROR_FAILED);
}

TEST(environment, listing_carries_the_derived_names)
{
    pditest::Shell sh;
    clearBaseEnv();

    pdiutil::vector<config_kv_t> all;
    Environment::list(all);

    bool sawpwd = false;
    bool sawuser = false;
    for (uint32_t i = 0; i < all.size(); i++)
    {
        if (all[i].m_key == pdiutil::string("PWD")) sawpwd = true;
        if (all[i].m_key == pdiutil::string("USER")) sawuser = true;
    }

    ASSERT_TRUE(sawpwd);
    ASSERT_TRUE(sawuser);
}

TEST(environment, a_logout_takes_the_session_variables_with_it)
{
    pditest::Shell sh;
    Environment::set("SECRET", "mine");

    sh.run("logout");

    pdiutil::string value;
    ASSERT_TRUE(!Environment::get("SECRET", value));
}

TEST(environment, switching_user_starts_from_a_clean_environment)
{
    pditest::Shell sh;
    Environment::set("SECRET", "mine");

    sh.run("su pdiStack pdiStack@123");

    pdiutil::string value;
    ASSERT_TRUE(!Environment::get("SECRET", value));
}

TEST(environment, clearing_the_session_leaves_the_base_file_alone)
{
    pditest::Shell sh;

#ifdef ENABLE_STORAGE_SERVICE
    Environment::setPersistent("SHARED", "base");
#endif
    Environment::set("SHARED", "session");

    Environment::clearSession();

    pdiutil::string value;
#ifdef ENABLE_STORAGE_SERVICE
    ASSERT_TRUE(Environment::get("SHARED", value));
    ASSERT_STREQ(value.c_str(), "base");
    clearBaseEnv();
#else
    ASSERT_TRUE(!Environment::get("SHARED", value));
#endif
}

TEST(environment, a_variable_expands_in_a_command_line)
{
    pditest::Shell sh;
    Environment::set("GREETING", "hello");

    ASSERT_TRUE(std::string(sh.run("echo $GREETING")).find("hello") != std::string::npos);
}

TEST(environment, a_name_that_is_not_set_expands_to_nothing)
{
    pditest::Shell sh;
    Environment::unset("ABSENT");

    std::string out = sh.run("echo [$ABSENT]");
    ASSERT_TRUE(out.find("[]") != std::string::npos);
}

TEST(environment, a_single_quoted_variable_is_left_alone)
{
    pditest::Shell sh;
    Environment::set("GREETING", "hello");

    std::string out = sh.run("echo '$GREETING'");
    ASSERT_TRUE(out.find("$GREETING") != std::string::npos);
    ASSERT_TRUE(out.find("hello") == std::string::npos);
}

TEST(environment, a_double_quoted_variable_still_expands)
{
    pditest::Shell sh;
    Environment::set("GREETING", "hello");

    ASSERT_TRUE(std::string(sh.run("echo \"$GREETING\"")).find("hello") != std::string::npos);
}

TEST(environment, braces_let_a_name_run_straight_into_text)
{
    pditest::Shell sh;
    Environment::set("NUM", "4");

    std::string out = sh.run("echo ${NUM}th");
    ASSERT_TRUE(out.find("4th") != std::string::npos);
}

TEST(environment, a_default_stands_in_for_a_name_that_is_unset_or_empty)
{
    pditest::Shell sh;
    Environment::unset("ABSENT");
    Environment::set("EMPTY", "");
    Environment::set("GREETING", "hello");

    ASSERT_TRUE(std::string(sh.run("echo [${ABSENT:-fallback}]")).find("[fallback]") != std::string::npos);
    ASSERT_TRUE(std::string(sh.run("echo [${EMPTY:-fallback}]")).find("[fallback]") != std::string::npos);
    ASSERT_TRUE(std::string(sh.run("echo [${GREETING:-fallback}]")).find("[hello]") != std::string::npos);
}

TEST(environment, a_default_may_hold_blanks_and_names)
{
    pditest::Shell sh;
    Environment::unset("ABSENT");
    Environment::set("GREETING", "hello");

    ASSERT_TRUE(std::string(sh.run("echo [${ABSENT:-a b}]")).find("[a b]") != std::string::npos);
    ASSERT_TRUE(std::string(sh.run("echo [${ABSENT:-$GREETING}]")).find("[hello]") != std::string::npos);
}

TEST(environment, a_hash_gives_the_length_of_a_value)
{
    pditest::Shell sh;
    Environment::set("GREETING", "hello");
    Environment::unset("ABSENT");

    ASSERT_TRUE(std::string(sh.run("echo [${#GREETING}]")).find("[5]") != std::string::npos);
    ASSERT_TRUE(std::string(sh.run("echo [${#ABSENT}]")).find("[0]") != std::string::npos);
}

TEST(environment, braces_in_single_quotes_and_unclosed_braces_stay_as_typed)
{
    pditest::Shell sh;
    Environment::set("NUM", "4");

    ASSERT_TRUE(std::string(sh.run("echo '${NUM}'")).find("${NUM}") != std::string::npos);
    ASSERT_TRUE(std::string(sh.run("echo ${NUM")).find("${NUM") != std::string::npos);
}

TEST(environment, arithmetic_follows_the_usual_precedence)
{
    pditest::Shell sh;

    ASSERT_TRUE(std::string(sh.run("echo [$((1 + 2 * 3))]")).find("[7]") != std::string::npos);
    ASSERT_TRUE(std::string(sh.run("echo [$(( (7 + 1) / 3 ))]")).find("[2]") != std::string::npos);
    ASSERT_TRUE(std::string(sh.run("echo [$((-4 % 3))]")).find("[-1]") != std::string::npos);
}

TEST(environment, arithmetic_reads_names_bare_or_with_a_dollar)
{
    pditest::Shell sh;
    Environment::set("NUM", "5");
    Environment::unset("ABSENT");

    ASSERT_TRUE(std::string(sh.run("echo [$((NUM - 2))]")).find("[3]") != std::string::npos);
    ASSERT_TRUE(std::string(sh.run("echo [$(( $NUM * 2 ))]")).find("[10]") != std::string::npos);
    ASSERT_TRUE(std::string(sh.run("echo [$((ABSENT + 1))]")).find("[1]") != std::string::npos);
}

TEST(environment, arithmetic_that_cannot_be_worked_out_stays_as_typed)
{
    pditest::Shell sh;
    Environment::set("WORD", "abc");

    ASSERT_TRUE(std::string(sh.run("echo [$((1 / 0))]")).find("[$((1 / 0))]") != std::string::npos);
    ASSERT_TRUE(std::string(sh.run("echo [$((WORD + 1))]")).find("[$((WORD + 1))]") != std::string::npos);
    ASSERT_TRUE(std::string(sh.run("echo '$((1 + 1))'")).find("$((1 + 1))") != std::string::npos);
}

TEST(environment, arithmetic_counts_a_loop_in_a_script)
{
    pditest::mountedVfs();
    pditest::Shell sh;
    Environment::set("I", "0");

    sh.run("echo 'while test $I -lt 3' > /wt_count.sh");
    sh.run("echo 'echo pass$I' >> /wt_count.sh");
    sh.run("echo 'export I=$((I + 1))' >> /wt_count.sh");
    sh.run("echo done >> /wt_count.sh");
    std::string out = sh.run("source /wt_count.sh");
    sh.run("rm /wt_count.sh");

    ASSERT_TRUE(out.find("pass0") != std::string::npos);
    ASSERT_TRUE(out.find("pass2") != std::string::npos);
    ASSERT_TRUE(out.find("pass3") == std::string::npos);
    ASSERT_STREQ(envValue("I").c_str(), "3");
}

TEST(environment, a_variable_ends_where_the_name_does)
{
    pditest::Shell sh;
    Environment::set("GREETING", "hello");

    ASSERT_TRUE(std::string(sh.run("echo pre$GREETING-post")).find("prehello-post") != std::string::npos);
}

TEST(environment, two_variables_run_together)
{
    pditest::Shell sh;
    Environment::set("GREETING", "hello");

    ASSERT_TRUE(std::string(sh.run("echo $GREETING$GREETING")).find("hellohello") != std::string::npos);
}

TEST(environment, a_derived_variable_expands_to_what_the_session_holds)
{
    pditest::Shell sh;

    sh.run("mkdir /expdir");
    sh.run("cd /expdir");

    ASSERT_TRUE(std::string(sh.run("echo $PWD")).find("/expdir") != std::string::npos);
}

TEST(environment, the_exit_status_still_expands_beside_variables)
{
    pditest::Shell sh;
    Environment::set("GREETING", "hello");

    std::string out = sh.run("echo $GREETING $?");
    ASSERT_TRUE(out.find("hello") != std::string::npos);
    ASSERT_TRUE(out.find("0") != std::string::npos);
}

#ifdef ENABLE_STORAGE_SERVICE

TEST(environment, the_base_file_is_seeded_with_its_defaults)
{
    pditest::Shell sh;
    clearBaseEnv();

    Environment::ensureBaseFile();
    ASSERT_TRUE(pditest::mountedVfs()->isFileExist(ENV_FILE_PATH));

    pdiutil::string value;
    ASSERT_TRUE(getConfigValue(ENV_FILE_PATH, ENV_KEY_HOME, value));
    ASSERT_STREQ(value.c_str(), FILE_SEPARATOR);

    clearBaseEnv();
}

TEST(environment, a_variable_in_the_base_file_is_visible)
{
    pditest::Shell sh;
    clearBaseEnv();

    ASSERT_EQ((int)Environment::setPersistent("SHARED", "base"), (int)PDI_OK);

    pdiutil::string value;
    ASSERT_TRUE(Environment::get("SHARED", value));
    ASSERT_STREQ(value.c_str(), "base");

    clearBaseEnv();
}

TEST(environment, a_session_variable_outranks_the_base_file)
{
    pditest::Shell sh;
    clearBaseEnv();

    Environment::setPersistent("SHARED", "base");
    Environment::set("SHARED", "session");

    pdiutil::string value;
    Environment::get("SHARED", value);
    ASSERT_STREQ(value.c_str(), "session");

    clearBaseEnv();
}

TEST(environment, dropping_a_session_variable_uncovers_the_base_one)
{
    pditest::Shell sh;
    clearBaseEnv();

    Environment::setPersistent("SHARED", "base");
    Environment::set("SHARED", "session");
    Environment::unset("SHARED");

    pdiutil::string value;
    ASSERT_TRUE(Environment::get("SHARED", value));
    ASSERT_STREQ(value.c_str(), "base");

    clearBaseEnv();
}

TEST(environment, unsetting_a_session_variable_leaves_the_base_file_alone)
{
    pditest::Shell sh;
    clearBaseEnv();

    Environment::setPersistent("SHARED", "base");
    Environment::set("SHARED", "session");
    Environment::unset("SHARED");

    pdiutil::string onfile;
    ASSERT_TRUE(getConfigValue(ENV_FILE_PATH, "SHARED", onfile));
    ASSERT_STREQ(onfile.c_str(), "base");

    clearBaseEnv();
}

TEST(environment, a_base_variable_can_be_dropped_from_the_file)
{
    pditest::Shell sh;
    clearBaseEnv();

    Environment::setPersistent("SHARED", "base");
    ASSERT_TRUE(Environment::unsetPersistent("SHARED"));
    ASSERT_TRUE(!Environment::unsetPersistent("SHARED"));

    pdiutil::string value;
    ASSERT_TRUE(!Environment::get("SHARED", value));

    clearBaseEnv();
}

TEST(environment, the_base_file_shows_once_in_a_listing_it_shares_a_name_with)
{
    pditest::Shell sh;
    clearBaseEnv();

    Environment::setPersistent("SHARED", "base");
    Environment::set("SHARED", "session");

    pdiutil::vector<config_kv_t> all;
    Environment::list(all);

    uint8_t seen = 0;
    pdiutil::string value;
    for (uint32_t i = 0; i < all.size(); i++)
    {
        if (all[i].m_key == pdiutil::string("SHARED"))
        {
            seen++;
            value = all[i].m_value;
        }
    }

    ASSERT_EQ((int)seen, 1);
    ASSERT_STREQ(value.c_str(), "session");

    clearBaseEnv();
}

#endif

#endif
