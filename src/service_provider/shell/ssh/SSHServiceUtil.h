/********************************* SSH util ***********************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 6th Apr 2025
******************************************************************************/

#ifndef _SSH_SERVICE_UTIL_H
#define _SSH_SERVICE_UTIL_H

#include "SSHCommon.h"
#include "SSHClientInterface.h"
#include <service_provider/ServiceProvider.h>
#include <utility/crypto/symmetric/aes/aes.h>
#include <utility/crypto/asymmetric/rsa/rsa.h>

namespace LWSSH {

// SSH name list type
typedef pdiutil::vector<pdiutil::string> ssh_name_list;

// SSH packet structure
struct ssh_packet {
    pdiutil::vector<uint8_t> payload; // vector to the packet payload data

    // Assignment operator
    const ssh_packet& operator = (const ssh_packet& other) {
        payload = other.payload;
        return *this;
    }
};

// ECDH initialization packet structure
struct EcdhInitPacket {
    pdiutil::vector<uint8_t> client_pubkey; // Should be 32 bytes for Curve25519
};

// ssh channel structure's to hold the channel details
struct SSHChannelRequest {
    uint32_t recipient_channel;
    pdiutil::string request_type;
    bool want_reply;
    pdiutil::vector<uint8_t> request_specific_data; // raw, for further parsing
};
struct SSHPtyReq {
    pdiutil::string term;
    uint32_t width_chars = 0;
    uint32_t height_rows = 0;
    uint32_t width_pixels = 0;
    uint32_t height_pixels = 0;
    pdiutil::vector<uint8_t> terminal_modes;
};
struct SSHSubsystemRequest {
    pdiutil::string subsystem;     // The subsystem protocol to use (e.g., "sftp", "shell")

    struct Sftp{
        pdiutil::string filepath;
        pdiutil::string handle; // Handle for the sftp, if needed. currently supporting 1 only at a time
        uint32_t fxp_write_totalrecvd = 0; // Total bytes received for current write operation
        uint32_t fxp_write_expectedrecvlen = 0; // Expected total length for current
        uint8_t fxp_write_header[28] = {0}; // per-session reassembled SSH_FXP_WRITE header
        uint64_t fxp_write_headeroffset = 0; // per-session base file offset for the write

        pdiutil::vector<uint8_t> rx_accum;

        // Directory-handle state for SSH_FXP_OPENDIR/READDIR.
        struct Entry {
            pdiutil::string name;
            uint64_t size = 0;
            bool is_dir = false;
            uint16_t uid = 0;
            uint16_t gid = 0;
            uint16_t perms = 0;
            uint32_t ctime = 0;
            uint32_t mtime = 0;
        };
        bool is_dir = false;
        uint32_t readdir_offset = 0;
        pdiutil::vector<Entry> dir_entries;
    } sftp;
};
struct SSHChannelData {
    uint32_t recipient_channel;
    pdiutil::vector<uint8_t> data; // raw data, can be text or binary
};
struct SSHChannel {
    pdiutil::string channel_type;     // e.g., "session"
    uint32_t client_channel_id = 0;   // The ID assigned by the client
    uint32_t server_channel_id = 0;   // The ID assigned by your server
    uint32_t window_size = 0;
    uint32_t max_packet_size = 0;
    // Add more fields as needed (state, pty info, etc.)
    // e.g., bool shell_started, std::string pty_type, etc.
    pdiutil::string req_type;
    SSHPtyReq pty_req;
    SSHSubsystemRequest subsystem_req;
    int ischannelreqsuccess = -1;
    pdiutil::function<bool(pdiutil::vector<uint8_t> &)> doHandleBolusChannelDataChunksCb = nullptr; // If provided, handle client bolus chunks
};

// SSH key exchange initialization fields
struct SSHKexInitFields {
    pdiutil::vector<uint8_t> cookie; // 16 bytes
    ssh_name_list kex_algorithms;
    ssh_name_list server_host_key_algorithms;
    ssh_name_list encryption_algorithms_ctos;
    ssh_name_list encryption_algorithms_stoc;
    ssh_name_list mac_algorithms_ctos;
    ssh_name_list mac_algorithms_stoc;
    ssh_name_list compression_algorithms_ctos;
    ssh_name_list compression_algorithms_stoc;
    ssh_name_list languages_ctos;
    ssh_name_list languages_stoc;
    bool first_kex_packet_follows;
    uint32_t reserved;
};

// User auth struct
struct SSHUserAuthRequest {
    pdiutil::string username;
    pdiutil::string service;
    pdiutil::string method;
    pdiutil::string password; // Only if method == "password"

    // Only if method == "publickey"
    bool has_signature = false;
    pdiutil::string pk_algorithm;
    pdiutil::vector<uint8_t> pubkey_blob;
    pdiutil::vector<uint8_t> signature;
};

// Represents a single SSH session
struct LWSSHSession { 
    // Session states
    enum SessionState {
        SESSION_STATE_WAITING_FOR_CLIENT = 0, // Waiting for a client to connect
        SESSION_STATE_CLIENT_CONNECTED,       // Client has connected
        SESSION_STATE_VEX_SEND,      // Exchanging SSH version information
        SESSION_STATE_VEX_RECV,      // Exchanging SSH version information
        SESSION_STATE_KEX_INIT_RECV,          // Key exchange process
        SESSION_STATE_KEX_INIT_SEND,          // Key exchange process
        SESSION_STATE_KEXDH_INIT_RECV,          // Key exchange process
        SESSION_STATE_KEXDH_INIT_SEND,          // Key exchange process
        SESSION_STATE_NEWKEYS_RECEIVED,      // New keys received
        SESSION_STATE_AUTHENTICATION_REQUEST, // Requesting client authentication
        SESSION_STATE_CHANNEL_REQUEST, // Processing client channel requests
        SESSION_STATE_SESSION_ESTABLISHED,     // SSH session established
        SESSION_STATE_SESSION_TIMEOUT,         // SSH session timeout
        SESSION_STATE_SESSION_CLOSE              // SSH session close
    };

    // Constructor
    LWSSHSession(iClientInterface* client) : 
        m_client(client), 
        m_sshclient(nullptr),
        m_state(SESSION_STATE_WAITING_FOR_CLIENT),
        m_server_version("SSH-2.0-LWSSH_0.1"), // Default server version
        m_server_kex_init_sent(0),
        m_last_recv_timestamp(0),
        m_session_timeout(SSH_HANDSHAKE_IDLE_MS), // Default session timeout in milliseconds
        packets_seq_num_ctos(0),
        packets_seq_num_stoc(0)
    { 
        memset(m_shared_secret_key, 0, sizeof(m_shared_secret_key));
        memset(m_exchange_hash_h, 0, sizeof(m_exchange_hash_h));

        memset(derived_iv_ctos, 0, sizeof(derived_iv_ctos));
        memset(derived_iv_stoc, 0, sizeof(derived_iv_stoc));
        memset(derived_enc_key_ctos, 0, sizeof(derived_enc_key_ctos));
        memset(derived_enc_key_stoc, 0, sizeof(derived_enc_key_stoc));
        memset(derived_mac_key_ctos, 0, sizeof(derived_mac_key_ctos));
        memset(derived_mac_key_stoc, 0, sizeof(derived_mac_key_stoc));

        // Create ssh client for channel data exchange
        m_sshclient = pdiutil::safe_new<SSHClientInterface>(
            static_cast<iTcpClientInterface*>(client)
        );
        if (m_sshclient) {
            m_sshclient->setSSHSession(this);
        }
    }
    // Destructor
    ~LWSSHSession() {
        if (m_sshclient) {
            m_sshclient->close();
        }
        pdiutil::safe_delete(m_sshclient);
        if (m_client) {
            m_client->close();
        }
        pdiutil::safe_delete(m_client);
    }

    // Check if the session is in a valid state
    bool isSessionTimeout() const {
        return (__i_dvc_ctrl.millis_now() - m_last_recv_timestamp) > m_session_timeout;
    }

    void markActive() {
        m_last_recv_timestamp = __i_dvc_ctrl.millis_now();
    }

    // member variables
    iClientInterface *m_client; // Pointer to the client interface, which will be set when a client connects
    SSHClientInterface *m_sshclient; // Pointer to the ssh client interface
    SessionState m_state;       // Current state of the SSH session
    ssh_packet m_sshpacket;
    EcdhInitPacket m_ecdh_init_packet; // ECDH initialization packet
    pdiutil::string m_server_version;  // Server version string
    pdiutil::string m_client_version;  // Client version string
    ssh_packet m_client_kexinit;       // Client KEXINIT packet
    ssh_packet m_server_kexinit;       // Server KEXINIT packet
    uint8_t m_server_kex_init_sent;    // Flag to indicate if server KEX init has been sent
    uint64_t m_last_recv_timestamp;    // Last received timestamp
    uint32_t m_session_timeout;        // Session timeout in milliseconds

    pdiutil::vector<uint8_t> m_server_ephermeral_pubkey;  // Server's ephemeral public key
    pdiutil::vector<uint8_t> m_server_ephermeral_privkey; // Server's ephemeral private key

    uint8_t m_shared_secret_key[32]; // Shared secret key for ECDH key exchange
    uint8_t m_exchange_hash_h[32];   // exchange hash H

    uint8_t derived_iv_ctos[16];      // 'A' key type Derived IV for client-to-server
    uint8_t derived_iv_stoc[16];      // 'B' key type Derived IV for server-to-client
    uint8_t derived_enc_key_ctos[16]; // 'C' key type Derived encryption key for client-to-server
    uint8_t derived_enc_key_stoc[16]; // 'D' key type Derived encryption key for server-to-client
    uint8_t derived_mac_key_ctos[32]; // 'E' key type Derived MAC key for client-to-server
    uint8_t derived_mac_key_stoc[32]; // 'F' key type Derived MAC key for server-to-client
    uint8_t mac_len = 20; // Negotiated MAC length in bytes (20 hmac-sha1, 32 hmac-sha2-256)

    AES_ctx aes_ctx_ctos; // AES context for client-to-server encryption
    AES_ctx aes_ctx_stoc; // AES context for server-to-client encryption
    uint32_t packets_seq_num_ctos; // Sequence number for client-to-server packets
    uint32_t packets_seq_num_stoc; // Sequence number for server-to-client packets

    SSHChannel current_channel; // For single-channel servers. currently keeping single channle only.
    ssh_config_t m_ssh_config;  // Per-session auth policy, loaded lazily before userauth.
    bool m_ssh_config_loaded = false;
    SSHKeyAlgorithm m_negotiated_hostkey_algo = SSH_KEY_ALGO_ED25519; // SSH_KEY_ALGO_MIN => none
    bool m_client_ext_info = false; // client advertised ext-info-c (RFC 8308)
};

// Helper functions
int parse_received_packet(LWSSHSession* session, ssh_packet& packet);
int parse_encrypted_packet(LWSSHSession* session, ssh_packet &packet);
bool parse_userauth_request(const pdiutil::vector<uint8_t>& payload, SSHUserAuthRequest& req);
void load_ssh_config(ssh_config_t& config);

void build_key_path(char* out, uint16_t outsize, const char* dir, const char* algo, const char* suffix);
bool generate_ed25519_key(const char* dir);
void build_rsa_hostkey_blob(const rsa_key& key, pdiutil::vector<uint8_t>& out);
bool save_rsa_key(const rsa_key& key, const char* dir);
bool load_rsa_host_key(rsa_key& key);
void ssh_rng_fill(uint8_t* buf, size_t len);
bool ed25519_hostkey_exists();
bool rsa_hostkey_exists();
void get_supported_hostkey_algos(ssh_name_list& out);
SSHKeyAlgorithm negotiate_hostkey_algo(const ssh_name_list& client_algos);
bool client_advertised_ext_info(const ssh_name_list& client_kex_algos);
void build_ext_info_packet(pdiutil::vector<uint8_t>& payload);
bool extract_ed25519_blob_field(const pdiutil::vector<uint8_t>& blob, pdiutil::vector<uint8_t>& out, uint32_t expected_size);
bool is_authorized_pubkey(const pdiutil::vector<uint8_t>& client_pubkey_blob);
void build_pubkey_auth_signed_data(LWSSHSession* session, const SSHUserAuthRequest& req, pdiutil::vector<uint8_t>& out);
bool verify_pubkey_signature(LWSSHSession* session, const SSHUserAuthRequest& req);
bool parse_name_list(const pdiutil::vector<uint8_t>& payload, uint32_t& offset, ssh_name_list& name_list);
bool parse_kex_init_fields(const pdiutil::vector<uint8_t>& payload, SSHKexInitFields& fields);
bool parse_kex_ecdh_init(const pdiutil::vector<uint8_t>& payload, EcdhInitPacket& packet);
bool parse_channel_open_request(LWSSHSession* session, ssh_packet &packet);
bool parse_channel_request(const pdiutil::vector<uint8_t>& payload, SSHChannelRequest& req);
bool parse_channel_request_pty_req(LWSSHSession* session, const pdiutil::vector<uint8_t>& data);
bool parse_channel_data_request(const pdiutil::vector<uint8_t>& payload, SSHChannelData& datareq);

void append_name_list(pdiutil::vector<uint8_t>& out, const ssh_name_list& names);
void append_ssh_string(pdiutil::vector<uint8_t>& out, const pdiutil::vector<uint8_t>& data);
void append_ssh_string(pdiutil::vector<uint8_t>& out, const char* data, uint32_t length);
void append_mpint(pdiutil::vector<uint8_t>& out, const uint8_t* data, int32_t len);

void prepare_server_kexinit(pdiutil::vector<uint8_t> &payload);
void encrypt_ssh_payload(LWSSHSession* session, pdiutil::vector<uint8_t> &payload);
void prepare_ssh_packet(pdiutil::vector<uint8_t> &payload, int32_t block_size = 8);
bool send_server_ssh_packet(LWSSHSession* session, pdiutil::vector<uint8_t> &payload, bool encrypt = false);
bool read_ssh_string(const pdiutil::vector<uint8_t>& payload, pdiutil::string& str, int32_t &offset);
bool read_ssh_string(const pdiutil::vector<uint8_t>& payload, pdiutil::vector<uint8_t>& str, int32_t &offset);
bool send_channel_open_confirmation(LWSSHSession* session, uint32_t window_size, uint32_t max_packet);
bool send_channel_data(LWSSHSession* session, const char* data, uint32_t length);

void build_exchange_hash(
    const pdiutil::string &client_version,           // e.g. "SSH-2.0-OpenSSH_9.0"
    const pdiutil::string &server_version,           // e.g. "SSH-2.0-LWSSH_0.1"
    const pdiutil::vector<uint8_t> &client_kexinit,  // raw KEXINIT payload (not SSH packet)
    const pdiutil::vector<uint8_t> &server_kexinit,  // raw KEXINIT payload (not SSH packet)
    const pdiutil::vector<uint8_t> &server_host_key, // SSH format
    const pdiutil::vector<uint8_t> &client_ecdh_pub, // 32 bytes
    const pdiutil::vector<uint8_t> &server_ecdh_pub, // 32 bytes
    const uint8_t *shared_secret,                    // 32 bytes
    uint8_t *hash_output);          // output buffer for SHA-256 hash

bool create_server_ephemeral_keys(pdiutil::vector<uint8_t> &pubkey,
                                  pdiutil::vector<uint8_t> &privkey,
                                  pdiutil::vector<uint8_t> *ed25519privkey_seedpubfmt=nullptr);

bool prepare_server_ecdh_reply(LWSSHSession *session,
                               const pdiutil::vector<uint8_t> &client_pubkey,
                               const pdiutil::vector<uint8_t> &server_host_pubkey,
                               const pdiutil::vector<uint8_t> &server_host_privkey,
                               pdiutil::vector<uint8_t> &payload);

bool prepare_server_ecdh_reply_rsa(LWSSHSession *session,
                                   const pdiutil::vector<uint8_t> &client_pubkey,
                                   const rsa_key &key,
                                   SSHKeyAlgorithm algo,
                                   pdiutil::vector<uint8_t> &payload);

void derive_key(const uint8_t *K, size_t K_len, // shared secret (mpint, usually 32 bytes)
                const uint8_t *H, size_t H_len, // exchange hash (SHA-256, 32 bytes)
                const uint8_t *session_id, size_t session_id_len,
                char key_type,               // 'A', 'B', 'C', etc. (see RFC 4253 7.2)
                uint8_t *out, size_t out_len); // output buffer for derived key
}

#endif // _SSH_SERVICE_UTIL_H