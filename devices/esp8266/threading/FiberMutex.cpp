/***************************** Fiber Muitex ***********************************
This file is part of the PDI stack.

This is free software. You can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
Created Date    : 1st June 2025
******************************************************************************/

#include "FiberMutex.h"

FiberMutex::lock(){

    if (!m_locked) { m_locked = true; return; }
    m_waiters.push_back(__i_exec_scheduler.current); 
    __i_exec_scheduler.yield(); // park this fiber    
}

FiberMutex::unlock(){

    if (!m_waiters.empty()) {
        Fiber* f = m_waiters.back();     // LIFO; use back for O(1)
        m_waiters.pop_back();
        noInterrupts(); 
        __i_exec_scheduler.ready.push_back(f);
        interrupts();
    } else {
        m_locked = false;
    }    
}
