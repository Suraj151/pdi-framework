/****************************** Ping Interface *******************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 16th Aug 2026
******************************************************************************/

#include "PingInterface.h"
#include "DeviceControlInterface.h"

mock_ping_host_t::mock_ping_host_t() : m_ip(0),
                                       m_reachable(false),
                                       m_rtt_ms(MOCKDEVICE_PING_DEFAULT_RTT_MS)
{
}

PingInterface::PingInterface() : m_wifi(nullptr),
                                 m_default_reachable(true),
                                 m_default_rtt_ms(MOCKDEVICE_PING_DEFAULT_RTT_MS),
                                 m_rtt_step_ms(0),
                                 m_drop_mask(0),
                                 m_interval_ms(MOCKDEVICE_PING_DEFAULT_INTERVAL_MS),
                                 m_timeout_ms(MOCKDEVICE_PING_DEFAULT_TIMEOUT_MS),
                                 m_stats({0, 0, 0, 0, 0}),
                                 m_sum_ms(0),
                                 m_seq(0),
                                 m_count(0),
                                 m_running(false),
                                 m_complete(false),
                                 m_host_resp(false),
                                 m_pkt_cb(nullptr),
                                 m_target_reachable(false),
                                 m_target_rtt_ms(MOCKDEVICE_PING_DEFAULT_RTT_MS),
                                 m_next_due_ms(0)
{
}

PingInterface::~PingInterface()
{
    this->m_wifi = nullptr;
    this->m_pkt_cb = nullptr;
}

void PingInterface::init_ping(iWiFiInterface *_wifi)
{
    this->m_wifi = _wifi;
}

bool PingInterface::ping(const ipaddress_t &target, uint16_t count, CallBackVoidPointerArgFn on_packet)
{
    ipaddress_t _target = target;
    if (!_target.isSet())
    {
        return false;
    }

    if (0 == count)
    {
        return false;
    }

    m_target_reachable = m_default_reachable;
    m_target_rtt_ms = m_default_rtt_ms;

    uint32_t ip = (uint32_t)_target;
    for (uint8_t i = 0; i < MOCKDEVICE_PING_MAX_HOSTS; i++)
    {
        if (0 != m_hosts[i].m_ip && ip == m_hosts[i].m_ip)
        {
            m_target_reachable = m_hosts[i].m_reachable;
            m_target_rtt_ms = m_hosts[i].m_rtt_ms;
            break;
        }
    }

    memset(&m_stats, 0, sizeof(m_stats));
    m_sum_ms = 0;
    m_seq = 0;
    m_count = count;
    m_running = true;
    m_complete = false;
    m_host_resp = false;
    m_pkt_cb = on_packet;
    m_next_due_ms = __i_dvc_ctrl.millis_now() +
                    (m_target_reachable ? m_target_rtt_ms : m_timeout_ms);

    return true;
}

bool PingInterface::isPingComplete(void)
{
    return m_complete;
}

bool PingInterface::isPingBusy(void)
{
    return m_running && !m_complete;
}

bool PingInterface::isHostRespondingToPing(void)
{
    return m_host_resp;
}

const ping_stats_t &PingInterface::getPingStats(void)
{
    return m_stats;
}

void PingInterface::setHostReachable(const ipaddress_t &target, bool reachable, uint32_t rtt_ms)
{
    ipaddress_t _target = target;
    uint32_t ip = (uint32_t)_target;
    if (0 == ip)
    {
        return;
    }

    for (uint8_t i = 0; i < MOCKDEVICE_PING_MAX_HOSTS; i++)
    {
        if (ip == m_hosts[i].m_ip || 0 == m_hosts[i].m_ip)
        {
            m_hosts[i].m_ip = ip;
            m_hosts[i].m_reachable = reachable;
            m_hosts[i].m_rtt_ms = rtt_ms;
            return;
        }
    }
}

void PingInterface::setDefaultReachable(bool reachable, uint32_t rtt_ms)
{
    m_default_reachable = reachable;
    m_default_rtt_ms = rtt_ms;
}

void PingInterface::setRttStep(int32_t step_ms)
{
    m_rtt_step_ms = step_ms;
}

void PingInterface::dropPacket(uint16_t seqno)
{
    if (0 == seqno || seqno > 32)
    {
        return;
    }

    m_drop_mask |= (1u << (seqno - 1));
}

void PingInterface::setPacketInterval(uint32_t interval_ms)
{
    m_interval_ms = interval_ms;
}

void PingInterface::setTimeout(uint32_t timeout_ms)
{
    m_timeout_ms = timeout_ms;
}

void PingInterface::clearScript(void)
{
    for (uint8_t i = 0; i < MOCKDEVICE_PING_MAX_HOSTS; i++)
    {
        m_hosts[i] = mock_ping_host_t();
    }

    m_default_reachable = true;
    m_default_rtt_ms = MOCKDEVICE_PING_DEFAULT_RTT_MS;
    m_rtt_step_ms = 0;
    m_drop_mask = 0;
    m_interval_ms = MOCKDEVICE_PING_DEFAULT_INTERVAL_MS;
    m_timeout_ms = MOCKDEVICE_PING_DEFAULT_TIMEOUT_MS;
}

uint16_t PingInterface::service(void)
{
    uint16_t reported = 0;

    while (m_running)
    {
        uint32_t now = __i_dvc_ctrl.millis_now();
        if ((int32_t)(now - m_next_due_ms) < 0)
        {
            break;
        }

        m_seq++;
        m_stats.m_transmitted = m_seq;

        bool dropped = (m_seq <= 32) && (0 != (m_drop_mask & (1u << (m_seq - 1))));
        bool replied = m_target_reachable && !dropped;

        uint32_t rtt = 0;
        if (replied)
        {
            int32_t scaled = (int32_t)m_target_rtt_ms + (m_rtt_step_ms * (int32_t)(m_seq - 1));
            rtt = (scaled > 0) ? (uint32_t)scaled : 1;

            m_stats.m_received++;
            if (1 == m_stats.m_received || rtt < m_stats.m_min_ms)
            {
                m_stats.m_min_ms = rtt;
            }
            if (rtt > m_stats.m_max_ms)
            {
                m_stats.m_max_ms = rtt;
            }
            m_sum_ms += rtt;
            m_stats.m_avg_ms = m_sum_ms / m_stats.m_received;
        }

        m_host_resp = replied;
        reported++;

        if (m_seq >= m_count)
        {
            m_running = false;
            m_complete = true;
        }
        else
        {
            m_next_due_ms = m_next_due_ms + m_interval_ms +
                            (m_target_reachable ? m_target_rtt_ms : m_timeout_ms);
        }

        if (m_pkt_cb)
        {
            ping_pkt_t pkt = {m_seq, replied, rtt};
            m_pkt_cb((void *)&pkt);
        }
    }

    return reported;
}

PingInterface __i_ping;
