/******************************** mDNS Tests **********************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

The responder answered over a socket that keeps every reply. Each reply is read
back record by record, so a test asserts on the names, types, port and address
a browser would see, and that a question for anything the board does not own
gets no reply at all.

Author          : Suraj I.
created Date    : 9th Oct 2026
******************************************************************************/

#include <MountedStack.h>
#include <pditest.h>

#ifdef ENABLE_MDNS_SERVICE

#include <service_provider/network/MdnsServiceProvider.h>
#include <string>
#include <vector>

namespace
{

    const uint16_t TYPE_A = 1;
    const uint16_t TYPE_PTR = 12;
    const uint16_t TYPE_TXT = 16;
    const uint16_t TYPE_SRV = 33;
    const uint16_t TYPE_ANY = 255;

    struct Record
    {
        std::string m_name;
        uint16_t m_type;
        uint16_t m_class;
        std::vector<uint8_t> m_data;
    };

    /**
     * @brief A socket that keeps each reply it is handed.
     */
    class KeepingUdp : public iUdpInterface
    {
    public:
        bool begin(uint16_t local_port) override { return true; }
        bool joinMulticastGroup(const ipaddress_t &group) override { return true; }
        void setOnPacketCallback(CallBackVoidPointerArgFn callbk) override {}
        void close() override {}

        int32_t send(const uint8_t *data, uint16_t len, const ipaddress_t &dst, uint16_t dst_port) override
        {
            m_replies.push_back(std::vector<uint8_t>(data, data + len));
            m_ports.push_back(dst_port);
            return len;
        }

        std::vector<std::vector<uint8_t>> m_replies;
        std::vector<uint16_t> m_ports;
    };

    /**
     * @brief The responder with a known name, address and services, its query
     * reader opened to the test, and the build's own responder kept registered.
     */
    class Responder : public MdnsServiceProvider
    {
    public:
        Responder()
        {
            ServiceProvider::m_services[SERVICE_MDNS] = &__mdns_service;
            m_udp = m_socket = new KeepingUdp();
            m_hostname = "pdi-a1b2c3";
            m_ip = ipaddress_t(192, 168, 1, 10);
            addService("_https", "_tcp", 443);
            addService("_ssh", "_tcp", 22);
        }

        ~Responder()
        {
            ServiceProvider::m_services[SERVICE_MDNS] = &__mdns_service;
        }

        void feed(const std::vector<uint8_t> &packet)
        {
            udp_packet_t pkt;
            pkt.m_data = packet.data();
            pkt.m_len = (uint16_t)packet.size();
            pkt.m_src_ip = ipaddress_t(192, 168, 1, 20);
            pkt.m_src_port = 5353;
            onUdpPacket(&pkt);
        }

        bool addMore(const char *type) { return addService(type, "_tcp", 1); }
        bool addRaw(const char *type, const char *proto) { return addService(type, proto, 1); }

        KeepingUdp *m_socket;
    };

    std::vector<uint8_t> name(std::initializer_list<const char *> labels)
    {
        std::vector<uint8_t> out;
        for (const char *label : labels)
        {
            out.push_back((uint8_t)strlen(label));
            out.insert(out.end(), label, label + strlen(label));
        }
        out.push_back(0);
        return out;
    }

    std::vector<uint8_t> query(std::initializer_list<std::pair<std::vector<uint8_t>, uint16_t>> questions, uint8_t flags = 0)
    {
        std::vector<uint8_t> out = {0, 0, flags, 0, 0, (uint8_t)questions.size(), 0, 0, 0, 0, 0, 0};
        for (const auto &q : questions)
        {
            out.insert(out.end(), q.first.begin(), q.first.end());
            out.push_back((uint8_t)(q.second >> 8));
            out.push_back((uint8_t)q.second);
            out.push_back(0);
            out.push_back(1);
        }
        return out;
    }

    uint16_t u16(const std::vector<uint8_t> &b, size_t at)
    {
        return (uint16_t)((b.at(at) << 8) | b.at(at + 1));
    }

    std::string readName(const std::vector<uint8_t> &b, size_t &at)
    {
        std::string out;
        while (b.at(at) != 0)
        {
            uint8_t len = b.at(at++);
            if (!out.empty()) out += ".";
            out.append((const char *)&b.at(at), len);
            at += len;
        }
        at++;
        return out;
    }

    /**
     * @brief Every record in a reply, answers and additional records alike;
     * fails the test when the counts disagree with what is there.
     */
    std::vector<Record> records(const std::vector<uint8_t> &reply)
    {
        std::vector<Record> out;
        uint16_t count = u16(reply, 6) + u16(reply, 8) + u16(reply, 10);
        size_t at = 12;

        for (uint16_t i = 0; i < count; i++)
        {
            Record r;
            r.m_name = readName(reply, at);
            r.m_type = u16(reply, at);
            r.m_class = u16(reply, at + 2);
            uint16_t rdlen = u16(reply, at + 8);
            at += 10;
            r.m_data.assign(reply.begin() + at, reply.begin() + at + rdlen);
            at += rdlen;
            out.push_back(r);
        }

        if (at != reply.size()) out.clear();
        return out;
    }

    const Record *find(const std::vector<Record> &all, uint16_t type)
    {
        for (const Record &r : all)
        {
            if (r.m_type == type) return &r;
        }
        return nullptr;
    }

} // namespace

TEST(mdns, the_board_answers_for_its_own_name_with_its_address)
{
    Responder responder;
    responder.feed(query({{name({"pdi-a1b2c3", "local"}), TYPE_A}}));

    ASSERT_EQ(responder.m_socket->m_replies.size(), 1u);
    ASSERT_EQ(responder.m_socket->m_ports[0], 5353);

    std::vector<Record> all = records(responder.m_socket->m_replies[0]);
    ASSERT_EQ(all.size(), 1u);
    ASSERT_STREQ(all[0].m_name.c_str(), "pdi-a1b2c3.local");
    ASSERT_EQ(all[0].m_type, TYPE_A);
    ASSERT_EQ(all[0].m_class, 0x8001);
    ASSERT_EQ(all[0].m_data, std::vector<uint8_t>({192, 168, 1, 10}));
}

TEST(mdns, a_name_is_matched_whatever_its_case)
{
    Responder responder;
    responder.feed(query({{name({"PDI-A1B2C3", "LOCAL"}), TYPE_A}}));

    ASSERT_EQ(responder.m_socket->m_replies.size(), 1u);
}

TEST(mdns, a_name_the_board_does_not_own_gets_no_reply)
{
    Responder responder;
    responder.feed(query({{name({"pdi-ffffff", "local"}), TYPE_A}}));
    responder.feed(query({{name({"pdi-a1b2c", "local"}), TYPE_A}}));
    responder.feed(query({{name({"pdi-a1b2c3x", "local"}), TYPE_A}}));
    responder.feed(query({{name({"pdi-a1b2c3", "lan"}), TYPE_A}}));
    responder.feed(query({{name({"pdi-a1b2c3", "local"}), TYPE_TXT}}));

    ASSERT_EQ(responder.m_socket->m_replies.size(), 0u);
}

TEST(mdns, a_reply_from_another_responder_is_never_answered)
{
    Responder responder;
    responder.feed(query({{name({"pdi-a1b2c3", "local"}), TYPE_A}}, 0x84));

    ASSERT_EQ(responder.m_socket->m_replies.size(), 0u);
}

TEST(mdns, enumeration_lists_every_advertised_service_type)
{
    Responder responder;
    responder.feed(query({{name({"_services", "_dns-sd", "_udp", "local"}), TYPE_PTR}}));

    ASSERT_EQ(responder.m_socket->m_replies.size(), 1u);
    std::vector<Record> all = records(responder.m_socket->m_replies[0]);
    ASSERT_EQ(all.size(), 2u);

    std::vector<std::string> types;
    for (const Record &r : all)
    {
        ASSERT_EQ(r.m_type, TYPE_PTR);
        size_t at = 0;
        types.push_back(readName(r.m_data, at));
    }
    ASSERT_STREQ(types[0].c_str(), "_https._tcp.local");
    ASSERT_STREQ(types[1].c_str(), "_ssh._tcp.local");
}

TEST(mdns, browsing_a_type_answers_with_the_instance_its_port_and_address)
{
    Responder responder;
    responder.feed(query({{name({"_ssh", "_tcp", "local"}), TYPE_PTR}}));

    ASSERT_EQ(responder.m_socket->m_replies.size(), 1u);
    std::vector<Record> all = records(responder.m_socket->m_replies[0]);
    ASSERT_EQ(all.size(), 4u);

    const Record *ptr = find(all, TYPE_PTR);
    ASSERT_TRUE(nullptr != ptr);
    size_t at = 0;
    ASSERT_STREQ(readName(ptr->m_data, at).c_str(), "pdi-a1b2c3._ssh._tcp.local");

    const Record *srv = find(all, TYPE_SRV);
    ASSERT_TRUE(nullptr != srv);
    ASSERT_EQ(u16(srv->m_data, 4), 22);
    at = 6;
    ASSERT_STREQ(readName(srv->m_data, at).c_str(), "pdi-a1b2c3.local");

    const Record *a = find(all, TYPE_A);
    ASSERT_TRUE(nullptr != a);
    ASSERT_EQ(a->m_data, std::vector<uint8_t>({192, 168, 1, 10}));

    ASSERT_TRUE(nullptr != find(all, TYPE_TXT));
}

TEST(mdns, resolving_an_instance_answers_for_that_service_only)
{
    Responder responder;
    responder.feed(query({{name({"pdi-a1b2c3", "_https", "_tcp", "local"}), TYPE_SRV}}));

    ASSERT_EQ(responder.m_socket->m_replies.size(), 1u);
    std::vector<Record> all = records(responder.m_socket->m_replies[0]);
    const Record *srv = find(all, TYPE_SRV);
    ASSERT_TRUE(nullptr != srv);
    ASSERT_EQ(u16(srv->m_data, 4), 443);
}

TEST(mdns, a_service_the_board_does_not_run_gets_no_reply)
{
    Responder responder;
    responder.feed(query({{name({"_ftp", "_tcp", "local"}), TYPE_PTR}}));
    responder.feed(query({{name({"pdi-a1b2c3", "_ftp", "_tcp", "local"}), TYPE_SRV}}));
    responder.feed(query({{name({"_ssh", "_udp", "local"}), TYPE_PTR}}));

    ASSERT_EQ(responder.m_socket->m_replies.size(), 0u);
}

TEST(mdns, the_question_it_owns_is_found_behind_one_it_does_not)
{
    Responder responder;
    responder.feed(query({{name({"printer", "local"}), TYPE_A}, {name({"pdi-a1b2c3", "local"}), TYPE_ANY}}));

    ASSERT_EQ(responder.m_socket->m_replies.size(), 1u);
}

TEST(mdns, a_compressed_or_truncated_question_is_dropped_without_a_reply)
{
    Responder responder;

    std::vector<uint8_t> compressed = {0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0xc0, 0x0c, 0, 1, 0, 1};
    responder.feed(compressed);

    std::vector<uint8_t> cut = query({{name({"pdi-a1b2c3", "local"}), TYPE_A}});
    cut.resize(cut.size() - 3);
    responder.feed(cut);

    std::vector<uint8_t> overlong = {0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 60, 'a', 'b'};
    responder.feed(overlong);

    std::vector<uint8_t> header_only = {0, 0, 0, 0, 0, 9, 0, 0, 0, 0, 0, 0};
    responder.feed(header_only);

    ASSERT_EQ(responder.m_socket->m_replies.size(), 0u);
}

TEST(mdns, a_name_of_more_labels_than_any_answer_is_dropped)
{
    Responder responder;
    responder.feed(query({{name({"a", "b", "c", "d", "e", "f", "g", "local"}), TYPE_ANY}}));

    ASSERT_EQ(responder.m_socket->m_replies.size(), 0u);
}

TEST(mdns, the_service_table_refuses_past_its_size)
{
    Responder responder;
    uint8_t added = 0;
    while (responder.addMore("_extra")) added++;

    ASSERT_EQ(added, (uint8_t)(MDNS_MAX_SERVICES - 2));
}

TEST(mdns, a_service_type_dns_sd_would_not_carry_is_refused)
{
    Responder responder;

    ASSERT_FALSE(responder.addRaw("_sixteen-chars-xx", "_tcp"));
    ASSERT_FALSE(responder.addRaw("noscore", "_tcp"));
    ASSERT_FALSE(responder.addRaw("_", "_tcp"));
    ASSERT_FALSE(responder.addRaw("_http", "_sctp"));
    ASSERT_TRUE(responder.addRaw("_fifteen-chars-x", "_udp"));
}

TEST(mdns, an_enumeration_of_long_types_stops_at_what_fits_and_still_parses)
{
    Responder responder;
    while (responder.addRaw("_fifteen-chars-x", "_tcp")) {}

    responder.feed(query({{name({"_services", "_dns-sd", "_udp", "local"}), TYPE_PTR}}));

    ASSERT_EQ(responder.m_socket->m_replies.size(), 1u);
    ASSERT_TRUE(responder.m_socket->m_replies[0].size() <= (size_t)MDNS_TX_BUFFER_SIZE);
    std::vector<Record> all = records(responder.m_socket->m_replies[0]);
    ASSERT_TRUE(all.size() >= 2u);
    for (const Record &r : all)
    {
        ASSERT_EQ(r.m_type, TYPE_PTR);
    }
}

#endif
