/**************************** TaskScheduler ***********************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st June 2019
******************************************************************************/

#ifndef __TASK_SCHEDULER_H__
#define __TASK_SCHEDULER_H__

#include "iUtilityInterface.h"

#define DEFAULT_TASK_PRIORITY 0

/**
 * task struct type for scheduler
 */
struct task_t
{
	int _task_id;
	int _max_attempts;
	uint32_t _duration;
	unsigned long _last_millis;
	CallBackVoidArgFn _task;
	int _task_priority; // from 0 onwards. default is 0
	uint32_t _task_exec_millis;
};

/**
 * TaskScheduler class
 */
class TaskScheduler
{

public:
	/**
	 * TaskScheduler constructor
	 */
	TaskScheduler();
	/**
	 * TaskScheduler destructor
	 */
	~TaskScheduler();
	/**
	 * TaskScheduler constructor
	 *
	 * @param	CallBackVoidArgFn	_task_fn
	 * @param	uint32_t	_duration
	 * @param	unsigned long|0	_last_millis
	 */
	TaskScheduler(CallBackVoidArgFn _task_fn, uint32_t _duration, int _task_priority = DEFAULT_TASK_PRIORITY, unsigned long _last_millis = 0);

	int setTimeout(CallBackVoidArgFn _task_fn, uint32_t _duration, unsigned long _now_millis, int _task_priority = DEFAULT_TASK_PRIORITY);
	int setInterval(CallBackVoidArgFn _task_fn, uint32_t _duration, unsigned long _now_millis, int _task_priority = DEFAULT_TASK_PRIORITY);
	int updateInterval(int _task_id, CallBackVoidArgFn _task_fn, uint32_t _duration, int _task_priority = DEFAULT_TASK_PRIORITY, unsigned long _last_millis = 0, int _max_attempts = -1);
	bool clearTimeout(int _id);
	bool clearInterval(int _id);
	int register_task(CallBackVoidArgFn _task_fn, uint32_t _duration, int _task_priority = DEFAULT_TASK_PRIORITY, unsigned long _last_millis = 0, int _max_attempts = -1);
	void handle_tasks();
	void remove_expired_tasks(void);
	int is_registered_task(int _id);
	bool remove_task(int _id);
	int get_unique_task_id(void);
	void setMaxTasksLimit(uint8_t maxtasks);
	// This needs to be set prior to start the scheduler handle
	void setUtilityInterface(iUtilityInterface * util);
	void printTaskSchedulerLogs(void);

protected:
	/**
	 * @var	pdiutil::vector<task_t> m_tasks
	 */
	pdiutil::vector<task_t> m_tasks;

	/**
	 * @var	iUtilityInterface* m_util
	 */
	iUtilityInterface *m_util;

private:
	/**
	 * @var	uint8_t m_max_tasks
	 */
	uint8_t m_max_tasks;
};

extern TaskScheduler __task_scheduler;

#endif
