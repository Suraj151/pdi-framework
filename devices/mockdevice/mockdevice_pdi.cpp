/************************* Mock device Interface *****************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st Jan 2024
******************************************************************************/
#include <config/Config.h>

#ifdef MOCK_DEVICE_TEST

#include "mockdevice_pdi.h"

LoggerInterface __i_logger;
DatabaseInterface __i_db;
DeviceControlInterface __i_dvc_ctrl;
WiFiInterface __i_wifi;
WiFiClientInterface __i_wifi_client;
WiFiServerInterface __i_wifi_server;
NtpInterface __i_ntp;
PingInterface __i_ping;

#endif
