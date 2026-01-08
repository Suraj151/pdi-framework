/***************************** Fiber Condvar *********************************
This file is part of the PDI stack.

This is free software. You can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
Created Date    : 1st June 2025
******************************************************************************/

#include "FiberCondvar.h"

FiberConditionVar::wait(iMutex& mtx){

    m_waiters.push_back(__i_exec_scheduler.current);
    fiber_mutex_unlock(mtx);
    __i_exec_scheduler.yield();
    fiber_mutex_lock(mtx);
}

FiberConditionVar::notify_one(){

    if (!m_waiters.empty()) {
        Fiber* f = m_waiters.back();     // LIFO; use back for O(1)
        m_waiters.pop_back();
        noInterrupts();
        __i_exec_scheduler.add_to_ready(f);
        interrupts();
    }    
}
