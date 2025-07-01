/*********************** Device Control Interface *****************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st Jan 2024
******************************************************************************/

#ifndef _DEVICE_CONTROL_INTERFACE_H_
#define _DEVICE_CONTROL_INTERFACE_H_

#include "esp32.h"
#include <interface/pdi/middlewares/iDeviceControlInterface.h>

/**
 * Gpio's that should not be touched
 */
const uint8_t EXCEPTIONAL_GPIO_PINS[] = {3, 9};

/**
 * DeviceControlInterface class
 */
class DeviceControlInterface: public iDeviceControlInterface
{

public:
  /**
   * DeviceControlInterface constructor.
   */
  DeviceControlInterface();

  /**
   * DeviceControlInterface destructor.
   */
  ~DeviceControlInterface();

  // GPIO methods
  void gpioMode( GPIO_MODE mode, gpio_id_t pin ) override;
  void gpioWrite( GPIO_MODE mode, gpio_id_t pin, gpio_val_t value ) override;
  gpio_val_t gpioRead( GPIO_MODE mode, gpio_id_t pin ) override;
  gpio_id_t gpioFromPinMap( gpio_id_t pin, bool isAnalog=false ) override;
  bool isExceptionalGpio( gpio_id_t pin ) override;
  iGpioBlinkerInterface *createGpioBlinkerInstance(gpio_id_t pin, gpio_val_t duration) override;
  void releaseGpioBlinkerInstance(iGpioBlinkerInterface *instance) override;

  // device control methods
  void initDeviceSpecificFeatures() override;
  void resetDevice() override;
  void restartDevice() override;

  // wdt methods
  void enableWdt(uint8_t mode_if_any=0) override;
  void disableWdt() override;
  void feedWdt() override;

  // misc methods
  void eraseConfig() override;
  uint32_t getDeviceId() override;
  pdiutil::string getDeviceMac() override;
  bool isDeviceFactoryRequested() override;
  iTerminalInterface* getTerminal(terminal_types_t terminal=TERMINAL_TYPE_SERIAL) override;

  // util methods
  void wait(uint64_t timeoutms) override;
  uint32_t millis_now() override;
  void log(logger_type_t log_type, const char *content) override;
  void yield() override;

  // upgrade api
  upgrade_status_t Upgrade(const char *path, const char *version) override;
};

/**
 * GpioBlinkerInterface class
 */
class GpioBlinkerInterface : public iGpioBlinkerInterface
{

private:

  gpio_id_t m_pin;
  gpio_val_t m_duration;
  Ticker m_ticker;

public:
  /**
   * GpioBlinkerInterface constructor.
   */
  GpioBlinkerInterface(gpio_id_t pin, gpio_val_t duration) : m_pin(pin),
                                                             m_duration(duration)
  {
    pinMode(this->m_pin, OUTPUT);
    this->start();
  }

  /**
   * GpioBlinkerInterface destructor.
   */
  ~GpioBlinkerInterface()
  {
    this->stop();
  };

  /**
   * blink callback function
   */
  void blink()
  {
    __i_dvc_ctrl.gpioWrite(DIGITAL_BLINK, this->m_pin, 0);
  }

  /**
   * blink configuration
   */
  void setConfig(gpio_id_t pin, gpio_val_t duration)
  {
    this->m_pin = pin;
    this->m_duration = duration;
  }

  /**
   * update configuration api
   */
  void updateConfig(gpio_id_t pin, gpio_val_t duration)
  {
    if (this->m_pin != pin || this->m_duration != duration)
    {
      this->setConfig(pin, duration);

      if (this->isRunning())
      {
        this->stop();
        this->start();
      }
    }
  }

  /**
   * start blinker
   */
  void start()
  {
    if (!this->isRunning() && GPIO_DIGITAL_BLINK_MIN_DURATION_MS <= this->m_duration)
    {
      this->m_ticker.attach_ms(this->m_duration, std::bind(&GpioBlinkerInterface::blink, this));
    }
  }

  /**
   * stop blinker
   */
  void stop()
  {
    this->m_ticker.detach();
  }

  /**
   * is blinker running
   */
  bool isRunning()
  {
    return this->m_ticker.active();
  }
};

#endif
