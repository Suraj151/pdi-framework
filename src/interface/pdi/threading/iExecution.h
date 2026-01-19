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

// Executive state
enum class ExecState { Ready, Running, Sleeping, Finished };

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

    uint8_t* stack = nullptr;       // allocated stack memory
    uint32_t stack_size;
    int task_id = -1; // link back to corresponding task_t
    void (*entry)(void*) = nullptr; // entry function 
    void* arg = nullptr; // argument    
};

/**
 * Soft IRQ's works on the behalf of Hardware IRQ's. It helps to keep minimal impl routine in ISR.
 * They get scheduled within ISR and executed within main context as soon as return from ISR.
 */

// SoftIRQ bitmask definitions
enum SoftIrq : uint32_t {
    SIRQ_NONE = 0,
    SIRQ_SCHED = 1u << 0,   // schedule
    SIRQ_WAKE  = 1u << 1,   // optional: process wake events
};

// Global softirq state (volatile because ISR writes it)
extern volatile uint32_t __softirq_bits;

inline void raise_softirq(SoftIrq bit) {
    __softirq_bits |= bit;  // atomic at word level on Xtensa for single writer (ISR)
}
// must wrap within interrupt calls
inline uint32_t fetch_softirq_bits() {
    // Atomically grab and clear
    uint32_t bits = __softirq_bits;
    __softirq_bits = 0;
    return bits;
}

// Generic execution scheduler interface
class iExecutionScheduler {
public:
    virtual ~iExecutionScheduler() {}

    virtual void schedule_task(task_t* task, uint32_t stacksize) {}
    virtual void yield() = 0;
    virtual void sleep(uint32_t ms) = 0;
    virtual void run() = 0;

    // useful api for passing control within across schedulers
    virtual void yield_from_othersched() {}
    virtual void sleep_from_othersched(uint32_t ms) {}
    virtual bool can_yield_from_othersched() { return false; }
    virtual bool can_sleep_from_othersched() { return false; }    
};

// forward declaration of derived schedulers from iExecutionScheduler
class CooperativeScheduler;
class PreemptiveScheduler;


// globals (for simplicity)
extern CooperativeScheduler __i_cooperative_scheduler;
extern PreemptiveScheduler __i_preemptive_scheduler;
#endif