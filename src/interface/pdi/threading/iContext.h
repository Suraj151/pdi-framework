/***************************** Context Interface *****************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st Jan 2024
******************************************************************************/

#ifndef _I_THREADING_CONTEXT_INTERFACE_H_
#define _I_THREADING_CONTEXT_INTERFACE_H_

#include <interface/interface_includes.h>

// Abstract execution context (fiber, RTOS task, thread)
class iContext {
public:
    virtual ~iContext() {}
    virtual void save(void* out) = 0;       // save current execution state
    virtual void restore(const void* in) = 0;    // restore previously saved state
};


#endif