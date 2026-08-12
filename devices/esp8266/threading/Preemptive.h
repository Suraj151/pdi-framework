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

    void suspend() override;
    void resume() override;
    void terminate() override;
    bool is_finished() override;
    void reap() override;
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

    // Returns true if the running stack belongs to the scheduler, i.e. a task
    // stack or the main loop cont stack. False in SDK / lwIP / ISR callback
    // contexts, which must never be parked or switched away from.
    bool is_scheduler_context() const;
#ifdef DEVICE_AVOID_SDK_STACK_CONTEXT_SWITCH
    // Returns true if the interrupted context identified by `sp` may be
    // switched away from. Only the main loop stack and our own task stacks
    // qualify. The sdk carries lwIP and its callbacks on a separate stack,
    // switching away from that lets a task reenter code that is not reentrant.
    bool IRAM_ATTR is_switchable_context(uint32_t sp) const;
#endif
    bool is_sched_active() const {
        return current != nullptr && preemptiveisr_active;
    }
};

#endif
