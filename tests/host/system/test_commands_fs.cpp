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
    ASSERT_EQ(shell.result(), CMD_ERROR_INVAL);

    shell.run("mkdir");
    ASSERT_EQ(shell.result(), CMD_ERROR_INVAL);
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

TEST(cmdfs, echo_appends_with_a_doubled_redirection)
{
    pditest::Shell shell;
    workspace(shell, "/w_append");

    shell.run("echo first line > note.txt");
    shell.run("echo second line >> note.txt");

    std::string out = shell.run("cat note.txt");
    ASSERT_TRUE(saw(out, "first line"));
    ASSERT_TRUE(saw(out, "second line"));

    scrub(shell, "/w_append");
}

TEST(cmdfs, a_single_redirection_still_replaces_the_file)
{
    pditest::Shell shell;
    workspace(shell, "/w_replace");

    shell.run("echo first line > note.txt");
    shell.run("echo second line > note.txt");

    std::string out = shell.run("cat note.txt");
    ASSERT_FALSE(saw(out, "first line"));
    ASSERT_TRUE(saw(out, "second line"));

    scrub(shell, "/w_replace");
}

TEST(cmdfs, appending_to_a_file_that_is_not_there_creates_it)
{
    pditest::Shell shell;
    workspace(shell, "/w_appendnew");

    shell.run("echo only line >> fresh.txt");

    ASSERT_TRUE(__i_fs.isFileExist("/w_appendnew/fresh.txt"));
    ASSERT_TRUE(saw(shell.run("cat fresh.txt"), "only line"));

    scrub(shell, "/w_appendnew");
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
    ASSERT_NE(shell.result(), CMD_ERROR_UNSET);

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

TEST(cmdfs, tail_of_one_line_prints_only_the_last_line)
{
    pditest::Shell shell;
    workspace(shell, "/w_tail1");
    __i_fs.createFile("/w_tail1/lines.txt", "alpha\nbravo\ncharlie\ndelta\n");

    std::string out = shell.run("tail lines.txt 1");
    ASSERT_TRUE(saw(out, "delta"));
    ASSERT_FALSE(saw(out, "charlie"));

    out = shell.run("tail lines.txt 2");
    ASSERT_TRUE(saw(out, "charlie"));
    ASSERT_FALSE(saw(out, "bravo"));

    scrub(shell, "/w_tail1");
}

TEST(cmdfs, tail_and_source_read_lines_from_a_file_in_tmp)
{
    pditest::Shell shell;
    __i_fs.writeFile("/tmp/lines.txt", "alpha\nbravo\ncharlie\n", 20, false);
    __i_fs.writeFile("/tmp/run.sh", "echo ran from tmp\necho second tmp line\n", 40, false);

    std::string last = shell.run("tail /tmp/lines.txt 1");
    std::string ran = shell.run("source /tmp/run.sh");

    shell.run("rm /tmp/lines.txt");
    shell.run("rm /tmp/run.sh");
    ASSERT_TRUE(saw(last, "charlie"));
    ASSERT_FALSE(saw(last, "bravo"));
    ASSERT_TRUE(saw(ran, "ran from tmp"));
    ASSERT_TRUE(saw(ran, "second tmp line"));
}

TEST(cmdfs, ls_of_a_file_prints_that_file)
{
    pditest::Shell shell;
    workspace(shell, "/w_lsfile");
    __i_fs.createFile("/w_lsfile/one.txt", "1234567");

    std::string out = shell.run("ls /w_lsfile/one.txt");
    std::string rel = shell.run("ls one.txt");

    scrub(shell, "/w_lsfile");
    ASSERT_TRUE(saw(out, "/w_lsfile/one.txt"));
    ASSERT_TRUE(saw(out, "-rw-"));
    ASSERT_TRUE(saw(out, "7"));
    ASSERT_FALSE(saw(out, "CmdErr"));
    ASSERT_TRUE(saw(rel, "one.txt"));
    ASSERT_FALSE(saw(rel, "CmdErr"));
}

TEST(cmdfs, ls_of_a_missing_path_names_it)
{
    pditest::Shell shell;
    std::string out = shell.run("ls /w_no_such_thing");
    ASSERT_TRUE(saw(out, "/w_no_such_thing"));
    ASSERT_TRUE(saw(out, "CmdErr"));
}

TEST(cmdfs, ls_lists_the_dot_entries_in_tmp_as_on_the_flash)
{
    pditest::Shell shell;
    __i_fs.writeFile("/tmp/dotted.txt", "d", 1, false);

    std::string out = shell.run("ls /tmp");
    shell.run("rm /tmp/dotted.txt");

    ASSERT_TRUE(saw(out, " ..\r") || saw(out, " ..\n"));
    ASSERT_TRUE(saw(out, " .\r") || saw(out, " .\n"));
    ASSERT_TRUE(saw(out, "dotted.txt"));
}

TEST(cmdfs, mkdir_of_something_already_there_is_refused)
{
    pditest::Shell shell;
    workspace(shell, "/w_mkdirx");
    __i_fs.createDirectory("/w_mkdirx/there");
    __i_fs.createFile("/w_mkdirx/file.txt", "f");

    std::string dir = shell.run("mkdir /w_mkdirx/there");
    std::string file = shell.run("mkdir /w_mkdirx/file.txt");
    std::string fresh = shell.run("mkdir /w_mkdirx/fresh");

    bool made = __i_fs.isDirectory("/w_mkdirx/fresh");
    bool stillfile = __i_fs.isFileExist("/w_mkdirx/file.txt") && !__i_fs.isDirectory("/w_mkdirx/file.txt");
    scrub(shell, "/w_mkdirx");
    ASSERT_TRUE(saw(dir, "Failed to create directory"));
    ASSERT_TRUE(saw(dir, "CmdErr"));
    ASSERT_TRUE(saw(file, "Failed to create directory"));
    ASSERT_FALSE(saw(fresh, "CmdErr"));
    ASSERT_TRUE(made);
    ASSERT_TRUE(stillfile);
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

/* --------------------------------------------------------------------- dd */

/**
 * The whole of a file, read back.
 */
static std::string fileBytes(const char *path)
{
    std::string got;
    __i_fs.readFile(path, 64, [&](char *data, uint32_t size) -> bool {
        got.append(data, size);
        return true;
    });
    return got;
}

/**
 * A file of the size given, each byte following from its place.
 */
static std::string patterned(const char *path, uint32_t size)
{
    std::string bytes;
    for (uint32_t i = 0; i < size; i++)
    {
        bytes += (char)('a' + (i * 7 + i / 256) % 26);
    }
    __i_fs.createFile(path, bytes.c_str(), (int64_t)bytes.size());
    return bytes;
}

TEST(cmdfs, dd_copies_a_whole_file_and_reports_the_records)
{
    pditest::Shell shell;
    workspace(shell, "/w_dd1");
    std::string src = patterned("/w_dd1/src.bin", 1000);

    std::string out = shell.run("dd if=src.bin of=dst.bin");

    ASSERT_TRUE(saw(out, "1+1 records in"));
    ASSERT_TRUE(saw(out, "1+1 records out"));
    ASSERT_TRUE(saw(out, "1000 bytes copied"));
    ASSERT_TRUE(fileBytes("/w_dd1/dst.bin") == src);

    scrub(shell, "/w_dd1");
}

TEST(cmdfs, dd_writes_a_new_file_into_tmp)
{
    pditest::Shell shell;
    workspace(shell, "/w_ddtmp");
    std::string src = patterned("/w_ddtmp/src.bin", 512);
    shell.run("rm /tmp/dd_new.bin");

    std::string out = shell.run("dd if=src.bin of=/tmp/dd_new.bin bs=512 count=1");

    ASSERT_TRUE(saw(out, "1+0 records out"));
    ASSERT_TRUE(saw(out, "512 bytes copied"));
    ASSERT_TRUE(fileBytes("/tmp/dd_new.bin") == src);

    shell.run("rm /tmp/dd_new.bin");
    scrub(shell, "/w_ddtmp");
}

TEST(cmdfs, dd_replaces_what_a_tmp_file_held_when_no_seek_is_given)
{
    pditest::Shell shell;
    workspace(shell, "/w_ddtrunc");
    std::string src = patterned("/w_ddtrunc/src.bin", 100);
    __i_fs.writeFile("/tmp/dd_old.bin", "0123456789012345678901234567890123456789012345678901234567890123456789"
                                       "0123456789012345678901234567890123456789", 110, false);

    shell.run("dd if=src.bin of=/tmp/dd_old.bin");

    ASSERT_TRUE(fileBytes("/tmp/dd_old.bin") == src);

    shell.run("rm /tmp/dd_old.bin");
    scrub(shell, "/w_ddtrunc");
}

TEST(cmdfs, dd_copies_count_blocks_of_bs_from_skip)
{
    pditest::Shell shell;
    workspace(shell, "/w_dd2");
    std::string src = patterned("/w_dd2/src.bin", 2048);

    std::string out = shell.run("dd if=src.bin of=part.bin bs=256 count=2 skip=1");

    ASSERT_TRUE(saw(out, "2+0 records in"));
    ASSERT_TRUE(saw(out, "512 bytes copied"));
    ASSERT_TRUE(fileBytes("/w_dd2/part.bin") == src.substr(256, 512));

    scrub(shell, "/w_dd2");
}

TEST(cmdfs, dd_writes_from_seek_and_keeps_the_rest_of_the_file)
{
    pditest::Shell shell;
    workspace(shell, "/w_dd3");
    __i_fs.createFile("/w_dd3/out.bin", std::string(1024, 'a').c_str(), 1024);
    __i_fs.createFile("/w_dd3/small.bin", std::string(16, 'b').c_str(), 16);

    shell.run("dd if=small.bin of=out.bin bs=16 seek=2");

    std::string got = fileBytes("/w_dd3/out.bin");
    ASSERT_EQ((int)got.size(), 1024);
    ASSERT_TRUE(got.substr(0, 32) == std::string(32, 'a'));
    ASSERT_TRUE(got.substr(32, 16) == std::string(16, 'b'));
    ASSERT_TRUE(got.substr(48) == std::string(1024 - 48, 'a'));

    scrub(shell, "/w_dd3");
}

TEST(cmdfs, dd_replaces_the_output_when_no_seek_is_given)
{
    pditest::Shell shell;
    workspace(shell, "/w_dd4");
    __i_fs.createFile("/w_dd4/out.bin", std::string(1024, 'a').c_str(), 1024);
    __i_fs.createFile("/w_dd4/small.bin", std::string(16, 'b').c_str(), 16);

    shell.run("dd if=small.bin of=out.bin");

    ASSERT_TRUE(fileBytes("/w_dd4/out.bin") == std::string(16, 'b'));

    scrub(shell, "/w_dd4");
}

TEST(cmdfs, dd_needs_an_input_and_an_output)
{
    pditest::Shell shell;
    workspace(shell, "/w_dd5");
    patterned("/w_dd5/src.bin", 10);

    ASSERT_TRUE(saw(shell.run("dd if=src.bin"), "CmdErr"));
    ASSERT_TRUE(saw(shell.run("dd of=dst.bin"), "CmdErr"));
    ASSERT_FALSE(__i_fs.isFileExist("/w_dd5/dst.bin"));

    scrub(shell, "/w_dd5");
}

TEST(cmdfs, dd_refuses_an_operand_it_does_not_know_or_a_size_it_cannot_take)
{
    pditest::Shell shell;
    workspace(shell, "/w_dd6");
    patterned("/w_dd6/src.bin", 10);

    ASSERT_TRUE(saw(shell.run("dd if=src.bin of=a.bin bs=0"), "CmdErr"));
    ASSERT_TRUE(saw(shell.run("dd if=src.bin of=a.bin bs=x"), "CmdErr"));
    ASSERT_TRUE(saw(shell.run("dd if=src.bin of=a.bin bs=8192"), "CmdErr"));
    ASSERT_TRUE(saw(shell.run("dd if=src.bin of=a.bin foo=1"), "CmdErr"));
    ASSERT_FALSE(__i_fs.isFileExist("/w_dd6/a.bin"));

    scrub(shell, "/w_dd6");
}

TEST(cmdfs, dd_names_the_input_it_could_not_read_and_leaves_no_output)
{
    pditest::Shell shell;
    workspace(shell, "/w_dd7");

    std::string out = shell.run("dd if=missing.bin of=x.bin");

    ASSERT_TRUE(saw(out, "dd: "));
    ASSERT_TRUE(saw(out, "missing.bin"));
    ASSERT_FALSE(__i_fs.isFileExist("/w_dd7/x.bin"));

    scrub(shell, "/w_dd7");
}

TEST(cmdfs, dd_reads_a_device_node)
{
    pditest::Shell shell;
    workspace(shell, "/w_dd8");

    std::string out = shell.run("dd if=/dev/zero of=z.bin bs=100 count=3");

    ASSERT_TRUE(saw(out, "3+0 records in"));
    ASSERT_TRUE(fileBytes("/w_dd8/z.bin") == std::string(300, '\0'));

    scrub(shell, "/w_dd8");
}

TEST(cmdfs, dd_wants_a_count_for_a_node_that_never_ends)
{
    pditest::Shell shell;
    workspace(shell, "/w_dd9");

    std::string out = shell.run("dd if=/dev/zero of=z.bin");

    ASSERT_TRUE(saw(out, "never ends"));
    ASSERT_FALSE(__i_fs.isFileExist("/w_dd9/z.bin"));

    scrub(shell, "/w_dd9");
}

TEST(cmdfs, dd_into_dev_null_takes_every_record_and_drops_it)
{
    pditest::Shell shell;
    workspace(shell, "/w_dd10");

    std::string out = shell.run("dd if=/dev/zero of=/dev/null bs=100 count=3");

    ASSERT_TRUE(saw(out, "3+0 records out"));
    ASSERT_TRUE(saw(out, "300 bytes copied"));
    scrub(shell, "/w_dd10");
}

TEST(cmdfs, an_edit_of_a_discarding_dev_node_is_taken_and_of_anything_else_refused)
{
    const char *const discarding[] = {"/dev/null", "/dev/zero", "/dev/random", "/dev/urandom"};
    for (const char *node : discarding)
    {
        ASSERT_EQ(__i_fs.editFile(node, 512, "abc", 3), 3);
    }
    ASSERT_EQ(__i_fs.editFile("/dev", 0, "abc", 3), (int)STORAGE_ERROR_NOT_A_FILE);
    ASSERT_EQ(__i_fs.editFile("/dev/nosuch", 0, "abc", 3), (int)PDI_ERR_NOT_FOUND);
}

/* ------------------------------------------------------------------- cat */

TEST(cmdfs, cat_shows_a_bare_newline_as_a_line_end_on_the_terminal)
{
    pditest::Shell shell;
    workspace(shell, "/w_cat1");
    __i_fs.writeFile("/w_cat1/lf.txt", "one\ntwo\n", 8);

    std::string out = shell.run("cat lf.txt");

    ASSERT_TRUE(saw(out, "one\r\ntwo\r\n"));
    ASSERT_FALSE(saw(out, "one\ntwo"));
    scrub(shell, "/w_cat1");
}

TEST(cmdfs, cat_leaves_a_carriage_return_line_end_as_it_is_even_split_across_reads)
{
    pditest::Shell shell;
    workspace(shell, "/w_cat2");
    __i_fs.writeFile("/w_cat2/crlf.txt", "123456789\r\nabc\r\n", 16);

    std::string out = shell.run("cat crlf.txt");

    ASSERT_TRUE(saw(out, "123456789\r\nabc\r\n"));
    ASSERT_FALSE(saw(out, "\r\r\n"));
    scrub(shell, "/w_cat2");
}

TEST(cmdfs, cat_into_a_file_or_a_pipe_passes_its_bytes_unchanged)
{
    pditest::Shell shell;
    workspace(shell, "/w_cat3");
    __i_fs.writeFile("/w_cat3/lf.txt", "one\ntwo\n", 8);

    shell.run("cat lf.txt > copy.txt");
    ASSERT_TRUE(fileBytes("/w_cat3/copy.txt") == std::string("one\ntwo\n"));
    ASSERT_TRUE(saw(shell.run("cat lf.txt | wc"), "2 2 8"));
    scrub(shell, "/w_cat3");
}

/* ------------------------------------------------------------- interrupt */

/* the code a command stopped by ctrl+c ends with, as the shell prints it */
static std::string canceled()
{
    return std::string("CmdErr : ") + std::to_string((int)CMD_ERROR_CANCELED);
}

/**
 * A file of numbered lines, "line 0000" to "line 2999", 30000 bytes in all.
 */
static void bigFile(const char *path)
{
    std::string body;
    char line[16];
    for (int i = 0; i < 3000; i++)
    {
        snprintf(line, sizeof(line), "line %04d\n", i);
        body += line;
    }
    __i_fs.writeFile(path, body.data(), (uint32_t)body.size());
}

TEST(cmdfs, ctrl_c_stops_a_command_reading_a_long_file_part_way)
{
    struct Case
    {
        const char *keys;
        const char *last;
    };
    const Case cases[] = {{"cat big.txt\r\x03", "line 2999"}, {"grep line big.txt\r\x03", "line 2999"},
                          {"head big.txt 3000\r\x03", "line 2999"}, {"tail big.txt 3000\r\x03", "line 2999"},
                          {"hexdump big.txt\r\x03", "00007520"}};
    for (const Case &c : cases)
    {
        pditest::Shell shell;
        workspace(shell, "/w_int1");
        bigFile("/w_int1/big.txt");

        std::string out = shell.type(c.keys);

        ASSERT_TRUE(saw(out, canceled().c_str()));
        ASSERT_FALSE(saw(out, c.last));
        scrub(shell, "/w_int1");
    }
}

TEST(cmdfs, ctrl_c_that_stops_a_read_leaves_the_shell_on_a_clean_prompt)
{
    pditest::Shell shell;
    workspace(shell, "/w_int8");
    bigFile("/w_int8/big.txt");

    std::string out = shell.type("cat big.txt\r\x03");

    std::string tail = out.substr(out.rfind(canceled()));
    size_t first = tail.find("w_int8): ");
    ASSERT_TRUE(std::string::npos != first);
    ASSERT_EQ(tail.find("w_int8): ", first + 1), std::string::npos);
    ASSERT_EQ(tail.substr(tail.size() - 3), std::string("): "));
    ASSERT_TRUE(SessionManager::current()->m_typeahead.empty());
    scrub(shell, "/w_int8");
}

TEST(cmdfs, ctrl_c_stops_wc_before_it_counts_the_whole_file)
{
    pditest::Shell shell;
    workspace(shell, "/w_int2");
    bigFile("/w_int2/big.txt");

    std::string out = shell.type("wc big.txt\r\x03");

    ASSERT_TRUE(saw(out, canceled().c_str()));
    ASSERT_FALSE(saw(out, "30000"));
    scrub(shell, "/w_int2");
}

TEST(cmdfs, ctrl_c_stops_cp_and_leaves_no_partial_copy)
{
    pditest::Shell shell;
    workspace(shell, "/w_int3");
    bigFile("/w_int3/big.txt");

    std::string out = shell.type("cp big.txt copy.txt\r\x03");

    ASSERT_TRUE(saw(out, canceled().c_str()));
    ASSERT_FALSE(__i_fs.isFileExist("/w_int3/copy.txt"));
    ASSERT_TRUE(fileBytes("/w_int3/big.txt").size() == 30000);
    scrub(shell, "/w_int3");
}

TEST(cmdfs, ctrl_c_stops_dd_part_way_with_its_records_so_far)
{
    pditest::Shell shell;
    workspace(shell, "/w_int4");
    bigFile("/w_int4/big.txt");

    std::string out = shell.type("dd if=big.txt of=out.bin bs=100\r\x03");

    ASSERT_TRUE(saw(out, canceled().c_str()));
    ASSERT_TRUE(saw(out, "records in"));
    ASSERT_TRUE(fileBytes("/w_int4/out.bin").size() < 30000);
    scrub(shell, "/w_int4");
}

TEST(cmdfs, keys_typed_while_a_command_runs_wait_for_the_shell)
{
    pditest::Shell shell;
    workspace(shell, "/w_int5");
    bigFile("/w_int5/big.txt");

    std::string out = shell.type("cat big.txt\recho typed ahead\r");

    ASSERT_TRUE(saw(out, "line 2999"));
    ASSERT_TRUE(saw(out, "typed ahead"));
    ASSERT_FALSE(saw(out, canceled().c_str()));
    scrub(shell, "/w_int5");
}

TEST(cmdfs, keys_typed_ahead_past_what_a_session_keeps_are_dropped)
{
    pditest::Shell shell;
    workspace(shell, "/w_int7");
    bigFile("/w_int7/big.txt");
    std::string word(CMD_TYPEAHEAD_MAX + 20, 'k');
    std::string keys = std::string("cat big.txt\recho ") + word + "\r";

    std::string out = shell.type(keys.c_str());

    ASSERT_TRUE(saw(out, "line 2999"));
    ASSERT_TRUE(saw(out, std::string(CMD_TYPEAHEAD_MAX - 5, 'k').c_str()));
    ASSERT_FALSE(saw(out, word.c_str()));
    shell.type("\x03");
    scrub(shell, "/w_int7");
}

TEST(cmdfs, ctrl_c_drops_the_keys_typed_ahead_of_it_and_the_next_line_runs_whole)
{
    pditest::Shell shell;
    workspace(shell, "/w_int6");
    bigFile("/w_int6/big.txt");

    std::string out = shell.type("cat big.txt\recho dropped\r\x03");
    ASSERT_TRUE(saw(out, canceled().c_str()));
    ASSERT_FALSE(saw(out, "dropped"));

    std::string again = shell.type("cat big.txt\r");
    ASSERT_TRUE(saw(again, "line 2999"));
    ASSERT_FALSE(saw(again, canceled().c_str()));
    scrub(shell, "/w_int6");
}
