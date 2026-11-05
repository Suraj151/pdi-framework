/**************************** Telnet Write Tests *******************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Covers what a telnet write reports: the caller's bytes that went out, with
every 0xFF doubled on the wire, and no more than the connection took.

Author          : Suraj I.
created Date    : 3rd Nov 2026
******************************************************************************/

#include <MountedStack.h>
#include <pditest.h>
#include <service_provider/transport/TelnetClientInterface.h>
#include <vector>

#ifdef ENABLE_TELNET_SERVICE

namespace {

/**
 * A connection that takes at most budget bytes in all and keeps what it took.
 */
class BudgetConnection : public iTcpClientInterface
{
public:
    explicit BudgetConnection(int32_t budget, int32_t refusal = 0) : m_budget(budget), m_refusal(refusal) {}

    int16_t connect(const uint8_t *host, uint16_t port) override { return 1; }
    int16_t disconnect() override { return 1; }
    int8_t connected() override { return 1; }
    void setTimeout(uint32_t timeout) override {}
    int32_t write(uint8_t c) override { return write(&c, 1); }
    int32_t write(const uint8_t *c_str) override { return write(c_str, (uint32_t)strlen((const char *)c_str)); }
    int32_t write(const uint8_t *c_str, uint32_t size) override
    {
        if (m_budget <= 0 && 0 != m_refusal) return m_refusal;
        uint32_t take = (uint32_t)m_budget < size ? (uint32_t)m_budget : size;
        m_wire.insert(m_wire.end(), c_str, c_str + take);
        m_budget -= (int32_t)take;
        return (int32_t)take;
    }
    bool availableforwrite(uint32_t size) override { return true; }
    int32_t available() override { return 0; }
    uint8_t read() override { return 0; }
    int32_t read(uint8_t *buf, uint32_t size) override { return 0; }
    ipaddress_t getLocalIp() const override { return ipaddress_t(); }
    uint16_t getLocalPort() const override { return 23; }
    ipaddress_t getRemoteIp() const override { return ipaddress_t(); }
    uint16_t getRemotePort() const override { return 4000; }
    bool setKeepAlive(uint16_t idleTime, uint16_t interval, uint16_t count) override { return true; }
    void setNoDelay(bool noDelay) override {}

    std::vector<uint8_t> m_wire;

private:
    int32_t m_budget;
    int32_t m_refusal;
};

} // namespace

TEST(telnetwrite, a_write_the_connection_takes_whole_reports_every_byte)
{
    BudgetConnection *conn = pdiutil::safe_new<BudgetConnection>(1000);
    TelnetClientInterface telnet(conn);
    const uint8_t data[] = {'a', 'b', 'c', 'd'};
    ASSERT_EQ(telnet.write(data, sizeof(data)), (int32_t)sizeof(data));
    ASSERT_EQ(conn->m_wire.size(), sizeof(data));
}

TEST(telnetwrite, a_write_the_connection_takes_in_part_reports_only_what_went_out)
{
    BudgetConnection *conn = pdiutil::safe_new<BudgetConnection>(3);
    TelnetClientInterface telnet(conn);
    const uint8_t data[] = {'a', 'b', 'c', 'd', 'e', 'f'};
    ASSERT_EQ(telnet.write(data, sizeof(data)), 3);
    ASSERT_EQ(conn->m_wire.size(), (size_t)3);
}

TEST(telnetwrite, a_doubled_byte_counts_once_for_the_caller)
{
    BudgetConnection *conn = pdiutil::safe_new<BudgetConnection>(1000);
    TelnetClientInterface telnet(conn);
    const uint8_t data[] = {'a', 0xFF, 'b'};
    ASSERT_EQ(telnet.write(data, sizeof(data)), 3);
    ASSERT_EQ(conn->m_wire.size(), (size_t)4);
    ASSERT_EQ(conn->m_wire[1], 0xFF);
    ASSERT_EQ(conn->m_wire[2], 0xFF);
}

TEST(telnetwrite, a_doubled_byte_the_connection_cannot_take_ends_the_count_before_it)
{
    BudgetConnection *conn = pdiutil::safe_new<BudgetConnection>(2);
    TelnetClientInterface telnet(conn);
    const uint8_t data[] = {'a', 0xFF, 'b', 'c'};
    ASSERT_EQ(telnet.write(data, sizeof(data)), 1);
}

TEST(telnetwrite, a_write_refused_before_any_byte_reports_the_refusal)
{
    BudgetConnection *conn = pdiutil::safe_new<BudgetConnection>(0, PDI_ERR_STATE);
    TelnetClientInterface telnet(conn);
    const uint8_t data[] = {'a', 'b'};
    ASSERT_EQ(telnet.write(data, sizeof(data)), (int32_t)PDI_ERR_STATE);
}

#endif
