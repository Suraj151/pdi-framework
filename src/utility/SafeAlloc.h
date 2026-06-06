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

/**
 * Safety margin (in bytes) retained on the heap beyond the requested
 * allocation.
 */
#ifndef PDI_SAFE_ALLOC_HEAP_MARGIN
#define PDI_SAFE_ALLOC_HEAP_MARGIN 2048
#endif

namespace pdiutil {

/**
 * @brief Returns true if the device's free heap can safely satisfy a
 *        request for `bytes` plus PDI_SAFE_ALLOC_HEAP_MARGIN.
 */
bool has_heap_for(size_t bytes);

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
    return new T(static_cast<Args&&>(args)...);
}

/**
 * @brief Heap-checked `new T[count]`. Returns nullptr if count is zero or
 *        the heap cannot safely satisfy the allocation, otherwise returns
 *        a default-initialized array.
 */
template <typename T>
inline T* safe_new_array(size_t count) {
    if (count == 0) return nullptr;
    if (!has_heap_for(count * sizeof(T))) return nullptr;
    return new T[count];
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
