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

#include "TaskScheduler.h"

#include "iUtilityInterface.h"
#include "iInstanceInterface.h"
#include "SafeAlloc.h"
#include "EventUtil.h"
#include "DataTypeConversions.h"
#include "StringOperations.h"
#include "queue/queue.h"
#include "Base64.h"
#include "iIOInterface.h"
#ifdef ENABLE_CMD_SERVICE
#include "CommandBase.h"
#endif
// #ifdef ENABLE_SSH_SERVICE
// #include "crypto/ed25519/ed25519.h"
// #endif


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
    for (size_t i = 0; i < sizeof((*_object)); i++)
    {
        *((char *)&(*_object) + i) = 0;
    }
}

/**
 * @brief Moves one item down a heap until no child orders after it.
 *
 * @tparam T The type of the items.
 * @tparam Before A callable taking two items, true when the first orders before the second.
 * @param _items The items the heap is laid out in.
 * @param _root The index of the item to move down.
 * @param _count How many items the heap holds.
 * @param _before The ordering of two items.
 */
template <typename T, typename Before>
void _HeapSiftDown(T *_items, uint32_t _root, uint32_t _count, Before &_before)
{
    while (true)
    {
        uint32_t largest = _root;
        uint32_t left = 2 * _root + 1;
        uint32_t right = left + 1;
        if (left < _count && _before(_items[largest], _items[left])) largest = left;
        if (right < _count && _before(_items[largest], _items[right])) largest = right;
        if (largest == _root) return;
        T held = _items[_root];
        _items[_root] = _items[largest];
        _items[largest] = held;
        _root = largest;
    }
}

/**
 * @brief Sorts items in place by heap sort, which needs no extra memory and keeps the stack flat.
 *
 * @tparam T The type of the items.
 * @tparam Before A callable taking two items, true when the first orders before the second.
 * @param _items The items to sort.
 * @param _count How many items there are.
 * @param _before The ordering of two items.
 * @param _yield Called every 64 items placed, so a long sort lets other work run.
 */
template <typename T, typename Before>
void _HeapSort(T *_items, uint32_t _count, Before _before, const CallBackVoidArgFn &_yield = nullptr)
{
    for (uint32_t i = _count / 2; i > 0; i--)
    {
        _HeapSiftDown(_items, i - 1, _count, _before);
    }
    for (uint32_t end = _count; end > 1; end--)
    {
        T top = _items[0];
        _items[0] = _items[end - 1];
        _items[end - 1] = top;
        _HeapSiftDown(_items, 0, end - 1, _before);
        if (nullptr != _yield && 0 == (end & 63))
        {
            _yield();
        }
    }
}


#endif
