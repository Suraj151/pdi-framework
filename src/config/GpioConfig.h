/***************************** GPIO Config page *******************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st June 2019
******************************************************************************/
#ifndef _GPIO_CONFIG_H_
#define _GPIO_CONFIG_H_

#include "Common.h"

#if !defined(MAX_DIGITAL_GPIO_PINS)
#define MAX_DIGITAL_GPIO_PINS         9
#endif
#if !defined(MAX_ANALOG_GPIO_PINS)
#define MAX_ANALOG_GPIO_PINS          1
#endif

#define GPIO_HOST_BUF_SIZE            60
#define ANALOG_GPIO_RESOLUTION        1024
#define GPIO_GRAPH_ADJ_POINT_DISTANCE 10
#define GPIO_MAX_GRAPH_WIDTH          300
#define GPIO_MAX_GRAPH_HEIGHT         150
#define GPIO_GRAPH_TOP_MARGIN         25
#define GPIO_GRAPH_BOTTOM_MARGIN      GPIO_GRAPH_TOP_MARGIN
#define GPIO_DATA_POST_FREQ           0
#define GPIO_DATA_POST_HTTP_URL       "/api/post/gpiodata"
#define GPIO_ALERT_POST_HTTP_URL      "/api/post/gpioalert"

#define GPIO_ALERT_DURATION_FOR_SUCCEED 3600000
#define GPIO_ALERT_DURATION_FOR_FAILED  300000

/**
 * @define gpio parameters
 */
#define GPIO_OPERATION_DURATION MILLISECOND_DURATION_1000
#define GPIO_TABLE_UPDATE_DURATION 300000

#ifndef ENABLE_GPIO_BASIC_ONLY
/**
 * global gpio alert status
 */
typedef struct {
  bool is_last_alert_succeed;
  uint32_t last_alert_millis;
  int8_t alert_gpio_pin;
} __gpio_alert_track_t;

extern __gpio_alert_track_t __gpio_alert_track;

#endif

struct last_gpio_monitor_point{
  int x;
  int y;
};

enum GPIO_CONFIG_TYPE {
  GPIO_MODE_CONFIG,
  GPIO_WRITE_CONFIG,
  GPIO_SERVER_CONFIG,
  GPIO_ALERT_CONFIG,
};

enum GPIO_STATE {
  GPIO_STATE_LOW=0,
  GPIO_STATE_HIGH=1,
  GPIO_STATE_MAX
};

enum GPIO_MODE {
  OFF=0,
  DIGITAL_WRITE,
  DIGITAL_READ,
  DIGITAL_BLINK,
  ANALOG_WRITE,
  ANALOG_READ,
  GPIO_MODE_MAX
};

enum GPIO_ALERT_COMPARATOR {
  EQUAL=0,
  GREATER_THAN,
  LESS_THAN,
  GPIO_ALERT_COMPARATOR_MAX
};

enum GPIO_ALERT_CHANNEL {
  NO_ALERT=0,
  EMAIL,
  HTTP_SERVER,
  GPIO_ALERT_CHANNEL_MAX
};

struct gpio_configs {

  // Default Constructor
  gpio_configs(){
    clear();
  }

  // Clear members method
  void clear(){
    memset(gpio_mode, OFF, MAX_DIGITAL_GPIO_PINS+MAX_ANALOG_GPIO_PINS);
    memset(gpio_readings, 0, MAX_DIGITAL_GPIO_PINS+MAX_ANALOG_GPIO_PINS);
#ifndef ENABLE_GPIO_BASIC_ONLY
    memset(gpio_alert_comparator, EQUAL, MAX_DIGITAL_GPIO_PINS+MAX_ANALOG_GPIO_PINS);
    memset(gpio_alert_channel, NO_ALERT, MAX_DIGITAL_GPIO_PINS+MAX_ANALOG_GPIO_PINS);
    memset(gpio_alert_values, 0, MAX_DIGITAL_GPIO_PINS+MAX_ANALOG_GPIO_PINS);
    memset(gpio_host, 0, GPIO_HOST_BUF_SIZE);
    gpio_port = 80;
    gpio_post_frequency = GPIO_DATA_POST_FREQ;
#endif
  }

  uint8_t gpio_mode[MAX_DIGITAL_GPIO_PINS+MAX_ANALOG_GPIO_PINS];
  uint16_t gpio_readings[MAX_DIGITAL_GPIO_PINS+MAX_ANALOG_GPIO_PINS];
#ifndef ENABLE_GPIO_BASIC_ONLY
  uint8_t gpio_alert_comparator[MAX_DIGITAL_GPIO_PINS+MAX_ANALOG_GPIO_PINS];
  uint8_t gpio_alert_channel[MAX_DIGITAL_GPIO_PINS+MAX_ANALOG_GPIO_PINS];
  uint16_t gpio_alert_values[MAX_DIGITAL_GPIO_PINS+MAX_ANALOG_GPIO_PINS];
  char gpio_host[GPIO_HOST_BUF_SIZE];
  int gpio_port;
  int gpio_post_frequency;
#endif
};

// const gpio_configs PROGMEM _gpio_config_defaults = {
//   {OFF}, {0}, {EQUAL}, {NO_ALERT}, {0}, "", 80, GPIO_DATA_POST_FREQ
// };

const int gpio_config_size = sizeof(gpio_configs) + 5;

using gpio_config_table = gpio_configs;

#endif
