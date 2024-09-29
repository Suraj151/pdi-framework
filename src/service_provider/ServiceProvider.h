/***************************** service provider *******************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st June 2019
******************************************************************************/

#ifndef _SERVICE_PROVIDER_H_
#define _SERVICE_PROVIDER_H_


#include <interface/pdi.h>
#include <config/Config.h>
#include <utility/Utility.h>

/**
 * ServiceProvider class
 */
class ServiceProvider{

  public:

    /**
     * ServiceProvider constructor.
     */
    ServiceProvider(){
    }

    /**
		 * ServiceProvider destructor
		 */
    ~ServiceProvider(){
    }

    /**
     * init service
     */
    virtual bool initService(){
      return true;
    }
};

#endif
