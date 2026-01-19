/**************************** Cooperative Mutex *******************************
This file is part of the PDI stack.

This is free software. You can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
Created Date    : 1st June 2025
******************************************************************************/

#ifndef __ESP866_COOPERATIVE_MUTEX_H__
#define __ESP866_COOPERATIVE_MUTEX_H__

#include "../esp8266.h"
#include <interface/pdi/threading/iMutex.h>
#include "Cooperative.h"

class CooperativeMutex : public iMutex {
    bool m_locked = false;
    pdiutil::vector<Cooperative*> m_waiters; 
public:
    virtual ~CooperativeMutex() { m_waiters.clear(); }
    void lock() override;
    void unlock() override;
};


#endif
