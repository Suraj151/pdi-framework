/******************************* Mutex Interface *****************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st Jan 2024
******************************************************************************/

#ifndef _I_THREADING_MUTEX_INTERFACE_H_
#define _I_THREADING_MUTEX_INTERFACE_H_

#include <interface/interface_includes.h>

class iMutex {
public:
    virtual ~iMutex() {}
    virtual void lock() = 0;
    virtual void unlock() = 0;

    virtual void critical_lock() = 0;
    virtual void critical_unlock() = 0;
};

#endif