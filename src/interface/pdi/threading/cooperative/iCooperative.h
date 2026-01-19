/*************************** Cooperative Interface ****************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st Jan 2024
******************************************************************************/

#ifndef _I_THREADING_COOPERATIVE_INTERFACE_H_
#define _I_THREADING_COOPERATIVE_INTERFACE_H_

#include <interface/interface_includes.h>
#include "../iExecution.h"

// Cooperative status
using CooperativeState = ExecState;

// Cooperative interface
class iCooperative : public iExecutive{
public:
    virtual ~iCooperative() {}

    uint8_t* stack_raw = nullptr;   // raw block for deletion
    CooperativeState state = CooperativeState::Ready;
    // int effective_priority = 0; // base_priority + aging boost
    int wait_ticks = 0; // counts how many times skipped
};

// forward declaration
class Cooperative;

// Cooperative Sheduler interface
class iCooperativeScheduler : public iExecutionScheduler {
public:
    virtual ~iCooperativeScheduler() {}

    virtual void destroy_cooperative(Cooperative* f) {}
    virtual void add_to_ready(Cooperative* f) {}
    virtual void remove_from_ready(Cooperative* f) {}
    virtual void remove_from_sleepers(Cooperative* f) {}

    virtual Cooperative* pick_next_ready() { return nullptr; }
};

#endif