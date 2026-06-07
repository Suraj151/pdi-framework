/********************************* Cooperative ********************************
This file is part of the PDI stack.

This is free software. You can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
Created Date    : 1st Jan 2026
******************************************************************************/

#ifndef __ESP32_COOPERATIVE_H__
#define __ESP32_COOPERATIVE_H__

#include "../esp32.h"
#include <interface/pdi/threading/cooperative/iCooperative.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

class Cooperative : public iCooperative {
public:
    virtual ~Cooperative();

    TaskHandle_t handle = nullptr;
    SemaphoreHandle_t resume_sem = nullptr;
};

class CooperativeScheduler : public iCooperativeScheduler {
    SemaphoreHandle_t sched_sem = nullptr;
    bool othersched_active = false;
public:
    CooperativeScheduler();
    virtual ~CooperativeScheduler();

    int schedule_task(task_t* task, uint32_t stacksize) override;
    void mute() override;
    void yield() override;
    void sleep(uint32_t ms) override;
    void run() override;
    void tick_from_loop() override { run(); }

    SemaphoreHandle_t get_sched_sem() const { return sched_sem; }

    Cooperative* current = nullptr;
    pdiutil::vector<Cooperative*> ready;
    struct SleepItem { uint32_t wake_ms; Cooperative* f = nullptr; };
    pdiutil::vector<SleepItem> sleepers;

    void destroy_cooperative(Cooperative* f) override;
    void add_to_ready(Cooperative* f) override;
    void remove_from_ready(Cooperative* f) override;
    void remove_from_sleepers(Cooperative* f) override;

    Cooperative* pick_next_ready() override;

    void yield_from_othersched() override;
    void sleep_from_othersched(uint32_t ms) override;
    bool can_yield_from_othersched() { return !othersched_active && !current; }
    bool can_sleep_from_othersched() { return !othersched_active && !current; }
};

#endif
