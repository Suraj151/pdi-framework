/***************************** Event Util Tests *******************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 16th Aug 2026
******************************************************************************/

#include <FakeClock.h>
#include <pditest.h>
#include <utility/EventUtil.h>

static int s_hits_a = 0;
static int s_hits_b = 0;
static void *s_last_arg = nullptr;

static void onEventA(void *arg)
{
    s_hits_a++;
    s_last_arg = arg;
}

static void onEventB(void *arg)
{
    s_hits_b++;
}

static void resetHits()
{
    s_hits_a = 0;
    s_hits_b = 0;
    s_last_arg = nullptr;
}

TEST(events, listener_runs_for_its_own_event)
{
    EventUtil bus;
    pditest::FakeClock clock;
    bus.begin(&clock);
    resetHits();

    ASSERT_TRUE(bus.add_event_listener(EVENT_WIFI_STA_CONNECTED, onEventA));
    bus.execute_event(EVENT_WIFI_STA_CONNECTED, nullptr);

    ASSERT_EQ(s_hits_a, 1);
}

TEST(events, listener_ignores_a_different_event)
{
    EventUtil bus;
    pditest::FakeClock clock;
    bus.begin(&clock);
    resetHits();

    bus.add_event_listener(EVENT_WIFI_STA_CONNECTED, onEventA);
    bus.execute_event(EVENT_WIFI_STA_GOT_IP, nullptr);

    ASSERT_EQ(s_hits_a, 0);
}

TEST(events, every_listener_for_an_event_runs)
{
    EventUtil bus;
    pditest::FakeClock clock;
    bus.begin(&clock);
    resetHits();

    bus.add_event_listener(EVENT_WIFI_STA_CONNECTED, onEventA);
    bus.add_event_listener(EVENT_WIFI_STA_CONNECTED, onEventB);
    bus.execute_event(EVENT_WIFI_STA_CONNECTED, nullptr);

    ASSERT_EQ(s_hits_a, 1);
    ASSERT_EQ(s_hits_b, 1);
}

TEST(events, argument_reaches_the_listener)
{
    EventUtil bus;
    pditest::FakeClock clock;
    bus.begin(&clock);
    resetHits();

    int payload = 42;
    bus.add_event_listener(EVENT_WIFI_STA_CONNECTED, onEventA);
    bus.execute_event(EVENT_WIFI_STA_CONNECTED, &payload);

    ASSERT_EQ(s_last_arg, (void *)&payload);
}

TEST(events, the_same_listener_fires_on_every_emit)
{
    EventUtil bus;
    pditest::FakeClock clock;
    bus.begin(&clock);
    resetHits();

    bus.add_event_listener(EVENT_WIFI_STA_CONNECTED, onEventA);
    bus.execute_event(EVENT_WIFI_STA_CONNECTED, nullptr);
    bus.execute_event(EVENT_WIFI_STA_CONNECTED, nullptr);
    bus.execute_event(EVENT_WIFI_STA_CONNECTED, nullptr);

    ASSERT_EQ(s_hits_a, 3);
}

TEST(events, emitting_with_no_listeners_is_harmless)
{
    EventUtil bus;
    pditest::FakeClock clock;
    bus.begin(&clock);
    resetHits();

    bus.execute_event(EVENT_WIFI_STA_CONNECTED, nullptr);
    ASSERT_EQ(s_hits_a, 0);
}

TEST(events, a_null_handler_is_skipped_rather_than_called)
{
    EventUtil bus;
    pditest::FakeClock clock;
    bus.begin(&clock);
    resetHits();

    bus.add_event_listener(EVENT_WIFI_STA_CONNECTED, nullptr);
    bus.add_event_listener(EVENT_WIFI_STA_CONNECTED, onEventA);
    bus.execute_event(EVENT_WIFI_STA_CONNECTED, nullptr);

    ASSERT_EQ(s_hits_a, 1);
}

TEST(events, registration_stops_at_the_listener_limit)
{
    EventUtil bus;
    pditest::FakeClock clock;
    bus.begin(&clock);

    for (uint16_t i = 0; i < MAX_EVENT_LISTENERS; i++)
    {
        ASSERT_TRUE(bus.add_event_listener(EVENT_WIFI_STA_CONNECTED, onEventA));
    }

    ASSERT_FALSE(bus.add_event_listener(EVENT_WIFI_STA_CONNECTED, onEventA));
}

TEST(events, works_before_a_utility_interface_is_attached)
{
    EventUtil bus;
    resetHits();

    bus.add_event_listener(EVENT_WIFI_STA_CONNECTED, onEventA);
    bus.execute_event(EVENT_WIFI_STA_CONNECTED, nullptr);

    ASSERT_EQ(s_hits_a, 1);
}
