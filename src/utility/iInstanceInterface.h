/****************************** Instance Interface *****************************
This file is part of the PDI stack.

This is free software. You can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
Created Date    : 1st Jan 2024
******************************************************************************/

#ifndef _I_INSTANCE_INTERFACE_H_
#define _I_INSTANCE_INTERFACE_H_

#include "DataTypeDef.h"
#include "iUtilityInterface.h"

// Forward declaration of interfaces
class iMutex;
class iClientInterface;
class iTcpServerInterface;
class iTcpClientInterface;
class iTlsServerInterface;
class iTlsClientInterface;
class iUdpInterface;
class iFileSystemInterface;

// forward declaration of derived class for this interface
class InstanceInterface;


/**
 * @class iInstanceInterface
 * @brief Interface to get instance of required interfaces
 */
class iInstanceInterface
{

public:
  /**
   * @brief Destructor for the iInstanceInterface class.
   *
   * Ensures proper cleanup of resources when the interface is destroyed.
   */
  virtual ~iInstanceInterface() {}

  virtual iUtilityInterface& getUtilityInstance() = 0;          // get utility instance

  #ifdef ENABLE_NETWORK_SERVICE
  virtual iTcpServerInterface* getNewTcpServerInstance() { return nullptr; }      // get new TCP server instance
  virtual iTcpClientInterface* getNewTcpClientInstance() { return nullptr; }      // get new TCP client instance
  virtual iUdpInterface* getNewUdpInstance() { return nullptr; }                  // get new UDP socket instance

  /**
   * The one outbound client the services share, built on first call and kept, so
   * a service can source it instead of being handed it.
   */
  iTcpClientInterface* getSharedTcpClientInstance();

  /**
   * Releases the shared client, for the stack shutting down; a service that stops
   * leaves it for the services still running.
   */
  void releaseSharedTcpClientInstance();
  #endif

  #ifdef ENABLE_TLS_SERVICE
  virtual iTlsServerInterface* getNewTlsServerInstance() { return nullptr; }      // get new TLS server instance
  virtual iTlsClientInterface* getNewTlsClientInstance() { return nullptr; }      // get new TLS client instance

  /**
   * The one outbound TLS client the services share, built on first call and kept,
   * so a service can source it instead of being handed it.
   */
  iTlsClientInterface* getSharedTlsClientInstance();

  /**
   * Releases the shared TLS client, for the stack shutting down; a service that
   * stops leaves it for the services still running.
   */
  void releaseSharedTlsClientInstance();
  #endif

  /**
   * The shared client that carries the named protocol, null where this build
   * has none of that kind.
   */
  iClientInterface* getSharedClientForScheme(protocol_scheme_t scheme);

  #ifdef ENABLE_CONTEXTUAL_EXECUTION
  /**
   * A mutex that holds against a preemptive task, null where the port has none.
   */
  virtual iMutex* getNewPreemptiveMutexInstance() { return nullptr; }
  #endif

  #ifdef ENABLE_STORAGE_SERVICE
  virtual iFileSystemInterface& getFileSystemInstance()  = 0;    // get new file system instance
  #endif

  protected:
  #ifdef ENABLE_NETWORK_SERVICE
    iTcpClientInterface *m_shared_tcp_client = nullptr;
  #endif
  #ifdef ENABLE_TLS_SERVICE
    iTlsClientInterface *m_shared_tls_client = nullptr;
  #endif
};

/// derived class must define this
extern InstanceInterface __i_instance;

#endif
