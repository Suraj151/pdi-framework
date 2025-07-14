/***************************** Instance Interface *****************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st Jan 2024
******************************************************************************/

#include "InstanceInterface.h"
#include "DeviceControlInterface.h"

/**
 * InstanceInterface constructor.
 */
InstanceInterface::InstanceInterface(){
}

/**
 * InstanceInterface destructor.
 */
InstanceInterface::~InstanceInterface(){
}

iUtilityInterface &InstanceInterface::getUtilityInstance()
{
    return __i_dvc_ctrl;
}

InstanceInterface __i_instance;
