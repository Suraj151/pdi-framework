/******************************* SafeAlloc *************************************
This file is part of the PDI stack.

This is free software. You can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
Created Date    : 6th June 2026
******************************************************************************/

#include "SafeAlloc.h"
#include <interface/pdi.h>

namespace pdiutil {

// Lives in .bss, so it reads false throughout static initialization. Allocations
// made from global constructors run before __i_instance and __i_dvc_ctrl exist,
// and querying them there dispatches on an unconstructed vtable.
static bool s_heap_check_armed = false;

void enable_heap_check() {
    s_heap_check_armed = true;
}

bool has_heap_for(size_t bytes) {
    if (!s_heap_check_armed) return true;
    uint32_t free_heap = __i_instance.getUtilityInstance().get_free_heap();
    if (free_heap == 0) return true;
    return free_heap > (bytes + PDI_SAFE_ALLOC_HEAP_MARGIN);
}

} // namespace pdiutil
