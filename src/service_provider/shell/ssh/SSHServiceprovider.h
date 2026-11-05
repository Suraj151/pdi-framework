/***************************** Light Weight SSH *******************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 6th Apr 2025
******************************************************************************/

#ifndef _SSH_SERVICE_PROVIDER_H
#define _SSH_SERVICE_PROVIDER_H

#include "SSHServiceUtil.h"

namespace LWSSH {

/**
 * SSH server class that provides a basic SSH server functionality.
 * It uses iServerInterface to listen for incoming SSH connections.
 */
class SSHServer : public ServiceProvider {
public:
    SSHServer();
    ~SSHServer();

    /**
     * @brief Start the SSH service on the specified port.
     * @param port The port to listen on (default 22).
     * @return true if started successfully, false otherwise.
     */
    bool start(uint16_t port = SSH_DEFAULT_PORT);

    /**
     * @brief Init the service.
     * @param arg Optional argument, can be used to pass the port number.
     * @return true if initialized successfully, false otherwise.
     */
    bool initService(void *arg = nullptr) override;

    /**
     * Needs the network it listens on before it can come up.
     */
    uint8_t getServiceDependencies(service_t *_out, uint8_t _max) const override {
      uint8_t _n = 0;
    #ifdef ENABLE_WIFI_SERVICE
      if( _max > _n ) _out[_n++] = SERVICE_WIFI;
    #endif
      return _n;
    }

    /**
     * @brief Stop the SSH service.
     */
    void stop();

    /**
     * Releases the listener and every open session, so a stopped ssh service
     * refuses a connection rather than accepting one it will never answer.
     */
    bool stopService() override;

    terminal_types_t getServiceTerminalType() const override { return TERMINAL_TYPE_SSH; }

    /**
     * @brief close current session.
     */
    void closeSession();

    /**
     * @brief Handle incoming ssh clients and data.
     */
    void handle();

    /**
     * @brief Get ssh key pairs.
     */
    bool getSSHKeyPairs(SSHKeyAlgorithm type, pdiutil::vector<uint8_t>& pubkey, pdiutil::vector<uint8_t>& privkey, bool privkeyInSeedPlusPubkeyformat = false);

protected:
    iServerInterface* m_server;
    LWSSHSession* m_sessions[SSH_MAX_SESSIONS]; // concurrent session pool
    LWSSHSession* m_session;                    // session currently being serviced
    bool m_handling = false;                    // re-entrancy guard for handle()
    uint32_t m_poolfullsince = 0;               // when the pool was first found full

    // Create the service config file with default policy when it is missing.
    void createDefaultSshConfig();

    // Create the ed25519 host key in SSH_HOST_KEY_DIR when it is missing.
    void createDefaultHostKeys();

    // Run the state machine for the current session (m_session).
    void serviceSession();

    // Private methods for handling SSH session states
    void handleVersionExchange();
    void handleKeyExchange();
    void handleAuthentication();
    void handleChannelRequest();
    void handleChannelSubsystemRequest(pdiutil::vector<uint8_t>& data);
    void handleChannelSubsystemSftpRequest(pdiutil::vector<uint8_t>& data);
    static void appendSftpStatus(pdiutil::vector<uint8_t> &reply, uint32_t request_id, uint32_t code);
    bool sendSftpReply(pdiutil::vector<uint8_t> &reply);
    bool sendHeldSftpReply();
    void writeSftpData(const uint8_t *bytes, uint32_t size);
    size_t takeSftpStream(const uint8_t *bytes, size_t size);
    void closeOnUnreadPacket(int status);
};

}

extern LWSSH::SSHServer __sshserver_service;

#endif // _SSH_SERVICE_PROVIDER_H