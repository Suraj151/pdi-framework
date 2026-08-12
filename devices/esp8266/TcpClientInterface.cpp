/************************* TCP Client Interface *******************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st May 2025
******************************************************************************/
#include "TcpClientInterface.h"
#include "DeviceControlInterface.h"

void TcpClientInterface::onDnsFound(const char *name, const ip_addr_t *ipaddr, void *arg)
{
    TcpClientInterface* self = static_cast<TcpClientInterface*>(arg);
    if (nullptr == self) return;
    if (nullptr != ipaddr) {
        self->m_dns.addr = *ipaddr;
        self->m_dns.found = true;
    }
    self->m_dns.in_flight = false;
}

/**
 * @brief Constructor for TcpClientInterface.
 */
TcpClientInterface::TcpClientInterface() :
    m_pcb(nullptr),
    m_isConnected(false),
    m_rxBuffer(nullptr),
    m_rxBufferSize(0),
    m_timeout(3000),
    m_isLastWriteAcked(true),
    m_dns{IP4_ADDRESS_NONE, false, false} {}

/**
 * @brief Parameterized Constructor for TcpClientInterface.
 * @note This constructor is used when you want to reuse an existing TCP connection.
 *       with this constructor.
 * @param pcb Pointer to an existing TCP protocol control block (pcb).
 */
TcpClientInterface::TcpClientInterface(struct tcp_pcb* pcb):
    m_pcb(pcb),
    m_isConnected(true),
    m_rxBuffer(nullptr),
    m_rxBufferSize(0),
    m_timeout(3000),
    m_isLastWriteAcked(true),
    m_dns{IP4_ADDRESS_NONE, false, false} {

    if (m_pcb) {
        tcp_arg(m_pcb, this);
        tcp_err(m_pcb, &TcpClientInterface::onError);
        tcp_recv(m_pcb, &TcpClientInterface::onReceive);
        tcp_sent(m_pcb, &TcpClientInterface::onSent);

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

        // A previous lookup may still be pending in lwIP if an earlier
        // connect() bailed on timeout. Starting a new lookup while the
        // old callback is still parked on &m_dns would let it overwrite
        // the new query's result, so wait for the old one to clear.
        if (m_dns.in_flight) {
            return PDI_ERR_STATE;
        }

        m_dns.in_flight = true;
        m_dns.found = false;

        err_t err = dns_gethostbyname(hostname, &serverIp, &TcpClientInterface::onDnsFound, this);

        if (err == ERR_OK) {
            // Resolved synchronously (cached) — callback was not invoked
            m_dns.in_flight = false;
        } else if (err == ERR_INPROGRESS) {

            while (m_dns.in_flight && (__i_dvc_ctrl.millis_now() - now) < m_timeout){
                __i_dvc_ctrl.yield();
            }

            if (m_dns.in_flight) {
                // Timed out before lwIP fired. Leave in_flight=true so the
                // next connect() bails until lwIP eventually calls back.
                return NET_ERROR_DNS_FAILED;
            }

            if (!m_dns.found) {
                return NET_ERROR_DNS_FAILED;
            }
            serverIp = m_dns.addr;
        } else {
            // lwIP rejected the request — it will not call the callback
            m_dns.in_flight = false;
            return PDI_ERR_FROM_LWIP(err);
        }
    }

    // the pcb allocation, its callbacks and the connect belong to one section
    // so a callback cannot fire on a half set up pcb
    #ifdef ENABLE_CONTEXTUAL_EXECUTION
    __lwip_mutex.critical_lock();
    #endif

    // Allocate a new TCP protocol control block
    m_pcb = tcp_new();
    if (!m_pcb) {
        #ifdef ENABLE_CONTEXTUAL_EXECUTION
        __lwip_mutex.critical_unlock();
        #endif
        return PDI_ERR_NO_MEM;
    }

    // Set the connection callback
    tcp_arg(m_pcb, this);
    tcp_err(m_pcb, &TcpClientInterface::onError);
    tcp_sent(m_pcb, &TcpClientInterface::onSent);

    // Connect to the server
    err_t err = tcp_connect(m_pcb, &serverIp, port, &TcpClientInterface::onConnected);

    #ifdef ENABLE_CONTEXTUAL_EXECUTION
    __lwip_mutex.critical_unlock();
    #endif
    if (err != ERR_OK) {
        close();
        return PDI_ERR_FROM_LWIP(err); // Return error code if connection fails
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

    // the pcb check and its release belong to one section, and our reference is
    // cleared before leaving it so nothing can reach a closed pcb
    NESTED_CRITICAL_SECTION_ENTER
    if (m_pcb) {

        tcp_arg(m_pcb, NULL);
        tcp_sent(m_pcb, NULL);
        tcp_recv(m_pcb, NULL);
        tcp_err(m_pcb, NULL);
        err_t err = tcp_close(m_pcb);
        if( err != ERR_OK ){
            tcp_abort(m_pcb); // Forcefully abort if close fails
        }
        m_pcb = nullptr;
    }
    m_isConnected = false;
    NESTED_CRITICAL_SECTION_EXIT
    return 0;
}

/**
 * @brief close the session.
 */
int16_t TcpClientInterface::close() {
    int16_t res = disconnect();
    flush();
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

    if (!m_isConnected || !m_pcb) {
        return PDI_ERR_STATE;
    }

    uint8_t flags = TCP_WRITE_FLAG_COPY;

    if (!m_isLastWriteAcked){
        flags |= TCP_WRITE_FLAG_MORE; // do not tcp-PuSH (yet)
    }

    err_t err = ERR_OK;
    int32_t total_sent = 0;

    while (total_sent < size) {

        int32_t remaining = size - total_sent;
        int32_t chunk = std::min(remaining, (int32_t)TCP_SND_BUF); // TCP_SND_BUF is lwIP's max send size

        // uint8_t flags = TCP_WRITE_FLAG_COPY;
        if (chunk < remaining)
            flags |= TCP_WRITE_FLAG_MORE; // do not tcp-PuSH (yet)

        // the pcb is rechecked inside the section, a lwip callback may drop it
        // between chunks otherwise
        #ifdef ENABLE_CONTEXTUAL_EXECUTION
        __lwip_mutex.critical_lock();
        #endif
        err = m_pcb ? tcp_write(m_pcb, c_str + total_sent, chunk, flags) : ERR_CLSD;
        #ifdef ENABLE_CONTEXTUAL_EXECUTION
        __lwip_mutex.critical_unlock();
        #endif
        if (err != ERR_OK) {
            return PDI_ERR_FROM_LWIP(err); // Return error code if write fails
        }

        total_sent += chunk;
    }

    // Ensure last write has been ackowledged
    if (m_isLastWriteAcked){

        #ifdef ENABLE_CONTEXTUAL_EXECUTION
        __lwip_mutex.critical_lock();
        #endif
        m_isLastWriteAcked = false;
        err = m_pcb ? tcp_output(m_pcb) : ERR_CLSD; // Ensure the data is sent
        #ifdef ENABLE_CONTEXTUAL_EXECUTION
        __lwip_mutex.critical_unlock();
        #endif
        if (err != ERR_OK) {
            return PDI_ERR_FROM_LWIP(err); // Return error code if write fails
        }
    }

    // m_isLastWriteAcked = !(size > 0);

    __i_dvc_ctrl.yield();

    return size;
}

/**
 * @brief Write read only string
 */
int32_t TcpClientInterface::write_ro(const char *c_str)
{
    PGM_P p = reinterpret_cast<PGM_P>(c_str);

    uint8_t buff[128] __attribute__ ((aligned(4)));
    auto len = strlen_P(p);
    int32_t n = 0;
    while (n < len) {
        size_t to_write = std::min(sizeof(buff), (size_t)(len - n));
        memcpy_P(buff, p, to_write);
        auto written = write(buff, to_write);
        n += written;
        p += written;
        if (!written) {
            // Some error, write() should write at least 1 byte before returning
            break;
        }
    }
    return n;
}

/**
 * @brief Read data from the server.
 */
int32_t TcpClientInterface::read(uint8_t* buffer, uint32_t size) {

    // the buffer is checked, copied out and consumed in one section, a receive
    // callback may replace it between those otherwise
    NESTED_CRITICAL_SECTION_ENTER

    if (!m_rxBuffer || m_rxBufferSize == 0) {
        NESTED_CRITICAL_SECTION_EXIT
        return PDI_ERR_STATE;
    }

    int32_t bytesToRead = (size < m_rxBufferSize) ? size : m_rxBufferSize;
    memset(buffer, 0, size); // Clear the buffer
    // Copy data from the receive buffer to the provided buffer
    memcpy(buffer, m_rxBuffer, bytesToRead);

    consumeRxBuffer(bytesToRead);

    NESTED_CRITICAL_SECTION_EXIT

    return bytesToRead;
}

/**
 * @brief Drop the leading bytes of the receive buffer and open the tcp window.
 */
void TcpClientInterface::consumeRxBuffer(uint32_t size) {

    // Adjust the receive buffer
    memmove(m_rxBuffer, m_rxBuffer + size, m_rxBufferSize - size);
    m_rxBufferSize -= size;

    if(m_pcb != nullptr){
        NESTED_CRITICAL_SECTION_ENTER
        tcp_recved(m_pcb, size); // Notify the TCP stack that data has been read
        NESTED_CRITICAL_SECTION_EXIT
    }
}

/**
 * @brief Reads until the provided char is found.
 */
void TcpClientInterface::readStringUntil(pdiutil::string &_outstr, char _delimiter, bool _keepdelimiterinstr, CallBackVoidArgFn _yield, uint32_t _maxlen) {

    uint32_t len = 0;

    if (_yield != nullptr) {
        _yield();
    }

    while (m_rxBuffer != nullptr && m_rxBufferSize > 0) {

        uint32_t blocklen = m_rxBufferSize;
        if (_maxlen > 0 && blocklen > (_maxlen - len)) {
            blocklen = _maxlen - len;
        }

        bool delimiterfound = false;
        if (_delimiter != 0) {
            const uint8_t *delimiterat = (const uint8_t *)memchr(m_rxBuffer, _delimiter, blocklen);
            if (delimiterat != nullptr) {
                blocklen = (uint32_t)(delimiterat - m_rxBuffer);
                delimiterfound = true;
            }
        }

        _outstr.append((const char *)m_rxBuffer, blocklen);
        consumeRxBuffer(blocklen + (delimiterfound ? 1 : 0));
        len += blocklen;

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
    return m_rxBufferSize;
}

/**
 * @brief Callback for when the connection is established.
 */
err_t TcpClientInterface::onConnected(void* arg, struct tcp_pcb* tpcb, err_t err) {
    TcpClientInterface* client = static_cast<TcpClientInterface*>(arg);

    if( client ){
        if (err == ERR_OK) {
            NESTED_CRITICAL_SECTION_ENTER
            client->m_isConnected = true;
            tcp_recv(tpcb, &TcpClientInterface::onReceive); // Set the receive callback
            NESTED_CRITICAL_SECTION_EXIT
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

        // the buffer length is read and grown in one section, a reader taking
        // bytes out in between would leave the copy sized against a stale length
        NESTED_CRITICAL_SECTION_ENTER
        // Append the received data to the receive buffer
        uint32_t newSize = client->m_rxBufferSize + p->tot_len;
        uint8_t* newBuffer = pdiutil::safe_new_array<uint8_t>(newSize);
        if (!newBuffer) {
            NESTED_CRITICAL_SECTION_EXIT
#ifndef ENABLE_CONTEXTUAL_EXECUTION
            // lwip callback context, kept off the logger when scheduling is enabled
            SysLogE("TCP onReceive: alloc fail, in=%u rxQ=%u\n",
                (unsigned)p->tot_len, (unsigned)client->m_rxBufferSize);
#endif
            return ERR_MEM;
        }
        if (client->m_rxBuffer) {
            memcpy(newBuffer, client->m_rxBuffer, client->m_rxBufferSize);
            pdiutil::safe_delete_array(client->m_rxBuffer);
        }
        pbuf_copy_partial(p, newBuffer + client->m_rxBufferSize, p->tot_len, 0);
        client->m_rxBuffer = newBuffer;
        client->m_rxBufferSize = newSize;
        NESTED_CRITICAL_SECTION_EXIT

        pbuf_free(p);
    }

    return ERR_OK;
}

/**
 * @brief Callback for when an error occurs.
 */
void TcpClientInterface::onError(void* arg, err_t err) {
    TcpClientInterface* client = static_cast<TcpClientInterface*>(arg);
    if (client) {

        bool haspcb = false;

        // lwip has already freed the pcb before raising this callback, so only
        // drop our reference to it, never call tcp_* on it here
        NESTED_CRITICAL_SECTION_ENTER
        haspcb = (nullptr != client->m_pcb);
        client->m_pcb = nullptr;
        client->m_isConnected = false;
        NESTED_CRITICAL_SECTION_EXIT

        if (haspcb) {
            client->flush();
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
ipaddress_t TcpClientInterface::getRemoteIp() const {
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
 * @brief Set the NoDelay option for TCP.
 * @param noDelay If true, disables Nagle's algorithm (reducing latency).
 */
void TcpClientInterface::setNoDelay(bool noDelay) {
    if (!m_pcb) {
        return; // No active connection
    }

    if (noDelay) {
        m_pcb->so_options |= TF_NODELAY; // Disable Nagle's algorithm
    } else {
        m_pcb->so_options &= ~TF_NODELAY; // Enable Nagle's algorithm
    }
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

    // err_t err = tcp_write_checks(m_pcb, size);
    err_t err = ERR_OK;

    // every pcb field read and the output call belong to one section
    #ifdef ENABLE_CONTEXTUAL_EXECUTION
    __lwip_mutex.critical_lock();
    #endif

    if ( m_pcb &&
        (m_pcb->state != ESTABLISHED) &&
        (m_pcb->state != CLOSE_WAIT) &&
        (m_pcb->state != SYN_SENT) &&
        (m_pcb->state != SYN_RCVD)) {

        // m_isConnected = false;
        err = ERR_CONN;
    }

    if(m_pcb && err == ERR_OK) {

        err_t tcpout_err = tcp_output(m_pcb);  // Ensure the data is sent

        uint32_t availablebuff = tcp_sndbuf(m_pcb);
        uint32_t queuelen = tcp_sndqueuelen(m_pcb);

        if((availablebuff < size) || (queuelen >= TCP_SND_QUEUELEN) || (queuelen > TCP_SNDQUEUELEN_OVERFLOW)){

            err = ERR_MEM;
        }

        // if(!m_isLastWriteAcked && err == ERR_OK && tcpout_err == ERR_OK){

        //     m_isLastWriteAcked = true;
        // }
    }

    #ifdef ENABLE_CONTEXTUAL_EXECUTION
    __lwip_mutex.critical_unlock();
    #endif

    __i_dvc_ctrl.yield();

    return m_isConnected && m_isLastWriteAcked && err == ERR_OK;
}

/**
 * @brief Flush the buffer.
 */
void TcpClientInterface::flush() {

    // the window is reopened and the buffer released in one section so a
    // receive callback cannot grow it in between
    NESTED_CRITICAL_SECTION_ENTER

    if (m_rxBuffer) {

        if(nullptr != m_pcb && m_rxBufferSize > 0){

            tcp_recved(m_pcb, m_rxBufferSize); // Notify the TCP stack that data has been read
        }

        pdiutil::safe_delete_array(m_rxBuffer);
        m_rxBufferSize = 0;
    }

    if(nullptr != m_pcb){

        tcp_output(m_pcb);
    }

    NESTED_CRITICAL_SECTION_EXIT

    m_isLastWriteAcked = true;
}
