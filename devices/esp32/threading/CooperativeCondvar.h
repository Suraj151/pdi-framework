/************************** Cooperative Condvar *******************************
This file is part of the PDI stack.

This is free software. You can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
Created Date    : 1st Jan 2026
******************************************************************************/

#ifndef __ESP32_COOPERATIVE_CONDVAR_H__
#define __ESP32_COOPERATIVE_CONDVAR_H__

#include "../esp32.h"
#include <interface/pdi/threading/iCondvar.h>
#include "Cooperative.h"

class CooperativeConditionVar : public iConditionVar {
    pdiutil::vector<Cooperative*> m_waiters;
public:
    virtual ~CooperativeConditionVar() { m_waiters.clear(); }
    void wait(iMutex& mtx) override;
    void notify_one() override;
    void notify_all() override {}
};

#endif
