/****************************** Size Column Tests *****************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Covers how ls, df and du show sizes a large disk holds: a file past 4 GiB, a
terabyte disk, a disk with large blocks. Every value is shown whole, every
column is as wide as its widest value, and du counts the blocks each file takes.

Author          : Suraj I.
created Date    : 5th Nov 2026
******************************************************************************/

#include <ShellHarness.h>
#include <pditest.h>
#include <interface/pdi/impl/modules/storage/TmpFs.h>
#include <string>
#include <vector>

namespace
{

const uint64_t BIG_FILE = 5368709120ULL;
const uint64_t HUGE_FILE = 1099511627776ULL;
const uint64_t INNER_FILE = 70000ULL;
const uint64_t DISK_TOTAL = 2199023255552ULL;
const uint64_t DISK_USED = 1099511627776ULL;
const uint32_t DISK_BLOCK = 32768;

/**
 * A disk that reports what a large card holds: three files at the top, one of
 * them past 4 GiB and one a terabyte, a directory holding one more, and 32 KiB
 * blocks.
 */
class HugeFs : public TmpFs
{
public:
    uint64_t getTotalSize() override { return DISK_TOTAL; }
    uint64_t getUsedSize() override { return DISK_USED; }
    uint64_t getFreeSize() override { return DISK_TOTAL - DISK_USED; }
    uint32_t getBlockSize() override { return DISK_BLOCK; }

    bool isDirectory(const char *path) override { return isTop(path) || isSub(path); }
    bool isDirExist(const char *path) override { return isDirectory(path); }

    int getDirFileList(const char *path, pdiutil::vector<file_info_t> &items, const char *pattern = nullptr) override
    {
        items.clear();
        if (isTop(path))
        {
            add(items, "big.bin", FILE_TYPE_REG, BIG_FILE);
            add(items, "tiny", FILE_TYPE_REG, 1);
            add(items, "huge.img", FILE_TYPE_REG, HUGE_FILE);
            add(items, "sub", FILE_TYPE_DIR, 0);
            return (int)items.size();
        }
        if (isSub(path))
        {
            add(items, "inner.bin", FILE_TYPE_REG, INNER_FILE);
            return (int)items.size();
        }
        return TmpFs::getDirFileList(path, items, pattern);
    }

private:
    static bool isTop(const char *path) { return 0 == strcmp(path, "/") || 0 == strcmp(path, ""); }
    static bool isSub(const char *path) { return 0 == strcmp(path, "/sub") || 0 == strcmp(path, "/sub/"); }

    static void add(pdiutil::vector<file_info_t> &items, const char *name, file_type_t type, uint64_t size)
    {
        file_info_t info;
        memset(&info, 0, sizeof(info));
        info.m_type = type;
        info.m_size = size;
        info.m_perms = (FILE_TYPE_DIR == type) ? 0755 : 0644;
        uint32_t len = (uint32_t)strlen(name);
        info.m_name = pdiutil::safe_new_array<char>(len + 1);
        memcpy(info.m_name, name, len + 1);
        items.push_back(info);
    }
};

/**
 * The huge disk mounted at /mnt/huge for one test, taken off again after it.
 */
struct HugeMount
{
    HugeFs m_fs;

    HugeMount()
    {
        pditest::mountedVfs();
        __i_fs.mount("/mnt/huge", &m_fs, "huge", VFS_TYPE_TMPFS, true);
    }

    ~HugeMount()
    {
        __i_fs.umount("/mnt/huge");
    }
};

std::vector<std::string> linesOf(const std::string &out)
{
    std::vector<std::string> lines;
    std::string line;
    for (char c : out)
    {
        if ('\n' == c)
        {
            if (!line.empty() && '\r' == line.back()) line.pop_back();
            if (!line.empty()) lines.push_back(line);
            line.clear();
        }
        else
        {
            line += c;
        }
    }
    if (!line.empty()) lines.push_back(line);
    return lines;
}

/**
 * The line that ends with the name given, or empty.
 */
std::string lineEndingWith(const std::string &out, const char *name)
{
    std::string tail = std::string(" ") + name;
    for (const std::string &line : linesOf(out))
    {
        if (line.size() >= tail.size() && 0 == line.compare(line.size() - tail.size(), tail.size(), tail))
        {
            return line;
        }
    }
    return std::string();
}

/**
 * The line that starts with the text given, or empty.
 */
std::string lineStartingWith(const std::string &out, const char *start)
{
    for (const std::string &line : linesOf(out))
    {
        if (0 == line.compare(0, strlen(start), start))
        {
            return line;
        }
    }
    return std::string();
}

bool has(const std::string &line, const char *text)
{
    return std::string::npos != line.find(text);
}

} // namespace

TEST(size_columns, ls_shows_a_file_past_4_gib_and_a_terabyte_file_whole)
{
    HugeMount mount;
    pditest::Shell shell;
    std::string out = shell.run("ls -la /mnt/huge");

    ASSERT_TRUE(has(lineEndingWith(out, "big.bin"), " 5368709120 "));
    ASSERT_TRUE(has(lineEndingWith(out, "huge.img"), " 1099511627776 "));
    ASSERT_TRUE(has(lineEndingWith(out, "tiny"), " 1 "));
}

TEST(size_columns, ls_right_aligns_every_size_to_the_widest_and_starts_every_name_in_one_column)
{
    HugeMount mount;
    pditest::Shell shell;
    std::string out = shell.run("ls -la /mnt/huge");

    std::string big = lineEndingWith(out, "big.bin");
    std::string huge = lineEndingWith(out, "huge.img");
    std::string tiny = lineEndingWith(out, "tiny");
    ASSERT_FALSE(big.empty() || huge.empty() || tiny.empty());

    size_t hugeend = huge.find("1099511627776") + strlen("1099511627776");
    size_t bigend = big.find("5368709120") + strlen("5368709120");
    ASSERT_EQ(bigend, hugeend);
    ASSERT_EQ(tiny.substr(hugeend - 2, 2), std::string(" 1"));
    ASSERT_EQ(big.size() - strlen("big.bin"), huge.size() - strlen("huge.img"));
    ASSERT_EQ(tiny.size() - strlen("tiny"), huge.size() - strlen("huge.img"));
}

TEST(size_columns, ls_alone_shows_names_and_the_flags_choose_rows_hidden_entries_and_short_sizes)
{
    HugeMount mount;
    pditest::Shell shell;

    std::string plain = shell.run("ls /mnt/huge");
    ASSERT_TRUE(has(plain, "big.bin"));
    ASSERT_TRUE(has(plain, "huge.img"));
    ASSERT_FALSE(has(plain, "rw-"));
    ASSERT_FALSE(has(plain, "5368709120"));
    ASSERT_FALSE(has(plain, ".."));

    std::string longrows = shell.run("ls -l /mnt/huge");
    ASSERT_TRUE(has(lineEndingWith(longrows, "big.bin"), " 5368709120 "));
    ASSERT_FALSE(has(longrows, ".."));
    ASSERT_EQ(shell.run("ls -la /mnt/huge"), shell.run("ls -l -a /mnt/huge"));

    std::string human = shell.run("ls -lah /mnt/huge");
    ASSERT_TRUE(has(lineEndingWith(human, "big.bin"), " 5G "));
    ASSERT_TRUE(has(lineEndingWith(human, "huge.img"), " 1T "));
    ASSERT_TRUE(has(lineEndingWith(human, "tiny"), " 1 "));
    ASSERT_FALSE(has(human, "5368709120"));
    ASSERT_EQ(shell.run("ls -h /mnt/huge"), plain);
}

TEST(size_columns, ls_refuses_an_option_it_does_not_take_and_a_second_path)
{
    pditest::Shell shell;
    std::string bad = shell.run("ls -x /etc");
    ASSERT_TRUE(has(bad, "invalid option : -x"));
    ASSERT_FALSE(has(bad, "hostname"));

    std::string two = shell.run("ls -la /etc /tmp");
    ASSERT_TRUE(has(two, "extra operand : /tmp"));
}

TEST(size_columns, df_shows_a_terabyte_disk_whole_with_its_numbers_right_aligned)
{
    HugeMount mount;
    pditest::Shell shell;
    std::string out = shell.run("df");

    std::string row = lineStartingWith(out, "/mnt/huge");
    ASSERT_TRUE(has(row, " 2199023255552 "));
    ASSERT_TRUE(has(row, " 1099511627776 "));
    ASSERT_TRUE(has(row, "1099511627776"));

    std::string header = lineStartingWith(out, "MOUNT");
    std::string root = lineStartingWith(out, "/ ");
    ASSERT_FALSE(header.empty() || root.empty());
    ASSERT_EQ(header.size(), row.size());
    ASSERT_EQ(root.size(), row.size());
    ASSERT_EQ(header.find("NAME"), row.find(" huge ") + 1);
}

TEST(size_columns, df_h_shows_a_terabyte_disk_in_human_sizes)
{
    HugeMount mount;
    pditest::Shell shell;
    std::string row = lineStartingWith(shell.run("df -h"), "/mnt/huge");

    ASSERT_TRUE(has(row, " 2T "));
    ASSERT_TRUE(has(row, " 1T "));
    ASSERT_FALSE(has(row, "2199023255552"));
}

TEST(size_columns, df_refuses_an_option_it_does_not_take_and_an_operand)
{
    pditest::Shell shell;
    ASSERT_TRUE(has(shell.run("df -x"), "invalid option : -x"));
    ASSERT_TRUE(has(shell.run("df /etc"), "extra operand : /etc"));
}

TEST(size_columns, du_counts_the_blocks_each_file_takes_on_a_disk_with_large_blocks)
{
    HugeMount mount;
    pditest::Shell shell;
    std::string out = shell.run("du /mnt/huge");

    ASSERT_TRUE(has(lineEndingWith(out, "/mnt/huge/sub"), "96 "));
    ASSERT_TRUE(has(lineEndingWith(out, "/mnt/huge"), "1078984832 "));
}

TEST(size_columns, du_b_counts_the_bytes_the_files_hold)
{
    HugeMount mount;
    pditest::Shell shell;
    std::string out = shell.run("du -b /mnt/huge");

    ASSERT_TRUE(has(lineEndingWith(out, "/mnt/huge/sub"), "70000 "));
    ASSERT_TRUE(has(lineEndingWith(out, "/mnt/huge"), "1104880406897 "));
}

TEST(size_columns, du_right_aligns_every_total_to_the_widest)
{
    HugeMount mount;
    pditest::Shell shell;
    std::string out = shell.run("du /mnt/huge");

    std::string sub = lineEndingWith(out, "/mnt/huge/sub");
    std::string top = lineEndingWith(out, "/mnt/huge");
    ASSERT_FALSE(sub.empty() || top.empty());
    ASSERT_EQ(sub.size() - strlen("/mnt/huge/sub"), top.size() - strlen("/mnt/huge"));
}

TEST(size_columns, du_h_and_du_sh_show_human_sizes_and_the_total_alone)
{
    HugeMount mount;
    pditest::Shell shell;
    ASSERT_TRUE(has(lineEndingWith(shell.run("du -h /mnt/huge"), "/mnt/huge/sub"), "96K "));

    std::string summary = shell.run("du -sh /mnt/huge");
    ASSERT_TRUE(has(lineEndingWith(summary, "/mnt/huge"), "1T "));
    ASSERT_TRUE(lineEndingWith(summary, "/mnt/huge/sub").empty());
}

TEST(size_columns, du_never_shows_a_directory_smaller_than_the_rows_below_it)
{
    pditest::Shell shell;
    shell.run("mkdir /wt_du");
    shell.run("mkdir /wt_du/a");
    shell.run("mkdir /wt_du/b");
    shell.run("echo 1 > /wt_du/a/x");
    shell.run("echo 22 > /wt_du/b/y");
    shell.run("echo 333 > /wt_du/z");
    std::string out = shell.run("du /wt_du");

    uint64_t a = strtoull(lineEndingWith(out, "/wt_du/a").c_str(), nullptr, 10);
    uint64_t b = strtoull(lineEndingWith(out, "/wt_du/b").c_str(), nullptr, 10);
    uint64_t top = strtoull(lineEndingWith(out, "/wt_du").c_str(), nullptr, 10);

    shell.run("rm /wt_du/a/x");
    shell.run("rm /wt_du/b/y");
    shell.run("rm /wt_du/z");
    shell.run("rm /wt_du/a");
    shell.run("rm /wt_du/b");
    shell.run("rm /wt_du");

    ASSERT_TRUE(a > 0 && b > 0);
    ASSERT_TRUE(top >= a + b);
}

TEST(size_columns, du_refuses_an_option_it_does_not_take)
{
    pditest::Shell shell;
    ASSERT_TRUE(has(shell.run("du -x /etc"), "invalid option : -x"));
}
