/**************************** TaskScheduler ***********************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st June 2019
******************************************************************************/

#include <config/Common.h>

#ifndef ENABLE_TIMER_TASK_SCHEDULER

#include "TaskScheduler.h"
#include "DataTypeConversions.h"

/**
 * TaskScheduler constructor
 */
TaskScheduler::TaskScheduler() : m_util(nullptr),
								 m_max_tasks(25)
{
}

/**
 * TaskScheduler destructor
 */
TaskScheduler::~TaskScheduler()
{
}

/**
 * TaskScheduler constructor
 *
 * @param	CallBackVoidArgFn	_task_fn
 * @param	uint32_t	_duration
 * @param	unsigned long|0	_last_millis
 */
TaskScheduler::TaskScheduler(CallBackVoidArgFn _task_fn, uint32_t _duration, int _task_priority, unsigned long _last_millis)
{
	this->register_task(_task_fn, _duration, _task_priority, _last_millis);
}

/**
 * set callback function which will execute once after given duration.
 * it will return callback task id for reference.
 *
 * @param		CallBackVoidArgFn	_task_fn
 * @param		uint32_t	_duration
 * @return	int
 */
int TaskScheduler::setTimeout(CallBackVoidArgFn _task_fn, uint32_t _duration, unsigned long _now_millis, int _task_priority)
{
	return this->register_task(_task_fn, _duration, _task_priority, _now_millis, 1);
}

/**
 * set callback function which will execute after every given duration.
 * it will return callback task id for reference.
 *
 * @param		CallBackVoidArgFn	_task_fn
 * @param		uint32_t	_duration
 * @return	int
 */
int TaskScheduler::setInterval(CallBackVoidArgFn _task_fn, uint32_t _duration, unsigned long _now_millis, int _task_priority)
{
	return this->register_task(_task_fn, _duration, _task_priority, _now_millis);
}

/**
 * this function updates already registered callback task.
 * it will return updated callback task id for reference.
 *
 * @param		int	_task_id
 * @param		CallBackVoidArgFn	_task_fn
 * @param		uint32_t	_duration
 * @param		unsigned long|0	_last_millis
 * @param		int|-1	_max_attempts
 * @return	int
 */
int TaskScheduler::updateInterval(int _task_id, CallBackVoidArgFn _task_fn, uint32_t _duration, int _task_priority, unsigned long _last_millis, int _max_attempts)
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
 * this function clears the callback task of given ref. callback task id
 * it return success/failure of operation.
 *
 * @param		int	_id
 * @return	bool
 */
bool TaskScheduler::clearTimeout(int _id)
{
	return this->remove_task(_id);
}

/**
 * this function clears the callback task of given ref. callback task id
 * it return success/failure of operation.
 *
 * @param		int	_id
 * @return	bool
 */
bool TaskScheduler::clearInterval(int _id)
{
	return this->remove_task(_id);
}

/**
 * this function registers callback task of given duration with unique id.
 * it will return callback task id for reference.
 *
 * @param		CallBackVoidArgFn	_task_fn
 * @param		uint32_t	_duration
 * @param		unsigned long|0	_last_millis
 * @param		int|-1	_max_attempts
 * @return	int
 */
int TaskScheduler::register_task(CallBackVoidArgFn _task_fn, uint32_t _duration, int _task_priority, unsigned long _last_millis, int _max_attempts)
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
 * this function handles/executes callback task on their given time periods.
 * this function must be called every millisecond in loop for proper task handles.
 *
 */
void TaskScheduler::handle_tasks()
{

	if (nullptr == m_util)
	{
		return;
	}

	uint16_t _task_count = this->m_tasks.size();
	uint16_t _priority_indices[_task_count];

	for (uint16_t i = 0; i < _task_count; i++)
		_priority_indices[i] = i;
	for (uint16_t i = 0; i < _task_count; i++)
	{
		for (uint16_t j = i + 1; j < _task_count; j++)
		{
			if (this->m_tasks[i]._task_priority < this->m_tasks[j]._task_priority)
			{
				std::swap(_priority_indices[i], _priority_indices[j]);
			}
		}
	}

	for (uint16_t i = 0; i < _task_count; i++)
	{
		unsigned long _last_start_ms = m_util->millis_now();

		if (_last_start_ms < this->m_tasks[_priority_indices[i]]._last_millis)
		{
			this->m_tasks[_priority_indices[i]]._last_millis = _last_start_ms;
		}

		if ((_last_start_ms - this->m_tasks[_priority_indices[i]]._last_millis) > this->m_tasks[_priority_indices[i]]._duration)
		{
			if (nullptr != this->m_tasks[_priority_indices[i]]._task)
			{
				this->m_tasks[_priority_indices[i]]._task();
			}
			this->m_tasks[_priority_indices[i]]._last_millis = _last_start_ms;
			this->m_tasks[_priority_indices[i]]._max_attempts = this->m_tasks[_priority_indices[i]]._max_attempts > 0 ? this->m_tasks[_priority_indices[i]]._max_attempts - 1 : this->m_tasks[_priority_indices[i]]._max_attempts;

			unsigned long _task_end_ms = m_util->millis_now();
			this->m_tasks[_priority_indices[i]]._task_exec_millis = _task_end_ms > _last_start_ms ? (_task_end_ms - _last_start_ms) : 0;
		}

		if( nullptr != m_util )
		{
			m_util->yield();
		}
	}
	this->remove_expired_tasks();
}

/**
 * this function removes expired callback task from lists
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
 * this function checks whether callback task is registered for given id.
 * it returns id on found registered else it returns -1
 *
 * @param		int	_id
 * @return	int
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
 * this function remove callback task by given id.
 * it returns true on removed successfully else it returns false on not found.
 *
 * @param		int	_id
 * @return	bool
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
 * this function returns available unique id required for callback task.
 *
 * @return	int
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
 * this function set the maximum number of tasks that scheduler can handle
 *
 * @return	none
 */
void TaskScheduler::setMaxTasksLimit(uint8_t maxtasks)
{
	m_max_tasks = maxtasks;
}

/**
 * this function set the utility interface needed by scheduler
 *
 * @return	none
 */
void TaskScheduler::setUtilityInterface(iUtilityInterface *util)
{
	m_util = util;
}

/**
 * this function prints task logs.
 *
 */
void TaskScheduler::printTaskSchedulerLogs()
{
	if (nullptr != m_util)
	{
		m_util->log(INFO_LOG, "\nTasks : \n");
		m_util->log(INFO_LOG, "id        "); // max column size=10
		m_util->log(INFO_LOG, "priority  "); // max column size=10
		m_util->log(INFO_LOG, "interval  "); // max column size=10
		m_util->log(INFO_LOG, "last_ms   "); // max column size=10
		m_util->log(INFO_LOG, "exc_ms    "); // max column size=10
		m_util->log(INFO_LOG, "max_attempts\n"); // max column size=14

		char content[20];

		for (int i = 0; i < this->m_tasks.size(); i++)
		{
			Int32ToString(this->m_tasks[i]._task_id, content, 20, 10);
			m_util->log(INFO_LOG, content);

			Int32ToString(this->m_tasks[i]._task_priority, content, 20, 10);
			m_util->log(INFO_LOG, content);

			Int32ToString(this->m_tasks[i]._duration, content, 20, 10);
			m_util->log(INFO_LOG, content);

			Int32ToString(this->m_tasks[i]._last_millis, content, 20, 10);
			m_util->log(INFO_LOG, content);

			Int32ToString(this->m_tasks[i]._task_exec_millis, content, 20, 10);
			m_util->log(INFO_LOG, content);

			Int32ToString(this->m_tasks[i]._max_attempts, content, 20, 14);
			m_util->log(INFO_LOG, content);
			m_util->log(INFO_LOG, "\n");
		}
	}
}

TaskScheduler __task_scheduler;

#endif
