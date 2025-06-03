/************************** Telnet service ***********************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st May 2025
******************************************************************************/

#ifndef _TELNET_SERVICE_PROVIDER_H_
#define _TELNET_SERVICE_PROVIDER_H_

#include <service_provider/ServiceProvider.h>

/**
 * @brief TelnetServiceProvider provides a basic Telnet server using TcpServerInterface.
 */
class TelnetServiceProvider : public ServiceProvider {
public:
    TelnetServiceProvider();
    virtual ~TelnetServiceProvider();

    /**
     * @brief Start the Telnet service on the specified port.
     * @param port The port to listen on (default 23).
     * @return true if started successfully, false otherwise.
     */
    bool start(uint16_t port = 23);

    /**
     * @brief Init the service.
     * @param arg Optional argument, can be used to pass the port number.
     * @return true if initialized successfully, false otherwise.
     */
    bool initService(void *arg = nullptr) override;

    /**
     * @brief Stop the Telnet service.
     */
    void stop();

    /**
     * @brief close current client.
     */
    void closeClient();

    /**
     * @brief Handle incoming Telnet clients and data.
     */
    void handle();

private:
    iTcpServerInterface* m_server;
    iClientInterface* m_client;
};

extern TelnetServiceProvider __telnet_service;

#endif