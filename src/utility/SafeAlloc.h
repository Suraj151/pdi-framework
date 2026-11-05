/******************************* SafeAlloc *************************************
This file is part of the PDI stack.

This is free software. You can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
Created Date    : 6th June 2026
******************************************************************************/

#ifndef __PDI_SAFE_ALLOC_H__
#define __PDI_SAFE_ALLOC_H__

#include <stddef.h>
#include <stdint.h>
#include <new>

/**
 * Headroom (in bytes) the rest of the system keeps on the total heap once the
 * requested allocation is taken. Not applied to the largest free block, which
 * only has to hold the request itself.
 */
#ifndef PDI_SAFE_ALLOC_HEAP_MARGIN
#define PDI_SAFE_ALLOC_HEAP_MARGIN 2048
#endif

#ifndef PDI_SAFE_ALLOC_SMALL_REQUEST
#define PDI_SAFE_ALLOC_SMALL_REQUEST 64 /* requests this size or smaller skip the largest-block query, which walks the heap */
#endif

namespace pdiutil {

/**
 * @brief How many allocations the heap check has refused since boot.
 */
uint32_t safe_alloc_refusals();

/**
 * @brief Counts one refused allocation, for a caller that judged the heap itself.
 */
void note_refusal();

/**
 * @brief Arms the heap check. Must be called once the device instances are
 *        constructed; PDIStack::initialize() does this.
 */
void enable_heap_check();

/**
 * @brief Returns true if the device's heap can safely satisfy a request for
 *        `bytes`. Two conditions, measured separately: the largest free block
 *        has to hold `bytes`, and the total free heap has to keep
 *        PDI_SAFE_ALLOC_HEAP_MARGIN once `bytes` is taken. A port that cannot
 *        report either figure answers zero and that check is skipped.
 *
 * Always true before enable_heap_check(), because global constructors run
 * before the device instances they would query exist.
 */
bool has_heap_for(size_t bytes);

/**
 * @brief The most bytes one allocation could take right now under the same two
 *        checks as has_heap_for(); the largest size_t when the check is not armed or the port reports neither figure.
 */
size_t heap_room();

/**
 * @brief Heap-checked `new T[n]` for the most elements up to `expected` the heap can hold now, never fewer
 *        than `minimum`; `granted` carries the count taken, and nullptr with 0 when not even `minimum` fits.
 */
template <typename T>
inline T* safe_new_array_upto(size_t expected, size_t minimum, size_t &granted) {
    granted = 0;
    if (expected == 0) return nullptr;
    if (minimum == 0 || minimum > expected) minimum = expected;
    size_t room = heap_room() / sizeof(T);
    size_t count = (room < expected) ? room : expected;
    if (count < minimum) {
        note_refusal();
        return nullptr;
    }
    T* block = new (std::nothrow) T[count]();
    if (block == nullptr) {
        note_refusal();
        return nullptr;
    }
    granted = count;
    return block;
}

/**
 * @brief Heap-checked `new T(args...)`. Returns nullptr if the heap
 *        cannot safely satisfy the allocation, otherwise behaves exactly
 *        like a regular `new T(args...)`.
 *
 * Usage:
 *   MyType* p = pdiutil::safe_new<MyType>();
 *   MyType* p = pdiutil::safe_new<MyType>(arg1, arg2);
 */
template <typename T, typename... Args>
inline T* safe_new(Args&&... args) {
    if (!has_heap_for(sizeof(T))) return nullptr;
    T* made = new (std::nothrow) T(static_cast<Args&&>(args)...);
    if (made == nullptr) note_refusal();
    return made;
}

/**
 * @brief Builds a T from args in storage already taken, from safe_alloc_bytes()
 *        or a container's allocator, and returns it; nothing is allocated.
 */
template <typename T, typename... Args>
inline T* safe_construct(void* where, Args&&... args) {
    return new (where) T(static_cast<Args&&>(args)...);
}

/**
 * @brief Heap-checked `new T[count]`. Returns nullptr if count is zero or
 *        the heap cannot safely satisfy the allocation, otherwise returns
 *        a value-initialized (zeroed for scalars) array.
 */
template <typename T>
inline T* safe_new_array(size_t count) {
    if (count == 0) return nullptr;
    if (!has_heap_for(count * sizeof(T))) return nullptr;
    T* block = new (std::nothrow) T[count]();
    if (block == nullptr) note_refusal();
    return block;
}

/**
 * @brief Heap-checked raw storage of `bytes`, left uninitialised for a container to build elements in; nullptr if refused.
 */
inline void* safe_alloc_bytes(size_t bytes) {
    if (bytes == 0) return nullptr;
    if (!has_heap_for(bytes)) return nullptr;
    void* storage = ::operator new(bytes, std::nothrow);
    if (storage == nullptr) note_refusal();
    return storage;
}

/**
 * @brief Releases storage taken by safe_alloc_bytes(); nullptr is ignored.
 */
inline void safe_free_bytes(void* p) {
    if (p != nullptr) {
        ::operator delete(p);
    }
}

/**
 * @brief Null-safe `delete`. Takes the pointer by reference so the
 *        caller's variable is set to nullptr after the delete, preventing
 *        accidental reuse / double-delete.
 */
template <typename T>
inline void safe_delete(T*& p) {
    if (p != nullptr) {
        delete p;
        p = nullptr;
    }
}

/**
 * @brief Null-safe `delete[]` counterpart of safe_delete().
 */
template <typename T>
inline void safe_delete_array(T*& p) {
    if (p != nullptr) {
        delete[] p;
        p = nullptr;
    }
}

} // namespace pdiutil

#endif
