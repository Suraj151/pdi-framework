/***************************** Light Weight SSH *******************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 6th Apr 2025
******************************************************************************/

#include <config/Config.h>

#if defined(ENABLE_SSH_SERVICE)

#include "SSHServiceUtil.h"
#include <helpers/ClientHelper.h>
#include <utility/crypto/asymmetric/curve25519/curve25519.h>
#include <utility/crypto/hash/sha256.h>
#include <utility/crypto/hmac/hmac_sha1.h>
#include <utility/crypto/asymmetric/ed25519/ed25519.h>

using namespace LWSSH;


/**
 * @brief Parses the received SSH packet.
 * @param session Active session of ssh
 * @param packet Reference to the ssh_packet structure to fill with parsed data.
 * @return true if the packet is valid, false otherwise.
 */
int LWSSH::parse_received_packet(LWSSHSession* session, ssh_packet &packet){

    if (!session || !session->m_client) {
        return -1; // No active session or client
    }
    
    if (session->m_client->available() < 5) {
        return 1; // Not enough data available for a valid packet
    }

    packet.payload.clear();

    // Read the first 4 bytes for packet length
    uint8_t lenbuf[4];
    for (int i = 0; i < 4; ++i) {
        lenbuf[i] = session->m_client->read();
    }
    uint32_t packet_length = (lenbuf[0] << 24) | (lenbuf[1] << 16) | (lenbuf[2] << 8) | lenbuf[3];

    // Read the next byte for padding length
    uint8_t padding_length = session->m_client->read();

    // Read the payload (packet_length - padding_length - 1)
    uint32_t payload_length = packet_length - padding_length - 1;
    for (uint32_t i = 0; i < payload_length && session->m_client->available() > 0; ++i) {
        packet.payload.push_back(session->m_client->read());
        __i_dvc_ctrl.yield();
    }

    // Read and discard the padding bytes
    for (uint8_t i = 0; i < padding_length && session->m_client->available() > 0; ++i) {
        session->m_client->read();
    }

    bool bStatus = (packet.payload.size() == payload_length && packet.payload[0] < 101);
    
    session->packets_seq_num_ctos++;

    return bStatus ? 0 : -1;
}

/**
 * @brief Parses an encrypted SSH packet.
 * @param session Active session of ssh
 * @param packet Reference to the ssh_packet structure to fill with parsed data.
 * @return 0 on success, -1 on failure.
 */
int LWSSH::parse_encrypted_packet(LWSSHSession* session, ssh_packet &packet) {

    if (!session || !session->m_client) {
        return -1; // No active session or client
    }
    
    // check whether it has minimum length as per hmac-sha1 MAC + payload
    if (session->m_client->available() < (4 + 16 + 20)) {
        return 1; // Not enough data available for a valid packet
    }

    // take header part to get packet length
    pdiutil::vector<uint8_t> header;
    for (uint32_t i = 0; i < 4; ++i) {
        header.push_back(session->m_client->read());
        __i_dvc_ctrl.yield();
    }
    pdiutil::vector<uint8_t> packetvec;
    for (uint32_t i = 0; i < 4; ++i) {
        packetvec.push_back(header[i]);
    }
    AES_ctx temp = session->aes_ctx_ctos;
    AES_CTR_xcrypt_buffer(&temp, packetvec.data(), packetvec.size());
    uint32_t packet_length = (packetvec[0] << 24) | (packetvec[1] << 16) | (packetvec[2] << 8) | packetvec[3];
    
    // Once decrypt packet length reset the header part as it was received
    packetvec.clear();
    for (uint32_t i = 0; i < 4; ++i) {
        packetvec.push_back(header[i]);
    }

    // take next packet_len bytes + 20 byte MAC part to form complete packet
    for (uint32_t i = 0; i < packet_length + 20; ++i) {
        packetvec.push_back(session->m_client->read());
        __i_dvc_ctrl.yield();
    }

    // seperate out MAC data
    pdiutil::vector<uint8_t> recv_mac(20);
    for (int32_t i = 20; i > 0; i--) {
        recv_mac[i-1] = packetvec.back();
        packetvec.pop_back();
    }

    // decrypt received payload seperated from MAC
    AES_CTR_xcrypt_buffer(&session->aes_ctx_ctos, packetvec.data(), packetvec.size());

    // Prepare sequence number (big-endian)
    uint8_t packetseqbuf[4];
    packetseqbuf[0] = (session->packets_seq_num_ctos >> 24) & 0xFF;
    packetseqbuf[1] = (session->packets_seq_num_ctos >> 16) & 0xFF;
    packetseqbuf[2] = (session->packets_seq_num_ctos >> 8) & 0xFF;
    packetseqbuf[3] = (session->packets_seq_num_ctos) & 0xFF;

    // Prepare MAC input: [packet sequence][encrypted header][encrypted rest]
    pdiutil::vector<uint8_t> mac_input;
    mac_input.insert(mac_input.end(), packetseqbuf, packetseqbuf + 4);
    mac_input.insert(mac_input.end(), packetvec.begin(), packetvec.end());

    uint8_t computed_mac[20];
    hmac_sha1(session->derived_mac_key_ctos, 20, mac_input.data(), mac_input.size(), computed_mac);

    if (memcmp(recv_mac.data(), computed_mac, 20) != 0) {
        // MAC verification failed
        return -1;
    }else{
        // MAC verification succeeded.
        session->packets_seq_num_ctos++;
    }

    __i_dvc_ctrl.yield();

    // Parse payload
    uint8_t padding_length = packetvec[4];
    uint32_t payload_length = packet_length - padding_length - 1;

    packet.payload.clear();
    for (uint32_t i = 0; i < payload_length; ++i) {
        packet.payload.push_back(packetvec[5 + i]);
    }

    bool bStatus = (packet.payload.size() == payload_length && packet.payload[0] < 101);
    
    return bStatus ? 0 : -1;
}

/**
 * @brief Parses the name list from the SSH packet payload.
 * @param payload The payload data to parse.
 * @param offset Reference to the offset in the payload.
 * @return whether the parsing was successful or not.
 * @param name_list Reference to the ssh_name_list to fill with parsed names.
 * @note The name list is expected to be a comma-separated string.
 *       The first 4 bytes of the payload indicate the length of the name list.
 *       The names are extracted and stored in the provided ssh_name_list.
 */
bool LWSSH::parse_name_list(const pdiutil::vector<uint8_t>& payload, uint32_t& offset, ssh_name_list& name_list) {
    if (offset + 4 > payload.size()) return false;
    uint32_t len = (payload[offset] << 24) | (payload[offset+1] << 16) | (payload[offset+2] << 8) | payload[offset+3];
    offset += 4;
    if (offset + len > payload.size()) return false;
    pdiutil::string names(reinterpret_cast<const char*>(&payload[offset]), len);
    offset += len;
    uint32_t start = 0, end;
    while ((end = names.find(',', start)) != pdiutil::string::npos) {
        name_list.push_back(names.substr(start, end - start));
        start = end + 1;
    }
    if (start < names.size()) name_list.push_back(names.substr(start));
    return true;
}

/**
 * @brief Parses the KEX_INIT payload from the SSH packet.
 * @param payload The payload data to parse.
 * @param fields fields to be filled post parsing
 * @return whether fields are parsed successfully
 */
bool LWSSH::parse_kex_init_fields(const pdiutil::vector<uint8_t>& payload, SSHKexInitFields& fields) {
    uint32_t offset = 0;

    // Message code (should be 0x14)
    if (payload.size() < 17 || payload[0] != SSH2_MSG_KEXINIT) return false;
    offset = 1;

    // Cookie (16 bytes)
    fields.cookie.assign(payload.begin() + offset, payload.begin() + offset + 16);
    offset += 16;

    // 11 name-lists
    if (!parse_name_list(payload, offset, fields.kex_algorithms)) return false;
    if (!parse_name_list(payload, offset, fields.server_host_key_algorithms)) return false;
    if (!parse_name_list(payload, offset, fields.encryption_algorithms_ctos)) return false;
    if (!parse_name_list(payload, offset, fields.encryption_algorithms_stoc)) return false;
    if (!parse_name_list(payload, offset, fields.mac_algorithms_ctos)) return false;
    if (!parse_name_list(payload, offset, fields.mac_algorithms_stoc)) return false;
    if (!parse_name_list(payload, offset, fields.compression_algorithms_ctos)) return false;
    if (!parse_name_list(payload, offset, fields.compression_algorithms_stoc)) return false;
    if (!parse_name_list(payload, offset, fields.languages_ctos)) return false;
    if (!parse_name_list(payload, offset, fields.languages_stoc)) return false;

    // first_kex_packet_follows (boolean)
    if (offset >= payload.size()) return false;
    fields.first_kex_packet_follows = (payload[offset++] != 0);

    // reserved (uint32)
    if (offset + 4 > payload.size()) return false;
    fields.reserved = (payload[offset] << 24) | (payload[offset+1] << 16) | (payload[offset+2] << 8) | payload[offset+3];
    offset += 4;

    return true;
}

/**
 * @brief Appends a name list to the output vector.
 *        Helper to serialize a name-list (RFC 4251)
 * @param out The output vector to append to.
 * @param names The list of names to append.
 */
void LWSSH::append_name_list(pdiutil::vector<uint8_t>& out, const ssh_name_list& names) {

    pdiutil::string joined;
    for (size_t i = 0; i < names.size(); ++i) {
        if (i > 0) joined += ",";
        joined += names[i];
    }
    uint32_t len = joined.size();
    out.push_back((len >> 24) & 0xFF);
    out.push_back((len >> 16) & 0xFF);
    out.push_back((len >> 8) & 0xFF);
    out.push_back(len & 0xFF);
    out.insert(out.end(), joined.begin(), joined.end());
}

/**
 * @brief Prepares the SSH KEXINIT message payload.
 * @param payload The payload vector to fill with the KEXINIT message.
 */
void LWSSH::prepare_server_kexinit(pdiutil::vector<uint8_t> &payload){
    // clear existing ssh packet payload
    payload.clear();

    // 1. Message code
    payload.push_back(SSH2_MSG_KEXINIT); // SSH_MSG_KEXINIT

    // 2. Cookie (16 random bytes)
    // srand(time(nullptr));
    for (int i = 0; i < 16; ++i) {
        payload.push_back(rand() & 0xFF);
    }

    // 3. Name-lists (choose algorithms you support)
    ssh_name_list kex_algorithms; kex_algorithms.push_back("curve25519-sha256");
    ssh_name_list server_host_key_algorithms; server_host_key_algorithms.push_back("ssh-ed25519");
    ssh_name_list encryption_algorithms; encryption_algorithms.push_back("aes128-ctr");
    ssh_name_list mac_algorithms; mac_algorithms.push_back("hmac-sha1");
    ssh_name_list compression_algorithms; compression_algorithms.push_back("none");
    ssh_name_list languages; // empty

    append_name_list(payload, kex_algorithms);
    append_name_list(payload, server_host_key_algorithms);
    append_name_list(payload, encryption_algorithms); // encryption_algorithms_client_to_server
    append_name_list(payload, encryption_algorithms); // encryption_algorithms_server_to_client
    append_name_list(payload, mac_algorithms);        // mac_algorithms_client_to_server
    append_name_list(payload, mac_algorithms);        // mac_algorithms_server_to_client
    append_name_list(payload, compression_algorithms);// compression_algorithms_client_to_server
    append_name_list(payload, compression_algorithms);// compression_algorithms_server_to_client
    append_name_list(payload, languages);             // languages_client_to_server
    append_name_list(payload, languages);             // languages_server_to_client

    // 4. first_kex_packet_follows (boolean)
    payload.push_back(0x00); // false

    // 5. reserved (uint32)
    payload.push_back(0x00);
    payload.push_back(0x00);
    payload.push_back(0x00);
    payload.push_back(0x00);
}

/**
 * @brief Encrypt the payload and append MAC.
 * @param payload The payload vector. same vector will be modified to form encrypted
 */
void LWSSH::encrypt_ssh_payload(LWSSHSession* session, pdiutil::vector<uint8_t> &payload){

    // Prepare sequence number (big-endian)
    uint8_t packetseqbuf[4];
    packetseqbuf[0] = (session->packets_seq_num_stoc >> 24) & 0xFF;
    packetseqbuf[1] = (session->packets_seq_num_stoc >> 16) & 0xFF;
    packetseqbuf[2] = (session->packets_seq_num_stoc >> 8) & 0xFF;
    packetseqbuf[3] = (session->packets_seq_num_stoc) & 0xFF;

    // put packetseqbuf in payload at front. (packet sequence || ssh packet)
    payload.insert(payload.begin(), packetseqbuf, packetseqbuf + sizeof(packetseqbuf));

    uint8_t computed_mac[20];
    hmac_sha1(session->derived_mac_key_stoc, sizeof(computed_mac), payload.data(), payload.size(), computed_mac);

    // remove packetseqbuf in payload from front. packet sequence can not be sent over wire
    payload.erase(payload.begin(), payload.begin() + sizeof(packetseqbuf));

    // Encrypt packet (except MAC)
    AES_CTR_xcrypt_buffer(&session->aes_ctx_stoc, payload.data(), payload.size());

    // Append MAC to encrypted packet
    payload.insert(payload.end(), computed_mac, computed_mac + sizeof(computed_mac));
}

/**
 * @brief Prepares the payload in ssh packet form.
 * @param payload The payload vector. same vector will be modified to form ssh packet
 */
void LWSSH::prepare_ssh_packet(pdiutil::vector<uint8_t> &payload, int32_t block_size){

    // --- SSH packet wrapper ---
    // Calculate padding to make total length a multiple of 8 (minimum 4 bytes)
    // size_t block_size = 16;
    size_t payload_len = payload.size();
    size_t min_packet_len = payload_len + 1 + 4; // +1 for padding_length, +4 for packet_length
    size_t padding_len = block_size - (min_packet_len % block_size);
    if (padding_len < 4) padding_len += block_size;
    uint32_t packet_length = payload_len + padding_len + 1; // payload + padding + padding_length byte

    // Write packet_length (4 bytes, big endian)
    uint8_t header[5];
    header[0] = (packet_length >> 24) & 0xFF;
    header[1] = (packet_length >> 16) & 0xFF;
    header[2] = (packet_length >> 8) & 0xFF;
    header[3] = packet_length & 0xFF;
    header[4] = padding_len;

    // put header in payload at front
    payload.insert(payload.begin(), header, header + sizeof(header));

    // add random padding
    for (size_t i = 0; i < padding_len; ++i) {
        payload.push_back(rand() & 0xFF);
    }
}

/**
 * @brief Sends the SSH packet to the client.
 * @param session The SSH session to use.
 * @param payload The payload vector to be send.
 * @param encrypt Whether to encrypt or not the packet
 * @return true if the message was sent successfully, false otherwise.
 */
bool LWSSH::send_server_ssh_packet(LWSSHSession* session, pdiutil::vector<uint8_t> &payload, bool encrypt) {
    if (!session || !session->m_client) {
        return false; // No active session or client
    }

    __i_dvc_ctrl.yield();

    // Encrypt if provided
    if(encrypt && session->m_state > LWSSHSession::SESSION_STATE_NEWKEYS_RECEIVED){

        prepare_ssh_packet(payload, 16);
        encrypt_ssh_payload(session, payload);
        
        if(!sendPacket(session->m_client, payload.data(), payload.size())) return false;
    }else{

        // --- SSH packet wrapper ---
        // Calculate padding to make total length a multiple of 8 (minimum 4 bytes)
        size_t block_size = 8;
        size_t payload_len = payload.size();
        size_t min_packet_len = payload_len + 1 + 4; // +1 for padding_length, +4 for packet_length
        size_t padding_len = block_size - (min_packet_len % block_size);
        if (padding_len < 4) padding_len += block_size;
        uint32_t packet_length = payload_len + padding_len + 1; // payload + padding + padding_length byte

        // Write packet_length (4 bytes, big endian)
        uint8_t header[5];
        header[0] = (packet_length >> 24) & 0xFF;
        header[1] = (packet_length >> 16) & 0xFF;
        header[2] = (packet_length >> 8) & 0xFF;
        header[3] = packet_length & 0xFF;
        header[4] = padding_len;

        // Send header
        if(session->m_client->write(header, 5) != 5) return false;
    
        // Send payload
        if(session->m_client->write(payload.data(), payload.size()) != payload.size()) return false;
        // return (session->m_client->write(payload.data(), payload.size()) == payload.size());

        // Send random padding
        for (size_t i = 0; i < padding_len; ++i) {
            uint8_t pad = rand() & 0xFF;
            if(session->m_client->write(pad) != 1) return false;
        }
    }

    // increment sent packet sequence
    session->packets_seq_num_stoc++;    

    return true;
}

/**
 * @brief Parses the ECDH_INIT packet from the SSH payload.
 * @param payload The payload data to parse.
 * @param packet Reference to the EcdhInitPacket structure to fill with parsed data.
 * @return whether the parsing was successful or not.
 */
bool LWSSH::parse_kex_ecdh_init(const pdiutil::vector<uint8_t> &payload, EcdhInitPacket &packet){
    size_t offset = 0;

    // Check message type
    if (payload.size() < 1 || payload[0] != SSH2_MSG_KEXDH_INIT) // SSH_MSG_KEX_ECDH_INIT
        return false;
    offset = 1;

    // Parse "string" (client public key)
    if (offset + 4 > payload.size()) return false;
    uint32_t len = (payload[offset] << 24) | (payload[offset+1] << 16) | (payload[offset+2] << 8) | payload[offset+3];
    offset += 4;
    if (len != 32 || offset + len > payload.size()) return false;

    packet.client_pubkey.assign(payload.begin() + offset, payload.begin() + offset + len);
    return true;
}

/**
 * @brief Appends an SSH string to the output vector.
 *        Helper to serialize a string (RFC 4251)
 *        (4-byte length + data)
 * @param out The output vector to append to.
 * @param data The data to append as an SSH string.
 */
void LWSSH::append_ssh_string(pdiutil::vector<uint8_t>& out, const pdiutil::vector<uint8_t>& data) {
    uint32_t len = data.size();
    out.push_back((len >> 24) & 0xFF);
    out.push_back((len >> 16) & 0xFF);
    out.push_back((len >> 8) & 0xFF);
    out.push_back(len & 0xFF);
    out.insert(out.end(), data.begin(), data.end());
}

/**
 * @brief Appends an SSH string to the output vector.
 *        Helper to serialize a string (RFC 4251)
 *        (4-byte length + data)
 * @param out The output vector to append to.
 * @param data The data to append as an SSH string.
 * @param length data length.
 */
void LWSSH::append_ssh_string(pdiutil::vector<uint8_t>& out, const char* data, uint32_t length) {
    out.push_back((length >> 24) & 0xFF);
    out.push_back((length >> 16) & 0xFF);
    out.push_back((length >> 8) & 0xFF);
    out.push_back(length & 0xFF);
    out.insert(out.end(), data, data + length);
}

/**
 * @brief read an SSH string to the output string.
 * @param payload payload from which string has to be read
 * @param str The output string to read in.
 * @param offset payload offset from which needs to read strign data.
 */
bool LWSSH::read_ssh_string(const pdiutil::vector<uint8_t>& payload, pdiutil::string& str, int32_t &offset){

    if (offset + 4 > payload.size()) return false;
    uint32_t len = (payload[offset] << 24) | (payload[offset+1] << 16) | (payload[offset+2] << 8) | payload[offset+3];
    offset += 4;
    if (offset + len > payload.size()) return false;
    str.assign((const char*)&payload[offset], len);
    offset += len;
    return true;
}

/**
 * @brief read an SSH string to the output string.
 * @param payload payload from which string has to be read
 * @param str The output string to read in vector.
 * @param offset payload offset from which needs to read strign data.
 */
bool LWSSH::read_ssh_string(const pdiutil::vector<uint8_t>& payload, pdiutil::vector<uint8_t>& str, int32_t &offset){

    if (offset + 4 > payload.size()) return false;
    uint32_t len = (payload[offset] << 24) | (payload[offset+1] << 16) | (payload[offset+2] << 8) | payload[offset+3];
    offset += 4;
    if (offset + len > payload.size()) return false;
    for (uint32_t i = 0; i < len; i++){
        str.push_back(payload[offset+i]);
    }
    offset += len;
    return true;
}

/**
 * @brief Prepare the channel open reply and send.
 * @param session The SSH session to use.
 * @param window_size Window size.
 * @param max_packet Max packet size.
 * @return true if the message was sent successfully, false otherwise.
 */
bool LWSSH::send_channel_open_confirmation(LWSSHSession *session, uint32_t window_size, uint32_t max_packet){

    // Build SSH2_MSG_CHANNEL_OPEN_CONFIRMATION
    pdiutil::vector<uint8_t> reply;
    reply.push_back(SSH2_MSG_CHANNEL_OPEN_CONFIRMATION);
    reply.push_back((session->current_channel.client_channel_id >> 24) & 0xFF);
    reply.push_back((session->current_channel.client_channel_id >> 16) & 0xFF);
    reply.push_back((session->current_channel.client_channel_id >> 8) & 0xFF);
    reply.push_back(session->current_channel.client_channel_id & 0xFF);

    reply.push_back((session->current_channel.server_channel_id >> 24) & 0xFF);
    reply.push_back((session->current_channel.server_channel_id >> 16) & 0xFF);
    reply.push_back((session->current_channel.server_channel_id >> 8) & 0xFF);
    reply.push_back(session->current_channel.server_channel_id & 0xFF);

    reply.push_back((window_size >> 24) & 0xFF);
    reply.push_back((window_size >> 16) & 0xFF);
    reply.push_back((window_size >> 8) & 0xFF);
    reply.push_back(window_size & 0xFF);

    reply.push_back((max_packet >> 24) & 0xFF);
    reply.push_back((max_packet >> 16) & 0xFF);
    reply.push_back((max_packet >> 8) & 0xFF);
    reply.push_back(max_packet & 0xFF);

    // Send as SSH packet (encrypted)
    return send_server_ssh_packet(session, reply, true);
}

/**
 * @brief Send channel data to client.
 * @param session The SSH session to use.
 * @param data data to send.
 * @param length data length.
 * @return true if the message was sent successfully, false otherwise.
 */
bool LWSSH::send_channel_data(LWSSHSession *session, const char *data, uint32_t length){
    pdiutil::vector<uint8_t> chdatapayload;
    chdatapayload.push_back(SSH2_MSG_CHANNEL_DATA); // 94

    // Use the client's channel id as recipient
    uint32_t client_channel_id = session->current_channel.client_channel_id;
    chdatapayload.push_back((client_channel_id >> 24) & 0xFF);
    chdatapayload.push_back((client_channel_id >> 16) & 0xFF);
    chdatapayload.push_back((client_channel_id >> 8) & 0xFF);
    chdatapayload.push_back(client_channel_id & 0xFF);

    // Prepare your data as an SSH string
    append_ssh_string(chdatapayload, data, length); // This adds 4-byte length + data

    // Send the packet (encrypted)
    return send_server_ssh_packet(session, chdatapayload, true);
}

/**
 * @brief Appends an SSH MPINT (multi-precision integer) to the output vector.
 *        Helper to serialize an MPINT (RFC 4251)
 *        (4-byte length + optional leading zero + data)
 * @param out The output vector to append to.
 * @param data Pointer to the MPINT data.
 * @param len Length of the MPINT data.
 */
void LWSSH::append_mpint(pdiutil::vector<uint8_t>& out, const uint8_t* data, int32_t len) {
    // Remove leading zeros
    int32_t first_nonzero = 0;
    while (first_nonzero < len && data[first_nonzero] == 0) ++first_nonzero;
    int32_t mpint_len = len - first_nonzero;
    // If the first byte >= 0x80, prepend a zero byte
    bool prepend_zero = (mpint_len > 0 && data[first_nonzero] & 0x80);
    uint32_t total_len = mpint_len + (prepend_zero ? 1 : 0);
    out.push_back((total_len >> 24) & 0xFF);
    out.push_back((total_len >> 16) & 0xFF);
    out.push_back((total_len >> 8) & 0xFF);
    out.push_back(total_len & 0xFF);
    if (prepend_zero) out.push_back(0x00);
    out.insert(out.end(), data + first_nonzero, data + len);
}

/**
 * @brief Builds the exchange hash for the key exchange process.
 * @param client_version The client's SSH version string.
 * @param server_version The server's SSH version string.
 * @param client_kexinit The client's KEXINIT payload.
 * @param server_kexinit The server's KEXINIT payload.
 * @param server_host_key The server's host key.
 * @param client_ecdh_pub The client's ECDH public key.
 * @param server_ecdh_pub The server's ECDH public key.
 * @param shared_secret The shared secret (Curve25519).
 * @param hash_output The output buffer for the SHA-256 hash.
 */
void LWSSH::build_exchange_hash(
    const pdiutil::string& client_version, // e.g. "SSH-2.0-OpenSSH_9.0"
    const pdiutil::string& server_version, // e.g. "SSH-2.0-LWSSH_0.1"
    const pdiutil::vector<uint8_t>& client_kexinit, // raw KEXINIT payload (not SSH packet)
    const pdiutil::vector<uint8_t>& server_kexinit, // raw KEXINIT payload (not SSH packet)
    const pdiutil::vector<uint8_t>& server_host_key_sshformat, // SSH format
    const pdiutil::vector<uint8_t>& client_ecdh_pub, // 32 bytes
    const pdiutil::vector<uint8_t>& server_ecdh_pub, // 32 bytes
    const uint8_t* shared_secret, // 32 bytes
    uint8_t *hash_output // output buffer for SHA-256 hash
) {
    pdiutil::vector<uint8_t> hash_input;
    pdiutil::vector<uint8_t> cversion(client_version.begin(), client_version.end());
    pdiutil::vector<uint8_t> sversion(server_version.begin(), server_version.end());

    // string V_C (without CRLF)
    append_ssh_string(hash_input, cversion);
    // string V_S (without CRLF)
    append_ssh_string(hash_input, sversion);
    // string I_C
    append_ssh_string(hash_input, client_kexinit);
    // string I_S
    append_ssh_string(hash_input, server_kexinit);
    // string K_S    
    append_ssh_string(hash_input, server_host_key_sshformat);
    // string Q_C
    append_ssh_string(hash_input, client_ecdh_pub);
    // string Q_S
    append_ssh_string(hash_input, server_ecdh_pub);
    // mpint K
    append_mpint(hash_input, shared_secret, 32);

    // Now hash_input contains the data to hash
    sha256(hash_input.data(), hash_input.size(), hash_output); // Use your SHA-256 implementation
}

/**
 * @brief Creates ephemeral server keys for the SSH session.
 *        Generates a random seed for ED25519 key generation
 *        and Curve25519 key pair using that seed.
 * @param pubkey Output vector to store the public key (Curve25519).
 * @param privkey Output vector to store the private key (Curve25519).
 * @return true if keys were created successfully, false otherwise.
 */
bool LWSSH::create_server_ephemeral_keys(pdiutil::vector<uint8_t> &pubkey, pdiutil::vector<uint8_t> &privkey, pdiutil::vector<uint8_t> *ed25519privkey_seedpubfmt){

    uint8_t curve25519_priv_key[CURVE25519_PRIVKEY_SIZE];
    uint8_t curve25519_pub_key[CURVE25519_PUBKEY_SIZE];
    memset(curve25519_priv_key, 0, sizeof(curve25519_priv_key));
    memset(curve25519_pub_key, 0, sizeof(curve25519_pub_key));
    pubkey.clear();
    privkey.clear();

    // 1. Generate ephemeral Curve25519 key pair for server
    if( ed25519privkey_seedpubfmt && ed25519privkey_seedpubfmt->size() >= CURVE25519_PRIVKEY_SIZE ) {
        curve25519_create_keypair_with_ed25519privkey(curve25519_pub_key, curve25519_priv_key, ed25519privkey_seedpubfmt->data());
    }else{
        curve25519_create_keypair(curve25519_pub_key, curve25519_priv_key);
    }

    pubkey.insert(pubkey.end(), curve25519_pub_key, curve25519_pub_key + sizeof(curve25519_pub_key));
    privkey.insert(privkey.end(), curve25519_priv_key, curve25519_priv_key + sizeof(curve25519_priv_key));
    
    return (pubkey.size() == CURVE25519_PUBKEY_SIZE && privkey.size() == CURVE25519_PRIVKEY_SIZE);
}

/**
 * @brief Prepare the SSH_MSG_KEX_ECDH_REPLY message.
 * @param session The SSH session to use.
 * @param client_pubkey The client's public key (32 bytes for Curve25519).
 * @param server_host_pubkey The server's host public key (Ed25519).
 * @param server_host_privkey The server's host private key (Ed25519).
 * @param payload builded reply message payload.
 * @return true if the message was build successfully, false otherwise.
 */
bool LWSSH::prepare_server_ecdh_reply(LWSSHSession* session,
                            const pdiutil::vector<uint8_t>& client_pubkey,
                            const pdiutil::vector<uint8_t>& server_host_pubkey,
                            const pdiutil::vector<uint8_t>& server_host_privkey,
                            pdiutil::vector<uint8_t>& payload) {

    if (!session || !session->m_client) {
        return false; // No active session or client
    }

    // disable watchdog as key operations may take time
    __i_dvc_ctrl.disableWdt();

    // 1. Generate ephemeral Curve25519 key pair for server
    if (!create_server_ephemeral_keys(session->m_server_ephermeral_pubkey, 
        session->m_server_ephermeral_privkey, (pdiutil::vector<uint8_t>*)&server_host_privkey)) {
        return false; // Key generation failed
    }

    __i_dvc_ctrl.yield();

    // 2. Compute shared secret
    crypto_scalarmult(session->m_shared_secret_key, session->m_server_ephermeral_privkey.data(), client_pubkey.data());

    __i_dvc_ctrl.yield();

    // server host key blob (as SSH string)
    pdiutil::vector<uint8_t> hostkey_blob;
    pdiutil::string keytype = "ssh-ed25519";
    append_ssh_string(hostkey_blob, pdiutil::vector<uint8_t>(keytype.begin(), keytype.end()));
    append_ssh_string(hostkey_blob, server_host_pubkey);

    // 3. Build exchange hash H (see RFC 4253, section 8, and RFC 8731)
    build_exchange_hash(
        session->m_client_version, 
        session->m_server_version,
        session->m_client_kexinit.payload,
        session->m_server_kexinit.payload,
        hostkey_blob, 
        client_pubkey, 
        session->m_server_ephermeral_pubkey, 
        session->m_shared_secret_key,
        session->m_exchange_hash_h
    );

    __i_dvc_ctrl.yield();

    // 4. Sign exchange hash with Ed25519 host key
    pdiutil::vector<uint8_t> signature(64);
    ed25519_sign(signature.data(), session->m_exchange_hash_h, 32,
                 server_host_pubkey.data(), server_host_privkey.data());

    // enable watchdog once time operations done
    __i_dvc_ctrl.enableWdt();
    __i_dvc_ctrl.yield();

    // 5. Build SSH_MSG_KEX_ECDH_REPLY payload
    // pdiutil::vector<uint8_t> payload;
    payload.push_back(SSH2_MSG_KEXDH_REPLY); // SSH_MSG_KEX_ECDH_REPLY

    // a. server host key (as SSH string)
    append_ssh_string(payload, hostkey_blob);

    // b. server ephemeral pubkey (as SSH string)
    append_ssh_string(payload, session->m_server_ephermeral_pubkey);

    // c. signature (as SSH string, must be in SSH format: type + signature)
    pdiutil::vector<uint8_t> sig_blob;
    // Format: string "ssh-ed25519" + string raw signature
    append_ssh_string(sig_blob, pdiutil::vector<uint8_t>(keytype.begin(), keytype.end()));
    append_ssh_string(sig_blob, signature);
    append_ssh_string(payload, sig_blob);

    return true;
}

/**
 * @brief Derives a key from the shared secret and exchange hash.
 *        Implements the key derivation function as per RFC 4253 section 7.2.
 * @param K The shared secret (Curve25519, usually 32 bytes).
 * @param H The exchange hash (SHA-256, 32 bytes).
 * @param session_id The session ID (optional, can be empty).
 * @param key_type The type of key to derive ('A', 'B', 'C', etc.).
 * @param out Output buffer for the derived key.
 */
void LWSSH::derive_key(
    const uint8_t *K, size_t K_len,           // shared secret (mpint, usually 32 bytes)
    const uint8_t *H, size_t H_len,           // exchange hash (SHA-256, 32 bytes)
    const uint8_t *session_id, size_t session_id_len,
    char key_type,                            // 'A', 'B', 'C', etc. (see RFC 4253 7.2)
    uint8_t *out, size_t out_len              // output buffer for derived key
) {
    // 1. KDF input: K || H || key_type || session_id
    uint8_t buf[1024];
    size_t offset = 0;
    memcpy(buf + offset, K, K_len); offset += K_len;
    memcpy(buf + offset, H, H_len); offset += H_len;
    buf[offset++] = key_type;
    memcpy(buf + offset, session_id, session_id_len); offset += session_id_len;

    // 2. Hash it (use SHA-256 for curve25519-sha256)
    uint8_t hash[32];
    sha256(buf, offset, hash);

    // 3. If more bytes needed, repeat with K || H || hash
    if (out_len <= 32) {
        memcpy(out, hash, out_len);
    } else {
        memcpy(out, hash, 32);
        // For longer keys, hash K || H || previous_hash
        // (not needed for AES-128-CTR or HMAC-SHA1)
    }

    __i_dvc_ctrl.yield();
}

/**
 * @brief Parse the user auth request from payload.
 * @param payload payload to parse.
 * @param req user auth structure to fill with data.
 */
bool LWSSH::parse_userauth_request(const pdiutil::vector<uint8_t>& payload, SSHUserAuthRequest& req) {
    
    int32_t offset = 1; // skip message type (already checked == 50)

    if (!read_ssh_string(payload, req.username, offset)) return false;
    if (!read_ssh_string(payload, req.service, offset)) return false;
    if (!read_ssh_string(payload, req.method, offset)) return false;

    if (req.method == "password") {
        if (offset >= payload.size()) return false;
        bool has_old_password = payload[offset++] != 0;
        if (!read_ssh_string(payload, req.password, offset)) return false;
    }

    return true;
}

/**
 * @brief Parse the channel open request.
 * @param session The SSH session to use.
 * @param packet recieved ssh packet.
 */
bool LWSSH::parse_channel_open_request(LWSSHSession *session, ssh_packet &packet){

    if (!session || packet.payload.empty() || packet.payload[0] != SSH2_MSG_CHANNEL_OPEN) {
        return false;
    }

    int32_t offset = 1; // skip message type

    // Parse channel type (SSH string)
    if (!read_ssh_string(packet.payload, session->current_channel.channel_type, offset)) return false;

    // Parse sender channel (uint32)
    if (offset + 4 > packet.payload.size()) return false;
    session->current_channel.client_channel_id =
        (packet.payload[offset] << 24) |
        (packet.payload[offset+1] << 16) |
        (packet.payload[offset+2] << 8) |
        (packet.payload[offset+3]);
    offset += 4;

    // Parse initial window size (uint32)
    if (offset + 4 > packet.payload.size()) return false;
    session->current_channel.window_size =
        (packet.payload[offset] << 24) |
        (packet.payload[offset+1] << 16) |
        (packet.payload[offset+2] << 8) |
        (packet.payload[offset+3]);
    offset += 4;

    // Parse max packet size (uint32)
    if (offset + 4 > packet.payload.size()) return false;
    session->current_channel.max_packet_size =
        (packet.payload[offset] << 24) |
        (packet.payload[offset+1] << 16) |
        (packet.payload[offset+2] << 8) |
        (packet.payload[offset+3]);
    offset += 4;

    session->current_channel.server_channel_id++;

    return true;
}

/**
 * @brief Parse the channel request.
 * @param payload received payload of channel request
 * @param req channel request structure to fill.
 */
bool LWSSH::parse_channel_request(const pdiutil::vector<uint8_t> &payload, SSHChannelRequest &req){

    size_t offset = 1; // skip message type (already checked == 98)

    // Parse recipient channel (uint32)
    if (offset + 4 > payload.size()) return false;
    req.recipient_channel =
        (payload[offset] << 24) | (payload[offset+1] << 16) |
        (payload[offset+2] << 8) | payload[offset+3];
    offset += 4;

    // Parse request type (SSH string)
    if (offset + 4 > payload.size()) return false;
    uint32_t str_len = (payload[offset] << 24) | (payload[offset+1] << 16) |
                       (payload[offset+2] << 8) | payload[offset+3];
    offset += 4;
    if (offset + str_len > payload.size()) return false;
    req.request_type.assign((const char*)&payload[offset], str_len);
    offset += str_len;

    // Parse want_reply (boolean)
    if (offset >= payload.size()) return false;
    req.want_reply = (payload[offset++] != 0);

    // The rest is request-specific data
    req.request_specific_data.assign(payload.begin() + offset, payload.end());

    return true;
}

/**
 * @brief Parse the channel request type pty-req.
 * @param session The SSH session to use.
 * @param data recieved payload.
 */
bool LWSSH::parse_channel_request_pty_req(LWSSHSession *session, const pdiutil::vector<uint8_t> &data){
 
    if (!session || data.empty()) {
        return false;
    }
    
    int32_t offset = 0;
    SSHPtyReq &req = session->current_channel.pty_req;

    // Parse TERM string
    if (!read_ssh_string(data, req.term, offset)) return false;
    // Parse width_chars
    if (offset + 4 > data.size()) return false;
    req.width_chars = (data[offset] << 24) | (data[offset+1] << 16) | (data[offset+2] << 8) | data[offset+3];
    offset += 4;
    // Parse height_rows
    if (offset + 4 > data.size()) return false;
    req.height_rows = (data[offset] << 24) | (data[offset+1] << 16) | (data[offset+2] << 8) | data[offset+3];
    offset += 4;
    // Parse width_pixels
    if (offset + 4 > data.size()) return false;
    req.width_pixels = (data[offset] << 24) | (data[offset+1] << 16) | (data[offset+2] << 8) | data[offset+3];
    offset += 4;
    // Parse height_pixels
    if (offset + 4 > data.size()) return false;
    req.height_pixels = (data[offset] << 24) | (data[offset+1] << 16) | (data[offset+2] << 8) | data[offset+3];
    offset += 4;
    // Parse terminal modes (SSH string)

    if (!read_ssh_string(data, req.terminal_modes, offset)) return false;
    return true;
}

/**
 * @brief Parse the channel data request.
 * @param payload received payload of channel data request
 * @param datareq channel data request structure to fill.
 */
bool LWSSH::parse_channel_data_request(const pdiutil::vector<uint8_t> &payload, SSHChannelData &datareq){
    size_t offset = 1; // skip message type (already checked == 94)

    // Parse recipient channel (uint32)
    if (offset + 4 > payload.size()) return false;
    datareq.recipient_channel =
        (payload[offset] << 24) | (payload[offset+1] << 16) |
        (payload[offset+2] << 8) | payload[offset+3];
    offset += 4;

    // Parse data (SSH string)
    if (offset + 4 > payload.size()) return false;
    uint32_t data_len = (payload[offset] << 24) | (payload[offset+1] << 16) |
                        (payload[offset+2] << 8) | payload[offset+3];
    offset += 4;
    if (offset + data_len > payload.size()) return false;
    datareq.data.assign(payload.begin() + offset, payload.begin() + offset + data_len);
    offset += data_len;

    return true;
}

#endif // ENABLE_SSH_SERVICE
