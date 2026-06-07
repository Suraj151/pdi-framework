/********************************* Preemptive *********************************
This file is part of the PDI stack.

This is free software. You can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
Created Date    : 1st Jan 2026
******************************************************************************/

#include "Preemptive.h"
#include "../DeviceControlInterface.h"

static void preemptive_trampoline(void *arg) {

    Preemptive* f = static_cast<Preemptive*>(arg);

    if (f){
        f->entry(f->arg);
        if (f->state == PreemptiveState::Running) {
            f->state = PreemptiveState::Finished;
        }
    }

    vTaskDelete(NULL);
}

Preemptive::~Preemptive(){
    if (handle) {
        TaskHandle_t h = handle;
        handle = nullptr;
        vTaskDelete(h);
    }
}

PreemptiveScheduler::PreemptiveScheduler(){
    tasks.reserve(MAX_SCHEDULABLE_TASKS + 2);
}

PreemptiveScheduler::~PreemptiveScheduler(){

    for (auto p : tasks) { pdiutil::safe_delete(p); }
    tasks.clear();
}

int PreemptiveScheduler::schedule_task(task_t* task, uint32_t stacksize){

    if(task->_task_mode != TASK_MODE_PREEMPTIVE) return -1;
    Preemptive* f = pdiutil::safe_new<Preemptive>();
    if (!f) return -2;
    task->_task_exec = f;

    f->stack      = nullptr;
    f->stack_size = stacksize;
    f->state      = PreemptiveState::Ready;
    f->task_id    = task->_task_id;
    f->arg        = static_cast<void*>(f);
    f->entry      = [](void* arg){
        Preemptive* f = static_cast<Preemptive*>(arg);
        if(f){
            task_t* t = __task_scheduler.get_task(f->task_id);
            if(t && t->_task) t->_task();
        }
    };

    UBaseType_t prio = (UBaseType_t)task->_task_priority + tskIDLE_PRIORITY + 1;
    if (prio >= configMAX_PRIORITIES) prio = configMAX_PRIORITIES - 1;

    BaseType_t ok = xTaskCreatePinnedToCore(
        preemptive_trampoline,
        "pdi_preempt",
        stacksize / sizeof(StackType_t),
        f,
        prio,
        &f->handle,
        tskNO_AFFINITY
    );

    if (ok != pdPASS) {
        pdiutil::safe_delete(f);
        task->_task_exec = nullptr;
        return -3;
    }

    CRITICAL_SECTION_ENTER
    tasks.push_back(f);
    CRITICAL_SECTION_EXIT

    return 0;
}

Preemptive* PreemptiveScheduler::current_from_handle(TaskHandle_t h){

    if (!h) return nullptr;
    for (auto* f : tasks) {
        if (f && f->handle == h) return f;
    }
    return nullptr;
}

void PreemptiveScheduler::mute(){

    Preemptive* f = current_from_handle(xTaskGetCurrentTaskHandle());
    if (!f) return;

    if (f->state == PreemptiveState::Running) {
        f->state = PreemptiveState::Mute;
    }
    vTaskSuspend(NULL);
}

void PreemptiveScheduler::yield(){

    taskYIELD();
}

void PreemptiveScheduler::sleep(uint32_t ms){

    Preemptive* f = current_from_handle(xTaskGetCurrentTaskHandle());
    if (f) f->state = PreemptiveState::Sleeping;
    vTaskDelay(pdMS_TO_TICKS(ms));
    if (f) f->state = PreemptiveState::Running;
}

void PreemptiveScheduler::destroy_preemptive(Preemptive* f) {

    if (!f) return;

    CRITICAL_SECTION_ENTER
    for (size_t i = 0; i < tasks.size(); ++i) {
        if (tasks[i] == f) { tasks[i] = tasks.back(); tasks.pop_back(); break; }
    }
    __task_scheduler.remove_task(f->task_id);
    pdiutil::safe_delete(f);
    CRITICAL_SECTION_EXIT
}

PreemptiveScheduler __i_preemptive_scheduler;
