/*********************** Device Control Interface *****************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st Jan 2024
******************************************************************************/

#include "DeviceControlInterface.h"
#include "LoggerInterface.h"

/**
 * DeviceControlInterface constructor.
 */
DeviceControlInterface::DeviceControlInterface()
{
}

/**
 * DeviceControlInterface destructor.
 */
DeviceControlInterface::~DeviceControlInterface()
{
}

/**
 * set the gpio mode
 */
void DeviceControlInterface::gpioMode(GPIO_MODE mode, gpio_id_t pin)
{
    // get the hw pin from map
    gpio_id_t hwpin = gpioFromPinMap(pin, (ANALOG_READ==mode));

    switch (mode)
    {
    case DIGITAL_WRITE:
    case DIGITAL_BLINK:
    case ANALOG_WRITE:
        pinMode(hwpin, OUTPUT);
        break;
    case DIGITAL_READ:
    case OFF:
        pinMode(hwpin, INPUT);
        break;
    default:
        break;
    }
}

/**
 * write digital/analog to gpio
 */
void DeviceControlInterface::gpioWrite(GPIO_MODE mode, gpio_id_t pin, gpio_val_t value)
{
    // get the hw pin from map
    gpio_id_t hwpin = gpioFromPinMap(pin, (ANALOG_READ==mode));

    switch (mode)
    {
    case DIGITAL_WRITE:
        digitalWrite(hwpin, value);
        break;
    case DIGITAL_BLINK:
        digitalWrite(hwpin, !gpioRead(DIGITAL_READ, pin));
        break;
    case ANALOG_WRITE:
        analogWrite(hwpin, value);
        break;
    default:
        break;
    }
}

/**
 * write digital/analog from gpio
 */
gpio_val_t DeviceControlInterface::gpioRead(GPIO_MODE mode, gpio_id_t pin)
{
    // get the hw pin from map
    gpio_id_t hwpin = gpioFromPinMap(pin, (ANALOG_READ==mode));

    gpio_val_t value = -1;

    switch (mode)
    {
    case DIGITAL_READ:
        value = digitalRead(hwpin);
        break;
    case ANALOG_READ:
        value = analogRead(hwpin);
        break;
    default:
        break;
    }

    return value;
}

/**
 * return HW gpio pin number from its digital gpio number
 */
gpio_id_t DeviceControlInterface::gpioFromPinMap(gpio_id_t pin, bool isAnalog)
{
  gpio_id_t mapped_pin;

  // Map
  switch ( pin ) {

    case 0:
      mapped_pin = isAnalog ? A0 : 0;
      break;
    case 1:
      mapped_pin = isAnalog ? A1 : 1;
      break;
    case 2:
      mapped_pin = isAnalog ? A2 : 2;
      break;
    case 3:
      mapped_pin = isAnalog ? A3 : 3;
      break;
    case 4:
      mapped_pin = isAnalog ? A4 : 4;
      break;
    case 5:
      mapped_pin = isAnalog ? A5 : 5;
      break;
    case 6:
      mapped_pin = 6;
      break;
    case 7:
      mapped_pin = 7;
      break;
    case 8:
      mapped_pin = 8;
      break;
    case 9:
      mapped_pin = 9;
      break;
    case 10:
      mapped_pin = 10;
      break;
    case 11:
      mapped_pin = 11;
      break;
    case 12:
      mapped_pin = 12;
      break;
    case 13:
      mapped_pin = 13;
      break;
    default:
      mapped_pin = 0;
  }

  return mapped_pin;
}

/**
 * return whether gpio is exceptional
 */
bool DeviceControlInterface::isExceptionalGpio(gpio_id_t pin)
{
  for (uint8_t j = 0; j < sizeof(EXCEPTIONAL_GPIO_PINS); j++) {

    if( EXCEPTIONAL_GPIO_PINS[j] == pin )return true;
  }
  return false;
}

/**
 * Get new GpioBlinkerInterface instance.
 */
iGpioBlinkerInterface *DeviceControlInterface::createGpioBlinkerInstance(gpio_id_t pin, gpio_val_t duration)
{
    return new GpioBlinkerInterface(pin, duration);
}

/**
 * Delete GpioBlinkerInterface instance.
 */
void DeviceControlInterface::releaseGpioBlinkerInstance(iGpioBlinkerInterface *instance)
{
    if (nullptr != instance)
    {
        delete instance;
    }
}

/**
 * Init device specific features here
 */
void DeviceControlInterface::initDeviceSpecificFeatures()
{
}

/**
 * reset the device
 */
void DeviceControlInterface::resetDevice()
{
}

/**
 * restart the device
 */
void DeviceControlInterface::restartDevice()
{
}

/**
 * device watchdog feed
 */
void DeviceControlInterface::feedWdt()
{
}

/**
 * erase device config if any
 */
void DeviceControlInterface::eraseConfig()
{
}

/**
 * get device id if any
 */
uint32_t DeviceControlInterface::getDeviceId()
{
    return 0;
}

/**
 * get device mac id if any
 */
pdiutil::string DeviceControlInterface::getDeviceMac()
{
    return pdiutil::string( "AvrUno" );
}

/**
 * check whether device factory reset is requested
 */
bool DeviceControlInterface::isDeviceFactoryRequested()
{
    static uint8_t m_flash_key_pressed = 0;

    if (__i_dvc_ctrl.gpioRead(DIGITAL_READ, FLASH_KEY_PIN) == LOW)
    {
        m_flash_key_pressed++;
        LogFmtI("Flash Key pressed : %d\n", m_flash_key_pressed);
    }
    else
    {
        m_flash_key_pressed = 0;
    }

    return (m_flash_key_pressed > FLASH_KEY_PRESS_COUNT_THR);
}

/**
 * keep hold on current execution
 */
void DeviceControlInterface::wait(uint64_t timeoutms)
{
    delay(timeoutms);
}

/**
 * return current time in milli second
 */
uint32_t DeviceControlInterface::millis_now()
{
    return millis();
}

/**
 * log helper for utility
 */
void DeviceControlInterface::log(logger_type_t log_type, const char *content)
{
    __i_logger.log(log_type, content);
}

/**
 * yield
 */
void DeviceControlInterface::yield()
{
    delay(0);
}

/**
 * Upgrade device with provided binary path and new version
 */
upgrade_status_t DeviceControlInterface::Upgrade(const char *path, const char *version)
{
    upgrade_status_t status = UPGRADE_STATUS_MAX;

    return status;
}

DeviceControlInterface __i_dvc_ctrl;
