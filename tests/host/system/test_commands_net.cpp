/************************ Network Command Tests *******************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

The commands that reach the radio, the resolver, the echo engine and the clock.

Author          : Suraj I.
created Date    : 16th Aug 2026
******************************************************************************/

#include <ShellHarness.h>
#include <service_provider/network/NameResolver.h>
#include <service_provider/network/WiFiServiceProvider.h>
#include <pditest.h>

using pditest::saw;

/**
 * A resolver with a known /etc/hosts, and a radio and echo engine a test drives
 * rather than a network.
 *
 * The WiFi service is brought up the way boot brings it up. `net` reads the
 * radio through it and does not check whether it was ever handed one, so a
 * command run against an uninitialised service would dereference null.
 */
static pditest::Shell *networked(pditest::Shell &shell)
{
    static bool wifiup = false;
    if (!wifiup)
    {
        wifiup = true;
        __wifi_service.initService(&__i_wifi);
    }

    NameResolver::ensureHostsFile();
    __i_ping.clearScript();
    __i_ping.init_ping(&__i_wifi);
    __i_ping.setPacketInterval(1);
    __i_ping.setTimeout(2);
    return &shell;
}

/* ------------------------------------------------------------------- host */

TEST(cmdnet, host_resolves_an_address_literal_to_itself)
{
    pditest::Shell shell;
    networked(shell);

    ASSERT_TRUE(saw(shell.run("host 192.168.4.7"), "192.168.4.7"));
}

TEST(cmdnet, host_resolves_a_name_from_the_hosts_file)
{
    pditest::Shell shell;
    networked(shell);
    __i_fs.writeFile(HOSTS_FILE_PATH, "10.1.2.3 fixture.local\n", 23, true);

    ASSERT_TRUE(saw(shell.run("host fixture.local"), "10.1.2.3"));
}

TEST(cmdnet, host_reports_a_name_it_cannot_resolve)
{
    pditest::Shell shell;
    networked(shell);

    std::string out = shell.run("host no.such.name.invalid");
    ASSERT_FALSE(saw(out, "192.168"));
    ASSERT_EQ(shell.result(), CMD_RESULT_OK);
}

TEST(cmdnet, host_without_a_name_asks_for_one)
{
    pditest::Shell shell;
    networked(shell);

    shell.run("host");
    ASSERT_NE(shell.result(), CMD_RESULT_OK);
}

/* ------------------------------------------------------------------- ping */

TEST(cmdnet, ping_reports_every_reply)
{
    pditest::Shell shell;
    networked(shell);
    ipaddress_t target(192, 168, 4, 9);
    __i_ping.setHostReachable(target, true, 5);

    std::string out = shell.run("ping 192.168.4.9 3");

    ASSERT_TRUE(saw(out, "PING"));
    ASSERT_TRUE(saw(out, "3 transmitted"));
    ASSERT_TRUE(saw(out, "3 received"));
    ASSERT_TRUE(saw(out, "0% loss"));
}

TEST(cmdnet, ping_reports_a_host_that_never_answers)
{
    pditest::Shell shell;
    networked(shell);
    ipaddress_t target(10, 9, 9, 9);
    __i_ping.setHostReachable(target, false);

    std::string out = shell.run("ping 10.9.9.9 2");

    ASSERT_TRUE(saw(out, "2 transmitted"));
    ASSERT_TRUE(saw(out, "0 received"));
    ASSERT_TRUE(saw(out, "100% loss"));
}

TEST(cmdnet, ping_reports_partial_loss)
{
    pditest::Shell shell;
    networked(shell);
    ipaddress_t target(192, 168, 4, 11);
    __i_ping.setHostReachable(target, true, 5);
    __i_ping.dropPacket(2);

    std::string out = shell.run("ping 192.168.4.11 4");

    ASSERT_TRUE(saw(out, "4 transmitted"));
    ASSERT_TRUE(saw(out, "3 received"));
    ASSERT_TRUE(saw(out, "25% loss"));
}

TEST(cmdnet, ping_summarises_the_round_trip_times)
{
    pditest::Shell shell;
    networked(shell);
    ipaddress_t target(192, 168, 4, 12);
    __i_ping.setHostReachable(target, true, 10);
    __i_ping.setRttStep(10);

    std::string out = shell.run("ping 192.168.4.12 3");

    ASSERT_TRUE(saw(out, "rtt min/avg/max"));
    ASSERT_TRUE(saw(out, "10/20/30"));
}

TEST(cmdnet, ping_defaults_to_four_packets)
{
    pditest::Shell shell;
    networked(shell);
    ipaddress_t target(192, 168, 4, 13);
    __i_ping.setHostReachable(target, true, 3);

    ASSERT_TRUE(saw(shell.run("ping 192.168.4.13"), "4 transmitted"));
}

TEST(cmdnet, ping_caps_the_count)
{
    pditest::Shell shell;
    networked(shell);
    ipaddress_t target(192, 168, 4, 14);
    __i_ping.setHostReachable(target, true, 1);

    std::string out = shell.run("ping 192.168.4.14 99");
    ASSERT_TRUE(saw(out, "10 transmitted"));
}

TEST(cmdnet, ping_of_a_name_it_cannot_resolve_says_so)
{
    pditest::Shell shell;
    networked(shell);

    ASSERT_TRUE(saw(shell.run("ping no.such.name.invalid"), "cannot resolve"));
}

/* -------------------------------------------------------------------- net */

TEST(cmdnet, net_ip_reports_the_addresses)
{
    pditest::Shell shell;
    networked(shell);
    char ssid[] = "pdiStack";
    __i_wifi.begin(ssid);

    std::string out = shell.run("net ip");
    ASSERT_TRUE(out.length() > 0);
    ASSERT_EQ(shell.result(), CMD_RESULT_OK);
}

TEST(cmdnet, net_scansta_lists_the_staged_networks)
{
    pditest::Shell shell;
    networked(shell);
    __i_wifi.clearScanResults();
    __i_wifi.addScanResult("visible-net", -55, 6);

    ASSERT_TRUE(saw(shell.run("net scansta"), "visible-net"));

    __i_wifi.clearScanResults();
}

TEST(cmdnet, net_without_a_subcommand_is_not_accepted)
{
    pditest::Shell shell;
    networked(shell);

    shell.run("net");
    ASSERT_NE(shell.result(), CMD_RESULT_OK);
}

/* ------------------------------------------------------------ date, tdctl */

TEST(cmdnet, date_prints_a_time)
{
    pditest::Shell shell;
    networked(shell);
    __i_ntp.set_ntp_time(1755300000);

    std::string out = shell.run("date");
    ASSERT_TRUE(out.length() > 0);
    ASSERT_EQ(shell.result(), CMD_RESULT_OK);
}

TEST(cmdnet, date_sets_the_clock_from_an_epoch)
{
    pditest::Shell shell;
    networked(shell);

    shell.run("date -s 1700000000");
    ASSERT_TRUE(__i_ntp.get_ntp_time() >= 1700000000);
}

TEST(cmdnet, date_in_utc_differs_from_local_when_the_zone_does)
{
    pditest::Shell shell;
    networked(shell);
    __i_ntp.set_ntp_time(1755300000);

    std::string local = shell.run("date");
    std::string utc = shell.run("date -u");

    ASSERT_TRUE(local.length() > 0);
    ASSERT_TRUE(utc.length() > 0);
}

TEST(cmdnet, tdctl_reports_the_clock_status)
{
    pditest::Shell shell;
    networked(shell);
    __i_ntp.set_ntp_time(1755300000);

    std::string out = shell.run("tdctl");
    ASSERT_TRUE(out.length() > 0);
    ASSERT_EQ(shell.result(), CMD_RESULT_OK);
}
