/********************************* Fiber **************************************
This file is part of the PDI stack.

This is free software. You can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
Created Date    : 1st June 2025
******************************************************************************/

#include "Fiber.h"
#include "../DeviceControlInterface.h"

static FiberContext __base_ctx; 
static bool __base_ctx_saved = false;

void fiber_trampoline(void *arg) {

    Fiber* f = static_cast<Fiber*>(arg);

    if (f){

        f->entry(f->arg);
        // Only mark finished if entry actually returns 
        if (f->state == FiberState::Running) { 
            f->state = FiberState::Finished; 
        }        
    }

    FiberScheduler::exit(); // never returns
}

/**
 * Fiber destructor.
 */
Fiber::~Fiber(){
    if( stack_raw != nullptr ){
        delete[] stack_raw;
        stack_raw = nullptr;
    }
    stack = nullptr;
}

/**
 * FiberScheduler destructor.
 */
FiberScheduler::~FiberScheduler(){

    for (auto p : sleepers) { if(p.f) delete p.f; p.f = nullptr; }
    for (auto p : ready) { if(p) delete p; p = nullptr; }
    sleepers.clear();
    ready.clear();
}

/**
 * Schedule the task.
 */
void FiberScheduler::schedule_task(task_t* task, uint32_t stacksize){

    // Init new fiber
    // if(task->_task_exec) { delete task->_task_exec; }
    Fiber* f = new Fiber;
    task->_task_exec = f;

    // Allocate with padding to guarantee a 16-byte aligned top-of-stack
    const uint32_t raw_bytes = stacksize + 64;
    if (f->stack_raw) { delete[] f->stack_raw; }
    f->stack_raw = new uint8_t[raw_bytes];
    memset(f->stack_raw, 0, raw_bytes);

    // 16-byte aligned top
    uintptr_t top = reinterpret_cast<uintptr_t>(f->stack_raw + raw_bytes);
    top &= ~static_cast<uintptr_t>(0xF);

    // Record stack and SP
    f->stack      = reinterpret_cast<uint8_t*>(top);
    f->stack_size = stacksize;
    f->state      = FiberState::Ready;

    // Entry wrapper
    f->entry = [](void* arg){ 
        Fiber* f = static_cast<Fiber*>(arg); 
        if(f){
            // Serial.printf("[fiber_entry_task] fiber=%p\n", f);
            task_t* t = __task_scheduler.get_task(f->task_id);
            if(t && t->_task) t->_task();
        }
    };
    f->arg = static_cast<void*>(f);
    f->task_id = task->_task_id;

    // Initialize context for first restore
    // a1 (SP), sp mirror
    f->ctx.a[1] = static_cast<uint32_t>(top - 16);
    f->ctx.sp   = static_cast<uint32_t>(top - 16);

    // a2 = argument (Fiber*)
    f->ctx.a[2] = reinterpret_cast<uint32_t>(f);

    // a0 = return address → exit (if trampoline ever returns)
    f->ctx.a[0] = reinterpret_cast<uint32_t>(&FiberScheduler::exit);

    // PS (start simple; UM|EXCM is typical — if it traps, try PS_EXCM only)
    uint32_t ps_now; asm volatile("rsr.ps %0" : "=a"(ps_now));
    f->ctx.ps = ps_now;
    // f->ctx.ps   = PS_UM | PS_EXCM;

    // PC = trampoline
    f->ctx.pc   = reinterpret_cast<uint32_t>(&fiber_trampoline);

    // Clear the rest for hygiene (optional)
    for (int i = 3; i <= 15; ++i) f->ctx.a[i] = 0;

    // Add to ready list
    noInterrupts();
    ready.push_back(f);
    interrupts();
}

/**
 * yield the current task.
 */
void FiberScheduler::yield(){

    Fiber* f = current;
    if (!f) return;

    noInterrupts(); 
    if (f->state == FiberState::Running) {
        f->state = FiberState::Ready;
        ready.push_back(f); 
    }
    interrupts();

    noInterrupts();
    xtensa_save_context(&f->ctx);
    interrupts();

    // current = nullptr;
    // run();
    if(f->state == FiberState::Ready)
        FiberScheduler::exit();
}

/**
 * sleep current task.
 */
void FiberScheduler::sleep(uint32_t ms){

    Fiber* f = current;
    if (!f) return;

    f->state = FiberState::Sleeping;
    noInterrupts(); 
    sleepers.push_back({ __i_dvc_ctrl.millis_now() + ms, current }); 
    interrupts();

    noInterrupts();
    xtensa_save_context(&f->ctx);
    interrupts();

    if(f->state == FiberState::Sleeping)
        FiberScheduler::exit();
}

/**
 * Run the scheduled tasks.
 */
void FiberScheduler::run(){

    if (!__base_ctx_saved) { 

        __base_ctx_saved = true; 
        // Save the main loop context once every time run called
        noInterrupts();
        xtensa_save_context(&__base_ctx); 
        interrupts();
    }    

    // Wake sleepers whose time has arrived
    uint32_t now = __i_dvc_ctrl.millis_now();
    for (size_t i = 0; i < sleepers.size();) {

        auto si = sleepers[i];
        if (si.wake_ms <= now && si.f) {

            si.f->state = FiberState::Ready;
            noInterrupts(); ready.push_back(si.f); interrupts();
            // erase by swap-pop for O(1)
            sleepers[i] = sleepers.back();
            sleepers.pop_back();
        } else {
            ++i;
        }
    }

    // If nothing is running, start one
    if (__base_ctx_saved && !current && !ready.empty()) {

        Fiber* next = nullptr;
        noInterrupts();
        next = ready.back(); ready.pop_back();
        interrupts();

        if (!next) return;
        current = next;
        next->state = FiberState::Running;

        noInterrupts();
        xtensa_restore_context(&next->ctx); // never returns        
    }

    // __base_ctx_saved = false;
}

/**
 * Exit.
 */
void FiberScheduler::exit(){

    // Pick next ready fiber (vector back for efficiency)
    // while (!__i_exec_scheduler.ready.empty()) {

    //     Fiber* nf = nullptr;
    //     noInterrupts();
    //     nf = __i_exec_scheduler.ready.back();
    //     __i_exec_scheduler.ready.pop_back();
    //     interrupts();
    //     if (!nf) continue;
        
    //     if (nf->state == FiberState::Ready) {

    //         __i_exec_scheduler.current = nf;
    //         nf->state = FiberState::Running;

    //         noInterrupts();
    //         xtensa_restore_context(&nf->ctx); // never returns
    //     }
    // }

    Fiber* f = __i_exec_scheduler.current;
    if (f && f->state == FiberState::Finished){

        __i_exec_scheduler.destroy_fiber(f);
    }
    __i_exec_scheduler.current = nullptr;

    if(__base_ctx_saved){
        __base_ctx_saved = false;
        noInterrupts();    
        xtensa_restore_context(&__base_ctx); // jumps back main
    }
}

/**
 * Destroy fiber
 */
void FiberScheduler::destroy_fiber(Fiber* f) {

    if (f){

        noInterrupts(); 
        remove_from_sleepers(f);
        remove_from_ready(f);
        interrupts();
    
        __task_scheduler.remove_task(f->task_id);
        delete f; 
        f = nullptr;
    }
}

/**
 * Remove fiber from ready queue
 */
void FiberScheduler::remove_from_ready(Fiber* f) {

    for (size_t i = 0; i < ready.size(); ++i) {
        if (ready[i] == f) { ready[i] = ready.back(); ready.pop_back(); break; }
    }
}

/**
 * Remove fiber from sleeper queue
 */
void FiberScheduler::remove_from_sleepers(Fiber* f) {

    for (size_t i = 0; i < sleepers.size();) {
        if (sleepers[i].f == f) { sleepers[i] = sleepers.back(); sleepers.pop_back(); }
        else { ++i; }
    }
}

ExecutionScheduler __i_exec_scheduler;
