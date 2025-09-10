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
            __i_dvc_ctrl.getTerminal(TERMINAL_TYPE_SERIAL)->writeln_ro(RODT_ATTR("SSH Client Session started."));
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

                    m_session->current_channel.req_type = recvreqst.request_type;

                    if( recvreqst.request_type == "pty-req" ){

                        // parse the pty-req channel request type specific data
                        parse_channel_request_pty_req(m_session, recvreqst.request_specific_data);
                    }else if( recvreqst.request_type == "shell" ){

                    }else if( recvreqst.request_type == "env" ){

                    // todo: update section when environment variables are supported
                    }else if( recvreqst.request_type == "subsystem" ){

                        int32_t offset = 0;
                        if (offset + 4 > recvreqst.request_specific_data.size()){

                            m_session->m_state = LWSSHSession::SESSION_STATE_SESSION_CLOSE;
                        }else{

                            int32_t vallen = (recvreqst.request_specific_data[offset] << 24) | (recvreqst.request_specific_data[offset+1] << 16) | (recvreqst.request_specific_data[offset+2] << 8) | recvreqst.request_specific_data[offset+3];
                            offset += 4;

                            if ((offset + vallen) > recvreqst.request_specific_data.size()){

                                m_session->m_state = LWSSHSession::SESSION_STATE_SESSION_CLOSE;
                            }else{

                                pdiutil::string val(reinterpret_cast<const char*>(recvreqst.request_specific_data.data() + offset), vallen);
                                m_session->current_channel.subsystem_req.subsystem = val;
                            }
                        }
                    }

                    // send reply if want
                    if (recvreqst.want_reply && (recvreqst.request_type == "shell" || 
                        recvreqst.request_type == "pty-req" ||
                        recvreqst.request_type == "subsystem"
                    )) {
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

                    if( m_session->current_channel.req_type == "shell" ||
                        m_session->current_channel.req_type == "pty-req"
                    ){
                        if( m_session->m_sshclient ){
                            m_session->m_sshclient->setReceivedChannelData(chdata.data);

                            #ifdef ENABLE_CMD_SERVICE

                            cmd_result_t res = __cmd_service.processTerminalInput(m_session->m_sshclient);

                            if( res == CMD_RESULT_ABORTED ||
                                res == CMD_RESULT_TERMINAL_ABORTED
                            ){
                                m_session->m_state = LWSSHSession::SESSION_STATE_SESSION_CLOSE;
                            }
                            #endif
                        }
                    }else if( m_session->current_channel.req_type == "exec" ){

                    }else if( m_session->current_channel.req_type == "subsystem" ){
                        handleChannelSubsystemRequest(chdata.data);
                    }
                }
            }else if(msg_type == SSH2_MSG_CHANNEL_EOF){

                pdiutil::vector<uint8_t> reply;
                reply.push_back(SSH2_MSG_CHANNEL_EOF); // 96
                // recipient channel (client's channel id)
                reply.push_back((m_session->current_channel.client_channel_id >> 24) & 0xFF);
                reply.push_back((m_session->current_channel.client_channel_id >> 16) & 0xFF);
                reply.push_back((m_session->current_channel.client_channel_id >> 8) & 0xFF);
                reply.push_back(m_session->current_channel.client_channel_id & 0xFF);

                if(send_server_ssh_packet(m_session, reply, true)){

                    __i_dvc_ctrl.wait(10); // Wait for a moment to ensure the client receives the EOF
                    // After sending EOF, we can send exit status and close the channel
                    // Send exit status before closing the channel
                    reply.clear();
                    reply.push_back(SSH2_MSG_CHANNEL_REQUEST); // 98
                    // recipient channel (client's channel id)
                    reply.push_back((m_session->current_channel.client_channel_id >> 24) & 0xFF);
                    reply.push_back((m_session->current_channel.client_channel_id >> 16) & 0xFF);
                    reply.push_back((m_session->current_channel.client_channel_id >> 8) & 0xFF);
                    reply.push_back(m_session->current_channel.client_channel_id & 0xFF);
                    // append equest type "exit-status"
                    const char* reqtype = "exit-status";
                    append_ssh_string(reply, reqtype, strlen(reqtype));
                    // append want reply flag
                    reply.push_back(0); // false, we don't want reply for exit-status
                    // append exit status
                    uint32_t exit_status = 0; // Set exit status as needed
                    reply.push_back((exit_status >> 24) & 0xFF);
                    reply.push_back((exit_status >> 16) & 0xFF);
                    reply.push_back((exit_status >> 8) & 0xFF);
                    reply.push_back(exit_status & 0xFF);
                    send_server_ssh_packet(m_session, reply, true);

                    // Schedule task in queue to send channel closure request
                    // if client not send close channel request within timeout
                    __task_scheduler.setTimeout( [&]() {

                        if( nullptr != m_session && m_session->current_channel.ischannelreqsuccess >= 0 ){

                            pdiutil::vector<uint8_t> chclose;
                            chclose.push_back(SSH2_MSG_CHANNEL_CLOSE); // 97
                            // recipient channel (client's channel id)
                            chclose.push_back((m_session->current_channel.client_channel_id >> 24) & 0xFF);
                            chclose.push_back((m_session->current_channel.client_channel_id >> 16) & 0xFF);
                            chclose.push_back((m_session->current_channel.client_channel_id >> 8) & 0xFF);
                            chclose.push_back(m_session->current_channel.client_channel_id & 0xFF);

                            if(send_server_ssh_packet(m_session, chclose, true)){
                                m_session->current_channel.ischannelreqsuccess = -1;
                            }else{
                                m_session->m_state = LWSSHSession::SESSION_STATE_SESSION_CLOSE;
                            }
                        }
                    }, 500, __i_dvc_ctrl.millis_now() );
                }else{
                    m_session->m_state = LWSSHSession::SESSION_STATE_SESSION_CLOSE;
                }
            }else if(msg_type == SSH2_MSG_CHANNEL_CLOSE){

                if( m_session->current_channel.ischannelreqsuccess >= 0 ){

                    pdiutil::vector<uint8_t> reply;
                    reply.push_back(SSH2_MSG_CHANNEL_CLOSE); // 97
                    // recipient channel (client's channel id)
                    reply.push_back((m_session->current_channel.client_channel_id >> 24) & 0xFF);
                    reply.push_back((m_session->current_channel.client_channel_id >> 16) & 0xFF);
                    reply.push_back((m_session->current_channel.client_channel_id >> 8) & 0xFF);
                    reply.push_back(m_session->current_channel.client_channel_id & 0xFF);

                    send_server_ssh_packet(m_session, reply, true);
                    m_session->current_channel.ischannelreqsuccess = -1;
                }
                m_session->m_state = LWSSHSession::SESSION_STATE_SESSION_CLOSE;
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

            if( m_session->current_channel.req_type == "shell" ||
                m_session->current_channel.req_type == "pty-req"
            ){
                #ifdef ENABLE_CMD_SERVICE
                __cmd_service.useTerminal(m_session->m_sshclient);
                #endif
            }
        }
    }
}


/**
 * @brief Handle channel subsystem requests from client
 */
void LWSSH::SSHServer::handleChannelSubsystemRequest(pdiutil::vector<uint8_t>& data){

    // Parse the exec request packet
    if (m_session->current_channel.subsystem_req.subsystem.find("sftp") == 0) {

        int32_t payloadoffset = 0;
        bool datahandled = false;

        do{

            data.erase(data.begin(), data.begin() + payloadoffset);
            payloadoffset = 0; // Reset offset for the next iteration

            if (payloadoffset + 4 > data.size()){

                m_session->m_state = LWSSHSession::SESSION_STATE_SESSION_CLOSE;
                datahandled = true; // Data not handled, close session
            }else{

                int32_t packetlen = (data[payloadoffset] << 24) | (data[payloadoffset+1] << 16) | (data[payloadoffset+2] << 8) | data[payloadoffset+3];
                payloadoffset += 4;

                if ( (payloadoffset + packetlen) > data.size() ){

                    m_session->m_state = LWSSHSession::SESSION_STATE_SESSION_CLOSE;
                    datahandled = true;
                }else if( (payloadoffset + packetlen) < data.size() ){

                    handleChannelSubsystemSftpRequest(data);
                }else{

                    handleChannelSubsystemSftpRequest(data);
                    datahandled = true; // Data handled successfully
                }

                payloadoffset += packetlen; // Move the offset to the next packet
            }
        } while (!datahandled);            
    }
}

/**
 * @brief Handle channel subsystem sftp requests from client
 */
void LWSSH::SSHServer::handleChannelSubsystemSftpRequest(pdiutil::vector<uint8_t>& data, bool expectReply){

    // Parse the exec request packet
    if (m_session->current_channel.subsystem_req.subsystem.find("sftp") == 0) {

        // __i_dvc_ctrl.getTerminal(TERMINAL_TYPE_SERIAL)->writeln();
        // __i_dvc_ctrl.getTerminal(TERMINAL_TYPE_SERIAL)->write_ro(RODT_ATTR("SSH Client channel sftp req : "));
        // for (size_t i = 0; i < data.size(); i++)
        // {
        //     __i_dvc_ctrl.getTerminal(TERMINAL_TYPE_SERIAL)->write_ro(RODT_ATTR("0x"));
        //     __i_dvc_ctrl.getTerminal(TERMINAL_TYPE_SERIAL)->write((uint32_t)data[i], true, true);
        //     __i_dvc_ctrl.getTerminal(TERMINAL_TYPE_SERIAL)->write_ro(RODT_ATTR(", "));
        // }
        // __i_dvc_ctrl.getTerminal(TERMINAL_TYPE_SERIAL)->writeln();

        int32_t payloadoffset = 0;
        if (payloadoffset + 4 > data.size()){

            m_session->m_state = LWSSHSession::SESSION_STATE_SESSION_CLOSE;
        }else{

            int32_t packetlen = (data[payloadoffset] << 24) | (data[payloadoffset+1] << 16) | (data[payloadoffset+2] << 8) | data[payloadoffset+3];
            payloadoffset += 4;

            if ( (payloadoffset + packetlen) > data.size() ){

                m_session->m_state = LWSSHSession::SESSION_STATE_SESSION_CLOSE;
            }else{

                uint8_t packettype = data[payloadoffset];
                payloadoffset += 1;
                pdiutil::vector<uint8_t> sftp_reply;
                int32_t errcode = -1;

                // Parse request-id
                uint32_t request_id = (data[payloadoffset] << 24) | (data[payloadoffset+1] << 16) | (data[payloadoffset+2] << 8) | data[payloadoffset+3];
                payloadoffset += 4;

                if (packettype == SSH_FXP_INIT) {

                    // version field will be 4 byte uint32 type
                    uint32_t sftpversion = request_id;

                    sftp_reply.push_back(0); // placeholder for length
                    sftp_reply.push_back(0); // placeholder for length
                    sftp_reply.push_back(0); // placeholder for length
                    sftp_reply.push_back(5); // length = 1 (type) + 4 (version) = 5
                    sftp_reply.push_back(SSH_FXP_VERSION); // type = 2
                    sftp_reply.push_back(0); // version = 3 (big-endian)
                    sftp_reply.push_back(0);
                    sftp_reply.push_back(0);
                    sftp_reply.push_back(3); // version = 3
                }else if (packettype == SSH_FXP_STAT || packettype == SSH_FXP_LSTAT){ // treat both same until symlinks not supported by FS

                    // Parse filename length
                    uint32_t fnamelen = (data[payloadoffset] << 24) | (data[payloadoffset+1] << 16) | (data[payloadoffset+2] << 8) | data[payloadoffset+3];
                    payloadoffset += 4;

                    // Parse filename
                    pdiutil::string filename(reinterpret_cast<const char*>(&data[payloadoffset]), fnamelen);
                    payloadoffset += fnamelen;

                    // (Optional: parse flags if present, for LSTAT usually not needed)

                    // Check if dir/file exists and get attributes 
                    bool file_exists = __i_fs.isFileExist(filename.c_str());
                    bool dir_exists = __i_fs.isDirExist(filename.c_str());

                    if (file_exists || dir_exists) {
                        // Prepare SSH_FXP_ATTRS reply (type 105)
                        // For demo: only size and permissions, fill as needed for your FS
                        uint32_t reply_len = 1 + 4 + 4 + 8 + 4; // type + reqid + flags + size + permissions
                        sftp_reply.push_back((reply_len >> 24) & 0xFF);
                        sftp_reply.push_back((reply_len >> 16) & 0xFF);
                        sftp_reply.push_back((reply_len >> 8) & 0xFF);
                        sftp_reply.push_back(reply_len & 0xFF);

                        // SSH_FXP_ATTRS reply
                        sftp_reply.push_back(SSH_FXP_ATTRS); // type 105

                        // Request ID
                        sftp_reply.push_back((request_id >> 24) & 0xFF);
                        sftp_reply.push_back((request_id >> 16) & 0xFF);
                        sftp_reply.push_back((request_id >> 8) & 0xFF);
                        sftp_reply.push_back(request_id & 0xFF);

                        // Flags: SSH_FILEXFER_ATTR_SIZE | SSH_FILEXFER_ATTR_PERMISSIONS
                        uint32_t flags = SSH_FILEXFER_ATTR_SIZE | SSH_FILEXFER_ATTR_PERMISSIONS;
                        sftp_reply.push_back((flags >> 24) & 0xFF);
                        sftp_reply.push_back((flags >> 16) & 0xFF);
                        sftp_reply.push_back((flags >> 8) & 0xFF);
                        sftp_reply.push_back(flags & 0xFF);

                        // File size
                        uint64_t filesize = dir_exists ? 0 : __i_fs.getFileSize(filename.c_str());
                        sftp_reply.push_back((filesize >> 56) & 0xFF);
                        sftp_reply.push_back((filesize >> 48) & 0xFF);
                        sftp_reply.push_back((filesize >> 40) & 0xFF);
                        sftp_reply.push_back((filesize >> 32) & 0xFF);
                        sftp_reply.push_back((filesize >> 24) & 0xFF);
                        sftp_reply.push_back((filesize >> 16) & 0xFF);
                        sftp_reply.push_back((filesize >> 8) & 0xFF);
                        sftp_reply.push_back(filesize & 0xFF);

                        // Permissions (S_IFREG | 0644) OR (S_IFDIR | 0755); S_IFREG = 0100000, S_IFDIR = 0040000
                        // todo: for now end general permissions until we dont support permissions in file system
                        uint32_t perms = dir_exists ? 0040755 : 0100644;
                        sftp_reply.push_back((perms >> 24) & 0xFF);
                        sftp_reply.push_back((perms >> 16) & 0xFF);
                        sftp_reply.push_back((perms >> 8) & 0xFF);
                        sftp_reply.push_back(perms & 0xFF);
                    } else {
                        // Status code: SSH_FX_NO_SUCH_FILE (2)
                        errcode = SSH_FX_NO_SUCH_FILE;
                    }
                }else if (packettype == SSH_FXP_OPEN || packettype == SSH_FXP_OPENDIR){ 
                    
                    // Parse filename length
                    uint32_t fnamelen = (data[payloadoffset] << 24) | (data[payloadoffset+1] << 16) | (data[payloadoffset+2] << 8) | data[payloadoffset+3];
                    payloadoffset += 4;

                    // Parse filename
                    pdiutil::string filename(reinterpret_cast<const char*>(&data[payloadoffset]), fnamelen);
                    payloadoffset += fnamelen;

                    // Parse flags if present
                    uint32_t flags = 0;
                    if (payloadoffset + 4 <= data.size()) {
                        flags = (data[payloadoffset] << 24) | (data[payloadoffset+1] << 16) | (data[payloadoffset+2] << 8) | data[payloadoffset+3];
                        payloadoffset += 4;
                    }

                    // parse attrs if present (for SSH_FXP_OPEN)
                    // currently not using until dont support attributes

                    // Check if dir/file exists and get attributes 
                    bool file_exists = __i_fs.isFileExist(filename.c_str());
                    bool dir_exists = __i_fs.isDirExist(filename.c_str());

                    if( packettype == SSH_FXP_OPEN && (flags & SSH_FXF_READ) && !(flags & SSH_FXF_WRITE) ){

                        if (file_exists || dir_exists) {
                            // Prepare SSH_FXP_HANDLE reply
                        }else{
                            // Status code: SSH_FX_NO_SUCH_FILE (2)
                            errcode = SSH_FX_NO_SUCH_FILE;
                        }
                    } else if( packettype == SSH_FXP_OPEN && (flags & SSH_FXF_CREAT) ) {

                        bool createit = false;

                        if( flags & SSH_FXF_EXCL ){

                            // If SSH_FXF_CREAT and SSH_FXF_EXCL are set, we should not open existing files
                            if( file_exists || dir_exists ){
                                errcode = SSH_FX_FILE_ALREADY_EXISTS; // Status code: SSH_FX_FILE_ALREADY_EXISTS (4)
                            }else{
                                createit = true; // Create the file if it does not exist
                            }
                        }else if( flags & SSH_FXF_TRUNC ){

                            if( file_exists ){
                                __i_fs.deleteFile(filename.c_str()); // If it exists, delete it first
                                file_exists = false; // Reset file_exists to false after deletion
                            }else if( dir_exists ){
                                __i_fs.deleteDirectory(filename.c_str()); // If it's a directory, delete it first
                                dir_exists = false; // Reset dir_exists to false after deletion
                            }
                            createit = true;
                        }else if( flags & SSH_FXF_WRITE ){

                            // Currently we will support append operation if file alread exists for write operation
                            createit = true;
                        }

                        if( createit ){

                            int istatus = 0;
                            if( !file_exists ){

                                istatus = __i_fs.createFile(filename.c_str(), ""); // If it does not exist, create it
                            }
                            
                            if( istatus < 0 ){
                                errcode = SSH_FX_NO_SUCH_PATH;
                            }else{
                                // This will be used to handle bolus chunks in the future which may receive from client
                                // along with SSH_FXP_WRITE paket. Reason to handle them in encryption layer is to avoid
                                // OOM on small devies having memory constraints.
                                m_session->current_channel.doHandleBolusChannelDataChunksCb = [&](pdiutil::vector<uint8_t> &boluschunk)->bool{
                                    return handleChannelSftpBolusChunks(boluschunk);
                                };
                            }
                        }
                    }

                    // If we have a valid handle, prepare the SSH_FXP_HANDLE reply
                    if( errcode == -1 ){

                        uint8_t handlesize = 3;
                        char handle[handlesize + 1]; memset(handle, 0, handlesize + 1); genUniqueKey(handle, handlesize); // Generate a unique handle for this open file
                        m_session->current_channel.subsystem_req.sftp.filepath = filename;
                        m_session->current_channel.subsystem_req.sftp.handle = handle;

                        uint32_t reply_len = 1 + 4 + 4 + handlesize; // type + reqid + handle size + handle bytes
                        sftp_reply.push_back((reply_len >> 24) & 0xFF);
                        sftp_reply.push_back((reply_len >> 16) & 0xFF);
                        sftp_reply.push_back((reply_len >> 8) & 0xFF);
                        sftp_reply.push_back(reply_len & 0xFF);

                        sftp_reply.push_back(SSH_FXP_HANDLE);

                        sftp_reply.push_back((request_id >> 24) & 0xFF);
                        sftp_reply.push_back((request_id >> 16) & 0xFF);
                        sftp_reply.push_back((request_id >> 8) & 0xFF); 
                        sftp_reply.push_back(request_id & 0xFF);

                        sftp_reply.push_back(0x00); sftp_reply.push_back(0x00); sftp_reply.push_back(0x00); sftp_reply.push_back(handlesize); // handle length
                        sftp_reply.insert(sftp_reply.end(), handle, handle+handlesize);
                    }

                }else if (packettype == SSH_FXP_READ){

                    // Parse handle length
                    uint32_t handlelen = (data[payloadoffset] << 24) | (data[payloadoffset+1] << 16) | (data[payloadoffset+2] << 8) | data[payloadoffset+3];
                    payloadoffset += 4;

                    // Parse handle
                    if (handlelen > 0 && payloadoffset + handlelen <= data.size()) {
                        pdiutil::string handle(reinterpret_cast<const char*>(&data[payloadoffset]), handlelen);
                        payloadoffset += handlelen;

                        if( m_session->current_channel.subsystem_req.sftp.handle == handle ){
                            // Parse offset
                            uint64_t fileoffset = ((uint64_t)data[payloadoffset] << 56) | ((uint64_t)data[payloadoffset+1] << 48) |
                                            ((uint64_t)data[payloadoffset+2] << 40) | ((uint64_t)data[payloadoffset+3] << 32) |
                                            ((uint64_t)data[payloadoffset+4] << 24) | ((uint64_t)data[payloadoffset+5] << 16) |
                                            ((uint64_t)data[payloadoffset+6] << 8) | (uint64_t)data[payloadoffset+7];
                            payloadoffset += 8;

                            // Parse length
                            uint32_t length = (data[payloadoffset] << 24) | (data[payloadoffset+1] << 16) | (data[payloadoffset+2] << 8) | data[payloadoffset+3];
                            payloadoffset += 4;

                            // Check if the file exists and read data
                            if (__i_fs.isFileExist(m_session->current_channel.subsystem_req.sftp.filepath.c_str())) {

                                uint64_t file_size = __i_fs.getFileSize(m_session->current_channel.subsystem_req.sftp.filepath.c_str());
                                
                                if (fileoffset >= file_size && length > 0) {

                                    errcode = SSH_FX_EOF; // EOF if offset is beyond file size and length is greater than zero
                                }else if(file_size > 0 && length == 0){

                                    errcode = SSH_FX_EOF; // end of file if accepted length is zero
                                }else{
                                    
                                    bool startsending = false;
                                    uint64_t totalskip = 0;

                                    int iStatus = __i_fs.readFile(m_session->current_channel.subsystem_req.sftp.filepath.c_str(), 1024, [&](char* data, uint32_t size)->bool{
                                        
                                        if( !startsending ){

                                            if( fileoffset == 0 ){

                                                startsending = true; // Start sending data from the beginning
                                            }else{

                                                if( totalskip < fileoffset ){

                                                    uint32_t skip = fileoffset - totalskip;
                                                    
                                                    if( skip >= size ){
                                                        totalskip += size;
                                                        return true; // Skip this chunk, continue reading
                                                    }else{
                                                        data += skip; // Skip the initial bytes
                                                        size -= skip; // Reduce the size to send
                                                        totalskip += skip; // Update total skip
                                                        startsending = true;
                                                    }
                                                }else{
                                                    startsending = true;
                                                }
                                            }
                                        }

                                        if( startsending ){
                                            // Prepare SSH_FXP_DATA reply (type 103)
                                            uint32_t reply_len = 1 + 4 + 4 + size; // type + reqid + data length + data
                                            sftp_reply.push_back((reply_len >> 24) & 0xFF);
                                            sftp_reply.push_back((reply_len >> 16) & 0xFF);
                                            sftp_reply.push_back((reply_len >> 8) & 0xFF);
                                            sftp_reply.push_back(reply_len & 0xFF);

                                            sftp_reply.push_back(SSH_FXP_DATA); // type 103

                                            sftp_reply.push_back((request_id >> 24) & 0xFF);
                                            sftp_reply.push_back((request_id >> 16) & 0xFF);
                                            sftp_reply.push_back((request_id >> 8) & 0xFF);
                                            sftp_reply.push_back(request_id & 0xFF);

                                            sftp_reply.push_back((size >> 24) & 0xFF);
                                            sftp_reply.push_back((size >> 16) & 0xFF);
                                            sftp_reply.push_back((size >> 8) & 0xFF);
                                            sftp_reply.push_back(size & 0xFF);

                                            sftp_reply.insert(sftp_reply.end(), data, data + size);
                                        }

                                        // return false to stop continue reading as we collected the chunk we need to send
                                        return false;
                                    });

                                    if( iStatus < 0 ){
                                        
                                        errcode = SSH_FX_FAILURE; // Failure if read operation failed
                                    }
                                }
                            }else{
                                errcode = SSH_FX_NO_SUCH_PATH;
                            }
                        }else{
                            errcode = SSH_FX_INVALID_HANDLE;
                        }
                    }else{
                        errcode = SSH_FX_BAD_MESSAGE;
                    }
                }else if (packettype == SSH_FXP_WRITE){

                    // Parse handle length
                    uint32_t handlelen = (data[payloadoffset] << 24) | (data[payloadoffset+1] << 16) | (data[payloadoffset+2] << 8) | data[payloadoffset+3];
                    payloadoffset += 4;

                    // Parse handle
                    if (handlelen > 0 && payloadoffset + handlelen <= data.size()) {
                        pdiutil::string handle(reinterpret_cast<const char*>(&data[payloadoffset]), handlelen);
                        payloadoffset += handlelen;

                        if( m_session->current_channel.subsystem_req.sftp.handle == handle ){
                            // Parse offset
                            uint64_t fileoffset = ((uint64_t)data[payloadoffset] << 56) | ((uint64_t)data[payloadoffset+1] << 48) |
                                            ((uint64_t)data[payloadoffset+2] << 40) | ((uint64_t)data[payloadoffset+3] << 32) |
                                            ((uint64_t)data[payloadoffset+4] << 24) | ((uint64_t)data[payloadoffset+5] << 16) |
                                            ((uint64_t)data[payloadoffset+6] << 8) | (uint64_t)data[payloadoffset+7];
                            payloadoffset += 8;

                            // Parse length
                            uint32_t length = (data[payloadoffset] << 24) | (data[payloadoffset+1] << 16) | (data[payloadoffset+2] << 8) | data[payloadoffset+3];
                            payloadoffset += 4;

                            int iStatus = __i_fs.editFile(  m_session->current_channel.subsystem_req.sftp.filepath.c_str(), 
                                                            fileoffset,
                                                            (const char*)&data[payloadoffset], 
                                                            length);
                            if( iStatus < 0 ){
                                errcode = SSH_FX_FAILURE; // Failure if write operation failed
                            }else{
                                errcode = SSH_FX_OK; // SSH_FX_OK (0) for successful write
                            }
                        }else{
                            errcode = SSH_FX_INVALID_HANDLE;
                        }
                    }else{
                        errcode = SSH_FX_BAD_MESSAGE;
                    }
                }else if (packettype == SSH_FXP_FSETSTAT){ 

                    // Parse handle length
                    uint32_t handlelen = (data[payloadoffset] << 24) | (data[payloadoffset+1] << 16) | (data[payloadoffset+2] << 8) | data[payloadoffset+3];
                    payloadoffset += 4;

                    // Parse handle
                    if (handlelen > 0 && payloadoffset + handlelen <= data.size()) {
                        pdiutil::string handle(reinterpret_cast<const char*>(&data[payloadoffset]), handlelen);
                        payloadoffset += handlelen;

                        if( m_session->current_channel.subsystem_req.sftp.handle == handle ){

                            // too: parse the attributes if present
                            // currently not using until dont support attributes
                            errcode = SSH_FX_OK;

                            m_session->current_channel.doHandleBolusChannelDataChunksCb = nullptr; // reset the bolus chunk handler if any
                        }else{
                            errcode = SSH_FX_INVALID_HANDLE;
                        }
                    }else{
                        errcode = SSH_FX_BAD_MESSAGE;
                    }
                }else if (packettype == SSH_FXP_CLOSE){ 

                    // Parse handle length
                    uint32_t handlelen = (data[payloadoffset] << 24) | (data[payloadoffset+1] << 16) | (data[payloadoffset+2] << 8) | data[payloadoffset+3];
                    payloadoffset += 4;

                    // Parse handle
                    if (handlelen > 0 && payloadoffset + handlelen <= data.size()) {
                        pdiutil::string handle(reinterpret_cast<const char*>(&data[payloadoffset]), handlelen);
                        payloadoffset += handlelen;

                        if( m_session->current_channel.subsystem_req.sftp.handle == handle ){
                            // Successfully closed the file, send success reply
                            errcode = SSH_FX_OK; // SSH_FX_OK (0) for successful close

                            m_session->current_channel.doHandleBolusChannelDataChunksCb = nullptr; // reset the bolus chunk handler if any
                        }else{
                            errcode = SSH_FX_INVALID_HANDLE;
                        }
                    }else{
                        errcode = SSH_FX_BAD_MESSAGE;
                    }
                }

                if(errcode != -1){
                    // Prepare SSH_FXP_STATUS reply (type 101)
                    uint32_t reply_len = 1 + 4 + 4 + 4 + 4; // type + reqid + code + msglen + lang-tag len
                    sftp_reply.clear();
                    sftp_reply.push_back((reply_len >> 24) & 0xFF);
                    sftp_reply.push_back((reply_len >> 16) & 0xFF);
                    sftp_reply.push_back((reply_len >> 8) & 0xFF);
                    sftp_reply.push_back(reply_len & 0xFF);

                    sftp_reply.push_back(SSH_FXP_STATUS); // type 101
                    
                    sftp_reply.push_back((request_id >> 24) & 0xFF);
                    sftp_reply.push_back((request_id >> 16) & 0xFF);
                    sftp_reply.push_back((request_id >> 8) & 0xFF);
                    sftp_reply.push_back(request_id & 0xFF);

                    uint32_t code = errcode;
                    sftp_reply.push_back((code >> 24) & 0xFF);
                    sftp_reply.push_back((code >> 16) & 0xFF);
                    sftp_reply.push_back((code >> 8) & 0xFF);
                    sftp_reply.push_back(code & 0xFF);

                    // Empty error message (length 0)
                    sftp_reply.push_back(0x00); sftp_reply.push_back(0x00); sftp_reply.push_back(0x00); sftp_reply.push_back(0x00);

                    // Empty language tag (length 0)
                    sftp_reply.push_back(0x00); sftp_reply.push_back(0x00); sftp_reply.push_back(0x00); sftp_reply.push_back(0x00);
                }

                // Send the SFTP reply back to the client
                if( sftp_reply.size() > 0 && expectReply ){
                    if (send_channel_data(m_session, (const char*)sftp_reply.data(), sftp_reply.size())) {
                        // __i_dvc_ctrl.getTerminal(TERMINAL_TYPE_SERIAL)->write_ro(RODT_ATTR("SSH SFTP subsystem rply sent : "));
                        // for (size_t i = 0; i < sftp_reply.size(); i++)
                        // {
                        //     __i_dvc_ctrl.getTerminal(TERMINAL_TYPE_SERIAL)->write_ro(RODT_ATTR("0x"));
                        //     __i_dvc_ctrl.getTerminal(TERMINAL_TYPE_SERIAL)->write((uint32_t)sftp_reply[i], true, true);
                        //     __i_dvc_ctrl.getTerminal(TERMINAL_TYPE_SERIAL)->write_ro(RODT_ATTR(", "));
                        // }
                        // __i_dvc_ctrl.getTerminal(TERMINAL_TYPE_SERIAL)->writeln();
                    } else {
                        __i_dvc_ctrl.getTerminal(TERMINAL_TYPE_SERIAL)->writeln_ro(RODT_ATTR("Failed to send SFTP subsystem reply."));
                        // m_session->m_state = LWSSHSession::SESSION_STATE_SESSION_CLOSE;
                    }
                }
            }
        }
    }        
}

bool LWSSH::SSHServer::handleChannelSftpBolusChunks(pdiutil::vector<uint8_t> &boluschunk){

    static uint8_t sftpheader[28] = {0};
    static uint64_t sftpheaderoffset = 0;
    uint32_t &expectedDataLen = m_session->current_channel.subsystem_req.sftp.fxp_write_expectedrecvlen;
    uint32_t &totalreceived = m_session->current_channel.subsystem_req.sftp.fxp_write_totalrecvd;
    bool continueReceiving = true;

    // initial chunk
    if( totalreceived == 0 ){

        memset(sftpheader, 0, sizeof(sftpheader));

        // If the first chunk is not SSH_FXP_WRITE, handle it normally and stop further chunk receiving
        if( boluschunk[4] != SSH_FXP_WRITE ){

            m_session->current_channel.doHandleBolusChannelDataChunksCb = nullptr; // reset the callback
            handleChannelSubsystemSftpRequest(boluschunk);
            boluschunk.clear();
            return false; // stop receiving further chunks
        }

        // Prepare SFTP header for SSH_FXP_WRITE from received first chunk
        memcpy(sftpheader, boluschunk.data(), 28);

        boluschunk.erase(boluschunk.begin(), boluschunk.begin() + 28);

        sftpheaderoffset = ((uint64_t)sftpheader[16] << 56) | ((uint64_t)sftpheader[17] << 48) |
                        ((uint64_t)sftpheader[18] << 40) | ((uint64_t)sftpheader[19] << 32) |
                        ((uint64_t)sftpheader[20] << 24) | ((uint64_t)sftpheader[21] << 16) |
                        ((uint64_t)sftpheader[22] << 8) | (uint64_t)sftpheader[23];

        // note the initial total packet length to be received in contineous chunks
        expectedDataLen  =  (sftpheader[24] << 24) | 
                            (sftpheader[25] << 16) | 
                            (sftpheader[26] << 8) | 
                            sftpheader[27];

        totalreceived = 0;                                    
    }

    // modify the length field in header as we get the data in chunks
    uint32_t chunksize = pdistd::min( (uint32_t)(expectedDataLen - totalreceived), (uint32_t)boluschunk.size());
    uint32_t newlen = 1 + 4 + 4 + 3 + 8 + 4 + chunksize; // type + reqid + handle length + we know handle + offset + data length + data bytes size
    sftpheader[0] = (newlen >> 24) & 0xFF;
    sftpheader[1] = (newlen >> 16) & 0xFF;
    sftpheader[2] = (newlen >> 8) & 0xFF;
    sftpheader[3] = newlen & 0xFF;

    // Modify file offset in header as we get the data in chunks
    uint64_t sftpfileoffset = sftpheaderoffset + totalreceived;
    sftpheader[16] = ((sftpfileoffset) >> 56) & 0xFF;
    sftpheader[17] = ((sftpfileoffset) >> 48) & 0xFF;
    sftpheader[18] = ((sftpfileoffset) >> 40) & 0xFF;
    sftpheader[19] = ((sftpfileoffset) >> 32) & 0xFF;
    sftpheader[20] = ((sftpfileoffset) >> 24) & 0xFF;
    sftpheader[21] = ((sftpfileoffset) >> 16) & 0xFF;
    sftpheader[22] = ((sftpfileoffset) >> 8) & 0xFF;
    sftpheader[23] = (sftpfileoffset) & 0xFF;

    // modify the data len bytes in header as we get the data in chunks
    sftpheader[24] = (chunksize >> 24) & 0xFF;
    sftpheader[25] = (chunksize >> 16) & 0xFF;
    sftpheader[26] = (chunksize >> 8) & 0xFF;
    sftpheader[27] = chunksize & 0xFF;

    // Insert the modified header and chunk data to file
    boluschunk.insert(boluschunk.begin(), sftpheader, sftpheader + 28);

    // Update the total received data bytes
    totalreceived += chunksize;

    // Check whether we received total expected data length
    if( totalreceived == expectedDataLen ){
        // todo: currently handling callback reset in close message
        continueReceiving = false; // indicate all data received
        totalreceived = 0;
        expectedDataLen = 0;
    }

    // Handle and Continue to receive bolus chunks
    if( chunksize < (boluschunk.size() - 28) ){
        pdiutil::vector<uint8_t> temp(boluschunk.begin(), boluschunk.begin() + 28 + chunksize);
        boluschunk.erase(boluschunk.begin(), boluschunk.begin() + 28 + chunksize);
        handleChannelSubsystemSftpRequest(temp, !continueReceiving);
    }else{
        handleChannelSubsystemSftpRequest(boluschunk, !continueReceiving);
        boluschunk.clear();
    }
    return true;
}

SSHServer __sshserver_service;

#endif // ENABLE_SSH_SERVICE
