/***************************** Light Weight SSH *******************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 6th Apr 2025
******************************************************************************/

#include <config/Config.h>

#if defined(ENABLE_SSH_SERVICE)

#include "SSHClientInterface.h"
#include "SSHServiceUtil.h"

using namespace LWSSH;

/**
 * @brief Connect to a remote server.
 * @param host The IP address of the server.
 * @param port The port number of the server.
 * @return Connection status (0 for success, negative for failure).
 */
int16_t SSHClientInterface::connect(const uint8_t *host, uint16_t port){
    if(m_tcpClient){
        return m_tcpClient->connect(host, port);
    }
    return -1;
}

/**
 * @brief Disconnects the current connection.
 * @return Disconnection status (0 for success, negative for failure).
 */
int16_t SSHClientInterface::disconnect(){
    if(m_tcpClient){
        return m_tcpClient->disconnect();
    }
    return -1;
}

/**
 * @brief Closes the current connection.
 * @return Disconnection status (0 for success, negative for failure).
 */
int16_t SSHClientInterface::close(){

    int16_t res = -1;
    if(m_tcpClient){
        // finish if anything left their and close
        commit();
        res = m_tcpClient->close();
    }
    m_written_data.clear();
    m_received_data.clear();
    __task_scheduler.clearTimeout(m_writeCommitTaskId);
    return res;
}

/**
 * @brief Check if the socket is connected.
 * @return Connection status (1 for connected, 0 for disconnected).
 */
int8_t SSHClientInterface::connected(){
    if(m_tcpClient){
        return m_tcpClient->connected();
    }
    return 0;
}

/**
 * @brief Write data to the server.
 * @param c_str The data to send.
 * @param size The size of the data.
 * @return The number of bytes written, or 0 on failure.
 */
int32_t SSHClientInterface::write(const uint8_t* c_str, uint32_t size){
    if(m_tcpClient && m_client_session && m_client_session->current_channel.ischannelreqsuccess > 1){


        m_written_data.insert(m_written_data.end(), c_str, c_str+size);

        int32_t datasize = m_written_data.size();
        if( datasize > (0.75*m_minSizeToWritePayload) ){

            if(send_channel_data(m_client_session, (const char*)m_written_data.data(), datasize)){
                m_written_data.clear();
                return size;
            }
        }else{

            // Schedule commiting written data task in queue
            m_writeCommitTaskId = __task_scheduler.updateTimeout( m_writeCommitTaskId, [&]() {
                this->commit();
            }, 1, __i_dvc_ctrl.millis_now() );

            return size;
        }
    }
    return 0;
}

/**
 * @brief Writes a single byte of data.
 * @param c The byte to write.
 * @return Number of bytes written.
 */
int32_t SSHClientInterface::write(uint8_t c){ 
    return write(&c, 1); 
}

/**
 * @brief Writes a string of bytes.
 * @param c_str Pointer to the byte string.
 * @return Number of bytes written.
 */
int32_t SSHClientInterface::write(const uint8_t *c_str){
    return write(c_str, strlen(reinterpret_cast<const char*>(c_str)));
}

/**
 * @brief Writes a read-only string.
 * @param c_str The string to write.
 * @return Number of bytes written.
 */
int32_t SSHClientInterface::write_ro(const char *c_str){
    PROG_RODT_PTR p = reinterpret_cast<PROG_RODT_PTR>(c_str);

    uint8_t buff[128];// __attribute__ ((aligned(4)));
    auto len = strlen_ro(p);
    int32_t n = 0;
    while (n < len) {
        int to_write = pdistd::min(sizeof(buff), (size_t)(len - n));
        memcpy_ro(buff, p, to_write);
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
 * @brief Read data from the socket.
 * @param buffer The buffer to store the read data.
 * @param size The maximum number of bytes to read.
 * @return The number of bytes read, or 0 on failure.
 */
int32_t SSHClientInterface::read(uint8_t* buffer, uint32_t size){
    int32_t readedbytes = 0;
    if(m_received_data.size() > 0){

        for (; readedbytes < size && readedbytes < m_received_data.size(); readedbytes++){
            buffer[readedbytes] = m_received_data[readedbytes];
        }

        // remove readed bytes from m_received_data
        m_received_data.erase(m_received_data.begin(), m_received_data.begin() + readedbytes);
        
    }
    return readedbytes;
}

/**
 * @brief Reads a single byte of data.
 * @return The byte read.
 */
uint8_t SSHClientInterface::read(){ 
    uint8_t byte = 0; read(&byte, 1); return byte; 
}

/**
 * @brief Check the number of bytes available to read.
 * @return The number of bytes available, or -1 on error.
 */
int32_t SSHClientInterface::available(){
    return m_received_data.size();
}

/**
 * @brief Get the local IP address.
 * @return The local IP address.
 */
ipaddress_t SSHClientInterface::getLocalIp() const{
    if(m_tcpClient){
        return m_tcpClient->getLocalIp();
    }
    return 0;
}

/**
 * @brief Get the local port.
 * @return The local port number.
 */
uint16_t SSHClientInterface::getLocalPort() const{
    if(m_tcpClient){
        return m_tcpClient->getLocalPort();
    }
    return 0;
}

/**
 * @brief Get the remote IP address.
 * @return The remote IP address.
 */
ipaddress_t SSHClientInterface::getRemoteIp() const{
    if(m_tcpClient){
        return m_tcpClient->getRemoteIp();
    }
    return 0;
}

/**
 * @brief Get the remote port.
 * @return The remote port number.
 */
uint16_t SSHClientInterface::getRemotePort() const{
    if(m_tcpClient){
        return m_tcpClient->getRemotePort();
    }
    return 0;
}

/**
 * @brief Enable TCP keep-alive and configure its parameters.
 * @param idleTime The idle time (in seconds) before sending the first keep-alive packet.
 * @param interval The interval (in seconds) between keep-alive packets.
 * @param count The number of keep-alive packets to send before considering the connection dead.
 * @return True if keep-alive was successfully enabled, false otherwise.
 */
bool SSHClientInterface::setKeepAlive(uint16_t idleTime, uint16_t interval, uint16_t count){
    if(m_tcpClient){
        return m_tcpClient->setKeepAlive(idleTime, interval, count);
    }
    return false;
}

/**
 * @brief Set the NoDelay option for TCP.
 * @param noDelay If true, disables Nagle's algorithm (reducing latency).
 */
void SSHClientInterface::setNoDelay(bool noDelay){
    if(m_tcpClient){
        m_tcpClient->setNoDelay(noDelay);
    }
}

/**
 * @brief Set the timeout.
 * @param timeout The timeout value in milliseconds.
 */
void SSHClientInterface::setTimeout(uint32_t timeout){
    if(m_tcpClient){
        m_tcpClient->setTimeout(timeout);
    }
}

/** 
 * @brief Flush the output buffer.
 */
void SSHClientInterface::flush(){
    if(m_tcpClient){
        m_tcpClient->flush();
        m_received_data.clear();
    }
}

/** 
 * @brief Commit IO operations in case if queued.
 */
int32_t SSHClientInterface::commit(){

    int32_t datasize = m_written_data.size();
    if( datasize > 0 ){

        if(send_channel_data(m_client_session, (const char*)m_written_data.data(), datasize)){
        }
        m_written_data.clear();
        return datasize;
    }
    return -1;
}

/** 
 * @brief set received decrypted payload from client
 */
void SSHClientInterface::setReceivedChannelData(pdiutil::vector<uint8_t> &recvpayload){
    m_received_data.insert(m_received_data.end(), recvpayload.begin(), recvpayload.end());
}


#endif // ENABLE_SSH_SERVICE
