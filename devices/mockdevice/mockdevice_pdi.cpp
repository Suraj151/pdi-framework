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
#ifdef ENABLE_WIFI_SERVICE
WiFiInterface __i_wifi;
ClientInterface __i_wifi_client;
ServerInterface __i_wifi_server;
#endif
#ifdef ENABLE_NETWORK_SERVICE
NtpInterface __i_ntp;
PingInterface __i_ping;
#endif

#endif
