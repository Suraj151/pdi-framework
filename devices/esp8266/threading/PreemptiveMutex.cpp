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
PreemptiveMutex::~PreemptiveMutex() {

    while (!m_waiters.empty()) {
        
        Preemptive* f = m_waiters.front();
        m_waiters.erase(m_waiters.begin());
        __i_preemptive_scheduler.add_to_ready(f);
    }
    m_locked = false;
    m_owner = nullptr;
}

/**
 * Lock this mutex or add the caller to waiting list in case already locked
 */
void PreemptiveMutex::lock(){


    if(!__i_preemptive_scheduler.current) return;

    noInterrupts();

    if (!m_locked) { 

        m_locked = true; 
        m_owner = __i_preemptive_scheduler.current;
        interrupts();
        return; 
    }

    // Avoid lock twice
    if (__i_preemptive_scheduler.current == m_owner) {

        interrupts(); 
        return; 
    }    

    m_waiters.push_back(__i_preemptive_scheduler.current); 
    interrupts();
    __i_preemptive_scheduler.mute(); // park this current preemptive        
}

/**
 * Unlock this mutex
 */
void PreemptiveMutex::unlock(){

    // if(!__i_preemptive_scheduler.current) return;

    // Only owner can unlock
    if (__i_preemptive_scheduler.current != m_owner) return;

    noInterrupts(); 
    
    if (!m_waiters.empty()) {
    
        Preemptive* f = m_waiters.front();     // FIFO;
        m_waiters.erase(m_waiters.begin());
        m_owner = f;
        __i_preemptive_scheduler.add_to_ready(f);
    } else {
    
        m_locked = false;
        m_owner = nullptr;
    }    
    
    interrupts();
}
