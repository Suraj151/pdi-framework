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

    iUtilityInterface &utility = __i_instance.getUtilityInstance();

    // malloc needs the bytes in one piece, so the largest free block has to hold
    // the request itself. A port that cannot report it answers zero.
    uint32_t largest_block = utility.get_max_free_block();
    if (largest_block != 0 && largest_block < bytes) {
        return false;
    }

    // the margin is headroom the rest of the system keeps once this request is
    // taken, so it belongs to the total heap and not to the block it comes from
    uint32_t free_heap = utility.get_free_heap();
    if (free_heap != 0 && free_heap < (bytes + PDI_SAFE_ALLOC_HEAP_MARGIN)) {
        return false;
    }

    return true;
}

} // namespace pdiutil
