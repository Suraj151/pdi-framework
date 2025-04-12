/******************************* Utility **************************************
This file is part of the PDI stack.

This is free software. You can redistribute it and/or modify it but without any
warranty.

The Utility module provides a collection of utility functions, interfaces, and
includes essential components for the PDI stack. It integrates task scheduling,
event handling, data type conversions, string operations, and more.

Author          : Suraj I.
Created Date    : 1st June 2019
******************************************************************************/

#ifndef __STACK_UTILITY_H__
#define __STACK_UTILITY_H__

#include <config/Config.h>

#ifdef ENABLE_TIMER_TASK_SCHEDULER
#include "TimerTaskScheduler.h"
#else
#include "TaskScheduler.h"
#endif

#include "iUtilityInterface.h"
#include "EventUtil.h"
#include "DataTypeConversions.h"
#include "StringOperations.h"
#include "queue/queue.h"
#include "Base64.h"
#include "iIOInterface.h"
#ifdef ENABLE_CMD_SERVICE
#include "CommandBase.h"
#endif

/**
 * @brief This template clone program memory object to data memory.
 *
 * @param	cost T* sce
 * @param	T&	dest
 */
// template <typename T> void PROGMEM_readAnything (const T * sce, T& dest)
// {
// 	memcpy_P (&dest, sce, sizeof (T));
// }

/**
 * @brief This template returns static copy of program memory object.
 *
 * @param		cost T* sce
 * @return	T
 */
// template <typename T> T PROGMEM_getAnything (const T * sce)
// {
// 	static T temp;
// 	memcpy_P (&temp, sce, sizeof (T));
// 	return temp;
// }

/**
 * @brief Template to clear the contents of an object or struct.
 *
 * This template function clears the memory of a given object or struct by
 * setting all its bytes to zero.
 *
 * @tparam Struct The type of the object or struct to clear.
 * @param _object Pointer to the object or struct to clear.
 */
template <typename Struct>
void _ClearObject(const Struct *_object)
{
    for (unsigned int i = 0; i < sizeof((*_object)); i++)
    {
        *((char *)&(*_object) + i) = 0;
    }
}


#endif
