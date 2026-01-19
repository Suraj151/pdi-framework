/*************************** Cooperative Condvar ******************************
This file is part of the PDI stack.

This is free software. You can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
Created Date    : 1st June 2025
******************************************************************************/

#include "CooperativeCondvar.h"

CooperativeConditionVar::wait(iMutex& mtx){

    m_waiters.push_back(__i_cooperative_scheduler.current);
    mtx.lock();
    __i_cooperative_scheduler.yield();
    mtx.unlock();
}

CooperativeConditionVar::notify_one(){

    if (!m_waiters.empty()) {
        Cooperative* f = m_waiters.back();     // LIFO; use back for O(1)
        m_waiters.pop_back();
        noInterrupts();
        __i_cooperative_scheduler.add_to_ready(f);
        interrupts();
    }    
}
