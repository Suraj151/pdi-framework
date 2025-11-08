/***************************** WiFi Config page *******************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st June 2019
******************************************************************************/
#ifndef _WIFI_CONFIG_H_
#define _WIFI_CONFIG_H_

#include "Common.h"

#define WIFI_CONFIGS_BUF_SIZE 30

#define DEFAULT_USERNAME     USER
#define DEFAULT_PASSWORD     PASSPHRASE

#define DEFAULT_SSID         USER
#define DEFAULT_PASSPHRASE   PASSPHRASE

const uint8_t DEFAULT_DNS_IP   [] = {8, 8, 8, 8};

const uint8_t DEFAULT_STA_LOCAL_IP [] = {0, 0, 0, 0};
const uint8_t DEFAULT_STA_GATEWAY  [] = {0, 0, 0, 0};
const uint8_t DEFAULT_STA_SUBNET   [] = {0, 0, 0, 0};

const uint8_t DEFAULT_AP_LOCAL_IP  [] = {192, 168, 0, 1};
const uint8_t DEFAULT_AP_GATEWAY   [] = {192, 168, 0, 1};
const uint8_t DEFAULT_AP_SUBNET    [] = {255, 255, 255, 0};

/**
 * enable/disable wifi config modification here
 */
#define ALLOW_WIFI_CONFIG_MODIFICATION
#define ALLOW_WIFI_SSID_PASSKEY_CONFIG_MODIFICATION_ONLY

/**
 * global wifi status
 */
typedef struct {
  bool wifi_connected;
  bool internet_available;
  uint32_t last_internet_millis;
  uint8_t ignore_bssid[6];
} __status_wifi_t;

extern __status_wifi_t __status_wifi;

struct wifi_configs {

  // Default Constructor
  wifi_configs(){
    clear();
  }

  // Clear members method
  void clear(){
    memset(sta_ssid, 0, WIFI_CONFIGS_BUF_SIZE);
    memset(sta_password, 0, WIFI_CONFIGS_BUF_SIZE);
    memset(ap_ssid, 0, WIFI_CONFIGS_BUF_SIZE);
    memset(ap_password, 0, WIFI_CONFIGS_BUF_SIZE);

    memcpy(sta_ssid, DEFAULT_SSID, sizeof(DEFAULT_SSID));
    memcpy(sta_password, DEFAULT_PASSPHRASE, sizeof(DEFAULT_PASSPHRASE));
    memcpy(ap_ssid, DEFAULT_USERNAME, sizeof(DEFAULT_USERNAME));
    memcpy(ap_password, DEFAULT_PASSWORD, sizeof(DEFAULT_PASSWORD));

    memcpy(sta_local_ip, DEFAULT_STA_LOCAL_IP, 4);
    memcpy(sta_gateway, DEFAULT_STA_GATEWAY, 4);
    memcpy(sta_subnet, DEFAULT_STA_SUBNET, 4);

    memcpy(ap_local_ip, DEFAULT_AP_LOCAL_IP, 4);
    memcpy(ap_gateway, DEFAULT_AP_GATEWAY, 4);
    memcpy(ap_subnet, DEFAULT_AP_SUBNET, 4);
  }

  char sta_ssid[WIFI_CONFIGS_BUF_SIZE];
  char sta_password[WIFI_CONFIGS_BUF_SIZE];
  char ap_ssid[WIFI_CONFIGS_BUF_SIZE];
  char ap_password[WIFI_CONFIGS_BUF_SIZE];

  uint8_t sta_local_ip[4];
  uint8_t sta_gateway[4];
  uint8_t sta_subnet[4];

  uint8_t ap_local_ip[4];
  uint8_t ap_gateway[4];
  uint8_t ap_subnet[4];
};

// const wifi_configs PROGMEM _wifi_config_defaults = {
//   DEFAULT_SSID, DEFAULT_PASSPHRASE, DEFAULT_USERNAME, DEFAULT_PASSWORD,
//   DEFAULT_STA_LOCAL_IP, DEFAULT_STA_GATEWAY, DEFAULT_STA_SUBNET,
//   DEFAULT_AP_LOCAL_IP, DEFAULT_AP_GATEWAY, DEFAULT_AP_SUBNET
// };

const int wifi_config_size = sizeof(wifi_configs) + 5;

using wifi_config_table = wifi_configs;

#endif
