/**************************** Preemptive Mutex *******************************
This file is part of the PDI stack.

This is free software. You can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
Created Date    : 1st Jan 2026
******************************************************************************/

#ifndef __ESP32_PREEMPTIVE_MUTEX_H__
#define __ESP32_PREEMPTIVE_MUTEX_H__

#include "../esp32.h"
#include <interface/pdi/threading/iMutex.h>
#include "Preemptive.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

class PreemptiveMutex : public iMutex {
    SemaphoreHandle_t m_mutex = nullptr;
public:
    PreemptiveMutex();
    virtual ~PreemptiveMutex();
    void lock() override;
    void unlock() override;

    void critical_lock() override;
    void critical_unlock() override;

    bool try_lock() override;
};

#endif
