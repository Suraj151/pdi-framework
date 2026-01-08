/******************************* Fiber Interface ******************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st Jan 2024
******************************************************************************/

#ifndef _I_THREADING_FIBER_INTERFACE_H_
#define _I_THREADING_FIBER_INTERFACE_H_

#include <interface/interface_includes.h>
#include "../iExecution.h"

// Fiber status
enum class FiberState { Ready, Running, Sleeping, Finished };

// Fiber interface
class iFiber : public iExecutive{
public:
    virtual ~iFiber() {}

    uint8_t* stack = nullptr;       // allocated stack memory
    uint8_t* stack_raw = nullptr;   // raw block for deletion
    uint32_t stack_size;
    void (*entry)(void*) = nullptr; // entry function 
    void* arg = nullptr; // argument    
    FiberState state = FiberState::Ready;
    int task_id = -1; // link back to corresponding task_t
    // int effective_priority = 0; // base_priority + aging boost
    int wait_ticks = 0; // counts how many times skipped
};

// forward declaration
class Fiber;

// Fiber Sheduler interface
class iFiberScheduler : public iExecutionScheduler {
public:
    virtual ~iFiberScheduler() {}

    virtual void destroy_fiber(Fiber* f) {}
    virtual void add_to_ready(Fiber* f) {}
    virtual void remove_from_ready(Fiber* f) {}
    virtual void remove_from_sleepers(Fiber* f) {}

    virtual Fiber* pick_next_ready() { return nullptr; }
};

#endif