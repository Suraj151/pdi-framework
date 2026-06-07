/********************************* Preemptive *********************************
This file is part of the PDI stack.

This is free software. You can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
Created Date    : 1st Jan 2026
******************************************************************************/

#ifndef __ESP32_PREEMPTIVE_H__
#define __ESP32_PREEMPTIVE_H__

#include "../esp32.h"
#include <interface/pdi/threading/preemptive/iPreemptive.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

class Preemptive : public iPreemptive {
public:
    virtual ~Preemptive();

    TaskHandle_t handle = nullptr;
};

class PreemptiveScheduler : public iPreemptiveScheduler {
public:
    PreemptiveScheduler();
    virtual ~PreemptiveScheduler();

    int schedule_task(task_t* task, uint32_t stacksize) override;
    void mute() override;
    void yield() override;
    void sleep(uint32_t ms) override;
    void run() override {}

    Preemptive* current_from_handle(TaskHandle_t h);

    pdiutil::vector<Preemptive*> tasks;

    void destroy_preemptive(Preemptive* f) override;
    void add_to_ready(Preemptive* f) override {}
    void remove_from_ready(Preemptive* f) override {}
    void remove_from_sleepers(Preemptive* f) override {}

    Preemptive* pick_next_ready() override { return nullptr; }
};

#endif
