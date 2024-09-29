/*************************** ESPNOW Config page *******************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st June 2019
******************************************************************************/
#ifndef _ESPNOW_CONFIG_H_
#define _ESPNOW_CONFIG_H_

#include <config/Common.h>

// #define ESP_NOW_KEY                  "jo bole so nihal"
#define ESP_NOW_KEY                     "lets kill people"
#define ESP_NOW_KEY_LENGTH              16
#define ESP_NOW_CHANNEL                 4
#define ESP_NOW_MAX_PEER                8
#define ESP_NOW_DEVICE_TABLE_MAX_SIZE   20
#define ESP_NOW_MAX_BUFF_SIZE           250

#define ESP_NOW_HANDLE_DURATION         MILLISECOND_DURATION_5000

enum esp_now_state {
  ESP_NOW_STATE_EMPTY,
  ESP_NOW_STATE_INIT,
  ESP_NOW_STATE_SENT,
	ESP_NOW_STATE_SENT_SUCCEED,
  ESP_NOW_STATE_SENT_FAILED,
  ESP_NOW_STATE_DATA_AVAILABLE,
  ESP_NOW_STATE_RECV_AVAILABLE
};

typedef struct {
  uint8_t mac[6];
  uint8_t mesh_level;
} esp_now_device_t;

typedef struct {
  uint8_t mac[6];
  uint8_t role;
  uint8_t channel;
  esp_now_state state;
  uint8_t* buffer;
  uint8_t data_length;
  uint32_t last_receive;
} esp_now_peer_t;

extern esp_now_peer_t esp_now_peers[ESP_NOW_MAX_PEER];
extern std::vector<esp_now_device_t> esp_now_device_table;

#endif
