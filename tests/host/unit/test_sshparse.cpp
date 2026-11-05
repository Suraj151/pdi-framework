/**************************** SSH Parser Tests ********************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

The ssh wire parsers, fed the packets a real client sends. The fuzz tier proves
these do not crash on rubbish; what it cannot prove is that they still accept a
well formed request, which is what a bounds check tightened one step too far
would break.

Author          : Suraj I.
created Date    : 27th Aug 2026
******************************************************************************/

#include <pditest.h>
#include <config/Config.h>

#ifdef ENABLE_SSH_SERVICE

#include <service_provider/shell/ssh/SSHServiceUtil.h>
#include <devices/posix/DeviceControlInterface.h>
#include <utility/SafeAlloc.h>

using namespace LWSSH;

namespace
{
    /**
     * @brief A length-prefixed field, the shape every ssh string arrives in.
     */
    void appendString(pdiutil::vector<uint8_t> &out, const char *text)
    {
        uint32_t len = (uint32_t)strlen(text);
        out.push_back((uint8_t)((len >> 24) & 0xFF));
        out.push_back((uint8_t)((len >> 16) & 0xFF));
        out.push_back((uint8_t)((len >> 8) & 0xFF));
        out.push_back((uint8_t)(len & 0xFF));
        for (uint32_t i = 0; i < len; i++)
        {
            out.push_back((uint8_t)text[i]);
        }
    }

    /**
     * @brief A length-prefixed blob of the given size, filled with a pattern.
     */
    void appendBlob(pdiutil::vector<uint8_t> &out, uint32_t len)
    {
        out.push_back((uint8_t)((len >> 24) & 0xFF));
        out.push_back((uint8_t)((len >> 16) & 0xFF));
        out.push_back((uint8_t)((len >> 8) & 0xFF));
        out.push_back((uint8_t)(len & 0xFF));
        for (uint32_t i = 0; i < len; i++)
        {
            out.push_back((uint8_t)(i & 0xFF));
        }
    }

    void appendUint32(pdiutil::vector<uint8_t> &out, uint32_t value)
    {
        out.push_back((uint8_t)((value >> 24) & 0xFF));
        out.push_back((uint8_t)((value >> 16) & 0xFF));
        out.push_back((uint8_t)((value >> 8) & 0xFF));
        out.push_back((uint8_t)(value & 0xFF));
    }
}

TEST(sshparse, a_string_round_trips)
{
    pdiutil::vector<uint8_t> payload;
    appendString(payload, "ssh-connection");

    pdiutil::string out;
    int32_t offset = 0;

    ASSERT_TRUE(read_ssh_string(payload, out, offset));
    ASSERT_STREQ(out.c_str(), "ssh-connection");
    ASSERT_EQ(offset, (int32_t)payload.size());
}

TEST(sshparse, a_string_filling_the_payload_exactly_is_accepted)
{
    // the boundary a tightened bounds check gets wrong: len is exactly what
    // remains, which is legal and must not be refused
    pdiutil::vector<uint8_t> payload;
    appendString(payload, "abcd");

    pdiutil::string out;
    int32_t offset = 0;

    ASSERT_TRUE(read_ssh_string(payload, out, offset));
    ASSERT_STREQ(out.c_str(), "abcd");
}

TEST(sshparse, a_string_one_byte_longer_than_the_payload_is_refused)
{
    pdiutil::vector<uint8_t> payload;
    appendString(payload, "abcd");
    payload.pop_back();

    pdiutil::string out;
    int32_t offset = 0;

    ASSERT_FALSE(read_ssh_string(payload, out, offset));
}

TEST(sshparse, an_empty_string_is_accepted)
{
    pdiutil::vector<uint8_t> payload;
    appendString(payload, "");

    pdiutil::string out;
    int32_t offset = 0;

    ASSERT_TRUE(read_ssh_string(payload, out, offset));
    ASSERT_EQ((int)out.length(), 0);
}

TEST(sshparse, several_strings_read_in_sequence)
{
    pdiutil::vector<uint8_t> payload;
    appendString(payload, "first");
    appendString(payload, "second");
    appendString(payload, "third");

    pdiutil::string a, b, c;
    int32_t offset = 0;

    ASSERT_TRUE(read_ssh_string(payload, a, offset));
    ASSERT_TRUE(read_ssh_string(payload, b, offset));
    ASSERT_TRUE(read_ssh_string(payload, c, offset));

    ASSERT_STREQ(a.c_str(), "first");
    ASSERT_STREQ(b.c_str(), "second");
    ASSERT_STREQ(c.c_str(), "third");
    ASSERT_EQ(offset, (int32_t)payload.size());
}

TEST(sshparse, a_length_near_the_top_of_the_range_is_refused)
{
    // defect AX: offset + len wrapped in 32 bits and let this through
    pdiutil::vector<uint8_t> payload;
    payload.push_back(0xFF);
    payload.push_back(0xFF);
    payload.push_back(0xFF);
    payload.push_back(0xFF);
    payload.push_back('x');

    pdiutil::string out;
    int32_t offset = 0;

    ASSERT_FALSE(read_ssh_string(payload, out, offset));
}

TEST(sshparse, a_name_list_splits_on_commas)
{
    pdiutil::vector<uint8_t> payload;
    appendString(payload, "curve25519-sha256,ecdh-sha2-nistp256,diffie-hellman-group14-sha256");

    ssh_name_list names;
    uint32_t offset = 0;

    ASSERT_TRUE(parse_name_list(payload, offset, names));
    ASSERT_EQ((int)names.size(), 3);
    ASSERT_STREQ(names[0].c_str(), "curve25519-sha256");
    ASSERT_STREQ(names[2].c_str(), "diffie-hellman-group14-sha256");
}

TEST(sshparse, a_name_list_of_one_name_has_no_separator)
{
    // defect AY: find()'s result was truncated into a uint32_t and compared
    // against a 64-bit npos, so a list with no comma never terminated
    pdiutil::vector<uint8_t> payload;
    appendString(payload, "ssh-ed25519");

    ssh_name_list names;
    uint32_t offset = 0;

    ASSERT_TRUE(parse_name_list(payload, offset, names));
    ASSERT_EQ((int)names.size(), 1);
    ASSERT_STREQ(names[0].c_str(), "ssh-ed25519");
}

TEST(sshparse, an_empty_name_list_yields_no_names)
{
    pdiutil::vector<uint8_t> payload;
    appendString(payload, "");

    ssh_name_list names;
    uint32_t offset = 0;

    ASSERT_TRUE(parse_name_list(payload, offset, names));
    ASSERT_EQ((int)names.size(), 0);
}

TEST(sshparse, a_kex_init_yields_every_algorithm_list)
{
    pdiutil::vector<uint8_t> payload;
    payload.push_back(SSH2_MSG_KEXINIT);
    for (uint8_t i = 0; i < 16; i++)
    {
        payload.push_back(i);
    }

    appendString(payload, "curve25519-sha256");
    appendString(payload, "ssh-ed25519,rsa-sha2-256");
    appendString(payload, "aes256-ctr");
    appendString(payload, "aes256-ctr");
    appendString(payload, "hmac-sha2-256,hmac-sha1");
    appendString(payload, "hmac-sha2-256,hmac-sha1");
    appendString(payload, "none");
    appendString(payload, "none");
    appendString(payload, "");
    appendString(payload, "");
    payload.push_back(0);
    appendUint32(payload, 0);

    SSHKexInitFields fields;
    ASSERT_TRUE(parse_kex_init_fields(payload, fields));

    ASSERT_EQ((int)fields.cookie.size(), 16);
    ASSERT_STREQ(fields.kex_algorithms[0].c_str(), "curve25519-sha256");
    ASSERT_EQ((int)fields.server_host_key_algorithms.size(), 2);
    ASSERT_EQ((int)fields.mac_algorithms_ctos.size(), 2);
    ASSERT_STREQ(fields.encryption_algorithms_ctos[0].c_str(), "aes256-ctr");
}

TEST(sshparse, an_ecdh_init_carries_a_thirty_two_byte_key)
{
    pdiutil::vector<uint8_t> payload;
    payload.push_back(SSH2_MSG_KEXDH_INIT);
    appendBlob(payload, 32);

    EcdhInitPacket packet;
    ASSERT_TRUE(parse_kex_ecdh_init(payload, packet));
    ASSERT_EQ((int)packet.client_pubkey.size(), 32);
}

TEST(sshparse, a_password_auth_request_parses)
{
    pdiutil::vector<uint8_t> payload;
    payload.push_back(SSH2_MSG_USERAUTH_REQUEST);
    appendString(payload, "pdiStack");
    appendString(payload, "ssh-connection");
    appendString(payload, "password");
    payload.push_back(0);
    appendString(payload, "pdiStack@123");

    SSHUserAuthRequest request;
    ASSERT_TRUE(parse_userauth_request(payload, request));

    ASSERT_STREQ(request.username.c_str(), "pdiStack");
    ASSERT_STREQ(request.service.c_str(), "ssh-connection");
    ASSERT_STREQ(request.method.c_str(), "password");
    ASSERT_STREQ(request.password.c_str(), "pdiStack@123");
}

TEST(sshparse, a_publickey_auth_request_with_a_signature_parses)
{
    // the request that carries a key and proves possession of it, which is
    // what "an authorized public key is accepted without a password" sends
    pdiutil::vector<uint8_t> blob;
    appendString(blob, "ssh-ed25519");
    appendBlob(blob, 32);

    pdiutil::vector<uint8_t> signature;
    appendString(signature, "ssh-ed25519");
    appendBlob(signature, 64);

    pdiutil::vector<uint8_t> payload;
    payload.push_back(SSH2_MSG_USERAUTH_REQUEST);
    appendString(payload, "pdiStack");
    appendString(payload, "ssh-connection");
    appendString(payload, "publickey");
    payload.push_back(1);
    appendString(payload, "ssh-ed25519");

    appendUint32(payload, (uint32_t)blob.size());
    for (uint32_t i = 0; i < blob.size(); i++) payload.push_back(blob[i]);

    appendUint32(payload, (uint32_t)signature.size());
    for (uint32_t i = 0; i < signature.size(); i++) payload.push_back(signature[i]);

    SSHUserAuthRequest request;
    ASSERT_TRUE(parse_userauth_request(payload, request));

    ASSERT_STREQ(request.username.c_str(), "pdiStack");
    ASSERT_STREQ(request.method.c_str(), "publickey");
    ASSERT_TRUE(request.has_signature);
    ASSERT_STREQ(request.pk_algorithm.c_str(), "ssh-ed25519");
    ASSERT_EQ((int)request.pubkey_blob.size(), (int)blob.size());
    ASSERT_EQ((int)request.signature.size(), (int)signature.size());
}

TEST(sshparse, a_publickey_offer_without_a_signature_parses)
{
    pdiutil::vector<uint8_t> blob;
    appendString(blob, "ssh-ed25519");
    appendBlob(blob, 32);

    pdiutil::vector<uint8_t> payload;
    payload.push_back(SSH2_MSG_USERAUTH_REQUEST);
    appendString(payload, "pdiStack");
    appendString(payload, "ssh-connection");
    appendString(payload, "publickey");
    payload.push_back(0);
    appendString(payload, "ssh-ed25519");
    appendUint32(payload, (uint32_t)blob.size());
    for (uint32_t i = 0; i < blob.size(); i++) payload.push_back(blob[i]);

    SSHUserAuthRequest request;
    ASSERT_TRUE(parse_userauth_request(payload, request));

    ASSERT_FALSE(request.has_signature);
    ASSERT_EQ((int)request.pubkey_blob.size(), (int)blob.size());
}

TEST(sshparse, an_ed25519_blob_yields_its_raw_key)
{
    pdiutil::vector<uint8_t> blob;
    appendString(blob, "ssh-ed25519");
    appendBlob(blob, 32);

    pdiutil::vector<uint8_t> raw;
    ASSERT_TRUE(extract_ed25519_blob_field(blob, raw, 32));
    ASSERT_EQ((int)raw.size(), 32);
}

TEST(sshparse, an_ed25519_signature_blob_yields_its_raw_signature)
{
    pdiutil::vector<uint8_t> blob;
    appendString(blob, "ssh-ed25519");
    appendBlob(blob, 64);

    pdiutil::vector<uint8_t> raw;
    ASSERT_TRUE(extract_ed25519_blob_field(blob, raw, 64));
    ASSERT_EQ((int)raw.size(), 64);
}

TEST(sshparse, a_blob_of_the_wrong_size_is_refused)
{
    pdiutil::vector<uint8_t> blob;
    appendString(blob, "ssh-ed25519");
    appendBlob(blob, 31);

    pdiutil::vector<uint8_t> raw;
    ASSERT_FALSE(extract_ed25519_blob_field(blob, raw, 32));
}

TEST(sshparse, a_channel_request_carries_its_type_and_the_rest)
{
    pdiutil::vector<uint8_t> payload;
    payload.push_back(SSH2_MSG_CHANNEL_REQUEST);
    appendUint32(payload, 0);
    appendString(payload, "pty-req");
    payload.push_back(1);
    appendString(payload, "xterm-256color");
    appendUint32(payload, 80);
    appendUint32(payload, 24);

    SSHChannelRequest request;
    ASSERT_TRUE(parse_channel_request(payload, request));

    ASSERT_STREQ(request.request_type.c_str(), "pty-req");
    ASSERT_TRUE(request.want_reply);
    ASSERT_TRUE(request.request_specific_data.size() > 0);
}

TEST(sshparse, channel_data_carries_the_bytes_the_client_typed)
{
    pdiutil::vector<uint8_t> payload;
    payload.push_back(SSH2_MSG_CHANNEL_DATA);
    appendUint32(payload, 0);
    appendString(payload, "ls -l\r\n");

    SSHChannelData data;
    ASSERT_EQ(parse_channel_data_request(payload, data), 0);
    ASSERT_EQ((int)data.data.size(), 7);
    ASSERT_EQ(data.data[0], (uint8_t)'l');
}

TEST(sshparse, channel_data_filling_the_payload_exactly_is_accepted)
{
    // the same boundary as the string case, on the path a shell session uses
    // for every keystroke
    pdiutil::vector<uint8_t> payload;
    payload.push_back(SSH2_MSG_CHANNEL_DATA);
    appendUint32(payload, 0);
    appendString(payload, "x");

    SSHChannelData data;
    ASSERT_EQ(parse_channel_data_request(payload, data), 0);
    ASSERT_EQ((int)data.data.size(), 1);
}

TEST(sshparse, channel_data_is_moved_out_of_the_packet_so_a_squeezed_heap_still_reads_it_whole)
{
    pdiutil::vector<uint8_t> payload;
    payload.push_back(SSH2_MSG_CHANNEL_DATA);
    appendUint32(payload, 0);
    appendBlob(payload, 1100);

    SSHChannelData data;
    pdiutil::enable_heap_check();
    __i_dvc_ctrl.capMaxFreeBlock(100);
    int parsed = parse_channel_data_request(payload, data);
    __i_dvc_ctrl.capMaxFreeBlock(0);

    ASSERT_EQ(parsed, 0);
    ASSERT_EQ((int)data.data.size(), 1100);
    ASSERT_EQ(data.data[0], (uint8_t)0);
    ASSERT_EQ(data.data[1099], (uint8_t)(1099 & 0xFF));
}

TEST(sshparse, channel_data_the_squeezed_heap_still_holds_is_read_whole)
{
    pdiutil::vector<uint8_t> payload;
    payload.push_back(SSH2_MSG_CHANNEL_DATA);
    appendUint32(payload, 0);
    appendBlob(payload, 900);

    SSHChannelData data;
    pdiutil::enable_heap_check();
    __i_dvc_ctrl.capMaxFreeBlock(1000);
    int parsed = parse_channel_data_request(payload, data);
    __i_dvc_ctrl.capMaxFreeBlock(0);

    ASSERT_EQ(parsed, 0);
    ASSERT_EQ((int)data.data.size(), 900);
    ASSERT_EQ(data.data[899], (uint8_t)(899 & 0xFF));
}

TEST(sshparse, a_malformed_channel_data_under_a_squeezed_heap_is_still_reported_malformed)
{
    pdiutil::vector<uint8_t> payload;
    payload.push_back(SSH2_MSG_CHANNEL_DATA);
    appendUint32(payload, 0);
    appendUint32(payload, 2000);
    payload.push_back('x');

    SSHChannelData data;
    pdiutil::enable_heap_check();
    __i_dvc_ctrl.capMaxFreeBlock(1000);
    int parsed = parse_channel_data_request(payload, data);
    __i_dvc_ctrl.capMaxFreeBlock(0);

    ASSERT_EQ(parsed, (int)PDI_ERR_CORRUPT);
}

TEST(sshparse, empty_channel_data_is_read_as_empty)
{
    pdiutil::vector<uint8_t> payload;
    payload.push_back(SSH2_MSG_CHANNEL_DATA);
    appendUint32(payload, 0);
    appendUint32(payload, 0);

    SSHChannelData data;
    ASSERT_EQ(parse_channel_data_request(payload, data), 0);
    ASSERT_EQ((int)data.data.size(), 0);
}

namespace
{
    bool exchangeHashOf(uint8_t secretbyte, uint8_t *out)
    {
        pdiutil::string cv("SSH-2.0-a"), sv("SSH-2.0-b");
        pdiutil::vector<uint8_t> ckex, skex, hostkey, qc(32, 0x11), qs(32, 0x22);
        ckex.push_back(1); ckex.push_back(2); ckex.push_back(3);
        skex.push_back(4);
        hostkey.push_back(5); hostkey.push_back(6);
        uint8_t secret[32];
        memset(secret, secretbyte, sizeof(secret));
        return build_exchange_hash(cv, sv, ckex, skex, hostkey, qc, qs, secret, out);
    }

    void hexOf(const uint8_t *in, char *out)
    {
        const char *digits = "0123456789abcdef";
        for (int i = 0; i < 32; i++)
        {
            out[2 * i] = digits[in[i] >> 4];
            out[2 * i + 1] = digits[in[i] & 0x0F];
        }
        out[64] = 0;
    }
}

TEST(sshparse, the_exchange_hash_is_the_sha256_of_every_field_in_order)
{
    uint8_t h[32];
    char hex[65];
    ASSERT_TRUE(exchangeHashOf(0x33, h));
    hexOf(h, hex);
    ASSERT_STREQ(hex, "c4f499722ffb6027834f3c4f7ef71daaa4d1f3e4c23d35810696527aad031f69");
}

TEST(sshparse, the_exchange_hash_carries_a_shared_secret_with_its_top_bit_set_as_a_positive_mpint)
{
    uint8_t h[32];
    char hex[65];
    ASSERT_TRUE(exchangeHashOf(0xAA, h));
    hexOf(h, hex);
    ASSERT_STREQ(hex, "453798e1c4bd6646fb599fe7e8f72b0386f78b624afac32a02f210017b02b9b4");
}

TEST(sshparse, an_exchange_hash_the_heap_cannot_build_is_refused_not_hashed_short)
{
    uint8_t h[32];
    memset(h, 0x5A, sizeof(h));
    pdiutil::enable_heap_check();
    __i_dvc_ctrl.capMaxFreeBlock(100);
    bool built = exchangeHashOf(0x33, h);
    __i_dvc_ctrl.capMaxFreeBlock(0);

    ASSERT_FALSE(built);
    ASSERT_EQ(h[0], (uint8_t)0x5A);
}

TEST(sshparse, a_channel_data_length_past_the_payload_is_refused)
{
    pdiutil::vector<uint8_t> payload;
    payload.push_back(SSH2_MSG_CHANNEL_DATA);
    appendUint32(payload, 0);
    appendUint32(payload, 0xFFFFFFFF);
    payload.push_back('x');

    SSHChannelData data;
    ASSERT_EQ(parse_channel_data_request(payload, data), (int)PDI_ERR_CORRUPT);
}

#endif
