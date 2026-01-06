/******************************* Fiber Condvar ********************************
This file is part of the PDI stack.

This is free software. You can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
Created Date    : 1st June 2025
******************************************************************************/

#ifndef __ESP866_FIBER_CONDVAR_H__
#define __ESP866_FIBER_CONDVAR_H__

#include "../esp8266.h"
#include <interface/pdi/threading/iCondvar.h>
#include "Fiber.h"

class FiberConditionVar : public iConditionVar {
    pdiutil::vector<Fiber*> m_waiters; 
public:
    virtual ~FiberConditionVar() { m_waiters.clear(); }
    void wait(iMutex& mtx) override;
    void notify_one() override;
    void notify_all() override {}
};


#endif
