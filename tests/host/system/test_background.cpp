/************************* Background Command Tests ****************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Covers commands that keep running behind a session: each run of their work acts
as the session that started them, shows only on its terminal, and ends with it.

Author          : Suraj I.
created Date    : 22nd Oct 2026
******************************************************************************/

#include <ShellHarness.h>
#include <pditest.h>

using pditest::saw;

namespace
{

/**
 * Runs scheduled tasks until the terminal shows the text, or long enough for
 * watch's shortest interval, one second, to pass twice.
 */
void pumpUntilShown(pditest::Shell &sh, const char *text)
{
    uint32_t began = __i_dvc_ctrl.millis_now();
    while (!saw(sh.terminal().captured(), text) && (__i_dvc_ctrl.millis_now() - began) < 2500)
    {
        __task_scheduler.handle_tasks();
    }
}

/**
 * Scheduled tasks running under the name given.
 */
int tasksNamed(const char *name)
{
    int count = 0;
    for (uint16_t i = 0; i < __task_scheduler.getTaskSlots(); i++)
    {
        task_t *task = __task_scheduler.getTaskByIndex(i);
        count += (nullptr != task && nullptr != task->m_name && 0 == strcmp(task->m_name, name)) ? 1 : 0;
    }
    return count;
}

/**
 * Runs scheduled tasks for a while, long enough for a 50 ms task to fire.
 */
void pumpFor(uint32_t ms)
{
    uint32_t began = __i_dvc_ctrl.millis_now();
    while ((__i_dvc_ctrl.millis_now() - began) < ms)
    {
        __task_scheduler.handle_tasks();
    }
}

} // namespace

TEST(background, watch_runs_its_command_as_the_user_who_started_it)
{
    pditest::Shell root;
    __i_fs.writeFile("/bg_secret.txt", "top secret", 10);
    ASSERT_EQ(__i_fs.setFilePermissions("/bg_secret.txt", 0600), 0);
    pditest::Shell user(1000, 1000);

    user.run("watch c=\"cat /bg_secret.txt\",i=1000,n=1");
    root.run("pwd");
    user.terminal().forget();
    root.terminal().forget();
    pumpUntilShown(user, std::to_string((int)PDI_ERR_PERM).c_str());

    ASSERT_TRUE(saw(user.terminal().captured(), std::to_string((int)PDI_ERR_PERM).c_str()));
    ASSERT_FALSE(saw(user.terminal().captured(), "top secret"));
    ASSERT_FALSE(saw(root.terminal().captured(), "top secret"));
    __i_fs.deleteFile("/bg_secret.txt");
}

TEST(background, watch_shows_its_command_only_on_the_terminal_that_started_it)
{
    pditest::Shell owner;
    pditest::Shell other(1000, 1000);

    owner.run("watch c=\"echo from the watch\",i=1000,n=1");
    other.run("pwd");
    owner.terminal().forget();
    other.terminal().forget();
    pumpUntilShown(owner, "from the watch");

    ASSERT_TRUE(saw(owner.terminal().captured(), "from the watch"));
    ASSERT_FALSE(saw(other.terminal().captured(), "from the watch"));
}

TEST(background, a_session_that_ends_takes_its_watch_and_top_with_it)
{
    pditest::Shell looker;
    pumpFor(150);
    int watches = tasksNamed("watch");
    int tops = tasksNamed("top");
    {
        pditest::Shell leaving;
        leaving.run("watch c=\"echo still here\",i=1000,n=1000");
        leaving.run("top i=500,n=1000");
        ASSERT_EQ(tasksNamed("watch"), watches + 1);
        ASSERT_EQ(tasksNamed("top"), tops + 1);

        SessionManager::detach(&leaving.terminal());
    }
    pumpFor(1200);

    ASSERT_EQ(tasksNamed("watch"), watches);
    ASSERT_EQ(tasksNamed("top"), tops);
    ASSERT_FALSE(saw(looker.terminal().captured(), "still here"));
}
