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
class iTcpServerInterface;
class iTcpClientInterface;
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
  #endif

  #ifdef ENABLE_STORAGE_SERVICE
  virtual iFileSystemInterface& getFileSystemInstance()  = 0;    // get new file system instance
  #endif
};

/// derived class must define this
extern InstanceInterface __i_instance;

#endif
