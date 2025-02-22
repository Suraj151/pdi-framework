/***************************** Client Interface ******************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st Jan 2024
******************************************************************************/

#ifndef _I_CLIENT_INTERFACE_H_
#define _I_CLIENT_INTERFACE_H_

#include <utility/iIOInterface.h>

/**
 * iClientInterface class
 */
class iClientInterface : public iIOInterface
{

public:
  /**
   * iClientInterface constructor.
   */
  iClientInterface() {}
  /**
   * iClientInterface destructor.
   */
  virtual ~iClientInterface() {}

  // useful api
  virtual void setTimeout(uint32_t timeout) = 0;
  virtual iClientInterface* getNewInstance() = 0;
  static void releaseInstance( iClientInterface **instance )
  {
    if( nullptr != *instance )
    {
      delete *instance;
      *instance = nullptr;
    }
  }
};

#endif
