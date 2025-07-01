/************************** SSH Client interface *****************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 6th Apr 2025
******************************************************************************/

#ifndef _SSH_SERVICE_CLIENT_INTERFACE_H
#define _SSH_SERVICE_CLIENT_INTERFACE_H

#include <service_provider/ServiceProvider.h>

namespace LWSSH {

// forward declaration
struct LWSSHSession;    

/**
 * SSH Client interface to interact with client over channel data packets.
 * This client will be used once SSH channels are successfully open. 
 */
class SSHClientInterface : public iTcpClientInterface {
public:

    // Constructor
    SSHClientInterface(iTcpClientInterface* tcpclient) : 
        m_tcpClient(tcpclient), 
        m_client_session(nullptr),
        m_minSizeToWritePayload(100),
        m_writeCommitTaskId(-1)
    {}
    
    // Destructor
    ~SSHClientInterface(){
        m_client_session = nullptr;
        m_tcpClient = nullptr;
        __task_scheduler.clearTimeout(m_writeCommitTaskId);
    }

    /**
     * @brief Connect to a remote server.
     * @param host The IP address of the server.
     * @param port The port number of the server.
     * @return Connection status (0 for success, negative for failure).
     */
    int16_t connect(const uint8_t *host, uint16_t port) override;

    /**
     * @brief Disconnects the current connection.
     * @return Disconnection status (0 for success, negative for failure).
     */
    int16_t disconnect() override;

    /**
     * @brief Closes the current connection.
     * @return Disconnection status (0 for success, negative for failure).
     */
    int16_t close() override;

    /**
     * @brief Check if the socket is connected.
     * @return Connection status (1 for connected, 0 for disconnected).
     */
    int8_t connected() override;

    /**
     * @brief Write data to the server.
     * @param c_str The data to send.
     * @param size The size of the data.
     * @return The number of bytes written, or 0 on failure.
     */
    int32_t write(const uint8_t* c_str, uint32_t size) override;

    /**
     * @brief Writes a single byte of data.
     * @param c The byte to write.
     * @return Number of bytes written.
     */
    int32_t write(uint8_t c) override;

    /**
     * @brief Writes a string of bytes.
     * @param c_str Pointer to the byte string.
     * @return Number of bytes written.
     */
    int32_t write(const uint8_t *c_str) override;

    /**
     * @brief Writes a read-only string.
     * @param c_str The string to write.
     * @return Number of bytes written.
     */
    int32_t write_ro(const char *c_str) override;

    /**
     * @brief Read data from the socket.
     * @param buffer The buffer to store the read data.
     * @param size The maximum number of bytes to read.
     * @return The number of bytes read, or 0 on failure.
     */
    int32_t read(uint8_t* buffer, uint32_t size) override;

    /**
     * @brief Reads a single byte of data.
     * @return The byte read.
     */
    uint8_t read() override;
    
    /**
     * @brief Check the number of bytes available to read.
     * @return The number of bytes available, or -1 on error.
     */
    int32_t available() override;

    /**
     * @brief Get the local IP address.
     * @return The local IP address.
     */
    ipaddress_t getLocalIp() const override;

    /**
     * @brief Get the local port.
     * @return The local port number.
     */
    uint16_t getLocalPort() const override;

    /**
     * @brief Get the remote IP address.
     * @return The remote IP address.
     */
    ipaddress_t getRemoteIp() const override;

    /**
     * @brief Get the remote port.
     * @return The remote port number.
     */
    uint16_t getRemotePort() const override;

    /**
     * @brief Enable TCP keep-alive and configure its parameters.
     * @param idleTime The idle time (in seconds) before sending the first keep-alive packet.
     * @param interval The interval (in seconds) between keep-alive packets.
     * @param count The number of keep-alive packets to send before considering the connection dead.
     * @return True if keep-alive was successfully enabled, false otherwise.
     */
    bool setKeepAlive(uint16_t idleTime, uint16_t interval, uint16_t count) override;

    /**
     * @brief Set the NoDelay option for TCP.
     * @param noDelay If true, disables Nagle's algorithm (reducing latency).
     */
    void setNoDelay(bool noDelay) override;

    /**
     * @brief Set the timeout.
     * @param timeout The timeout value in milliseconds.
     */
    void setTimeout(uint32_t timeout) override;

    /** 
     * @brief Flush the output buffer.
     */
    void flush() override;

    /**
     * @brief Commit IO operations in case if queued.
     */
    int32_t commit() override;
    
    /** 
     * @brief set received decrypted payload from client
     */
    void setReceivedChannelData(pdiutil::vector<uint8_t> &recvpayload);

    /** 
     * @brief set ssh session of client
     */
    void setSSHSession(LWSSHSession* session){
        m_client_session = session;
    }

private:
    pdiutil::vector<uint8_t> m_received_data;
    pdiutil::vector<uint8_t> m_written_data;
    iTcpClientInterface* m_tcpClient;
    LWSSHSession* m_client_session;
    uint32_t m_minSizeToWritePayload;
    int m_writeCommitTaskId;
};

}

#endif // _SSH_SERVICE_CLIENT_INTERFACE_H