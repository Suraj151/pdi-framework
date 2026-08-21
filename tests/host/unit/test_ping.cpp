/******************************** Ping Tests **********************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 16th Aug 2026
******************************************************************************/

#include <interface/pdi.h>
#include <pditest.h>

/**
 * Return the shared engine to a known state, since it is a device global.
 */
static PingInterface *freshPing()
{
    __i_ping.clearScript();
    __i_ping.init_ping(&__i_wifi);
    return &__i_ping;
}

/**
 * Drive the run to completion the way a caller does, without letting a broken
 * completion flag hang the suite.
 */
static bool drain(PingInterface *ping)
{
    for (uint16_t spins = 0; spins < 2000 && !ping->isPingComplete(); spins++)
    {
        __i_dvc_ctrl.wait(5);
        __i_dvc_ctrl.yield();
    }

    return ping->isPingComplete();
}

TEST(ping, an_unset_address_is_refused)
{
    PingInterface *ping = freshPing();
    ipaddress_t nowhere;

    ASSERT_FALSE(ping->ping(nowhere, 1));
}

TEST(ping, a_zero_count_is_refused)
{
    PingInterface *ping = freshPing();
    ipaddress_t host(192, 168, 0, 10);

    ASSERT_FALSE(ping->ping(host, 0));
}

TEST(ping, a_reachable_host_answers_every_packet)
{
    PingInterface *ping = freshPing();
    ipaddress_t host(192, 168, 0, 10);
    ping->setHostReachable(host, true, 12);

    ASSERT_TRUE(ping->ping(host, 4));
    ASSERT_TRUE(drain(ping));

    const ping_stats_t &st = ping->getPingStats();
    ASSERT_EQ(st.m_transmitted, 4);
    ASSERT_EQ(st.m_received, 4);
    ASSERT_TRUE(ping->isHostRespondingToPing());
}

TEST(ping, a_run_is_not_complete_before_it_is_serviced)
{
    PingInterface *ping = freshPing();
    ipaddress_t host(192, 168, 0, 10);

    ASSERT_TRUE(ping->ping(host, 3));
    ASSERT_FALSE(ping->isPingComplete());
    ASSERT_EQ(ping->getPingStats().m_transmitted, 0);
}

TEST(ping, an_unreachable_host_loses_every_packet)
{
    PingInterface *ping = freshPing();
    ipaddress_t host(10, 20, 30, 40);
    ping->setHostReachable(host, false);
    ping->setTimeout(5);

    ASSERT_TRUE(ping->ping(host, 3));
    ASSERT_TRUE(drain(ping));

    const ping_stats_t &st = ping->getPingStats();
    ASSERT_EQ(st.m_transmitted, 3);
    ASSERT_EQ(st.m_received, 0);
    ASSERT_FALSE(ping->isHostRespondingToPing());
}

TEST(ping, the_default_policy_covers_an_unscripted_address)
{
    PingInterface *ping = freshPing();
    ipaddress_t host(8, 8, 8, 8);
    ping->setDefaultReachable(false);
    ping->setTimeout(5);

    ASSERT_TRUE(ping->ping(host, 2));
    ASSERT_TRUE(drain(ping));
    ASSERT_EQ(ping->getPingStats().m_received, 0);
}

TEST(ping, a_scripted_host_overrides_the_default_policy)
{
    PingInterface *ping = freshPing();
    ipaddress_t reachable(192, 168, 0, 10);
    ipaddress_t other(192, 168, 0, 11);
    ping->setDefaultReachable(false);
    ping->setTimeout(5);
    ping->setHostReachable(reachable, true, 4);

    ASSERT_TRUE(ping->ping(reachable, 2));
    ASSERT_TRUE(drain(ping));
    ASSERT_EQ(ping->getPingStats().m_received, 2);

    ASSERT_TRUE(ping->ping(other, 2));
    ASSERT_TRUE(drain(ping));
    ASSERT_EQ(ping->getPingStats().m_received, 0);
}

TEST(ping, a_dropped_sequence_number_times_out_on_a_reachable_host)
{
    PingInterface *ping = freshPing();
    ipaddress_t host(192, 168, 0, 10);
    ping->setHostReachable(host, true, 4);
    ping->dropPacket(2);

    ASSERT_TRUE(ping->ping(host, 4));
    ASSERT_TRUE(drain(ping));

    const ping_stats_t &st = ping->getPingStats();
    ASSERT_EQ(st.m_transmitted, 4);
    ASSERT_EQ(st.m_received, 3);
}

TEST(ping, round_trip_times_are_summarised)
{
    PingInterface *ping = freshPing();
    ipaddress_t host(192, 168, 0, 10);
    ping->setHostReachable(host, true, 10);
    ping->setRttStep(10);

    ASSERT_TRUE(ping->ping(host, 4));
    ASSERT_TRUE(drain(ping));

    const ping_stats_t &st = ping->getPingStats();
    ASSERT_EQ(st.m_min_ms, 10u);
    ASSERT_EQ(st.m_max_ms, 40u);
    ASSERT_EQ(st.m_avg_ms, 25u);
}

TEST(ping, a_lost_packet_does_not_enter_the_time_summary)
{
    PingInterface *ping = freshPing();
    ipaddress_t host(192, 168, 0, 10);
    ping->setHostReachable(host, true, 20);
    ping->dropPacket(1);

    ASSERT_TRUE(ping->ping(host, 2));
    ASSERT_TRUE(drain(ping));

    const ping_stats_t &st = ping->getPingStats();
    ASSERT_EQ(st.m_received, 1);
    ASSERT_EQ(st.m_min_ms, 20u);
    ASSERT_EQ(st.m_max_ms, 20u);
    ASSERT_EQ(st.m_avg_ms, 20u);
}

TEST(ping, every_packet_is_reported_to_the_callback)
{
    PingInterface *ping = freshPing();
    ipaddress_t host(192, 168, 0, 10);
    ping->setHostReachable(host, true, 6);

    static uint16_t seen;
    static uint16_t lastseq;
    static bool inorder;
    seen = 0;
    lastseq = 0;
    inorder = true;

    ASSERT_TRUE(ping->ping(host, 5, [](void *arg) {
        ping_pkt_t *p = (ping_pkt_t *)arg;
        if (nullptr == p)
        {
            return;
        }
        if (p->m_seqno != (uint16_t)(lastseq + 1))
        {
            inorder = false;
        }
        lastseq = p->m_seqno;
        seen++;
    }));
    ASSERT_TRUE(drain(ping));

    ASSERT_EQ(seen, 5);
    ASSERT_TRUE(inorder);
}

TEST(ping, the_callback_sees_a_loss_as_it_happens)
{
    PingInterface *ping = freshPing();
    ipaddress_t host(192, 168, 0, 10);
    ping->setHostReachable(host, true, 6);
    ping->dropPacket(2);

    static uint16_t lostseq;
    static uint16_t lostcount;
    lostseq = 0;
    lostcount = 0;

    ASSERT_TRUE(ping->ping(host, 3, [](void *arg) {
        ping_pkt_t *p = (ping_pkt_t *)arg;
        if (nullptr == p || p->m_replied)
        {
            return;
        }
        lostseq = p->m_seqno;
        lostcount++;
    }));
    ASSERT_TRUE(drain(ping));

    ASSERT_EQ(lostcount, 1);
    ASSERT_EQ(lostseq, 2);
}

TEST(ping, a_reply_carries_its_own_round_trip)
{
    PingInterface *ping = freshPing();
    ipaddress_t host(192, 168, 0, 10);
    ping->setHostReachable(host, true, 15);

    static uint32_t rtt;
    static bool replied;
    rtt = 0;
    replied = false;

    ASSERT_TRUE(ping->ping(host, 1, [](void *arg) {
        ping_pkt_t *p = (ping_pkt_t *)arg;
        if (nullptr == p)
        {
            return;
        }
        replied = p->m_replied;
        rtt = p->m_rtt_ms;
    }));
    ASSERT_TRUE(drain(ping));

    ASSERT_TRUE(replied);
    ASSERT_EQ(rtt, 15u);
}

TEST(ping, a_lost_packet_reports_no_round_trip)
{
    PingInterface *ping = freshPing();
    ipaddress_t host(10, 20, 30, 40);
    ping->setHostReachable(host, false);
    ping->setTimeout(5);

    static uint32_t rtt;
    static bool replied;
    rtt = 0xFFFFFFFFu;
    replied = true;

    ASSERT_TRUE(ping->ping(host, 1, [](void *arg) {
        ping_pkt_t *p = (ping_pkt_t *)arg;
        if (nullptr == p)
        {
            return;
        }
        replied = p->m_replied;
        rtt = p->m_rtt_ms;
    }));
    ASSERT_TRUE(drain(ping));

    ASSERT_FALSE(replied);
    ASSERT_EQ(rtt, 0u);
}

TEST(ping, a_run_can_be_started_without_a_callback)
{
    PingInterface *ping = freshPing();
    ipaddress_t host(192, 168, 0, 10);
    ping->setHostReachable(host, true, 3);

    ASSERT_TRUE(ping->ping(host, 2, nullptr));
    ASSERT_TRUE(drain(ping));
    ASSERT_EQ(ping->getPingStats().m_received, 2);
}

TEST(ping, a_second_run_starts_from_clean_statistics)
{
    PingInterface *ping = freshPing();
    ipaddress_t host(192, 168, 0, 10);
    ping->setHostReachable(host, true, 7);

    ASSERT_TRUE(ping->ping(host, 4));
    ASSERT_TRUE(drain(ping));
    ASSERT_EQ(ping->getPingStats().m_transmitted, 4);

    ASSERT_TRUE(ping->ping(host, 1));
    ASSERT_TRUE(drain(ping));

    const ping_stats_t &st = ping->getPingStats();
    ASSERT_EQ(st.m_transmitted, 1);
    ASSERT_EQ(st.m_received, 1);
}

TEST(ping, the_last_packet_decides_whether_the_host_is_responding)
{
    PingInterface *ping = freshPing();
    ipaddress_t host(192, 168, 0, 10);
    ping->setHostReachable(host, true, 4);
    ping->dropPacket(3);

    ASSERT_TRUE(ping->ping(host, 3));
    ASSERT_TRUE(drain(ping));
    ASSERT_FALSE(ping->isHostRespondingToPing());
}

TEST(ping, the_script_can_be_cleared_back_to_reachable)
{
    PingInterface *ping = freshPing();
    ipaddress_t host(192, 168, 0, 10);
    ping->setHostReachable(host, false);
    ping->clearScript();

    ASSERT_TRUE(ping->ping(host, 2));
    ASSERT_TRUE(drain(ping));
    ASSERT_EQ(ping->getPingStats().m_received, 2);
}

TEST(ping, packets_are_reported_one_at_a_time_rather_than_all_at_once)
{
    PingInterface *ping = freshPing();
    ipaddress_t host(192, 168, 0, 10);
    ping->setHostReachable(host, true, 20);
    ping->setPacketInterval(20);

    ASSERT_TRUE(ping->ping(host, 3));

    __i_dvc_ctrl.wait(25);
    ASSERT_EQ(ping->service(), 1);
    ASSERT_FALSE(ping->isPingComplete());

    ASSERT_TRUE(drain(ping));
    ASSERT_EQ(ping->getPingStats().m_transmitted, 3);
}

TEST(ping, the_serve_loop_advances_a_run)
{
    PingInterface *ping = freshPing();
    ipaddress_t host(192, 168, 0, 10);
    ping->setHostReachable(host, true, 2);
    ping->setPacketInterval(1);

    ASSERT_TRUE(ping->ping(host, 2));

    for (uint16_t spins = 0; spins < 2000 && !ping->isPingComplete(); spins++)
    {
        __i_dvc_ctrl.wait(2);
        __i_dvc_ctrl.handleEvents();
    }

    ASSERT_TRUE(ping->isPingComplete());
    ASSERT_EQ(ping->getPingStats().m_received, 2);
}
