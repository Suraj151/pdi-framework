/************************ Process Command Tests *******************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

The scheduler and service side of the command surface, plus the handful of
commands that do not belong to any other group.

Author          : Suraj I.
created Date    : 16th Aug 2026
******************************************************************************/

#include <ShellHarness.h>
#include <utility/TaskScheduler.h>
#include <pditest.h>

using pditest::saw;

/**
 * A task that does nothing, so ps and the signal commands have something real
 * to find. Returned id is released by the caller.
 */
static pdiutil::task_id_t parkTask(const char *name, uint8_t owner = 0)
{
    // a short interval keeps the test quick. a signalled task stops running
    // immediately but keeps its slot until the scheduler next walks past it,
    // which for a long interval task is its own next due point
    return __task_scheduler.setInterval([]() {}, 1, __i_dvc_ctrl.millis_now(),
                                        DEFAULT_TASK_PRIORITY, name, owner);
}

/**
 * A signalled or cleared task stops running at once but is unlinked later: the
 * scheduler walks one task per pass, so several passes are needed before the
 * slot is actually freed and the task leaves the ps listing.
 */
static void reap()
{
    // one pass consumes signals only up to the task it decides to run, so the
    // bound scales with how many tasks the rest of the suite has registered
    for (uint16_t pass = 0; pass < (MAX_SCHEDULABLE_TASKS * 2) + 16; pass++)
    {
        __task_scheduler.run();
    }
}

/**
 * @brief The ps row naming this task, or an empty string.
 */
static std::string psRow(pditest::Shell &shell, const char *name)
{
    std::string out = shell.run("ps");
    size_t at = out.find(name);
    if (at == std::string::npos)
    {
        return std::string();
    }

    size_t begin = out.rfind('\n', at);
    begin = (begin == std::string::npos) ? 0 : begin + 1;
    size_t end = out.find('\n', at);
    end = (end == std::string::npos) ? out.length() : end;

    return out.substr(begin, end - begin);
}

static void release(pdiutil::task_id_t id)
{
    if (id > 0)
    {
        __task_scheduler.clearInterval(id);
    }
}

/* ------------------------------------------------------------------- ps */

TEST(cmdproc, ps_prints_a_header)
{
    pditest::Shell shell;
    std::string out = shell.run("ps");

    ASSERT_TRUE(saw(out, "PID"));
}

TEST(cmdproc, ps_lists_a_registered_task_by_name)
{
    pditest::Shell shell;
    pdiutil::task_id_t id = parkTask("parked");

    ASSERT_TRUE(saw(shell.run("ps"), "parked"));

    release(id);
    reap();
}

TEST(cmdproc, a_cleared_task_leaves_the_listing)
{
    pditest::Shell shell;
    pdiutil::task_id_t id = parkTask("transient");
    ASSERT_TRUE(saw(shell.run("ps"), "transient"));

    release(id);
    reap();
    ASSERT_FALSE(saw(shell.run("ps"), "transient"));
}

TEST(cmdproc, ps_can_be_filtered_by_owner)
{
    pditest::Shell shell;
    pdiutil::task_id_t mine = parkTask("ownedbyone", 1);
    pdiutil::task_id_t theirs = parkTask("ownedbytwo", 2);

    std::string out = shell.run("ps 1");
    ASSERT_TRUE(saw(out, "ownedbyone"));
    ASSERT_FALSE(saw(out, "ownedbytwo"));

    release(mine);
    release(theirs);
    reap();
}

/* --------------------------------------------------------- kill and friends */

TEST(cmdproc, kill_stops_and_continues_a_task)
{
    pditest::Shell shell;
    pdiutil::task_id_t id = parkTask("signalled");
    char line[32];

    snprintf(line, sizeof(line), "kill 19 %d", (int)id);
    shell.run(line);
    reap();
    ASSERT_TRUE(pditest::saw(psRow(shell, "signalled"), "T"));

    snprintf(line, sizeof(line), "kill 18 %d", (int)id);
    shell.run(line);
    reap();
    ASSERT_FALSE(pditest::saw(psRow(shell, "signalled"), "T"));

    release(id);
    reap();
}

TEST(cmdproc, kill_removes_a_task_outright)
{
    pditest::Shell shell;
    pdiutil::task_id_t id = parkTask("doomedtask");
    ASSERT_TRUE(saw(shell.run("ps"), "doomedtask"));

    char line[32];
    snprintf(line, sizeof(line), "kill 9 %d", (int)id);
    shell.run(line);
    reap();

    ASSERT_FALSE(saw(shell.run("ps"), "doomedtask"));
}

TEST(cmdproc, kill_of_a_pid_that_is_not_there_reports_it)
{
    pditest::Shell shell;
    std::string out = shell.run("kill 9 30000");

    ASSERT_FALSE(saw(out, "PID"));
    ASSERT_NE(shell.result(), CMD_RESULT_MAX);
}

TEST(cmdproc, pkill_signals_every_task_with_the_name)
{
    pditest::Shell shell;
    pdiutil::task_id_t first = parkTask("twinned");
    pdiutil::task_id_t second = parkTask("twinned");

    shell.run("pkill 9 twinned");
    reap();
    ASSERT_FALSE(saw(shell.run("ps"), "twinned"));

    release(first);
    release(second);
    reap();
}

TEST(cmdproc, killall_removes_every_task_with_the_name)
{
    pditest::Shell shell;
    pdiutil::task_id_t first = parkTask("tripled");
    pdiutil::task_id_t second = parkTask("tripled");
    pdiutil::task_id_t third = parkTask("survivor");

    shell.run("killall tripled");
    reap();

    std::string out = shell.run("ps");
    ASSERT_FALSE(saw(out, "tripled"));
    ASSERT_TRUE(saw(out, "survivor"));

    release(first);
    release(second);
    release(third);
    reap();
}

TEST(cmdproc, renice_changes_a_live_task_priority)
{
    pditest::Shell shell;
    pdiutil::task_id_t id = parkTask("reniced");

    char line[32];
    snprintf(line, sizeof(line), "renice 10 %d", (int)id);
    shell.run(line);

    ASSERT_TRUE(saw(shell.run("ps"), "reniced"));
    ASSERT_TRUE(saw(shell.run("ps"), "10"));

    release(id);
    reap();
}

/**
 * renice documents the range as clamped rather than rejected, so a value past
 * the end lands on the end.
 */
TEST(cmdproc, renice_clamps_a_value_past_the_range)
{
    pditest::Shell shell;
    pdiutil::task_id_t id = parkTask("outofrange");

    char line[32];
    snprintf(line, sizeof(line), "renice 99 %d", (int)id);
    shell.run(line);
    ASSERT_EQ(shell.result(), CMD_RESULT_OK);
    ASSERT_TRUE(saw(shell.run("ps"), "19"));

    release(id);
    reap();
}

/* ----------------------------------------------------------------- srvc */

TEST(cmdproc, srvc_list_names_the_running_services)
{
    pditest::Shell shell;
    std::string out = shell.run("srvc list");

    ASSERT_TRUE(out.length() > 0);
    ASSERT_EQ(shell.result(), CMD_RESULT_OK);
}

TEST(cmdproc, srvc_status_of_an_unknown_service_is_not_ok)
{
    pditest::Shell shell;
    shell.run("srvc status nosuchservice");

    ASSERT_NE(shell.result(), CMD_RESULT_MAX);
}

/* --------------------------------------------------------------- uptime */

TEST(cmdproc, uptime_reports_a_duration)
{
    pditest::Shell shell;
    std::string out = shell.run("uptime");

    ASSERT_TRUE(saw(out, "s"));
    ASSERT_EQ(shell.result(), CMD_RESULT_OK);
}

TEST(cmdproc, uptime_grows_with_the_clock)
{
    pditest::Shell shell;
    std::string first = shell.run("uptime");
    __i_dvc_ctrl.wait(1100);
    std::string second = shell.run("uptime");

    ASSERT_STRNE(first.c_str(), second.c_str());
}

/* ------------------------------------------------------------------ misc */

TEST(cmdmisc, help_lists_every_registered_command)
{
    pditest::Shell shell;
    std::string out = shell.run("help");

    ASSERT_TRUE(saw(out, "Registered commands"));
    ASSERT_TRUE(saw(out, "ls"));
    ASSERT_TRUE(saw(out, "reboot"));
    ASSERT_TRUE(saw(out, "help"));
}

TEST(cmdmisc, help_prints_a_usage_line_for_each_command)
{
    pditest::Shell shell;
    std::string out = shell.run("help");

    ASSERT_TRUE(saw(out, "print current working directory"));
    ASSERT_TRUE(saw(out, "list files and directories"));
}

TEST(cmdmisc, cls_sends_a_clear_sequence)
{
    pditest::Shell shell;
    std::string out = shell.run("cls");

    ASSERT_TRUE(saw(out, "\x1b["));
}

TEST(cmdmisc, an_unknown_command_is_reported_as_not_found)
{
    pditest::Shell shell;
    shell.run("definitelynotacommand");

    ASSERT_EQ(shell.result(), CMD_RESULT_NOT_FOUND);
}

TEST(cmdmisc, reboot_asks_the_device_to_restart)
{
    pditest::Shell shell;
    uint32_t before = __i_dvc_ctrl.getRestartCount();

    shell.run("reboot");

    ASSERT_EQ(__i_dvc_ctrl.getRestartCount(), before + 1);
}
