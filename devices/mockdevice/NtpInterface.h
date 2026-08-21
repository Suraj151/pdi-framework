/************************ N/W Time Protocol Interface *************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st June 2019
******************************************************************************/

#ifndef _MOCKDEVICE_NTP_SERVICE_PROVIDER_H_
#define _MOCKDEVICE_NTP_SERVICE_PROVIDER_H_

#include "mockdevice.h"
#include <interface/pdi/middlewares/iNtpInterface.h>

/**
 * NtpInterface class
 *
 * Starts unsynced the way a freshly booted device does, so nothing reads a
 * wall clock time before something has supplied one. init_ntp_time takes the
 * host clock, and a caller can set an exact epoch instead when a test needs
 * timestamps to be reproducible.
 */
class NtpInterface : public iNtpInterface
{

public:
  NtpInterface();
  ~NtpInterface();

  void init_ntp_time() override;
  bool is_valid_ntptime() override;
  pdiutil::epoch_time_t get_ntp_time() override;
  bool set_ntp_time(pdiutil::epoch_time_t epoch) override;

  /**
   * @brief Return to the unsynced state.
   */
  void clearTime();

private:
  pdiutil::epoch_time_t m_epoch;
};

#endif
