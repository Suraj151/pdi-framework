/*************************** Preemptive Interface *****************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st Jan 2024
******************************************************************************/

#ifndef _I_THREADING_PREEMPTIVE_INTERFACE_H_
#define _I_THREADING_PREEMPTIVE_INTERFACE_H_

#include <interface/interface_includes.h>
#include "../iExecution.h"

// Preemptive status
using PreemptiveState = ExecState;

// Preemptive interface
class iPreemptive : public iExecutive{
public:
    virtual ~iPreemptive() {}

    uint8_t* stack_raw = nullptr;   // raw block for deletion
    PreemptiveState state = PreemptiveState::Ready;
    // int effective_priority = 0; // base_priority + aging boost
    int wait_ticks = 0; // counts how many times skipped
};

// forward declaration
class Preemptive;

// Preemptive Scheduler interface
class iPreemptiveScheduler : public iExecutionScheduler {
public:
    virtual ~iPreemptiveScheduler() {}

    virtual void destroy_preemptive(Preemptive* f) {}
    virtual void add_to_ready(Preemptive* f) {}
    virtual void remove_from_ready(Preemptive* f) {}
    virtual void remove_from_sleepers(Preemptive* f) {}

    virtual Preemptive* pick_next_ready() { return nullptr; }
};

#endif