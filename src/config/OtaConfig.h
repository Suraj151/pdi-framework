/****************************** OTA Config page *******************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st June 2019
******************************************************************************/
#ifndef _OTA_CONFIG_H_
#define _OTA_CONFIG_H_

#include "Common.h"

#define OTA_HOST_BUF_SIZE           50
#define OTA_VERSION_KEY             "latest"
#define OTA_VERSION_LENGTH          15
#define OTA_VERSION_API_RESP_LENGTH OTA_HOST_BUF_SIZE
#define OTA_API_CHECK_DURATION      15000

#define OTA_VERSION_CHECK_URL       "/api/fordevice/ota-version?mac_id=[mac]"
#define OTA_BINARY_DOWNLOAD_URL     "/api/fordevice/ota-bin?mac_id=[mac]&version="

/**
 * enable/disable ota config modification here
 */
#define ALLOW_OTA_CONFIG_MODIFICATION

struct ota_configs {

  // Default Constructor
  ota_configs(){
    clear();
  }

  // Clear members method
  void clear(){
    memset(ota_host, 0, OTA_HOST_BUF_SIZE);
    ota_port = 80;
  }

  char ota_host[OTA_HOST_BUF_SIZE];
  int ota_port;
};

// const ota_configs PROGMEM _ota_config_defaults = {
//   {0}, 80
// };

const int ota_config_size = sizeof(ota_configs) + 5;

using ota_config_table = ota_configs;

#endif
