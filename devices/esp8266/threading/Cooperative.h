/********************************* Cooperative ********************************
This file is part of the PDI stack.

This is free software. You can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
Created Date    : 1st June 2025
******************************************************************************/

#ifndef __ESP866_COOPERATIVE_H__
#define __ESP866_COOPERATIVE_H__

#include "../esp8266.h"
#include <interface/pdi/threading/cooperative/iCooperative.h>
#include "XtensaContext.h"

// Cooperative
class Cooperative : public iCooperative {
public:
    virtual ~Cooperative();

    XtensaContext ctx;
};

// Cooperative(fiber) scheduler
class CooperativeScheduler : public iCooperativeScheduler {
    bool othersched_active = false; // true if another scheduler has yielded/slept in
public:
    CooperativeScheduler();
    virtual ~CooperativeScheduler();

    void schedule_task(task_t* task, uint32_t stacksize) override;
    void yield() override;
    void sleep(uint32_t ms) override;
    void run() override;

    static IRAM_ATTR void exit();

    Cooperative* current = nullptr;
    pdiutil::vector<Cooperative*> ready;
    struct SleepItem { uint32_t wake_ms; Cooperative* f = nullptr; };
    pdiutil::vector<SleepItem> sleepers; // optional timing wheel for delays

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
