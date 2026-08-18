/************************* TCP Client Interface *******************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st May 2025
******************************************************************************/
#include "TcpClientInterface.h"
#include "DeviceControlInterface.h"

// #define TCP_GUARD_BEGIN
// #define TCP_GUARD_END
#define TCP_GUARD_BEGIN \
    bool _pdi_need_lock = !sys_thread_tcpip(LWIP_CORE_LOCK_QUERY_HOLDER); \
    if (_pdi_need_lock) { LOCK_LWIP_TCPIP_CORE; }
#define TCP_GUARD_END \
    if (_pdi_need_lock) { UNLOCK_LWIP_TCPIP_CORE; }

/**
 * @brief Constructor for TcpClientInterface.
 */
TcpClientInterface::TcpClientInterface() : 
    m_pcb(nullptr), 
    m_isConnected(false), 
    m_rxBuf(nullptr),
    m_rxBufOffset(0),
    m_timeout(3000),
    m_isLastWriteAcked(true) {}

/**
 * @brief Parameterized Constructor for TcpClientInterface.
 * @note This constructor is used when you want to reuse an existing TCP connection.
 *       with this constructor.
 * @param pcb Pointer to an existing TCP protocol control block (pcb).
 */
TcpClientInterface::TcpClientInterface(struct tcp_pcb* pcb):
    m_pcb(pcb),
    m_isConnected(true),
    m_rxBuf(nullptr),
    m_rxBufOffset(0),
    m_timeout(3000),
    m_isLastWriteAcked(true) {

    if (m_pcb) {
        TCP_GUARD_BEGIN
        tcp_arg(m_pcb, this);
        tcp_err(m_pcb, &TcpClientInterface::onError);
        tcp_recv(m_pcb, &TcpClientInterface::onReceive);
        tcp_sent(m_pcb, &TcpClientInterface::onSent);
        TCP_GUARD_END

        setNoDelay(true);
    }
}

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

    uint32_t now = __i_dvc_ctrl.millis_now();
    const char* hostname = reinterpret_cast<const char*>(host);
    ip_addr_t serverIp;

    // Convert the host to an IP address
    if (!ipaddr_aton(hostname, &serverIp)) {
        // It's a hostname, resolve via DNS
        struct addrinfo *res = nullptr;
        struct addrinfo hints;
        memset(&hints, 0, sizeof(hints));
        hints.ai_family = AF_INET; // hint for inet

        err_t err = lwip_getaddrinfo(hostname, "0", &hints, &res);

        if (err == ERR_OK && nullptr != res) {
            // Resolved connect now
            if (res->ai_family == AF_INET6) {
                // for now consider only ip4
            }else{
                serverIp.type = IPADDR_TYPE_V4;
                serverIp.u_addr.ip4.addr = ((struct sockaddr_in *)res->ai_addr)->sin_addr.s_addr;
            }
            lwip_freeaddrinfo(res);
        } else {
            if (nullptr != res) lwip_freeaddrinfo(res);
            return PDI_ERR_STATE;
        }
    }

    TCP_GUARD_BEGIN
    m_pcb = tcp_new();
    if (!m_pcb) {
        TCP_GUARD_END
        return PDI_ERR_NO_MEM;
    }

    tcp_arg(m_pcb, this);
    tcp_err(m_pcb, &TcpClientInterface::onError);
    tcp_sent(m_pcb, &TcpClientInterface::onSent);

    err_t err = tcp_connect(m_pcb, &serverIp, port, &TcpClientInterface::onConnected);
    TCP_GUARD_END
    if (err != ERR_OK) {
        close();
        return PDI_ERR_FROM_LWIP(err);
    }
    setNoDelay(true);

    while (!connected() && (__i_dvc_ctrl.millis_now() - now) < m_timeout){
        __i_dvc_ctrl.yield();
    }

    if( !connected() ) {
        close();
        return PDI_ERR_TIMEOUT;  // timeout
    }    

    return 0;
}

/**
 * @brief Disconnect from the remote server.
 */
int16_t TcpClientInterface::disconnect() {
    TCP_GUARD_BEGIN
    if (m_pcb) {
        tcp_arg(m_pcb, NULL);
        tcp_sent(m_pcb, NULL);
        tcp_recv(m_pcb, NULL);
        tcp_err(m_pcb, NULL);
        err_t err = tcp_close(m_pcb);
        if( err != ERR_OK ){
            tcp_abort(m_pcb);
        }
        m_pcb = nullptr;
    }
    m_isConnected = false;
    TCP_GUARD_END
    return 0;
}

/**
 * @brief close the session.
 */
int16_t TcpClientInterface::close() {
    int16_t res = disconnect();
    flush(FLUSH_ALL);
    return res;
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

    TCP_GUARD_BEGIN
    if (!m_isConnected || !m_pcb) {
        TCP_GUARD_END
        return PDI_ERR_STATE;
    }

    uint8_t flags = TCP_WRITE_FLAG_COPY;

    if (!m_isLastWriteAcked){
        flags |= TCP_WRITE_FLAG_MORE;
    }

    err_t err = ERR_OK;
    int32_t total_sent = 0;

    while (total_sent < size) {

        int32_t remaining = size - total_sent;
        int32_t chunk = std::min(remaining, (int32_t)TCP_SND_BUF);

        if (chunk < remaining)
            flags |= TCP_WRITE_FLAG_MORE;

        err = tcp_write(m_pcb, c_str + total_sent, chunk, flags);
        if (err != ERR_OK) {
            TCP_GUARD_END
            return PDI_ERR_FROM_LWIP(err);
        }

        total_sent += chunk;
    }

    if (m_isLastWriteAcked){

        m_isLastWriteAcked = false;
        err = tcp_output(m_pcb);
        if (err != ERR_OK) {
            TCP_GUARD_END
            return PDI_ERR_FROM_LWIP(err);
        }
    }
    TCP_GUARD_END

    __i_dvc_ctrl.yield();

    return size;
}

/**
 * @brief Write read only string
 */
int32_t TcpClientInterface::write_ro(const char *c_str)
{
    return write(reinterpret_cast<const uint8_t*>(c_str), strlen_P(c_str));
}

/**
 * @brief Read data from the server.
 */
int32_t TcpClientInterface::read(uint8_t* buffer, uint32_t size) {
    TCP_GUARD_BEGIN
    if (!m_rxBuf || size == 0) {
        TCP_GUARD_END
        return PDI_ERR_STATE;
    }

    memset(buffer, 0, size);

    uint32_t copied = 0;

    while (copied < size && m_rxBuf != nullptr) {

        uint32_t headleft = m_rxBuf->len - m_rxBufOffset;
        if (headleft == 0) {
            break;
        }

        uint32_t toread = (size - copied) < headleft ? (size - copied) : headleft;

        // Copy data from the head of the receive chain to the provided buffer
        memcpy(buffer + copied, (const uint8_t*)m_rxBuf->payload + m_rxBufOffset, toread);

        consumeRxBuffer(toread);
        copied += toread;
    }
    TCP_GUARD_END

    return copied;
}

/**
 * @brief Drop the leading bytes of the receive chain and open the tcp window.
 * Releases each buffer as it is emptied, so nothing is copied or moved.
 * @note The caller holds the lwip core lock.
 */
void TcpClientInterface::consumeRxBuffer(uint32_t size) {

    uint32_t consumed = 0;

    while (size > 0 && m_rxBuf != nullptr) {

        uint32_t headleft = m_rxBuf->len - m_rxBufOffset;
        uint32_t taken = size < headleft ? size : headleft;

        if (taken < headleft) {

            m_rxBufOffset += taken;
        } else {

            struct pbuf *released = m_rxBuf;
            m_rxBuf = m_rxBuf->next;
            m_rxBufOffset = 0;
            if (m_rxBuf != nullptr) {
                pbuf_ref(m_rxBuf);
            }
            pbuf_free(released);
        }

        size -= taken;
        consumed += taken;
    }

    if(m_pcb != nullptr && consumed > 0)
        tcp_recved(m_pcb, consumed);
}

/**
 * @brief Reads until the provided char is found.
 */
void TcpClientInterface::readStringUntil(pdiutil::string &_outstr, char _delimiter, bool _keepdelimiterinstr, const CallBackVoidArgFn &_yield, uint32_t _maxlen) {

    uint32_t len = 0;

    if (_yield != nullptr) {
        _yield();
    }

    while (1) {

        bool delimiterfound = false;
        bool blockread = false;

        // The lwip thread can grow the receive buffer at any time, so the scan
        // and the copy out of it stay inside the core lock.
        TCP_GUARD_BEGIN
        if (m_rxBuf != nullptr && (m_rxBuf->len - m_rxBufOffset) > 0) {

            const uint8_t *head = (const uint8_t *)m_rxBuf->payload + m_rxBufOffset;
            uint32_t blocklen = m_rxBuf->len - m_rxBufOffset;
            if (_maxlen > 0 && blocklen > (_maxlen - len)) {
                blocklen = _maxlen - len;
            }

            if (_delimiter != 0) {
                const uint8_t *delimiterat = (const uint8_t *)memchr(head, _delimiter, blocklen);
                if (delimiterat != nullptr) {
                    blocklen = (uint32_t)(delimiterat - head);
                    delimiterfound = true;
                }
            }

            _outstr.append((const char *)head, blocklen);
            consumeRxBuffer(blocklen + (delimiterfound ? 1 : 0));
            len += blocklen;
            blockread = true;
        }
        TCP_GUARD_END

        if (!blockread) {
            break;
        }

        if (delimiterfound) {
            if (_keepdelimiterinstr) {
                _outstr += _delimiter;
            }
            break;
        }

        if (_maxlen > 0 && len >= _maxlen) {
            break; // Stop reading if max length is reached
        }

        if (_yield != nullptr) {
            _yield();
        }
    }
}

/**
 * @brief Check the number of bytes available to read.
 */
int32_t TcpClientInterface::available() {
    TCP_GUARD_BEGIN
    int32_t v = m_rxBuf != nullptr ? (int32_t)(m_rxBuf->tot_len - m_rxBufOffset) : 0;
    TCP_GUARD_END
    return v;
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

        if (!p) {
            if( err == ERR_OK ){
                client->disconnect();
            }
            return err;
        }

        // The chain is kept as delivered, so nothing is copied and no heap is
        // taken. The tcp window stays closed until the reader consumes it, which
        // is what holds the sender back.
        TCP_GUARD_BEGIN
        if (client->m_rxBuf != nullptr) {

            pbuf_cat(client->m_rxBuf, p);
        } else {

            client->m_rxBuf = p;
            client->m_rxBufOffset = 0;
        }
        TCP_GUARD_END
    }

    return ERR_OK;
}

/**
 * @brief Callback for when an error occurs.
 */
void TcpClientInterface::onError(void* arg, err_t err) {
    TcpClientInterface* client = static_cast<TcpClientInterface*>(arg);
    if (client) {

        // lwip has already freed the pcb by the time it reports the error, so
        // drop it before anything else and never call tcp_* on it here
        bool haspcb = (nullptr != client->m_pcb);
        client->m_pcb = nullptr;
        client->m_isConnected = false;

        if (haspcb) {
            client->flush(FLUSH_ALL);
        }
    }
}

/**
 * @brief Callback for when an data sent occurs.
 */
err_t TcpClientInterface::onSent(void* arg, struct tcp_pcb* tpcb, u16_t len) {
    TcpClientInterface* client = static_cast<TcpClientInterface*>(arg);
    if (client) {
        client->m_isLastWriteAcked = true;
    }
    return ERR_OK;
}

/**
 * @brief Get the local IP address.
 */
ipaddress_t TcpClientInterface::getLocalIp() const {
    TCP_GUARD_BEGIN
    ipaddress_t v = m_pcb ? m_pcb->local_ip.u_addr.ip4.addr : 0;
    TCP_GUARD_END
    return v;
}

/**
 * @brief Get the local port.
 */
uint16_t TcpClientInterface::getLocalPort() const {
    TCP_GUARD_BEGIN
    uint16_t v = m_pcb ? m_pcb->local_port : 0;
    TCP_GUARD_END
    return v;
}

/**
 * @brief Get the remote IP address.
 */
ipaddress_t TcpClientInterface::getRemoteIp() const {
    TCP_GUARD_BEGIN
    ipaddress_t v = m_pcb ? m_pcb->remote_ip.u_addr.ip4.addr : 0;
    TCP_GUARD_END
    return v;
}

/**
 * @brief Get the remote port.
 */
uint16_t TcpClientInterface::getRemotePort() const {
    TCP_GUARD_BEGIN
    uint16_t v = m_pcb ? m_pcb->remote_port : 0;
    TCP_GUARD_END
    return v;
}

/**
 * @brief Enable TCP keep-alive and configure its parameters.
 */
bool TcpClientInterface::setKeepAlive(uint16_t idleTime, uint16_t interval, uint16_t count) {
    TCP_GUARD_BEGIN
    if (!m_pcb) {
        TCP_GUARD_END
        return false;
    }

    if (0 == idleTime || 0 == interval || 0 == count) {
        m_pcb->so_options &= ~SOF_KEEPALIVE;
        TCP_GUARD_END
        return true;
    }

    m_pcb->so_options |= SOF_KEEPALIVE;
    m_pcb->keep_idle = idleTime * 1000;
    m_pcb->keep_intvl = interval * 1000;
    m_pcb->keep_cnt = count;
    TCP_GUARD_END

    return true;
}

/**
 * @brief Set the NoDelay option for TCP.
 * @param noDelay If true, disables Nagle's algorithm (reducing latency).
 */
void TcpClientInterface::setNoDelay(bool noDelay) {
    TCP_GUARD_BEGIN
    if (m_pcb) {
        if (noDelay) {
            m_pcb->so_options |= TF_NODELAY;
        } else {
            m_pcb->so_options &= ~TF_NODELAY;
        }
    }
    TCP_GUARD_END
}

/**
 * @brief Set the timeout.
 * @param timeout The timeout value in milliseconds.
 */
void TcpClientInterface::setTimeout(uint32_t timeout) {
    m_timeout = timeout;
}

/**
 * @brief Check whether available for write
 */
bool TcpClientInterface::availableforwrite(uint32_t size) {

    err_t err = ERR_OK;
    bool isConn, isAcked;

    TCP_GUARD_BEGIN
    if ( m_pcb &&
        (m_pcb->state != ESTABLISHED) &&
        (m_pcb->state != CLOSE_WAIT) &&
        (m_pcb->state != SYN_SENT) &&
        (m_pcb->state != SYN_RCVD)) {

        err = ERR_CONN;
    }

    if(m_pcb && err == ERR_OK) {

        tcp_output(m_pcb);
        uint32_t availablebuff = tcp_sndbuf(m_pcb);
        uint32_t queuelen = tcp_sndqueuelen(m_pcb);

        if((availablebuff < size) || (queuelen >= TCP_SND_QUEUELEN) || (queuelen > TCP_SNDQUEUELEN_OVERFLOW)){
            err = ERR_MEM;
        }
    }
    isConn = m_isConnected;
    isAcked = m_isLastWriteAcked;
    TCP_GUARD_END

    __i_dvc_ctrl.yield();

    return isConn && isAcked && err == ERR_OK;
}

/**
 * @brief Flush the buffer.
 */
void TcpClientInterface::flush(int16_t flushtype) {
    TCP_GUARD_BEGIN
    if (IsFlushRx(flushtype) && m_rxBuf) {

        uint32_t pending = m_rxBuf->tot_len - m_rxBufOffset;

        if(nullptr != m_pcb && pending > 0)
            tcp_recved(m_pcb, pending);

        pbuf_free(m_rxBuf);
        m_rxBuf = nullptr;
        m_rxBufOffset = 0;
    }

    if(IsFlushTx(flushtype) && nullptr != m_pcb){
        tcp_output(m_pcb);
    }

    m_isLastWriteAcked = true;
    TCP_GUARD_END
}
