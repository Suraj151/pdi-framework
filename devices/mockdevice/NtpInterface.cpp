/************************ N/W Time Protocol Interface *************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 16th Aug 2026
******************************************************************************/

#include "NtpInterface.h"
#include <time.h>

NtpInterface::NtpInterface() : m_epoch(0)
{
}

NtpInterface::~NtpInterface()
{
}

void NtpInterface::init_ntp_time()
{
    m_epoch = (pdiutil::epoch_time_t)time(nullptr);
}

bool NtpInterface::is_valid_ntptime()
{
    return m_epoch > 0;
}

pdiutil::epoch_time_t NtpInterface::get_ntp_time()
{
    return m_epoch;
}

bool NtpInterface::set_ntp_time(pdiutil::epoch_time_t epoch)
{
    m_epoch = epoch;
    return true;
}

void NtpInterface::clearTime()
{
    m_epoch = 0;
}

NtpInterface __i_ntp;
