/******************************** Cron Tests **********************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Covers the two halves cron is made of: reading a table row into its five field
specs and a command, and deciding whether a field spec covers a value. The
grammar is where a scheduler like this gets things wrong quietly — a spec that
matches nothing looks the same as a job that simply has not come round yet.

Author          : Suraj I.
created Date    : 7th Sep 2026
******************************************************************************/

#include <MountedStack.h>
#include <pditest.h>

#ifdef ENABLE_CRON_SERVICE

#include <service_provider/cron/CronServiceProvider.h>
#include <service_provider/session/SessionManager.h>

namespace
{

bool minuteCovers(const char *spec, uint8_t value)
{
    pdiutil::string s = spec;
    return CronServiceProvider::fieldMatches(s, value, 0, 59);
}

// a suite run on its own starts from a bare filesystem, so the directory the
// table lives in is made here rather than left to whichever suite ran first
void writeCrontab(const char *rows, uint32_t len)
{
    pdiutil::string path = CRONTAB_FILE_PATH;
    pdiutil::string dir = path.substr(0, path.rfind('/'));

    if (!dir.empty() && !__i_fs.isDirectory(dir.c_str()))
    {
        __i_fs.createDirectory(dir.c_str());
    }

    __i_fs.writeFile(CRONTAB_FILE_PATH, rows, len, true);
}

} // namespace

TEST(cron, a_star_covers_every_value_in_the_field)
{
    ASSERT_TRUE(minuteCovers("*", 0));
    ASSERT_TRUE(minuteCovers("*", 30));
    ASSERT_TRUE(minuteCovers("*", 59));
}

TEST(cron, a_bare_number_covers_only_itself)
{
    ASSERT_TRUE(minuteCovers("30", 30));
    ASSERT_FALSE(minuteCovers("30", 29));
    ASSERT_FALSE(minuteCovers("30", 31));
}

TEST(cron, a_step_covers_every_nth_value_from_the_start_of_the_field)
{
    ASSERT_TRUE(minuteCovers("*/5", 0));
    ASSERT_TRUE(minuteCovers("*/5", 5));
    ASSERT_TRUE(minuteCovers("*/5", 55));
    ASSERT_FALSE(minuteCovers("*/5", 1));
    ASSERT_FALSE(minuteCovers("*/5", 54));
}

TEST(cron, a_range_covers_its_ends_and_nothing_outside)
{
    ASSERT_TRUE(minuteCovers("10-20", 10));
    ASSERT_TRUE(minuteCovers("10-20", 15));
    ASSERT_TRUE(minuteCovers("10-20", 20));
    ASSERT_FALSE(minuteCovers("10-20", 9));
    ASSERT_FALSE(minuteCovers("10-20", 21));
}

TEST(cron, a_step_over_a_range_counts_from_the_range_start)
{
    ASSERT_TRUE(minuteCovers("10-20/5", 10));
    ASSERT_TRUE(minuteCovers("10-20/5", 15));
    ASSERT_TRUE(minuteCovers("10-20/5", 20));
    ASSERT_FALSE(minuteCovers("10-20/5", 11));
    ASSERT_FALSE(minuteCovers("10-20/5", 25));
}

TEST(cron, a_list_covers_each_of_its_items)
{
    ASSERT_TRUE(minuteCovers("0,15,30,45", 0));
    ASSERT_TRUE(minuteCovers("0,15,30,45", 30));
    ASSERT_TRUE(minuteCovers("0,15,30,45", 45));
    ASSERT_FALSE(minuteCovers("0,15,30,45", 1));
    ASSERT_FALSE(minuteCovers("0,15,30,45", 44));
}

TEST(cron, a_list_may_mix_ranges_and_steps)
{
    ASSERT_TRUE(minuteCovers("1,10-12,*/30", 1));
    ASSERT_TRUE(minuteCovers("1,10-12,*/30", 11));
    ASSERT_TRUE(minuteCovers("1,10-12,*/30", 30));
    ASSERT_FALSE(minuteCovers("1,10-12,*/30", 13));
}

TEST(cron, a_value_outside_the_field_matches_nothing)
{
    ASSERT_FALSE(minuteCovers("*", 60));

    pdiutil::string any = "*";
    ASSERT_FALSE(CronServiceProvider::fieldMatches(any, 24, 0, 23));
    ASSERT_FALSE(CronServiceProvider::fieldMatches(any, 0, 1, 31));
}

TEST(cron, a_malformed_spec_matches_nothing_rather_than_everything)
{
    ASSERT_FALSE(minuteCovers("", 0));
    ASSERT_FALSE(minuteCovers("abc", 0));
    ASSERT_FALSE(minuteCovers("*/0", 0));
    ASSERT_FALSE(minuteCovers("20-10", 15));
    ASSERT_FALSE(minuteCovers("70", 70));
    ASSERT_FALSE(minuteCovers("-", 0));
}

TEST(cron, a_row_splits_into_five_fields_and_the_rest_is_the_command)
{
    pdiutil::string fields[CRON_FIELD_COUNT];
    pdiutil::string command;
    pdiutil::string row = "*/5 * * * * echo hello world";

    ASSERT_TRUE(CronServiceProvider::parseRow(row, fields, command));
    ASSERT_STREQ(fields[CRON_FIELD_MINUTE].c_str(), "*/5");
    ASSERT_STREQ(fields[CRON_FIELD_DOW].c_str(), "*");
    ASSERT_STREQ(command.c_str(), "echo hello world");
}

TEST(cron, a_row_tolerates_runs_of_spaces_between_fields)
{
    pdiutil::string fields[CRON_FIELD_COUNT];
    pdiutil::string command;
    pdiutil::string row = "   0    3   *  *   0    source /etc/weekly.sh";

    ASSERT_TRUE(CronServiceProvider::parseRow(row, fields, command));
    ASSERT_STREQ(fields[CRON_FIELD_HOUR].c_str(), "3");
    ASSERT_STREQ(command.c_str(), "source /etc/weekly.sh");
}

TEST(cron, a_comment_or_a_blank_line_is_not_a_row)
{
    pdiutil::string fields[CRON_FIELD_COUNT];
    pdiutil::string command;

    pdiutil::string comment = "# min hour dom mon dow  command";
    ASSERT_FALSE(CronServiceProvider::parseRow(comment, fields, command));

    pdiutil::string indented = "   # still a comment";
    ASSERT_FALSE(CronServiceProvider::parseRow(indented, fields, command));

    pdiutil::string blank = "    ";
    ASSERT_FALSE(CronServiceProvider::parseRow(blank, fields, command));
}

TEST(cron, a_row_missing_its_command_is_refused)
{
    pdiutil::string fields[CRON_FIELD_COUNT];
    pdiutil::string command;

    pdiutil::string nocmd = "* * * * *";
    ASSERT_FALSE(CronServiceProvider::parseRow(nocmd, fields, command));

    pdiutil::string shortrow = "* * * echo hi";
    ASSERT_FALSE(CronServiceProvider::parseRow(shortrow, fields, command));
}

TEST(cron, the_table_is_walked_and_only_the_matching_rows_are_taken)
{
    pditest::mountedVfs();

    const char *rows =
        "# a comment that is not a job\n"
        "*/5 * * * * echo every_five\n"
        "\n"
        "30 4 * * * echo half_past_four\n"
        "* * * * * echo always\n";

    writeCrontab(rows, (uint32_t)strlen(rows));

    datetime_t moment;
    moment.m_valid = true;
    moment.m_minute = 30;
    moment.m_hour = 4;
    moment.m_day = 15;
    moment.m_month = 6;
    moment.m_weekday = 3;

    // 30 matches */5, the pinned 4:30 row, and the open row
    ASSERT_EQ((uint32_t)__cron_service.runDueJobs(moment, true), 3u);

    moment.m_minute = 31;

    // 31 is not a multiple of five and not the pinned minute, so only the open row
    ASSERT_EQ((uint32_t)__cron_service.runDueJobs(moment, true), 1u);

    __i_fs.deleteFile(CRONTAB_FILE_PATH);
}

TEST(cron, no_more_than_the_cap_is_taken_from_the_table)
{
    pditest::mountedVfs();

    pdiutil::string rows;
    for (uint8_t i = 0; i < (uint8_t)(CRON_MAX_ENTRIES + 4); i++) {
        rows += "* * * * * echo row\n";
    }

    writeCrontab(rows.c_str(), (uint32_t)rows.size());

    datetime_t moment;
    moment.m_valid = true;
    moment.m_minute = 0;
    moment.m_hour = 0;
    moment.m_day = 1;
    moment.m_month = 1;
    moment.m_weekday = 0;

    ASSERT_EQ((uint32_t)__cron_service.runDueJobs(moment, true), (uint32_t)CRON_MAX_ENTRIES);

    __i_fs.deleteFile(CRONTAB_FILE_PATH);
}

TEST(cron, the_service_holds_no_jobs_while_the_clock_is_not_valid)
{
    pditest::mountedVfs();

    const char *rows = "* * * * * echo always\n";
    writeCrontab(rows, (uint32_t)strlen(rows));

    datetime_t moment;
    moment.m_valid = false;
    moment.m_minute = 0;
    moment.m_hour = 0;
    moment.m_day = 1;
    moment.m_month = 1;
    moment.m_weekday = 0;

    ASSERT_EQ((uint32_t)__cron_service.runDueJobs(moment, true), 0u);

    // the same table with a trustworthy clock does have a job to take
    moment.m_valid = true;
    ASSERT_EQ((uint32_t)__cron_service.runDueJobs(moment, true), 1u);

    __i_fs.deleteFile(CRONTAB_FILE_PATH);
}

#endif

TEST(cron, starting_the_service_leaves_a_table_to_edit)
{
    pditest::mountedVfs();

    __i_fs.deleteFile(CRONTAB_FILE_PATH);
    ASSERT_FALSE(__i_fs.isFileExist(CRONTAB_FILE_PATH));

    ASSERT_TRUE(__cron_service.startService());
    ASSERT_TRUE(__i_fs.isFileExist(CRONTAB_FILE_PATH));

    pdiutil::string first;
    ASSERT_TRUE(__i_fs.readLineInFile(CRONTAB_FILE_PATH, 0, first) >= 0);
    ASSERT_TRUE(first.size() > 0);
    ASSERT_EQ((int)first[0], (int)CRON_COMMENT_CHAR);

    // the seeded table names its columns and holds no job
    datetime_t moment;
    moment.m_valid = true;
    moment.m_minute = 0;
    moment.m_hour = 0;
    moment.m_day = 1;
    moment.m_month = 1;
    moment.m_weekday = 0;

    ASSERT_EQ((uint32_t)__cron_service.runDueJobs(moment, true), 0u);

    __cron_service.stopService();
    __i_fs.deleteFile(CRONTAB_FILE_PATH);
}

TEST(cron, a_restart_does_not_leave_a_second_listener_behind)
{
    pditest::mountedVfs();

    const char *rows = "* * * * * echo always\n";

    ASSERT_TRUE(__cron_service.startService());
    writeCrontab(rows, (uint32_t)strlen(rows));

    __cron_service.stopService();
    ASSERT_EQ((int)__cron_service.getServiceState(), (int)SERVICE_STATE_INACTIVE);

    ASSERT_TRUE(__cron_service.startService());
    __cron_service.stopService();
    ASSERT_TRUE(__cron_service.startService());

    // one job in the table is one job due, however many times the service has
    // been started; a listener left behind on each start would multiply it
    datetime_t moment;
    moment.m_valid = true;
    moment.m_minute = 0;
    moment.m_hour = 0;
    moment.m_day = 1;
    moment.m_month = 1;
    moment.m_weekday = 0;

    ASSERT_EQ((uint32_t)__cron_service.runDueJobs(moment, true), 1u);

    __cron_service.stopService();
    __i_fs.deleteFile(CRONTAB_FILE_PATH);
}

TEST(cron, a_job_fires_even_while_the_console_holds_a_session)
{
    VfsDispatcher *fs = pditest::mountedVfs();

    const char *marker = "/wt_cron_fired";
    fs->beginPrivileged();
    if (fs->isFileExist(marker)) fs->deleteFile(marker);
    fs->endPrivileged();

    // somebody is logged in on the console, which is the ordinary state of a
    // board; a scheduled job must neither borrow that session nor wait for it
    session_t *console = SessionManager::attach(__i_dvc_ctrl.getTerminal());
    ASSERT_TRUE(nullptr != console);

    const char *rows = "* * * * * echo fired > /wt_cron_fired\n";
    writeCrontab(rows, (uint32_t)strlen(rows));

    datetime_t moment;
    moment.m_valid = true;
    moment.m_minute = 0;
    moment.m_hour = 0;
    moment.m_day = 1;
    moment.m_month = 1;
    moment.m_weekday = 0;

    ASSERT_EQ((uint32_t)__cron_service.runDueJobs(moment), 1u);

    bool landed = __i_fs.isFileExist(marker);

    SessionManager::detach(__i_dvc_ctrl.getTerminal());
    SessionManager::setCurrent(nullptr);

    fs->beginPrivileged();
    if (fs->isFileExist(marker)) fs->deleteFile(marker);
    fs->endPrivileged();
    __i_fs.deleteFile(CRONTAB_FILE_PATH);

    ASSERT_TRUE(landed);
}
