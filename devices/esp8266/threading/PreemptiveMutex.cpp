/*************************** Preemptive Muitex ******************************
This file is part of the PDI stack.

This is free software. You can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
Created Date    : 1st June 2025
******************************************************************************/

#include "PreemptiveMutex.h"

/**
 * Constructor
 */
PreemptiveMutex::PreemptiveMutex(){
}

/**
 * Destructor
 */
PreemptiveMutex::~PreemptiveMutex(){

    // Shift every waiters to ready list
    while (!m_waiters.empty()){
        unlock();
    }
}

/**
 * Lock this mutex or add the caller to waiting list in case already locked
 */
void PreemptiveMutex::lock(){


    if(!__i_preemptive_scheduler.current) return;

    noInterrupts();

    if (!m_locked) { 

        m_locked = true; 
        interrupts();
        return; 
    }

    m_waiters.push_back(__i_preemptive_scheduler.current); 
    __i_preemptive_scheduler.mute(); // park this current preemptive    
    
    interrupts();
}

/**
 * Unlock this mutex
 */
void PreemptiveMutex::unlock(){

    if(!__i_preemptive_scheduler.current) return;

    noInterrupts(); 
    
    if (!m_waiters.empty()) {
    
        Preemptive* f = m_waiters.back();     // LIFO; use back for O(1)
        m_waiters.pop_back();
        __i_preemptive_scheduler.add_to_ready(f);
    } else {
    
        m_locked = false;
    }    
    
    interrupts();
}
