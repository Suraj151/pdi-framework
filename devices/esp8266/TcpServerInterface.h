/************************* TCP Server Interface *******************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st May 2025
******************************************************************************/
#ifndef _TCP_SERVER_INTERFACE_H_
#define _TCP_SERVER_INTERFACE_H_

#include "esp8266.h"
#include "TcpClientInterface.h"
#include <interface/pdi/middlewares/iServerInterface.h>

class TcpServerInterface : public iTcpServerInterface {
public:
    TcpServerInterface();
    ~TcpServerInterface();

    /**
     * @brief begines listening on the specified port.
     * @param port The port number to listen on.
     * @return 0 on success, negative error code on failure.
     */
    int32_t begin(uint16_t port) override;

    /**
     * @brief Check if there is a client connected.
     * @return true if a client is connected, false otherwise.
     */
    bool hasClient() const override;
    
    /**
     * @brief Accepts a new client connection.
     * @return Pointer to the accepted client interface, or nullptr if no client is connected.
     */
    iClientInterface* accept() override;
    
    /**
     * @brief Set the timeout for client connections.
     * @param timeout_ms The timeout value in milliseconds.
     */
    void setTimeout(uint32_t timeout_ms) override;
    
    /**
     * @brief Closes the server and all connected clients.
     */
    void close() override;

    /**
     * @brief Get new instance
     */
    iServerInterface* getNewServerInterface() override {
        return new TcpServerInterface();
    }

private:

    /**
     * @brief Callback for when a new client connection is accepted.
     * @param arg User-defined argument (this instance).
     * @param newpcb Pointer to the new TCP protocol control block.
     * @param err Error code.
     * @return Error code.
     */
    static err_t onAccept(void* arg, struct tcp_pcb* newpcb, err_t err);

    struct tcp_pcb* m_serverPcb;
    struct tcp_pcb* m_clientPcb;
    uint32_t m_timeout;
    bool m_hasClient;
};

#endif // _TCP_SERVER_INTERFACE_H_