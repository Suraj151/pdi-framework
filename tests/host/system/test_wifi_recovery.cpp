/************************ WiFi Recovery Tests *********************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

How the station recovers from a lost or unreachable network, and when it gives
up and restarts the device.

Author          : Suraj I.
created Date    : 2nd Nov 2026
******************************************************************************/

#include <interface/pdi.h>
#include <service_provider/network/WiFiServiceProvider.h>
#include <pditest.h>

/**
 * The service brought up once on a radio the test drives, with the clock
 * detached so minutes of outage pass in a loop.
 */
static void wifiUp()
{
    static bool up = false;
    if (!up)
    {
        up = true;
        __wifi_service.startService(&__i_wifi);
    }
    __i_dvc_ctrl.useVirtualClock(true);
    __i_dvc_ctrl.clearControlCounters();
}

/**
 * Runs the connectivity check every five seconds for the given span.
 */
static void outageFor(uint32_t seconds)
{
    for (uint32_t s = 0; s < seconds; s += 5)
    {
        __i_dvc_ctrl.advanceVirtualClock(5ULL * 1000000ULL);
        __wifi_service.handleWiFiConnectivity();
    }
}

TEST(wifirecovery, a_station_that_never_joined_keeps_retrying_and_never_restarts_the_device)
{
    wifiUp();
    __status_wifi.wifi_joined_since_boot = false;
    __i_wifi.setAssociationAllowed(false);
    __i_wifi.dropConnection();
    __i_wifi.clearCounters();

    outageFor(600);

    ASSERT_EQ(__i_dvc_ctrl.getResetCount(), 0u);
    ASSERT_TRUE(__i_wifi.getBeginCount() > 0u);
    __i_dvc_ctrl.useVirtualClock(false);
}

TEST(wifirecovery, a_station_that_joined_and_then_lost_its_network_restarts_the_device_after_five_minutes)
{
    wifiUp();
    __i_wifi.setAssociationAllowed(true);
    __i_wifi.setStationIp(ipaddress_t(10, 0, 0, 7));
    char ssid[] = "pdiStack";
    __i_wifi.begin(ssid);
    __wifi_service.handleWiFiConnectivity();
    ASSERT_TRUE(__status_wifi.wifi_joined_since_boot);

    __i_wifi.setAssociationAllowed(false);
    __i_wifi.dropConnection();
    outageFor(240);
    ASSERT_EQ(__i_dvc_ctrl.getResetCount(), 0u);

    outageFor(120);
    ASSERT_TRUE(__i_dvc_ctrl.getResetCount() > 0u);
    __i_dvc_ctrl.useVirtualClock(false);
}

#ifdef ENABLE_SYSLOG_SERVICE

namespace
{
    uint16_t s_lost_lines = 0;
    uint16_t s_back_lines = 0;
    uint16_t s_restart_lines = 0;
    uint16_t s_wifi_lines = 0;
    logger_type_t s_restart_level = INFO_LOG;

    /** Sorts the syslog lines the station writes about its network. */
    void countWifiLines(logger_type_t level, const char *line, uint16_t len)
    {
        pdiutil::string text(line, len);
        if (pdiutil::string::npos == text.find("WiFi")) return;
        s_wifi_lines++;
        if (pdiutil::string::npos != text.find("lost")) s_lost_lines++;
        if (pdiutil::string::npos != text.find("back after")) s_back_lines++;
        if (pdiutil::string::npos != text.find("restarting")) {
            s_restart_lines++;
            s_restart_level = level;
        }
    }

    /** Joins the test network and lets the service see the station up. */
    void joinNetwork()
    {
        __i_wifi.setAssociationAllowed(true);
        __i_wifi.setStationIp(ipaddress_t(10, 0, 0, 7));
        char ssid[] = "pdiStack";
        __i_wifi.begin(ssid);
        __wifi_service.handleWiFiConnectivity();
    }

    /** Clears the counters and routes the syslog to them. */
    void watchSyslog()
    {
        s_lost_lines = s_back_lines = s_restart_lines = s_wifi_lines = 0;
        s_restart_level = INFO_LOG;
        __log_manager.setSyslogSink(&countWifiLines);
    }
}

TEST(wifirecovery, a_lost_network_and_its_return_each_leave_one_syslog_line)
{
    wifiUp();
    joinNetwork();
    watchSyslog();

    __i_wifi.setAssociationAllowed(false);
    __i_wifi.dropConnection();
    outageFor(60);
    uint16_t lostWhileDown = s_lost_lines;
    uint16_t linesWhileDown = s_wifi_lines;

    joinNetwork();
    __wifi_service.handleWiFiConnectivity();
    uint16_t back = s_back_lines;

    __log_manager.setSyslogSink(nullptr);
    __i_dvc_ctrl.useVirtualClock(false);

    ASSERT_EQ(lostWhileDown, (uint16_t)1);
    ASSERT_EQ(linesWhileDown, (uint16_t)1);
    ASSERT_EQ(back, (uint16_t)1);
}

TEST(wifirecovery, a_restart_for_want_of_a_network_is_in_the_syslog_before_it_happens)
{
    wifiUp();
    joinNetwork();
    watchSyslog();

    __i_wifi.setAssociationAllowed(false);
    __i_wifi.dropConnection();
    uint32_t resetsBefore = __i_dvc_ctrl.getResetCount();
    outageFor(360);
    uint32_t resets = __i_dvc_ctrl.getResetCount() - resetsBefore;
    uint16_t restartLines = s_restart_lines;
    logger_type_t level = s_restart_level;

    __log_manager.setSyslogSink(nullptr);
    joinNetwork();
    __i_dvc_ctrl.useVirtualClock(false);

    ASSERT_TRUE(resets > 0u);
    ASSERT_EQ(restartLines, (uint16_t)resets);
    ASSERT_EQ((int)level, (int)ERROR_LOG);
}

TEST(wifirecovery, a_station_that_never_joined_writes_no_lost_line)
{
    wifiUp();
    __status_wifi.wifi_joined_since_boot = false;
    __status_wifi.wifi_connected = false;
    watchSyslog();

    __i_wifi.setAssociationAllowed(false);
    __i_wifi.dropConnection();
    outageFor(60);
    uint16_t lines = s_wifi_lines;

    __log_manager.setSyslogSink(nullptr);
    joinNetwork();
    __i_dvc_ctrl.useVirtualClock(false);

    ASSERT_EQ(lines, (uint16_t)0);
}

#endif
