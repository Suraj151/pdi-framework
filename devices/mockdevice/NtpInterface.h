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
 */
class NtpInterface : public iNtpInterface {

  public:

    /**
     * NtpInterface constructor.
     */
    NtpInterface(){}

    /**
		 * NtpInterface destructor
		 */
    ~NtpInterface(){}

    void init_ntp_time() override{}
    bool is_valid_ntptime() override{return 0;}
    long get_ntp_time() override{return 0;}
};

#endif
