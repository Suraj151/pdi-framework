/****************************** PDI Semantic Types ****************************
This file is part of the PDI stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Central semantic typedefs mapped onto stdint fixed-width primitives.
Kept minimal and dependency-free so it can be included anywhere.

Author          : Suraj I.
Created Date    : 16th July 2026
******************************************************************************/

#ifndef _PDI_TYPES_H_
#define _PDI_TYPES_H_

#include <stdint.h>

namespace pdiutil {

    // Time
    using millis_t     = uint64_t; ///< millisecond timestamp / duration
    using epoch_time_t = int64_t;  ///< seconds since Unix epoch

    // Task scheduler
    using task_id_t       = int16_t; ///< task identity, -1 = invalid
    using task_priority_t = uint8_t; ///< task priority, 0 = lowest
    using attempts_t      = int16_t; ///< remaining attempts, -1 = unlimited, 0 = expired

    // Network
    using net_port_t = uint16_t; ///< TCP / UDP port number

} // namespace pdiutil

#endif // _PDI_TYPES_H_
