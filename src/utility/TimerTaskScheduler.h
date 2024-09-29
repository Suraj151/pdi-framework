// /************************** TimerTaskScheduler *********************************
// This file is part of the pdi stack.

// This is free software. you can redistribute it and/or modify it but without any
// warranty.

// Author          : Suraj I.
// created Date    : 1st June 2019
// ******************************************************************************/

// #ifndef __TIMER_TASK_SCHEDULER_H__
// #define __TIMER_TASK_SCHEDULER_H__


// #include "Log.h"
// #include <config/Config.h>
// #include <Ticker.h>

// #define DEFAULT_TASK_PRIORITY 0

// class TimerTask
// {

// private:
//   int m_task_id;
//   int m_max_attempts;
//   uint32_t m_duration;
//   void *m_arg;
//   Ticker m_ticker;
//   CallBackVoidArgFn m_cbfunction;

// public:
//   /**
//    * TimerTask constructor.
//    */
//   TimerTask(CallBackVoidArgFn cbfunction, void *arg, uint32_t duration, int task_id, int max_attempts) : m_arg(arg),
//                                                                                                          m_task_id(task_id),
//                                                                                                          m_max_attempts(max_attempts),
//                                                                                                          m_duration(duration),
//                                                                                                          m_cbfunction(cbfunction)
//   {
//     // this->start();
//   }

//   /**
//    * TimerTask destructor.
//    */
//   ~TimerTask()
//   {
//     this->stop();
//   };

//   /**
//    * task callback function
//    */
//   void task()
//   {
//     Serial.print(F("executing task : "));
//     Serial.println(this->m_task_id);

//     if (this->m_cbfunction && !this->isExpired())
//     {
//       // this->m_cbfunction(this->m_arg);
//       this->m_cbfunction();
//       this->m_max_attempts = this->m_max_attempts > 0 ? this->m_max_attempts - 1 : this->m_max_attempts;
//     }
//   }

//   void updateTask(CallBackVoidArgFn cbfunction, void *arg, uint32_t duration, int max_attempts)
//   {
//     this->m_arg = arg;
//     this->m_cbfunction = cbfunction;
//     this->m_max_attempts = max_attempts;
//     this->updateDuration(duration);
//   }

//   void updateDuration(uint32_t duration)
//   {
//     if (this->m_duration != duration)
//     {
//       this->m_duration = duration;

//       if (this->isRunning())
//       {
//         this->stop();
//         this->start();
//       }
//     }
//   }

//   void start()
//   {
//     // currently 50 ms is kept as lowest duration
//     if (!this->isRunning() && 50 <= this->m_duration)
//     {
//       this->m_ticker.attach_ms(this->m_duration, std::bind(&TimerTask::task, this));
//     }
//   }

//   void stop()
//   {
//     this->m_ticker.detach();
//   }

//   bool isRunning()
//   {
//     return this->m_ticker.active();
//   }

//   bool isExpired()
//   {
//     return this->m_max_attempts == 0;
//   }

//   void doExpire()
//   {
//     this->m_max_attempts = 0;
//   }

//   int getTaskId()
//   {
//     return this->m_task_id;
//   }

//   uint32_t getTaskDuration()
//   {
//     return this->m_duration;
//   }

//   int getMaxAttempts()
//   {
//     return this->m_max_attempts;
//   }
// };

// /**
//  * TimerTaskScheduler class
//  */
// class TimerTaskScheduler
// {

// public:
//   /**
//    * TimerTaskScheduler constructor
//    */
//   TimerTaskScheduler();
//   /**
//    * TimerTaskScheduler destructor
//    */
//   ~TimerTaskScheduler();
//   /**
//    * TimerTaskScheduler constructor
//    *
//    * @param	CallBackVoidArgFn	_task_fn
//    * @param	uint32_t	_duration
//    * @param	unsigned long|0	_last_millis
//    */
//   TimerTaskScheduler(CallBackVoidArgFn _task_fn, uint32_t _duration, int _task_priority = DEFAULT_TASK_PRIORITY, unsigned long _last_millis = 0);

//   int setTimeout(CallBackVoidArgFn _task_fn, uint32_t _duration, unsigned long _now_millis, int _task_priority = DEFAULT_TASK_PRIORITY);
//   int setInterval(CallBackVoidArgFn _task_fn, uint32_t _duration, unsigned long _now_millis, int _task_priority = DEFAULT_TASK_PRIORITY);
//   int updateInterval(int _task_id, CallBackVoidArgFn _task_fn, uint32_t _duration, int _task_priority = DEFAULT_TASK_PRIORITY, unsigned long _last_millis = 0, int _max_attempts = -1);
//   bool clearTimeout(int _id);
//   bool clearInterval(int _id);
//   int register_task(CallBackVoidArgFn _task_fn, uint32_t _duration, int _task_priority = DEFAULT_TASK_PRIORITY, unsigned long _last_millis = 0, int _max_attempts = -1);
//   void handle_tasks(void);
//   void remove_expired_tasks(void);
//   int is_registered_task(int _id);
//   bool remove_task(int _id);
//   int get_unique_task_id(void);
//   void printTaskSchedulerLogs(void);

// protected:
//   /**
//    * @var	std::vector<TimerTask> vector
//    */
//   std::vector<TimerTask> m_tasks;

// private:
//   /**
//    * @var	uint8_t m_max_tasks
//    */
//   uint8_t m_max_tasks;
// };

// extern TimerTaskScheduler __task_scheduler;

// #endif
