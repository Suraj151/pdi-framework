/***************************** MQTT Config page *******************************
This file is part of the Ewings Esp Stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st June 2019
******************************************************************************/
#ifndef _MQTT_CONFIG_H_
#define _MQTT_CONFIG_H_

#include "Common.h"

#define MQTT_HOST_BUF_SIZE      50
#define MQTT_CLIENT_ID_BUF_SIZE 100
#define MQTT_DEFAULT_KEEPALIVE  30      /*second*/
#define MQTT_HOST_CONNECT_TIMEOUT  5    /*second*/
#define MQTT_DEFAULT_PORT       1883
#define MQTT_USERNAME_BUF_SIZE  25
#define MQTT_PASSWORD_BUF_SIZE  500
#define MQTT_TOPIC_BUF_SIZE     MQTT_HOST_BUF_SIZE
#define MQTT_WILL_MSG_BUF_SIZE  80

#define MQTT_MAX_QOS_LEVEL      2
#define MQTT_MAX_PUBLISH_TOPIC  2
#define MQTT_MAX_SUBSCRIBE_TOPIC  MQTT_MAX_PUBLISH_TOPIC

#define MQTT_INITIALIZE_DURATION   MILLISECOND_DURATION_5000

/**
 * enable/disable mqtt default payload for publish if user not assigned explicitely
 */
#ifndef ENABLE_DEVICE_IOT
#define ENABLE_MQTT_DEFAULT_PAYLOAD
#endif
/**
 * enable/disable mqtt config modification here
 */
#define ALLOW_MQTT_CONFIG_MODIFICATION

enum MQTT_CONFIG_TYPE {
  MQTT_GENERAL_CONFIG,
  MQTT_LWT_CONFIG,
  MQTT_PUBSUB_CONFIG,
};

typedef struct mqtt_sub_topics{

  // Default Constructor
  mqtt_sub_topics(){
    clear();
  }

  // Clear members method
  void clear(){
    memset(topic, 0, MQTT_TOPIC_BUF_SIZE);
    qos = 0;
  }

  char topic[MQTT_TOPIC_BUF_SIZE];
  uint8_t qos;
} mqtt_sub_topics_t;

typedef struct mqtt_pub_topics{

  // Default Constructor
  mqtt_pub_topics(){
    clear();
  }

  // Clear members method
  void clear(){
    memset(topic, 0, MQTT_TOPIC_BUF_SIZE);
    qos = 0;
    retain = 0;
  }

  char topic[MQTT_TOPIC_BUF_SIZE];
  uint8_t qos;
  uint8_t retain;
} mqtt_pub_topics_t;

struct mqtt_general_configs {

  // Default Constructor
  mqtt_general_configs(){
    clear();
  }

  // Clear members method
  void clear(){
    memset(host, 0, MQTT_HOST_BUF_SIZE);
    port = MQTT_DEFAULT_PORT;
    security = 0;
    memset(client_id, 0, MQTT_CLIENT_ID_BUF_SIZE);
    memset(username, 0, MQTT_USERNAME_BUF_SIZE);
    memset(password, 0, MQTT_PASSWORD_BUF_SIZE);
    keepalive = MQTT_DEFAULT_KEEPALIVE;
    clean_session = 1;
  }

  char host[MQTT_HOST_BUF_SIZE];
  int port;
  int security;
  char client_id[MQTT_CLIENT_ID_BUF_SIZE];
  char username[MQTT_USERNAME_BUF_SIZE];
  char password[MQTT_PASSWORD_BUF_SIZE];
  int keepalive;
  int clean_session;
};

struct mqtt_lwt_configs {

  // Default Constructor
  mqtt_lwt_configs(){
    clear();
  }

  // Clear members method
  void clear(){
    memset(will_topic, 0, MQTT_TOPIC_BUF_SIZE);
    memset(will_message, 0, MQTT_WILL_MSG_BUF_SIZE);
    will_qos = 0;
    will_retain = 0;
  }

  char will_topic[MQTT_TOPIC_BUF_SIZE];
  char will_message[MQTT_WILL_MSG_BUF_SIZE];
  int will_qos;
  int will_retain;
};

struct mqtt_pubsub_configs {

  // Default Constructor
  mqtt_pubsub_configs(){
    clear();
  }

  // Clear members method
  void clear(){
    publish_frequency = 0;
  }

  mqtt_pub_topics_t publish_topics[MQTT_MAX_PUBLISH_TOPIC];
  mqtt_sub_topics_t subscribe_topics[MQTT_MAX_SUBSCRIBE_TOPIC];
  int publish_frequency;
};

// const mqtt_sub_topics_t PROGMEM _mqtt_sub_topic_defaults = {
//   "", 0
// };

// const mqtt_pub_topics_t PROGMEM _mqtt_pub_topic_defaults = {
//   "", 0, 0
// };

// const mqtt_general_configs PROGMEM _mqtt_general_config_defaults = {
//   "", MQTT_DEFAULT_PORT, 0, "", "", "", MQTT_DEFAULT_KEEPALIVE, 1
// };

// const mqtt_lwt_configs PROGMEM _mqtt_lwt_config_defaults = {
//   "", "", 0, 0
// };

// const mqtt_pubsub_configs PROGMEM _mqtt_pubsub_config_defaults = {
//   {_mqtt_pub_topic_defaults}, {_mqtt_sub_topic_defaults}, 0
// };

// const mqtt_pubsub_configs PROGMEM _mqtt_pubsub_config_defaults = {
//   NULL, NULL, 0
// };


const int mqtt_general_config_size = sizeof(mqtt_general_configs) + 5;
const int mqtt_lwt_config_size     = sizeof(mqtt_lwt_configs) + 5;
const int mqtt_pubsub_config_size  = sizeof(mqtt_pubsub_configs) + 5;

using mqtt_general_config_table = mqtt_general_configs;
using mqtt_lwt_config_table = mqtt_lwt_configs;
using mqtt_pubsub_config_table = mqtt_pubsub_configs;


#endif
