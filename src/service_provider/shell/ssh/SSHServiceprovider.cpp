/***************************** Light Weight SSH *******************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 6th Apr 2025
******************************************************************************/

#include <config/Config.h>

#if defined(ENABLE_SSH_SERVICE)

#include "SSHServiceprovider.h"
#ifdef ENABLE_CMD_SERVICE
#include <service_provider/cmd/CommandLineServiceProvider.h>
#endif

using namespace LWSSH;

/**
 * @brief Constructor for SSHServer.
 */
SSHServer::SSHServer(): 
    m_server(nullptr), 
    m_session(nullptr),
    ServiceProvider(SERVICE_SSH, RODT_ATTR("SSH")) 
    {}

/**
 * @brief Destructor for SSHServer.
 */
SSHServer::~SSHServer() {
    stop();
}

/**
 * @brief Start the SSH service on the specified port.
 */
bool SSHServer::start(uint16_t port) {
    if (!m_server) {
        m_server = __i_instance.getNewTcpServerInstance();
    }

    if (m_server->begin(port) == 0) {

        m_server->setOnAcceptClientEventCallback([](void* arg){
            LWSSH::SSHServer *sshserver = reinterpret_cast<LWSSH::SSHServer*>(arg);

            if(sshserver){

                sshserver->handle();
            }
        }, this);

        return true;
    }

    return false;
}

/**
 * @brief Start the SSH service on the specified port.
 */
bool SSHServer::initService(void *arg) {

    bool started = false;

    if (arg) {
        
        uint16_t port = *(uint16_t*)arg;
        started = start(port);
    }else{

        started = start();
    }

    // If the service started successfully, set up a periodic task to handle incoming clients
    if(started){
        __task_scheduler.setInterval( [&]() {
            this->handle();
        }, 1, __i_dvc_ctrl.millis_now() );
    }
    
    return started && ServiceProvider::initService(arg);
}

/**
 * @brief Stop the SSH service.
 */
void SSHServer::stop() {
    closeSession();
    if (m_server) {
        m_server->close();
    }
}

/**
 * @brief close current session.
 */
void SSHServer::closeSession() {
    #ifdef ENABLE_CMD_SERVICE
    // Notify command service to stop using the terminal
    __cmd_service.useTerminal(nullptr);
    // Inform serial terminal about the end of telnet client session
    if(__i_dvc_ctrl.getTerminal(TERMINAL_TYPE_SERIAL)){
        __i_dvc_ctrl.getTerminal(TERMINAL_TYPE_SERIAL)->writeln();
        __i_dvc_ctrl.getTerminal(TERMINAL_TYPE_SERIAL)->writeln_ro(RODT_ATTR("SSH Client Session ended."));
    }
    #endif
    __i_dvc_ctrl.yield();
    __i_dvc_ctrl.wait(5);
    __i_dvc_ctrl.yield();
    if (m_session) {
        delete m_session;
        m_session = nullptr;
    }
}

/**
 * @brief Accepts a new client connection and handles SSH session setup.
 * @return Pointer to the accepted client interface, or nullptr if no client is connected.
 */
void SSHServer::handle() {

    if (!m_server) {
        return; // Server not initialized
    }

    // Check if there is a new client connection
    if (!m_session && m_server->hasClient()) {

        m_session = new LWSSHSession(m_server->accept());

        #ifdef ENABLE_CMD_SERVICE
        m_session->m_sshclient->set_terminal_type(TERMINAL_TYPE_SSH);
        // Inform serial terminal about the new telnet client session
        if(__i_dvc_ctrl.getTerminal(TERMINAL_TYPE_SERIAL)){
            __i_dvc_ctrl.getTerminal(TERMINAL_TYPE_SERIAL)->writeln();
            __i_dvc_ctrl.getTerminal(TERMINAL_TYPE_SERIAL)->writeln_ro(RODT_ATTR("SSH Client Session starting."));
        }
        #endif            
    }

    // If a client is connected, handle the SSH session
    if (m_session && m_session->m_client && m_session->m_client->connected()) {

        switch (m_session->m_state)
        {
            case LWSSHSession::SESSION_STATE_WAITING_FOR_CLIENT:
                // Handle waiting for client
                // m_session->m_state = LWSSHSession::SESSION_STATE_CLIENT_CONNECTED;
                // break;
            case LWSSHSession::SESSION_STATE_CLIENT_CONNECTED:
                // Handle client connected
                // client connected so record first timestamp
                m_session->m_last_recv_timestamp = __i_dvc_ctrl.millis_now();
                m_session->m_state = LWSSHSession::SESSION_STATE_VEX_SEND;
                handleVersionExchange();
                break;
            case LWSSHSession::SESSION_STATE_VEX_RECV:
                // Handle version exchange
                handleVersionExchange();
                break;
            case LWSSHSession::SESSION_STATE_KEX_INIT_RECV:
            case LWSSHSession::SESSION_STATE_KEX_INIT_SEND:
            case LWSSHSession::SESSION_STATE_KEXDH_INIT_RECV:
            case LWSSHSession::SESSION_STATE_KEXDH_INIT_SEND:
                // Handle key exchange
                handleKeyExchange();
                break;
            case LWSSHSession::SESSION_STATE_NEWKEYS_RECEIVED:
            case LWSSHSession::SESSION_STATE_AUTHENTICATION_REQUEST:
                // Handle key exchange
                handleAuthentication();
                break;
            case LWSSHSession::SESSION_STATE_CHANNEL_REQUEST:
                handleChannelRequest();
                break;
            case LWSSHSession::SESSION_STATE_SESSION_ESTABLISHED:
                break;
            case LWSSHSession::SESSION_STATE_SESSION_TIMEOUT:
            case LWSSHSession::SESSION_STATE_SESSION_CLOSE:
                closeSession();
                // Handle session established
                break;
            default:
                break;
        }
        // __i_dvc_ctrl.yield();
    } else if (m_session && m_session->m_client && !m_session->m_client->connected()) {
        closeSession();
    }
}

/**
 * @brief Get the key pairs for the specified SSH key type.
 * @param type The type of SSH key to retrieve (e.g., rsa, ed25519).
 * @param pubkey Output vector to store the public key.
 * @param privkey Output vector to store the private key.
 */
bool SSHServer::getSSHKeyPairs(SSHKeyAlgorithm type, pdiutil::vector<uint8_t> &pubkey, pdiutil::vector<uint8_t> &privkey, bool privkeyInSeedPlusPubkeyformat){
    
    bool bStatus = false;
    if (type == SSH_KEY_ALGO_ED25519) {

        const char* homedir = __i_fs.getHomeDirectory();
        char sshdir[30]; memset(sshdir, 0, sizeof(sshdir));
        char privkeyOrseed_path[45]; memset(privkeyOrseed_path, 0, sizeof(privkeyOrseed_path));
        char pubkey_path[45]; memset(pubkey_path, 0, sizeof(pubkey_path));
        pubkey.clear();
        privkey.clear();
        int bytesRead = 0;

        snprintf(sshdir, sizeof(sshdir), "%s/%s", (strlen(homedir) > 1 ? homedir : ""), SSH_DEFAULT_DIR);
        snprintf(privkeyOrseed_path, sizeof(privkeyOrseed_path), (privkeyInSeedPlusPubkeyformat ? "%s/%s.seed" : "%s/%s"), sshdir, SSH_KEY_ALGO_ED25519_STR);
        snprintf(pubkey_path, sizeof(pubkey_path), "%s/%s.pub", sshdir, SSH_KEY_ALGO_ED25519_STR);

        if (!__i_fs.isFileExist(privkeyOrseed_path) || !__i_fs.isFileExist(pubkey_path)) {
            return bStatus; // No SSH keys found, cannot proceed
        }

        bytesRead = __i_fs.readFile(pubkey_path, 32, [&](char* data, uint32_t size)->bool{
            if( pubkey.size() <= ED25519_PUBKEY_SIZE ){
                pubkey.insert(pubkey.end(), data, data + size);
            }
            // return true to continue reading
            return true;
        });

        bytesRead = __i_fs.readFile(privkeyOrseed_path, 32, [&](char* data, uint32_t size)->bool{
            if( privkey.size() <= ED25519_PRIVKEY_SIZE ){
                privkey.insert(privkey.end(), data, data + size);
            }
            // return true to continue reading
            return true;
        });
        __i_dvc_ctrl.yield();

        if( privkeyInSeedPlusPubkeyformat && privkey.size() < ED25519_PRIVKEY_SIZE ){
            // Format the private key in the format: [seed][public key]
            privkey.insert(privkey.end(), pubkey.begin(), pubkey.end());
        }

        bStatus = true;
    }

    return bStatus;
}

/**
 * @brief Handle version exchange with the client.
 * This function is called when the session state is VERSION_EXCHANGE.
 */
void SSHServer::handleVersionExchange() {

    // Send SSH version information to the client
    if (m_session->m_state == LWSSHSession::SESSION_STATE_VEX_SEND) {
        
        bool bStatus = ( (m_session->m_server_version.size()+2) == m_session->m_client->writeln(m_session->m_server_version.c_str()) );
        m_session->m_state = LWSSHSession::SESSION_STATE_VEX_RECV;
    } if(m_session->m_state == LWSSHSession::SESSION_STATE_VEX_RECV){

        while (m_session->m_client->available() > 0) {
            // Read client version information
            char c = m_session->m_client->read();
            if( c == '\n'){
                break; // Stop reading when a newline character is encountered
            }else if( c == '\r'){
                continue; // Ignore carriage return characters
            }
            m_session->m_client_version += c;
            __i_dvc_ctrl.wait(1);
        }

        if (m_session->m_client_version.length() > 4 && m_session->m_client_version.substr(0, 4) == "SSH-") {
            m_session->m_state = LWSSHSession::SESSION_STATE_KEX_INIT_RECV;
        }

        if( m_session->isSessionTimeout() ){
            m_session->m_state = LWSSHSession::SESSION_STATE_SESSION_TIMEOUT;
        }
    }
}

/**
 * @brief Handle key exchange with the client.
 * This function is called when the session state is KEY_EXCHANGE.
 */
void SSHServer::handleKeyExchange(){

    if( m_session->m_state == LWSSHSession::SESSION_STATE_KEX_INIT_RECV || 
        m_session->m_state == LWSSHSession::SESSION_STATE_KEXDH_INIT_RECV){

        int parsestatus = parse_received_packet(m_session, m_session->m_sshpacket);
        if( 0 == parsestatus ){

            uint8_t msg_type = m_session->m_sshpacket.payload[0];

            if(msg_type == SSH2_MSG_KEXINIT){
                
                // Store the received KEXINIT packet once server send its KEXINIT
                m_session->m_client_kexinit = m_session->m_sshpacket; 

                // Handle key exchange initialization
                SSHKexInitFields kex_init_fields;
                if(parse_kex_init_fields(m_session->m_sshpacket.payload, kex_init_fields)){
                    // Successfully parsed key exchange initialization fields
                    m_session->m_state = LWSSHSession::SESSION_STATE_KEX_INIT_SEND;
                }else{
                    m_session->m_state = LWSSHSession::SESSION_STATE_SESSION_CLOSE;
                }
            }else if(msg_type == SSH2_MSG_KEXDH_INIT){
                // Handle ECDH_INIT packet
                if(parse_kex_ecdh_init(m_session->m_sshpacket.payload, m_session->m_ecdh_init_packet)){
                    m_session->m_state = LWSSHSession::SESSION_STATE_KEXDH_INIT_SEND;
                }else{
                    m_session->m_state = LWSSHSession::SESSION_STATE_SESSION_CLOSE;
                }
            }
        }else if(parsestatus < 0){
            m_session->m_state = LWSSHSession::SESSION_STATE_SESSION_CLOSE;
        }
    }else if(m_session->m_state == LWSSHSession::SESSION_STATE_KEX_INIT_SEND){

        if(m_session->m_server_kex_init_sent > 0){
            m_session->m_state = LWSSHSession::SESSION_STATE_KEXDH_INIT_RECV;
            return; // KEXINIT already sent, no need to send again
        }
        m_session->m_server_kex_init_sent++;
        // Send KEXINIT to the client
        prepare_server_kexinit(m_session->m_server_kexinit.payload);
        if(send_server_ssh_packet(m_session, m_session->m_server_kexinit.payload)){

            m_session->m_state = LWSSHSession::SESSION_STATE_KEXDH_INIT_RECV;
        }else{

            m_session->m_state = LWSSHSession::SESSION_STATE_SESSION_CLOSE;
        }
    }else if(m_session->m_state == LWSSHSession::SESSION_STATE_KEXDH_INIT_SEND){

        pdiutil::vector<uint8_t> server_host_pubkey, server_host_privkey, payload;

        bool bstatus = getSSHKeyPairs(SSH_KEY_ALGO_ED25519, server_host_pubkey, server_host_privkey);
        if (!bstatus) {
            if(__i_dvc_ctrl.getTerminal(TERMINAL_TYPE_SERIAL)){
                __i_dvc_ctrl.getTerminal(TERMINAL_TYPE_SERIAL)->writeln();
                __i_dvc_ctrl.getTerminal(TERMINAL_TYPE_SERIAL)->writeln_ro(RODT_ATTR("SSH keys not found. Please create server ssh keys."));
            }
            m_session->m_state = LWSSHSession::SESSION_STATE_SESSION_CLOSE;
            return; // No SSH keys found, cannot proceed
        }

        bstatus = prepare_server_ecdh_reply(
            m_session, 
            m_session->m_ecdh_init_packet.client_pubkey, // client public key from ECDH_INIT
            server_host_pubkey, // server host public key (Ed25519)
            server_host_privkey, // server host private key (Ed25519)
            payload
        );

        bstatus &= send_server_ssh_packet(m_session, payload);

        if(bstatus){
            m_session->m_state = LWSSHSession::SESSION_STATE_NEWKEYS_RECEIVED;
        }else{
            m_session->m_state = LWSSHSession::SESSION_STATE_SESSION_CLOSE;
        }
    }
}

/**
 * @brief Handle authentication with the client.
 * This function is called when the session state is AUTHENTICATION_REQUEST.
 */
void LWSSH::SSHServer::handleAuthentication(){

    if (m_session->m_state == LWSSHSession::SESSION_STATE_AUTHENTICATION_REQUEST ||
        m_session->m_state == LWSSHSession::SESSION_STATE_NEWKEYS_RECEIVED
    ) {

        int parsestatus = 0;

        if(m_session->m_state > LWSSHSession::SESSION_STATE_NEWKEYS_RECEIVED){

            parsestatus = parse_encrypted_packet(m_session, m_session->m_sshpacket);
        }else {
            // If we are here, it means we have received the NEWKEYS packet
            // and we need to parse it to proceed with authentication
            parsestatus = parse_received_packet(m_session, m_session->m_sshpacket);
        }

        if( 0 == parsestatus ){

            uint8_t msg_type = m_session->m_sshpacket.payload[0];

            if(msg_type == SSH2_MSG_NEWKEYS){

                pdiutil::vector<uint8_t> payload;
                payload.push_back(SSH2_MSG_NEWKEYS);
                bool bstatus = send_server_ssh_packet(m_session, payload);
                // derive keys after receiving NEWKEYS
                if(bstatus){

                    // shared secret in mpint format
                    pdiutil::vector<uint8_t> mpint_K;
                    LWSSH::append_mpint(mpint_K, m_session->m_shared_secret_key, 32);
                    
                    derive_key(
                        mpint_K.data(), mpint_K.size(),
                        m_session->m_exchange_hash_h, sizeof(m_session->m_exchange_hash_h),
                        m_session->m_exchange_hash_h, sizeof(m_session->m_exchange_hash_h),
                        'A', // Key type 'A' for client-to-server
                        m_session->derived_iv_ctos, sizeof(m_session->derived_iv_ctos)
                    );
                    derive_key(
                        mpint_K.data(), mpint_K.size(),
                        m_session->m_exchange_hash_h, sizeof(m_session->m_exchange_hash_h),
                        m_session->m_exchange_hash_h, sizeof(m_session->m_exchange_hash_h),
                        'B', // Key type 'B' for server-to-client
                        m_session->derived_iv_stoc, sizeof(m_session->derived_iv_stoc)
                    );
                    derive_key(
                        mpint_K.data(), mpint_K.size(),
                        m_session->m_exchange_hash_h, sizeof(m_session->m_exchange_hash_h),
                        m_session->m_exchange_hash_h, sizeof(m_session->m_exchange_hash_h),
                        'C', // Key type 'C' for client-to-server ENC
                        m_session->derived_enc_key_ctos, sizeof(m_session->derived_enc_key_ctos)
                    );
                    derive_key(
                        mpint_K.data(), mpint_K.size(),
                        m_session->m_exchange_hash_h, sizeof(m_session->m_exchange_hash_h),
                        m_session->m_exchange_hash_h, sizeof(m_session->m_exchange_hash_h),
                        'D', // Key type 'D' for server-to-client ENC
                        m_session->derived_enc_key_stoc, sizeof(m_session->derived_enc_key_stoc)
                    );
                    derive_key(
                        mpint_K.data(), mpint_K.size(),
                        m_session->m_exchange_hash_h, sizeof(m_session->m_exchange_hash_h),
                        m_session->m_exchange_hash_h, sizeof(m_session->m_exchange_hash_h),
                        'E', // Key type 'E' for client-to-server HMAC
                        m_session->derived_mac_key_ctos, sizeof(m_session->derived_mac_key_ctos)
                    );
                    derive_key(
                        mpint_K.data(), mpint_K.size(),
                        m_session->m_exchange_hash_h, sizeof(m_session->m_exchange_hash_h),
                        m_session->m_exchange_hash_h, sizeof(m_session->m_exchange_hash_h),
                        'F', // Key type 'F' for server-to-client HMAC
                        m_session->derived_mac_key_stoc, sizeof(m_session->derived_mac_key_stoc)
                    );

                    AES_init_ctx_iv(&m_session->aes_ctx_ctos, m_session->derived_enc_key_ctos, m_session->derived_iv_ctos);
                    AES_init_ctx_iv(&m_session->aes_ctx_stoc, m_session->derived_enc_key_stoc, m_session->derived_iv_stoc);

                    m_session->m_state = LWSSHSession::SESSION_STATE_AUTHENTICATION_REQUEST;
                }else{
                    m_session->m_state = LWSSHSession::SESSION_STATE_SESSION_CLOSE;
                }
            }else if(msg_type == SSH2_MSG_SERVICE_REQUEST){

                pdiutil::string userauth = "ssh-userauth";
                pdiutil::vector<uint8_t> userauthvec(userauth.begin(), userauth.end());

                pdiutil::vector<uint8_t> payload;
                payload.push_back(SSH2_MSG_SERVICE_ACCEPT); // 6
                // Append SSH string "ssh-userauth"
                append_ssh_string(payload, userauthvec);

                send_server_ssh_packet(m_session, payload, true);
                // Now wait for SSH2_MSG_USERAUTH_REQUEST
            }else if(msg_type == SSH2_MSG_USERAUTH_REQUEST){

                SSHUserAuthRequest authreq;
                bool bstatus = parse_userauth_request(m_session->m_sshpacket.payload, authreq);

                // check if we can check check authentication
                if( bstatus ){

                    bstatus = (authreq.username.size() > 1 && authreq.password.size() > 1);
                    if( bstatus ){

                        bstatus = __auth_service.isAuthorized(authreq.username.c_str(), authreq.password.c_str());
                    }
                }

                // if false ask for password
                if(!bstatus){
                    pdiutil::vector<uint8_t> reply;
                    reply.push_back(SSH2_MSG_USERAUTH_FAILURE); // 51
                    pdiutil::string methods = "password";
                    pdiutil::vector<uint8_t> methodvec(methods.begin(), methods.end());
                    append_ssh_string(reply, methodvec);
                    reply.push_back(0); // partial success = FALSE

                    bstatus = send_server_ssh_packet(m_session, reply, true);
                }else{
                    pdiutil::vector<uint8_t> reply;
                    reply.push_back(SSH2_MSG_USERAUTH_SUCCESS); // 52
                    bstatus = send_server_ssh_packet(m_session, reply, true);
                    if(bstatus){
                        m_session->m_state = LWSSHSession::SESSION_STATE_CHANNEL_REQUEST;
                    }
                }
            }else{
                // __i_dvc_ctrl.getTerminal(TERMINAL_TYPE_SERIAL)->writeln();
                // __i_dvc_ctrl.getTerminal(TERMINAL_TYPE_SERIAL)->write_ro(RODT_ATTR("SSH Client Authentication req else : "));
                // __i_dvc_ctrl.getTerminal(TERMINAL_TYPE_SERIAL)->writeln((int32_t)m_session->m_sshpacket.payload.size());
                // for (size_t i = 0; i < m_session->m_sshpacket.payload.size(); i++)
                // {
                //     __i_dvc_ctrl.getTerminal(TERMINAL_TYPE_SERIAL)->write(m_session->m_sshpacket.payload[i]);
                // }
                // __i_dvc_ctrl.getTerminal(TERMINAL_TYPE_SERIAL)->writeln();
            }
        }else if(parsestatus < 0){
            m_session->m_state = LWSSHSession::SESSION_STATE_SESSION_CLOSE;
        }
    }
}

/**
 * @brief Handle channel requests from client
 */
void LWSSH::SSHServer::handleChannelRequest(){

    if(m_session->m_state == LWSSHSession::SESSION_STATE_CHANNEL_REQUEST){

        int parsestatus = parse_encrypted_packet(m_session, m_session->m_sshpacket);

        if( 0 == parsestatus ){

            uint8_t msg_type = m_session->m_sshpacket.payload[0];

            if(msg_type == SSH2_MSG_CHANNEL_OPEN){

                bool bstatus = parse_channel_open_request(m_session, m_session->m_sshpacket);

                if(bstatus){

                    // Currently setting same received client window size & packet size 
                    bstatus = send_channel_open_confirmation(m_session, m_session->current_channel.window_size, m_session->current_channel.max_packet_size);

                    if(!bstatus){

                        m_session->m_state = LWSSHSession::SESSION_STATE_SESSION_CLOSE;
                    }
                }else{
                    m_session->m_state = LWSSHSession::SESSION_STATE_SESSION_CLOSE;
                }
            }else if(msg_type == SSH2_MSG_CHANNEL_REQUEST){

                SSHChannelRequest recvreqst;
                bool bstatus = parse_channel_request(m_session->m_sshpacket.payload, recvreqst);

                if(bstatus){

                    if( recvreqst.request_type == "pty-req" ){

                        // parse the pty-req channel request type specific data
                        parse_channel_request_pty_req(m_session, recvreqst.request_specific_data);
                    }else if( recvreqst.request_type == "shell" ){

                    }else if( recvreqst.request_type == "env" ){

                    }

                    // send reply if want
                    if (recvreqst.want_reply && (recvreqst.request_type == "shell" || recvreqst.request_type == "pty-req")) {
                        pdiutil::vector<uint8_t> reply;
                        reply.push_back(SSH2_MSG_CHANNEL_SUCCESS); // 99
                        // recipient channel (client's channel id)
                        reply.push_back((m_session->current_channel.client_channel_id >> 24) & 0xFF);
                        reply.push_back((m_session->current_channel.client_channel_id >> 16) & 0xFF);
                        reply.push_back((m_session->current_channel.client_channel_id >> 8) & 0xFF);
                        reply.push_back(m_session->current_channel.client_channel_id & 0xFF);

                        if(send_server_ssh_packet(m_session, reply, true)){
                            if(m_session->current_channel.ischannelreqsuccess < 0){
                                m_session->current_channel.ischannelreqsuccess = 1;
                            }
                        }else{
                            m_session->m_state = LWSSHSession::SESSION_STATE_SESSION_CLOSE;
                        }
                    }
                }else{
                    m_session->m_state = LWSSHSession::SESSION_STATE_SESSION_CLOSE;
                } 
            }else if(msg_type == SSH2_MSG_CHANNEL_DATA){

                SSHChannelData chdata;
                bool bstatus = parse_channel_data_request(m_session->m_sshpacket.payload, chdata);

                if( bstatus ){

                    if( m_session->m_sshclient ){
                        m_session->m_sshclient->setReceivedChannelData(chdata.data);

                        #ifdef ENABLE_CMD_SERVICE

                        // __i_dvc_ctrl.getTerminal(TERMINAL_TYPE_SERIAL)->write_ro(RODT_ATTR("data from ssh client ("));
                        // __i_dvc_ctrl.getTerminal(TERMINAL_TYPE_SERIAL)->write((int32_t)chdata.data.size());
                        // __i_dvc_ctrl.getTerminal(TERMINAL_TYPE_SERIAL)->write_ro(RODT_ATTR(") : "));
                        // for (size_t i = 0; i < chdata.data.size(); i++)
                        // {
                        //     __i_dvc_ctrl.getTerminal(TERMINAL_TYPE_SERIAL)->write(chdata.data[i]);
                        // }
                        // __i_dvc_ctrl.getTerminal(TERMINAL_TYPE_SERIAL)->write_ro(RODT_ATTR(" : "));
                        // for (size_t i = 0; i < chdata.data.size(); i++)
                        // {
                        //     __i_dvc_ctrl.getTerminal(TERMINAL_TYPE_SERIAL)->write_ro(RODT_ATTR("0x"));
                        //     __i_dvc_ctrl.getTerminal(TERMINAL_TYPE_SERIAL)->write((uint32_t)chdata.data[i], true, true);
                        //     __i_dvc_ctrl.getTerminal(TERMINAL_TYPE_SERIAL)->write_ro(RODT_ATTR(", "));
                        // }
                        // __i_dvc_ctrl.getTerminal(TERMINAL_TYPE_SERIAL)->writeln();

                        cmd_result_t res = __cmd_service.processTerminalInput(m_session->m_sshclient);

                        // __i_dvc_ctrl.getTerminal(TERMINAL_TYPE_SERIAL)->write_ro(RODT_ATTR("cmd res : "));
                        // __i_dvc_ctrl.getTerminal(TERMINAL_TYPE_SERIAL)->writeln((int32_t)res);
                        // __i_dvc_ctrl.getTerminal(TERMINAL_TYPE_SERIAL)->writeln();

                        if( res == CMD_RESULT_ABORTED ||
                            res == CMD_RESULT_TERMINAL_ABORTED
                        ){
                            m_session->m_state = LWSSHSession::SESSION_STATE_SESSION_CLOSE;
                        }
                        #endif
                    }
                }
            }else{
                __i_dvc_ctrl.getTerminal(TERMINAL_TYPE_SERIAL)->writeln();
                __i_dvc_ctrl.getTerminal(TERMINAL_TYPE_SERIAL)->write_ro(RODT_ATTR("SSH Client channel req else : "));
                __i_dvc_ctrl.getTerminal(TERMINAL_TYPE_SERIAL)->writeln((int32_t)m_session->m_sshpacket.payload.size());
                for (size_t i = 0; i < m_session->m_sshpacket.payload.size(); i++)
                {
                    __i_dvc_ctrl.getTerminal(TERMINAL_TYPE_SERIAL)->write_ro(RODT_ATTR("0x"));
                    __i_dvc_ctrl.getTerminal(TERMINAL_TYPE_SERIAL)->write((uint32_t)m_session->m_sshpacket.payload[i], true, true);
                    __i_dvc_ctrl.getTerminal(TERMINAL_TYPE_SERIAL)->write_ro(RODT_ATTR(", "));
                }
                __i_dvc_ctrl.getTerminal(TERMINAL_TYPE_SERIAL)->writeln();
            }
        }else if(parsestatus < 0){
            m_session->m_state = LWSSHSession::SESSION_STATE_SESSION_CLOSE;
        }

        // Once channel establish set the ssh client interface i.e. terminal interface to commandline
        if( m_session->m_sshclient && m_session->current_channel.ischannelreqsuccess == 1 ){

            m_session->current_channel.ischannelreqsuccess = 2;
            #ifdef ENABLE_CMD_SERVICE
            __cmd_service.useTerminal(m_session->m_sshclient);
            #endif
        }
    }
}

SSHServer __sshserver_service;

#endif // ENABLE_SSH_SERVICE
