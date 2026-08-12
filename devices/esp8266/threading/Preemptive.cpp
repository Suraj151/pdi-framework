/********************************* Preemptive *********************************
This file is part of the PDI stack.

This is free software. You can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
Created Date    : 1st June 2025
******************************************************************************/

#include "Preemptive.h"
#include "../DeviceControlInterface.h"
#ifdef DEVICE_AVOID_SDK_STACK_CONTEXT_SWITCH
#include <cont.h>
#endif

static XtensaContext* __isr_ctx; 
static Preemptive __non_preemptive;
static volatile bool __non_preemptive_saved = false;
static volatile bool __preemptive_sched_active = true;
static volatile uint32_t __last_time_spent_in_isr = 0; // in microseconds
#ifdef DEVICE_AVOID_SDK_STACK_CONTEXT_SWITCH
// A task waiting in exit() owns no context the isr can recognise, so it asks
// for the switch directly. Held only while that spin runs, nothing else runs.
static volatile bool __switch_requested = false;
#endif

// Context switch period required to set in timer
// Real time tuning requires period < 1ms - ISR context switch period/cycles in microseconds
static uint32_t __timer_period = 1000; // in microseconds

/**
 * hardware timer ISR coroutine get called from timer ISR handler with the interrupted context captured
 */
void IRAM_ATTR __attribute__((naked)) timer1_isr_coroutine(XtensaContext* ctx){

    // CRITICAL_SECTION_ENTER

    // Tune the period at runtime
    timer1_update_us(__timer_period - __last_time_spent_in_isr);
    if(!__preemptive_sched_active) return;

    // Capture microseconds
    uint32_t entry_us = micros();

#ifdef DEVICE_AVOID_SDK_STACK_CONTEXT_SWITCH
    // Leave the sdk stack alone, it carries lwIP and other non reentrant
    // callbacks. The next tick catches the loop once control returns to it.
    bool switch_requested = __switch_requested;
    __switch_requested = false;
    if (!switch_requested && !__i_preemptive_scheduler.is_switchable_context(ctx->sp)) return;
#endif

    // Keep copy of interrupted context
    __isr_ctx = ctx;

    // Capture the main loop once
    if (!__non_preemptive_saved) {
        __non_preemptive_saved = true;
        __non_preemptive.ctx = *__isr_ctx;
        CRITICAL_SECTION_ENTER
        __i_preemptive_scheduler.add_to_ready(&__non_preemptive);
        CRITICAL_SECTION_EXIT
    }

    // Switch tasks
    __i_preemptive_scheduler.run();

    // Calculate spent time in ISR in microseconds.
    // These are not approximates as we are not considering the rest time spent outside measured boundaries
    // Make sure that spent time in ISR should be less than period
    __last_time_spent_in_isr = micros() - entry_us;
    if( __last_time_spent_in_isr > __timer_period ) __last_time_spent_in_isr = 0;

    // CRITICAL_SECTION_EXIT
}

/**
 * Start entry function here
 */
void preemptive_trampoline(void *arg) {

    Preemptive* f = static_cast<Preemptive*>(arg);

    if (f){

        f->entry(f->arg);

        // Only mark finished if entry actually returns
        CRITICAL_SECTION_ENTER
        if (f->state == PreemptiveState::Running) {
            f->state = PreemptiveState::Finished;
        }
        CRITICAL_SECTION_EXIT
    }

    PreemptiveScheduler::exit(); // never returns
}

/**
 * Preemptive destructor.
 */
Preemptive::~Preemptive(){
    pdiutil::safe_delete_array(stack_raw);
    stack = nullptr;
}

/**
 * PreemptiveScheduler constructor.
 */
PreemptiveScheduler::PreemptiveScheduler(){
    // Pre-reserve so push_back() never reallocates at runtime.
    ready.reserve(MAX_SCHEDULABLE_TASKS + 2);
    sleepers.reserve(MAX_SCHEDULABLE_TASKS + 2);
}

/**
 * PreemptiveScheduler destructor.
 */
PreemptiveScheduler::~PreemptiveScheduler(){

    for (auto p : sleepers) { pdiutil::safe_delete(p.f); }
    for (auto p : ready) { pdiutil::safe_delete(p); }
    sleepers.clear();
    ready.clear();
}

/**
 * Schedule the task.
 */
int PreemptiveScheduler::schedule_task(task_t* task, uint32_t stacksize){

    // Init new Preemptive
    // if(task->m_task_exec) { delete task->m_task_exec; }
    if(task->m_task_mode != TASK_MODE_PREEMPTIVE) return TASK_ERROR_INVALID_MODE;
    Preemptive* f = pdiutil::safe_new<Preemptive>();
    if (!f) return PDI_ERR_NO_MEM;
    task->m_task_exec = f;

    // Allocate with padding to guarantee a 16-byte aligned top-of-stack
    const uint32_t raw_bytes = stacksize + 64;
    pdiutil::safe_delete_array(f->stack_raw);
    f->stack_raw = pdiutil::safe_new_array<uint8_t>(raw_bytes);
    if (!f->stack_raw) {
        pdiutil::safe_delete(f);
        task->m_task_exec = nullptr;
        return TASK_ERROR_CREATION_FAILED;
    }
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
            if(t && t->m_task) t->m_task();
        }
    };
    f->arg = static_cast<void*>(f);
    f->task_id = task->m_task_id;

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
    CRITICAL_SECTION_ENTER
    add_to_ready(f);
    CRITICAL_SECTION_EXIT

    // Start timer
    if(!preemptiveisr_active){
        preemptiveisr_active = true;
        timer1_start_us(__timer_period);
    }

    return 0;
}

/**
 * mute the current task.
 */
void PreemptiveScheduler::mute(){

    // the spin in exit() forces a switch off whatever stack it runs on. from
    // an sdk stack that abandons a half finished sdk callback, so refuse it.
    if (!is_scheduler_context()) return;

    CRITICAL_SECTION_ENTER

    Preemptive* f = current;
    if (!f) {
        CRITICAL_SECTION_EXIT
        return;
    }


    if (f->state == PreemptiveState::Running) {
        f->state = PreemptiveState::Mute;
    }

    // xtensa_save_context(&f->ctx);
    // if(f->state == PreemptiveState::Mute)
        PreemptiveScheduler::exit();
    
    CRITICAL_SECTION_EXIT
}

/**
 * yield the current task.
 */
void PreemptiveScheduler::yield(){

    if (!is_scheduler_context()) return;

    CRITICAL_SECTION_ENTER

    Preemptive* f = current;
    if (!f) {
        CRITICAL_SECTION_EXIT
        return;
    }

    if (f->state == PreemptiveState::Running) {
        add_to_ready(f);
    }

    // xtensa_save_context(&f->ctx);
    // if(f->state == PreemptiveState::Ready)
        PreemptiveScheduler::exit();
    
    CRITICAL_SECTION_EXIT
}

/**
 * sleep current task.
 */
void PreemptiveScheduler::sleep(uint32_t ms){

    if (!is_scheduler_context()) return;

    CRITICAL_SECTION_ENTER

    Preemptive* f = current;
    if (!f) {
        CRITICAL_SECTION_EXIT
        return;
    }

    f->state = PreemptiveState::Sleeping;
    sleepers.push_back({ __i_dvc_ctrl.millis_now() + ms, current }); 

    // xtensa_save_context(&f->ctx);
    // if(f->state == PreemptiveState::Sleeping)
        PreemptiveScheduler::exit();

    CRITICAL_SECTION_EXIT
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

            CRITICAL_SECTION_ENTER
            add_to_ready(si.f); 
            sleepers[i] = sleepers.back();
            sleepers.pop_back();
            CRITICAL_SECTION_EXIT
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

            CRITICAL_SECTION_ENTER
            add_to_ready(current);
            CRITICAL_SECTION_EXIT
        }else if(current->state == PreemptiveState::Ready){ // yielded task

            if( ready.empty() ){
                current->state = PreemptiveState::Running;
                return; // continue same task
            }

        }else if(current->state == PreemptiveState::Finished){ // tearing down

            return; // no branch below re-queues it, it would be lost

        // }else if(current->state == PreemptiveState::Sleeping){

        // }else if(current->state == PreemptiveState::Mute){
        }

        // Save the last interrupted context and remove task from current pointer
        current->ctx = *__isr_ctx;
        // current = nullptr;
    }

    // If any task ready then switch to it
    if(!ready.empty()){

        Preemptive* next = pick_next_ready();
        if (!next) return;

        CRITICAL_SECTION_ENTER
        // Assign running ISR exc frame 
        next->ctx.excframe = __isr_ctx->excframe;
        next->state = PreemptiveState::Running;
        current = next;

        xtensa_restore_context_isr(&next->ctx);
        CRITICAL_SECTION_EXIT 

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
 * Enable scheduler
 * This will yield current task and start timer ticks if disabled earlier.
 */
void PreemptiveScheduler::enable_sched(){

    if (!current) return;

    CRITICAL_SECTION_ENTER
    __preemptive_sched_active = true;
    
    yield();
}

/**
 * Disable scheduler
 * This will stop timer ticks if it is running
 */
void PreemptiveScheduler::disable_sched(){

    if (!current) return;
    
    CRITICAL_SECTION_ENTER
    __preemptive_sched_active = false;
    CRITICAL_SECTION_EXIT
}

/**
 * Exit.
 */
void PreemptiveScheduler::exit(){

    Preemptive* f = __i_preemptive_scheduler.current;
    if (f && f->state == PreemptiveState::Finished){

        __i_preemptive_scheduler.destroy_preemptive(f);
        
        __i_preemptive_scheduler.current = nullptr; 
        f = nullptr;
    }

    // Fast path: voluntary yield where we're the only ready entry.
    // No other task to switch to → just resume ourselves, no ISR
    // round-trip, no spin.
    // if (f && f->state == PreemptiveState::Ready
    //       && __i_preemptive_scheduler.ready.size() == 1 && __i_preemptive_scheduler.ready[0] == f) {
    //     CRITICAL_SECTION_ENTER
    //     __i_preemptive_scheduler.ready.pop_back();
    //     f->state = PreemptiveState::Running;
    //     CRITICAL_SECTION_EXIT
    //     return;
    // }

    CRITICAL_SECTION_ENTER
    // __i_preemptive_scheduler.current = nullptr; // can we remove pointer in ISR ?
    timer1_update_us(1);
    CRITICAL_SECTION_EXIT

    while(1) // wait for context switch by ISR
    {
        #ifdef DEVICE_AVOID_SDK_STACK_CONTEXT_SWITCH
        __switch_requested = true;
        #endif
        if(f && f->state == PreemptiveState::Running)
            break;
    }

    #ifdef DEVICE_AVOID_SDK_STACK_CONTEXT_SWITCH
    __switch_requested = false;
    #endif
}

/**
 * Destroy Preemptive
 */
void PreemptiveScheduler::destroy_preemptive(Preemptive* f) {

    if (f){

        CRITICAL_SECTION_ENTER
        remove_from_sleepers(f);
        remove_from_ready(f);

        if (current == f) current = nullptr;

        __task_scheduler.remove_task(f->task_id);
        pdiutil::safe_delete(f);
        CRITICAL_SECTION_EXIT
    }
}

/**
 * Add Preemptive to ready queue
 */
void PreemptiveScheduler::add_to_ready(Preemptive* f) {
    if(f){

        // Idempotency guard — never push a task that's already in ready.
        for (auto* r : ready) {
            if (r == f) return;
        }

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
    int32_t bestScore = -1;
    Preemptive* nonpreemptive = &__non_preemptive;

    for (uint16_t i = 0; i < ready.size(); ++i) {

        Preemptive* f = ready[i];
        int32_t p = 0; // default priority
        task_t* t = __task_scheduler.get_task(f->task_id);

        if(t) p = t->m_task_priority;
        if (!t && f != nonpreemptive) {
            // todo: think on this dangling pointer ?
            continue;
        }

        // Aging: score(effective priority) = base_priority + wait_ticks
        int32_t score = p + f->wait_ticks;

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
        CRITICAL_SECTION_ENTER
        remove_from_ready(best);
        CRITICAL_SECTION_EXIT
    }

    return best;
}

/**
 * Returns true if the current CPU execution context is actually a preemptive
 * task (or the tracked main-loop wrapper). Returns false when called from an
 * SDK / lwIP / NMI callback that happens to run on top of whichever task we
 * last context-switched to.
 *
 * Why this matters: PreemptiveMutex::lock() / mute() use `current` as the
 * task to park. When called from an SDK callback, `current` is NOT the code
 * actually running — it's whichever preemptive task we last switched to. If
 * we park `current` in that scenario, exit()'s spin waits for a state
 * transition that never matches the executing code → deadlock → soft WDT.
 *
 * Detection method: read the live stack pointer and check whether it falls
 * inside any preemptive task's allocated stack range. SDK / lwIP / cont
 * callbacks run on their own (different) stacks, so SP will be outside.
 * __non_preemptive has no allocated stack range of its own — it shares the
 * main-loop cont stack — so we accept it as a task context only when the
 * scheduler hasn't yet been preempted (i.e. ISR has not yet captured
 * __non_preemptive's context). Practically: if `current == &__non_preemptive`
 * and the caller is on the cont stack, we accept it. We can't perfectly
 * distinguish "main loop on cont stack" from "SDK callback on cont stack" —
 * but SDK callbacks reach our code via lwIP / event paths that don't share
 * SP range with main loop's `loop()`, so SP comparison is a reasonable proxy.
 */
bool PreemptiveScheduler::is_task_context() const {
    uint32_t sp_now;
    asm volatile("mov %0, a1" : "=a"(sp_now));

    // If running on a preemptive task's allocated stack range, we're a task.
    if (current && current->stack_raw && current->stack) {
        uintptr_t lo = reinterpret_cast<uintptr_t>(current->stack_raw);
        uintptr_t hi = reinterpret_cast<uintptr_t>(current->stack); // top
        if (sp_now >= lo && sp_now < hi) return true;
    }
    // __non_preemptive has no `stack_raw` of its own — it lives on the
    // cont/main-loop stack, which is not a preemptive task. Callers that only
    // need "may this context be parked" want is_scheduler_context() instead.
    return false;
}

/**
 * Tell whether the running stack is one the scheduler owns, either a task
 * stack or the main loop cont stack. Anything else is the sdk, which must
 * never be parked because the spin in exit() would abandon its callback.
 */
bool PreemptiveScheduler::is_scheduler_context() const {

    if (is_task_context()) return true;

#ifdef DEVICE_AVOID_SDK_STACK_CONTEXT_SWITCH
    uint32_t sp_now;
    asm volatile("mov %0, a1" : "=a"(sp_now));

    cont_t *loopctx = g_pcont;
    if (loopctx) {
        uintptr_t lo = reinterpret_cast<uintptr_t>(loopctx->stack);
        uintptr_t hi = reinterpret_cast<uintptr_t>(loopctx->stack_end);
        if (sp_now >= lo && sp_now < hi) return true;
    }
#endif

    return false;
}

#ifdef DEVICE_AVOID_SDK_STACK_CONTEXT_SWITCH
/**
 * Tell whether the interrupted stack belongs to us. The loop owns the cont
 * stack and every task owns its allocated block, anything else is the sdk.
 */
bool PreemptiveScheduler::is_switchable_context(uint32_t sp) const {

    if (current && current->stack_raw && current->stack) {
        uintptr_t lo = reinterpret_cast<uintptr_t>(current->stack_raw);
        uintptr_t hi = reinterpret_cast<uintptr_t>(current->stack);
        if (sp >= lo && sp < hi) return true;
    }

    cont_t *loopctx = g_pcont;
    if (loopctx) {
        uintptr_t lo = reinterpret_cast<uintptr_t>(loopctx->stack);
        uintptr_t hi = reinterpret_cast<uintptr_t>(loopctx->stack_end);
        if (sp >= lo && sp < hi) return true;
    }

    return false;
}
#endif

void Preemptive::suspend(){
    if (state == PreemptiveState::Mute) return;
    CRITICAL_SECTION_ENTER
    __i_preemptive_scheduler.remove_from_ready(this);
    __i_preemptive_scheduler.remove_from_sleepers(this);
    state = PreemptiveState::Mute;
    CRITICAL_SECTION_EXIT
}

void Preemptive::resume(){
    if (state != PreemptiveState::Mute) return;
    CRITICAL_SECTION_ENTER
    __i_preemptive_scheduler.add_to_ready(this);
    CRITICAL_SECTION_EXIT
}

void Preemptive::terminate(){
    CRITICAL_SECTION_ENTER
    __i_preemptive_scheduler.remove_from_ready(this);
    __i_preemptive_scheduler.remove_from_sleepers(this);
    state = PreemptiveState::Finished;
    CRITICAL_SECTION_EXIT
}

bool Preemptive::is_finished(){
    return state == PreemptiveState::Finished;
}

void Preemptive::reap(){
    __i_preemptive_scheduler.destroy_preemptive(this);
}

PreemptiveScheduler __i_preemptive_scheduler;
