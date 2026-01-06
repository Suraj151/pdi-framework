/**************************** TaskScheduler ***********************************
This file is part of the PDI stack.

This is free software. You can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
Created Date    : 1st June 2019
******************************************************************************/

#include <config/Common.h>

#ifndef ENABLE_TIMER_TASK_SCHEDULER

#include "TaskScheduler.h"
#include "DataTypeConversions.h"

/**
 * @brief Default constructor for the TaskScheduler class.
 *
 * Initializes the task scheduler with default values.
 */
TaskScheduler::TaskScheduler() : m_util(nullptr),
                                 m_max_tasks(MAX_SCHEDULABLE_TASKS),
                                 m_rebase_start_priotask(false)
{
    this->m_tasks.reserve(MAX_SCHEDULABLE_TASKS);
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
TaskScheduler::TaskScheduler(CallBackVoidArgFn _task_fn, uint64_t _duration, int _task_priority, uint64_t _last_millis)
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
int TaskScheduler::setTimeout(CallBackVoidArgFn _task_fn, uint64_t _duration, uint64_t _now_millis, int _task_priority)
{
    return this->register_task(_task_fn, _duration, _task_priority, _now_millis, 1);
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
int TaskScheduler::updateTimeout(int _task_id, CallBackVoidArgFn _task_fn, uint64_t _duration, uint64_t _now_millis, int _task_priority)
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
int TaskScheduler::setInterval(CallBackVoidArgFn _task_fn, uint64_t _duration, uint64_t _now_millis, int _task_priority)
{
    return this->register_task(_task_fn, _duration, _task_priority, _now_millis);
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
int TaskScheduler::updateInterval(int _task_id, CallBackVoidArgFn _task_fn, uint64_t _duration, int _task_priority, uint64_t _last_millis, int _max_attempts)
{
    int _registered_index = this->is_registered_task(_task_id);
    if (_registered_index > -1)
    {
        this->m_tasks[_registered_index]._task = _task_fn;
        this->m_tasks[_registered_index]._duration = _duration;
        this->m_tasks[_registered_index]._task_priority = _task_priority;
        this->m_tasks[_registered_index]._last_millis = _last_millis == 0 ? this->m_tasks[_registered_index]._last_millis : _last_millis;
        this->m_tasks[_registered_index]._max_attempts = _max_attempts;
        return _task_id;
    }
    else
    {
        return this->register_task(_task_fn, _duration, _task_priority, _last_millis, _max_attempts);
    }
}

/**
 * @brief Clears a one-time timeout task.
 *
 * @param _id The unique ID of the task to clear.
 * @return True if the task was successfully cleared, false otherwise.
 */
bool TaskScheduler::clearTimeout(int _id)
{
    return this->remove_task(_id);
}

/**
 * @brief Clears a recurring interval task.
 *
 * @param _id The unique ID of the task to clear.
 * @return True if the task was successfully cleared, false otherwise.
 */
bool TaskScheduler::clearInterval(int _id)
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
int TaskScheduler::register_task(CallBackVoidArgFn _task_fn, uint64_t _duration, int _task_priority, uint64_t _last_millis, int _max_attempts)
{
    if (this->m_tasks.size() < this->m_max_tasks)
    {
        task_t _new_task;
        _new_task._task = _task_fn;
        _new_task._duration = _duration;
        _new_task._task_priority = _task_priority;
        _new_task._last_millis = _last_millis;
        _new_task._max_attempts = _max_attempts;
        _new_task._task_exec_millis = 0;
        _new_task._task_id = this->get_unique_task_id();
        this->m_tasks.push_back(_new_task);
        return _new_task._task_id;
    }
    return -1;
}

/**
 * @brief Return the computed score for task.
 *
 */
int TaskScheduler::computeScore(const task_t& _t, uint64_t _now)
{
    uint64_t next_due = _t._last_millis + _t._duration;

    // --- Catch-up logic: overdue tasks get priority ---
    bool overdue = (_now >= next_due);

    // --- Compute lateness in ms (0 if not overdue) ---
    int64_t lateness = overdue ? (int64_t)(_now - next_due) : 0;

    // --- Compute earlyness in ms (0 if overdue) ---
    int64_t earlyness = overdue ? 0 : (int64_t)(next_due - _now);

    // --- Compute recentness in ms ---
    int64_t recentness = (int64_t)(_now - _t._last_millis);

    // Priority Weight
    int priority_weight = 100;

    // Policy Weight is policy_boost * policy_cap
    int policy_cap = 50;
    double policy_boost = 1.0;

    switch (_t._task_policy) { 
        case TASK_POLICY_DEADLINE: { policy_boost = 2; break; }
        case TASK_POLICY_FAIRSHARE: { policy_boost = 1.5; break; }
        default: break;
    }

    // --- Blend policy, priority into a score ---
    switch (_t._task_policy) { 
        case TASK_POLICY_ROUNDROBIN: 
            // Equal slices: favor tasks that ran less recently 
            return _t._task_priority * priority_weight + policy_boost * pdistd::min((int)floor(log2((double)recentness + 1.0)), policy_cap); 
        case TASK_POLICY_DEADLINE: 
            // Earliest deadline first: smaller next_due is higher urgency 
            return _t._task_priority * priority_weight + policy_boost * pdistd::max(0, policy_cap - pdistd::min((int)floor(log2((double)earlyness + 1.0)), policy_cap)); 
        case TASK_POLICY_FAIRSHARE: 
            // Favor tasks that consumed less CPU time 
            return _t._task_priority * priority_weight + policy_boost * pdistd::max(0, policy_cap - pdistd::min((int)floor(log2((double)_t._task_exec_millis + 1.0)), policy_cap)); 
        case TASK_POLICY_FIFO: 
        default: 
            // Priority dominates, lateness nudges overdue tasks 
            return _t._task_priority * priority_weight + policy_boost * pdistd::min((int)lateness / 10, policy_cap); 
    }
}

/**
 * @brief Sort the task indices according to their priority and score.
 *
 */
void TaskScheduler::getSortedTaskList(uint16_t* _priority_indices, uint16_t _task_count)
{
    const uint32_t tolerance = 3; // ms tolerance window
    uint64_t now = m_util->millis_now();

    for (uint16_t i = 0; i < _task_count; i++)
        _priority_indices[i] = i;

    for (uint16_t i = 0; i < _task_count; i++)
    {
        for (uint16_t j = i + 1; j < _task_count; j++)
        {
            auto &task_i = this->m_tasks[_priority_indices[i]];
            auto &task_j = this->m_tasks[_priority_indices[j]];

            uint64_t next_due_i = task_i._last_millis + task_i._duration;
            uint64_t next_due_j = task_j._last_millis + task_j._duration;

            bool swap_needed = false;

            // --- Blend policy, priority into a score ---
            int score_i = this->computeScore(task_i, now);
            int score_j = this->computeScore(task_j, now);
            
            // --- Compare blended score first (priority-biased) ---
            if (score_i != score_j) {

                swap_needed = (score_j > score_i);
            } else {
                // --- Scores equal → compare due times with tolerance ---
                int64_t diff = (int64_t)(next_due_i - next_due_j);

                if (pdistd::abs(diff) > (int64_t)tolerance) {
                    swap_needed = (next_due_j < next_due_i);
                } else {
                    // --- Due times effectively equal → compare exec time ---
                    if (task_i._task_exec_millis > task_j._task_exec_millis) {
                        swap_needed = true;
                    }
                }
            }

            if (swap_needed) {
                pdistd::swap(_priority_indices[i], _priority_indices[j]);
            }
        }
    }
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

    uint16_t _task_count = this->m_tasks.size();
    uint16_t _priority_indices[_task_count];

    this->getSortedTaskList(_priority_indices, _task_count);
    this->m_rebase_start_priotask = true; // run only one task and break for next turn

    for (uint16_t i = 0; i < _task_count; i++)
    {
        uint64_t _last_start_ms = m_util->millis_now();
        auto &_task = this->m_tasks[_priority_indices[i]];

        #ifdef ENABLE_CONCURRENT_EXECUTION
        if( _task._task_mode == TASK_MODE_CONCURRENT_EXEC ) continue;
        #endif

        if (_last_start_ms < _task._last_millis)
        {
            _task._last_millis = _last_start_ms;
        }

        if (_task._max_attempts != 0 && (_last_start_ms - _task._last_millis) >= _task._duration)
        {
            if (nullptr != _task._task)
            {
                _task._task();
            }

            if( 0 == _task._last_millis ){

                _task._last_millis = _last_start_ms;
            }else{

                // --- Catch-up: advance last_millis by multiples of duration ---
                int catchupround = 0;
                while ((_last_start_ms - _task._last_millis) >= _task._duration) {
                    _task._last_millis += _task._duration; // Reduced drift
                    if (++catchupround > 3) break;  // break for max catchup rounds
                }
            }

            _task._max_attempts = _task._max_attempts > 0 ? _task._max_attempts - 1 : _task._max_attempts;

            uint64_t _task_end_ms = m_util->millis_now();
            _task._task_exec_millis = _task_end_ms > _last_start_ms ? (_task_end_ms - _last_start_ms) : 0;
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
        if (this->m_tasks[i]._max_attempts == 0)
        {
            this->m_tasks.erase(this->m_tasks.begin() + i);
        }
    }
}

/**
 * @brief Checks if a task is registered.
 *
 * @param _id The unique ID of the task to check.
 * @return The index of the task if registered, or -1 if not found.
 */
int TaskScheduler::is_registered_task(int _id)
{
    for (uint16_t i = 0; i < this->m_tasks.size(); i++)
    {
        if (this->m_tasks[i]._task_id == _id)
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
bool TaskScheduler::remove_task(int _id)
{
    bool _removed = false;
    for (uint16_t i = 0; i < this->m_tasks.size(); i++)
    {
        if (this->m_tasks[i]._task_id == _id)
        {
			// removing task create bug if this function will call inside another task
			// hence making its max attempts to 0 which will considered as expired task
			// this->m_tasks.erase( this->m_tasks.begin() + i );
            this->m_tasks[i]._duration = 10;
            this->m_tasks[i]._task_priority = 0;
            this->m_tasks[i]._max_attempts = 0;
            this->m_tasks[i]._task = nullptr;
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
int TaskScheduler::get_unique_task_id()
{
    for (int _id = 1; _id < this->m_max_tasks; _id++)
    {
        bool _id_used = false;
        for (int i = 0; i < this->m_tasks.size(); i++)
        {
            if (this->m_tasks[i]._task_id == _id)
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
task_t* TaskScheduler::get_task(int _id)
{
    for (int i = 0; i < this->m_tasks.size(); i++)
    {
        if (this->m_tasks[i]._task_id == _id)
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
 * @brief Prints all registered tasks to the terminal.
 *
 * @param terminal Pointer to the terminal interface.
 */
void TaskScheduler::printTasksToTerminal(iTerminalInterface *terminal)
{
    if( nullptr != terminal ){

        terminal->writeln();
        terminal->writeln_ro(RODT_ATTR("Tasks : "));
        terminal->write_ro(RODT_ATTR("id        ")); // max column size=10
        terminal->write_ro(RODT_ATTR("priority  ")); // max column size=10
        terminal->write_ro(RODT_ATTR("policy    ")); // max column size=10
        terminal->write_ro(RODT_ATTR("interval  ")); // max column size=10
        terminal->write_ro(RODT_ATTR("last_ms   ")); // max column size=10
        terminal->write_ro(RODT_ATTR("exc_ms    ")); // max column size=10
        terminal->writeln_ro(RODT_ATTR("max_attempts")); // max column size=14

        char content[20];

        for (int i = 0; i < this->m_tasks.size(); i++)
        {
            Int32ToString(this->m_tasks[i]._task_id, content, 20, 10);
            terminal->write(content);

            Int32ToString(this->m_tasks[i]._task_priority, content, 20, 10);
            terminal->write(content);

            Int32ToString(this->m_tasks[i]._task_policy, content, 20, 10);
            terminal->write(content);

            Int64ToString(this->m_tasks[i]._duration, content, 20, 10);
            terminal->write(content);

            Int64ToString(this->m_tasks[i]._last_millis, content, 20, 10);
            terminal->write(content);

            Int64ToString(this->m_tasks[i]._task_exec_millis, content, 20, 10);
            terminal->write(content);

            Int32ToString(this->m_tasks[i]._max_attempts, content, 20, 14);
            terminal->write(content);
            terminal->writeln();
        }
    }
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
 * @brief Run the scheduled tasks.
 * This needs to be called from main entry loop to run the cooperative tasks.
 */
void TaskScheduler::run() 
{
    this->handle_tasks();
}

#ifdef ENABLE_CONCURRENT_EXECUTION

/**
 * @brief Schedule task under execution scheduler
 */
void TaskScheduler::scheduleUnderExecSched(iExecutionScheduler* _exec_sched, int _task_id, uint32_t stackdepth)
{
    if( _exec_sched ){

        task_t* t = __task_scheduler.get_task(_task_id);

        if(nullptr != t){

            t->_task_mode = TASK_MODE_CONCURRENT_EXEC;
            _exec_sched->schedule_task(t, stackdepth);
        }
    }
}

#endif

/**
 * @brief Global instance of the TaskScheduler class.
 *
 * This instance is used to manage tasks throughout the PDI stack.
 */
TaskScheduler __task_scheduler;

#endif
