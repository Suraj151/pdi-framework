/*************************** Service Task Tests *******************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Covers the invariant service control rests on: a task a service owns is registered
through the service task API, so service start/stop/disable reaches it. A task
registered straight on the scheduler still runs, and would silently be beyond the
reach of every one of those commands.

Author          : Suraj I.
created Date    : 30th Aug 2026
******************************************************************************/

#include <MountedStack.h>
#include <ShellHarness.h>
#include <pditest.h>

#include <service_provider/ServiceProvider.h>

namespace
{

struct ProbeTaskService : public ServiceProvider
{
    ProbeTaskService() : ServiceProvider(SERVICE_DATABASE, "TaskProbe") {}
};

/* The scheduler marks a cleared task a zombie and frees the slot only when it is
   reaped, and the table is a fixed 25 shared with everything the stack started. A
   probe that does not reap leaves the next test with nowhere to register. */
struct BorrowedTaskSlot
{
    BorrowedTaskSlot() : m_saved(ServiceProvider::getService(SERVICE_DATABASE)) {}
    ~BorrowedTaskSlot()
    {
        ServiceProvider::m_services[SERVICE_DATABASE] = m_saved;
        __task_scheduler.remove_expired_tasks();
    }
    ServiceProvider *m_saved;
};

} // namespace

TEST(servicetasks, a_task_scheduled_through_the_service_is_tracked)
{
    BorrowedTaskSlot slot;
    ProbeTaskService probe;

    pdiutil::task_id_t id = probe.serviceSetTimeout([]() {}, 10000, __i_dvc_ctrl.millis_now());
    ASSERT_TRUE(id > 0);
    ASSERT_TRUE(probe.isServiceTaskTracked(id));
    ASSERT_EQ((uint32_t)probe.getServiceTaskCount(), 1u);

    __task_scheduler.clearTimeout(id);
}

TEST(servicetasks, a_tracked_task_carries_the_service_name)
{
    BorrowedTaskSlot slot;
    ProbeTaskService probe;

    pdiutil::task_id_t id = probe.serviceSetInterval([]() {}, 10000, __i_dvc_ctrl.millis_now());
    ASSERT_TRUE(id > 0);

    task_t *t = __task_scheduler.get_task(id);
    ASSERT_TRUE(nullptr != t);
    ASSERT_TRUE(nullptr != t->m_name);
    ASSERT_STREQ(t->m_name, "TaskProbe");

    __task_scheduler.clearInterval(id);
}

TEST(servicetasks, stopping_the_service_reaps_every_tracked_task)
{
    BorrowedTaskSlot slot;
    ProbeTaskService probe;

    probe.serviceSetInterval([]() {}, 10000, __i_dvc_ctrl.millis_now());
    probe.serviceSetTimeout([]() {}, 10000, __i_dvc_ctrl.millis_now());
    ASSERT_EQ((uint32_t)probe.getServiceTaskCount(), 2u);

    probe.stopService();
    ASSERT_EQ((uint32_t)probe.getServiceTaskCount(), 0u);
}

TEST(servicetasks, a_signal_reaches_every_tracked_task)
{
    BorrowedTaskSlot slot;
    ProbeTaskService probe;

    probe.serviceSetInterval([]() {}, 10000, __i_dvc_ctrl.millis_now());
    probe.serviceSetInterval([]() {}, 10000, __i_dvc_ctrl.millis_now());

    ASSERT_EQ((uint32_t)probe.signalAllServiceTasks(SIG_STOP), 2u);

    probe.stopService();
}

TEST(servicetasks, a_task_registered_straight_on_the_scheduler_is_out_of_reach)
{
    BorrowedTaskSlot slot;
    ProbeTaskService probe;

    pdiutil::task_id_t loose = __task_scheduler.setTimeout([]() {}, 10000, __i_dvc_ctrl.millis_now());
    ASSERT_TRUE(loose > 0);

    ASSERT_FALSE(probe.isServiceTaskTracked(loose));
    ASSERT_EQ((uint32_t)probe.signalAllServiceTasks(SIG_STOP), 0u);

    probe.stopService();
    ASSERT_TRUE(nullptr != __task_scheduler.get_task(loose));

    __task_scheduler.clearTimeout(loose);
}

TEST(servicetasks, handing_a_loose_task_to_the_service_brings_it_back_in_reach)
{
    BorrowedTaskSlot slot;
    ProbeTaskService probe;

    pdiutil::task_id_t loose = __task_scheduler.setTimeout([]() {}, 10000, __i_dvc_ctrl.millis_now());
    ASSERT_TRUE(probe.trackServiceTask(loose));
    ASSERT_TRUE(probe.isServiceTaskTracked(loose));
    ASSERT_EQ((uint32_t)probe.signalAllServiceTasks(SIG_STOP), 1u);

    probe.stopService();
}

TEST(servicetasks, the_tracked_list_refuses_rather_than_overflowing_quietly)
{
    BorrowedTaskSlot slot;
    ProbeTaskService probe;

    pdiutil::task_id_t ids[MAX_SERVICE_TASKS];
    for (uint8_t i = 0; i < MAX_SERVICE_TASKS; i++)
    {
        ids[i] = probe.serviceSetInterval([]() {}, 10000, __i_dvc_ctrl.millis_now());
        ASSERT_TRUE(ids[i] > 0);
    }
    ASSERT_EQ((uint32_t)probe.getServiceTaskCount(), (uint32_t)MAX_SERVICE_TASKS);

    pdiutil::task_id_t overflow = probe.serviceSetInterval([]() {}, 10000, __i_dvc_ctrl.millis_now());
    ASSERT_TRUE(overflow > 0);
    ASSERT_FALSE(probe.isServiceTaskTracked(overflow));

    __task_scheduler.clearInterval(overflow);
    probe.stopService();
}

/* ------------------------------------------------- the whole stack, at boot */

TEST(servicetasks, every_running_task_named_for_a_service_is_tracked_by_it)
{
    pditest::mountedVfs();

    for (uint16_t idx = 0; idx < __task_scheduler.getTaskSlots(); idx++)
    {
        task_t *t = __task_scheduler.getTaskByIndex(idx);
        if (nullptr == t || nullptr == t->m_name)
        {
            continue;
        }

        for (uint8_t i = 0; i < SERVICE_MAX; i++)
        {
            ServiceProvider *s = ServiceProvider::getService((service_t)i);
            if (nullptr == s || nullptr == s->m_service_name)
            {
                continue;
            }

            if (0 == strcmp_ro(t->m_name, s->m_service_name))
            {
                ASSERT_TRUE(s->isServiceTaskTracked(t->m_task_id));
            }
        }
    }
}

TEST(servicetasks, the_boot_scan_actually_sees_service_tasks)
{
    pditest::mountedVfs();
    BorrowedTaskSlot slot;
    ProbeTaskService probe;

    pdiutil::task_id_t id = probe.serviceSetInterval([]() {}, 10000, __i_dvc_ctrl.millis_now());
    ASSERT_TRUE(id > 0);

    uint16_t named = 0;
    for (uint16_t idx = 0; idx < __task_scheduler.getTaskSlots(); idx++)
    {
        task_t *t = __task_scheduler.getTaskByIndex(idx);
        if (nullptr == t || nullptr == t->m_name) continue;

        for (uint8_t i = 0; i < SERVICE_MAX; i++)
        {
            ServiceProvider *s = ServiceProvider::getService((service_t)i);
            if (nullptr == s || nullptr == s->m_service_name) continue;
            if (0 == strcmp_ro(t->m_name, s->m_service_name)) named++;
        }
    }

    ASSERT_TRUE(named > 0);

    probe.stopService();
}

TEST(servicetasks, a_priority_above_the_ceiling_is_clamped_to_it)
{
    // the ceiling has to bind where the task is taken, the way renice clamps
    // the nice it is scored against, or it is only advice
    pdiutil::task_id_t id = __task_scheduler.setInterval([](){}, 1000,
        __i_dvc_ctrl.millis_now(), (pdiutil::task_priority_t)250);
    ASSERT_TRUE(id >= 0);

    task_t *t = nullptr;
    for (uint16_t idx = 0; idx < __task_scheduler.getTaskSlots(); idx++)
    {
        task_t *c = __task_scheduler.getTaskByIndex(idx);
        if (nullptr != c && c->m_task_id == id) { t = c; break; }
    }

    ASSERT_TRUE(nullptr != t);
    ASSERT_EQ((int)t->m_task_priority, (int)MAX_TASK_PRIORITY);

    __task_scheduler.remove_task(id);
}

TEST(servicetasks, a_task_can_be_given_the_deadline_policy)
{
    pdiutil::task_id_t id = __task_scheduler.setInterval([](){}, 1000,
        __i_dvc_ctrl.millis_now(), MAX_TASK_PRIORITY);
    ASSERT_TRUE(id >= 0);

    ASSERT_TRUE(__task_scheduler.setTaskPolicy(id, TASK_POLICY_DEADLINE));

    task_t *t = nullptr;
    for (uint16_t idx = 0; idx < __task_scheduler.getTaskSlots(); idx++)
    {
        task_t *c = __task_scheduler.getTaskByIndex(idx);
        if (nullptr != c && c->m_task_id == id) { t = c; break; }
    }

    ASSERT_TRUE(nullptr != t);
    ASSERT_EQ((int)t->m_task_policy, (int)TASK_POLICY_DEADLINE);

    // a policy cannot be set on a task that is not there
    ASSERT_FALSE(__task_scheduler.setTaskPolicy((pdiutil::task_id_t)31000, TASK_POLICY_DEADLINE));

    __task_scheduler.remove_task(id);
}

TEST(servicetasks, a_task_that_is_rarely_due_does_not_hold_back_one_that_is)
{
    pditest::readyScheduler();
    int fast = 0;
    int slow = 0;

    pdiutil::task_id_t slow_id = __task_scheduler.setInterval([&](){ slow++; }, 60000,
        __i_dvc_ctrl.millis_now());
    pdiutil::task_id_t fast_id = __task_scheduler.setInterval([&](){ fast++; }, 1,
        __i_dvc_ctrl.millis_now());

    ASSERT_TRUE(slow_id >= 0);
    ASSERT_TRUE(fast_id >= 0);

    uint32_t start = __i_dvc_ctrl.millis_now();
    while ((__i_dvc_ctrl.millis_now() - start) < 300)
    {
        __task_scheduler.run();
    }

    __task_scheduler.remove_task(slow_id);
    __task_scheduler.remove_task(fast_id);

    ASSERT_EQ(slow, 0);
    ASSERT_TRUE(fast > 10);
}

TEST(servicetasks, a_deadline_task_is_served_more_often_than_a_default_one)
{
    pditest::readyScheduler();
    int deadline_runs = 0;
    int fifo_runs = 0;

    pdiutil::task_id_t d_id = __task_scheduler.setInterval([&](){
        deadline_runs++;
        uint32_t s = __i_dvc_ctrl.millis_now();
        while ((__i_dvc_ctrl.millis_now() - s) < 1) { }
    }, 1, __i_dvc_ctrl.millis_now(), MEDIUM_TASK_PRIORITY);

    pdiutil::task_id_t f_id = __task_scheduler.setInterval([&](){
        fifo_runs++;
        uint32_t s = __i_dvc_ctrl.millis_now();
        while ((__i_dvc_ctrl.millis_now() - s) < 1) { }
    }, 1, __i_dvc_ctrl.millis_now(), MEDIUM_TASK_PRIORITY);

    ASSERT_TRUE(d_id >= 0);
    ASSERT_TRUE(f_id >= 0);
    ASSERT_TRUE(__task_scheduler.setTaskPolicy(d_id, TASK_POLICY_DEADLINE));

    uint32_t start = __i_dvc_ctrl.millis_now();
    while ((__i_dvc_ctrl.millis_now() - start) < 400)
    {
        __task_scheduler.run();
    }

    __task_scheduler.remove_task(d_id);
    __task_scheduler.remove_task(f_id);

    ASSERT_TRUE(fifo_runs > 0);
    ASSERT_TRUE(deadline_runs > (fifo_runs * 2));
}

TEST(servicetasks, round_robin_ignores_priority_where_the_default_policy_does_not)
{
    pditest::readyScheduler();
    int low_runs = 0;
    int high_runs = 0;

    pdiutil::task_id_t low_id = __task_scheduler.setInterval([&](){
        low_runs++;
        uint32_t s = __i_dvc_ctrl.millis_now();
        while ((__i_dvc_ctrl.millis_now() - s) < 1) { }
    }, 1, __i_dvc_ctrl.millis_now(), LOW_TASK_PRIORITY);

    pdiutil::task_id_t high_id = __task_scheduler.setInterval([&](){
        high_runs++;
        uint32_t s = __i_dvc_ctrl.millis_now();
        while ((__i_dvc_ctrl.millis_now() - s) < 1) { }
    }, 1, __i_dvc_ctrl.millis_now(), HIGH_TASK_PRIORITY);

    ASSERT_TRUE(low_id >= 0);
    ASSERT_TRUE(high_id >= 0);
    ASSERT_TRUE(__task_scheduler.setTaskPolicy(low_id, TASK_POLICY_ROUNDROBIN));
    ASSERT_TRUE(__task_scheduler.setTaskPolicy(high_id, TASK_POLICY_ROUNDROBIN));

    uint32_t start = __i_dvc_ctrl.millis_now();
    while ((__i_dvc_ctrl.millis_now() - start) < 400)
    {
        __task_scheduler.run();
    }

    __task_scheduler.remove_task(low_id);
    __task_scheduler.remove_task(high_id);

    ASSERT_TRUE(low_runs > 0);
    ASSERT_TRUE(high_runs > 0);

    int diff = (low_runs > high_runs) ? (low_runs - high_runs) : (high_runs - low_runs);
    ASSERT_TRUE(diff <= (low_runs + high_runs) / 4);
}

TEST(servicetasks, every_single_priority_step_changes_the_weight)
{
    task_t lower;
    task_t upper;

    for (uint8_t p = 0; p < MAX_TASK_PRIORITY; p++)
    {
        lower.m_task_priority = p;
        upper.m_task_priority = (uint8_t)(p + 1);

        ASSERT_TRUE(TaskScheduler::taskWeight(lower) != TaskScheduler::taskWeight(upper));
    }
}

TEST(servicetasks, a_renice_of_one_moves_the_cached_weight)
{
    pdiutil::task_id_t id = __task_scheduler.setInterval([&](){ }, 1000,
        __i_dvc_ctrl.millis_now(), LOW_TASK_PRIORITY);

    ASSERT_TRUE(id >= 0);

    int16_t idx = __task_scheduler.is_registered_task(id);
    ASSERT_TRUE(idx >= 0);

    uint32_t before = __task_scheduler.get_task(id)->m_weight;
    ASSERT_TRUE(__task_scheduler.setTaskNice(id, -1));
    uint32_t after = __task_scheduler.get_task(id)->m_weight;

    __task_scheduler.remove_task(id);

    ASSERT_TRUE(before > 0);
    ASSERT_TRUE(after > before);
}

/* -------------------------------------------- who a service's task runs as */

namespace
{

void pumpFor(uint32_t ms)
{
    uint32_t began = __i_dvc_ctrl.millis_now();
    while ((__i_dvc_ctrl.millis_now() - began) < ms)
    {
        __task_scheduler.handle_tasks();
    }
}

} // namespace

/**
 * A service's own task runs between lines typed on terminals, so it must not
 * pick up the identity of whoever typed last.
 */
TEST(servicetasks, a_service_task_runs_as_the_system_after_a_user_typed)
{
    BorrowedTaskSlot slot;
    ProbeTaskService probe;
    pditest::Shell user(1000, 1000);
    user.run("pwd");
    session_t *typed = SessionManager::current();
    ASSERT_EQ(SessionManager::getCurrentUid(), 1000);

    int seen = -1;
    bool hadSession = true;
    pdiutil::task_id_t id = probe.serviceSetTimeout([&]() {
        seen = SessionManager::getCurrentUid();
        hadSession = (nullptr != SessionManager::current());
    }, 0, __i_dvc_ctrl.millis_now());
    ASSERT_TRUE(id > 0);
    pumpFor(50);

    ASSERT_EQ(seen, 0);
    ASSERT_FALSE(hadSession);
    ASSERT_TRUE(SessionManager::current() == typed);
}

TEST(servicetasks, a_task_registered_straight_on_the_scheduler_keeps_the_session)
{
    BorrowedTaskSlot slot;
    pditest::Shell user(1000, 1000);
    user.run("pwd");

    int seen = -1;
    pdiutil::task_id_t id = __task_scheduler.setTimeout([&]() {
        seen = SessionManager::getCurrentUid();
    }, 0, __i_dvc_ctrl.millis_now());
    ASSERT_TRUE(id > 0);
    pumpFor(50);

    ASSERT_EQ(seen, 1000);
}

TEST(servicetasks, a_session_let_go_during_a_service_task_is_not_put_back)
{
    BorrowedTaskSlot slot;
    ProbeTaskService probe;
    pditest::Shell *user = new pditest::Shell(1000, 1000);
    user->run("pwd");
    session_t *typed = SessionManager::current();
    iTerminalInterface *terminal = typed->m_terminal;

    bool ran = false;
    pdiutil::task_id_t id = probe.serviceSetTimeout([&]() {
        SessionManager::detach(terminal);
        ran = true;
    }, 0, __i_dvc_ctrl.millis_now());
    ASSERT_TRUE(id > 0);
    pumpFor(50);

    ASSERT_TRUE(ran);
    ASSERT_TRUE(nullptr == SessionManager::current());
    delete user;
}
