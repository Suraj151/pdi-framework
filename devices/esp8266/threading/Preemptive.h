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

    int schedule_task(task_t* task, uint32_t stacksize) override;
    void mute() override;
    void yield() override;
    void sleep(uint32_t ms) override;
    void IRAM_ATTR run() override;

    void enable_sched() override;
    void disable_sched() override;

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

    // Returns true if we're currently executing on a preemptive task's own
    // stack (i.e. on `current`'s stack range, or on the main-loop / cont
    // stack tracked as __non_preemptive). Returns false in SDK / lwIP / ISR
    // callback contexts where the running code is not the task pointed to
    // by `current` — in those contexts, calling mute() would park an
    // unrelated task and the spin in exit() would deadlock.
    bool is_task_context() const;
    bool is_sched_active() const {
        return current != nullptr && preemptiveisr_active;
    }
};

#endif
