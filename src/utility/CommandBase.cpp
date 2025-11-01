/****************************** Command Base **********************************
This file is part of the PDI stack.

This is free software. You can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
Created Date    : 1st June 2019
******************************************************************************/

#include "CommandBase.h"

// Initialize static members of DatabaseTableAbstractLayer
CommandExecutionInterface * CommandBase::m_cmdexecinterface = nullptr;
