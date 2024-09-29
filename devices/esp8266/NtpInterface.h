/************************ N/W Time Protocol Interface *************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st June 2019
******************************************************************************/

#ifndef _NTP_SERVICE_PROVIDER_H_
#define _NTP_SERVICE_PROVIDER_H_

#include "esp8266.h"
#include <interface/pdi/middlewares/iNtpInterface.h>


/**
 * NtpInterface class
 */
class NtpInterface : public iNtpInterface {

  public:

    /**
     * NtpInterface constructor.
     */
    NtpInterface();

    /**
		 * NtpInterface destructor
		 */
    ~NtpInterface();

    void init_ntp_time();
    bool is_valid_ntptime();
    time_t get_ntp_time();
};

#endif
