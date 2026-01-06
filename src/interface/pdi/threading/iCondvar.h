/*********************** Condition Variable Interface *************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st Jan 2024
******************************************************************************/

#ifndef _I_THREADING_CONDVAR_INTERFACE_H_
#define _I_THREADING_CONDVAR_INTERFACE_H_

#include <interface/interface_includes.h>
#include "iMutex.h"

class iConditionVar {
public:
    virtual ~iConditionVar() {}
    virtual void wait(iMutex& mtx) = 0;
    virtual void notify_one() = 0;
    virtual void notify_all() = 0;
};

#endif