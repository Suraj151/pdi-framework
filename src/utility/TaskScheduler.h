/**************************** TaskScheduler ***********************************
This file is part of the PDI stack.

This is free software. You can redistribute it and/or modify it but without any
warranty.

The TaskScheduler class provides functionality for scheduling and managing 
tasks with specified intervals or timeouts. It supports task registration, 
execution, and removal, making it useful for managing periodic or delayed 
operations in embedded systems.

Author          : Suraj I.
Created Date    : 1st June 2019
******************************************************************************/

#ifndef __TASK_SCHEDULER_H__
#define __TASK_SCHEDULER_H__

#include "iUtilityInterface.h"

/**
 * @class TaskScheduler
 * @brief Provides functionality for scheduling and managing tasks.
 *
 * The TaskScheduler class allows for registering tasks with specific intervals
 * or timeouts, executing them, and removing them when necessary. It supports
 * task prioritization and handles expired tasks automatically.
 */
class TaskScheduler
{
public:
    /**
     * @brief Default constructor for the TaskScheduler class.
     */
    TaskScheduler();

    /**
     * @brief Destructor for the TaskScheduler class.
     */
    ~TaskScheduler();

    /**
     * @brief Parameterized constructor for the TaskScheduler class.
     *
     * Registers a task with the specified parameters.
     *
     * @param _task_fn The callback function to execute for the task.
     * @param _duration The interval or timeout duration for the task in milliseconds.
     * @param _task_priority The priority of the task (default is DEFAULT_TASK_PRIORITY).
     * @param _last_millis The last execution time of the task (default is 0).
     */
    TaskScheduler(CallBackVoidArgFn _task_fn, uint64_t _duration, int _task_priority = DEFAULT_TASK_PRIORITY, uint64_t _last_millis = 0);

    /**
     * @brief Sets a one-time timeout for a task.
     *
     * @param _task_fn The callback function to execute after the timeout.
     * @param _duration The timeout duration in milliseconds.
     * @param _now_millis The current time in milliseconds.
     * @param _task_priority The priority of the task (default is DEFAULT_TASK_PRIORITY).
     * @return The unique ID of the registered task.
     */
    int setTimeout(CallBackVoidArgFn _task_fn, uint64_t _duration, uint64_t _now_millis, int _task_priority = DEFAULT_TASK_PRIORITY);

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
    int updateTimeout(int _task_id, CallBackVoidArgFn _task_fn, uint64_t _duration, uint64_t _now_millis, int _task_priority = DEFAULT_TASK_PRIORITY);

    /**
     * @brief Sets a recurring interval for a task.
     *
     * @param _task_fn The callback function to execute at each interval.
     * @param _duration The interval duration in milliseconds.
     * @param _now_millis The current time in milliseconds.
     * @param _task_priority The priority of the task (default is DEFAULT_TASK_PRIORITY).
     * @return The unique ID of the registered task.
     */
    int setInterval(CallBackVoidArgFn _task_fn, uint64_t _duration, uint64_t _now_millis, int _task_priority = DEFAULT_TASK_PRIORITY);

    /**
     * @brief Updates the interval of an existing task.
     *
     * @param _task_id The unique ID of the task to update.
     * @param _task_fn The callback function for the task.
     * @param _duration The new interval duration in milliseconds.
     * @param _task_priority The priority of the task (default is DEFAULT_TASK_PRIORITY).
     * @param _last_millis The last execution time of the task (default is 0).
     * @param _max_attempts The maximum number of attempts for the task (default is -1 for unlimited).
     * @return The unique ID of the updated task.
     */
    int updateInterval(int _task_id, CallBackVoidArgFn _task_fn, uint64_t _duration, int _task_priority = DEFAULT_TASK_PRIORITY, uint64_t _last_millis = 0, int _max_attempts = -1);

    /**
     * @brief Clears a one-time timeout task.
     *
     * @param _id The unique ID of the task to clear.
     * @return True if the task was successfully cleared, false otherwise.
     */
    bool clearTimeout(int _id);

    /**
     * @brief Clears a recurring interval task.
     *
     * @param _id The unique ID of the task to clear.
     * @return True if the task was successfully cleared, false otherwise.
     */
    bool clearInterval(int _id);

    /**
     * @brief Registers a new task.
     *
     * @param _task_fn The callback function for the task.
     * @param _duration The interval or timeout duration for the task in milliseconds.
     * @param _task_priority The priority of the task (default is DEFAULT_TASK_PRIORITY).
     * @param _last_millis The last execution time of the task (default is 0).
     * @param _max_attempts The maximum number of attempts for the task (default is -1 for unlimited).
     * @return The unique ID of the registered task.
     */
    int register_task(CallBackVoidArgFn _task_fn, uint64_t _duration, int _task_priority = DEFAULT_TASK_PRIORITY, uint64_t _last_millis = 0, int _max_attempts = -1);

    /**
     * @brief Executes all registered tasks that are due.
     */
    void handle_tasks();

    /**
     * @brief Removes all expired tasks from the scheduler.
     */
    void remove_expired_tasks(void);

    /**
     * @brief Checks if a task is registered.
     *
     * @param _id The unique ID of the task to check.
     * @return The index of the task if registered, or -1 if not found.
     */
    int is_registered_task(int _id);

    /**
     * @brief Removes a task from the scheduler.
     *
     * @param _id The unique ID of the task to remove.
     * @return True if the task was successfully removed, false otherwise.
     */
    bool remove_task(int _id);

    /**
     * @brief Generates a unique ID for a new task.
     *
     * @return A unique task ID.
     */
    int get_unique_task_id(void);

    /**
     * @brief Sets the maximum number of tasks allowed in the scheduler.
     *
     * @param maxtasks The maximum number of tasks.
     */
    void setMaxTasksLimit(uint8_t maxtasks);

    /**
     * @brief Sets the utility interface for the scheduler.
     *
     * This must be set before starting the scheduler.
     *
     * @param util Pointer to the utility interface.
     */
    void setUtilityInterface(iUtilityInterface *util);

    /**
     * @brief Break the task execution, sort with priorities and restart the task queue.
     *
     */
    void rebaseAndRestartPrioTasks();

    /**
     * @brief Prints all registered tasks to the terminal.
     *
     * @param terminal Pointer to the terminal interface.
     */
    void printTasksToTerminal(iTerminalInterface *terminal);

protected:
    /**
     * @var pdiutil::vector<task_t> m_tasks
     * @brief Vector of registered tasks.
     */
    pdiutil::vector<task_t> m_tasks;

    /**
     * @var iUtilityInterface* m_util
     * @brief Pointer to the utility interface.
     */
    iUtilityInterface *m_util;

    /**
     * @brief Sort the task indices according to their priority and score.
     *
     */
    void getSortedTaskList(uint16_t* _priority_indices, uint16_t _task_count);

private:
    /**
     * @var uint8_t m_max_tasks
     * @brief Maximum number of tasks allowed in the scheduler.
     */
    uint8_t m_max_tasks;

    /**
     * @var bool m_rebase_start_priotask
     * @brief Break the task execution, sort with priorities and restart the task queue.
     */
    bool m_rebase_start_priotask;
};

/**
 * @brief Global instance of the TaskScheduler class.
 *
 * This instance is used to manage tasks throughout the PDI stack.
 */
extern TaskScheduler __task_scheduler;

#endif
