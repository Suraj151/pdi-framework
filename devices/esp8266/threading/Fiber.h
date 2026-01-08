/********************************* Fiber **************************************
This file is part of the PDI stack.

This is free software. You can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
Created Date    : 1st June 2025
******************************************************************************/

#ifndef __ESP866_FIBER_H__
#define __ESP866_FIBER_H__

#include "../esp8266.h"
#include <interface/pdi/threading/fiber/iFiber.h>
#include "FiberContext.h"

// Fiber
class Fiber : public iFiber {
public:
    virtual ~Fiber();

    FiberContext ctx;
};

// Cooperative fiber scheduler
class FiberScheduler : public iFiberScheduler {
    bool othersched_active = false; // true if another scheduler has yielded/slept in
public:
    virtual ~FiberScheduler();

    void schedule_task(task_t* task, uint32_t stacksize) override;
    void yield() override;
    void sleep(uint32_t ms) override;
    void run() override;

    static IRAM_ATTR void exit();

    Fiber* current = nullptr;
    pdiutil::vector<Fiber*> ready;
    struct SleepItem { uint32_t wake_ms; Fiber* f = nullptr; };
    pdiutil::vector<SleepItem> sleepers; // optional timing wheel for delays

    void destroy_fiber(Fiber* f) override;
    void add_to_ready(Fiber* f) override;
    void remove_from_ready(Fiber* f) override;
    void remove_from_sleepers(Fiber* f) override;

    Fiber* pick_next_ready() override;

    void yield_from_othersched() override;
    void sleep_from_othersched(uint32_t ms) override;
    bool can_yield_from_othersched() { return !othersched_active && !current; }
    bool can_sleep_from_othersched() { return !othersched_active && !current; }    
};

// Derive it as required by interface
class ExecutionScheduler : public FiberScheduler{
public:
    virtual ~ExecutionScheduler(){}
};

#endif
