/************************** Telnet service ***********************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st May 2025
******************************************************************************/
#include <config/Config.h>

#if defined(ENABLE_TELNET_SERVICE)

#include "TelnetServiceProvider.h"
#ifdef ENABLE_CMD_SERVICE
#include <service_provider/cmd/CommandLineServiceProvider.h>
#endif

/**
 * @brief Constructor for TelnetServiceProvider.
 * Initializes the Telnet service provider with a new TcpServerInterface instance.
 * Sets the service type to SERVICE_TELNET and the service name to "Telnet".
 */
TelnetServiceProvider::TelnetServiceProvider() : 
    m_server(nullptr), 
    m_client(nullptr),
    ServiceProvider(SERVICE_TELNET, RODT_ATTR("Telnet")) 
{}


/**
 * @brief Destructor for TelnetServiceProvider.
 */
TelnetServiceProvider::~TelnetServiceProvider() {
    stop();
    if(m_server) {
        delete m_server;
    }
}

/**
 * @brief Start the Telnet service on the specified port.
 */
bool TelnetServiceProvider::start(uint16_t port) {
    if (!m_server) {
        m_server = new TcpServerInterface();
    }

    if (m_server->begin(port) == 0) {
        return true;
    }

    return false;
}

/**
 * @brief Start the Telnet service on the specified port.
 */
bool TelnetServiceProvider::initService(void *arg) {

    bool started = false;

    if (arg) {
        
        uint16_t port = *(uint16_t*)arg;
        started = start(port);
    }else{

        started = start();
    }

    // If the service started successfully, set up a periodic task to handle incoming clients
    if(started){
        __task_scheduler.setInterval( [&]() {
            this->handle();
        }, 1, __i_dvc_ctrl.millis_now() );
    }
    
    return started && ServiceProvider::initService(arg);
}

/**
 * @brief Stop the Telnet service.
 */
void TelnetServiceProvider::stop() {
    closeClient();
    if (m_server) {
        m_server->close();
    }
}

/**
 * @brief close current client.
 */
void TelnetServiceProvider::closeClient() {
    if (m_client) {
        #ifdef ENABLE_CMD_SERVICE
        // Notify command service to stop using the terminal
        __cmd_service.useTerminal(nullptr);
        // Inform serial terminal about the end of telnet client session
        if(__i_dvc_ctrl.getTerminal(TERMINAL_TYPE_SERIAL)){
            __i_dvc_ctrl.getTerminal(TERMINAL_TYPE_SERIAL)->writeln();
            __i_dvc_ctrl.getTerminal(TERMINAL_TYPE_SERIAL)->writeln_ro(RODT_ATTR("Telnet Client Session ended."));
        }
        #endif
        m_client->close();
        delete m_client;
        m_client = nullptr;
    }
}

/**
 * @brief Handle incoming Telnet clients and data.
 */
void TelnetServiceProvider::handle() {
    if (!m_server) {
        return; // Server not initialized
    }
    // Check if there is a new client connection
    if (!m_client && m_server->hasClient()) {
        m_client = m_server->accept();
        if (m_client) {
            // Optionally send welcome message
            // const char* welcome = "Welcome to Telnet Service!\r\n";
            // m_client->write((const uint8_t*)welcome, strlen(welcome));

            // process and start interaction with telnet remote client
            m_client->set_terminal_type(TERMINAL_TYPE_TELNET);
            #ifdef ENABLE_CMD_SERVICE
            // Inform serial terminal about the new telnet client session
            if(__i_dvc_ctrl.getTerminal(TERMINAL_TYPE_SERIAL)){
                __i_dvc_ctrl.getTerminal(TERMINAL_TYPE_SERIAL)->writeln();
                __i_dvc_ctrl.getTerminal(TERMINAL_TYPE_SERIAL)->writeln_ro(RODT_ATTR("Telnet Client Session starting."));
            }

            iTerminalInterface *current_terminal = __cmd_service.getTerminal();
            if( current_terminal && TERMINAL_TYPE_SSH == current_terminal->get_terminal_type() ){

                if(__i_dvc_ctrl.getTerminal(TERMINAL_TYPE_SERIAL)){
                    __i_dvc_ctrl.getTerminal(TERMINAL_TYPE_SERIAL)->writeln();
                    __i_dvc_ctrl.getTerminal(TERMINAL_TYPE_SERIAL)->writeln_ro(RODT_ATTR("Unable to start telnet client session."));
                }

                m_client->writeln_ro(RODT_ATTR("Can't start session due to existing ssh session."));
                closeClient();
            }else{
                __cmd_service.useTerminal(m_client);
            }
            #endif            
        }
    }

    if (m_client && m_client->connected()) {

        if( m_client->available() ){

            // process and execute if command has provided
            #ifdef ENABLE_CMD_SERVICE
            cmd_result_t res = __cmd_service.processTerminalInput(m_client);
            if( res == CMD_RESULT_ABORTED ||
                res == CMD_RESULT_TERMINAL_ABORTED
            ){
                m_client->disconnect();
            }
            #endif

            // Flush the client buffer
            m_client->flush();
        }
    } else if (m_client && !m_client->connected()) {
        closeClient();
    }
}

TelnetServiceProvider __telnet_service;

#endif // ENABLE_TELNET_SERVICE