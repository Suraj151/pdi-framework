/*************************** Preemptive Mutex ********************************
This file is part of the PDI stack.

This is free software. You can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
Created Date    : 1st Jan 2026
******************************************************************************/

#include "PreemptiveMutex.h"

PreemptiveMutex::PreemptiveMutex() {
    m_mutex = xSemaphoreCreateRecursiveMutex();
}

PreemptiveMutex::~PreemptiveMutex() {
    if (m_mutex) {
        vSemaphoreDelete(m_mutex);
        m_mutex = nullptr;
    }
}

void PreemptiveMutex::lock(){
    if (!m_mutex) return;
    xSemaphoreTakeRecursive(m_mutex, portMAX_DELAY);
}

void PreemptiveMutex::unlock(){
    if (!m_mutex) return;
    xSemaphoreGiveRecursive(m_mutex);
}

void PreemptiveMutex::critical_lock(){
    lock();
    portDISABLE_INTERRUPTS();
}

void PreemptiveMutex::critical_unlock(){
    portENABLE_INTERRUPTS();
    unlock();
}

bool PreemptiveMutex::try_lock(){
    if (!m_mutex) return false;
    return xSemaphoreTakeRecursive(m_mutex, 0) == pdTRUE;
}
