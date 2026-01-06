/**************************** Execution Interface *****************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st Jan 2024
******************************************************************************/

#ifndef _I_THREADING_EXECUTION_INTERFACE_H_
#define _I_THREADING_EXECUTION_INTERFACE_H_

#include <interface/interface_includes.h>

// Generic execution context interface
class iExecutionContext {
public:
    virtual ~iExecutionContext() {}
    virtual void start() = 0;
    virtual void suspend() = 0;
    virtual void resume() = 0;
};

// Generic executive interface
class iExecutive {
public:
    virtual ~iExecutive() {}
};

// forward declaration
class ExecutionScheduler;

// Generic execution scheduler interface
class iExecutionScheduler {
public:
    virtual ~iExecutionScheduler() {}

    virtual void schedule_task(task_t* task, uint32_t stacksize) {}
    virtual void yield() = 0;
    virtual void sleep(uint32_t ms) = 0;
    virtual void run() = 0;
};

// globals (for simplicity)
extern ExecutionScheduler __i_exec_scheduler;

#endif