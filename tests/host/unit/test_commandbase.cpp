/**************************** Command Base Tests ******************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 16th Aug 2026
******************************************************************************/

#include <pditest.h>
#include <utility/CommandBase.h>

/**
 * CommandBase leaves execute() pure, and clears the parsed options as soon as
 * execute() returns, so a test reads them from inside execute the way a real
 * command does.
 */
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

    cmd_result_t execute(cmd_term_inseq_t terminputaction) override
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
        return CMD_RESULT_OK;
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
static cmd_result_t parse(cmd_t &command, const char *line, char *scratch, size_t scratchsize)
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

    ASSERT_EQ(parse(command, "pwd", scratch, sizeof(scratch)), CMD_RESULT_OK);
}

TEST(cmdbase, rejects_a_line_for_another_command)
{
    TestCommand command;
    char scratch[64];
    makeCommand(command, "pwd");

    ASSERT_EQ(parse(command, "cat", scratch, sizeof(scratch)), CMD_RESULT_INVALID);
}

TEST(cmdbase, parses_a_named_option)
{
    TestCommand command;
    char scratch[64];
    makeCommand(command, "ssh", "t");

    ASSERT_EQ(parse(command, "ssh t=2", scratch, sizeof(scratch)), CMD_RESULT_OK);
    ASSERT_TRUE(command.ran);
    ASSERT_STREQ(command.valueOf("t"), "2");
}

TEST(cmdbase, parses_two_named_options)
{
    TestCommand command;
    char scratch[64];
    makeCommand(command, "sshkgen", "t", "f");

    ASSERT_EQ(parse(command, "sshkgen t=1,f=2", scratch, sizeof(scratch)), CMD_RESULT_OK);
    ASSERT_STREQ(command.valueOf("t"), "1");
    ASSERT_STREQ(command.valueOf("f"), "2");
}

TEST(cmdbase, parses_a_multi_character_option_value)
{
    TestCommand command;
    char scratch[64];
    makeCommand(command, "tls", "n");

    ASSERT_EQ(parse(command, "tls n=device.local", scratch, sizeof(scratch)), CMD_RESULT_OK);
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

    ASSERT_EQ(parse(command, "ssh q=9", scratch, sizeof(scratch)), CMD_RESULT_INVALID_OPTION);
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

    ASSERT_EQ(parse(command, "cat /etc/passwd", scratch, sizeof(scratch)), CMD_RESULT_OK);
    ASSERT_TRUE(command.present[0]);
    ASSERT_STREQ(command.seen[0], "/etc/passwd");
}

TEST(cmdbase, trims_whitespace_around_a_positional_argument)
{
    TestCommand command;
    char scratch[64];
    makeCommand(command, "cat", "a");
    command.setAcceptArgsOptions(true);

    ASSERT_EQ(parse(command, "cat   /etc/hosts   ", scratch, sizeof(scratch)), CMD_RESULT_OK);
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

    ASSERT_EQ(parse(command, "ping example.com 3", scratch, sizeof(scratch)), CMD_RESULT_OK);
    ASSERT_STREQ(command.seen[0], "example.com");
    ASSERT_STREQ(command.seen[1], "3");
}

TEST(cmdbase, clear_resets_the_command_name)
{
    TestCommand command;
    makeCommand(command, "ssh", "t");
    ASSERT_TRUE(command.isValidCommand("ssh"));

    command.Clear();
    ASSERT_FALSE(command.isValidCommand("ssh"));
}
