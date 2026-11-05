/***************************** Instance Interface *****************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st Jan 2024
******************************************************************************/

#ifndef _ARDUINOMEGA_INSTANCE_INTERFACE_H_
#define _ARDUINOMEGA_INSTANCE_INTERFACE_H_

#include "arduinomega.h"
#include <interface/interface_includes.h>
/**
 * InstanceInterface class
 */
class InstanceInterface: public iInstanceInterface
{

public:
  /**
   * InstanceInterface constructor.
   */
  InstanceInterface();

  /**
   * InstanceInterface destructor.
   */
  ~InstanceInterface();

  iUtilityInterface& getUtilityInstance() override;
};

#endif
