/*************************** Cooperative Condvar ******************************
This file is part of the PDI stack.

This is free software. You can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
Created Date    : 1st Jan 2026
******************************************************************************/

#include "CooperativeCondvar.h"

void CooperativeConditionVar::wait(iMutex& mtx){

    m_waiters.push_back(__i_cooperative_scheduler.current);
    mtx.lock();
    __i_cooperative_scheduler.yield();
    mtx.unlock();
}

void CooperativeConditionVar::notify_one(){

    if (!m_waiters.empty()) {
        Cooperative* f = m_waiters.back();
        m_waiters.pop_back();
        CRITICAL_SECTION_ENTER
        __i_cooperative_scheduler.add_to_ready(f);
        CRITICAL_SECTION_EXIT
    }
}
