/************************** File Command Tests ********************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

The file half of the command surface, driven through the same command line
service the serial, telnet and ssh terminals drive.

Author          : Suraj I.
created Date    : 16th Aug 2026
******************************************************************************/

#include <ShellHarness.h>
#include <pditest.h>

using pditest::saw;

/**
 * Every test works inside its own directory under the root, so one leaving
 * something behind cannot change what another sees.
 */
static void scrub(pditest::Shell &shell, const char *dir)
{
    pdiutil::vector<file_info_t> items;
    if (__i_fs.getDirFileList(dir, items) >= 0)
    {
        for (file_info_t &item : items)
        {
            if (nullptr == item.m_name) continue;
            if (0 == strcmp(item.m_name, ".") || 0 == strcmp(item.m_name, "..")) continue;

            pdiutil::string path(dir);
            path += "/";
            path += item.m_name;
            __i_fs.deleteFile(path.c_str());
        }
        for (file_info_t &item : items)
        {
            pdiutil::safe_delete_array(item.m_name);
        }
    }

    __i_fs.deleteDirectory(dir);
    shell.run("pwd");
}

static pditest::Shell *workspace(pditest::Shell &shell, const char *dir)
{
    scrub(shell, dir);
    __i_fs.createDirectory(dir);
    shell.run((pdiutil::string("cd ") + dir).c_str());
    return &shell;
}

/* --------------------------------------------------------------------- ls */

TEST(cmdfs, ls_lists_what_was_created)
{
    pditest::Shell shell;
    workspace(shell, "/w_ls");
    shell.run("touch one.txt");
    shell.run("touch two.txt");

    std::string out = shell.run("ls");
    ASSERT_TRUE(saw(out, "one.txt"));
    ASSERT_TRUE(saw(out, "two.txt"));

    scrub(shell, "/w_ls");
}

TEST(cmdfs, ls_takes_a_directory_argument)
{
    pditest::Shell shell;
    workspace(shell, "/w_lsarg");
    shell.run("touch listed.txt");
    shell.run("cd /");

    std::string out = shell.run("ls /w_lsarg");
    ASSERT_TRUE(saw(out, "listed.txt"));

    scrub(shell, "/w_lsarg");
}

TEST(cmdfs, ls_shows_the_dot_entries_and_the_mode)
{
    pditest::Shell shell;
    workspace(shell, "/w_lsmode");
    shell.run("touch moded.txt");

    std::string out = shell.run("ls");
    ASSERT_TRUE(saw(out, "."));
    ASSERT_TRUE(saw(out, ".."));
    ASSERT_TRUE(saw(out, "-rw-"));

    scrub(shell, "/w_lsmode");
}

/* ------------------------------------------------------------ mkdir, touch */

TEST(cmdfs, mkdir_creates_a_directory)
{
    pditest::Shell shell;
    workspace(shell, "/w_mkdir");

    shell.run("mkdir sub");
    ASSERT_TRUE(__i_fs.isDirectory("/w_mkdir/sub"));

    __i_fs.deleteDirectory("/w_mkdir/sub");
    scrub(shell, "/w_mkdir");
}

TEST(cmdfs, touch_creates_an_empty_file)
{
    pditest::Shell shell;
    workspace(shell, "/w_touch");

    shell.run("touch fresh.txt");
    ASSERT_TRUE(__i_fs.isFileExist("/w_touch/fresh.txt"));
    ASSERT_EQ(__i_fs.getFileSize("/w_touch/fresh.txt"), 0);

    scrub(shell, "/w_touch");
}

/**
 * A free-argument command given on its own used to read past the end of the
 * command line, because the first value is looked for one byte after the
 * command name. It reports an argument error now instead.
 */
TEST(cmdfs, a_command_needing_an_argument_reports_one_missing)
{
    pditest::Shell shell;

    shell.run("touch");
    ASSERT_EQ(shell.result(), CMD_RESULT_ARGS_ERROR);

    shell.run("mkdir");
    ASSERT_EQ(shell.result(), CMD_RESULT_ARGS_ERROR);
}

TEST(cmdfs, a_command_with_no_argument_still_runs_when_it_needs_none)
{
    pditest::Shell shell;

    ASSERT_TRUE(saw(shell.run("pwd"), "/"));
    ASSERT_TRUE(saw(shell.run("ls"), "."));
}

/* -------------------------------------------------------------- echo, cat */

TEST(cmdfs, echo_prints_its_argument)
{
    pditest::Shell shell;
    std::string out = shell.run("echo hello there");

    ASSERT_TRUE(saw(out, "hello there"));
}

TEST(cmdfs, echo_writes_to_a_file_when_redirected)
{
    pditest::Shell shell;
    workspace(shell, "/w_echo");

    shell.run("echo written by echo > note.txt");
    ASSERT_TRUE(__i_fs.isFileExist("/w_echo/note.txt"));
    ASSERT_TRUE(saw(shell.run("cat note.txt"), "written by echo"));

    scrub(shell, "/w_echo");
}

TEST(cmdfs, cat_prints_the_contents)
{
    pditest::Shell shell;
    workspace(shell, "/w_cat");
    shell.run("echo first line > c.txt");

    ASSERT_TRUE(saw(shell.run("cat c.txt"), "first line"));

    scrub(shell, "/w_cat");
}

TEST(cmdfs, cat_of_a_missing_file_does_not_print_content)
{
    pditest::Shell shell;
    workspace(shell, "/w_catmiss");

    std::string out = shell.run("cat absent.txt");
    ASSERT_FALSE(saw(out, "absent content"));
    ASSERT_NE(shell.result(), CMD_RESULT_MAX);

    scrub(shell, "/w_catmiss");
}

/* ------------------------------------------------------- head, tail, wc */

TEST(cmdfs, head_prints_from_the_top)
{
    pditest::Shell shell;
    workspace(shell, "/w_head");
    __i_fs.createFile("/w_head/lines.txt", "alpha\nbravo\ncharlie\ndelta\n");

    std::string out = shell.run("head lines.txt 2");
    ASSERT_TRUE(saw(out, "alpha"));
    ASSERT_TRUE(saw(out, "bravo"));
    ASSERT_FALSE(saw(out, "charlie"));

    scrub(shell, "/w_head");
}

TEST(cmdfs, tail_prints_from_the_bottom)
{
    pditest::Shell shell;
    workspace(shell, "/w_tail");
    __i_fs.createFile("/w_tail/lines.txt", "alpha\nbravo\ncharlie\ndelta\n");

    std::string out = shell.run("tail lines.txt 2");
    ASSERT_TRUE(saw(out, "delta"));
    ASSERT_TRUE(saw(out, "charlie"));
    ASSERT_FALSE(saw(out, "alpha"));

    scrub(shell, "/w_tail");
}

TEST(cmdfs, wc_counts_lines_words_and_bytes)
{
    pditest::Shell shell;
    workspace(shell, "/w_wc");
    __i_fs.createFile("/w_wc/counted.txt", "one two\nthree four\n");

    std::string out = shell.run("wc counted.txt");
    ASSERT_TRUE(saw(out, "2"));
    ASSERT_TRUE(saw(out, "4"));
    ASSERT_TRUE(saw(out, "19"));

    scrub(shell, "/w_wc");
}

/* ------------------------------------------------------------ grep, hexdump */

TEST(cmdfs, grep_finds_a_match_and_reports_where)
{
    pditest::Shell shell;
    workspace(shell, "/w_grep");
    __i_fs.createFile("/w_grep/hay.txt", "nothing here\nneedle here\nnothing again\n");

    std::string out = shell.run("grep needle hay.txt");
    ASSERT_TRUE(saw(out, "needle"));
    ASSERT_TRUE(saw(out, "hay.txt"));

    scrub(shell, "/w_grep");
}

TEST(cmdfs, grep_reports_nothing_when_there_is_no_match)
{
    pditest::Shell shell;
    workspace(shell, "/w_grepno");
    __i_fs.createFile("/w_grepno/hay.txt", "nothing here\n");

    ASSERT_FALSE(saw(shell.run("grep needle hay.txt"), "needle here"));

    scrub(shell, "/w_grepno");
}

TEST(cmdfs, hexdump_shows_offsets_and_ascii)
{
    pditest::Shell shell;
    workspace(shell, "/w_hex");
    __i_fs.createFile("/w_hex/bin.txt", "AB");

    std::string out = shell.run("hexdump bin.txt");
    ASSERT_TRUE(saw(out, "41"));
    ASSERT_TRUE(saw(out, "42"));
    ASSERT_TRUE(saw(out, "AB"));

    scrub(shell, "/w_hex");
}

/* ------------------------------------------------------------- cp, mv, rm */

TEST(cmdfs, cp_leaves_both_copies)
{
    pditest::Shell shell;
    workspace(shell, "/w_cp");
    shell.run("echo copy me > src.txt");

    shell.run("cp src.txt dst.txt");
    ASSERT_TRUE(__i_fs.isFileExist("/w_cp/src.txt"));
    ASSERT_TRUE(__i_fs.isFileExist("/w_cp/dst.txt"));
    ASSERT_TRUE(saw(shell.run("cat dst.txt"), "copy me"));

    scrub(shell, "/w_cp");
}

TEST(cmdfs, mv_renames_within_a_directory)
{
    pditest::Shell shell;
    workspace(shell, "/w_mv");
    shell.run("echo move me > before.txt");

    shell.run("mv before.txt after.txt");
    ASSERT_FALSE(__i_fs.isFileExist("/w_mv/before.txt"));
    ASSERT_TRUE(__i_fs.isFileExist("/w_mv/after.txt"));
    ASSERT_TRUE(saw(shell.run("cat after.txt"), "move me"));

    scrub(shell, "/w_mv");
}

TEST(cmdfs, rm_removes_a_file)
{
    pditest::Shell shell;
    workspace(shell, "/w_rm");
    shell.run("touch doomed.txt");
    ASSERT_TRUE(__i_fs.isFileExist("/w_rm/doomed.txt"));

    shell.run("rm doomed.txt");
    ASSERT_FALSE(__i_fs.isFileExist("/w_rm/doomed.txt"));

    scrub(shell, "/w_rm");
}

TEST(cmdfs, rm_removes_an_empty_directory)
{
    pditest::Shell shell;
    workspace(shell, "/w_rmdir");
    shell.run("mkdir gone");
    ASSERT_TRUE(__i_fs.isDirectory("/w_rmdir/gone"));

    shell.run("rm gone");
    ASSERT_FALSE(__i_fs.isDirectory("/w_rmdir/gone"));

    scrub(shell, "/w_rmdir");
}

TEST(cmdfs, a_file_written_by_one_command_is_read_by_another)
{
    pditest::Shell shell;
    workspace(shell, "/w_chain");

    shell.run("echo chained > a.txt");
    shell.run("cp a.txt b.txt");
    shell.run("mv b.txt c.txt");

    ASSERT_TRUE(saw(shell.run("cat c.txt"), "chained"));
    ASSERT_TRUE(saw(shell.run("wc c.txt"), "1"));

    scrub(shell, "/w_chain");
}
