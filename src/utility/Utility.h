/******************************* Utility **************************************
This file is part of the Ewings Esp Stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st June 2019
******************************************************************************/

#ifndef __EWINGS_UTILITY_H__
#define __EWINGS_UTILITY_H__

#include <config/Config.h>

#ifdef ENABLE_TIMER_TASK_SCHEDULER
#include "TimerTaskScheduler.h"
#else
#include "TaskScheduler.h"
#endif

#include "iUtilityInterface.h"
#include "DataTypeConversions.h"
#include "StringOperations.h"
#include "queue/queue.h"
#include "Log.h"

/**
 * This template clone program memory object to data memory.
 *
 * @param	cost T* sce
 * @param	T&	dest
 */
template <typename T> void PROGMEM_readAnything (const T * sce, T& dest)
{
	memcpy_P (&dest, sce, sizeof (T));
}

/**
 * This template returns static copy of program memory object.
 *
 * @param		cost T* sce
 * @return	T
 */
template <typename T> T PROGMEM_getAnything (const T * sce)
{
	static T temp;
	memcpy_P (&temp, sce, sizeof (T));
	return temp;
}

/**
 * Template to clear type of object/struct.
 *
 * @param	cost Struct* _object
 */
template <typename Struct> void _ClearObject (const Struct * _object) {
  for (unsigned int i = 0; i < sizeof((*_object)); i++){
    *((char*) & (*_object) + i) = 0;
  }
}


#endif
