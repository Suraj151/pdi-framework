/******************************** GPIO Tests **********************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

The gpio service's own json: what it publishes about the pins, what it accepts
back, and which pins an allow list lets a server touch. The payload arrives from
a remote server, so a value out of range, a pin it was never granted or a pin
the board keeps for itself must change nothing.

Author          : Suraj I.
created Date    : 8th Oct 2026
******************************************************************************/

#include <MountedStack.h>
#include <ShellHarness.h>
#include <pditest.h>

#ifdef ENABLE_GPIO_SERVICE

#include <service_provider/device/GpioServiceProvider.h>
#include <string>

namespace
{

    const uint8_t ANALOG0 = MAX_DIGITAL_GPIO_PINS;
    const uint8_t RESERVED = 3;

    /**
     * Holds the service's working copy for the test and puts it back after,
     * so one test's pins never become another's starting point.
     */
    struct PinState
    {
        PinState() : m_saved(__gpio_service.m_gpio_config_copy)
        {
            __gpio_service.m_gpio_config_copy.clear();
        }

        ~PinState()
        {
            __gpio_service.m_gpio_config_copy = m_saved;
        }

        gpio_config_table m_saved;
    };

    void apply(const char *payload, pdiutil::vector<pdiutil::string> *allowed = nullptr)
    {
        pdiutil::string copy = payload;
        __gpio_service.applyGpioJsonPayload((char *)copy.c_str(), (uint16_t)copy.size(), allowed);
    }

    uint8_t modeOf(uint8_t pin)
    {
        return __gpio_service.m_gpio_config_copy.gpio_mode[pin];
    }

    uint16_t valueOf(uint8_t pin)
    {
        return __gpio_service.m_gpio_config_copy.gpio_readings[pin];
    }

    std::string published(bool event = false, pdiutil::vector<pdiutil::string> *allowed = nullptr)
    {
        pdiutil::string payload;
        __gpio_service.appendGpioJsonPayload(payload, event, allowed);
        return std::string(payload.c_str());
    }

    bool has(const std::string &text, const char *needle)
    {
        return text.find(needle) != std::string::npos;
    }
}

TEST(gpio, with_no_allow_list_every_pin_is_allowed)
{
    for (uint8_t pin = 0; pin < MAX_GPIO_PINS; pin++)
    {
        ASSERT_TRUE(__gpio_service.isAllowedGpioPin(pin, nullptr));
    }
}

TEST(gpio, an_allow_list_grants_only_the_pins_it_names)
{
    pdiutil::vector<pdiutil::string> allowed;
    allowed.push_back("D1");
    allowed.push_back("a0");

    ASSERT_TRUE(__gpio_service.isAllowedGpioPin(1, &allowed));
    ASSERT_FALSE(__gpio_service.isAllowedGpioPin(2, &allowed));
    ASSERT_TRUE(__gpio_service.isAllowedGpioPin(ANALOG0, &allowed));
    ASSERT_FALSE(__gpio_service.isAllowedGpioPin(ANALOG0 + 1, &allowed));
}

TEST(gpio, an_allow_list_entry_does_not_grant_a_pin_whose_name_it_starts_with)
{
    pdiutil::vector<pdiutil::string> allowed;
    allowed.push_back("D10");

    ASSERT_FALSE(__gpio_service.isAllowedGpioPin(1, &allowed));
    ASSERT_TRUE(__gpio_service.isAllowedGpioPin(10, &allowed));
}

TEST(gpio, an_empty_allow_list_grants_nothing)
{
    pdiutil::vector<pdiutil::string> allowed;

    ASSERT_FALSE(__gpio_service.isAllowedGpioPin(1, &allowed));
    ASSERT_FALSE(__gpio_service.isAllowedGpioPin(ANALOG0, &allowed));
}

TEST(gpio, a_payload_sets_the_mode_and_value_of_the_pins_it_names)
{
    PinState state;

    apply("{\"data\":{\"D1\":{\"mode\":1,\"val\":1},\"A0\":{\"mode\":4,\"val\":512}}}");

    ASSERT_EQ(modeOf(1), (uint8_t)DIGITAL_WRITE);
    ASSERT_EQ(valueOf(1), (uint16_t)1);
    ASSERT_EQ(modeOf(ANALOG0), (uint8_t)ANALOG_WRITE);
    ASSERT_EQ(valueOf(ANALOG0), (uint16_t)512);
    ASSERT_EQ(modeOf(2), (uint8_t)OFF);
}

TEST(gpio, a_pin_takes_its_own_settings_when_a_longer_label_comes_first)
{
    PinState state;

    apply("{\"data\":{\"D10\":{\"mode\":1,\"val\":1},\"D1\":{\"mode\":2,\"val\":0}}}");

    ASSERT_EQ(modeOf(1), (uint8_t)DIGITAL_READ);
    ASSERT_EQ(modeOf(10), (uint8_t)DIGITAL_WRITE);
}

TEST(gpio, a_pin_named_only_inside_a_longer_label_is_left_alone)
{
    PinState state;

    apply("{\"data\":{\"D10\":{\"mode\":1,\"val\":1}}}");

    ASSERT_EQ(modeOf(1), (uint8_t)OFF);
    ASSERT_EQ(modeOf(10), (uint8_t)DIGITAL_WRITE);
}

TEST(gpio, a_pin_entry_whose_mode_is_refused_changes_nothing_value_included)
{
    PinState state;

    apply("{\"data\":{\"D4\":{\"mode\":1,\"val\":1}}}");
    apply("{\"data\":{\"D4\":{\"mode\":99,\"val\":0}}}");

    ASSERT_EQ(modeOf(4), (uint8_t)DIGITAL_WRITE);
    ASSERT_EQ(valueOf(4), (uint16_t)1);
}

TEST(gpio, a_pin_entry_whose_value_is_refused_keeps_the_old_mode_too)
{
    PinState state;

    apply("{\"data\":{\"D4\":{\"mode\":1,\"val\":1}}}");
    apply("{\"data\":{\"D4\":{\"mode\":4,\"val\":5000}}}");

    ASSERT_EQ(modeOf(4), (uint8_t)DIGITAL_WRITE);
    ASSERT_EQ(valueOf(4), (uint16_t)1);
}

TEST(gpio, a_pin_entry_with_no_mode_change_still_sets_its_value)
{
    PinState state;

    apply("{\"data\":{\"D4\":{\"mode\":1,\"val\":1}}}");
    apply("{\"data\":{\"D4\":{\"mode\":\"NA\",\"val\":0}}}");

    ASSERT_EQ(modeOf(4), (uint8_t)DIGITAL_WRITE);
    ASSERT_EQ(valueOf(4), (uint16_t)0);
}

TEST(gpio, a_lowercase_pin_label_is_read_the_same_way)
{
    PinState state;

    apply("{\"data\":{\"d4\":{\"mode\":2,\"val\":0}}}");

    ASSERT_EQ(modeOf(4), (uint8_t)DIGITAL_READ);
}

TEST(gpio, a_mode_past_the_last_one_is_ignored)
{
    PinState state;

    apply("{\"data\":{\"D1\":{\"mode\":1,\"val\":0}}}");
    apply("{\"data\":{\"D1\":{\"mode\":9,\"val\":0}}}");

    ASSERT_EQ(modeOf(1), (uint8_t)DIGITAL_WRITE);
}

TEST(gpio, a_value_past_what_the_mode_can_hold_is_ignored)
{
    PinState state;

    apply("{\"data\":{\"D1\":{\"mode\":1,\"val\":1}}}");
    apply("{\"data\":{\"D1\":{\"mode\":1,\"val\":5}}}");
    ASSERT_EQ(valueOf(1), (uint16_t)1);

    apply("{\"data\":{\"A0\":{\"mode\":4,\"val\":700}}}");
    apply("{\"data\":{\"A0\":{\"mode\":4,\"val\":1024}}}");
    ASSERT_EQ(valueOf(ANALOG0), (uint16_t)700);
}

TEST(gpio, not_applicable_leaves_that_half_of_a_pin_alone)
{
    PinState state;

    apply("{\"data\":{\"D5\":{\"mode\":1,\"val\":1}}}");
    apply("{\"data\":{\"D5\":{\"mode\":\"NA\",\"val\":0}}}");

    ASSERT_EQ(modeOf(5), (uint8_t)DIGITAL_WRITE);
    ASSERT_EQ(valueOf(5), (uint16_t)0);

    apply("{\"data\":{\"D5\":{\"mode\":2,\"val\":\"NA\"}}}");

    ASSERT_EQ(modeOf(5), (uint8_t)DIGITAL_READ);
    ASSERT_EQ(valueOf(5), (uint16_t)0);
}

TEST(gpio, a_pin_the_board_keeps_for_itself_is_never_touched)
{
    PinState state;

    ASSERT_TRUE(__i_dvc_ctrl.isExceptionalGpio(RESERVED));
    apply("{\"data\":{\"D3\":{\"mode\":1,\"val\":1}}}");

    ASSERT_EQ(modeOf(RESERVED), (uint8_t)OFF);
    ASSERT_EQ(valueOf(RESERVED), (uint16_t)0);
}

TEST(gpio, a_pin_outside_the_allow_list_is_left_as_it_was)
{
    PinState state;

    pdiutil::vector<pdiutil::string> allowed;
    allowed.push_back("D1");

    apply("{\"data\":{\"D1\":{\"mode\":1,\"val\":1},\"D2\":{\"mode\":1,\"val\":1}}}", &allowed);

    ASSERT_EQ(modeOf(1), (uint8_t)DIGITAL_WRITE);
    ASSERT_EQ(modeOf(2), (uint8_t)OFF);
}

TEST(gpio, a_payload_missing_its_keys_changes_nothing)
{
    PinState state;

    apply("{\"D1\":{\"mode\":1,\"val\":1}}");
    apply("{\"data\":{\"D1\":{\"val\":1}}}");
    apply("{\"data\":{\"D1\":{\"mode\":1}}}");
    apply("");
    apply("not json at all");

    ASSERT_EQ(modeOf(1), (uint8_t)OFF);
    ASSERT_EQ(valueOf(1), (uint16_t)0);
}

TEST(gpio, the_published_payload_carries_every_pin_but_the_reserved_ones)
{
    PinState state;

    std::string out = published();

    ASSERT_TRUE(has(out, "\"data\":{"));
    ASSERT_TRUE(has(out, "\"D0\":{"));
    ASSERT_TRUE(has(out, "\"D15\":{"));
    ASSERT_TRUE(has(out, "\"A0\":{"));
    ASSERT_TRUE(has(out, "\"A1\":{"));
    ASSERT_FALSE(has(out, "\"D3\":{"));
    ASSERT_FALSE(has(out, "alrtkey"));
}

TEST(gpio, the_published_payload_is_balanced_and_ends_without_a_stray_comma)
{
    PinState state;

    std::string out = published();

    int depth = 0;
    for (size_t i = 0; i < out.size(); i++)
    {
        if ('{' == out[i]) depth++;
        if ('}' == out[i]) depth--;
        ASSERT_GE(depth, 0);
    }
    ASSERT_EQ(depth, 0);
    ASSERT_FALSE(has(out, ",}"));
    ASSERT_EQ(out[0], '{');
    ASSERT_EQ(out[out.size() - 1], '}');
}

TEST(gpio, an_allow_list_narrows_what_is_published)
{
    PinState state;

    pdiutil::vector<pdiutil::string> allowed;
    allowed.push_back("D2");

    std::string out = published(false, &allowed);

    ASSERT_TRUE(has(out, "\"D2\":{"));
    ASSERT_FALSE(has(out, "\"D1\":{"));
    ASSERT_FALSE(has(out, "\"A0\":{"));
    ASSERT_FALSE(has(out, ",}"));
}

TEST(gpio, an_allow_list_that_matches_nothing_publishes_an_empty_data_object)
{
    PinState state;

    pdiutil::vector<pdiutil::string> allowed;
    allowed.push_back("D99");

    std::string out = published(false, &allowed);

    ASSERT_TRUE(has(out, "\"data\":{}"));
}

TEST(gpio, an_event_post_names_the_pin_that_raised_it)
{
    PinState state;

    std::string out = published(true);

    ASSERT_TRUE(has(out, "\"alrtkey\":\""));
}

TEST(gpio, clearing_the_table_zeroes_every_pins_reading)
{
    gpio_config_table table;
    for (uint8_t pin = 0; pin < MAX_GPIO_PINS; pin++)
    {
        table.gpio_mode[pin] = DIGITAL_WRITE;
        table.gpio_readings[pin] = 0xFFFF;
    }

    table.clear();

    for (uint8_t pin = 0; pin < MAX_GPIO_PINS; pin++)
    {
        ASSERT_EQ(table.gpio_mode[pin], (uint8_t)OFF);
        ASSERT_EQ(table.gpio_readings[pin], (uint16_t)0);
    }
}

TEST(gpio, an_event_condition_is_judged_the_way_it_names)
{
    gpio_event event;
    event.eventConditionValue = 10;

    event.eventCondition = EQUAL;
    ASSERT_TRUE(event.isEventOccur(10));
    ASSERT_FALSE(event.isEventOccur(11));

    event.eventCondition = GREATER_THAN;
    ASSERT_TRUE(event.isEventOccur(11));
    ASSERT_FALSE(event.isEventOccur(10));

    event.eventCondition = LESS_THAN;
    ASSERT_TRUE(event.isEventOccur(9));
    ASSERT_FALSE(event.isEventOccur(10));

    event.eventCondition = GPIO_EVENT_CONDITION_MAX;
    ASSERT_FALSE(event.isEventOccur(10));
}

TEST(gpio, an_event_is_added_then_updated_in_place)
{
    gpio_config_table table;

    ASSERT_FALSE(table.gpioHasEvents(2));
    ASSERT_TRUE(table.updateGpioEvent(2, HTTP_SERVER, EQUAL, 1));
    ASSERT_TRUE(table.gpioHasEvents(2));
    ASSERT_TRUE(table.gpioHasEvents(2, EQUAL));
    ASSERT_FALSE(table.gpioHasEvents(2, GREATER_THAN));

    ASSERT_TRUE(table.updateGpioEvent(2, HTTP_SERVER, EQUAL, 7));

    gpio_event found;
    found.gpioNumber = 2;
    found.eventCondition = EQUAL;
    ASSERT_TRUE(table.getGpioEvent(found));
    ASSERT_EQ(found.eventConditionValue, (uint16_t)7);
    ASSERT_EQ(found.eventChannel, (uint8_t)HTTP_SERVER);

    int used = 0;
    for (int i = 0; i < MAX_GPIO_EVENTS; i++)
    {
        if (2 == table.gpio_events[i].gpioNumber) used++;
    }
    ASSERT_EQ(used, 1);
}

TEST(gpio, a_full_event_table_refuses_another)
{
    gpio_config_table table;

    for (uint8_t i = 0; i < MAX_GPIO_EVENTS; i++)
    {
        ASSERT_TRUE(table.updateGpioEvent((uint8_t)(i % 4), HTTP_SERVER, (uint8_t)(i % 3), i));
    }

    ASSERT_FALSE(table.updateGpioEvent(9, HTTP_SERVER, EQUAL, 1));
}

TEST(gpio, clearing_a_pins_events_leaves_the_others)
{
    gpio_config_table table;
    table.updateGpioEvent(2, HTTP_SERVER, EQUAL, 1);
    table.updateGpioEvent(2, HTTP_SERVER, GREATER_THAN, 5);
    table.updateGpioEvent(4, HTTP_SERVER, LESS_THAN, 3);

    table.clearGpioEvents(2);

    ASSERT_FALSE(table.gpioHasEvents(2));
    ASSERT_TRUE(table.gpioHasEvents(4, LESS_THAN));

    table.clearAllGpioEvents();
    ASSERT_FALSE(table.gpioHasEvents(4));
}

TEST(gpio, an_event_payload_sets_up_to_the_limit_of_events_on_a_pin)
{
    PinState state;
    __gpio_service.m_gpio_config_copy.updateGpioEvent(4, HTTP_SERVER, EQUAL, 1);

    apply("{\"data\":{}}");
    pdiutil::string payload =
        "{\"D2\":[{\"cmp\":0,\"val\":1},{\"cmp\":1,\"val\":5}]}";
    __gpio_service.applyGpioEventJsonPayload((char *)payload.c_str(), (uint16_t)payload.size());

    ASSERT_TRUE(__gpio_service.m_gpio_config_copy.gpioHasEvents(2, EQUAL));
    ASSERT_TRUE(__gpio_service.m_gpio_config_copy.gpioHasEvents(2, GREATER_THAN));
    ASSERT_FALSE(__gpio_service.m_gpio_config_copy.gpioHasEvents(4));

    gpio_event found;
    found.gpioNumber = 2;
    found.eventCondition = GREATER_THAN;
    ASSERT_TRUE(__gpio_service.m_gpio_config_copy.getGpioEvent(found));
    ASSERT_EQ(found.eventConditionValue, (uint16_t)5);
}

TEST(gpio, an_event_payload_only_reaches_the_pins_allowed)
{
    PinState state;

    pdiutil::vector<pdiutil::string> allowed;
    allowed.push_back("D5");

    pdiutil::string payload =
        "{\"D2\":[{\"cmp\":0,\"val\":1}],\"D5\":[{\"cmp\":2,\"val\":9}]}";
    __gpio_service.applyGpioEventJsonPayload((char *)payload.c_str(), (uint16_t)payload.size(), &allowed);

    ASSERT_FALSE(__gpio_service.m_gpio_config_copy.gpioHasEvents(2));
    ASSERT_TRUE(__gpio_service.m_gpio_config_copy.gpioHasEvents(5, LESS_THAN));
}

TEST(gpio, each_tick_drives_an_output_pin_to_its_value)
{
    PinState state;

    __gpio_service.m_gpio_config_copy.gpio_mode[1] = DIGITAL_WRITE;
    __gpio_service.m_gpio_config_copy.gpio_readings[1] = 1;
    __gpio_service.handleGpioOperations();
    ASSERT_EQ((int)__i_dvc_ctrl.gpioRead(DIGITAL_READ, 1), 1);

    __gpio_service.m_gpio_config_copy.gpio_readings[1] = 0;
    __gpio_service.handleGpioOperations();
    ASSERT_EQ((int)__i_dvc_ctrl.gpioRead(DIGITAL_READ, 1), 0);
}

TEST(gpio, each_tick_samples_an_input_pin)
{
    PinState state;

    __gpio_service.m_gpio_config_copy.gpio_mode[2] = DIGITAL_READ;
    __i_dvc_ctrl.gpioWrite(DIGITAL_WRITE, 2, 1);
    __gpio_service.handleGpioOperations();
    ASSERT_EQ(valueOf(2), (uint16_t)1);

    __i_dvc_ctrl.gpioWrite(DIGITAL_WRITE, 2, 0);
    __gpio_service.handleGpioOperations();
    ASSERT_EQ(valueOf(2), (uint16_t)0);
}

TEST(gpio, each_tick_samples_an_analog_input_from_its_own_channel)
{
    PinState state;

    __gpio_service.m_gpio_config_copy.gpio_mode[ANALOG0 + 1] = ANALOG_READ;
    __i_dvc_ctrl.setAnalogGpioValue(1, 612);
    __gpio_service.handleGpioOperations();

    ASSERT_EQ(valueOf(ANALOG0 + 1), (uint16_t)612);
    __i_dvc_ctrl.setAnalogGpioValue(1, 0);
}

TEST(gpio, an_analog_write_drives_its_digital_pin_not_an_analog_input)
{
    PinState state;

    __i_dvc_ctrl.setAnalogGpioValue(1, 0);
    __i_dvc_ctrl.gpioWrite(ANALOG_WRITE, 1, 700);

    ASSERT_EQ(__i_dvc_ctrl.gpioRead(ANALOG_READ, 1), (gpio_val_t)0);
    ASSERT_EQ(__i_dvc_ctrl.gpioRead(DIGITAL_READ, 1), (gpio_val_t)700);
    __i_dvc_ctrl.gpioWrite(DIGITAL_WRITE, 1, 0);
}

TEST(gpio, a_pin_switched_off_reads_zero_and_drops_its_events)
{
    PinState state;

    __gpio_service.m_gpio_config_copy.gpio_mode[6] = OFF;
    __gpio_service.m_gpio_config_copy.gpio_readings[6] = 1;
    __gpio_service.m_gpio_config_copy.updateGpioEvent(6, NO_CHANNEL, EQUAL, 1);

    __gpio_service.handleGpioOperations();

    ASSERT_EQ(valueOf(6), (uint16_t)0);
    ASSERT_FALSE(__gpio_service.m_gpio_config_copy.gpioHasEvents(6));
}

TEST(gpio, a_reserved_pin_is_treated_as_off_whatever_it_is_set_to)
{
    PinState state;

    __gpio_service.m_gpio_config_copy.gpio_mode[RESERVED] = DIGITAL_WRITE;
    __gpio_service.m_gpio_config_copy.gpio_readings[RESERVED] = 1;

    __gpio_service.handleGpioOperations();

    ASSERT_EQ(valueOf(RESERVED), (uint16_t)0);
}

TEST(gpio, a_met_condition_raises_its_event_and_an_unmet_one_does_not)
{
    PinState state;
    __gpio_event_track.event_gpio_pin = -1;

    __gpio_service.m_gpio_config_copy.gpio_mode[2] = DIGITAL_READ;
    __gpio_service.m_gpio_config_copy.updateGpioEvent(2, NO_CHANNEL, EQUAL, 1);

    __i_dvc_ctrl.gpioWrite(DIGITAL_WRITE, 2, 0);
    __gpio_service.handleGpioOperations();
    ASSERT_EQ((int)__gpio_event_track.event_gpio_pin, -1);

    __i_dvc_ctrl.gpioWrite(DIGITAL_WRITE, 2, 1);
    __gpio_service.handleGpioOperations();
    ASSERT_EQ((int)__gpio_event_track.event_gpio_pin, 2);

    __i_dvc_ctrl.gpioWrite(DIGITAL_WRITE, 2, 0);
    __gpio_event_track.event_gpio_pin = -1;
}

#if defined(ENABLE_HTTP_CLIENT) && !defined(ENABLE_GPIO_BASIC_ONLY)

namespace
{
    /**
     * Holds the shared http client with an async response not yet handed over,
     * and hands it over when asked or at the end of the test.
     */
    struct BusyClient
    {
        BusyClient() : m_delivered(false)
        {
            pditest::readyScheduler();
            Http_Client::GetStaticInstance()->GetAsync("http://127.0.0.1:1/v", [this](void *arg) {
                reinterpret_cast<Http_Client *>(arg)->End(true);
                m_delivered = true;
            });
        }

        ~BusyClient()
        {
            release();
        }

        bool release()
        {
            uint32_t began = __i_dvc_ctrl.millis_now();
            while (!m_delivered && (__i_dvc_ctrl.millis_now() - began) < 200)
            {
                __i_dvc_ctrl.wait(1);
                __task_scheduler.handle_tasks();
            }
            return m_delivered;
        }

        bool m_delivered;
    };

    int8_t eventIndexOf(uint8_t pin)
    {
        for (int8_t i = 0; i < MAX_GPIO_EVENTS; i++)
        {
            if (pin == __gpio_service.m_gpio_config_copy.gpio_events[i].gpioNumber) return i;
        }
        return -1;
    }
}

TEST(gpio, an_http_alert_raised_while_the_client_is_busy_waits_for_the_next_tick)
{
    PinState state;
    __gpio_event_track.event_gpio_pin = -1;
    __gpio_service.m_gpio_config_copy.gpio_mode[2] = DIGITAL_READ;
    __gpio_service.m_gpio_config_copy.updateGpioEvent(2, HTTP_SERVER, EQUAL, 1);
    int8_t index = eventIndexOf(2);
    ASSERT_TRUE(index >= 0);

    BusyClient busy;
    ASSERT_TRUE(HTTP_ASYNC_IDLE != Http_Client::GetStaticInstance()->GetAsyncState());

    __i_dvc_ctrl.gpioWrite(DIGITAL_WRITE, 2, 1);
    __gpio_service.handleGpioOperations();
    ASSERT_EQ((int)__gpio_service.m_pending_event_post, (int)index);

    __gpio_service.handleGpioOperations();
    ASSERT_EQ((int)__gpio_service.m_pending_event_post, (int)index);

    ASSERT_TRUE(busy.release());
    __gpio_service.handleGpioOperations();
    ASSERT_EQ((int)__gpio_service.m_pending_event_post, -1);

    __i_dvc_ctrl.gpioWrite(DIGITAL_WRITE, 2, 0);
    __gpio_event_track.event_gpio_pin = -1;
}

TEST(gpio, a_waiting_http_alert_is_dropped_when_its_rule_changes)
{
    PinState state;
    __gpio_event_track.event_gpio_pin = -1;
    __gpio_service.m_gpio_config_copy.gpio_mode[2] = DIGITAL_READ;
    __gpio_service.m_gpio_config_copy.updateGpioEvent(2, HTTP_SERVER, EQUAL, 1);

    BusyClient busy;
    __i_dvc_ctrl.gpioWrite(DIGITAL_WRITE, 2, 1);
    __gpio_service.handleGpioOperations();
    ASSERT_TRUE(__gpio_service.m_pending_event_post >= 0);

    __gpio_service.m_gpio_config_copy.updateGpioEvent(2, HTTP_SERVER, EQUAL, 0);
    __gpio_service.handleGpioOperations();
    ASSERT_EQ((int)__gpio_service.m_pending_event_post, -1);

    __i_dvc_ctrl.gpioWrite(DIGITAL_WRITE, 2, 0);
    __gpio_event_track.event_gpio_pin = -1;
}

#endif

TEST(gpio, a_change_from_a_payload_is_written_to_the_record_on_the_next_tick)
{
    pditest::mountedDb();
    PinState state;

    apply("{\"data\":{\"D7\":{\"mode\":1,\"val\":1}}}");
    __gpio_service.handleGpioOperations();

    gpio_config_table stored;
    ASSERT_TRUE(__database_service.get_gpio_config_table(&stored));
    ASSERT_EQ(stored.gpio_mode[7], (uint8_t)DIGITAL_WRITE);
    ASSERT_EQ(stored.gpio_readings[7], (uint16_t)1);

    stored.gpio_mode[7] = OFF;
    stored.gpio_readings[7] = 0;
    __database_service.set_gpio_config_table(&stored);
}

TEST(gpio, what_is_published_reads_back_to_the_same_pins)
{
    PinState state;

    apply("{\"data\":{\"D1\":{\"mode\":1,\"val\":1},\"D6\":{\"mode\":3,\"val\":250},"
          "\"A1\":{\"mode\":4,\"val\":300}}}");
    std::string out = published();

    __gpio_service.m_gpio_config_copy.clear();
    apply(out.c_str());

    ASSERT_EQ(modeOf(1), (uint8_t)DIGITAL_WRITE);
    ASSERT_EQ(valueOf(1), (uint16_t)1);
    ASSERT_EQ(modeOf(6), (uint8_t)DIGITAL_BLINK);
    ASSERT_EQ(valueOf(6), (uint16_t)250);
    ASSERT_EQ(modeOf(ANALOG0 + 1), (uint8_t)ANALOG_WRITE);
    ASSERT_EQ(valueOf(ANALOG0 + 1), (uint16_t)300);
}

#endif
