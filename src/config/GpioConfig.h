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

#if !defined(MAX_GPIO_PINS)
#define MAX_GPIO_PINS                 MAX_DIGITAL_GPIO_PINS + MAX_ANALOG_GPIO_PINS // Assuming analog and digital pins are not same
#endif

#if !defined(MAX_GPIO_EVENTS)
#define MAX_GPIO_EVENTS               8
#endif

#define INVALID_GPIO_NUMBER       UINT8_MAX

#define GPIO_HOST_BUF_SIZE            60
#define ANALOG_GPIO_RESOLUTION        1024

#define GPIO_PAYLOAD_DATA_KEY     "data"
#define GPIO_PAYLOAD_MODE_KEY     "mode"
#define GPIO_PAYLOAD_VALUE_KEY    "val"
#define GPIO_PAYLOAD_MAC_KEY      "mac"
#define GPIO_PAYLOAD_DUID_KEY     "duid"
#define GPIO_ALERT_COMPARATOR_KEY "cmp"
#define GPIO_ALERT_PIN_KEY        "alrtkey"

#ifndef ENABLE_GPIO_BASIC_ONLY

#define GPIO_GRAPH_ADJ_POINT_DISTANCE 10
#define GPIO_MAX_GRAPH_WIDTH          300
#define GPIO_MAX_GRAPH_HEIGHT         150
#define GPIO_GRAPH_TOP_MARGIN         25
#define GPIO_GRAPH_BOTTOM_MARGIN      GPIO_GRAPH_TOP_MARGIN
#define GPIO_DATA_POST_FREQ           0
#define GPIO_DATA_POST_HTTP_URL       "/api/fordevice/data/[duid]"
#define GPIO_ALERT_POST_HTTP_URL      "/api/fordevice/event/[duid]"

#define GPIO_ALERT_DURATION_FOR_SUCCEED 600000
#define GPIO_ALERT_DURATION_FOR_FAILED  150000

/**
 * global gpio alert status
 */
typedef struct {
  bool is_last_alert_succeed;
  uint32_t last_alert_millis;
  int8_t alert_gpio_pin;
} __gpio_alert_track_t;

extern __gpio_alert_track_t __gpio_alert_track;

struct last_gpio_monitor_point{
  int x;
  int y;
};

#endif


/**
 * @define gpio parameters
 */
#define GPIO_OPERATION_DURATION MILLISECOND_DURATION_1000
#define GPIO_TABLE_UPDATE_DURATION 300000


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

enum GPIO_EVENT_CONDITION {
  EQUAL=0,
  GREATER_THAN,
  LESS_THAN,
  GPIO_EVENT_CONDITION_MAX
};

enum GPIO_EVENT_CHANNEL {
  NO_CHANNEL,
#ifdef ENABLE_EMAIL_SERVICE
  EMAIL,
#endif
  HTTP_SERVER,
  GPIO_EVENT_CHANNEL_MAX
};

#ifndef ENABLE_GPIO_BASIC_ONLY
struct gpio_event {
  uint8_t gpioNumber;
  uint8_t eventCondition;
  uint8_t eventChannel;
  uint16_t eventConditionValue;

  // Default Constructor
  gpio_event(){
    clear();
  }

  // Clear members method
  void clear(){

    gpioNumber = INVALID_GPIO_NUMBER;
    eventCondition = GPIO_EVENT_CONDITION_MAX;
    eventChannel = GPIO_EVENT_CHANNEL_MAX;
    eventConditionValue = 0;
  }

  // Return whether event occured or not
  bool isEventOccur(uint16_t value){

    bool _is_condition = false;

    switch ( eventCondition ) {

      case EQUAL:{
        _is_condition = ( value == eventConditionValue );
        break;
      }
      case GREATER_THAN:{
        _is_condition = ( value > eventConditionValue );
        break;
      }
      case LESS_THAN:{
        _is_condition = ( value < eventConditionValue );
        break;
      }
      default: break;
    }

    return _is_condition;
  }
};
#endif

struct gpio_configs {

  // Default Constructor
  gpio_configs(){
    clear();
  }

  // Clear members method
  void clear(){
    memset(gpio_mode, OFF, MAX_GPIO_PINS);
    memset(gpio_readings, 0, MAX_GPIO_PINS);
#ifndef ENABLE_GPIO_BASIC_ONLY
    for (size_t i = 0; i < MAX_GPIO_EVENTS; i++) gpio_events[i].clear();
    memset(gpio_host, 0, GPIO_HOST_BUF_SIZE);
    gpio_port = 80;
    gpio_post_frequency = GPIO_DATA_POST_FREQ;
#endif
  }

  uint8_t gpio_mode[MAX_GPIO_PINS];
  uint16_t gpio_readings[MAX_GPIO_PINS];
#ifndef ENABLE_GPIO_BASIC_ONLY
  gpio_event gpio_events[MAX_GPIO_EVENTS];
  char gpio_host[GPIO_HOST_BUF_SIZE];
  int gpio_port;
  int gpio_post_frequency;

  // return true if gpio has event
  bool gpioHasEvents(uint8_t gpionumber, uint8_t condition = GPIO_EVENT_CONDITION_MAX){

    return -1 != getGpioEventIndex(gpionumber, condition);
  }

  // return event if found
  bool getGpioEvent(gpio_event &gpioevent){

    int16_t eventidx = getGpioEventIndex(gpioevent.gpioNumber, gpioevent.eventCondition);

    if( eventidx != -1 && eventidx < MAX_GPIO_EVENTS ){

      gpioevent.eventChannel = gpio_events[eventidx].eventChannel;
      gpioevent.eventCondition = gpio_events[eventidx].eventCondition;
      gpioevent.eventConditionValue = gpio_events[eventidx].eventConditionValue;
      return true;
    }

    return false;
  }

  // update or add gpio event
  bool updateGpioEvent( uint8_t gpionumber, uint8_t channel, uint8_t condition, uint16_t value, bool addforce = false ){

    int16_t eventidx = getGpioEventIndex(gpionumber, condition, addforce);

    // if event not found then add
    if(eventidx == -1){
      eventidx = getGpioEventIndex(gpionumber, condition, true);
    }

    if( eventidx != -1 && eventidx < MAX_GPIO_EVENTS ){
      
      gpio_events[eventidx].gpioNumber = gpionumber;
      gpio_events[eventidx].eventChannel = channel;
      gpio_events[eventidx].eventCondition = condition;
      gpio_events[eventidx].eventConditionValue = value;
      return true;
    }

    return false;
  }

  // clear gpio event
  void clearGpioEvents( uint8_t gpionumber ){

    for (int16_t i = 0; i < MAX_GPIO_EVENTS; i++){
      
      if( gpio_events[i].gpioNumber == gpionumber ){

        gpio_events[i].clear();
      }
    }
  }

private:

  // return index 
  int16_t getGpioEventIndex(uint8_t gpionumber, uint8_t condition = GPIO_EVENT_CONDITION_MAX, bool returnunusedindex = false){

    for (int16_t i = 0; i < MAX_GPIO_EVENTS && gpionumber < MAX_GPIO_PINS; i++){
      
      if(
        (returnunusedindex && gpio_events[i].eventChannel == GPIO_EVENT_CHANNEL_MAX) ||
        (!returnunusedindex && gpio_events[i].gpioNumber == gpionumber && GPIO_EVENT_CONDITION_MAX == condition && gpio_events[i].eventChannel != GPIO_EVENT_CHANNEL_MAX) ||
        (!returnunusedindex && gpio_events[i].gpioNumber == gpionumber && gpio_events[i].eventCondition == condition && gpio_events[i].eventChannel != GPIO_EVENT_CHANNEL_MAX)
      ){
        return i;
      }
    }
    return -1;
  }

#endif
};

// const gpio_configs PROGMEM _gpio_config_defaults = {
//   {OFF}, {0}, {EQUAL}, {GPIO_EVENT_CHANNEL_MAX}, {0}, "", 80, GPIO_DATA_POST_FREQ
// };

const int gpio_config_size = sizeof(gpio_configs) + 5;

using gpio_config_table = gpio_configs;

#endif
