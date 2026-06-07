/********************************* Cooperative ********************************
This file is part of the PDI stack.

This is free software. You can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
Created Date    : 1st Jan 2026
******************************************************************************/

#include "Cooperative.h"
#include "../DeviceControlInterface.h"

static void coop_trampoline(void *arg) {

    Cooperative* f = static_cast<Cooperative*>(arg);
    if (!f) { vTaskDelete(NULL); return; }

    xSemaphoreTake(f->resume_sem, portMAX_DELAY);

    if (f->state == CooperativeState::Running) {
        task_t* t = __task_scheduler.get_task(f->task_id);
        if (t && t->_task) t->_task();
    }

    f->state = CooperativeState::Finished;
    xSemaphoreGive(__i_cooperative_scheduler.get_sched_sem());

    for (;;) vTaskSuspend(NULL);
}

Cooperative::~Cooperative(){
    if (resume_sem) {
        vSemaphoreDelete(resume_sem);
        resume_sem = nullptr;
    }
    handle = nullptr;
}

CooperativeScheduler::CooperativeScheduler(){
    sched_sem = xSemaphoreCreateBinary();
    ready.reserve(MAX_SCHEDULABLE_TASKS + 2);
    sleepers.reserve(MAX_SCHEDULABLE_TASKS + 2);
}

CooperativeScheduler::~CooperativeScheduler(){

    for (auto p : sleepers) { destroy_cooperative(p.f); }
    for (auto p : ready) { destroy_cooperative(p); }
    sleepers.clear();
    ready.clear();
    if (sched_sem) {
        vSemaphoreDelete(sched_sem);
        sched_sem = nullptr;
    }
}

int CooperativeScheduler::schedule_task(task_t* task, uint32_t stacksize){

    if (task->_task_mode != TASK_MODE_COOPERATIVE) return -1;
    Cooperative* f = pdiutil::safe_new<Cooperative>();
    if (!f) return -2;
    task->_task_exec = f;

    f->resume_sem = xSemaphoreCreateBinary();
    if (!f->resume_sem) {
        pdiutil::safe_delete(f);
        task->_task_exec = nullptr;
        return -3;
    }

    f->stack      = nullptr;
    f->stack_size = stacksize;
    f->state      = CooperativeState::Ready;
    f->task_id    = task->_task_id;
    f->arg        = static_cast<void*>(f);
    f->entry      = [](void* arg){
        Cooperative* f = static_cast<Cooperative*>(arg);
        if (f) {
            task_t* t = __task_scheduler.get_task(f->task_id);
            if (t && t->_task) t->_task();
        }
    };

    BaseType_t ok = xTaskCreatePinnedToCore(
        coop_trampoline,
        "pdi_coop",
        stacksize / sizeof(StackType_t),
        f,
        tskIDLE_PRIORITY + 1,
        &f->handle,
        tskNO_AFFINITY
    );

    if (ok != pdPASS) {
        vSemaphoreDelete(f->resume_sem);
        pdiutil::safe_delete(f);
        task->_task_exec = nullptr;
        return -4;
    }

    CRITICAL_SECTION_ENTER
    add_to_ready(f);
    CRITICAL_SECTION_EXIT
    return 0;
}

void CooperativeScheduler::mute(){

    Cooperative* f = current;
    if (!f) return;
    if (xTaskGetCurrentTaskHandle() != f->handle) return;

    if (f->state == CooperativeState::Running) {
        f->state = CooperativeState::Mute;
    }
    xSemaphoreGive(sched_sem);
    xSemaphoreTake(f->resume_sem, portMAX_DELAY);
}

void CooperativeScheduler::yield(){

    Cooperative* f = current;
    if (!f) return;
    if (xTaskGetCurrentTaskHandle() != f->handle) return;

    if (f->state == CooperativeState::Running) {
        CRITICAL_SECTION_ENTER
        add_to_ready(f);
        CRITICAL_SECTION_EXIT
    }
    xSemaphoreGive(sched_sem);
    xSemaphoreTake(f->resume_sem, portMAX_DELAY);
}

void CooperativeScheduler::sleep(uint32_t ms){

    Cooperative* f = current;
    if (!f) return;
    if (xTaskGetCurrentTaskHandle() != f->handle) return;

    f->state = CooperativeState::Sleeping;
    CRITICAL_SECTION_ENTER
    sleepers.push_back({ (uint32_t)__i_dvc_ctrl.millis_now() + ms, f });
    CRITICAL_SECTION_EXIT

    xSemaphoreGive(sched_sem);
    xSemaphoreTake(f->resume_sem, portMAX_DELAY);
}

void CooperativeScheduler::run(){

    uint32_t now = (uint32_t)__i_dvc_ctrl.millis_now();
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

    if (current) return;
    if (ready.empty()) return;

    Cooperative* next = pick_next_ready();
    if (!next) return;

    current = next;
    next->state = CooperativeState::Running;

    xSemaphoreGive(next->resume_sem);
    xSemaphoreTake(sched_sem, portMAX_DELAY);

    Cooperative* finished = (current && current->state == CooperativeState::Finished) ? current : nullptr;
    current = nullptr;
    if (finished) destroy_cooperative(finished);
}

void CooperativeScheduler::destroy_cooperative(Cooperative* f) {

    if (!f) return;

    CRITICAL_SECTION_ENTER
    remove_from_sleepers(f);
    remove_from_ready(f);
    __task_scheduler.remove_task(f->task_id);
    if (current == f) current = nullptr;
    CRITICAL_SECTION_EXIT

    if (f->handle) {
        TaskHandle_t h = f->handle;
        f->handle = nullptr;
        vTaskDelete(h);
    }
    pdiutil::safe_delete(f);
}

void CooperativeScheduler::add_to_ready(Cooperative* f) {
    if (!f) return;
    for (auto* r : ready) {
        if (r == f) return;
    }
    f->state = CooperativeState::Ready;
    ready.push_back(f);
}

void CooperativeScheduler::remove_from_ready(Cooperative* f) {

    for (size_t i = 0; i < ready.size(); ++i) {
        if (ready[i] == f) { ready[i] = ready.back(); ready.pop_back(); break; }
    }
}

void CooperativeScheduler::remove_from_sleepers(Cooperative* f) {

    for (size_t i = 0; i < sleepers.size();) {
        if (sleepers[i].f == f) { sleepers[i] = sleepers.back(); sleepers.pop_back(); }
        else { ++i; }
    }
}

void CooperativeScheduler::yield_from_othersched() {

    if (!can_yield_from_othersched()) return;
    othersched_active = true;
    run();
    othersched_active = false;
}

void CooperativeScheduler::sleep_from_othersched(uint32_t ms) {

    if (!can_sleep_from_othersched()) return;
    othersched_active = true;

    uint32_t exitms = (uint32_t)__i_dvc_ctrl.millis_now() + ms;
    while ((int32_t)((uint32_t)__i_dvc_ctrl.millis_now() - exitms) < 0) {
        run();
        vTaskDelay(1);
    }

    othersched_active = false;
}

Cooperative* CooperativeScheduler::pick_next_ready() {

    Cooperative* best = nullptr;
    int bestScore = -1;

    for (int i = 0; i < ready.size(); ++i) {

        Cooperative* f = ready[i];
        task_t* t = __task_scheduler.get_task(f->task_id);
        if (!t) continue;

        int score = t->_task_priority + f->wait_ticks;
        if (score > bestScore) {
            bestScore = score;
            best = f;
        }
        if (f->wait_ticks < 255) f->wait_ticks++;
    }

    if (nullptr != best) {
        best->wait_ticks = 0;
        CRITICAL_SECTION_ENTER
        remove_from_ready(best);
        CRITICAL_SECTION_EXIT
    }
    return best;
}

CooperativeScheduler __i_cooperative_scheduler;
