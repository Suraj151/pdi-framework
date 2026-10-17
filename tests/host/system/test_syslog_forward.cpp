/**************************** Syslog Forward Tests ******************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Every syslog line is also shipped to a collector as one RFC 3164 datagram over
udp. A socket on the loopback stands in for the collector, so each test reads
back exactly what went on the wire.

Author          : Suraj I.
created Date    : 16th Oct 2026
******************************************************************************/

#include <MountedStack.h>
#include <pditest.h>

#if defined(ENABLE_SYSLOG_SERVICE) && defined(ENABLE_SYSLOG_FORWARD)

#include <service_provider/network/SyslogServiceProvider.h>
#include <interface/pdi/impl/log/LogMacros.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>

namespace
{

    /** Reaches the collector settings that only a build-time macro sets on a board. */
    struct SyslogProbe : public SyslogServiceProvider
    {
        /** Points the running service at host:port and resolves it. */
        static bool aim(const char *host, uint16_t port)
        {
            __syslog_service.*(&SyslogProbe::m_host) = host;
            __syslog_service.*(&SyslogProbe::m_port) = port;
            return (__syslog_service.*(&SyslogProbe::resolveCollector))();
        }

        /** True while the collector address is resolved. */
        static bool resolved()
        {
            return __syslog_service.*(&SyslogProbe::m_resolved);
        }

        /** True while the forwarding socket is open. */
        static bool socketOpen()
        {
            return nullptr != __syslog_service.*(&SyslogProbe::m_udp);
        }
    };

    /** A udp socket on the loopback that plays the remote collector. */
    class Collector
    {
    public:
        Collector() : m_fd(-1), m_port(0)
        {
            m_fd = socket(AF_INET, SOCK_DGRAM, 0);
            struct sockaddr_in addr;
            memset(&addr, 0, sizeof(addr));
            addr.sin_family = AF_INET;
            addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
            addr.sin_port = 0;
            if (m_fd >= 0 && 0 == bind(m_fd, (struct sockaddr *)&addr, sizeof(addr)))
            {
                socklen_t len = sizeof(addr);
                getsockname(m_fd, (struct sockaddr *)&addr, &len);
                m_port = ntohs(addr.sin_port);
            }
        }

        ~Collector()
        {
            if (m_fd >= 0) close(m_fd);
        }

        uint16_t port() const { return m_port; }

        /** The next datagram, or an empty string when none arrives within wait_ms. */
        pdiutil::string next(uint32_t wait_ms = 500)
        {
            struct timeval tv;
            tv.tv_sec = wait_ms / 1000;
            tv.tv_usec = (wait_ms % 1000) * 1000;
            setsockopt(m_fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

            char buf[1024];
            ssize_t got = recv(m_fd, buf, sizeof(buf), 0);
            return got > 0 ? pdiutil::string(buf, (size_t)got) : pdiutil::string();
        }

    private:
        int m_fd;
        uint16_t m_port;
    };

    /** Brings the service up on a mounted filesystem with no clock, aimed at the collector. */
    bool forwardTo(Collector &collector)
    {
        pditest::mountedVfs();
        __i_ntp.clearTime();
        __syslog_service.initService();
        return SyslogProbe::aim("127.0.0.1", collector.port());
    }

    /** Leaves the service the way the rest of the binary expects it: stopped, aimed nowhere. */
    void standDown()
    {
        __syslog_service.stopService();
        SyslogProbe::aim("", SYSLOG_REMOTE_PORT);
        __i_ntp.clearTime();
    }

    /** The header a line of this priority carries while the clock is unsynced. */
    pdiutil::string unsyncedHeader(uint8_t pri)
    {
        pdiutil::string host = __i_wifi.localIP();
        char head[64];
        snprintf(head, sizeof(head), "<%u>%s %s: ", (unsigned)pri, host.c_str(), SYSLOG_FORWARD_TAG);
        return pdiutil::string(head);
    }

    /** Sends one line straight through the service, the way the log manager hands it over. */
    void handOver(logger_type_t type, const char *line)
    {
        __syslog_service.handleLine(type, line, (uint16_t)strlen(line));
    }
}

TEST(syslog_forward, a_line_reaches_the_collector_as_one_datagram)
{
    Collector collector;
    ASSERT_GT(collector.port(), (uint16_t)0);
    bool aimed = forwardTo(collector);

    handOver(ERROR_LOG, "disk is on fire\n");
    pdiutil::string got = collector.next();
    pdiutil::string extra = collector.next(200);

    standDown();

    ASSERT_TRUE(aimed);
    ASSERT_STREQ(got.c_str(), (unsyncedHeader(131) + "disk is on fire").c_str());
    ASSERT_TRUE(extra.empty());
}

TEST(syslog_forward, each_level_carries_its_own_severity)
{
    Collector collector;
    forwardTo(collector);

    handOver(ERROR_LOG, "e");
    pdiutil::string error = collector.next();
    handOver(WARNING_LOG, "w");
    pdiutil::string warning = collector.next();
    handOver(SUCCESS_LOG, "s");
    pdiutil::string success = collector.next();
    handOver(INFO_LOG, "i");
    pdiutil::string info = collector.next();

    standDown();

    ASSERT_STREQ(error.c_str(), (unsyncedHeader(131) + "e").c_str());
    ASSERT_STREQ(warning.c_str(), (unsyncedHeader(132) + "w").c_str());
    ASSERT_STREQ(success.c_str(), (unsyncedHeader(133) + "s").c_str());
    ASSERT_STREQ(info.c_str(), (unsyncedHeader(134) + "i").c_str());
}

TEST(syslog_forward, the_timestamp_appears_once_the_clock_is_synced)
{
    Collector collector;
    forwardTo(collector);

    handOver(WARNING_LOG, "before");
    pdiutil::string before = collector.next();

    __i_ntp.set_ntp_time(1760400000);
    handOver(WARNING_LOG, "after");
    pdiutil::string after = collector.next();

    standDown();

    pdiutil::string host = __i_wifi.localIP();
    pdiutil::string synced = pdiutil::string("<132>Oct 14 00:00:00 ") + host + " " + SYSLOG_FORWARD_TAG + ": after";

    ASSERT_STREQ(before.c_str(), (unsyncedHeader(132) + "before").c_str());
    ASSERT_STREQ(after.c_str(), synced.c_str());
}

TEST(syslog_forward, a_day_below_ten_is_padded_with_a_space)
{
    Collector collector;
    forwardTo(collector);

    __i_ntp.set_ntp_time(1759968000);
    handOver(ERROR_LOG, "early in the month");
    pdiutil::string got = collector.next();

    standDown();

    pdiutil::string host = __i_wifi.localIP();
    pdiutil::string expected = pdiutil::string("<131>Oct  9 00:00:00 ") + host + " " + SYSLOG_FORWARD_TAG + ": early in the month";

    ASSERT_STREQ(got.c_str(), expected.c_str());
}

TEST(syslog_forward, a_line_longer_than_the_datagram_is_cut_to_it)
{
    Collector collector;
    forwardTo(collector);

    pdiutil::string longline(SYSLOG_FORWARD_DGRAM_MAX * 2, 'x');
    handOver(INFO_LOG, longline.c_str());
    pdiutil::string got = collector.next();

    standDown();

    pdiutil::string head = unsyncedHeader(134);
    ASSERT_EQ(got.size(), (size_t)SYSLOG_FORWARD_DGRAM_MAX);
    ASSERT_STREQ(got.substr(0, head.size()).c_str(), head.c_str());
    ASSERT_EQ(got.find_first_not_of('x', head.size()), pdiutil::string::npos);
}

TEST(syslog_forward, an_empty_collector_sends_nothing)
{
    Collector collector;
    forwardTo(collector);
    __syslog_service.stopService();
    __syslog_service.initService();

    bool aimed = SyslogProbe::aim("", collector.port());
    handOver(ERROR_LOG, "nobody listens");
    pdiutil::string got = collector.next(200);
    bool opened = SyslogProbe::socketOpen();

    standDown();

    ASSERT_FALSE(aimed);
    ASSERT_TRUE(got.empty());
    ASSERT_FALSE(opened);
}

TEST(syslog_forward, a_collector_that_cannot_be_resolved_sends_nothing_but_the_file_still_gets_the_line)
{
    Collector collector;
    forwardTo(collector);

    int64_t sizeBefore = __i_fs.getFileSize(SYSLOG_FILE_WARNING);
    bool aimed = SyslogProbe::aim("no-such-collector.invalid", collector.port());
    handOver(WARNING_LOG, "kept locally");
    pdiutil::string got = collector.next(200);
    int64_t sizeAfter = __i_fs.getFileSize(SYSLOG_FILE_WARNING);

    standDown();

    ASSERT_FALSE(aimed);
    ASSERT_TRUE(got.empty());
    ASSERT_GT(sizeAfter, sizeBefore < 0 ? (int64_t)0 : sizeBefore);
}

TEST(syslog_forward, a_stopped_service_forwards_nothing)
{
    Collector collector;
    forwardTo(collector);

    SysLogE("while running");
    pdiutil::string running = collector.next();

    __syslog_service.stopService();
    bool openAfterStop = SyslogProbe::socketOpen();
    SysLogE("while stopped");
    pdiutil::string stopped = collector.next(200);

    standDown();

    ASSERT_TRUE(running.find("while running") != pdiutil::string::npos);
    ASSERT_FALSE(openAfterStop);
    ASSERT_TRUE(stopped.empty());
}

TEST(syslog_forward, a_restart_forgets_the_resolved_collector)
{
    Collector collector;
    bool aimed = forwardTo(collector);

    __syslog_service.stopService();
    __syslog_service.resetServiceState();
    bool resolvedAfterReset = SyslogProbe::resolved();

    standDown();

    ASSERT_TRUE(aimed);
    ASSERT_FALSE(resolvedAfterReset);
}

TEST(syslog_forward, a_stopped_service_no_longer_answers_the_station_getting_an_address)
{
    Collector collector;
    forwardTo(collector);

    __syslog_service.stopService();
    __syslog_service.initService();
    __syslog_service.stopService();
    __syslog_service.resetServiceState();
    SyslogProbe::aim("127.0.0.1", collector.port());
    __syslog_service.resetServiceState();

    __utl_event.execute_event(EVENT_WIFI_STA_GOT_IP);
    bool resolvedByEvent = SyslogProbe::resolved();

    standDown();

    ASSERT_FALSE(resolvedByEvent);
}

#endif
