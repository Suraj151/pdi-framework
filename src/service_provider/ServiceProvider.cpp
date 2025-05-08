/***************************** service provider *******************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st June 2019
******************************************************************************/

#include "ServiceProvider.h"


// Static member variable to hold the service instances
ServiceProvider *ServiceProvider::m_services[SERVICE_MAX] = {nullptr};

// Static member variable to hold the terminal interface
iTerminalInterface *ServiceProvider::m_terminal = nullptr;
