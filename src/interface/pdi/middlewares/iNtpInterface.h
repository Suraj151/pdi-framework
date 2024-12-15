/************************ i network time Interface ****************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st June 2019
******************************************************************************/

#ifndef _I_NTP_INTERFACE_H_
#define _I_NTP_INTERFACE_H_

#include <interface/interface_includes.h>

#ifndef TZ_Asia_Kolkata
#define TZ_Asia_Kolkata	"IST-5:30"
#endif

#define TZ              5.5       // (utc+) TZ in hours
#define TZ_MN           ((TZ)*60)
#define TZ_SEC          ((TZ)*3600)
#define DST_MN          0      // use 60mn for summer time in some countries
#define DST_SEC         ((DST_MN)*60)

/**
 * NTP servers
 */
#define NTP_SERVER1     "pool.ntp.org"

// forward declaration of derived class for this interface
class NtpInterface;

/**
 * iNtpInterface class
 */
class iNtpInterface
{

public:
  /**
   * iNtpInterface constructor.
   */
  iNtpInterface() {}
  /**
   * iNtpInterface destructor.
   */
  virtual ~iNtpInterface() {}

  virtual void init_ntp_time() = 0;
  virtual bool is_valid_ntptime() = 0;
  virtual long get_ntp_time() = 0;
};

// derived class must define this
extern NtpInterface __i_ntp;

#endif
