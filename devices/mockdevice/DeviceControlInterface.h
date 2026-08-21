/*********************** Device Control Interface *****************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st Jan 2024
******************************************************************************/

#ifndef _MOCKDEVICE_DEVICE_CONTROL_INTERFACE_H_
#define _MOCKDEVICE_DEVICE_CONTROL_INTERFACE_H_

#include "mockdevice.h"
#include <interface/pdi/middlewares/iDeviceControlInterface.h>

/**
 * Gpio's that should not be touched
 */
const uint8_t EXCEPTIONAL_GPIO_PINS[] = {3};

/**
 * DeviceControlInterface class
 */
class DeviceControlInterface : public iDeviceControlInterface
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
  void gpioMode(GPIO_MODE mode, gpio_id_t pin) override;
  void gpioWrite(GPIO_MODE mode, gpio_id_t pin, gpio_val_t value) override;
  gpio_val_t gpioRead(GPIO_MODE mode, gpio_id_t pin) override;
  gpio_id_t gpioFromPinMap(gpio_id_t pin, bool isAnalog = false) override;
  bool isExceptionalGpio(gpio_id_t pin) override;
  iGpioBlinkerInterface *createGpioBlinkerInstance(gpio_id_t pin, gpio_val_t duration) override;
  void releaseGpioBlinkerInstance(iGpioBlinkerInterface *instance) override;

  // device control methods
  void initDeviceSpecificFeatures() override;
  void resetDevice() override;
  void restartDevice() override;
  void handleEvents() override;

  // wdt methods
  void enableWdt(uint8_t mode_if_any = 0) override;
  void disableWdt() override;
  void feedWdt() override;

  // misc methods
  void eraseConfig() override;
  uint32_t getDeviceId() override;
  pdiutil::string getDeviceMac() override;
  bool isDeviceFactoryRequested() override;
  iTerminalInterface *getTerminal(terminal_types_t terminal = TERMINAL_TYPE_SERIAL) override;

  // util methods
  void wait(double timeoutms) override;
  uint32_t millis_now() override;
  uint64_t micros_now() override;
  uint32_t random_now() override;
  uint32_t get_free_heap() override;
  uint32_t get_max_free_block() override;
  void log(logger_type_t log_type, const char *content) override;
  void yield() override;

  // upgrade api
#ifdef ENABLE_OTA_SERVICE
  upgrade_status_t Upgrade(const char *path, const char *version, void *client = nullptr) override;
#endif

  /**
   * @brief Detach the clock from the host monotonic source so a caller drives
   *        time explicitly. Time freezes at its current reading when enabled.
   */
  void useVirtualClock(bool enable);

  /**
   * @brief Move the virtual clock forward. Ignored while the clock is real.
   */
  void advanceVirtualClock(uint64_t microseconds);

  /**
   * @brief Seed the pseudo random source so a run reproduces exactly.
   */
  void seedRandom(uint32_t seed);

  /**
   * @brief Number of resetDevice() calls since the last counter clear.
   */
  uint32_t getResetCount() const;

  /**
   * @brief Number of restartDevice() calls since the last counter clear.
   */
  uint32_t getRestartCount() const;

  /**
   * @brief Clear the reset and restart counters.
   */
  void clearControlCounters();

  /**
   * @brief Drive the value an analog read reports for a pin.
   */
  void setAnalogGpioValue(gpio_id_t pin, gpio_val_t value);

  /**
   * @brief Read back the mode last applied to a pin.
   */
  GPIO_MODE getGpioMode(gpio_id_t pin) const;

private:
  uint64_t m_boot_micros;
  uint64_t m_virtual_micros;
  bool m_virtual_clock;
  uint32_t m_random_state;
  uint32_t m_reset_count;
  uint32_t m_restart_count;
  gpio_val_t m_digital_value[MAX_DIGITAL_GPIO_PINS];
  gpio_val_t m_analog_value[MAX_ANALOG_GPIO_PINS];
  GPIO_MODE m_pin_mode[MAX_DIGITAL_GPIO_PINS];

  uint64_t host_micros() const;
};

/**
 * GpioBlinkerInterface class
 */
class GpioBlinkerInterface : public iGpioBlinkerInterface
{

private:
  gpio_id_t m_pin;
  gpio_val_t m_duration;
  bool m_running;

public:
  /**
   * GpioBlinkerInterface constructor.
   */
  GpioBlinkerInterface(gpio_id_t pin, gpio_val_t duration);

  /**
   * GpioBlinkerInterface destructor.
   */
  ~GpioBlinkerInterface();

  /**
   * blink configuration
   */
  void setConfig(gpio_id_t pin, gpio_val_t duration) override;

  /**
   * update configuration api
   */
  void updateConfig(gpio_id_t pin, gpio_val_t duration) override;

  /**
   * start blinker
   */
  void start() override;

  /**
   * stop blinker
   */
  void stop() override;

  /**
   * is blinker running
   */
  bool isRunning() override;
};

#endif // _MOCKDEVICE_DEVICE_CONTROL_INTERFACE_H_
