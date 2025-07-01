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
class SSHServer : ServiceProvider {
public:
    SSHServer();
    ~SSHServer();

    /**
     * @brief Start the SSH service on the specified port.
     * @param port The port to listen on (default 22).
     * @return true if started successfully, false otherwise.
     */
    bool start(uint16_t port = 22);

    /**
     * @brief Init the service.
     * @param arg Optional argument, can be used to pass the port number.
     * @return true if initialized successfully, false otherwise.
     */
    bool initService(void *arg = nullptr) override;

    /**
     * @brief Stop the SSH service.
     */
    void stop();

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

private:
    iServerInterface* m_server;
    LWSSHSession* m_session;

    // Private methods for handling SSH session states
    void handleVersionExchange();
    void handleKeyExchange();
    void handleAuthentication();
    void handleChannelRequest();
};

}

extern LWSSH::SSHServer __sshserver_service;

#endif // _SSH_SERVICE_PROVIDER_H