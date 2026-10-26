/*********************** Device Control Interface *****************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st Jan 2024
******************************************************************************/

#ifndef _DEVICE_CONTROL_INTERFACE_H_
#define _DEVICE_CONTROL_INTERFACE_H_

#include "esp8266.h"
#include <interface/pdi/middlewares/iDeviceControlInterface.h>
#ifdef ENABLE_CONTEXTUAL_EXECUTION
#include "threading/PreemptiveMutex.h"
#endif

/**
 * Gpio's that should not be touched
 * 1 = UART0 TX, 3 = UART0 RX, 6-11 = SPI flash pins
 */
const uint8_t EXCEPTIONAL_GPIO_PINS[] = {1, 3, 6, 7, 8, 9, 10, 11};

/**
 * Gpio's the I2S bus runs on, in the order SCK, WS, SD; held only while a line uses
 * the bus.
 */
#if defined(ENABLE_DRIVER_CORE) && defined(ENABLE_I2S_BUS)
const uint8_t I2S_GPIO_PINS[] = {13, 14, 12};
#endif

/**
 * Gpio's the I2C bus runs on when no bus line names its own, in the order SDA, SCL; held only while a line uses the bus
 */
#if defined(ENABLE_DRIVER_CORE) && defined(ENABLE_I2C_BUS)
const uint8_t I2C_GPIO_PINS[] = {4, 5};
#endif

/**
 * Gpio's the SPI bus runs on, in the order SCK, MISO, MOSI; held only while a line
 * uses the bus.
 */
#if defined(ENABLE_DRIVER_CORE) && defined(ENABLE_SPI_BUS)
const uint8_t SPI_GPIO_PINS[] = {14, 12, 13};
#endif

/**
 * DeviceControlInterface class
 */
class DeviceControlInterface: public iDeviceControlInterface
{
  #ifdef ENABLE_CONTEXTUAL_EXECUTION
  PreemptiveMutex m_mutex;
  #endif

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

  /**
   * Fills in what this port can report about the hardware it runs on, leaving
   * untouched whatever it has no answer for.
   */
  void getDeviceInfo(device_info_t &_out) override;

  // util methods
  void wait(double timeoutms) override;
  uint32_t millis_now() override;
  uint64_t micros_now() override;
  uint32_t random_now() override;
  uint32_t get_free_heap() override;
  uint32_t get_max_free_block() override;
  void log(logger_type_t log_type, const char *content) override;
  void yield() override;
  void handleEvents() override;

  /**
   * Whether the caller may wait: true on the loop and in a preemptive task,
   * false in the system context callbacks run in.
   */
  bool canWait() const;

  // upgrade api
#ifdef ENABLE_OTA_SERVICE
  upgrade_status_t Upgrade(const char *path, const char *version, void *client = nullptr) override;
#ifdef ENABLE_STORAGE_SERVICE
  upgrade_status_t UpgradeFromFile(const char *path) override;
#endif
#endif
};

/**
 * GpioBlinkerInterface class
 */
class GpioBlinkerInterface : public iGpioBlinkerInterface
{

private:

  gpio_id_t m_pin;
  gpio_val_t m_duration;
  pdiutil::task_id_t m_ticker_id;

public:
  /**
   * GpioBlinkerInterface constructor.
   */
  GpioBlinkerInterface(gpio_id_t pin, gpio_val_t duration) : m_pin(pin),
                                                             m_duration(duration),
                                                             m_ticker_id(-1)
  {
    __i_dvc_ctrl.gpioMode(DIGITAL_BLINK, this->m_pin);
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
      this->m_ticker_id = __task_scheduler.updateInterval( this->m_ticker_id, [&]() {
        this->blink();
        }, this->m_duration, DEFAULT_TASK_PRIORITY, __i_dvc_ctrl.millis_now()
      );
    }
  }

  /**
   * stop blinker
   */
  void stop()
  {
    __task_scheduler.clearInterval(this->m_ticker_id);
    this->m_ticker_id = -1;
  }

  /**
   * is blinker running
   */
  bool isRunning()
  {
    return ( this->m_ticker_id != -1 );
  }
};

#endif
