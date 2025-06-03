/*************************** Device IOT Config page ***************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st June 2019
******************************************************************************/
#ifndef _DEVICE_IOT_CONFIG_H_
#define _DEVICE_IOT_CONFIG_H_

#include "Common.h"
#include <config/MqttConfig.h>


#define DEVICE_IOT_PACKET_VERSION      "1.0.0"

#define DEVICE_IOT_OTP_REQ_URL         "/api/fordevice/get-otp?mac_id=[mac]"
#define DEVICE_IOT_HOST_BUF_SIZE       50
#define DEVICE_IOT_OTP_KEY             "otp"
#define DEVICE_IOT_OTP_STATUS_KEY      "status"
#define DEVICE_IOT_OTP_LENGTH          10
#define DEVICE_IOT_OTP_API_RESP_LENGTH 100

#define SENSOR_DATA_PUBLISH_FREQ            300	// every 300 seconds
#define SENSOR_DATA_SAMPLING_PER_PUBLISH		10  // 10 samples for each publish to average and send feature
#define SENSOR_DATA_PUBLISH_FREQ_MIN_LIMIT	2
#define SENSOR_DATA_PUBLISH_FREQ_MAX_LIMIT	3600
#define SENSOR_DATA_SAMPLES_PER_PUBLISH_MAX_BUFF	100
#define SENSOR_DATA_SAMPLES_PER_PUBLISH_MAX_LIMIT	60

#define DEVICE_IOT_CONFIG_REQ_URL             "/api/fordevice/get-config?mac_id=[mac]"
#define DEVICE_IOT_ALERTS_REQ_URL             "/api/fordevice/get-alerts?mac_id=[mac]"
#define DEVICE_IOT_CONFIG_RESP_MAX_SIZE       300
#define DEVICE_IOT_CONFIG_TOKEN_KEY           "token"
#define DEVICE_IOT_CONFIG_TOKEN_MAX_SIZE      DEVICE_IOT_OTP_API_RESP_LENGTH
#define DEVICE_IOT_CONFIG_TOPIC_KEY           "topic"
#define DEVICE_IOT_CONFIG_TOPIC_MAX_SIZE      DEVICE_IOT_HOST_BUF_SIZE
#define DEVICE_IOT_CONFIG_DATA_RATE_KEY       "data_rate"
#define DEVICE_IOT_CONFIG_SAMPLING_RATE_KEY   "sample_rate"
#define DEVICE_IOT_CONFIG_MQTT_KEEP_ALIVE_KEY "keep_alive"
#define DEVICE_IOT_MQTT_KEEP_ALIVE_MIN        10
#define DEVICE_IOT_MQTT_KEEP_ALIVE_MAX        600

#define DEVICE_IOT_MQTT_DATA_HOST             "192.168.0.100"
#define DEVICE_IOT_MQTT_DATA_PORT             1883
#define DEVICE_IOT_MQTT_WILL_TOPIC            "disconnect"

struct device_iot_configs {
  
  // Default Constructor
  device_iot_configs(){
    clear();
  }

  // Clear members method
  void clear(){
    memset(device_iot_host, 0, DEVICE_IOT_HOST_BUF_SIZE);
    memset(device_iot_token, 0, DEVICE_IOT_CONFIG_TOKEN_MAX_SIZE);
    memset(device_iot_topic, 0, DEVICE_IOT_CONFIG_TOPIC_MAX_SIZE);
  }

  char device_iot_host[DEVICE_IOT_HOST_BUF_SIZE];
  char device_iot_token[DEVICE_IOT_CONFIG_TOKEN_MAX_SIZE];
  char device_iot_topic[DEVICE_IOT_CONFIG_TOPIC_MAX_SIZE];
};

// const device_iot_configs PROGMEM _device_iot_config_defaults = {
//   {0},{0},{0}
// };

const int device_iot_config_size = sizeof(device_iot_configs) + 5;

using device_iot_config_table = device_iot_configs;

#endif
