/**************************** Command Base Tests ******************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 16th Aug 2026
******************************************************************************/

#include <pditest.h>
#include <StringTerminal.h>
#include <utility/CommandBase.h>

/**
 * CommandBase leaves execute() pure, and clears the parsed options as soon as
 * execute() returns, so a test reads them from inside execute the way a real
 * command does.
 */
namespace
{

struct TestCommand : public cmd_t
{
    char seen[CMD_OPTION_MAX][64];
    bool present[CMD_OPTION_MAX];
    bool ran;

    TestCommand() { forget(); }

    void forget()
    {
        ran = false;
        for (uint8_t i = 0; i < CMD_OPTION_MAX; i++)
        {
            present[i] = false;
            memset(seen[i], 0, sizeof(seen[i]));
        }
    }

    pdi_err_t execute(cmd_term_inseq_t terminputaction) override
    {
        ran = true;
        for (uint8_t i = 0; i < CMD_OPTION_MAX; i++)
        {
            if (nullptr != m_options[i].optionval && m_options[i].optionvalsize > 0)
            {
                // the value points into the command line and is bounded by
                // optionvalsize, not by a nul at the option boundary
                size_t len = (size_t)m_options[i].optionvalsize;
                if (len > sizeof(seen[i]) - 1)
                {
                    len = sizeof(seen[i]) - 1;
                }
                present[i] = true;
                memcpy(seen[i], m_options[i].optionval, len);
                seen[i][len] = '\0';
            }
        }
        return PDI_OK;
    }

    /**
     * @brief Value captured for a named option during execute, or nullptr.
     */
    const char *valueOf(const char *name)
    {
        int8_t index = getOptionIndex(name);
        if (index < 0 || !present[index])
        {
            return nullptr;
        }
        return seen[index];
    }
};

} // namespace

/**
 * Build a command with the given name and options, ready to parse a line.
 */
static void makeCommand(cmd_t &command, const char *name, const char *optn1 = nullptr,
                        const char *optn2 = nullptr)
{
    command.Clear();
    command.SetCommand(name);
    if (nullptr != optn1)
    {
        command.AddOption(optn1);
    }
    if (nullptr != optn2)
    {
        command.AddOption(optn2);
    }
}

/**
 * executeCommand parses in place, so the line has to be a writable copy.
 */
static pdi_err_t parse(cmd_t &command, const char *line, char *scratch, size_t scratchsize)
{
    memset(scratch, 0, scratchsize);
    strcpy(scratch, line);
    return command.executeCommand(scratch, (int16_t)strlen(scratch));
}

TEST(cmdbase, set_command_accepts_a_short_name)
{
    TestCommand command;
    command.Clear();
    ASSERT_TRUE(command.SetCommand("ls"));
}

TEST(cmdbase, set_command_rejects_a_name_at_the_size_limit)
{
    TestCommand command;
    command.Clear();
    char toolong[CMD_SIZE_MAX + 2];
    memset(toolong, 'a', CMD_SIZE_MAX + 1);
    toolong[CMD_SIZE_MAX + 1] = '\0';
    ASSERT_FALSE(command.SetCommand(toolong));
}

TEST(cmdbase, set_command_rejects_null)
{
    TestCommand command;
    command.Clear();
    ASSERT_FALSE(command.SetCommand(nullptr));
}

TEST(cmdbase, add_option_rejects_an_oversized_option)
{
    TestCommand command;
    command.Clear();
    char toolong[CMD_OPTION_SIZE_MAX + 2];
    memset(toolong, 'o', CMD_OPTION_SIZE_MAX + 1);
    toolong[CMD_OPTION_SIZE_MAX + 1] = '\0';
    ASSERT_FALSE(command.AddOption(toolong));
}

TEST(cmdbase, add_option_stops_at_the_option_limit)
{
    TestCommand command;
    command.Clear();

    for (uint8_t i = 0; i < CMD_OPTION_MAX; i++)
    {
        char optn[CMD_OPTION_SIZE_MAX];
        memset(optn, 0, sizeof(optn));
        optn[0] = (char)('a' + i);
        ASSERT_TRUE(command.AddOption(optn));
    }

    ASSERT_FALSE(command.AddOption("z"));
}

TEST(cmdbase, command_match_accepts_exact_name)
{
    ASSERT_TRUE(cmd_t::isCommandMatch("ls", "ls"));
}

TEST(cmdbase, command_match_accepts_name_followed_by_args)
{
    ASSERT_TRUE(cmd_t::isCommandMatch("ls", "ls /etc"));
}

TEST(cmdbase, command_match_rejects_a_different_name)
{
    ASSERT_FALSE(cmd_t::isCommandMatch("ls", "cat"));
}

TEST(cmdbase, command_match_rejects_null)
{
    ASSERT_FALSE(cmd_t::isCommandMatch("ls", nullptr));
}

TEST(cmdbase, command_match_supports_partial_prefix)
{
    ASSERT_TRUE(cmd_t::isCommandMatch("ls", "l", true));
    ASSERT_FALSE(cmd_t::isCommandMatch("ls", "x", true));
}

TEST(cmdbase, parses_a_bare_command)
{
    TestCommand command;
    char scratch[64];
    makeCommand(command, "pwd");

    ASSERT_EQ(parse(command, "pwd", scratch, sizeof(scratch)), PDI_OK);
}

TEST(cmdbase, rejects_a_line_for_another_command)
{
    TestCommand command;
    char scratch[64];
    makeCommand(command, "pwd");

    ASSERT_EQ(parse(command, "cat", scratch, sizeof(scratch)), CMD_ERROR_INVALID);
}

TEST(cmdbase, parses_a_named_option)
{
    TestCommand command;
    char scratch[64];
    makeCommand(command, "ssh", "t");

    ASSERT_EQ(parse(command, "ssh t=2", scratch, sizeof(scratch)), PDI_OK);
    ASSERT_TRUE(command.ran);
    ASSERT_STREQ(command.valueOf("t"), "2");
}

TEST(cmdbase, parses_two_named_options)
{
    TestCommand command;
    char scratch[64];
    makeCommand(command, "sshkgen", "t", "f");

    ASSERT_EQ(parse(command, "sshkgen t=1,f=2", scratch, sizeof(scratch)), PDI_OK);
    ASSERT_STREQ(command.valueOf("t"), "1");
    ASSERT_STREQ(command.valueOf("f"), "2");
}

TEST(cmdbase, parses_a_multi_character_option_value)
{
    TestCommand command;
    char scratch[64];
    makeCommand(command, "tls", "n");

    ASSERT_EQ(parse(command, "tls n=device.local", scratch, sizeof(scratch)), PDI_OK);
    ASSERT_STREQ(command.valueOf("n"), "device.local");
}

TEST(cmdbase, options_are_cleared_once_the_command_has_run)
{
    TestCommand command;
    char scratch[64];
    makeCommand(command, "ssh", "t");

    parse(command, "ssh t=2", scratch, sizeof(scratch));
    ASSERT_NULL(command.RetrieveOption("t"));
}

TEST(cmdbase, reports_an_unknown_named_option)
{
    TestCommand command;
    char scratch[64];
    makeCommand(command, "ssh", "t");

    ASSERT_EQ(parse(command, "ssh q=9", scratch, sizeof(scratch)), CMD_ERROR_OPT);
}

TEST(cmdbase, retrieve_option_returns_null_for_an_unset_option)
{
    TestCommand command;
    char scratch[64];
    makeCommand(command, "ssh", "t");
    parse(command, "ssh", scratch, sizeof(scratch));

    ASSERT_NULL(command.RetrieveOption("t"));
}

TEST(cmdbase, retrieve_option_returns_null_for_null_name)
{
    TestCommand command;
    command.Clear();
    ASSERT_NULL(command.RetrieveOption(nullptr));
}

TEST(cmdbase, parses_a_positional_argument)
{
    TestCommand command;
    char scratch[64];
    makeCommand(command, "cat", "a");
    command.setAcceptArgsOptions(true);

    ASSERT_EQ(parse(command, "cat /etc/passwd", scratch, sizeof(scratch)), PDI_OK);
    ASSERT_TRUE(command.present[0]);
    ASSERT_STREQ(command.seen[0], "/etc/passwd");
}

TEST(cmdbase, trims_whitespace_around_a_positional_argument)
{
    TestCommand command;
    char scratch[64];
    makeCommand(command, "cat", "a");
    command.setAcceptArgsOptions(true);

    ASSERT_EQ(parse(command, "cat   /etc/hosts   ", scratch, sizeof(scratch)), PDI_OK);
    ASSERT_TRUE(command.present[0]);
    ASSERT_STREQ(command.seen[0], "/etc/hosts");
}

TEST(cmdbase, parses_two_positional_arguments)
{
    TestCommand command;
    char scratch[64];
    makeCommand(command, "ping", "a", "b");
    command.setAcceptArgsOptions(true);
    command.setCmdOptionSeparator(CMD_OPTION_SEPERATOR_SPACE);

    ASSERT_EQ(parse(command, "ping example.com 3", scratch, sizeof(scratch)), PDI_OK);
    ASSERT_STREQ(command.seen[0], "example.com");
    ASSERT_STREQ(command.seen[1], "3");
}

TEST(cmdbase, a_quoted_value_keeps_the_separator_inside_it)
{
    TestCommand command;
    char scratch[96];
    makeCommand(command, "watch", "c", "i");
    command.setCmdOptionSeparator(CMD_OPTION_SEPERATOR_COMMA);

    ASSERT_EQ(parse(command, "watch c=\"login u=a,p=b\",i=300", scratch, sizeof(scratch)), PDI_OK);
    ASSERT_STREQ(command.seen[0], "login u=a,p=b");
    ASSERT_STREQ(command.seen[1], "300");
}

TEST(cmdbase, quotes_are_removed_from_the_middle_of_a_value)
{
    TestCommand command;
    char scratch[64];
    makeCommand(command, "echo", "a");
    command.setAcceptArgsOptions(true);

    ASSERT_EQ(parse(command, "echo a\"b\"c", scratch, sizeof(scratch)), PDI_OK);
    ASSERT_STREQ(command.seen[0], "abc");
}

TEST(cmdbase, a_single_quoted_run_keeps_a_double_quote_as_text)
{
    TestCommand command;
    char scratch[64];
    makeCommand(command, "echo", "a");
    command.setAcceptArgsOptions(true);

    ASSERT_EQ(parse(command, "echo \'a\"b\'", scratch, sizeof(scratch)), PDI_OK);
    ASSERT_STREQ(command.seen[0], "a\"b");
}

TEST(cmdbase, a_backslash_escapes_a_quote_in_a_value)
{
    TestCommand command;
    char scratch[64];
    makeCommand(command, "echo", "a");
    command.setAcceptArgsOptions(true);

    ASSERT_EQ(parse(command, "echo a\\\"b", scratch, sizeof(scratch)), PDI_OK);
    ASSERT_STREQ(command.seen[0], "a\"b");
}

TEST(cmdbase, clear_resets_the_command_name)
{
    TestCommand command;
    makeCommand(command, "ssh", "t");
    ASSERT_TRUE(command.isValidCommand("ssh"));

    command.Clear();
    ASSERT_FALSE(command.isValidCommand("ssh"));
}

/**
 * A positional command declaring the words it takes, with its replies caught.
 */
static void makeDeclared(TestCommand &command, pditest::StringTerminal &term, const char *flags,
                         int16_t operands, bool keyvalue = false)
{
    makeCommand(command, "ls", "a", "b");
    command.setAcceptArgsOptions(true);
    command.setCmdOptionSeparator(CMD_OPTION_SEPERATOR_SPACE);
    command.setArgs(flags, operands, keyvalue);
    command.SetTerminal(&term);
    command.forget();
    term.forget();
}

TEST(cmdbase, declared_flags_and_operands_run_the_command)
{
    TestCommand command;
    pditest::StringTerminal term;
    char scratch[64];
    makeDeclared(command, term, "la", 1);

    ASSERT_EQ(parse(command, "ls -l -a -la /etc", scratch, sizeof(scratch)), PDI_OK);
    ASSERT_TRUE(command.ran);
    ASSERT_FALSE(term.sawText(CMD_MSG_INVALID_OPTION));

    makeDeclared(command, term, "la", 1);
    ASSERT_EQ(parse(command, "ls", scratch, sizeof(scratch)), PDI_OK);
    ASSERT_TRUE(command.ran);
}

TEST(cmdbase, an_undeclared_flag_is_named_and_nothing_runs)
{
    TestCommand command;
    pditest::StringTerminal term;
    char scratch[64];
    makeDeclared(command, term, "la", 1);

    ASSERT_EQ(parse(command, "ls -Z /etc", scratch, sizeof(scratch)), CMD_ERROR_OPT);
    ASSERT_FALSE(command.ran);
    ASSERT_TRUE(term.sawText("invalid option : -Z"));

    makeDeclared(command, term, "la", 1);
    ASSERT_EQ(parse(command, "ls -laZ", scratch, sizeof(scratch)), CMD_ERROR_OPT);
    ASSERT_FALSE(command.ran);
    ASSERT_TRUE(term.sawText("invalid option : -laZ"));

    makeDeclared(command, term, "", 2);
    ASSERT_EQ(parse(command, "ls --help", scratch, sizeof(scratch)), CMD_ERROR_OPT);
    ASSERT_FALSE(command.ran);
    ASSERT_TRUE(term.sawText("invalid option : --help"));
}

TEST(cmdbase, an_operand_past_the_last_is_named_and_nothing_runs)
{
    TestCommand command;
    pditest::StringTerminal term;
    char scratch[64];
    makeDeclared(command, term, "", 0);

    ASSERT_EQ(parse(command, "ls now", scratch, sizeof(scratch)), CMD_ERROR_INVAL);
    ASSERT_FALSE(command.ran);
    ASSERT_TRUE(term.sawText("extra operand : now"));

    makeDeclared(command, term, "la", 1);
    ASSERT_EQ(parse(command, "ls /a /b", scratch, sizeof(scratch)), CMD_ERROR_INVAL);
    ASSERT_FALSE(command.ran);
    ASSERT_TRUE(term.sawText("extra operand : /b"));
}

TEST(cmdbase, quotes_and_escapes_keep_one_operand_whole)
{
    TestCommand command;
    pditest::StringTerminal term;
    char scratch[64];
    makeDeclared(command, term, "", 1);

    ASSERT_EQ(parse(command, "ls \"a b c\"", scratch, sizeof(scratch)), PDI_OK);
    ASSERT_TRUE(command.ran);

    makeDeclared(command, term, "", 1);
    ASSERT_EQ(parse(command, "ls 'a b' c", scratch, sizeof(scratch)), CMD_ERROR_INVAL);
    ASSERT_TRUE(term.sawText("extra operand : c"));

    makeDeclared(command, term, "", 1);
    ASSERT_EQ(parse(command, "ls a\\ b", scratch, sizeof(scratch)), PDI_OK);
    ASSERT_TRUE(command.ran);

    makeDeclared(command, term, "", 1);
    ASSERT_EQ(parse(command, "ls \"-Z\"", scratch, sizeof(scratch)), PDI_OK);
    ASSERT_TRUE(command.ran);
    ASSERT_STREQ(command.seen[0], "-Z");
}

TEST(cmdbase, a_given_empty_word_is_an_operand)
{
    TestCommand command;
    pditest::StringTerminal term;
    char scratch[64];
    makeDeclared(command, term, "", 1);

    ASSERT_EQ(parse(command, "ls \"\"", scratch, sizeof(scratch)), PDI_OK);
    ASSERT_TRUE(command.ran);

    makeDeclared(command, term, "", 1);
    ASSERT_EQ(parse(command, "ls \"\" \"\"", scratch, sizeof(scratch)), CMD_ERROR_INVAL);
    ASSERT_FALSE(command.ran);
}

TEST(cmdbase, a_lone_dash_and_a_negative_number_are_operands_where_no_flags_are_declared)
{
    TestCommand command;
    pditest::StringTerminal term;
    char scratch[64];
    makeDeclared(command, term, "", 1);

    ASSERT_EQ(parse(command, "ls -", scratch, sizeof(scratch)), PDI_OK);
    ASSERT_TRUE(command.ran);

    makeDeclared(command, term, nullptr, 2);
    ASSERT_EQ(parse(command, "ls -20 5", scratch, sizeof(scratch)), PDI_OK);
    ASSERT_TRUE(command.ran);
}

TEST(cmdbase, a_name_value_command_refuses_a_word_without_a_value)
{
    TestCommand command;
    pditest::StringTerminal term;
    char scratch[64];
    makeDeclared(command, term, "", 2, true);

    ASSERT_EQ(parse(command, "ls a=1 b=2", scratch, sizeof(scratch)), PDI_OK);
    ASSERT_TRUE(command.ran);

    makeDeclared(command, term, "", 2, true);
    ASSERT_EQ(parse(command, "ls a=1 junk", scratch, sizeof(scratch)), CMD_ERROR_OPT);
    ASSERT_FALSE(command.ran);
    ASSERT_TRUE(term.sawText("invalid option : junk"));

    makeDeclared(command, term, "", 2, true);
    ASSERT_EQ(parse(command, "ls -Z", scratch, sizeof(scratch)), CMD_ERROR_OPT);
    ASSERT_FALSE(command.ran);

    makeDeclared(command, term, "", 1, true);
    ASSERT_EQ(parse(command, "ls a=1 b=2", scratch, sizeof(scratch)), CMD_ERROR_INVAL);
    ASSERT_FALSE(command.ran);
    ASSERT_TRUE(term.sawText("extra operand : b=2"));
}

TEST(cmdbase, a_name_the_command_does_not_have_is_named)
{
    TestCommand command;
    pditest::StringTerminal term;
    char scratch[64];
    makeDeclared(command, term, "", 2, true);

    ASSERT_EQ(parse(command, "ls a=1 z=2", scratch, sizeof(scratch)), CMD_ERROR_OPT);
    ASSERT_FALSE(command.ran);
    ASSERT_TRUE(term.sawText("invalid option : z"));
}

TEST(cmdbase, a_300_character_word_is_named_whole)
{
    TestCommand command;
    pditest::StringTerminal term;
    char scratch[400];
    makeDeclared(command, term, "", 0);

    std::string word(300, 'w');
    std::string line = "ls " + word;
    ASSERT_EQ(parse(command, line.c_str(), scratch, sizeof(scratch)), CMD_ERROR_INVAL);
    ASSERT_FALSE(command.ran);
    ASSERT_TRUE(term.sawText((std::string("extra operand : ") + word).c_str()));
}

TEST(cmdbase, an_undeclared_command_takes_any_word)
{
    TestCommand command;
    pditest::StringTerminal term;
    char scratch[64];
    makeCommand(command, "echo", "a");
    command.setAcceptArgsOptions(true);
    command.SetTerminal(&term);

    ASSERT_EQ(parse(command, "echo -Z a b c", scratch, sizeof(scratch)), PDI_OK);
    ASSERT_TRUE(command.ran);
}

TEST(cmdbase, take_number_reads_the_range_and_names_a_refused_word)
{
    TestCommand command;
    pditest::StringTerminal term;
    command.SetTerminal(&term);
    uint32_t out = 9;

    ASSERT_EQ(command.takeNumber("invalid pid : ", "0", 1, 0, 32767, out), PDI_OK);
    ASSERT_EQ(out, 0u);
    ASSERT_EQ(command.takeNumber("invalid pid : ", "32767", 5, 0, 32767, out), PDI_OK);
    ASSERT_EQ(out, 32767u);
    ASSERT_FALSE(term.sawText("invalid"));

    const char *refused[] = { "32768", "4294967296", "-1", "x", "1x", " 1", "99999999999999999999" };
    for (const char *word : refused)
    {
        term.forget();
        ASSERT_EQ(command.takeNumber("invalid pid : ", word, (int16_t)strlen(word), 0, 32767, out), CMD_ERROR_INVAL);
        ASSERT_TRUE(term.sawText((std::string("invalid pid : ") + word + " (0 to 32767)").c_str()));
    }

    term.forget();
    ASSERT_EQ(command.takeNumber("invalid count : ", "0", 1, 1, 10, out), CMD_ERROR_INVAL);
    ASSERT_EQ(command.takeNumber("invalid count : ", "", 0, 0, 10, out), CMD_ERROR_INVAL);
}

TEST(cmdbase, a_failure_names_what_failed_and_why_in_words)
{
    TestCommand command;
    pditest::StringTerminal term;
    command.SetTerminal(&term);

    command.writeFailure("cp: ", "/a", PDI_ERR_EXISTS);
    ASSERT_EQ(term.captured(), std::string("cp: /a : File exists"));

    term.forget();
    command.writeFailure("cp: ", "", PDI_ERR_NOT_FOUND);
    ASSERT_EQ(term.captured(), std::string("cp: '' : Not found"));

    term.forget();
    command.writeFailure("cp: ", "/a", -32000);
    ASSERT_EQ(term.captured(), std::string("cp: /a : -32000"));

    term.forget();
    command.writeName(nullptr);
    ASSERT_EQ(term.captured(), std::string("''"));
}

