#ifndef _TCP_CLIENT_INTERFACE_H_
#define _TCP_CLIENT_INTERFACE_H_

#include "esp8266.h"
#include <interface/pdi/middlewares/iClientInterface.h>


/**
 * @class TcpClientInterface
 * @brief A TCP client implementation using the raw TCP API from LWIP.
 */
class TcpClientInterface : public iTcpClientInterface {
public:
    TcpClientInterface();
    ~TcpClientInterface();

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
    uint32_t write(const uint8_t* c_str, uint32_t size) override;

    /**
     * @brief Writes a single byte of data.
     * @param c The byte to write.
     * @return Number of bytes written.
     */
    uint32_t write(uint8_t c) override {
        return write(&c, 1);
    }

    /**
     * @brief Writes a string of bytes.
     * @param c_str Pointer to the byte string.
     * @return Number of bytes written.
     */
    uint32_t write(const uint8_t *c_str) override {
        return write(c_str, strlen(reinterpret_cast<const char*>(c_str)));
    }

    /**
     * @brief Read data from the socket.
     * @param buffer The buffer to store the read data.
     * @param size The maximum number of bytes to read.
     * @return The number of bytes read, or 0 on failure.
     */
    uint32_t read(uint8_t* buffer, uint32_t size) override;

    /**
     * @brief Reads a single byte of data.
     * @return The byte read.
     */
    uint8_t read() override {
        uint8_t byte = 0;
        read(&byte, 1);
        return byte;
    }
    
    /**
     * @brief Check the number of bytes available to read.
     * @return The number of bytes available, or -1 on error.
     */
    int32_t available() override;

    /**
     * @brief Get the local IP address.
     * @return The local IP address as uint32_t.
     */
    uint32_t getLocalIp() const;

    /**
     * @brief Get the local port.
     * @return The local port number.
     */
    uint16_t getLocalPort() const;

    /**
     * @brief Get the remote IP address.
     * @return The remote IP address as uint32_t.
     */
    uint32_t getRemoteIp() const;

    /**
     * @brief Get the remote port.
     * @return The remote port number.
     */
    uint16_t getRemotePort() const;

    /**
     * @brief Enable TCP keep-alive and configure its parameters.
     * @param idleTime The idle time (in seconds) before sending the first keep-alive packet.
     * @param interval The interval (in seconds) between keep-alive packets.
     * @param count The number of keep-alive packets to send before considering the connection dead.
     * @return True if keep-alive was successfully enabled, false otherwise.
     */
    bool setKeepAlive(uint16_t idleTime, uint16_t interval, uint16_t count);

    /**
     * @brief Set the timeout.
     * @param timeout The timeout value in milliseconds.
     */
    void setTimeout(uint32_t timeout) override;

    /**
     * @brief Get new instance
     */
    iClientInterface* getNewInstance() override {
        return new TcpClientInterface();
    }
    
private:
    struct tcp_pcb* m_pcb; ///< The TCP protocol control block.
    bool m_isConnected;    ///< Connection status.
    uint8_t* m_rxBuffer;   ///< Receive buffer.
    uint32_t m_rxBufferSize; ///< Size of the receive buffer.
    uint32_t m_timeout;    ///< Timeout value in milliseconds.

    // LWIP callback functions
    static err_t onConnected(void* arg, struct tcp_pcb* tpcb, err_t err);
    static err_t onReceive(void* arg, struct tcp_pcb* tpcb, struct pbuf* p, err_t err);
    static void onError(void* arg, err_t err);
};

#endif // _TCP_CLIENT_INTERFACE_H_