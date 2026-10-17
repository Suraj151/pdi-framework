/******************************** Fuzz mDNS ***********************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

The mdns responder's query reader: the header, the question count, and each
question's labels, type and class. Any host on the network multicasts to it
without a login, and the answers it picks are built in a fixed buffer, so the
reply is read back here as well and a record built past its end is caught.

Author          : Suraj I.
created Date    : 9th Oct 2026
******************************************************************************/

#include <FuzzCommon.h>

#ifdef ENABLE_MDNS_SERVICE

#include <service_provider/network/MdnsServiceProvider.h>

namespace
{

    /**
     * @brief A socket that reads every byte of each reply it is handed.
     */
    class SinkUdp : public iUdpInterface
    {
    public:
        bool begin(uint16_t local_port) override { return true; }
        bool joinMulticastGroup(const ipaddress_t &group) override { return true; }
        void setOnPacketCallback(CallBackVoidPointerArgFn callbk) override {}
        void close() override {}

        int32_t send(const uint8_t *data, uint16_t len, const ipaddress_t &dst, uint16_t dst_port) override
        {
            volatile uint8_t fold = 0;
            for (uint16_t i = 0; i < len; i++)
            {
                fold ^= data[i];
            }
            return len;
        }
    };

    /**
     * @brief The responder with every service a full build advertises, and its
     * query reader opened to the harness.
     */
    class Responder : public MdnsServiceProvider
    {
    public:
        Responder()
        {
            m_udp = pdiutil::safe_new<SinkUdp>();
            m_hostname = "pdi-a1b2c3";
            m_ip = ipaddress_t(192, 168, 1, 10);
            addService("_https", "_tcp", 443);
            addService("_ssh", "_tcp", 22);
            addService("_sftp-ssh", "_tcp", 22);
            addService("_telnet", "_tcp", 23);
        }

        void feed(udp_packet_t *pkt) { onUdpPacket(pkt); }
    };

} // namespace

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    // never destroyed: it stands in for the service the build registered
    static Responder *responder = new Responder();

    udp_packet_t pkt;
    pkt.m_data = data;
    pkt.m_len = size > 0xFFFF ? 0xFFFF : (uint16_t)size;
    pkt.m_src_ip = ipaddress_t(192, 168, 1, 20);
    pkt.m_src_port = 5353;

    responder->feed(&pkt);
    return 0;
}

#else

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    return 0;
}

#endif
