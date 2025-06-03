/************************ N/W Time Protocol Interface *************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st June 2019
******************************************************************************/

#include "NtpInterface.h"

/**
 * Set sntp update delay by defining weak function in lwip arduino build
 * default delay of 1hour is used we can change it here and should be > 15 seconds
 */
uint32_t sntp_update_delay_MS_rfc_not_less_than_15000()
{
  return 120000; // 120 seconds
}

/**
 * NtpInterface constructor.
 */
NtpInterface::NtpInterface()
{
  this->init_ntp_time();
}

/**
 * NtpInterface destructor
 */
NtpInterface::~NtpInterface()
{
}

/**
 * initialize network time
 */
void NtpInterface::init_ntp_time()
{
  configTime(TZ_SEC, DST_SEC, NTP_SERVER1);
  // configTime( TZ_Asia_Kolkata, NTP_SERVER1 );
}

/**
 * check whether ntp time is valid
 */
bool NtpInterface::is_valid_ntptime()
{
  return (time(nullptr) > LAUNCH_UNIX_TIME);
}

/**
 * get network time epoch time
 *
 * @return  time_t
 */
long NtpInterface::get_ntp_time()
{
  return time(nullptr);
}

NtpInterface __i_ntp;
