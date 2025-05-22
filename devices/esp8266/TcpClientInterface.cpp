#include "TcpClientInterface.h"

/**
 * @brief Constructor for TcpClientInterface.
 */
TcpClientInterface::TcpClientInterface() : 
    m_pcb(nullptr), 
    m_isConnected(false), 
    m_rxBuffer(nullptr),
    m_rxBufferSize(0),
    m_timeout(3000) {}

/**
 * @brief Destructor for TcpClientInterface.
 */
TcpClientInterface::~TcpClientInterface() {
    close();
}

/**
 * @brief Connect to a remote server async.
 */
int16_t TcpClientInterface::connect(const uint8_t* host, uint16_t port) {
    close(); // Ensure any previous connection is closed

    // Allocate a new TCP protocol control block
    m_pcb = tcp_new();
    if (!m_pcb) {
        return -1;
    }

    // Convert the host to an IP address
    ip_addr_t serverIp;
    if (!ipaddr_aton(reinterpret_cast<const char*>(host), &serverIp)) {
        close();
        return -1;
    }

    // Set the connection callback
    tcp_arg(m_pcb, this);
    tcp_err(m_pcb, &TcpClientInterface::onError);

    // Connect to the server
    err_t err = tcp_connect(m_pcb, &serverIp, port, &TcpClientInterface::onConnected);
    if (err != ERR_OK) {
        close();
        return -1;
    }

    return 0;
}

/**
 * @brief Disconnect from the remote server.
 */
int16_t TcpClientInterface::disconnect() {
    if (m_pcb) {
        tcp_arg(m_pcb, NULL);
        tcp_sent(m_pcb, NULL);
        tcp_recv(m_pcb, NULL);
        tcp_err(m_pcb, NULL);
        tcp_close(m_pcb);
        m_pcb = nullptr;
    }
    m_isConnected = false;
    return 0;
}

/**
 * @brief close the session.
 */
int16_t TcpClientInterface::close() {
    disconnect();
    if (m_rxBuffer) {
        delete[] m_rxBuffer;
        m_rxBuffer = nullptr;
        m_rxBufferSize = 0;
    }
    return 0;
}

/**
 * @brief Check if the connection is active.
 */
int8_t TcpClientInterface::connected() {
    return m_isConnected ? 1 : 0;
}

/**
 * @brief Write data to the server.
 */
int32_t TcpClientInterface::write(const uint8_t* c_str, uint32_t size) {
    if (!m_isConnected || !m_pcb) {
        return 0;
    }

    err_t err = tcp_write(m_pcb, c_str, size, TCP_WRITE_FLAG_COPY);
    if (err != ERR_OK) {
        return 0;
    }

    err = tcp_output(m_pcb); // Ensure the data is sent
    if (err != ERR_OK) {
        return 0;
    }
    return size;
}

/**
 * @brief Read data from the server.
 */
int32_t TcpClientInterface::read(uint8_t* buffer, uint32_t size) {
    if (!m_rxBuffer || m_rxBufferSize == 0) {
        return 0;
    }

    int32_t bytesToRead = (size < m_rxBufferSize) ? size : m_rxBufferSize;
    memset(buffer, 0, size); // Clear the buffer
    // Copy data from the receive buffer to the provided buffer
    memcpy(buffer, m_rxBuffer, bytesToRead);

    // Adjust the receive buffer
    memmove(m_rxBuffer, m_rxBuffer + bytesToRead, m_rxBufferSize - bytesToRead);
    m_rxBufferSize -= bytesToRead;

    return bytesToRead;
}

/**
 * @brief Check the number of bytes available to read.
 */
int32_t TcpClientInterface::available() {
    return m_rxBufferSize;
}

/**
 * @brief Callback for when the connection is established.
 */
err_t TcpClientInterface::onConnected(void* arg, struct tcp_pcb* tpcb, err_t err) {
    TcpClientInterface* client = static_cast<TcpClientInterface*>(arg);

    if( client ){
        if (err == ERR_OK) {
            client->m_isConnected = true;
            tcp_recv(tpcb, &TcpClientInterface::onReceive); // Set the receive callback
        } else {
            client->close();
        }
    }
    return err;
}

/**
 * @brief Callback for when data is received.
 */
err_t TcpClientInterface::onReceive(void* arg, struct tcp_pcb* tpcb, struct pbuf* p, err_t err) {
    TcpClientInterface* client = static_cast<TcpClientInterface*>(arg);

    if( client && tpcb ){

        if (!p) { // Connection closed

            if( err == ERR_OK ){
                client->disconnect();    
            }else{
            }
            return err;
        }

        // Append the received data to the receive buffer
        uint32_t newSize = client->m_rxBufferSize + p->tot_len;
        uint8_t* newBuffer = new uint8_t[newSize];
        if (client->m_rxBuffer) {
            memcpy(newBuffer, client->m_rxBuffer, client->m_rxBufferSize);
            delete[] client->m_rxBuffer;
        }
        pbuf_copy_partial(p, newBuffer + client->m_rxBufferSize, p->tot_len, 0);
        client->m_rxBuffer = newBuffer;
        client->m_rxBufferSize = newSize;
    
        tcp_recved(tpcb, p->tot_len); // Notify the TCP stack that data has been read
        pbuf_free(p); // Free the pbuf
    }

    return ERR_OK;
}

/**
 * @brief Callback for when an error occurs.
 */
void TcpClientInterface::onError(void* arg, err_t err) {
    TcpClientInterface* client = static_cast<TcpClientInterface*>(arg);
    if (client) {
        tcp_arg(client->m_pcb, NULL); // Updated to use client->m_pcb
        tcp_sent(client->m_pcb, NULL);
        tcp_recv(client->m_pcb, NULL);
        tcp_err(client->m_pcb, NULL);
        client->m_pcb = nullptr;
        client->disconnect();
    }
}

/**
 * @brief Get the local IP address.
 */
uint32_t TcpClientInterface::getLocalIp() const {
    // if (!m_pcb) {
    //     return "";
    // }
    // char ipStr[16];
    // ipaddr_ntoa_r(&m_pcb->local_ip, ipStr, sizeof(ipStr));
    // return std::string(ipStr);

    if (!m_pcb) {
        return 0;
    }
    return (m_pcb->local_ip.addr);
}

/**
 * @brief Get the local port.
 */
uint16_t TcpClientInterface::getLocalPort() const {
    if (!m_pcb) {
        return 0;
    }
    return m_pcb->local_port;
}

/**
 * @brief Get the remote IP address.
 */
uint32_t TcpClientInterface::getRemoteIp() const {
    if (!m_pcb) {
        return 0;
    }
    return (m_pcb->remote_ip.addr);
}

/**
 * @brief Get the remote port.
 */
uint16_t TcpClientInterface::getRemotePort() const {
    if (!m_pcb) {
        return 0;
    }
    return m_pcb->remote_port;
}

/**
 * @brief Enable TCP keep-alive and configure its parameters.
 */
bool TcpClientInterface::setKeepAlive(uint16_t idleTime, uint16_t interval, uint16_t count) {
    if (!m_pcb) {
        return false; // No active connection
    }

    if (0 == idleTime || 0 == interval || 0 == count) {
        m_pcb->so_options &= ~SOF_KEEPALIVE;
        return true;
    }

    // Enable TCP keep-alive
    m_pcb->so_options |= SOF_KEEPALIVE;

    // Set the keep-alive idle time (in seconds)
    m_pcb->keep_idle = idleTime * 1000; // Convert to milliseconds

    // Set the keep-alive interval (in seconds)
    m_pcb->keep_intvl = interval * 1000; // Convert to milliseconds

    // Set the number of keep-alive probes
    m_pcb->keep_cnt = count;

    return true;
}

/**
 * @brief Set the timeout.
 * @param timeout The timeout value in milliseconds.
 */
void TcpClientInterface::setTimeout(uint32_t timeout) {
    m_timeout = timeout;
}
