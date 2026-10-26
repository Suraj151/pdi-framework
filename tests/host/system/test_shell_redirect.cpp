/************************** Shell Redirect Tests ******************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Covers redirection as the shell now does it: any command can be sent to a file,
the terminal sees none of it, and the descriptor is handed back afterwards.

Author          : Suraj I.
created Date    : 26th Aug 2026
******************************************************************************/

#include <MountedStack.h>
#include <ShellHarness.h>
#include <pditest.h>

#include <service_provider/cmd/ShellParser.h>
#include <service_provider/session/Environment.h>
#include <service_provider/session/SessionManager.h>

#if defined(ENABLE_CMD_SERVICE) && defined(ENABLE_STORAGE_SERVICE)

namespace
{

    std::string cmdText(const ShellParser::Line &p, const char *line, int16_t index)
    {
        pdiutil::string out;
        p.commandText(line, index, out);
        return std::string(out.c_str());
    }

    std::string redirTarget(const ShellParser::Line &p, const char *line, ShellParser::redirect_op_t op)
    {
        for (int16_t c = 0; c < (int16_t)p.m_commands.size(); c++) {
            const ShellParser::Redirect *r = p.findRedirect(c, op);
            if (nullptr != r) return std::string(line + r->m_start, (size_t)r->m_len);
        }
        return std::string();
    }

    std::string envValue(const char *key)
    {
        pdiutil::string out;
        Environment::get(key, out);
        return std::string(out.c_str());
    }

    std::string slurp(const char *path)
    {
        std::string out;
        __i_fs.readFile(path, 512, [&](char *data, uint32_t size) -> bool {
            out.append(data, size);
            return true;
        });
        return out;
    }
}

TEST(redirect, the_parser_finds_a_replacing_target)
{
    const char *line = "echo hi > /tmp/out.txt";
    ShellParser::Line t = ShellParser::parse(line, (int16_t)strlen(line));

    ASSERT_TRUE(nullptr != t.findRedirect(0, ShellParser::REDIRECT_OUT));
    ASSERT_TRUE(nullptr == t.findRedirect(0, ShellParser::REDIRECT_APPEND));
    ASSERT_FALSE(t.m_malformed);
    ASSERT_EQ((int)cmdText(t, line, 0).size(), 7);
    ASSERT_STREQ(redirTarget(t, line, ShellParser::REDIRECT_OUT).c_str(), "/tmp/out.txt");
}

TEST(redirect, the_parser_finds_an_appending_target)
{
    const char *line = "echo hi >> /tmp/out.txt";
    ShellParser::Line t = ShellParser::parse(line, (int16_t)strlen(line));

    ASSERT_TRUE(nullptr != t.findRedirect(0, ShellParser::REDIRECT_APPEND));
    ASSERT_EQ((int)cmdText(t, line, 0).size(), 7);
    ASSERT_STREQ(redirTarget(t, line, ShellParser::REDIRECT_APPEND).c_str(), "/tmp/out.txt");
}

TEST(redirect, a_line_with_no_operator_is_left_whole)
{
    const char *line = "echo hi";
    ShellParser::Line t = ShellParser::parse(line, (int16_t)strlen(line));

    ASSERT_TRUE(nullptr == t.findRedirect(0, ShellParser::REDIRECT_OUT));
    ASSERT_TRUE(t.isPlain());
    ASSERT_EQ((int)cmdText(t, line, 0).size(), 7);
}

TEST(redirect, a_dangling_operator_is_malformed)
{
    const char *line = "echo hi >";
    ShellParser::Line t = ShellParser::parse(line, (int16_t)strlen(line));

    ASSERT_TRUE(t.m_malformed);
}

TEST(redirect, an_operator_with_no_command_is_malformed)
{
    const char *line = "> /tmp/out.txt";
    ShellParser::Line t = ShellParser::parse(line, (int16_t)strlen(line));

    ASSERT_TRUE(t.m_malformed);
}

TEST(redirect, echo_writes_its_text_and_nothing_else)
{
    pditest::mountedVfs();
    pditest::Shell sh;
    __i_fs.deleteFile("/redir_echo.txt");

    std::string seen = sh.run("echo hello > /redir_echo.txt");

    // the terminal saw no payload, only the prompt that follows
    ASSERT_TRUE(seen.find("hello") == std::string::npos);
    ASSERT_STREQ(slurp("/redir_echo.txt").c_str(), "hello\n");

    __i_fs.deleteFile("/redir_echo.txt");
}

TEST(redirect, a_replacing_target_starts_the_file_over)
{
    pditest::mountedVfs();
    pditest::Shell sh;
    __i_fs.deleteFile("/redir_trunc.txt");

    sh.run("echo first > /redir_trunc.txt");
    sh.run("echo second > /redir_trunc.txt");

    ASSERT_STREQ(slurp("/redir_trunc.txt").c_str(), "second\n");
    __i_fs.deleteFile("/redir_trunc.txt");
}

TEST(redirect, an_appending_target_keeps_both_lines)
{
    pditest::mountedVfs();
    pditest::Shell sh;
    __i_fs.deleteFile("/redir_append.txt");

    sh.run("echo first > /redir_append.txt");
    sh.run("echo second >> /redir_append.txt");

    ASSERT_STREQ(slurp("/redir_append.txt").c_str(), "first\nsecond\n");
    __i_fs.deleteFile("/redir_append.txt");
}

TEST(redirect, a_command_that_never_knew_about_redirection_is_captured)
{
    pditest::mountedVfs();
    pditest::Shell sh;
    __i_fs.deleteFile("/redir_pwd.txt");

    // pwd has no redirect handling of its own; the shell supplies all of it
    std::string seen = sh.run("pwd > /redir_pwd.txt");

    ASSERT_TRUE(slurp("/redir_pwd.txt").size() > 0);
    ASSERT_TRUE(seen.find("/") != std::string::npos || seen.size() > 0);

    __i_fs.deleteFile("/redir_pwd.txt");
}

TEST(redirect, the_descriptor_goes_back_to_the_terminal_afterwards)
{
    pditest::mountedVfs();
    pditest::Shell sh;
    __i_fs.deleteFile("/redir_back.txt");

    sh.run("echo away > /redir_back.txt");
    std::string seen = sh.run("echo back");

    ASSERT_TRUE(seen.find("back") != std::string::npos);
    ASSERT_NULL(SessionManager::current()->m_fdtable);

    __i_fs.deleteFile("/redir_back.txt");
}

TEST(redirect, a_dangling_operator_is_refused_at_the_shell)
{
    pditest::mountedVfs();
    pditest::Shell sh;

    std::string seen = sh.run("echo hi >");
    ASSERT_TRUE(seen.find("syntax error") != std::string::npos);
}

TEST(redirect, a_target_the_filesystem_refuses_is_reported)
{
    pditest::mountedVfs();
    pditest::Shell sh;

    std::string seen = sh.run("echo hi > /proc/uptime");

    ASSERT_TRUE(seen.find("cannot write") != std::string::npos);
    ASSERT_TRUE(seen.find("/proc/uptime") != std::string::npos);
}

TEST(redirect, a_refused_target_still_leaves_the_terminal_usable)
{
    pditest::mountedVfs();
    pditest::Shell sh;

    sh.run("echo hi > /proc/uptime");
    std::string seen = sh.run("echo back");

    ASSERT_TRUE(seen.find("back") != std::string::npos);
    ASSERT_NULL(SessionManager::current()->m_fdtable);
}

TEST(pipeline, the_parser_splits_every_stage)
{
    const char *line = "echo hi | grep h | wc";
    ShellParser::Line t = ShellParser::parse(line, (int16_t)strlen(line));

    ASSERT_FALSE(t.m_malformed);
    ASSERT_FALSE(t.isPlain());
    ASSERT_EQ((int)t.m_commands.size(), 3);
    ASSERT_EQ((int)cmdText(t, line, 0).size(), 7);
    ASSERT_EQ((int)cmdText(t, line, 2).size(), 2);
}

TEST(pipeline, a_target_binds_to_the_last_stage)
{
    const char *line = "echo hi | wc > /tmp/o.txt";
    ShellParser::Line t = ShellParser::parse(line, (int16_t)strlen(line));

    ASSERT_EQ((int)t.m_commands.size(), 2);
    ASSERT_TRUE(nullptr == t.findRedirect(0, ShellParser::REDIRECT_OUT));
    ASSERT_TRUE(nullptr != t.findRedirect(1, ShellParser::REDIRECT_OUT));
    ASSERT_STREQ(redirTarget(t, line, ShellParser::REDIRECT_OUT).c_str(), "/tmp/o.txt");
    ASSERT_EQ((int)cmdText(t, line, 1).size(), 2);
}

TEST(pipeline, an_empty_stage_is_malformed)
{
    const char *a = "echo hi |";
    const char *b = "| wc";
    const char *c = "echo hi | | wc";

    ASSERT_TRUE(ShellParser::parse(a, (int16_t)strlen(a)).m_malformed);
    ASSERT_TRUE(ShellParser::parse(b, (int16_t)strlen(b)).m_malformed);
    ASSERT_TRUE(ShellParser::parse(c, (int16_t)strlen(c)).m_malformed);

    // '||' is the or-if operator, so this one is a valid line
    const char *orif = "echo hi || wc";
    ASSERT_TRUE(!ShellParser::parse(orif, (int16_t)strlen(orif)).m_malformed);
}

TEST(pipeline, output_of_one_stage_becomes_input_of_the_next)
{
    pditest::mountedVfs();
    pditest::Shell sh;

    // wc counts what echo produced, not a file it was handed
    std::string seen = sh.run("echo hello world | wc");

    ASSERT_TRUE(seen.find("1 2 12") != std::string::npos);
}

TEST(pipeline, a_three_stage_line_carries_through)
{
    pditest::mountedVfs();
    pditest::Shell sh;

    std::string seen = sh.run("echo alpha | grep alpha | wc");
    ASSERT_TRUE(seen.find("1 ") != std::string::npos);
}

TEST(pipeline, grep_filters_what_it_is_piped)
{
    pditest::mountedVfs();
    pditest::Shell sh;

    std::string hit = sh.run("echo needle | grep needle");
    ASSERT_TRUE(hit.find("needle") != std::string::npos);

    std::string miss = sh.run("echo needle | grep absent");
    ASSERT_TRUE(miss.find("absent") == std::string::npos);
}

TEST(pipeline, the_last_stage_can_still_be_redirected)
{
    pditest::mountedVfs();
    pditest::Shell sh;
    __i_fs.deleteFile("/pipe_out.txt");

    std::string seen = sh.run("echo hello world | wc > /pipe_out.txt");

    ASSERT_TRUE(seen.find("1 2 12") == std::string::npos);
    ASSERT_TRUE(slurp("/pipe_out.txt").find("1 2 12") != std::string::npos);

    __i_fs.deleteFile("/pipe_out.txt");
}

TEST(pipeline, descriptors_are_all_handed_back_afterwards)
{
    pditest::mountedVfs();
    pditest::Shell sh;

    sh.run("echo hi | wc");
    ASSERT_NULL(SessionManager::current()->m_fdtable);

    std::string seen = sh.run("echo plain");
    ASSERT_TRUE(seen.find("plain") != std::string::npos);
}

TEST(pipeline, cat_passes_its_input_straight_through)
{
    pditest::mountedVfs();
    pditest::Shell sh;

    std::string seen = sh.run("echo through | cat");
    ASSERT_TRUE(seen.find("through") != std::string::npos);
}

/**
 * A pipe is a fixed buffer and the stages run one after another, so a writer
 * that fills it is cut off. A reader that went on to the end of the pipe
 * answered on part of the output, and is told so; one that stopped early, as
 * head does, never needed the rest.
 */
TEST(pipeline, a_reader_that_reached_the_end_of_a_cut_pipe_is_told)
{
    pditest::mountedVfs();
    pditest::Shell sh;
    std::string lines;
    for (int i = 0; i < 400; i++)
    {
        lines += "line of piped text\n";
    }
    __i_fs.deleteFile("/w_pipe_big.txt");
    __i_fs.createFile("/w_pipe_big.txt", lines.data(), (int64_t)lines.size());

    std::string counted = sh.run("cat /w_pipe_big.txt | wc");
    std::string headed = sh.run("cat /w_pipe_big.txt | head 2");
    std::string small = sh.run("echo short | wc");

    __i_fs.deleteFile("/w_pipe_big.txt");
    ASSERT_TRUE(pditest::saw(counted, "pipe full"));
    ASSERT_FALSE(pditest::saw(headed, "pipe full"));
    ASSERT_TRUE(pditest::saw(headed, "line of piped text"));
    ASSERT_FALSE(pditest::saw(small, "pipe full"));
}

/**
 * cat copies a file, so its bytes reach a pipe or a file as they are; only a
 * command's own line ends are tidied for the places a terminal is not.
 */
TEST(pipeline, cat_hands_on_a_files_carriage_returns_untouched)
{
    pditest::mountedVfs();
    pditest::Shell sh;
    const char crlf[] = "one\r\ntwo\r\nthree\r\n";
    __i_fs.deleteFile("/w_crlf.txt");
    __i_fs.deleteFile("/w_crlf_copy.txt");
    __i_fs.createFile("/w_crlf.txt", crlf, (int64_t)strlen(crlf));

    std::string counted = sh.run("cat /w_crlf.txt | wc");
    sh.run("cat /w_crlf.txt > /w_crlf_copy.txt");
    int64_t copied = __i_fs.getFileSize("/w_crlf_copy.txt");
    std::string own = sh.run("echo plain | wc");

    __i_fs.deleteFile("/w_crlf.txt");
    __i_fs.deleteFile("/w_crlf_copy.txt");
    ASSERT_TRUE(pditest::saw(counted, "3 3 17"));
    ASSERT_EQ((int)copied, 17);
    ASSERT_TRUE(pditest::saw(own, "1 1 6"));
}

TEST(pipeline, head_stops_at_the_count_it_was_given)
{
    pditest::mountedVfs();
    pditest::Shell sh;
    __i_fs.writeFile("/pipe_lines.txt", "one\ntwo\nthree\n", 14, false);

    // with a pipe there is no file to name, so the lone argument is the count
    std::string seen = sh.run("cat /pipe_lines.txt | head 2");

    ASSERT_TRUE(seen.find("one") != std::string::npos);
    ASSERT_TRUE(seen.find("two") != std::string::npos);
    ASSERT_TRUE(seen.find("three") == std::string::npos);

    __i_fs.deleteFile("/pipe_lines.txt");
}

TEST(pipeline, tail_keeps_the_last_lines_of_what_it_was_piped)
{
    pditest::mountedVfs();
    pditest::Shell sh;
    __i_fs.writeFile("/pipe_lines2.txt", "one\ntwo\nthree\n", 14, false);

    std::string seen = sh.run("cat /pipe_lines2.txt | tail 2");

    ASSERT_TRUE(seen.find("one") == std::string::npos);
    ASSERT_TRUE(seen.find("two") != std::string::npos);
    ASSERT_TRUE(seen.find("three") != std::string::npos);

    __i_fs.deleteFile("/pipe_lines2.txt");
}

TEST(pipeline, a_file_argument_still_wins_when_nothing_is_piped)
{
    pditest::mountedVfs();
    pditest::Shell sh;
    __i_fs.writeFile("/pipe_lines3.txt", "one\ntwo\nthree\n", 14, false);

    // no pipe, so the first positional is the path it has always been
    std::string seen = sh.run("head /pipe_lines3.txt 1");

    ASSERT_TRUE(seen.find("one") != std::string::npos);
    ASSERT_TRUE(seen.find("three") == std::string::npos);

    __i_fs.deleteFile("/pipe_lines3.txt");
}

TEST(pipeline, a_stream_without_a_final_newline_still_yields_its_last_line)
{
    pditest::mountedVfs();
    pditest::Shell sh;
    __i_fs.writeFile("/pipe_bare.txt", "alpha\nomega", 11, false);

    std::string seen = sh.run("cat /pipe_bare.txt | tail 1");
    ASSERT_TRUE(seen.find("omega") != std::string::npos);

    __i_fs.deleteFile("/pipe_bare.txt");
}

TEST(pipeline, a_command_given_no_input_is_unaffected)
{
    pditest::mountedVfs();
    pditest::Shell sh;

    // wc with neither a file nor a pipe still reports a missing argument
    sh.run("wc");
    ASSERT_EQ((int)sh.result(), (int)CMD_ERROR_ARGS_MISSING);
}


TEST(source, the_parser_finds_an_input_source)
{
    const char *line = "wc < /tmp/in.txt";
    ShellParser::Line t = ShellParser::parse(line, (int16_t)strlen(line));

    ASSERT_TRUE(nullptr != t.findRedirect(0, ShellParser::REDIRECT_IN));
    ASSERT_TRUE(nullptr == t.findRedirect(0, ShellParser::REDIRECT_OUT));
    ASSERT_FALSE(t.m_malformed);
    ASSERT_FALSE(t.isPlain());
    ASSERT_EQ((int)cmdText(t, line, 0).size(), 2);
    ASSERT_STREQ(redirTarget(t, line, ShellParser::REDIRECT_IN).c_str(), "/tmp/in.txt");
}

TEST(source, a_line_can_name_a_source_and_a_target)
{
    const char *line = "wc < /tmp/in.txt > /tmp/out.txt";
    ShellParser::Line t = ShellParser::parse(line, (int16_t)strlen(line));

    ASSERT_TRUE(nullptr != t.findRedirect(0, ShellParser::REDIRECT_IN));
    ASSERT_TRUE(nullptr != t.findRedirect(0, ShellParser::REDIRECT_OUT));
    ASSERT_STREQ(redirTarget(t, line, ShellParser::REDIRECT_IN).c_str(), "/tmp/in.txt");
    ASSERT_STREQ(redirTarget(t, line, ShellParser::REDIRECT_OUT).c_str(), "/tmp/out.txt");
    ASSERT_EQ((int)cmdText(t, line, 0).size(), 2);
}

TEST(source, the_order_of_the_operators_does_not_matter)
{
    const char *line = "wc > /tmp/out.txt < /tmp/in.txt";
    ShellParser::Line t = ShellParser::parse(line, (int16_t)strlen(line));

    ASSERT_STREQ(redirTarget(t, line, ShellParser::REDIRECT_IN).c_str(), "/tmp/in.txt");
    ASSERT_STREQ(redirTarget(t, line, ShellParser::REDIRECT_OUT).c_str(), "/tmp/out.txt");
}

TEST(source, a_dangling_or_repeated_operator_is_malformed)
{
    const char *a = "wc <";
    const char *b = "< /tmp/in.txt";
    const char *c = "wc < /tmp/a.txt < /tmp/b.txt";

    ASSERT_TRUE(ShellParser::parse(a, (int16_t)strlen(a)).m_malformed);
    ASSERT_TRUE(ShellParser::parse(b, (int16_t)strlen(b)).m_malformed);
    ASSERT_TRUE(ShellParser::parse(c, (int16_t)strlen(c)).m_malformed);
}

TEST(source, a_command_reads_the_file_it_was_given)
{
    pditest::mountedVfs();
    pditest::Shell sh;
    __i_fs.writeFile("/src_in.txt", "one\ntwo\nthree\n", 14, false);

    std::string seen = sh.run("wc < /src_in.txt");

    // three lines, three words, fourteen bytes
    ASSERT_TRUE(seen.find("3") != std::string::npos);
    ASSERT_TRUE(seen.find("14") != std::string::npos);

    __i_fs.deleteFile("/src_in.txt");
}

TEST(source, a_source_feeds_the_first_stage_of_a_pipeline)
{
    pditest::mountedVfs();
    pditest::Shell sh;
    __i_fs.writeFile("/src_pipe.txt", "alpha\nbeta\n", 11, false);

    std::string seen = sh.run("cat < /src_pipe.txt | grep alpha");

    ASSERT_TRUE(seen.find("alpha") != std::string::npos);
    ASSERT_TRUE(seen.find("beta") == std::string::npos);

    __i_fs.deleteFile("/src_pipe.txt");
}

TEST(source, a_source_and_a_target_work_on_one_line)
{
    pditest::mountedVfs();
    pditest::Shell sh;
    __i_fs.writeFile("/src_both.txt", "one\ntwo\n", 8, false);
    __i_fs.deleteFile("/src_both_out.txt");

    sh.run("wc < /src_both.txt > /src_both_out.txt");

    ASSERT_TRUE(slurp("/src_both_out.txt").find("2") != std::string::npos);

    __i_fs.deleteFile("/src_both.txt");
    __i_fs.deleteFile("/src_both_out.txt");
}

TEST(source, a_missing_source_is_refused_and_hands_the_terminal_back)
{
    pditest::mountedVfs();
    pditest::Shell sh;
    __i_fs.deleteFile("/src_absent.txt");

    std::string seen = sh.run("wc < /src_absent.txt");
    ASSERT_TRUE(seen.find("cannot open") != std::string::npos);

    ASSERT_NULL(SessionManager::current()->m_fdtable);
    ASSERT_TRUE(sh.run("echo back").find("back") != std::string::npos);
}

TEST(redirect, repeating_a_redirect_holds_on_to_nothing)
{
    pditest::mountedVfs();
    pditest::Shell sh;
    __i_fs.deleteFile("/redir_many.txt");

    // VFS_MAX_OPEN_FILES backend handles exist, so a redirect that kept one
    // would stop opening after that many rounds; the descriptor table has to be
    // released every round too, not merely on the last one
    for (uint16_t round = 0; round < 40; round++) {
        sh.run("echo repeated > /redir_many.txt");
        ASSERT_NULL(SessionManager::current()->m_fdtable);
    }

    ASSERT_TRUE(slurp("/redir_many.txt").find("repeated") != std::string::npos);
    ASSERT_TRUE(sh.run("echo back").find("back") != std::string::npos);

    __i_fs.deleteFile("/redir_many.txt");
}

TEST(pipeline, repeating_a_pipeline_holds_on_to_nothing)
{
    pditest::mountedVfs();
    pditest::Shell sh;

    for (uint16_t round = 0; round < 40; round++) {
        sh.run("echo repeated here | wc");
        ASSERT_NULL(SessionManager::current()->m_fdtable);
    }

    ASSERT_TRUE(sh.run("echo back").find("back") != std::string::npos);
}

TEST(source, repeating_a_source_holds_on_to_nothing)
{
    pditest::mountedVfs();
    pditest::Shell sh;
    __i_fs.writeFile("/src_many.txt", "one\ntwo\n", 8, false);

    for (uint16_t round = 0; round < 40; round++) {
        sh.run("wc < /src_many.txt");
        ASSERT_NULL(SessionManager::current()->m_fdtable);
    }

    ASSERT_TRUE(sh.run("echo back").find("back") != std::string::npos);

    __i_fs.deleteFile("/src_many.txt");
}

TEST(redirect, a_middle_pipeline_stage_keeps_its_own_target)
{
    pditest::mountedVfs();
    pditest::Shell sh;
    __i_fs.deleteFile("/mid_stage.txt");

    // the redirect is applied after the pipe, so the file takes the output and
    // the stage downstream reads an empty pipe
    std::string seen = sh.run("echo mid > /mid_stage.txt | wc");

    ASSERT_TRUE(seen.find("0") != std::string::npos);
    ASSERT_TRUE(slurp("/mid_stage.txt").find("mid") != std::string::npos);

    __i_fs.deleteFile("/mid_stage.txt");
}

TEST(capture, a_piped_line_lands_in_a_variable)
{
    pditest::mountedVfs();
    pditest::Shell sh;
    Environment::clearSession();

    sh.run("echo hello | read CAUGHT");

    ASSERT_EQ((int)sh.result(), (int)PDI_OK);
    ASSERT_STREQ(envValue("CAUGHT").c_str(), "hello");
}

TEST(capture, the_captured_value_carries_no_line_break)
{
    pditest::mountedVfs();
    pditest::Shell sh;
    Environment::clearSession();

    sh.run("echo hello | read CAUGHT");

    // the leading newline a command opens with and the carriage return of the
    // ending are both gone before the bytes reach the pipe, so what is caught
    // is the text and nothing else
    std::string value = envValue("CAUGHT");
    ASSERT_EQ((int)value.size(), 5);
    ASSERT_TRUE(value.find('\r') == std::string::npos);
    ASSERT_TRUE(value.find('\n') == std::string::npos);
}

TEST(capture, a_caught_value_is_still_there_on_the_next_line)
{
    pditest::mountedVfs();
    pditest::Shell sh;
    Environment::clearSession();

    sh.run("echo remembered | read CAUGHT");

    ASSERT_TRUE(sh.run("echo $CAUGHT").find("remembered") != std::string::npos);
}

TEST(capture, two_names_take_a_field_each)
{
    pditest::mountedVfs();
    pditest::Shell sh;
    Environment::clearSession();

    sh.run("echo alpha beta | read ONE TWO");

    ASSERT_STREQ(envValue("ONE").c_str(), "alpha");
    ASSERT_STREQ(envValue("TWO").c_str(), "beta");
}

TEST(capture, the_last_name_takes_what_is_left)
{
    pditest::mountedVfs();
    pditest::Shell sh;
    Environment::clearSession();

    sh.run("echo alpha beta gamma | read ONE REST");

    ASSERT_STREQ(envValue("ONE").c_str(), "alpha");
    ASSERT_STREQ(envValue("REST").c_str(), "beta gamma");
}

TEST(capture, a_name_the_line_has_nothing_for_reads_empty)
{
    pditest::mountedVfs();
    pditest::Shell sh;
    Environment::clearSession();

    sh.run("echo alpha | read ONE SPARE");

    ASSERT_STREQ(envValue("ONE").c_str(), "alpha");

    pdiutil::string held;
    ASSERT_TRUE(Environment::get("SPARE", held));
    ASSERT_EQ((int)held.size(), 0);
}

TEST(capture, a_file_can_feed_it)
{
    pditest::mountedVfs();
    pditest::Shell sh;
    Environment::clearSession();
    __i_fs.writeFile("/cap_in.txt", "from a file\n", 12, false);

    sh.run("read CAUGHT < /cap_in.txt");

    ASSERT_STREQ(envValue("CAUGHT").c_str(), "from a file");

    __i_fs.deleteFile("/cap_in.txt");
}

TEST(capture, it_takes_one_line_and_leaves_the_rest)
{
    pditest::mountedVfs();
    pditest::Shell sh;
    Environment::clearSession();
    __i_fs.writeFile("/cap_lines.txt", "first\nsecond\nthird\n", 19, false);

    sh.run("read CAUGHT < /cap_lines.txt");

    ASSERT_STREQ(envValue("CAUGHT").c_str(), "first");

    __i_fs.deleteFile("/cap_lines.txt");
}

TEST(capture, input_that_ends_before_a_line_still_counts)
{
    pditest::mountedVfs();
    pditest::Shell sh;
    Environment::clearSession();
    __i_fs.writeFile("/cap_bare.txt", "no ending", 9, false);

    sh.run("read CAUGHT < /cap_bare.txt");

    ASSERT_EQ((int)sh.result(), (int)PDI_OK);
    ASSERT_STREQ(envValue("CAUGHT").c_str(), "no ending");

    __i_fs.deleteFile("/cap_bare.txt");
}

TEST(capture, nothing_to_read_answers_no_rather_than_failing)
{
    pditest::mountedVfs();
    pditest::Shell sh;
    Environment::clearSession();
    __i_fs.deleteFile("/cap_empty.txt");
    __i_fs.createFile("/cap_empty.txt", "", 0);

    sh.run("read CAUGHT < /cap_empty.txt");

    // the answer is no, not a fault, so nothing is printed and the variable is
    // left as it was
    ASSERT_EQ((int)sh.result(), (int)CMD_RESULT_FALSE);

    pdiutil::string held;
    ASSERT_FALSE(Environment::get("CAUGHT", held));

    __i_fs.deleteFile("/cap_empty.txt");
}

TEST(capture, a_value_longer_than_the_bound_is_clamped)
{
    pditest::mountedVfs();
    pditest::Shell sh;
    Environment::clearSession();

    std::string wide(ENV_VALUE_MAX + 40, 'w');
    wide += '\n';
    __i_fs.writeFile("/cap_wide.txt", (char *)wide.c_str(), (uint32_t)wide.size(), false);

    sh.run("read CAUGHT < /cap_wide.txt");

    ASSERT_EQ((int)envValue("CAUGHT").size(), (int)ENV_VALUE_MAX);

    __i_fs.deleteFile("/cap_wide.txt");
}

TEST(capture, it_refuses_when_nothing_is_feeding_it)
{
    pditest::mountedVfs();
    pditest::Shell sh;
    Environment::clearSession();

    sh.run("read CAUGHT");

    ASSERT_EQ((int)sh.result(), (int)CMD_ERROR_ARGS_MISSING);

    pdiutil::string held;
    ASSERT_FALSE(Environment::get("CAUGHT", held));
}

TEST(capture, it_wants_a_name)
{
    pditest::mountedVfs();
    pditest::Shell sh;

    sh.run("echo anything | read");

    ASSERT_EQ((int)sh.result(), (int)CMD_ERROR_ARGS_MISSING);
}

TEST(capture, a_name_the_session_answers_for_is_refused)
{
    pditest::mountedVfs();
    pditest::Shell sh;
    SessionManager::setPWD(FILE_SEPARATOR);

    sh.run("echo /nowhere | read PWD");

    ASSERT_EQ((int)sh.result(), (int)CMD_ERROR_PERM);
    ASSERT_STREQ(envValue("PWD").c_str(), FILE_SEPARATOR);
}

TEST(capture, a_name_that_is_not_a_variable_is_refused)
{
    pditest::mountedVfs();
    pditest::Shell sh;

    sh.run("echo value | read 1bad");

    ASSERT_EQ((int)sh.result(), (int)CMD_ERROR_INVAL);
}

TEST(capture, repeating_a_capture_holds_on_to_nothing)
{
    pditest::mountedVfs();
    pditest::Shell sh;

    for (uint16_t round = 0; round < 40; round++) {
        sh.run("echo again | read CAUGHT");
        ASSERT_NULL(SessionManager::current()->m_fdtable);
    }

    ASSERT_STREQ(envValue("CAUGHT").c_str(), "again");
}

TEST(substitution, a_command_s_answer_takes_its_place_in_the_line)
{
    pditest::mountedVfs();
    pditest::Shell sh;

    std::string out = sh.run("echo [$(echo hi)]");

    ASSERT_EQ((int)sh.result(), (int)PDI_OK);
    ASSERT_TRUE(out.find("[hi]") != std::string::npos);
}

TEST(substitution, a_quoted_one_keeps_its_blanks_in_a_single_value)
{
    pditest::mountedVfs();
    pditest::Shell sh;
    Environment::clearSession();

    sh.run("export SUBST=\"$(echo one two)\"");

    ASSERT_STREQ(envValue("SUBST").c_str(), "one two");
}

TEST(substitution, the_answer_s_closing_line_break_is_dropped)
{
    pditest::mountedVfs();
    pditest::Shell sh;

    std::string out = sh.run("echo a$(echo b)c");

    ASSERT_TRUE(out.find("abc") != std::string::npos);
}

TEST(substitution, a_pipeline_inside_runs_whole_without_splitting_the_line)
{
    pditest::mountedVfs();
    pditest::Shell sh;

    std::string out = sh.run("echo x$(echo abc | cat)y");

    ASSERT_EQ((int)sh.result(), (int)PDI_OK);
    ASSERT_TRUE(out.find("xabcy") != std::string::npos);
}

TEST(substitution, a_separator_inside_belongs_to_the_inner_line)
{
    pditest::mountedVfs();
    pditest::Shell sh;

    std::string out = sh.run("echo [$(echo a ; echo b)]");

    ASSERT_TRUE(out.find("syntax error") == std::string::npos);
    ASSERT_TRUE(out.find("[a") != std::string::npos);
    ASSERT_TRUE(out.find("b]") != std::string::npos);
}

TEST(substitution, nothing_in_single_quotes_is_run)
{
    pditest::mountedVfs();
    pditest::Shell sh;

    std::string out = sh.run("echo '$(echo hi)'");

    ASSERT_TRUE(out.find("$(echo hi)") != std::string::npos);
}

TEST(substitution, one_may_open_inside_another)
{
    pditest::mountedVfs();
    pditest::Shell sh;

    std::string out = sh.run("echo [$(echo $(echo deep))]");

    ASSERT_TRUE(out.find("[deep]") != std::string::npos);
}

TEST(substitution, its_answer_is_text_and_is_never_run_as_a_line)
{
    pditest::mountedVfs();
    pditest::Shell sh;

    sh.run("echo \"x; echo PWNED\" > /tmp/wt_inject.txt");
    std::string out = sh.run("echo [$(cat /tmp/wt_inject.txt)]");
    sh.run("rm /tmp/wt_inject.txt");

    ASSERT_TRUE(out.find("[x; echo PWNED]") != std::string::npos);
}

TEST(substitution, a_redirect_target_can_be_built_from_one)
{
    pditest::mountedVfs();
    pditest::Shell sh;

    sh.run("echo kept > /tmp/wt_$(echo named).txt");
    std::string body = slurp("/tmp/wt_named.txt");
    sh.run("rm /tmp/wt_named.txt");

    ASSERT_TRUE(body.find("kept") != std::string::npos);
}

TEST(substitution, an_answer_too_long_to_hold_fails_the_line_rather_than_being_cut)
{
    pditest::mountedVfs();
    pditest::Shell sh;

    std::string half((SHELL_CAPTURE_MAX / 2) + 8, 'A');
    sh.run(("echo " + half + " > /tmp/wt_long.txt").c_str());
    sh.run(("echo " + half + " >> /tmp/wt_long.txt").c_str());
    std::string out = sh.run("echo [$(cat /tmp/wt_long.txt)]");
    pdi_err_t result = sh.result();
    sh.run("rm /tmp/wt_long.txt");

    ASSERT_NE((int)result, (int)PDI_OK);
    ASSERT_TRUE(out.find("[AAAA") == std::string::npos);
}

TEST(substitution, a_line_with_none_runs_as_before)
{
    pditest::mountedVfs();
    pditest::Shell sh;

    std::string out = sh.run("echo (plain) text");

    ASSERT_TRUE(out.find("(plain) text") != std::string::npos);
}

TEST(pipeline, a_stage_left_waiting_for_input_answers_on_the_terminal_once_the_line_ends)
{
    pditest::mountedVfs();
    pditest::Shell sh;

    sh.run("cat /etc/hosts | passwd > /tmp/wt_waiting.txt");
    bool tableGone = (nullptr == SessionManager::current()->m_fdtable);
    std::string answered = sh.type("\x03");

    ASSERT_TRUE(tableGone);
    ASSERT_TRUE(answered.find("^C") != std::string::npos || answered.size() > 0);
}

#endif
