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

bool has_heap_for(size_t bytes) {
    uint32_t free_heap = __i_instance.getUtilityInstance().get_free_heap();
    if (free_heap == 0) return true;
    return free_heap > (bytes + PDI_SAFE_ALLOC_HEAP_MARGIN);
}

} // namespace pdiutil
