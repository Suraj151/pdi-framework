/*********************** Device Control Interface *****************************
This file is part of the Ewings Esp Stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st Jan 2024
******************************************************************************/

#ifndef _MOCKDEVICE_DEVICE_CONTROL_INTERFACE_H_
#define _MOCKDEVICE_DEVICE_CONTROL_INTERFACE_H_

#include "mockdevice.h"
#include <interface/pdi/middlewares/iDeviceControlInterface.h>
#include "LoggerInterface.h"
#include "WiFiClientInterface.h"
#include "PingInterface.h"

/**
 * Gpio's that should not be touched
 */
const uint8_t EXCEPTIONAL_GPIO_PINS[] = {3};

/**
 * DeviceControlInterface class
 */
class DeviceControlInterface: public iDeviceControlInterface
{

public:
  /**
   * DeviceControlInterface constructor.
   */
  DeviceControlInterface(){}

  /**
   * DeviceControlInterface destructor.
   */
  ~DeviceControlInterface(){}

  // GPIO methods
  void gpioMode( GPIO_MODE mode, gpio_id_t pin ) override{}
  void gpioWrite( GPIO_MODE mode, gpio_id_t pin, gpio_val_t value ) override{}
  gpio_val_t gpioRead( GPIO_MODE mode, gpio_id_t pin ) override{return 0;}
  gpio_id_t gpioFromPinMap( gpio_id_t pin, bool isAnalog=false ) override{return 0;}
  bool isExceptionalGpio( gpio_id_t pin ) override{return 0;}
  iGpioBlinkerInterface *createGpioBlinkerInstance(gpio_id_t pin, gpio_val_t duration) override{return nullptr;}
  void releaseGpioBlinkerInstance(iGpioBlinkerInterface *instance) override{}

  // device control methods
  void initDeviceSpecificFeatures() override{}
  void resetDevice() override{}
  void restartDevice() override{}

  // wdt methods
  void enableWdt(uint8_t mode_if_any=0) override {} // bypassing these methods since not used
  void disableWdt() override {} // bypassing these methods since not used
  void feedWdt() override{}

  // misc methods
  void eraseConfig() override{}
  uint32_t getDeviceId() override{return 0;}
  std::string getDeviceMac() override{return "XX:XX:XX:XX:XX:XX";}
  bool isDeviceFactoryRequested() override{return 0;}

  // util methods
  void wait(uint64_t timeoutms) override{}
  uint32_t millis_now() override{ static uint32_t now=0; return now++; }
  void log(logger_type_t log_type, const char *content) override{}
  void yield() override{}

  // upgrade api
  upgrade_status_t Upgrade(const char *path, const char *version) override{return UPGRADE_STATUS_IGNORE;}
};


/**
 * GpioBlinkerInterface class
 */
class GpioBlinkerInterface : public iGpioBlinkerInterface
{

private:

  gpio_id_t m_pin;
  gpio_val_t m_duration;
  // Ticker m_ticker;

public:
  /**
   * GpioBlinkerInterface constructor.
   */
  GpioBlinkerInterface(gpio_id_t pin, gpio_val_t duration) : m_pin(pin),
                                                             m_duration(duration)
  {
  }

  /**
   * GpioBlinkerInterface destructor.
   */
  ~GpioBlinkerInterface(){};

  /**
   * blink callback function
   */
  void blink(){}

  /**
   * blink configuration
   */
  void setConfig(gpio_id_t pin, gpio_val_t duration){}

  /**
   * update configuration api
   */
  void updateConfig(gpio_id_t pin, gpio_val_t duration){}

  /**
   * start blinker
   */
  void start(){}

  /**
   * stop blinker
   */
  void stop(){}

  /**
   * is blinker running
   */
  bool isRunning(){return 0;}
};

#endif
