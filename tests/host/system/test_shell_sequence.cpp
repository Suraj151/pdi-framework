/************************* Shell Sequencing Tests *****************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Covers running several commands off one line with ';', '&&' and '||', and the
'$?' that reports what the last one answered.

Author          : Suraj I.
created Date    : 28th Aug 2026
******************************************************************************/

#include <MountedStack.h>
#include <ShellHarness.h>
#include <pditest.h>

#include <service_provider/session/SessionManager.h>

#if defined(ENABLE_CMD_SERVICE) && defined(ENABLE_STORAGE_SERVICE)

TEST(sequence, a_semicolon_runs_both_commands)
{
    pditest::mountedVfs();
    pditest::Shell sh;

    std::string out = sh.run("echo first ; echo second");

    ASSERT_TRUE(out.find("first") != std::string::npos);
    ASSERT_TRUE(out.find("second") != std::string::npos);
}

TEST(sequence, a_semicolon_runs_the_second_even_after_a_failure)
{
    pditest::mountedVfs();
    pditest::Shell sh;

    std::string out = sh.run("nosuchcommand ; echo after");

    ASSERT_TRUE(out.find("after") != std::string::npos);
}

TEST(sequence, and_runs_the_second_only_after_success)
{
    pditest::mountedVfs();
    pditest::Shell sh;

    std::string out = sh.run("pwd && echo ran");
    ASSERT_TRUE(out.find("ran") != std::string::npos);
}

TEST(sequence, and_skips_the_second_after_a_failure)
{
    pditest::mountedVfs();
    pditest::Shell sh;

    std::string out = sh.run("nosuchcommand && echo skipped");
    ASSERT_TRUE(out.find("skipped") == std::string::npos);
}

TEST(sequence, or_runs_the_second_only_after_a_failure)
{
    pditest::mountedVfs();
    pditest::Shell sh;

    std::string out = sh.run("nosuchcommand || echo fallback");
    ASSERT_TRUE(out.find("fallback") != std::string::npos);
}

TEST(sequence, or_skips_the_second_after_success)
{
    pditest::mountedVfs();
    pditest::Shell sh;

    std::string out = sh.run("pwd || echo skipped");
    ASSERT_TRUE(out.find("skipped") == std::string::npos);
}

TEST(sequence, a_chain_stops_at_the_first_failure)
{
    pditest::mountedVfs();
    pditest::Shell sh;

    std::string out = sh.run("pwd && nosuchcommand && echo never");

    ASSERT_TRUE(out.find("never") == std::string::npos);
    ASSERT_TRUE(SessionManager::getLastExit() != PDI_OK);
}

TEST(sequence, a_skipped_segment_leaves_the_result_of_the_one_that_ran)
{
    pditest::mountedVfs();
    pditest::Shell sh;

    // the echo never runs, so the failure stays the answer for the line
    sh.run("nosuchcommand && echo skipped");
    ASSERT_EQ((int)SessionManager::getLastExit(), (int)CMD_ERROR_NOENT);
}

TEST(sequence, a_pipeline_still_works_inside_a_segment)
{
    pditest::mountedVfs();
    pditest::Shell sh;

    std::string out = sh.run("echo counted | wc && echo done");

    ASSERT_TRUE(out.find("done") != std::string::npos);
}

TEST(sequence, a_redirect_still_works_inside_a_segment)
{
    pditest::mountedVfs();
    pditest::Shell sh;
    __i_fs.deleteFile("/seq_redirect.txt");

    std::string out = sh.run("echo saved > /seq_redirect.txt ; echo shown");

    ASSERT_TRUE(out.find("saved") == std::string::npos);
    ASSERT_TRUE(out.find("shown") != std::string::npos);
    ASSERT_TRUE(__i_fs.isFileExist("/seq_redirect.txt"));

    __i_fs.deleteFile("/seq_redirect.txt");
}

TEST(sequence, a_dangling_operator_is_a_syntax_error)
{
    pditest::mountedVfs();
    pditest::Shell sh;

    std::string out = sh.run("echo one &&");
    ASSERT_TRUE(out.find("syntax error") != std::string::npos);
}

TEST(sequence, the_last_exit_reads_back_through_the_question_mark)
{
    pditest::mountedVfs();
    pditest::Shell sh;

    sh.run("pwd");
    std::string ok = sh.run("echo $?");
    ASSERT_TRUE(ok.find("0") != std::string::npos);
}

TEST(sequence, the_question_mark_reports_a_failure_code)
{
    pditest::mountedVfs();
    pditest::Shell sh;

    sh.run("nosuchcommand");

    char expected[12] = {0};
    Int32ToString((int32_t)CMD_ERROR_NOENT, expected, sizeof(expected) - 1);

    std::string out = sh.run("echo $?");
    ASSERT_TRUE(out.find(expected) != std::string::npos);
}

TEST(sequence, the_question_mark_expands_within_one_line)
{
    pditest::mountedVfs();
    pditest::Shell sh;

    // the echo sees what pwd answered, because the segments run in order
    std::string out = sh.run("pwd ; echo status=$?");
    ASSERT_TRUE(out.find("status=0") != std::string::npos);
}

TEST(sequence, the_question_mark_sees_the_segment_before_it)
{
    pditest::mountedVfs();
    pditest::Shell sh;

    // zero first, so a status carried in from the previous line cannot pass
    sh.run("pwd");

    char expected[12] = {0};
    Int32ToString((int32_t)CMD_ERROR_NOENT, expected, sizeof(expected) - 1);

    std::string out = sh.run("nosuchcommand ; echo $?");
    ASSERT_TRUE(out.find(expected) != std::string::npos);
}


TEST(sequence, single_quotes_suppress_expansion)
{
    pditest::mountedVfs();
    pditest::Shell sh;

    sh.run("pwd");
    std::string out = sh.run("echo '$?'");

    // a single quoted word is literal, so the shell must not expand inside it
    ASSERT_TRUE(out.find("$?") != std::string::npos);
}

TEST(sequence, double_quotes_still_expand)
{
    pditest::mountedVfs();
    pditest::Shell sh;

    sh.run("pwd");
    std::string out = sh.run("echo \"$?\"");

    ASSERT_TRUE(out.find("$?") == std::string::npos);
    ASSERT_TRUE(out.find("0") != std::string::npos);
}

TEST(sequence, a_bang_turns_a_pipeline_s_answer_around)
{
    pditest::mountedVfs();
    pditest::Shell sh;

    std::string inverted = sh.run("! test a = b && echo inverted");
    std::string flipped = sh.run("! test a = a || echo flipped");
    std::string kept = sh.run("! test a = a && echo wrong");

    ASSERT_TRUE(inverted.find("inverted") != std::string::npos);
    ASSERT_TRUE(flipped.find("flipped") != std::string::npos);
    ASSERT_TRUE(kept.find("wrong") == std::string::npos);
}

TEST(sequence, a_bang_s_answer_is_what_the_question_mark_reads)
{
    pditest::mountedVfs();
    pditest::Shell sh;

    sh.run("! test a = b");
    pdi_err_t whenFalse = sh.result();
    sh.run("! test a = a");
    pdi_err_t whenTrue = sh.result();

    ASSERT_EQ((int)whenFalse, (int)PDI_OK);
    ASSERT_NE((int)whenTrue, (int)PDI_OK);
}

TEST(sequence, true_and_false_answer_without_printing)
{
    pditest::mountedVfs();
    pditest::Shell sh;

    std::string yes = sh.run("true");
    pdi_err_t yesResult = sh.result();
    std::string no = sh.run("false");
    pdi_err_t noResult = sh.result();
    std::string chained = sh.run("false || true && echo chained");

    ASSERT_EQ((int)yesResult, (int)PDI_OK);
    ASSERT_EQ((int)noResult, (int)CMD_RESULT_FALSE);
    ASSERT_TRUE(yes.find("CmdErr") == std::string::npos);
    ASSERT_TRUE(no.find("CmdErr") == std::string::npos);
    ASSERT_TRUE(chained.find("chained") != std::string::npos);
}

TEST(sequence, brackets_test_the_way_test_does)
{
    pditest::mountedVfs();
    pditest::Shell sh;

    ASSERT_TRUE(sh.run("[ a = a ] && echo same").find("same") != std::string::npos);
    ASSERT_TRUE(sh.run("[ a = b ] || echo differ").find("differ") != std::string::npos);
    ASSERT_TRUE(sh.run("[ -d / ] && echo dir").find("dir") != std::string::npos);
    ASSERT_TRUE(sh.run("! [ a = b ] && echo negated").find("negated") != std::string::npos);
}

TEST(sequence, an_opening_bracket_without_its_close_is_refused)
{
    pditest::mountedVfs();
    pditest::Shell sh;

    std::string out = sh.run("[ a = a && echo ran");
    std::string word = sh.run("echo [x]");

    ASSERT_TRUE(out.find("ran") == std::string::npos);
    ASSERT_TRUE(word.find("[x]") != std::string::npos);
}

TEST(sequence, a_bang_with_nothing_after_it_is_a_syntax_error_and_inside_a_word_is_text)
{
    pditest::mountedVfs();
    pditest::Shell sh;

    std::string alone = sh.run("!");
    std::string word = sh.run("echo !x");

    ASSERT_TRUE(alone.find("syntax error") != std::string::npos);
    ASSERT_TRUE(word.find("!x") != std::string::npos);
}

#endif
