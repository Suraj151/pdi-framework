/******************************** WiFi Tests **********************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 16th Aug 2026
******************************************************************************/

#include <interface/pdi.h>
#include <pditest.h>

/**
 * Return the shared radio to a known state, since it is a device global.
 */
static WiFiInterface *freshRadio()
{
    __i_wifi.disconnect(true);
    __i_wifi.softAPdisconnect(true);
    __i_wifi.clearScanResults();
    __i_wifi.clearCounters();
    __i_wifi.setAssociationAllowed(true);
    return &__i_wifi;
}

TEST(wifi, starts_disconnected)
{
    WiFiInterface *wifi = freshRadio();
    ASSERT_FALSE(wifi->isConnected());
    ASSERT_EQ(wifi->status(), CONN_STATUS_DISCONNECTED);
}

TEST(wifi, associates_when_allowed)
{
    WiFiInterface *wifi = freshRadio();
    char ssid[] = "pdiStack";
    char pass[] = "pdiStack@123";

    ASSERT_EQ(wifi->begin(ssid, pass), CONN_STATUS_CONNECTED);
    ASSERT_TRUE(wifi->isConnected());
    ASSERT_EQ(wifi->status(), CONN_STATUS_CONNECTED);
    ASSERT_STREQ(wifi->SSID().c_str(), "pdiStack");
}

TEST(wifi, association_can_be_made_to_fail)
{
    WiFiInterface *wifi = freshRadio();
    char ssid[] = "unreachable";

    wifi->setAssociationAllowed(false);
    ASSERT_EQ(wifi->begin(ssid), CONN_STATUS_CONNECTION_FAILED);
    ASSERT_FALSE(wifi->isConnected());
}

TEST(wifi, begin_attempts_are_counted)
{
    WiFiInterface *wifi = freshRadio();
    char ssid[] = "counted";

    wifi->setAssociationAllowed(false);
    wifi->begin(ssid);
    wifi->begin(ssid);
    wifi->begin(ssid);

    ASSERT_EQ(wifi->getBeginCount(), 3u);
}

TEST(wifi, a_link_can_be_dropped_underneath_the_stack)
{
    WiFiInterface *wifi = freshRadio();
    char ssid[] = "pdiStack";

    wifi->begin(ssid);
    ASSERT_TRUE(wifi->isConnected());

    wifi->dropConnection();
    ASSERT_FALSE(wifi->isConnected());
}

TEST(wifi, reconnect_succeeds_when_association_is_allowed)
{
    WiFiInterface *wifi = freshRadio();
    char ssid[] = "pdiStack";

    wifi->begin(ssid);
    wifi->dropConnection();

    ASSERT_TRUE(wifi->reconnect());
    ASSERT_TRUE(wifi->isConnected());
}

TEST(wifi, reconnect_fails_while_association_is_refused)
{
    WiFiInterface *wifi = freshRadio();
    char ssid[] = "pdiStack";

    wifi->begin(ssid);
    wifi->dropConnection();
    wifi->setAssociationAllowed(false);

    ASSERT_FALSE(wifi->reconnect());
    ASSERT_FALSE(wifi->isConnected());
}

TEST(wifi, disconnect_is_counted_and_clears_the_link)
{
    WiFiInterface *wifi = freshRadio();
    char ssid[] = "pdiStack";

    wifi->begin(ssid);
    ASSERT_TRUE(wifi->disconnect());

    ASSERT_FALSE(wifi->isConnected());
    ASSERT_EQ(wifi->getDisconnectCount(), 1u);
}

TEST(wifi, an_address_is_only_reported_while_connected)
{
    WiFiInterface *wifi = freshRadio();
    char ssid[] = "pdiStack";

    ASSERT_FALSE(wifi->localIP().isSet());

    wifi->begin(ssid);
    ASSERT_TRUE(wifi->localIP().isSet());
    ASSERT_EQ(wifi->localIP().ip4[0], (uint8_t)192);
}

TEST(wifi, the_station_address_can_be_chosen)
{
    WiFiInterface *wifi = freshRadio();
    char ssid[] = "pdiStack";

    wifi->setStationIp(ipaddress_t(10, 0, 0, 7));
    wifi->begin(ssid);

    ASSERT_EQ(wifi->localIP().ip4[0], (uint8_t)10);
    ASSERT_EQ(wifi->localIP().ip4[3], (uint8_t)7);
}

TEST(wifi, gateway_subnet_and_dns_are_reported)
{
    WiFiInterface *wifi = freshRadio();

    ASSERT_TRUE(wifi->subnetMask().isSet());
    ASSERT_TRUE(wifi->gatewayIP().isSet());
    ASSERT_TRUE(wifi->dnsIP().isSet());
}

TEST(wifi, config_overrides_the_addresses)
{
    WiFiInterface *wifi = freshRadio();
    ipaddress_t ip(172, 16, 0, 5);
    ipaddress_t gw(172, 16, 0, 1);
    ipaddress_t mask(255, 255, 0, 0);

    ASSERT_TRUE(wifi->config(ip, gw, mask));
    ASSERT_EQ(wifi->gatewayIP().ip4[0], (uint8_t)172);
    ASSERT_EQ(wifi->subnetMask().ip4[1], (uint8_t)255);
}

TEST(wifi, the_mac_address_is_stable_and_formatted)
{
    WiFiInterface *wifi = freshRadio();
    pdiutil::string mac = wifi->macAddress();

    ASSERT_EQ(mac.size(), (size_t)17);
    ASSERT_EQ(mac[2], ':');

    uint8_t raw[6] = {0};
    wifi->macAddress(raw);
    ASSERT_EQ(raw[0], (uint8_t)0x02);
}

TEST(wifi, the_station_mac_can_be_replaced)
{
    WiFiInterface *wifi = freshRadio();
    uint8_t wanted[6] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};
    uint8_t readback[6] = {0};

    wifi->setSTAmacAddress(wanted);
    wifi->macAddress(readback);
    ASSERT_MEMEQ(readback, wanted, 6);

    uint8_t original[6] = {0x02, 0x00, 0x00, 0xAB, 0xCD, 0xEF};
    wifi->setSTAmacAddress(original);
}

TEST(wifi, rssi_is_only_meaningful_while_connected)
{
    WiFiInterface *wifi = freshRadio();
    char ssid[] = "pdiStack";

    ASSERT_EQ(wifi->RSSI(), 0);
    wifi->begin(ssid);
    ASSERT_LT(wifi->RSSI(), 0);
}

TEST(wifi, mode_round_trips)
{
    WiFiInterface *wifi = freshRadio();

    ASSERT_TRUE(wifi->setMode(MOCK_WIFI_AP_STA));
    ASSERT_EQ(wifi->getMode(), (pdi_wifi_mode_t)MOCK_WIFI_AP_STA);
}

TEST(wifi, enabling_the_station_and_the_access_point_combines_the_mode)
{
    WiFiInterface *wifi = freshRadio();

    wifi->setMode(MOCK_WIFI_OFF);
    wifi->enableSTA(true);
    ASSERT_EQ(wifi->getMode(), (pdi_wifi_mode_t)MOCK_WIFI_STA);

    wifi->enableAP(true);
    ASSERT_EQ(wifi->getMode(), (pdi_wifi_mode_t)MOCK_WIFI_AP_STA);

    wifi->enableAP(false);
    ASSERT_EQ(wifi->getMode(), (pdi_wifi_mode_t)MOCK_WIFI_STA);
}

TEST(wifi, the_access_point_reports_its_address_and_mac)
{
    WiFiInterface *wifi = freshRadio();

    ASSERT_TRUE(wifi->softAP("pdiStack", "pdiStack@123"));
    ASSERT_TRUE(wifi->softAPIP().isSet());
    ASSERT_EQ(wifi->softAPmacAddress().size(), (size_t)17);

    wifi->softAPdisconnect(true);
}

TEST(wifi, the_access_point_address_can_be_configured)
{
    WiFiInterface *wifi = freshRadio();

    wifi->softAP("pdiStack");
    ASSERT_TRUE(wifi->softAPConfig(ipaddress_t(10, 10, 10, 1), ipaddress_t(10, 10, 10, 1),
                                   ipaddress_t(255, 255, 255, 0)));
    ASSERT_EQ(wifi->softAPIP().ip4[0], (uint8_t)10);

    wifi->softAPdisconnect(true);
}

TEST(wifi, connected_stations_are_only_reported_while_the_ap_is_up)
{
    WiFiInterface *wifi = freshRadio();
    pdiutil::vector<wifi_station_info_t> stations;

    ASSERT_FALSE(wifi->getApsConnectedStations(stations));

    wifi->softAP("pdiStack");
    ASSERT_TRUE(wifi->getApsConnectedStations(stations));

    wifi->softAPdisconnect(true);
}

TEST(wifi, a_scan_reports_the_staged_networks)
{
    WiFiInterface *wifi = freshRadio();

    ASSERT_EQ(wifi->scanNetworks(), (int8_t)0);

    ASSERT_TRUE(wifi->addScanResult("home", -40, 6));
    ASSERT_TRUE(wifi->addScanResult("office", -70, 11));

    ASSERT_EQ(wifi->scanNetworks(), (int8_t)2);
    ASSERT_STREQ(wifi->SSID(0).c_str(), "home");
    ASSERT_STREQ(wifi->SSID(1).c_str(), "office");
    ASSERT_EQ(wifi->RSSI(0), -40);
    ASSERT_EQ(wifi->RSSI(1), -70);
}

TEST(wifi, a_scan_entry_past_the_end_is_empty)
{
    WiFiInterface *wifi = freshRadio();
    wifi->addScanResult("only", -50, 1);

    ASSERT_STREQ(wifi->SSID(5).c_str(), "");
    ASSERT_EQ(wifi->RSSI(5), 0);
}

TEST(wifi, an_async_scan_reports_through_its_callback)
{
    static int reported = -1;
    reported = -1;

    WiFiInterface *wifi = freshRadio();
    wifi->addScanResult("one", -50, 1);
    wifi->addScanResult("two", -60, 6);

    wifi->scanNetworksAsync([](int count) { reported = count; });
    ASSERT_EQ(reported, 2);
}

TEST(wifi, a_scanned_bssid_can_be_looked_up_by_ssid)
{
    WiFiInterface *wifi = freshRadio();
    wifi->addScanResult("target", -45, 3);

    char ssid[] = "target";
    uint8_t bssid[6] = {0};
    ASSERT_TRUE(wifi->get_bssid_within_scanned_nw_ignoring_connected_stations(ssid, bssid, nullptr, 4));
    ASSERT_EQ(bssid[0], (uint8_t)0x0A);
}

TEST(wifi, a_bssid_lookup_skips_the_ignored_one)
{
    WiFiInterface *wifi = freshRadio();
    wifi->addScanResult("target", -45, 3);

    char ssid[] = "target";
    uint8_t bssid[6] = {0};
    uint8_t ignore[6] = {0};
    wifi->get_bssid_within_scanned_nw_ignoring_connected_stations(ssid, ignore, nullptr, 4);

    ASSERT_FALSE(wifi->get_bssid_within_scanned_nw_ignoring_connected_stations(ssid, bssid, ignore, 4));
}

TEST(wifi, an_unknown_ssid_has_no_bssid)
{
    WiFiInterface *wifi = freshRadio();
    wifi->addScanResult("present", -45, 3);

    char ssid[] = "absent";
    uint8_t bssid[6] = {0};
    ASSERT_FALSE(wifi->get_bssid_within_scanned_nw_ignoring_connected_stations(ssid, bssid, nullptr, 4));
}

TEST(wifi, a_name_resolves_through_the_host_resolver)
{
    WiFiInterface *wifi = freshRadio();
    ipaddress_t resolved;

    ASSERT_EQ(wifi->hostByName("localhost", resolved, 1000), 1);
    ASSERT_EQ(resolved.ip4[0], (uint8_t)127);
}

TEST(wifi, an_unresolvable_name_reports_failure)
{
    WiFiInterface *wifi = freshRadio();
    ipaddress_t resolved;

    ASSERT_EQ(wifi->hostByName("this-host-should-not-exist.invalid", resolved, 1000), 0);
}

TEST(wifi, host_by_name_rejects_null)
{
    WiFiInterface *wifi = freshRadio();
    ipaddress_t resolved;

    ASSERT_EQ(wifi->hostByName(nullptr, resolved, 1000), 0);
}
