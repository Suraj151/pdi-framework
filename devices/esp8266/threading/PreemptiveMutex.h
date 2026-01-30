/**************************** Preemptive Mutex *******************************
This file is part of the PDI stack.

This is free software. You can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
Created Date    : 1st June 2025
******************************************************************************/

#ifndef __ESP866_PREEMPTIVE_MUTEX_H__
#define __ESP866_PREEMPTIVE_MUTEX_H__

#include "../esp8266.h"
#include <interface/pdi/threading/iMutex.h>
#include "Preemptive.h"

class PreemptiveMutex : public iMutex {
    bool m_locked = false;
    pdiutil::vector<Preemptive*> m_waiters; 
    Preemptive* m_owner = nullptr;
public:
    PreemptiveMutex();
    virtual ~PreemptiveMutex();
    void lock() override;
    void unlock() override;

    void critical_lock() override;
    void critical_unlock() override;    
};


#endif
