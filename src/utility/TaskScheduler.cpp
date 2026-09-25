/**************************** TaskScheduler ***********************************
This file is part of the PDI stack.

This is free software. You can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
Created Date    : 1st June 2019
******************************************************************************/

#include <config/Common.h>

#include "TaskScheduler.h"
#include "DataTypeConversions.h"

/**
 * @brief Default constructor for the TaskScheduler class.
 *
 * Initializes the task scheduler with default values.
 */
TaskScheduler::TaskScheduler() : m_util(nullptr),
                                 m_max_tasks(MAX_SCHEDULABLE_TASKS),
                                 m_rebase_start_priotask(false),
                                 m_min_vruntime(0),
                                 m_next_task_id(1)
{
    // the table is sized once and never grown or compacted after this, so a
    // slot never moves and a running task keeps the callable it is executing.
    // a slot with m_task_id < 0 is free, which is what task_t() leaves behind
    this->m_tasks.reserve(MAX_SCHEDULABLE_TASKS);
    this->m_tasks.resize(MAX_SCHEDULABLE_TASKS);
}

/**
 * @brief Destructor for the TaskScheduler class.
 *
 * Cleans up resources used by the task scheduler.
 */
TaskScheduler::~TaskScheduler()
{
}

/**
 * @brief Parameterized constructor for the TaskScheduler class.
 *
 * Registers a task with the specified parameters.
 *
 * @param _task_fn The callback function to execute for the task.
 * @param _duration The interval or timeout duration for the task in milliseconds.
 * @param _task_priority The priority of the task.
 * @param _last_millis The last execution time of the task.
 */
TaskScheduler::TaskScheduler(CallBackVoidArgFn _task_fn, pdiutil::millis_t _duration, pdiutil::task_priority_t _task_priority, pdiutil::millis_t _last_millis) : TaskScheduler()
{
    this->register_task(_task_fn, _duration, _task_priority, _last_millis);
}

/**
 * @brief Sets a one-time timeout for a task.
 *
 * @param _task_fn The callback function to execute after the timeout.
 * @param _duration The timeout duration in milliseconds.
 * @param _now_millis The current time in milliseconds.
 * @param _task_priority The priority of the task.
 * @return The unique ID of the registered task.
 */
pdiutil::task_id_t TaskScheduler::setTimeout(CallBackVoidArgFn _task_fn, pdiutil::millis_t _duration, pdiutil::millis_t _now_millis, pdiutil::task_priority_t _task_priority, const char* _name, uint8_t _owner)
{
    return this->register_task(_task_fn, _duration, _task_priority, _now_millis, 1, _name, _owner);
}

/**
 * @brief Update a one-time timeout for a task.
 *
 * @param _task_id The unique ID of the task to update.
 * @param _task_fn The callback function to execute after the timeout.
 * @param _duration The timeout duration in milliseconds.
 * @param _now_millis The current time in milliseconds.
 * @param _task_priority The priority of the task (default is DEFAULT_TASK_PRIORITY).
 * @return The unique ID of the registered task.
 */
pdiutil::task_id_t TaskScheduler::updateTimeout(pdiutil::task_id_t _task_id, CallBackVoidArgFn _task_fn, pdiutil::millis_t _duration, pdiutil::millis_t _now_millis, pdiutil::task_priority_t _task_priority)
{
    return updateInterval(_task_id, _task_fn, _duration, _task_priority, _now_millis, 1);
}

/**
 * @brief Sets a recurring interval for a task.
 *
 * @param _task_fn The callback function to execute at each interval.
 * @param _duration The interval duration in milliseconds.
 * @param _now_millis The current time in milliseconds.
 * @param _task_priority The priority of the task.
 * @return The unique ID of the registered task.
 */
pdiutil::task_id_t TaskScheduler::setInterval(CallBackVoidArgFn _task_fn, pdiutil::millis_t _duration, pdiutil::millis_t _now_millis, pdiutil::task_priority_t _task_priority, const char* _name, uint8_t _owner)
{
    return this->register_task(_task_fn, _duration, _task_priority, _now_millis, -1, _name, _owner);
}

/**
 * @brief Updates the properties of an existing task.
 *
 * @param _task_id The unique ID of the task to update.
 * @param _task_fn The callback function for the task.
 * @param _duration The new interval duration in milliseconds.
 * @param _task_priority The priority of the task.
 * @param _last_millis The last execution time of the task.
 * @param _max_attempts The maximum number of attempts for the task.
 * @return The unique ID of the updated task.
 */
pdiutil::task_id_t TaskScheduler::updateInterval(pdiutil::task_id_t _task_id, CallBackVoidArgFn _task_fn, pdiutil::millis_t _duration, pdiutil::task_priority_t _task_priority, pdiutil::millis_t _last_millis, pdiutil::attempts_t _max_attempts, const char* _name, uint8_t _owner)
{
    int16_t _registered_index = this->is_registered_task(_task_id);
    if (_registered_index > -1)
    {
        CRITICAL_SECTION_ENTER
        this->m_tasks[_registered_index].m_task = _task_fn;
        this->m_tasks[_registered_index].m_duration = _duration;
        setTaskPriority(this->m_tasks[_registered_index], _task_priority);
        this->m_tasks[_registered_index].m_last_millis = _last_millis == 0 ? this->m_tasks[_registered_index].m_last_millis : _last_millis;
        this->m_tasks[_registered_index].m_max_attempts = _max_attempts;
        if (nullptr != _name) this->m_tasks[_registered_index].m_name = _name;
        this->m_tasks[_registered_index].m_owner = _owner;
        CRITICAL_SECTION_EXIT
        return _task_id;
    }
    else
    {
        return this->register_task(_task_fn, _duration, _task_priority, _last_millis, _max_attempts, _name, _owner);
    }
}

/**
 * @brief Clears a one-time timeout task.
 *
 * @param _id The unique ID of the task to clear.
 * @return True if the task was successfully cleared, false otherwise.
 */
bool TaskScheduler::clearTimeout(pdiutil::task_id_t _id)
{
    return this->remove_task(_id);
}

/**
 * @brief Clears a recurring interval task.
 *
 * @param _id The unique ID of the task to clear.
 * @return True if the task was successfully cleared, false otherwise.
 */
bool TaskScheduler::clearInterval(pdiutil::task_id_t _id)
{
    return this->remove_task(_id);
}

/**
 * @brief Registers a new task.
 *
 * @param _task_fn The callback function for the task.
 * @param _duration The interval or timeout duration for the task in milliseconds.
 * @param _task_priority The priority of the task.
 * @param _last_millis The last execution time of the task.
 * @param _max_attempts The maximum number of attempts for the task.
 * @return The unique ID of the registered task.
 */
pdiutil::task_id_t TaskScheduler::register_task(CallBackVoidArgFn _task_fn, pdiutil::millis_t _duration, pdiutil::task_priority_t _task_priority, pdiutil::millis_t _last_millis, pdiutil::attempts_t _max_attempts, const char* _name, uint8_t _owner)
{
    uint64_t _start_vruntime = this->m_min_vruntime;

    // claim a free slot in place, the table is never grown so slots never move
    uint16_t _slot = this->m_tasks.size();
    for (uint16_t i = 0; i < this->m_tasks.size() && i < this->m_max_tasks; i++)
    {
        if (this->m_tasks[i].m_task_id < 0)
        {
            _slot = i;
            break;
        }
    }

    if (_slot < this->m_tasks.size())
    {
        pdiutil::task_id_t _new_id = this->get_unique_task_id();
        if (_new_id < 0)
        {
            return -1;
        }

        CRITICAL_SECTION_ENTER
        task_t &_new_task = this->m_tasks[_slot];
        _new_task.m_task = _task_fn;
        _new_task.m_duration = _duration;
        _new_task.m_last_millis = _last_millis;
        _new_task.m_max_attempts = _max_attempts;
        _new_task.m_task_exec_us = 0;
        _new_task.m_vruntime = _start_vruntime;
        _new_task.m_task_id = _new_id;
        _new_task.m_state = TASK_STATE_SLEEPING;
        setTaskPolicy(_new_task, TASK_POLICY_FIFO);
        setTaskPriority(_new_task, _task_priority);
        _new_task.m_created_ms = (nullptr != m_util) ? m_util->millis_now() : 0;
        _new_task.m_name = _name;
        _new_task.m_owner = _owner;
        CRITICAL_SECTION_EXIT
        return _new_id;
    }
    return -1;
}

bool TaskScheduler::setTaskName(pdiutil::task_id_t _id, const char* _name)
{
    int16_t _idx = this->is_registered_task(_id);
    if (_idx < 0) return false;
    CRITICAL_SECTION_ENTER
    this->m_tasks[_idx].m_name = _name;
    CRITICAL_SECTION_EXIT
    return true;
}

bool TaskScheduler::setTaskOwner(pdiutil::task_id_t _id, uint8_t _owner)
{
    int16_t _idx = this->is_registered_task(_id);
    if (_idx < 0) return false;
    CRITICAL_SECTION_ENTER
    this->m_tasks[_idx].m_owner = _owner;
    CRITICAL_SECTION_EXIT
    return true;
}

/**
 * @brief Set the priority on a task record, keeping its weight in step.
 */
void TaskScheduler::setTaskPriority(task_t& _t, pdiutil::task_priority_t _priority)
{
    if (_priority > MAX_TASK_PRIORITY) _priority = MAX_TASK_PRIORITY;
    _t.m_task_priority = _priority;
    _t.m_weight = taskWeight(_t);
}

/**
 * @brief Set the policy on a task record, keeping its weight in step.
 */
void TaskScheduler::setTaskPolicy(task_t& _t, task_policy_t _policy)
{
    _t.m_task_policy = _policy;
    _t.m_weight = taskWeight(_t);
}

/**
 * @brief Set the nice value on a task record, keeping its weight in step.
 */
void TaskScheduler::setTaskNice(task_t& _t, int8_t _nice)
{
    _t.m_nice = _nice;
    _t.m_weight = taskWeight(_t);
}

bool TaskScheduler::setTaskNice(pdiutil::task_id_t _id, int8_t _nice)
{
    int16_t _idx = this->is_registered_task(_id);
    if (_idx < 0) return false;
    if (_nice < -20) _nice = -20;
    if (_nice > 19) _nice = 19;
    CRITICAL_SECTION_ENTER
    setTaskNice(this->m_tasks[_idx], _nice);
    CRITICAL_SECTION_EXIT
    return true;
}

/**
 * @brief Choose how a task is scored against the others, for one that is
 *        wanted on time rather than merely eventually.
 * @return true if the task was found and updated.
 */
bool TaskScheduler::setTaskPolicy(pdiutil::task_id_t _id, task_policy_t _policy)
{
    int16_t _idx = this->is_registered_task(_id);
    if (_idx < 0) return false;
    CRITICAL_SECTION_ENTER
    setTaskPolicy(this->m_tasks[_idx], _policy);
    CRITICAL_SECTION_EXIT
    return true;
}

/**
 * @brief Change a registered task's priority.
 * @return true if the task was found and updated.
 */
bool TaskScheduler::setTaskPriority(pdiutil::task_id_t _id, pdiutil::task_priority_t _priority)
{
    int16_t _idx = this->is_registered_task(_id);
    if (_idx < 0) return false;
    CRITICAL_SECTION_ENTER
    setTaskPriority(this->m_tasks[_idx], _priority);
    CRITICAL_SECTION_EXIT
    return true;
}

uint8_t TaskScheduler::getTaskOwner(pdiutil::task_id_t _id)
{
    int16_t _idx = this->is_registered_task(_id);
    if (_idx < 0) return 0;
    return this->m_tasks[_idx].m_owner;
}

bool TaskScheduler::sendSignal(pdiutil::task_id_t _id, signal_t _sig)
{
    int16_t _idx = this->is_registered_task(_id);
    if (_idx < 0) return false;
    CRITICAL_SECTION_ENTER
    this->m_tasks[_idx].m_pending_sig = (uint8_t)_sig;
    CRITICAL_SECTION_EXIT
    return true;
}

uint16_t TaskScheduler::sendSignalByName(const char* _name, signal_t _sig, uint8_t _requester_sid, bool _is_root)
{
    if (nullptr == _name || 0 == _name[0]) return 0;
    // m_name may live in read-only / PROGMEM flash; use strncmp_ro to compare
    // without dereferencing the RO pointer as if it were RAM. Compare span
    // covers the trailing NUL so unequal lengths also mismatch.
    size_t match_len = strlen(_name) + 1;
    uint16_t hits = 0;
    for (uint16_t i = 0; i < this->m_tasks.size(); i++)
    {
        task_t &t = this->m_tasks[i];
        if (t.m_task_id < 0) continue; // free slot
        if (t.m_state == TASK_STATE_ZOMBIE) continue;
        if (nullptr == t.m_name) continue;
        // strncmp_ro maps to strncmp_P on AVR/ESP where the SECOND arg is the
        // PROGMEM/flash string. m_name is the RO one; user-supplied _name is RAM.
        if (0 != strncmp_ro(_name, t.m_name, match_len)) continue;
        if (!_is_root && t.m_owner != _requester_sid) continue;
        if ((SIG_STOP == _sig || SIG_CONT == _sig) && !t.m_stoppable) continue;
        CRITICAL_SECTION_ENTER
        t.m_pending_sig = (uint8_t)_sig;
        CRITICAL_SECTION_EXIT
        hits++;
    }
    return hits;
}

/**
 * @brief The weight a task's service is divided by, so a heavier one is
 *        charged less for the same run and comes up again sooner.
 * @return the weight, never zero.
 */
uint32_t TaskScheduler::taskWeight(const task_t& _t)
{
    // equal slices among peers, so the ladder is not consulted at all
    if (TASK_POLICY_ROUNDROBIN == _t.m_task_policy)
    {
        return TASK_WEIGHT_NOMINAL;
    }

    int32_t effective = (int32_t)_t.m_task_priority - (int32_t)_t.m_nice;

    if (effective < 0) effective = 0;
    if (effective > MAX_TASK_PRIORITY) effective = MAX_TASK_PRIORITY;

    int32_t step = ((effective * (TASK_WEIGHT_LEVELS - 1)) / MAX_TASK_PRIORITY) - (TASK_WEIGHT_LEVELS / 2);

    uint64_t weight = TASK_WEIGHT_NOMINAL;

    for (int32_t i = 0; i < step; i++) weight = (weight * TASK_WEIGHT_STEP_NUM) / TASK_WEIGHT_STEP_DEN;
    for (int32_t i = 0; i > step; i--) weight = (weight * TASK_WEIGHT_STEP_DEN) / TASK_WEIGHT_STEP_NUM;

    if (TASK_POLICY_DEADLINE == _t.m_task_policy)
    {
        weight *= TASK_WEIGHT_DEADLINE_BOOST;
    }

    return (weight < 1) ? 1 : (uint32_t)weight;
}

/**
 * @brief Return the computed score for task.
 *
 */
int TaskScheduler::computeScore(const task_t& _t, uint64_t _now)
{
    uint64_t next_due = _t.m_last_millis + _t.m_duration;

    // --- Catch-up logic: overdue tasks get priority ---
    bool overdue = (_now >= next_due);

    // --- Compute lateness in ms (0 if not overdue) ---
    int64_t lateness = overdue ? (int64_t)(_now - next_due) : 0;

    // --- Compute earlyness in ms (0 if overdue) ---
    int64_t earlyness = overdue ? 0 : (int64_t)(next_due - _now);

    // --- Compute recentness in ms ---
    int64_t recentness = (int64_t)(_now - _t.m_last_millis);

    // Priority Weight
    int priority_weight = (int)_t.m_duration;

    if (priority_weight < TASK_PRIORITY_WEIGHT_MIN) priority_weight = TASK_PRIORITY_WEIGHT_MIN;
    if (priority_weight > TASK_PRIORITY_WEIGHT_MAX) priority_weight = TASK_PRIORITY_WEIGHT_MAX;

    // Policy Weight is policy_boost * policy_cap
    int policy_cap = 50;
    double policy_boost = 320.0;

    switch (_t.m_task_policy) {
        case TASK_POLICY_DEADLINE: { policy_boost = 2.0 * policy_boost; break; }
        case TASK_POLICY_FAIRSHARE: { policy_boost = 1.5 * policy_boost; break; }
        default: break;
    }

    // POSIX nice: lower nice = higher priority. Subtract from base priority
    // so nice=-20 lifts a task by 20 slots and nice=+19 drops it by 19.
    int effective_priority = (int)_t.m_task_priority - (int)_t.m_nice;

    // --- Blend policy, priority into a score ---
    switch (_t.m_task_policy) {
        case TASK_POLICY_ROUNDROBIN:
            // Equal slices: favor tasks that ran less recently
            return effective_priority * priority_weight + policy_boost * pdistd::min((int)floor(log2((double)recentness + 1.0)), policy_cap);
        case TASK_POLICY_DEADLINE:
            // Earliest deadline first: smaller next_due is higher urgency
            return effective_priority * priority_weight + policy_boost * pdistd::max(0, policy_cap - pdistd::min((int)floor(log2((double)earlyness + 1.0)), policy_cap));
        case TASK_POLICY_FAIRSHARE:
            // Favor tasks that consumed less CPU time
            return effective_priority * priority_weight + policy_boost * pdistd::max(0, policy_cap - pdistd::min((int)floor(log2((double)_t.m_task_exec_us + 1.0)), policy_cap));
        case TASK_POLICY_FIFO:
        default:
            // Priority dominates, lateness nudges overdue tasks
            return effective_priority * priority_weight + policy_boost * pdistd::min((int)lateness / 100, policy_cap);
    }
}

/**
 * @brief Sort the task indices according to their priority and score.
 *
 */
uint16_t TaskScheduler::getSortedTaskList(uint16_t* _priority_indices, uint16_t _task_count)
{
    const uint32_t tolerance = 3; // ms tolerance window
    uint64_t now = m_util->millis_now();

    // only live slots enter the list. a free slot has no due time, which scores
    // as maximally overdue and would sort above every real task
    uint16_t _live_count = 0;
    for (uint16_t i = 0; i < _task_count && i < this->m_tasks.size(); i++)
    {
        if (this->m_tasks[i].m_task_id >= 0)
        {
            _priority_indices[_live_count++] = i;
        }
    }
    _task_count = _live_count;

    for (uint16_t i = 0; i < _task_count; i++)
    {
        for (uint16_t j = i + 1; j < _task_count; j++)
        {
            auto &task_i = this->m_tasks[_priority_indices[i]];
            auto &task_j = this->m_tasks[_priority_indices[j]];

            uint64_t next_due_i = task_i.m_last_millis + task_i.m_duration;
            uint64_t next_due_j = task_j.m_last_millis + task_j.m_duration;

            bool swap_needed = false;

            // --- Due first, then least service taken for what the task weighs ---
            bool due_i = (now >= next_due_i);
            bool due_j = (now >= next_due_j);

            if (due_i != due_j) {

                swap_needed = due_j;
            } else if (task_i.m_vruntime != task_j.m_vruntime) {

                swap_needed = (task_j.m_vruntime < task_i.m_vruntime);
            } else {

                // --- Blend policy, priority into a score ---
                int score_i = this->computeScore(task_i, now);
                int score_j = this->computeScore(task_j, now);

                // --- Compare blended score first (priority-biased) ---
                if (score_i != score_j) {

                    swap_needed = (score_j > score_i);
                } else {
                    // --- Scores equal → compare due times with tolerance ---
                    int64_t diff = (int64_t)(next_due_i - next_due_j);

                    if (abs(diff) > (int64_t)tolerance) {
                        swap_needed = (next_due_j < next_due_i);
                    } else {
                        // --- Due times effectively equal → compare exec time ---
                        if (task_i.m_task_exec_us > task_j.m_task_exec_us) {
                            swap_needed = true;
                        }
                    }
                }
            }

            if (swap_needed) {
                pdistd::swap(_priority_indices[i], _priority_indices[j]);
            }
        }
    }

    return _task_count;
}

/**
 * @brief Executes all registered tasks that are due.
 *
 * This function must be called periodically (ideally every millisecond) to handle tasks.
 */
void TaskScheduler::handle_tasks()
{
    if (nullptr == m_util)
    {
        return;
    }

    uint16_t _priority_indices[MAX_SCHEDULABLE_TASKS];

    // free slots are left out of the list, so this is the live task count
    uint16_t _task_count = this->getSortedTaskList(_priority_indices, this->m_tasks.size());
    this->m_rebase_start_priotask = true; // run only one task and break for next turn

    for (uint16_t i = 0; i < _task_count; i++)
    {
        uint64_t _last_start_ms = m_util->millis_now();
        auto &_task = this->m_tasks[_priority_indices[i]];

        #ifdef ENABLE_CONTEXTUAL_EXECUTION
        if( _task.m_task_mode != TASK_MODE_INLINE ){

            if( nullptr != _task.m_task_exec ){

                iExecutive *_exec = _task.m_task_exec;

                if( _task.m_pending_sig != SIG_NONE ){
                    uint8_t sig = _task.m_pending_sig;
                    CRITICAL_SECTION_ENTER
                    _task.m_pending_sig = SIG_NONE;
                    CRITICAL_SECTION_EXIT
                    if( sig == SIG_STOP ){
                        if( _task.m_stoppable ){
                            _exec->suspend();
                            _task.m_state = TASK_STATE_STOPPED;
                        }
                    } else if( sig == SIG_CONT ){
                        if( _task.m_stoppable ){
                            _exec->resume();
                            if( _task.m_state == TASK_STATE_STOPPED ) _task.m_state = TASK_STATE_SLEEPING;
                        }
                    } else if( sig == SIG_KILL || sig == SIG_TERM ){
                        _exec->terminate();
                    }
                }

                if( _exec->is_finished() ){
                    _exec->reap();
                    if( _task.m_finalizer ){
                        CallBackVoidPointerArgFn _fin = _task.m_finalizer;
                        _task.m_finalizer = nullptr;
                        _fin(&_task);
                    }
                }
            }

            continue;
        }
        #endif

        // --- Consume pending signal (arrives from sendSignal/sendSignalByName)
        if (_task.m_pending_sig != SIG_NONE)
        {
            uint8_t sig = _task.m_pending_sig;
            CRITICAL_SECTION_ENTER
            _task.m_pending_sig = SIG_NONE;
            if (sig == SIG_KILL || sig == SIG_TERM) {
                _task.m_task = nullptr;
                _task.m_max_attempts = 0;
                _task.m_state = TASK_STATE_ZOMBIE;
            } else if (sig == SIG_STOP) {
                _task.m_state = TASK_STATE_STOPPED;
            } else if (sig == SIG_CONT) {
                if (_task.m_state == TASK_STATE_STOPPED) {
                    _task.m_state = TASK_STATE_SLEEPING;
                }
            }
            CRITICAL_SECTION_EXIT
            if (sig == SIG_KILL || sig == SIG_TERM) continue;
        }

        if (_task.m_state == TASK_STATE_STOPPED)
        {
            continue;
        }

        if (_last_start_ms < _task.m_last_millis)
        {
            CRITICAL_SECTION_ENTER
            _task.m_last_millis = _last_start_ms;
            CRITICAL_SECTION_EXIT
        }

        if (_task.m_max_attempts != 0 && (_last_start_ms - _task.m_last_millis) >= _task.m_duration)
        {
            if (nullptr != _task.m_task)
            {
                if (_task.m_vruntime > this->m_min_vruntime)
                {
                    this->m_min_vruntime = _task.m_vruntime;
                }

                CRITICAL_SECTION_ENTER
                _task.m_state = TASK_STATE_RUNNING;
                CRITICAL_SECTION_EXIT
                // User task callback — must NOT run with interrupts disabled.
                uint64_t _cb_start_us = m_util->micros_now();
                _task.m_task();
                uint64_t _cb_end_us = m_util->micros_now();
                CRITICAL_SECTION_ENTER
                _task.m_task_exec_us = (uint32_t)(_cb_end_us - _cb_start_us);
                _task.m_total_exec_us += (uint64_t)_task.m_task_exec_us;

                uint32_t _charge_weight = (_task.m_weight > 0) ? _task.m_weight : 1;
                _task.m_vruntime += ((uint64_t)_task.m_task_exec_us * (uint64_t)TASK_WEIGHT_NOMINAL)
                                    / (uint64_t)_charge_weight;
                _task.m_run_count++;
                _task.m_state = TASK_STATE_SLEEPING;
                CRITICAL_SECTION_EXIT
            }

            if( 0 == _task.m_last_millis ){

                CRITICAL_SECTION_ENTER
                _task.m_last_millis = _last_start_ms;
                CRITICAL_SECTION_EXIT
            }else{

                // --- Catch-up: advance last_millis by multiples of duration ---
                int catchupround = 0;
                CRITICAL_SECTION_ENTER
                while ((_last_start_ms - _task.m_last_millis) >= _task.m_duration) {
                    _task.m_last_millis += _task.m_duration; // Reduced drift
                    if (++catchupround > 3) break;  // break for max catchup rounds
                }
                CRITICAL_SECTION_EXIT
            }

            CRITICAL_SECTION_ENTER
            _task.m_max_attempts = _task.m_max_attempts > 0 ? _task.m_max_attempts - 1 : _task.m_max_attempts;
            if (_task.m_max_attempts == 0)
            {
                _task.m_state = TASK_STATE_ZOMBIE;
            }
            CRITICAL_SECTION_EXIT
        }

        if (nullptr != m_util)
        {
            m_util->yield();
        }

        // Break the loop in case resorting is needed
        if( this->m_rebase_start_priotask ){
            this->m_rebase_start_priotask = false;
            break;
        }
    }
    this->remove_expired_tasks();
}

/**
 * @brief Removes all expired tasks from the scheduler.
 */
void TaskScheduler::remove_expired_tasks()
{
    for (uint16_t i = 0; i < this->m_tasks.size(); i++)
    {
        if (this->m_tasks[i].m_task_id >= 0 && this->m_tasks[i].m_max_attempts == 0)
        {
            if (this->m_tasks[i].m_finalizer)
            {
                CallBackVoidPointerArgFn _fin = this->m_tasks[i].m_finalizer;
                CRITICAL_SECTION_ENTER
                this->m_tasks[i].m_finalizer = nullptr;
                CRITICAL_SECTION_EXIT
                _fin(&this->m_tasks[i]);
            }

            // released in place. erasing would shift later entries, and that
            // reassignment frees the callable a running task is executing
            CRITICAL_SECTION_ENTER
            this->m_tasks[i].clear();
            CRITICAL_SECTION_EXIT
        }
    }
}

/**
 * @brief Sets the teardown hook run once when a task is reaped.
 *
 * @param _id The unique ID of the task.
 * @param _fn The callback to run when the task is released.
 * @return True if the task was found and the hook set.
 */
bool TaskScheduler::setTaskFinalizer(pdiutil::task_id_t _id, CallBackVoidPointerArgFn _fn)
{
    int16_t _index = this->is_registered_task(_id);
    if (_index < 0)
    {
        return false;
    }

    CRITICAL_SECTION_ENTER
    this->m_tasks[_index].m_finalizer = _fn;
    CRITICAL_SECTION_EXIT
    return true;
}

/**
 * @brief Reports whether a task already carries a teardown hook.
 *
 * @param _id The unique ID of the task.
 * @return True when the task exists and has a finalizer set.
 */
bool TaskScheduler::hasTaskFinalizer(pdiutil::task_id_t _id)
{
    int16_t _index = this->is_registered_task(_id);
    if (_index < 0)
    {
        return false;
    }

    return (bool)this->m_tasks[_index].m_finalizer;
}

/**
 * @brief Checks if a task is registered.
 *
 * @param _id The unique ID of the task to check.
 * @return The index of the task if registered, or -1 if not found.
 */
int16_t TaskScheduler::is_registered_task(pdiutil::task_id_t _id)
{
    // a free slot carries a negative id, so it must never match a lookup
    if (_id < 0)
    {
        return -1;
    }

    for (uint16_t i = 0; i < this->m_tasks.size(); i++)
    {
        if (this->m_tasks[i].m_task_id == _id)
        {
            return i;
        }
    }
    return -1;
}

/**
 * @brief Removes a task from the scheduler.
 *
 * @param _id The unique ID of the task to remove.
 * @return True if the task was successfully removed, false otherwise.
 */
bool TaskScheduler::remove_task(pdiutil::task_id_t _id)
{
    // a free slot carries a negative id, so it must never match a lookup
    if (_id < 0)
    {
        return false;
    }

    bool _removed = false;
    for (uint16_t i = 0; i < this->m_tasks.size(); i++)
    {
        if (this->m_tasks[i].m_task_id == _id)
        {
			// removing task create bug if this function will call inside another task
			// hence making its max attempts to 0 which will considered as expired task
			// this->m_tasks.erase( this->m_tasks.begin() + i );
            CRITICAL_SECTION_ENTER
            this->m_tasks[i].m_duration = 10;
            setTaskPriority(this->m_tasks[i], 0);
            this->m_tasks[i].m_max_attempts = 0;
            this->m_tasks[i].m_task = nullptr;
            this->m_tasks[i].m_state = TASK_STATE_ZOMBIE;
            #ifdef ENABLE_CONTEXTUAL_EXECUTION
            this->m_tasks[i].m_task_exec = nullptr;
            #endif
            CRITICAL_SECTION_EXIT
            _removed = true;
        }
    }
    return _removed;
}

/**
 * @brief Generates a unique ID for a new task.
 *
 * @return A unique task ID.
 */
pdiutil::task_id_t TaskScheduler::get_unique_task_id()
{
    // ids advance instead of refilling the lowest gap, so the id of a task that
    // has gone is not handed straight to the next one registered, and a caller
    // still holding it updates nothing rather than another task
    for (uint16_t _attempt = 0; _attempt <= (uint16_t)this->m_max_tasks; _attempt++)
    {
        pdiutil::task_id_t _id = this->m_next_task_id;
        this->m_next_task_id = (_id >= MAX_TASK_ID) ? 1 : (pdiutil::task_id_t)(_id + 1);

        bool _id_used = false;
        for (uint16_t i = 0; i < this->m_tasks.size(); i++)
        {
            if (this->m_tasks[i].m_task_id == _id)
            {
                _id_used = true;
            }
        }
        if (!_id_used)
            return _id;
    }
    return -1;
}

/**
 * @brief Get task by its id
 *
 * @return task pointer pointing to task
 */
task_t* TaskScheduler::get_task(pdiutil::task_id_t _id)
{
    // a free slot carries a negative id, so it must never match a lookup
    if (_id < 0)
    {
        return nullptr;
    }

    for (uint16_t i = 0; i < this->m_tasks.size(); i++)
    {
        if (this->m_tasks[i].m_task_id == _id)
        {
            return &this->m_tasks[i];
        }
    }
    return nullptr;
}

/**
 * @brief Sets the maximum number of tasks allowed in the scheduler.
 *
 * @param maxtasks The maximum number of tasks.
 */
void TaskScheduler::setMaxTasksLimit(uint8_t maxtasks)
{
    m_max_tasks = maxtasks;
}

/**
 * @brief Sets the utility interface for the scheduler.
 *
 * This must be set before starting the scheduler.
 *
 * @param util Pointer to the utility interface.
 */
void TaskScheduler::setUtilityInterface(iUtilityInterface *util)
{
    m_util = util;
}

/**
 * @brief Break the task execution, sort with priorities and restart the task queue.
 *
 */
void TaskScheduler::rebaseAndRestartPrioTasks()
{
    m_rebase_start_priotask = true;
}

/**
 * @brief Base class api to yield the running task.
 * Currently handling device specific yield not switching context as this is under cooperative schedule context.
 */
void TaskScheduler::yield() 
{
    m_util->yield();
}

/**
 * @brief Sleep the current task .
 * Currently not handling here as this is under cooperative schedule context.
 * @param ms sleep time in milliseconds.
 */
void TaskScheduler::sleep(uint32_t ms) 
{
    
}

/**
 * @brief mute the current task .
 * Currently not handling here as this is under cooperative schedule context.
 */
void TaskScheduler::mute() 
{
    
}

/**
 * @brief Run the scheduled tasks.
 * This needs to be called from main entry loop to run the cooperative tasks.
 */
void TaskScheduler::run() 
{
    this->handle_tasks();
}

#ifdef ENABLE_CONTEXTUAL_EXECUTION

/**
 * @brief Schedule task under context based execution scheduler
 */
int TaskScheduler::scheduleUnderExecSched(iExecutionScheduler* _exec_sched, pdiutil::task_id_t _task_id, task_mode_t _task_mode, uint32_t _stackdepth)
{
    int ret = PDI_ERR_NOT_IMPLEMENTED;

    if( _exec_sched ){

        task_t* t = __task_scheduler.get_task(_task_id);

        if( 
            nullptr != t && 
            _task_mode != TASK_MODE_INLINE && 
            nullptr == t->m_task_exec
        ){

            t->m_task_mode = _task_mode;
            ret = _exec_sched->schedule_task(t, _stackdepth);
        }
    }
    return ret;
}

#endif

/**
 * @brief Global instance of the TaskScheduler class.
 *
 * This instance is used to manage tasks throughout the PDI stack.
 */
TaskScheduler __task_scheduler;
