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
#include "TelnetClientInterface.h"
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
    m_poolfullsince(0),
    ServiceProvider(SERVICE_TELNET, RODT_ATTR("Telnet"))
{
    for (uint8_t i = 0; i < TELNET_MAX_SESSIONS; i++) {
        m_clients[i] = nullptr;
        m_last_activity[i] = 0;
    }
}


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

        iTerminalInterface *line = serviceBootLine();
        if(nullptr != line){
            line->write_ro(RODT_ATTR("listening on port "));
            line->write((uint32_t)(arg ? *(uint16_t*)arg : TELNET_DEFAULT_PORT));
            line->write_ro(RODT_ATTR(", "));
            line->write((uint32_t)TELNET_MAX_SESSIONS);
            line->writeln_ro(RODT_ATTR(" session slots"));
        }
    }

    return started && ServiceProvider::initService(arg);
}

/**
 * @brief Stop the Telnet service.
 */
void TelnetServiceProvider::stop() {
    closeAllClients();
    if (m_server) {
        m_server->close();
    }
}

/**
 * Clear the pool-full grace window, so a restart does not open with a period
 * that appears to have been full since the previous run.
 */
void TelnetServiceProvider::resetServiceState() {

    m_poolfullsince = 0;
}

/**
 * Releases the listener and every pooled client, so a stopped telnet service
 * refuses a connection rather than leaving it to hang without a banner.
 */
bool TelnetServiceProvider::stopService() {
    stop();
    return ServiceProvider::stopService();
}

/**
 * @brief close the client held in one pool slot.
 */
void TelnetServiceProvider::closeClient(uint8_t slot) {
    if (slot >= TELNET_MAX_SESSIONS || nullptr == m_clients[slot]) {
        return;
    }

    #ifdef ENABLE_CMD_SERVICE
    SessionManager::detach(m_clients[slot]);
    if(__i_dvc_ctrl.getTerminal(TERMINAL_TYPE_SERIAL)){
        __i_dvc_ctrl.getTerminal(TERMINAL_TYPE_SERIAL)->writeln();
        __i_dvc_ctrl.getTerminal(TERMINAL_TYPE_SERIAL)->write_ro(RODT_ATTR("Telnet #"));
        __i_dvc_ctrl.getTerminal(TERMINAL_TYPE_SERIAL)->write((int32_t)slot);
        __i_dvc_ctrl.getTerminal(TERMINAL_TYPE_SERIAL)->writeln_ro(RODT_ATTR(" Client Session ended."));
    }
    #endif
    m_clients[slot]->close();
    pdiutil::safe_delete(m_clients[slot]);
    m_clients[slot] = nullptr;
    m_last_activity[slot] = 0;
}

/**
 * @brief close every client the pool holds.
 */
void TelnetServiceProvider::closeAllClients() {
    for (uint8_t i = 0; i < TELNET_MAX_SESSIONS; i++) {
        closeClient(i);
    }
}

/**
 * @brief Handle incoming Telnet clients and data.
 */
void TelnetServiceProvider::handle() {
    if (!m_server) {
        return; // Server not initialized
    }

    // A client that has already gone keeps its slot until it is serviced, so
    // reap those first or an arriving client is refused a slot that is free in
    // all but name.
    for (uint8_t i = 0; i < TELNET_MAX_SESSIONS; i++) {
        if (nullptr != m_clients[i] && !m_clients[i]->connected()) {
            closeClient(i);
        }
    }

    // The grace times how long a waiting client is held, so a period with
    // nobody queued is not one. Left standing, the next client to arrive would
    // find a grace that expired without it and be refused at once.
    if (!m_server->hasClient()) {
        m_poolfullsince = 0;
    }

    // Accept new client connections into any free pool slot. A client that
    // arrives while the pool is full is refused rather than left connected.
    while (m_server->hasClient()) {

        int8_t slot = -1;
        for (uint8_t i = 0; i < TELNET_MAX_SESSIONS; i++) {
            if (nullptr == m_clients[i]) { slot = i; break; }
        }

        if (slot < 0) {

            // the connection is already established, so a client left queued
            // here waits on a prompt that never comes. Give a slot a short
            // while to free, then say so and close rather than hold it.
            uint32_t now = (uint32_t)__i_dvc_ctrl.millis_now();

            if (0 == m_poolfullsince) {
                m_poolfullsince = now;
                break;
            }

            if ((now - m_poolfullsince) < TELNET_POOL_FULL_GRACE_MS) {
                break;
            }

            iClientInterface* refused = m_server->accept();
            if (nullptr == refused) {
                break;
            }

            refused->writeln();
            refused->writeln_ro(RODT_ATTR("no telnet session available, try again later"));
            refused->close();
            pdiutil::safe_delete(refused);

            // the next client waits its own grace rather than inheriting the
            // remains of this one's
            m_poolfullsince = (uint32_t)__i_dvc_ctrl.millis_now();
            continue;
        }

        m_poolfullsince = 0;

        iClientInterface* accepted = m_server->accept();
        if (nullptr == accepted) {
            break;
        }

        m_clients[slot] = pdiutil::safe_new<TelnetClientInterface>(static_cast<iTcpClientInterface*>(accepted));
        if (nullptr == m_clients[slot]) {
            accepted->close();
            pdiutil::safe_delete(accepted);
            break;
        }

        // process and start interaction with telnet remote client
        m_clients[slot]->set_terminal_type(TERMINAL_TYPE_TELNET);
        #ifdef ENABLE_CMD_SERVICE
        // Inform serial terminal about the new telnet client session
        if(__i_dvc_ctrl.getTerminal(TERMINAL_TYPE_SERIAL)){
            __i_dvc_ctrl.getTerminal(TERMINAL_TYPE_SERIAL)->writeln();
            __i_dvc_ctrl.getTerminal(TERMINAL_TYPE_SERIAL)->write_ro(RODT_ATTR("Telnet #"));
            __i_dvc_ctrl.getTerminal(TERMINAL_TYPE_SERIAL)->write((int32_t)slot);
            __i_dvc_ctrl.getTerminal(TERMINAL_TYPE_SERIAL)->writeln_ro(RODT_ATTR(" Client Session started."));
        }

        if( !__cmd_service.useTerminal(m_clients[slot]) ){
            closeClient(slot);
            continue;
        }
        #endif
        m_last_activity[slot] = __i_dvc_ctrl.millis_now();
    }

    for (uint8_t i = 0; i < TELNET_MAX_SESSIONS; i++) {
        serviceClient(i);
    }
}

/**
 * @brief Run the idle check and the input pass for one pool slot.
 */
void TelnetServiceProvider::serviceClient(uint8_t slot) {

    iClientInterface *client = m_clients[slot];
    if (nullptr == client) {
        return;
    }

    if (!client->connected()) {
        closeClient(slot);
        return;
    }

    session_t *termsession = SessionManager::findByTerminal(client);

    #ifdef ENABLE_CMD_SERVICE
    if( nullptr != termsession && __cmd_service.isSessionBusy(termsession) ){
        termsession->m_lastActivityAt = (uint32_t)__i_dvc_ctrl.millis_now();
        m_last_activity[slot] = __i_dvc_ctrl.millis_now();
    }else
    #endif
    {

        uint32_t idle = nullptr != termsession ?
            ((uint32_t)__i_dvc_ctrl.millis_now() - termsession->m_lastActivityAt) :
            ((uint32_t)__i_dvc_ctrl.millis_now() - m_last_activity[slot]);

        if( idle > TELNET_SHELL_IDLE_MS ){
            closeClient(slot);
            return;
        }
    }

    if( client->available() ){

        m_last_activity[slot] = __i_dvc_ctrl.millis_now();

        // process and execute if command has provided
        #ifdef ENABLE_CMD_SERVICE
        pdi_err_t res = __cmd_service.processTerminalInput(client);
        // Only an explicit terminal abort (logout / EOF) closes the channel.
        // CMD_ERROR_CANCELED = command-scope Ctrl+C/Ctrl+Z; session stays.
        if( res == CMD_ERROR_INTR ){
            #ifdef ENABLE_AUTH_SERVICE
            __auth_service.setAuthorized(false);
            #endif
            client->disconnect();
        }
        #endif

        // Flush the client buffer
        client->flush();
    }
}

TelnetServiceProvider __telnet_service;

#endif // ENABLE_TELNET_SERVICE