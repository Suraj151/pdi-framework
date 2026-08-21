/****************************** Ping Interface *******************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st June 2019
******************************************************************************/

#ifndef _MOCKDEVICE_PING_INTERFACE_H_
#define _MOCKDEVICE_PING_INTERFACE_H_

#include "mockdevice.h"
#include <interface/pdi/middlewares/iPingInterface.h>

#ifndef MOCKDEVICE_PING_MAX_HOSTS
#define MOCKDEVICE_PING_MAX_HOSTS 4
#endif

#ifndef MOCKDEVICE_PING_DEFAULT_RTT_MS
#define MOCKDEVICE_PING_DEFAULT_RTT_MS 8
#endif

#ifndef MOCKDEVICE_PING_DEFAULT_INTERVAL_MS
#define MOCKDEVICE_PING_DEFAULT_INTERVAL_MS 10
#endif

#ifndef MOCKDEVICE_PING_DEFAULT_TIMEOUT_MS
#define MOCKDEVICE_PING_DEFAULT_TIMEOUT_MS 40
#endif

/**
 * how one scripted host answers an echo request
 */
struct mock_ping_host_t
{
    uint32_t m_ip;
    bool m_reachable;
    uint32_t m_rtt_ms;

    mock_ping_host_t();
};

/**
 * @class PingInterface
 * @brief An echo engine a test scripts instead of a network answering it.
 *
 * Raw icmp needs a privileged socket, so nothing here leaves the process. A
 * caller says which addresses answer and how quickly, and the run still
 * unfolds over time the way a real one does: packets are reported one at a
 * time from service(), which the device control interface calls out of yield
 * and handleEvents, so a caller that waits for isPingComplete sees the same
 * ordering it sees on hardware.
 */
class PingInterface : public iPingInterface
{

public:
    PingInterface();
    ~PingInterface();

    void init_ping(iWiFiInterface *_wifi) override;
    bool ping(const ipaddress_t &target, uint16_t count = 1,
              CallBackVoidPointerArgFn on_packet = nullptr) override;
    bool isPingComplete(void) override;
    bool isHostRespondingToPing(void) override;
    const ping_stats_t &getPingStats(void) override;
    bool isPingBusy(void) override;

    /**
     * @brief Decide how one address answers. Overrides the default policy.
     */
    void setHostReachable(const ipaddress_t &target, bool reachable,
                          uint32_t rtt_ms = MOCKDEVICE_PING_DEFAULT_RTT_MS);

    /**
     * @brief Answer for every address no host entry covers.
     */
    void setDefaultReachable(bool reachable, uint32_t rtt_ms = MOCKDEVICE_PING_DEFAULT_RTT_MS);

    /**
     * @brief Change each successive reply's round trip by this much, so a run
     *        produces a spread of times rather than one repeated value.
     */
    void setRttStep(int32_t step_ms);

    /**
     * @brief Lose the packet with this sequence number even from a reachable
     *        host. Sequence numbers are 1 based and up to 32 are honoured.
     */
    void dropPacket(uint16_t seqno);

    /**
     * @brief Gap between one packet's outcome and the next request.
     */
    void setPacketInterval(uint32_t interval_ms);

    /**
     * @brief How long a lost packet takes to be declared lost.
     */
    void setTimeout(uint32_t timeout_ms);

    /**
     * @brief Forget every scripted host, the drop list and the timings.
     */
    void clearScript();

    /**
     * @brief Report whichever packets have come due.
     * @return number of packets reported.
     */
    uint16_t service();

protected:
    /**
     * @var iWiFiInterface* wifi
     */
    iWiFiInterface *m_wifi;

    mock_ping_host_t m_hosts[MOCKDEVICE_PING_MAX_HOSTS];
    bool m_default_reachable;
    uint32_t m_default_rtt_ms;
    int32_t m_rtt_step_ms;
    uint32_t m_drop_mask;
    uint32_t m_interval_ms;
    uint32_t m_timeout_ms;

    ping_stats_t m_stats;
    uint32_t m_sum_ms;
    uint16_t m_seq;
    uint16_t m_count;
    bool m_running;
    bool m_complete;
    bool m_host_resp;
    CallBackVoidPointerArgFn m_pkt_cb;
    bool m_target_reachable;
    uint32_t m_target_rtt_ms;
    uint32_t m_next_due_ms;
};

#endif
