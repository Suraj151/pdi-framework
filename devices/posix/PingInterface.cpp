/****************************** Ping Interface *******************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 16th Aug 2026
******************************************************************************/

#include "PingInterface.h"
#include "DeviceControlInterface.h"

#ifndef MOCK_DEVICE_TEST
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

namespace {

  const uint8_t ICMP_ECHO_REQUEST = 8;
  const uint8_t ICMP_ECHO_REPLY = 0;
  const uint8_t ICMP_HEADER_LEN = 8;
  const uint16_t ECHO_PAYLOAD_LEN = 32;

  /**
   * The ones complement sum an icmp header carries, computed over the whole
   * message with the checksum field left zero.
   */
  uint16_t icmpChecksum(const uint8_t *data, uint32_t len) {

    uint32_t sum = 0;

    for (uint32_t i = 0; i + 1 < len; i += 2) {
      sum += (uint32_t)((data[i] << 8) | data[i + 1]);
    }

    if (len & 1) {
      sum += (uint32_t)(data[len - 1] << 8);
    }

    while (sum >> 16) {
      sum = (sum & 0xFFFF) + (sum >> 16);
    }

    return (uint16_t)(~sum);
  }
}
#endif

#ifdef MOCK_DEVICE_TEST
mock_ping_host_t::mock_ping_host_t() : m_ip(0),
                                       m_reachable(false),
                                       m_rtt_ms(PDI_POSIX_PING_DEFAULT_RTT_MS)
{
}
#endif

PingInterface::PingInterface() : m_wifi(nullptr),
#ifdef MOCK_DEVICE_TEST
                                 m_default_reachable(true),
                                 m_default_rtt_ms(PDI_POSIX_PING_DEFAULT_RTT_MS),
                                 m_rtt_step_ms(0),
                                 m_drop_mask(0),
                                 m_target_reachable(false),
                                 m_target_rtt_ms(PDI_POSIX_PING_DEFAULT_RTT_MS),
#else
                                 m_sock(-1),
                                 m_raw(false),
                                 m_id(0),
                                 m_dest_ip(0),
                                 m_sent_at_ms(0),
                                 m_outstanding(false),
#endif
                                 m_interval_ms(PDI_POSIX_PING_DEFAULT_INTERVAL_MS),
                                 m_timeout_ms(PDI_POSIX_PING_DEFAULT_TIMEOUT_MS),
                                 m_stats({0, 0, 0, 0, 0}),
                                 m_sum_ms(0),
                                 m_seq(0),
                                 m_count(0),
                                 m_running(false),
                                 m_complete(false),
                                 m_host_resp(false),
                                 m_pkt_cb(nullptr),
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

    uint32_t ip = (uint32_t)_target;

#ifdef MOCK_DEVICE_TEST
    m_target_reachable = m_default_reachable;
    m_target_rtt_ms = m_default_rtt_ms;

    for (uint8_t i = 0; i < PDI_POSIX_PING_MAX_HOSTS; i++)
    {
        if (0 != m_hosts[i].m_ip && ip == m_hosts[i].m_ip)
        {
            m_target_reachable = m_hosts[i].m_reachable;
            m_target_rtt_ms = m_hosts[i].m_rtt_ms;
            break;
        }
    }
#else
    if (!openSocket())
    {
        return false;
    }

    m_dest_ip = ip;
    m_id = (uint16_t)(__i_dvc_ctrl.millis_now() & 0xFFFF);
    m_outstanding = false;
#endif

    memset(&m_stats, 0, sizeof(m_stats));
    m_sum_ms = 0;
    m_seq = 0;
    m_count = count;
    m_running = true;
    m_complete = false;
    m_host_resp = false;
    m_pkt_cb = on_packet;

#ifdef MOCK_DEVICE_TEST
    m_next_due_ms = __i_dvc_ctrl.millis_now() +
                    (m_target_reachable ? m_target_rtt_ms : m_timeout_ms);
#else
    m_next_due_ms = __i_dvc_ctrl.millis_now();
#endif

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

#ifdef MOCK_DEVICE_TEST

void PingInterface::setHostReachable(const ipaddress_t &target, bool reachable, uint32_t rtt_ms)
{
    ipaddress_t _target = target;
    uint32_t ip = (uint32_t)_target;
    if (0 == ip)
    {
        return;
    }

    for (uint8_t i = 0; i < PDI_POSIX_PING_MAX_HOSTS; i++)
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
    for (uint8_t i = 0; i < PDI_POSIX_PING_MAX_HOSTS; i++)
    {
        m_hosts[i] = mock_ping_host_t();
    }

    m_default_reachable = true;
    m_default_rtt_ms = PDI_POSIX_PING_DEFAULT_RTT_MS;
    m_rtt_step_ms = 0;
    m_drop_mask = 0;
    m_interval_ms = PDI_POSIX_PING_DEFAULT_INTERVAL_MS;
    m_timeout_ms = PDI_POSIX_PING_DEFAULT_TIMEOUT_MS;
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

#else

/**
 * Opens the echo socket, preferring the unprivileged datagram form.
 */
bool PingInterface::openSocket()
{
    if (m_sock >= 0)
    {
        return true;
    }

    // the host kernel serves echo requests on a datagram socket to anyone inside
    // net.ipv4.ping_group_range; a raw socket needs CAP_NET_RAW
    m_raw = false;
    int fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_ICMP);

    if (fd < 0)
    {
        fd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
        m_raw = true;
    }

    if (fd < 0)
    {
        return false;
    }

    int flags = fcntl(fd, F_GETFL, 0);
    if (flags < 0 || fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0)
    {
        close(fd);
        return false;
    }

    m_sock = fd;
    return true;
}

/**
 * Sends one echo request and notes when it went out.
 */
bool PingInterface::sendEcho()
{
    uint8_t packet[ICMP_HEADER_LEN + ECHO_PAYLOAD_LEN];
    memset(packet, 0, sizeof(packet));

    packet[0] = ICMP_ECHO_REQUEST;
    packet[4] = (uint8_t)(m_id >> 8);
    packet[5] = (uint8_t)(m_id & 0xFF);
    packet[6] = (uint8_t)((m_seq + 1) >> 8);
    packet[7] = (uint8_t)((m_seq + 1) & 0xFF);

    for (uint16_t i = 0; i < ECHO_PAYLOAD_LEN; i++)
    {
        packet[ICMP_HEADER_LEN + i] = (uint8_t)('a' + (i % 23));
    }

    uint16_t sum = icmpChecksum(packet, sizeof(packet));
    packet[2] = (uint8_t)(sum >> 8);
    packet[3] = (uint8_t)(sum & 0xFF);

    struct sockaddr_in to;
    memset(&to, 0, sizeof(to));
    to.sin_family = AF_INET;
    to.sin_addr.s_addr = htonl(m_dest_ip);

    ssize_t put = sendto(m_sock, packet, sizeof(packet), 0,
                         (struct sockaddr *)&to, sizeof(to));

    m_sent_at_ms = __i_dvc_ctrl.millis_now();
    m_outstanding = true;
    m_seq++;
    m_stats.m_transmitted = m_seq;

    return put == (ssize_t)sizeof(packet);
}

/**
 * Takes a reply if one is waiting, reporting its round trip.
 */
bool PingInterface::takeReply(uint32_t &rtt_ms)
{
    uint8_t buf[128];

    while (true)
    {
        ssize_t got = recv(m_sock, buf, sizeof(buf), 0);

        if (got < 0)
        {
            return false;
        }

        // a raw socket hands back the ip header as well; the datagram one
        // starts at the icmp message
        uint32_t offset = 0;
        if (m_raw)
        {
            if (got < 20)
            {
                continue;
            }
            offset = (uint32_t)((buf[0] & 0x0F) * 4);
        }

        if ((uint32_t)got < offset + ICMP_HEADER_LEN)
        {
            continue;
        }

        const uint8_t *icmp = buf + offset;
        if (ICMP_ECHO_REPLY != icmp[0])
        {
            continue;
        }

        // the kernel rewrites the id on a datagram socket, so only the
        // sequence number is ours to match on there
        uint16_t seq = (uint16_t)((icmp[6] << 8) | icmp[7]);
        if (seq != m_seq)
        {
            continue;
        }

        if (m_raw)
        {
            uint16_t id = (uint16_t)((icmp[4] << 8) | icmp[5]);
            if (id != m_id)
            {
                continue;
            }
        }

        uint32_t now = __i_dvc_ctrl.millis_now();
        rtt_ms = (now > m_sent_at_ms) ? (now - m_sent_at_ms) : 1;
        return true;
    }
}

/**
 * Records one packet's outcome and hands it to the caller.
 */
void PingInterface::reportPacket(bool replied, uint32_t rtt_ms)
{
    if (replied)
    {
        m_stats.m_received++;
        if (1 == m_stats.m_received || rtt_ms < m_stats.m_min_ms)
        {
            m_stats.m_min_ms = rtt_ms;
        }
        if (rtt_ms > m_stats.m_max_ms)
        {
            m_stats.m_max_ms = rtt_ms;
        }
        m_sum_ms += rtt_ms;
        m_stats.m_avg_ms = m_sum_ms / m_stats.m_received;
    }

    m_host_resp = replied;
    m_outstanding = false;

    if (m_seq >= m_count)
    {
        m_running = false;
        m_complete = true;

        if (m_sock >= 0)
        {
            close(m_sock);
            m_sock = -1;
        }
    }
    else
    {
        m_next_due_ms = __i_dvc_ctrl.millis_now() + m_interval_ms;
    }

    if (m_pkt_cb)
    {
        ping_pkt_t pkt = {m_seq, replied, rtt_ms};
        m_pkt_cb((void *)&pkt);
    }
}

uint16_t PingInterface::service(void)
{
    uint16_t reported = 0;

    while (m_running)
    {
        uint32_t now = __i_dvc_ctrl.millis_now();

        if (m_outstanding)
        {
            uint32_t rtt = 0;
            if (takeReply(rtt))
            {
                reportPacket(true, rtt);
                reported++;
                continue;
            }

            if ((now - m_sent_at_ms) >= m_timeout_ms)
            {
                reportPacket(false, 0);
                reported++;
                continue;
            }

            break;
        }

        if ((int32_t)(now - m_next_due_ms) < 0)
        {
            break;
        }

        if (m_seq >= m_count)
        {
            m_running = false;
            m_complete = true;
            break;
        }

        if (!sendEcho())
        {
            reportPacket(false, 0);
            reported++;
        }
    }

    return reported;
}

#endif

PingInterface __i_ping;
