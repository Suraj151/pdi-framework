/********************************* Preemptive *********************************
This file is part of the PDI stack.

This is free software. You can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
Created Date    : 1st June 2025
******************************************************************************/

#ifndef __ESP866_PREEMPTIVE_H__
#define __ESP866_PREEMPTIVE_H__

#include "../esp8266.h"
#include <interface/pdi/threading/preemptive/iPreemptive.h>
#include "XtensaTimer.h"

// Preemptive
class Preemptive : public iPreemptive {
public:
    virtual ~Preemptive();

    XtensaContext ctx;
};

// Preemptive(thread) scheduler
class PreemptiveScheduler : public iPreemptiveScheduler {
    bool preemptiveisr_active = false;
public:
    PreemptiveScheduler();
    virtual ~PreemptiveScheduler();

    void schedule_task(task_t* task, uint32_t stacksize) override;
    void mute() override;
    void yield() override;
    void sleep(uint32_t ms) override;
    void run() override;

    static IRAM_ATTR void exit();

    Preemptive* current = nullptr;
    pdiutil::vector<Preemptive*> ready;
    struct SleepItem { uint32_t wake_ms; Preemptive* f = nullptr; };
    pdiutil::vector<SleepItem> sleepers; // optional timing wheel for delays

    void destroy_preemptive(Preemptive* f) override;
    void add_to_ready(Preemptive* f) override;
    void remove_from_ready(Preemptive* f) override;
    void remove_from_sleepers(Preemptive* f) override;

    Preemptive* pick_next_ready() override;
};

#endif
