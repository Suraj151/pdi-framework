/********************************* Cooperative ********************************
This file is part of the PDI stack.

This is free software. You can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
Created Date    : 1st June 2025
******************************************************************************/

#include "Cooperative.h"
#include "../DeviceControlInterface.h"

static XtensaContext __base_ctx; 
static bool __base_ctx_saved = false;

/**
 * Start entry function here
 */
void cooperative_trampoline(void *arg) {

    Cooperative* f = static_cast<Cooperative*>(arg);

    if (f){

        f->entry(f->arg);
        // Only mark finished if entry actually returns 
        if (f->state == CooperativeState::Running) { 
            f->state = CooperativeState::Finished; 
        }        
    }

    CooperativeScheduler::exit(); // never returns
}

/**
 * Cooperative destructor.
 */
Cooperative::~Cooperative(){
    if( stack_raw != nullptr ){
        delete[] stack_raw;
        stack_raw = nullptr;
    }
    stack = nullptr;
}

/**
 * CooperativeScheduler constructor.
 */
CooperativeScheduler::CooperativeScheduler(){
}

/**
 * CooperativeScheduler destructor.
 */
CooperativeScheduler::~CooperativeScheduler(){

    for (auto p : sleepers) { if(p.f) delete p.f; p.f = nullptr; }
    for (auto p : ready) { if(p) delete p; p = nullptr; }
    sleepers.clear();
    ready.clear();
}

/**
 * Schedule the task.
 */
void CooperativeScheduler::schedule_task(task_t* task, uint32_t stacksize){

    // Init new Cooperative
    // if(task->_task_exec) { delete task->_task_exec; }
    if(task->_task_mode != TASK_MODE_COOPERATIVE) return;
    Cooperative* f = new Cooperative;
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
    f->state      = CooperativeState::Ready;

    // Entry wrapper
    f->entry = [](void* arg){ 
        Cooperative* f = static_cast<Cooperative*>(arg); 
        if(f){
            // Serial.printf("[cooperative_entry_task] cooperative=%p\n", f);
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

    // a2 = argument (Cooperative*)
    f->ctx.a[2] = reinterpret_cast<uint32_t>(f);

    // a0 = return address → exit (if trampoline ever returns)
    f->ctx.a[0] = reinterpret_cast<uint32_t>(&CooperativeScheduler::exit);

    // PS (start simple; UM|EXCM is typical — if it traps, try PS_EXCM only)
    uint32_t ps_now; asm volatile("rsr.ps %0" : "=a"(ps_now));
    f->ctx.ps = ps_now;
    // f->ctx.ps   = PS_UM | PS_EXCM;

    // PC = trampoline
    f->ctx.pc   = reinterpret_cast<uint32_t>(&cooperative_trampoline);

    // Clear the rest for hygiene (optional)
    for (int i = 3; i <= 15; ++i) f->ctx.a[i] = 0;

    // Add to ready list    
    noInterrupts();
    add_to_ready(f);
    interrupts();
}

/**
 * mute the current task.
 */
void CooperativeScheduler::mute(){

    Cooperative* f = current;
    if (!f) return;

    noInterrupts(); 

    if (f->state == CooperativeState::Running) {
        f->state = CooperativeState::Mute;
    }

    xtensa_save_context(&f->ctx);
    if(f->state == CooperativeState::Mute)
        CooperativeScheduler::exit();

    interrupts();
}

/**
 * yield the current task.
 */
void CooperativeScheduler::yield(){

    Cooperative* f = current;
    if (!f) return;

    noInterrupts(); 

    if (f->state == CooperativeState::Running) {
        add_to_ready(f);
    }

    xtensa_save_context(&f->ctx);
    if(f->state == CooperativeState::Ready)
        CooperativeScheduler::exit();

    interrupts();
}

/**
 * sleep current task.
 */
void CooperativeScheduler::sleep(uint32_t ms){

    Cooperative* f = current;
    if (!f) return;

    noInterrupts(); 

    f->state = CooperativeState::Sleeping;
    sleepers.push_back({ __i_dvc_ctrl.millis_now() + ms, current }); 

    xtensa_save_context(&f->ctx);
    if(f->state == CooperativeState::Sleeping)
        CooperativeScheduler::exit();

    interrupts();
}

/**
 * Run the scheduled tasks.
 */
void CooperativeScheduler::run(){

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
        if (si.f && (int32_t)(now - si.wake_ms) >= 0) {

            noInterrupts(); 
            add_to_ready(si.f); 
            // erase by swap-pop for O(1)
            sleepers[i] = sleepers.back();
            sleepers.pop_back();
            interrupts();
        } else {
            ++i;
        }
    }

    // If nothing is running, start one
    if (__base_ctx_saved && !current && !ready.empty()) {

        Cooperative* next = pick_next_ready();
        if (!next) return;

        noInterrupts();
        current = next;
        next->state = CooperativeState::Running;
        xtensa_restore_context(&next->ctx); // never returns        
    }

    // __base_ctx_saved = false;
}

/**
 * Exit.
 */
void CooperativeScheduler::exit(){

    Cooperative* f = __i_cooperative_scheduler.current;
    if (f && f->state == CooperativeState::Finished){

        __i_cooperative_scheduler.destroy_cooperative(f);
    }

    noInterrupts();
    __i_cooperative_scheduler.current = nullptr;
    if(__base_ctx_saved){
        __base_ctx_saved = false;
        xtensa_restore_context(&__base_ctx); // jumps back main
    }
    interrupts();
}

/**
 * Destroy Cooperative
 */
void CooperativeScheduler::destroy_cooperative(Cooperative* f) {

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
 * Add Cooperative to ready queue
 */
void CooperativeScheduler::add_to_ready(Cooperative* f) {
    if(f){
        // f->wait_ticks = 0;
        f->state = PreemptiveState::Ready;
        ready.push_back(f);
    }
}

/**
 * Remove Cooperative from ready queue
 */
void CooperativeScheduler::remove_from_ready(Cooperative* f) {

    for (size_t i = 0; i < ready.size(); ++i) {
        if (ready[i] == f) { ready[i] = ready.back(); ready.pop_back(); break; }
    }
}

/**
 * Remove Cooperative from sleeper queue
 */
void CooperativeScheduler::remove_from_sleepers(Cooperative* f) {

    for (size_t i = 0; i < sleepers.size();) {
        if (sleepers[i].f == f) { sleepers[i] = sleepers.back(); sleepers.pop_back(); }
        else { ++i; }
    }
}

/**
 * Pick the next ready Cooperative
 */
Cooperative* CooperativeScheduler::pick_next_ready() {

    Cooperative* best = nullptr;
    int bestScore = -1;

    for (int i = 0; i < ready.size(); ++i) {

        Cooperative* f = ready[i];

        task_t* t = __task_scheduler.get_task(f->task_id);
        if (!t) continue;

        // Aging: score(effective priority) = base_priority + wait_ticks
        int score = t->_task_priority + f->wait_ticks;

        if (score > bestScore) {
            bestScore = score;
            best = f;
        }

        // Increment wait_ticks for all. max boost 255
        if (f->wait_ticks < 255) f->wait_ticks++;
    }

    if (nullptr != best) {
        // Reset wait_ticks for the chosen Cooperative
        best->wait_ticks = 0;

        // Remove from ready list
        noInterrupts();
        remove_from_ready(best);
        interrupts();
    }

    return best;
}

/**
 * Yield other scheduler to manage pending work in this scheduler
 */
void CooperativeScheduler::yield_from_othersched() {

    if (!can_yield_from_othersched()) return; // reject if already active
    othersched_active = true;

    // capture caller context as base to switch back and return control back to caller
    __base_ctx_saved = false;
    run();
    // safer side if exit of this scheduler missed
    __base_ctx_saved = false;

    othersched_active = false;
}

/**
 * Sleep other scheduler to manage pending work in this scheduler
 */
void CooperativeScheduler::sleep_from_othersched(uint32_t ms) {

    if (!can_sleep_from_othersched()) return; // reject if already active
    othersched_active = true;

    uint32_t exitms = __i_dvc_ctrl.millis_now() + ms;

    while ((int32_t)(__i_dvc_ctrl.millis_now() - exitms) < 0)
    {
        // capture caller context as base to switch back and return control back to caller
        __base_ctx_saved = false;
        run();
        // yield device level functionalities
        __i_dvc_ctrl.yield();
    }
    
    // safer side if exit of this scheduler missed
    __base_ctx_saved = false;

    othersched_active = false;
}

CooperativeScheduler __i_cooperative_scheduler;
