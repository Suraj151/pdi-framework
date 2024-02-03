/************************** TimerTaskScheduler *********************************
This file is part of the Ewings Esp Stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st June 2019
******************************************************************************/
#include <config/Config.h>

#ifdef ENABLE_TIMER_TASK_SCHEDULER

#include "TimerTaskScheduler.h"

/**
 * TimerTaskScheduler constructor
 */
TimerTaskScheduler::TimerTaskScheduler(): m_max_tasks(25){
}

/**
 * TimerTaskScheduler destructor
 */
TimerTaskScheduler::~TimerTaskScheduler(){
}

/**
 * TimerTaskScheduler constructor
 *
 * @param	CallBackVoidArgFn	_task_fn
 * @param	uint32_t	_duration
 * @param	unsigned long|0	_last_millis
 */
TimerTaskScheduler::TimerTaskScheduler( CallBackVoidArgFn _task_fn, uint32_t _duration, int _task_priority, unsigned long _last_millis ){

	this->register_task( _task_fn, _duration, _task_priority, _last_millis );
}

/**
 * set callback function which will execute once after given duration.
 * it will return callback task id for reference.
 *
 * @param		CallBackVoidArgFn	_task_fn
 * @param		uint32_t	_duration
 * @return	int
 */
int TimerTaskScheduler::setTimeout( CallBackVoidArgFn _task_fn, uint32_t _duration, unsigned long _now_millis, int _task_priority ){
	return this->register_task( _task_fn, _duration, _task_priority, _now_millis, 1 );
}

/**
 * set callback function which will execute after every given duration.
 * it will return callback task id for reference.
 *
 * @param		CallBackVoidArgFn	_task_fn
 * @param		uint32_t	_duration
 * @return	int
 */
int TimerTaskScheduler::setInterval( CallBackVoidArgFn _task_fn, uint32_t _duration, unsigned long _now_millis, int _task_priority ){
	return this->register_task( _task_fn, _duration, _task_priority, _now_millis );
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
int TimerTaskScheduler::updateInterval( int _task_id, CallBackVoidArgFn _task_fn, uint32_t _duration, int _task_priority, unsigned long _last_millis, int _max_attempts ){

	int _registered_index = this->is_registered_task( _task_id );
	if( _registered_index > -1 ){

		this->m_tasks[_registered_index].updateTask( _task_fn, nullptr, _duration, _max_attempts );
		return _task_id;
	}else{

		return this->register_task( _task_fn, _duration, _task_priority,_last_millis, _max_attempts );
	}
}

/**
 * this function clears the callback task of given ref. callback task id
 * it return success/failure of operation.
 *
 * @param		int	_id
 * @return	bool
 */
bool TimerTaskScheduler::clearTimeout( int _id ){
	return this->remove_task( _id );
}

/**
 * this function clears the callback task of given ref. callback task id
 * it return success/failure of operation.
 *
 * @param		int	_id
 * @return	bool
 */
bool TimerTaskScheduler::clearInterval( int _id ){
	return this->remove_task( _id );
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
int TimerTaskScheduler::register_task( CallBackVoidArgFn _task_fn, uint32_t _duration, int _task_priority, unsigned long _last_millis, int _max_attempts ){

	if( this->m_tasks.size() < this->m_max_tasks ){

		int _task_id = this->get_unique_task_id();
		Serial.print(F("creating task : "));
		Serial.println(_task_id);
		TimerTask _new_task( _task_fn, nullptr, _duration, _task_id, _max_attempts );
		this->m_tasks.push_back(_new_task);
		return _task_id;
	}
	return -1;
}

/**
 * this function handles/executes callback task on their given time periods.
 * this function must be called every millisecond in loop for proper task handles.
 *
 */
void TimerTaskScheduler::handle_tasks(){

	Serial.println(F("handeling tasks"));
	for ( uint16_t i = 0; i < this->m_tasks.size(); i++) {

		this->m_tasks[i].start();
	}
	this->remove_expired_tasks();
	//__i_dvc_ctrl.wait(0);
}

/**
 * this function removes expired callback task from lists
 */
void TimerTaskScheduler::remove_expired_tasks(){

	for ( uint16_t i = 0; i < this->m_tasks.size(); i++) {

		if( this->m_tasks[i].isExpired() ){

			Serial.print(F("removing task : "));
			Serial.println(this->m_tasks[i].getTaskId());

			this->m_tasks[i].stop();
			this->m_tasks.erase( this->m_tasks.begin() + i );
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
int TimerTaskScheduler::is_registered_task( int _id ){

	for ( uint16_t i = 0; i < this->m_tasks.size(); i++) {

		if( this->m_tasks[i].getTaskId() == _id ){
			return i;
    }
	}return -1;
}

/**
 * this function remove callback task by given id.
 * it returns true on removed successfully else it returns false on not found.
 *
 * @param		int	_id
 * @return	bool
 */
bool TimerTaskScheduler::remove_task( int _id ){

	bool _removed = false;
	for ( uint16_t i = 0; i < this->m_tasks.size(); i++) {

		if( this->m_tasks[i].getTaskId() == _id ){
			// removing task create bug if this function will call inside another task
			// hence making its max attempts to 0 which will considered as expired task
			// this->m_tasks.erase( this->m_tasks.begin() + i );
			this->m_tasks[i].doExpire();
			_removed = true;
    }
	}return _removed;
}

/**
 * this function returns available unique id required for callback task.
 *
 * @return	int
 */
int TimerTaskScheduler::get_unique_task_id( ){

	for ( int _id = 1; _id < this->m_max_tasks; _id++) {

		bool _id_used = false;
		for ( int i = 0; i < this->m_tasks.size(); i++) {

			if( this->m_tasks[i].getTaskId() == _id ){
				_id_used = true;
	    }
		}
		if( !_id_used ) return _id;
	} return -1;
}

#ifdef EW_SERIAL_LOG
/**
 * this function prints task logs.
 *
 */
void TimerTaskScheduler::printTaskSchedulerLogs(){

	Logln(F("\nTasks : "));
	Log(F("id\t"));
	Log(F("duration\t"));
	Log(F("max_attempts\n"));
	for ( int i = 0; i < this->m_tasks.size(); i++) {

    Log(this->m_tasks[i].getTaskId());Log("\t");
		Log(this->m_tasks[i].getTaskDuration());Log("\t\t");
		Log(this->m_tasks[i].getMaxAttempts());Logln();
	}
}
#endif

TimerTaskScheduler __task_scheduler;

#endif
