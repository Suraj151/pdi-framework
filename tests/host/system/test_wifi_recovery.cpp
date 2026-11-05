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
