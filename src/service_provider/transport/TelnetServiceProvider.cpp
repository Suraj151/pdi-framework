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
#include <service_provider/session/SessionManager.h>
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
    m_last_activity(0),
    ServiceProvider(SERVICE_TELNET, RODT_ATTR("Telnet"))
{}


/**
 * @brief Destructor for TelnetServiceProvider.
 */
TelnetServiceProvider::~TelnetServiceProvider() {
    stop();
    if(m_server) {
        pdiutil::safe_delete(m_server);
    }
}

/**
 * @brief Start the Telnet service on the specified port.
 */
bool TelnetServiceProvider::start(uint16_t port) {
    if (!m_server) {
        m_server = pdiutil::safe_new<TcpServerInterface>();
    }

    if (!m_server) {
        return false;
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
        this->serviceSetInterval( [&]() {
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
        SessionManager::detach(m_client);
        if(__i_dvc_ctrl.getTerminal(TERMINAL_TYPE_SERIAL)){
            __i_dvc_ctrl.getTerminal(TERMINAL_TYPE_SERIAL)->writeln();
            __i_dvc_ctrl.getTerminal(TERMINAL_TYPE_SERIAL)->writeln_ro(RODT_ATTR("Telnet Client Session ended."));
        }
        #endif
        m_client->close();
        pdiutil::safe_delete(m_client);
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
                __i_dvc_ctrl.getTerminal(TERMINAL_TYPE_SERIAL)->writeln_ro(RODT_ATTR("Telnet Client Session started."));
            }

            if( !__cmd_service.useTerminal(m_client) ){
                closeClient();
                return;
            }
            #endif
            m_last_activity = __i_dvc_ctrl.millis_now();
        }
    }

    if (m_client && m_client->connected()) {

        session_t *termsession = SessionManager::findByTerminal(m_client);

        if( nullptr != termsession && __cmd_service.isSessionBusy(termsession) ){
            termsession->m_lastActivityAt = (uint32_t)__i_dvc_ctrl.millis_now();
            m_last_activity = __i_dvc_ctrl.millis_now();
        }else{

            uint32_t idle = nullptr != termsession ?
                ((uint32_t)__i_dvc_ctrl.millis_now() - termsession->m_lastActivityAt) :
                ((uint32_t)__i_dvc_ctrl.millis_now() - m_last_activity);

            if( idle > TELNET_SHELL_IDLE_MS ){
                closeClient();
                return;
            }
        }

        if( m_client->available() ){

            m_last_activity = __i_dvc_ctrl.millis_now();

            // process and execute if command has provided
            #ifdef ENABLE_CMD_SERVICE
            cmd_result_t res = __cmd_service.processTerminalInput(m_client);
            // Only an explicit terminal abort (logout / EOF) closes the channel.
            // CMD_RESULT_ABORTED = command-scope Ctrl+C/Ctrl+Z; session stays.
            if( res == CMD_RESULT_TERMINAL_ABORTED ){
                __auth_service.setAuthorized(false);
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