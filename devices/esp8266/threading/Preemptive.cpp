/********************************* Preemptive *********************************
This file is part of the PDI stack.

This is free software. You can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
Created Date    : 1st June 2025
******************************************************************************/

#include "Preemptive.h"
#include "../DeviceControlInterface.h"

static XtensaContext* __isr_ctx; 
static Preemptive __non_preemptive;
static volatile bool __non_preemptive_saved = false;

// Context switch period required to set in timer
// Real time tuning requires period < 1ms - ISR context switch period/cycles in microseconds
static uint32_t __timer_period = 1000; // in microseconds

/**
 * hardware timer ISR coroutine get called from timer ISR handler with the interrupted context captured
 */
void IRAM_ATTR __attribute__((naked)) timer1_isr_coroutine(XtensaContext* ctx){

    // noInterrupts();

    // Capture microseconds
    uint32_t current_us = micros();

    // Keep copy of interrupted context
    __isr_ctx = ctx;

    // Capture the main loop once
    if (!__non_preemptive_saved) {
        __non_preemptive_saved = true;
        __non_preemptive.ctx = *__isr_ctx;
        __i_preemptive_scheduler.add_to_ready(&__non_preemptive);
    }

    // Switch tasks
    __i_preemptive_scheduler.run();

    // Calculate spent time in ISR in microseconds.
    // These are not approximates as we are not considering the rest time spent outside measured boundaries
    // Make sure that spent time in ISR should be less than period
    uint32_t spent_time_in_us = micros() - current_us;
    if( spent_time_in_us > __timer_period ) spent_time_in_us = 0;

    // Tune the period at runtime
    timer1_update_us(__timer_period - spent_time_in_us);

    // interrupts();
}

/**
 * Start entry function here
 */
void preemptive_trampoline(void *arg) {

    Preemptive* f = static_cast<Preemptive*>(arg);

    if (f){

        f->entry(f->arg);
        // Only mark finished if entry actually returns 
        if (f->state == PreemptiveState::Running) { 
            f->state = PreemptiveState::Finished; 
        }        
    }

    PreemptiveScheduler::exit(); // never returns
}

/**
 * Preemptive destructor.
 */
Preemptive::~Preemptive(){
    if( stack_raw != nullptr ){
        delete[] stack_raw;
        stack_raw = nullptr;
    }
    stack = nullptr;
}

/**
 * PreemptiveScheduler constructor.
 */
PreemptiveScheduler::PreemptiveScheduler(){
}

/**
 * PreemptiveScheduler destructor.
 */
PreemptiveScheduler::~PreemptiveScheduler(){

    for (auto p : sleepers) { if(p.f) delete p.f; p.f = nullptr; }
    for (auto p : ready) { if(p) delete p; p = nullptr; }
    sleepers.clear();
    ready.clear();
}

/**
 * Schedule the task.
 */
void PreemptiveScheduler::schedule_task(task_t* task, uint32_t stacksize){

    // Init new Preemptive
    // if(task->_task_exec) { delete task->_task_exec; }
    if(task->_task_mode != TASK_MODE_PREEMPTIVE) return;
    Preemptive* f = new Preemptive;
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
    f->state      = PreemptiveState::Ready;

    // Entry wrapper
    f->entry = [](void* arg){ 
        Preemptive* f = static_cast<Preemptive*>(arg); 
        if(f){
            // Serial.printf("[preemptive_entry_task] preemptive=%p\n", f);
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

    // a2 = argument (Preemptive*)
    f->ctx.a[2] = reinterpret_cast<uint32_t>(f);

    // a0 = return address → exit (if trampoline ever returns)
    f->ctx.a[0] = reinterpret_cast<uint32_t>(&PreemptiveScheduler::exit);

    // PS (start simple; UM|EXCM is typical — if it traps, try PS_EXCM only)
    uint32_t ps_now; asm volatile("rsr.ps %0" : "=a"(ps_now));
    f->ctx.ps   = ps_now;//(ps_now & ~(0x0000000F));
    // f->ctx.ps   = PS_UM | PS_EXCM;

    // PC = trampoline
    f->ctx.pc   = reinterpret_cast<uint32_t>(&preemptive_trampoline);

    // Clear the rest for hygiene (optional)
    for (int i = 3; i <= 15; ++i) f->ctx.a[i] = 0;

    // Default no exception frame
    f->ctx.excframe = 0;

    // Add to ready list    
    noInterrupts();
    add_to_ready(f);
    interrupts();

    // Start timer
    if(!preemptiveisr_active){
        preemptiveisr_active = true;
        timer1_start_us(__timer_period);
    }
}

/**
 * mute the current task.
 */
void PreemptiveScheduler::mute(){

    Preemptive* f = current;
    if (!f) return;

    noInterrupts(); 

    if (f->state == PreemptiveState::Running) {
        f->state = PreemptiveState::Mute;
    }

    // xtensa_save_context(&f->ctx);
    // if(f->state == PreemptiveState::Mute)
        PreemptiveScheduler::exit();
    
    interrupts();
}

/**
 * yield the current task.
 */
void PreemptiveScheduler::yield(){

    Preemptive* f = current;
    if (!f) return;

    noInterrupts(); 

    if (f->state == PreemptiveState::Running) {
        add_to_ready(f);
    }

    // xtensa_save_context(&f->ctx);
    // if(f->state == PreemptiveState::Ready)
        PreemptiveScheduler::exit();
    
    interrupts();
}

/**
 * sleep current task.
 */
void PreemptiveScheduler::sleep(uint32_t ms){

    Preemptive* f = current;
    if (!f) return;

    noInterrupts(); 

    f->state = PreemptiveState::Sleeping;
    sleepers.push_back({ __i_dvc_ctrl.millis_now() + ms, current }); 

    // xtensa_save_context(&f->ctx);
    // if(f->state == PreemptiveState::Sleeping)
        PreemptiveScheduler::exit();

    interrupts();
}

/**
 * Run the scheduled tasks from ISR context.
 * Caution : this function get called from ISR. Dont process much here
 */
void PreemptiveScheduler::run(){

    // Wake sleepers whose time has arrived
    // todo : try to avoid using millis inside ISR. need to find alternate
    uint32_t now = __i_dvc_ctrl.millis_now();
    
    for (size_t i = 0; i < sleepers.size();) {

        auto si = sleepers[i];
        if (si.f && (int32_t)(now - si.wake_ms) >= 0) {

            add_to_ready(si.f); 
            sleepers[i] = sleepers.back();
            sleepers.pop_back();
        } else {
            ++i;
        }
    }

    // Already running in Preemptive Scheduler context. store the interrupted context
    if(current){

        if(current->state == PreemptiveState::Running){

            if( ready.empty() ){
                return; // continue same task
            }

            add_to_ready(current);
        }else if(current->state == PreemptiveState::Ready){ // yielded task

            if( ready.empty() ){
                current->state = PreemptiveState::Running;
                return; // continue same task
            }
            
        // }else if(current->state == PreemptiveState::Sleeping){

        // }else if(current->state == PreemptiveState::Mute){
        }

        // Save the last interrupted context and remove task from current pointer
        current->ctx = *__isr_ctx;
        current = nullptr;
    }

    // If any task ready then switch to it
    if(!ready.empty()){

        Preemptive* next = pick_next_ready();
        if (!next) return;

        // Assign running ISR exc frame 
        next->ctx.excframe = __isr_ctx->excframe;

        next->state = PreemptiveState::Running;
        current = next;

        xtensa_restore_context_isr(&next->ctx); 

        // Serial.printf("[restore_context] ps=%08x, pc=%08x, sp=%08x, sar=%08x, frame=%08x, a0=%08x, a1=%08x, a2=%08x, a3=%08x, a4=%08x, a5=%08x, a6=%08x, a7=%08x, a8=%08x, a9=%08x, a10=%08x, a11=%08x, a12=%08x, a13=%08x, a14=%08x, a15=%08x, t0=%08x, t1=%08x, t2=%08x, t3=%08x, t4=%08x, t5=%08x, t6=%08x, t7=%08x, t8=%08x, t9=%08x, t10=%08x, t11=%08x, t12=%08x, t13=%08x, t14=%08x, t15=%08x\n", 
        //     next->ctx.ps, next->ctx.pc, next->ctx.sp, next->ctx.sar, next->ctx.excframe,

        //     next->ctx.a[0], next->ctx.a[1], next->ctx.a[2], next->ctx.a[3], next->ctx.a[4], next->ctx.a[5],
        //     next->ctx.a[6], next->ctx.a[7], next->ctx.a[8], next->ctx.a[9], next->ctx.a[10], next->ctx.a[11],
        //     next->ctx.a[12], next->ctx.a[13], next->ctx.a[14], next->ctx.a[15],

        //     next->ctx.t[0], next->ctx.t[1], next->ctx.t[2], next->ctx.t[3], next->ctx.t[4], next->ctx.t[5],
        //     next->ctx.t[6], next->ctx.t[7], next->ctx.t[8], next->ctx.t[9], next->ctx.t[10], next->ctx.t[11],
        //     next->ctx.t[12], next->ctx.t[13], next->ctx.t[14], next->ctx.t[15]
        // );
    }
}

/**
 * Exit.
 */
void PreemptiveScheduler::exit(){

    Preemptive* f = __i_preemptive_scheduler.current;
    if (f && f->state == PreemptiveState::Finished){

        __i_preemptive_scheduler.destroy_preemptive(f);
    }

    noInterrupts();
    // __i_preemptive_scheduler.current = nullptr; // can we remove pointer in ISR ?
    timer1_update_us(1);
    interrupts();

    while(1) // wait for context switch by ISR
    {
        if(f && f->state == PreemptiveState::Running)
            break;
    }
}

/**
 * Destroy Preemptive
 */
void PreemptiveScheduler::destroy_preemptive(Preemptive* f) {

    if (f){

        noInterrupts(); 
        remove_from_sleepers(f);
        remove_from_ready(f);
    
        __task_scheduler.remove_task(f->task_id);
        delete f; 
        f = nullptr;
        interrupts();
    }
}

/**
 * Add Preemptive to ready queue
 */
void PreemptiveScheduler::add_to_ready(Preemptive* f) {
    if(f){
        // f->wait_ticks = 0;
        f->state = PreemptiveState::Ready;
        ready.push_back(f);
    }
}

/**
 * Remove Preemptive from ready queue
 */
void PreemptiveScheduler::remove_from_ready(Preemptive* f) {

    for (size_t i = 0; i < ready.size(); ++i) {
        if (ready[i] == f) { ready[i] = ready.back(); ready.pop_back(); break; }
    }
}

/**
 * Remove Preemptive from sleeper queue
 */
void PreemptiveScheduler::remove_from_sleepers(Preemptive* f) {

    for (size_t i = 0; i < sleepers.size();) {
        if (sleepers[i].f == f) { sleepers[i] = sleepers.back(); sleepers.pop_back(); }
        else { ++i; }
    }
}

/**
 * Pick the next ready Preemptive
 */
Preemptive* PreemptiveScheduler::pick_next_ready() {

    Preemptive* best = nullptr;
    int bestScore = -1;
    Preemptive* nonpreemptive = &__non_preemptive;

    for (int i = 0; i < ready.size(); ++i) {

        Preemptive* f = ready[i];
        int p = 0; // default priority
        task_t* t = __task_scheduler.get_task(f->task_id);

        if(t) p = t->_task_priority;
        if (!t && f != nonpreemptive) continue;

        // Aging: score(effective priority) = base_priority + wait_ticks
        int score = p + f->wait_ticks;

        if (score > bestScore) {
            bestScore = score;
            best = f;
        }

        // Increment wait_ticks for all. max boost 255
        if (f->wait_ticks < 255) f->wait_ticks++;
    }

    if (nullptr != best) {
        // Reset wait_ticks for the chosen Preemptive
        best->wait_ticks = 0;

        // Remove from ready list
        remove_from_ready(best);
    }

    return best;
}

PreemptiveScheduler __i_preemptive_scheduler;
