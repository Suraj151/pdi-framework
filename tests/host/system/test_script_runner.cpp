/**************************** Script Runner Tests *****************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Covers the two things a shell cannot reach: what an unattended script does
with a line that asks for input, and how the boot script decides whose
identity it runs under.

Author          : Suraj I.
created Date    : 5th Sep 2026
******************************************************************************/

#include <MountedStack.h>
#include <ShellHarness.h>
#include <pditest.h>

#include <service_provider/cmd/ScriptRunner.h>
#include <service_provider/cmd/CommandLineServiceProvider.h>
#include <service_provider/session/SessionManager.h>

#ifdef ENABLE_SCRIPT_RUNNER

namespace
{

const char SCRIPT_PATH[] = "/hostscript.sh";
const char SCRIPT_USER[] = "scriptowner";
const uint16_t SCRIPT_UID = 1234;
const uint16_t ABSENT_UID = 4242;

/**
 * A user this file owns, so the directive tests do not depend on whichever
 * accounts the files that ran before them happened to leave behind.
 */
void addScriptUser()
{
    user_record_t rec;
    rec.m_username = SCRIPT_USER;
    rec.m_uid = SCRIPT_UID;
    rec.m_gid = SCRIPT_UID;
    rec.m_home = FILE_SEPARATOR;
    __user_store_service.addUser(rec, "scriptpass");
}

void removeScriptUser()
{
    __user_store_service.removeUser(SCRIPT_USER);
}

void writeScript(const char *body)
{
    VfsDispatcher *fs = pditest::mountedVfs();
    fs->beginPrivileged();
    if (fs->isFileExist(SCRIPT_PATH))
    {
        fs->deleteFile(SCRIPT_PATH);
    }
    fs->createFile(SCRIPT_PATH, body, (int64_t)strlen(body));
    fs->endPrivileged();
}

void removeScript()
{
    VfsDispatcher *fs = pditest::mountedVfs();
    fs->beginPrivileged();
    if (fs->isFileExist(SCRIPT_PATH))
    {
        fs->deleteFile(SCRIPT_PATH);
    }
    fs->endPrivileged();
}

const char LOOP_PATH[] = "/hostloop.txt";

/**
 * What a loop body appended, which is how a pass that ran is told apart from
 * one that was skipped and how many of them is told from which.
 */
std::string loopOutput()
{
    std::string out;
    __i_fs.readFile(LOOP_PATH, 128, [&](char *data, uint32_t size) -> bool {
        out.append(data, size);
        return true;
    });
    return out;
}

void removeLoopOutput()
{
    VfsDispatcher *fs = pditest::mountedVfs();
    fs->beginPrivileged();
    if (fs->isFileExist(LOOP_PATH))
    {
        fs->deleteFile(LOOP_PATH);
    }
    fs->endPrivileged();
}

/**
 * Runs a for body that records each pass, so a test states the list and reads
 * back exactly the words the loop bound.
 */
std::string runLoop(const char *script)
{
    removeLoopOutput();
    writeScript(script);
    ScriptRunner::run(SCRIPT_PATH, true);
    std::string out = loopOutput();
    removeScript();
    removeLoopOutput();
    Environment::unset("FORVAR");
    return out;
}

/**
 * What the script answered, for the cases where the interest is a malformed
 * block being reported rather than what its body did.
 */
pdi_err_t runFor(const char *script)
{
    removeLoopOutput();
    writeScript(script);
    pdi_err_t res = ScriptRunner::run(SCRIPT_PATH, true);
    removeScript();
    removeLoopOutput();
    Environment::unset("FORVAR");
    return res;
}

/**
 * How many times a body recorded a pass, which is what tells a loop that ran
 * the right number of times from one that merely ran.
 */
size_t countOf(const std::string &hay, const char *needle)
{
    size_t n = 0;
    size_t at = 0;
    size_t len = strlen(needle);

    while ((at = hay.find(needle, at)) != std::string::npos)
    {
        n++;
        at += len;
    }

    return n;
}

} // namespace

TEST(script_runner, an_unattended_script_refuses_a_line_that_asks_for_input)
{
    pditest::Shell sh;

    // su with no arguments stops for a username. Nobody is watching an
    // unattended script, so waiting would hold the session forever and the
    // line after it would be read as the answer
    writeScript("su\r\necho pastthepromptline\r\n");

    pdi_err_t res = ScriptRunner::run(SCRIPT_PATH, true);

    ASSERT_EQ((int)res, (int)CMD_ERROR_CANCELED);
    ASSERT_TRUE(!ScriptRunner::pending());

    removeScript();
}

TEST(script_runner, an_unattended_script_leaves_nothing_waiting_behind)
{
    pditest::Shell sh;

    writeScript("su\r\n");
    ScriptRunner::run(SCRIPT_PATH, true);

    // the waiter is cancelled rather than left holding the session, so the
    // next line typed is run as a command instead of answering it
    std::string out = sh.run("echo stillashell");
    ASSERT_TRUE(out.find("stillashell") != std::string::npos);

    removeScript();
}

TEST(script_runner, an_attended_script_keeps_its_place_when_a_line_asks_for_input)
{
    pditest::Shell sh;

    writeScript("su\r\necho afterthepause\r\n");

    pdi_err_t res = ScriptRunner::run(SCRIPT_PATH, false);

    ASSERT_EQ((int)res, (int)CMD_ERROR_AGAIN);
    ASSERT_TRUE(ScriptRunner::pending());

    ScriptRunner::clearSession();
    removeScript();
}

TEST(script_runner, a_missing_file_is_not_reported_as_a_missing_command)
{
    pditest::Shell sh;

    pdi_err_t res = ScriptRunner::run("/nosuchscriptfile", true);

    ASSERT_EQ((int)res, (int)CMD_ERROR_FAILED);
    ASSERT_TRUE((int)res != (int)CMD_ERROR_NOENT);
}

TEST(script_runner, a_script_runs_its_lines_in_order)
{
    pditest::Shell sh;

    writeScript("export SCRIPTONE=first\r\nexport SCRIPTTWO=second\r\n");
    ScriptRunner::run(SCRIPT_PATH, true);

    pdiutil::string value;
    ASSERT_TRUE(Environment::get("SCRIPTONE", value));
    ASSERT_STREQ(value.c_str(), "first");
    ASSERT_TRUE(Environment::get("SCRIPTTWO", value));
    ASSERT_STREQ(value.c_str(), "second");

    Environment::unset("SCRIPTONE");
    Environment::unset("SCRIPTTWO");
    removeScript();
}

TEST(script_runner, carriage_returns_do_not_reach_the_command)
{
    pditest::Shell sh;

    // the on device editor writes CRLF, so a script saved with it must run
    // the same as one written with bare newlines
    writeScript("export CRLFVAR=clean\r\n");
    ScriptRunner::run(SCRIPT_PATH, true);

    pdiutil::string value;
    ASSERT_TRUE(Environment::get("CRLFVAR", value));
    ASSERT_STREQ(value.c_str(), "clean");

    Environment::unset("CRLFVAR");
    removeScript();
}

TEST(script_runner, the_depth_limit_stops_a_script_that_runs_itself)
{
    pditest::Shell sh;

    pdiutil::string body = CHARPTR_WRAP("source ");
    body += SCRIPT_PATH;
    body += CHARPTR_WRAP("\r\n");

    writeScript(body.c_str());

    ScriptRunner::run(SCRIPT_PATH, true);

    // it returns at all, and leaves nothing open behind it
    ASSERT_TRUE(!ScriptRunner::pending());

    removeScript();
}

TEST(script_runner, a_script_with_no_directive_runs_as_root)
{
    pditest::Shell sh;

    writeScript("echo plainscript\r\n");

    uint16_t uid = 4242;
    bool named = true;

    ASSERT_TRUE(ScriptRunner::declaredUid(SCRIPT_PATH, uid, named));
    ASSERT_TRUE(!named);
    ASSERT_EQ((int)uid, (int)USER_STORE_ROOT_UID);

    removeScript();
}

TEST(script_runner, a_directive_naming_a_real_user_is_taken)
{
    pditest::Shell sh;

    addScriptUser();
    writeScript("# UID 1234\r\necho ownedscript\r\n");

    uint16_t uid = 0;
    bool named = false;

    ASSERT_TRUE(ScriptRunner::declaredUid(SCRIPT_PATH, uid, named));
    ASSERT_TRUE(named);
    ASSERT_EQ((int)uid, (int)SCRIPT_UID);

    removeScript();
    removeScriptUser();
}

TEST(script_runner, a_directive_naming_nobody_stops_the_script_running)
{
    pditest::Shell sh;

    // an unresolvable identity is not quietly downgraded to root; the script
    // does not run at all
    removeScriptUser();
    writeScript("# UID 4242\r\necho mustnotrun\r\n");

    uint16_t uid = 0;
    bool named = false;

    ASSERT_TRUE(!ScriptRunner::declaredUid(SCRIPT_PATH, uid, named));
    ASSERT_TRUE(named);

    removeScript();
}

TEST(script_runner, a_directive_below_the_header_does_not_count)
{
    pditest::Shell sh;

    // a UID written in a comment further down is documentation, not policy,
    // and must not change who the script runs as
    writeScript("echo firstcommand\r\n# UID 4242\r\n");

    uint16_t uid = 4242;
    bool named = true;

    ASSERT_TRUE(ScriptRunner::declaredUid(SCRIPT_PATH, uid, named));
    ASSERT_TRUE(!named);
    ASSERT_EQ((int)uid, (int)USER_STORE_ROOT_UID);

    removeScript();
}

TEST(script_runner, a_directive_with_no_number_is_ignored)
{
    pditest::Shell sh;

    writeScript("# UID notanumber\r\necho stillruns\r\n");

    uint16_t uid = 4242;
    bool named = true;

    ASSERT_TRUE(ScriptRunner::declaredUid(SCRIPT_PATH, uid, named));
    ASSERT_TRUE(!named);

    removeScript();
}

TEST(script_runner, a_directive_after_blank_and_comment_lines_is_found)
{
    pditest::Shell sh;

    addScriptUser();
    writeScript("\r\n# what this script does\r\n#   UID 1234\r\necho found\r\n");

    uint16_t uid = 0;
    bool named = false;

    ASSERT_TRUE(ScriptRunner::declaredUid(SCRIPT_PATH, uid, named));
    ASSERT_TRUE(named);
    ASSERT_EQ((int)uid, (int)SCRIPT_UID);

    removeScript();
    removeScriptUser();
}

TEST(script_runner, clearing_the_session_abandons_a_pending_script)
{
    pditest::Shell sh;

    writeScript("su\r\necho neverreached\r\n");
    ScriptRunner::run(SCRIPT_PATH, false);
    ASSERT_TRUE(ScriptRunner::pending());

    ScriptRunner::clearSession();
    ASSERT_TRUE(!ScriptRunner::pending());

    removeScript();
}

TEST(script_runner, a_for_runs_its_body_once_for_each_word)
{
    pditest::Shell sh;

    std::string out = runLoop("for FORVAR in alpha beta gamma\r\n"
                              "echo $FORVAR >> /hostloop.txt\r\n"
                              "done\r\n");

    ASSERT_TRUE(out.find("alpha") != std::string::npos);
    ASSERT_TRUE(out.find("beta") != std::string::npos);
    ASSERT_TRUE(out.find("gamma") != std::string::npos);

    // the order of the list is the order of the passes, not just the set
    ASSERT_TRUE(out.find("alpha") < out.find("beta"));
    ASSERT_TRUE(out.find("beta") < out.find("gamma"));
}

TEST(script_runner, a_for_leaves_the_last_word_bound_after_its_done)
{
    pditest::Shell sh;

    removeLoopOutput();
    writeScript("for FORVAR in alpha beta gamma\r\necho $FORVAR >> /hostloop.txt\r\ndone\r\n");
    ScriptRunner::run(SCRIPT_PATH, true);

    pdiutil::string value;
    ASSERT_TRUE(Environment::get("FORVAR", value));
    ASSERT_STREQ(value.c_str(), "gamma");

    Environment::unset("FORVAR");
    removeScript();
    removeLoopOutput();
}

TEST(script_runner, a_for_over_an_empty_list_never_runs_its_body)
{
    pditest::Shell sh;

    std::string out = runLoop("for FORVAR in\r\n"
                              "echo rananyway >> /hostloop.txt\r\n"
                              "done\r\n");

    ASSERT_TRUE(out.find("rananyway") == std::string::npos);
}

TEST(script_runner, a_quoted_word_carrying_a_blank_is_one_pass)
{
    pditest::Shell sh;

    std::string out = runLoop("for FORVAR in \"one two\" three\r\n"
                              "echo [$FORVAR] >> /hostloop.txt\r\n"
                              "done\r\n");

    // quoting is what decides the word count, so this is two passes and not
    // three, and the first of them keeps its blank
    ASSERT_TRUE(out.find("[one two]") != std::string::npos);
    ASSERT_TRUE(out.find("[three]") != std::string::npos);
}

TEST(script_runner, an_unquoted_value_holding_several_words_is_several_passes)
{
    pditest::Shell sh;

    Environment::set("FORLIST", "red green blue");

    std::string out = runLoop("for FORVAR in $FORLIST\r\n"
                              "echo [$FORVAR] >> /hostloop.txt\r\n"
                              "done\r\n");

    ASSERT_TRUE(out.find("[red]") != std::string::npos);
    ASSERT_TRUE(out.find("[green]") != std::string::npos);
    ASSERT_TRUE(out.find("[blue]") != std::string::npos);
    ASSERT_TRUE(out.find("[red green blue]") == std::string::npos);

    Environment::unset("FORLIST");
}

TEST(script_runner, a_for_inside_an_if_that_is_not_taken_is_passed_over_whole)
{
    pditest::Shell sh;

    std::string out = runLoop("if test 1 -eq 2\r\n"
                              "for FORVAR in alpha beta\r\n"
                              "echo rananyway >> /hostloop.txt\r\n"
                              "done\r\n"
                              "fi\r\n"
                              "echo pastthefi >> /hostloop.txt\r\n");

    // the done inside the skipped if must not be read as closing the if, or
    // the line after fi would be skipped too
    ASSERT_TRUE(out.find("rananyway") == std::string::npos);
    ASSERT_TRUE(out.find("pastthefi") != std::string::npos);
}

TEST(script_runner, a_for_inside_an_if_that_is_taken_runs_every_pass)
{
    pditest::Shell sh;

    // the paired case of the test above: without it that one passes whether or
    // not for is a keyword at all, because a skipped block skips either way
    std::string out = runLoop("if test 1 -eq 1\r\n"
                              "for FORVAR in alpha beta\r\n"
                              "echo [$FORVAR] >> /hostloop.txt\r\n"
                              "done\r\n"
                              "fi\r\n"
                              "echo pastthefi >> /hostloop.txt\r\n");

    ASSERT_EQ(countOf(out, "[alpha]"), (size_t)1);
    ASSERT_EQ(countOf(out, "[beta]"), (size_t)1);
    ASSERT_TRUE(out.find("pastthefi") != std::string::npos);
}

TEST(script_runner, a_for_nested_in_a_for_runs_the_product_of_the_two)
{
    pditest::Shell sh;

    std::string out = runLoop("for FORVAR in a b\r\n"
                              "for FORINNER in x y\r\n"
                              "echo $FORVAR$FORINNER >> /hostloop.txt\r\n"
                              "done\r\n"
                              "done\r\n");

    ASSERT_TRUE(out.find("ax") != std::string::npos);
    ASSERT_TRUE(out.find("ay") != std::string::npos);
    ASSERT_TRUE(out.find("bx") != std::string::npos);
    ASSERT_TRUE(out.find("by") != std::string::npos);

    Environment::unset("FORINNER");
}

TEST(script_runner, a_for_with_no_in_fails_the_script)
{
    pditest::Shell sh;

    writeScript("for FORVAR alpha beta\r\necho neverreached\r\n");
    pdi_err_t res = ScriptRunner::run(SCRIPT_PATH, true);

    ASSERT_EQ((int)res, (int)CMD_ERROR_FAILED);
    ASSERT_TRUE(!ScriptRunner::pending());

    removeScript();
}

TEST(script_runner, a_for_cannot_bind_a_name_the_session_answers_itself)
{
    pditest::Shell sh;

    // PWD is derived, so a loop binding it would be writing a value the next
    // lookup would throw away
    writeScript("for PWD in alpha beta\r\necho neverreached\r\ndone\r\n");
    pdi_err_t res = ScriptRunner::run(SCRIPT_PATH, true);

    ASSERT_EQ((int)res, (int)CMD_ERROR_FAILED);

    removeScript();
}

TEST(script_runner, a_for_with_nothing_after_it_fails)
{
    pditest::Shell sh;
    ASSERT_EQ((int)runFor("for\r\ndone\r\n"), (int)CMD_ERROR_FAILED);
}

TEST(script_runner, a_for_naming_only_a_variable_fails)
{
    pditest::Shell sh;
    ASSERT_EQ((int)runFor("for FORVAR\r\ndone\r\n"), (int)CMD_ERROR_FAILED);
}

TEST(script_runner, a_for_binding_a_name_no_shell_would_accept_fails)
{
    pditest::Shell sh;
    ASSERT_EQ((int)runFor("for 9bad in a b\r\ndone\r\n"), (int)CMD_ERROR_FAILED);
    ASSERT_EQ((int)runFor("for bad-name in a b\r\ndone\r\n"), (int)CMD_ERROR_FAILED);
}

TEST(script_runner, a_for_whose_list_never_closes_its_quote_fails)
{
    pditest::Shell sh;
    ASSERT_EQ((int)runFor("for FORVAR in \"a b\r\ndone\r\n"), (int)CMD_ERROR_FAILED);
}

TEST(script_runner, a_for_whose_list_holds_an_operator_fails)
{
    pditest::Shell sh;

    // a list is words, so a pipe in it is a script meaning something the
    // runner cannot honour rather than a word spelled oddly
    ASSERT_EQ((int)runFor("for FORVAR in a | b\r\ndone\r\n"), (int)CMD_ERROR_FAILED);
}

TEST(script_runner, a_for_left_open_is_reported_rather_than_passing)
{
    pditest::Shell sh;

    std::string out = runLoop("for FORVAR in alpha beta\r\n"
                              "echo [$FORVAR] >> /hostloop.txt\r\n");

    // the body still ran, but a script whose block never closed must not be
    // reported as one that finished
    ASSERT_TRUE(out.find("[alpha]") != std::string::npos);
    ASSERT_EQ((int)runFor("for FORVAR in alpha beta\r\necho x\r\n"), (int)CMD_ERROR_FAILED);
}

TEST(script_runner, a_done_that_closes_no_loop_is_reported)
{
    pditest::Shell sh;

    ASSERT_EQ((int)runFor("echo first\r\ndone\r\n"), (int)CMD_ERROR_FAILED);

    // an if is not a loop, so its close is fi and a done here is a typo that
    // used to pass silently
    ASSERT_EQ((int)runFor("if test 1 -eq 1\r\necho x\r\ndone\r\n"), (int)CMD_ERROR_FAILED);
}

TEST(script_runner, an_fi_that_closes_no_if_is_reported)
{
    pditest::Shell sh;

    ASSERT_EQ((int)runFor("echo first\r\nfi\r\n"), (int)CMD_ERROR_FAILED);

    // a for closed by fi would run one pass and stop, which looks like a loop
    // that simply had one word in it
    ASSERT_EQ((int)runFor("for FORVAR in a b\r\necho x\r\nfi\r\n"), (int)CMD_ERROR_FAILED);
    ASSERT_EQ((int)runFor("while test 1 -eq 2\r\necho x\r\nfi\r\n"), (int)CMD_ERROR_FAILED);
}

TEST(script_runner, an_else_that_belongs_to_no_if_is_reported)
{
    pditest::Shell sh;

    ASSERT_EQ((int)runFor("echo first\r\nelse\r\n"), (int)CMD_ERROR_FAILED);
    ASSERT_EQ((int)runFor("for FORVAR in a b\r\nelse\r\ndone\r\n"), (int)CMD_ERROR_FAILED);
}

TEST(script_runner, a_quoted_value_holding_several_words_stays_one_pass)
{
    pditest::Shell sh;

    Environment::set("FORLIST", "red green blue");

    std::string out = runLoop("for FORVAR in \"$FORLIST\"\r\n"
                              "echo [$FORVAR] >> /hostloop.txt\r\n"
                              "done\r\n");

    ASSERT_TRUE(out.find("[red green blue]") != std::string::npos);
    ASSERT_EQ(countOf(out, "["), (size_t)1);

    Environment::unset("FORLIST");
}

TEST(script_runner, a_name_the_environment_does_not_carry_contributes_no_word)
{
    pditest::Shell sh;

    std::string out = runLoop("for FORVAR in one $FORNOSUCHNAME two\r\n"
                              "echo [$FORVAR] >> /hostloop.txt\r\n"
                              "done\r\n");

    // an unset name expands to nothing, and nothing is no word rather than an
    // empty one, so this is two passes
    ASSERT_EQ(countOf(out, "["), (size_t)2);
    ASSERT_TRUE(out.find("[one]") != std::string::npos);
    ASSERT_TRUE(out.find("[two]") != std::string::npos);
}

TEST(script_runner, a_single_quoted_word_is_taken_literally)
{
    pditest::Shell sh;

    Environment::set("FORLIST", "expanded");

    std::string out = runLoop("for FORVAR in '$FORLIST'\r\n"
                              "echo [$FORVAR] >> /hostloop.txt\r\n"
                              "done\r\n");

    ASSERT_TRUE(out.find("[$FORLIST]") != std::string::npos);
    ASSERT_TRUE(out.find("[expanded]") == std::string::npos);

    Environment::unset("FORLIST");
}

TEST(script_runner, blanks_between_words_do_not_make_empty_passes)
{
    pditest::Shell sh;

    std::string out = runLoop("for FORVAR in    one     two   \r\n"
                              "echo [$FORVAR] >> /hostloop.txt\r\n"
                              "done\r\n");

    ASSERT_EQ(countOf(out, "["), (size_t)2);
}

TEST(script_runner, a_list_of_one_word_runs_exactly_once)
{
    pditest::Shell sh;

    std::string out = runLoop("for FORVAR in only\r\n"
                              "echo [$FORVAR] >> /hostloop.txt\r\n"
                              "done\r\n");

    ASSERT_EQ(countOf(out, "[only]"), (size_t)1);
}

TEST(script_runner, a_body_that_rebinds_the_loop_variable_still_advances)
{
    pditest::Shell sh;

    // the place in the list is the block's, not the variable's, so writing to
    // the variable cannot make the loop repeat or skip
    std::string out = runLoop("for FORVAR in one two three\r\n"
                              "echo [$FORVAR] >> /hostloop.txt\r\n"
                              "export FORVAR=clobbered\r\n"
                              "done\r\n");

    ASSERT_EQ(countOf(out, "["), (size_t)3);
    ASSERT_TRUE(out.find("[one]") != std::string::npos);
    ASSERT_TRUE(out.find("[three]") != std::string::npos);
}

TEST(script_runner, a_while_inside_a_for_runs_on_every_pass)
{
    pditest::Shell sh;

    std::string out = runLoop("for FORVAR in a b\r\n"
                              "while test 1 -eq 2\r\n"
                              "echo neverreached >> /hostloop.txt\r\n"
                              "done\r\n"
                              "echo [$FORVAR] >> /hostloop.txt\r\n"
                              "done\r\n");

    ASSERT_TRUE(out.find("neverreached") == std::string::npos);
    ASSERT_EQ(countOf(out, "["), (size_t)2);
}

TEST(script_runner, an_if_inside_a_for_branches_on_each_pass)
{
    pditest::Shell sh;

    std::string out = runLoop("for FORVAR in one two\r\n"
                              "if test $FORVAR = one\r\n"
                              "echo matched >> /hostloop.txt\r\n"
                              "else\r\n"
                              "echo other >> /hostloop.txt\r\n"
                              "fi\r\n"
                              "done\r\n");

    ASSERT_EQ(countOf(out, "matched"), (size_t)1);
    ASSERT_EQ(countOf(out, "other"), (size_t)1);
}

TEST(script_runner, nesting_past_the_block_limit_is_reported)
{
    pditest::Shell sh;

    std::string deep;
    for (int i = 0; i < SCRIPT_BLOCK_MAX + 1; i++) deep += "for FORVAR in a\r\n";
    deep += "echo rananyway >> /hostloop.txt\r\n";
    for (int i = 0; i < SCRIPT_BLOCK_MAX + 1; i++) deep += "done\r\n";

    removeLoopOutput();
    writeScript(deep.c_str());
    pdi_err_t res = ScriptRunner::run(SCRIPT_PATH, true);

    ASSERT_EQ((int)res, (int)CMD_ERROR_FAILED);
    ASSERT_TRUE(loopOutput().find("rananyway") == std::string::npos);
    ASSERT_TRUE(!ScriptRunner::pending());

    removeScript();
    removeLoopOutput();
    Environment::unset("FORVAR");
}

TEST(script_runner, nesting_up_to_the_block_limit_runs)
{
    pditest::Shell sh;

    // the limit is a limit and not an off by one, so the deepest allowed nest
    // has to run rather than being refused with the one past it
    std::string deep;
    for (int i = 0; i < SCRIPT_BLOCK_MAX; i++) deep += "for FORVAR in a\r\n";
    deep += "echo reachedthebottom >> /hostloop.txt\r\n";
    for (int i = 0; i < SCRIPT_BLOCK_MAX; i++) deep += "done\r\n";

    std::string out = runLoop(deep.c_str());

    ASSERT_EQ(countOf(out, "reachedthebottom"), (size_t)1);
}

TEST(script_runner, a_for_whose_list_grows_under_it_is_capped_not_endless)
{
    pditest::Shell sh;

    // the list is read again each pass, so a body that adds to it would loop
    // for ever if nothing bounded it
    Environment::set("FORGROW", "seed");
    removeLoopOutput();

    writeScript("for FORVAR in $FORGROW\r\n"
                "echo . >> /hostloop.txt\r\n"
                "export FORGROW=\"$FORGROW more\"\r\n"
                "done\r\n");
    pdi_err_t res = ScriptRunner::run(SCRIPT_PATH, true);

    ASSERT_EQ((int)res, (int)CMD_ERROR_FAILED);
    ASSERT_TRUE(!ScriptRunner::pending());

    // the loop has to have been stopped by the cap and not by the body giving
    // up early, or this would pass for a reason that has nothing to do with it
    ASSERT_EQ(countOf(loopOutput(), "."), (size_t)SCRIPT_LOOP_MAX);

    removeScript();
    removeLoopOutput();
    Environment::unset("FORGROW");
    Environment::unset("FORVAR");
}

TEST(script_runner, a_long_loop_runs_every_pass_and_finishes)
{
    pditest::Shell sh;

    std::string list;
    for (int i = 0; i < 60; i++) list += "w ";

    std::string script = "for FORVAR in " + list + "\r\necho . >> /hostloop.txt\r\ndone\r\n";

    // CT was a loop leaving every command it ran alive, so a loop long enough
    // to have shown it has to finish rather than only start
    std::string out = runLoop(script.c_str());

    ASSERT_EQ(countOf(out, "."), (size_t)60);
}

TEST(script_runner, running_the_same_loop_many_times_stays_stable)
{
    pditest::Shell sh;

    for (int round = 0; round < 25; round++)
    {
        std::string out = runLoop("for FORVAR in a b c\r\n"
                                  "echo . >> /hostloop.txt\r\n"
                                  "done\r\n");
        ASSERT_EQ(countOf(out, "."), (size_t)3);
    }
}

TEST(script_runner, elif_takes_the_first_branch_whose_condition_holds)
{
    pditest::Shell sh;

    std::string out = runLoop("if test a = b\r\necho first >> /hostloop.txt\r\n"
                              "elif test a = a\r\necho second >> /hostloop.txt\r\n"
                              "elif test b = b\r\necho third >> /hostloop.txt\r\n"
                              "else\r\necho fourth >> /hostloop.txt\r\nfi\r\n");

    ASSERT_EQ(countOf(out, "second"), (size_t)1);
    ASSERT_EQ(countOf(out, "first") + countOf(out, "third") + countOf(out, "fourth"), (size_t)0);
}

TEST(script_runner, else_runs_when_no_elif_holds_and_a_skipped_if_runs_none)
{
    pditest::Shell sh;

    std::string none = runLoop("if test a = b\r\necho first >> /hostloop.txt\r\n"
                               "elif test a = c\r\necho second >> /hostloop.txt\r\n"
                               "else\r\necho fourth >> /hostloop.txt\r\nfi\r\n");
    std::string nested = runLoop("if test a = b\r\n"
                                 "if test a = a\r\necho inner >> /hostloop.txt\r\n"
                                 "elif test b = b\r\necho innerelif >> /hostloop.txt\r\nfi\r\n"
                                 "fi\r\necho after >> /hostloop.txt\r\n");

    ASSERT_EQ(countOf(none, "fourth"), (size_t)1);
    ASSERT_EQ(countOf(none, "first") + countOf(none, "second"), (size_t)0);
    ASSERT_EQ(countOf(nested, "inner"), (size_t)0);
    ASSERT_EQ(countOf(nested, "after"), (size_t)1);
}

TEST(script_runner, break_leaves_the_loop_it_is_in)
{
    pditest::Shell sh;

    std::string out = runLoop("for FORVAR in a b c d\r\n"
                              "if test $FORVAR = c\r\nbreak\r\nfi\r\n"
                              "echo x$FORVAR >> /hostloop.txt\r\ndone\r\n"
                              "echo end >> /hostloop.txt\r\n");

    ASSERT_EQ(countOf(out, "xa"), (size_t)1);
    ASSERT_EQ(countOf(out, "xb"), (size_t)1);
    ASSERT_EQ(countOf(out, "xc") + countOf(out, "xd"), (size_t)0);
    ASSERT_EQ(countOf(out, "end"), (size_t)1);
}

TEST(script_runner, break_ends_a_while_that_would_otherwise_run_on)
{
    pditest::Shell sh;
    Environment::set("COUNTER", "0");

    std::string out = runLoop("while test 1 = 1\r\n"
                              "export COUNTER=$((COUNTER + 1))\r\n"
                              "if test $COUNTER = 3\r\nbreak\r\nfi\r\n"
                              "done\r\necho end >> /hostloop.txt\r\n");

    pdiutil::string counter;
    Environment::get("COUNTER", counter);
    Environment::unset("COUNTER");

    ASSERT_STREQ(counter.c_str(), "3");
    ASSERT_EQ(countOf(out, "end"), (size_t)1);
}

TEST(script_runner, continue_skips_the_rest_of_one_pass)
{
    pditest::Shell sh;

    std::string out = runLoop("for FORVAR in a b c\r\n"
                              "if test $FORVAR = b\r\ncontinue\r\nfi\r\n"
                              "echo x$FORVAR >> /hostloop.txt\r\ndone\r\n");

    ASSERT_EQ(countOf(out, "xa"), (size_t)1);
    ASSERT_EQ(countOf(out, "xb"), (size_t)0);
    ASSERT_EQ(countOf(out, "xc"), (size_t)1);
}

TEST(script_runner, break_or_continue_outside_a_loop_fails_the_script)
{
    pditest::Shell sh;

    writeScript("break\r\n");
    pdi_err_t brk = ScriptRunner::run(SCRIPT_PATH, true);
    writeScript("continue\r\n");
    pdi_err_t cont = ScriptRunner::run(SCRIPT_PATH, true);
    removeScript();

    ASSERT_EQ((int)brk, (int)CMD_ERROR_FAILED);
    ASSERT_EQ((int)cont, (int)CMD_ERROR_FAILED);
}

TEST(script_runner, exit_ends_the_script_with_the_status_it_names)
{
    pditest::Shell sh;

    removeLoopOutput();
    writeScript("echo before >> /hostloop.txt\r\nexit 7\r\necho after >> /hostloop.txt\r\n");
    pdi_err_t res = ScriptRunner::run(SCRIPT_PATH, true);
    std::string out = loopOutput();
    removeScript();
    removeLoopOutput();

    ASSERT_EQ((int)res, 7);
    ASSERT_EQ((int)SessionManager::getLastExit(), 7);
    ASSERT_EQ(countOf(out, "before"), (size_t)1);
    ASSERT_EQ(countOf(out, "after"), (size_t)0);
}

TEST(script_runner, exit_in_a_sourced_script_returns_to_the_one_that_sourced_it)
{
    pditest::Shell sh;

    VfsDispatcher *fs = pditest::mountedVfs();
    const char inner[] = "exit\r\necho ininner >> /hostloop.txt\r\n";
    fs->beginPrivileged();
    if (fs->isFileExist("/hostinner.sh")) fs->deleteFile("/hostinner.sh");
    fs->createFile("/hostinner.sh", inner, (int64_t)strlen(inner));
    fs->endPrivileged();

    std::string out = runLoop("source /hostinner.sh\r\necho afterinner >> /hostloop.txt\r\n");

    fs->beginPrivileged();
    fs->deleteFile("/hostinner.sh");
    fs->endPrivileged();

    ASSERT_EQ(countOf(out, "ininner"), (size_t)0);
    ASSERT_EQ(countOf(out, "afterinner"), (size_t)1);
}

TEST(script_runner, a_loop_body_that_asks_for_input_unattended_is_cancelled)
{
    pditest::Shell sh;

    writeScript("for FORVAR in a b c\r\nsu\r\ndone\r\n");
    pdi_err_t res = ScriptRunner::run(SCRIPT_PATH, true);

    ASSERT_EQ((int)res, (int)CMD_ERROR_CANCELED);
    ASSERT_TRUE(!ScriptRunner::pending());

    removeScript();
    Environment::unset("FORVAR");
}

TEST(script_runner, a_loop_body_that_asks_for_input_attended_keeps_its_place)
{
    pditest::Shell sh;

    writeScript("for FORVAR in a b c\r\nsu\r\ndone\r\n");
    pdi_err_t res = ScriptRunner::run(SCRIPT_PATH, false);

    ASSERT_EQ((int)res, (int)CMD_ERROR_AGAIN);
    ASSERT_TRUE(ScriptRunner::pending());

    ScriptRunner::clearSession();
    ASSERT_TRUE(!ScriptRunner::pending());

    removeScript();
    Environment::unset("FORVAR");
}

const char INNER_PATH[] = "/hostinner.sh";

void writeInner(const char *body)
{
    VfsDispatcher *fs = pditest::mountedVfs();
    fs->beginPrivileged();
    if (fs->isFileExist(INNER_PATH)) fs->deleteFile(INNER_PATH);
    fs->createFile(INNER_PATH, body, (int64_t)strlen(body));
    fs->endPrivileged();
}

void removeInner()
{
    VfsDispatcher *fs = pditest::mountedVfs();
    fs->beginPrivileged();
    if (fs->isFileExist(INNER_PATH)) fs->deleteFile(INNER_PATH);
    fs->endPrivileged();
}

TEST(script_runner, a_sourced_script_reads_the_arguments_it_was_given)
{
    pditest::Shell sh;

    writeScript("echo [$1:$2:$#]\r\n");
    std::string out = sh.run("source /hostscript.sh one two");
    removeScript();

    ASSERT_TRUE(out.find("[one:two:2]") != std::string::npos);
}

TEST(script_runner, an_argument_the_script_was_not_given_is_empty)
{
    pditest::Shell sh;

    writeScript("echo [$1:$2:$#]\r\n");
    std::string one = sh.run("source /hostscript.sh one");
    std::string none = sh.run("source /hostscript.sh");
    removeScript();

    ASSERT_TRUE(one.find("[one::1]") != std::string::npos);
    ASSERT_TRUE(none.find("[::0]") != std::string::npos);
}

TEST(script_runner, a_quoted_argument_is_one_argument)
{
    pditest::Shell sh;

    writeScript("echo \"[$1:$2:$#]\"\r\n");
    std::string out = sh.run("source /hostscript.sh \"a b\" c");
    removeScript();

    ASSERT_TRUE(out.find("[a b:c:2]") != std::string::npos);
}

TEST(script_runner, every_argument_reads_back_as_one_list)
{
    pditest::Shell sh;

    writeScript("echo [$@]\r\n");
    std::string out = sh.run("source /hostscript.sh one two");
    removeScript();

    ASSERT_TRUE(out.find("[one two]") != std::string::npos);
}

TEST(script_runner, more_arguments_than_the_option_limit_are_refused)
{
    pditest::Shell sh;

    writeScript("echo ranwith$#\r\n");
    std::string line = "source /hostscript.sh";
    for (int i = 0; i < CMD_OPTION_MAX; i++) line += " x";
    std::string out = sh.run(line.c_str());
    pdi_err_t res = SessionManager::getLastExit();
    removeScript();

    ASSERT_TRUE(out.find("ranwith") == std::string::npos);
    ASSERT_TRUE(out.find("too many arguments") != std::string::npos);
    ASSERT_EQ((int)res, (int)CMD_ERROR_INVAL);
}

TEST(script_runner, as_many_arguments_as_the_option_limit_allows_are_taken)
{
    pditest::Shell sh;

    writeScript("echo ranwith$#\r\n");
    std::string line = "source /hostscript.sh";
    for (int i = 1; i < CMD_OPTION_MAX; i++) line += " x";
    std::string out = sh.run(line.c_str());
    removeScript();

    std::string expect = "ranwith" + std::to_string(CMD_OPTION_MAX - 1);
    ASSERT_TRUE(out.find(expect) != std::string::npos);
}

TEST(script_runner, an_argument_goes_in_as_text_and_is_never_run)
{
    pditest::Shell sh;

    writeScript("echo [$1]\r\n");
    std::string out = sh.run("source /hostscript.sh 'x; echo INJ$((2 + 3))'");
    removeScript();

    ASSERT_TRUE(out.find("[x; echo INJ$((2 + 3))]") != std::string::npos);
    ASSERT_TRUE(out.find("INJ5") == std::string::npos);
}

TEST(script_runner, single_quotes_keep_an_argument_reference_as_typed)
{
    pditest::Shell sh;

    writeScript("echo '[$1:$#]'\r\n");
    std::string out = sh.run("source /hostscript.sh one");
    removeScript();

    ASSERT_TRUE(out.find("[$1:$#]") != std::string::npos);
}

TEST(script_runner, a_nested_source_with_no_arguments_sees_the_callers)
{
    pditest::Shell sh;

    writeInner("echo inner[$1:$#]\r\n");
    writeScript("source /hostinner.sh\r\n");
    std::string out = sh.run("source /hostscript.sh one two");
    removeScript();
    removeInner();

    ASSERT_TRUE(out.find("inner[one:2]") != std::string::npos);
}

TEST(script_runner, a_nested_source_with_arguments_has_its_own_until_it_returns)
{
    pditest::Shell sh;

    writeInner("echo inner[$1:$#]\r\n");
    writeScript("source /hostinner.sh nine\r\necho outer[$1:$#]\r\n");
    std::string out = sh.run("source /hostscript.sh one two");
    removeScript();
    removeInner();

    ASSERT_TRUE(out.find("inner[nine:1]") != std::string::npos);
    ASSERT_TRUE(out.find("outer[one:2]") != std::string::npos);
}

TEST(script_runner, outside_a_script_there_are_no_arguments)
{
    pditest::Shell sh;

    std::string out = sh.run("echo [$1:$#:$@]");

    ASSERT_TRUE(out.find("[:0:]") != std::string::npos);
}

#endif
