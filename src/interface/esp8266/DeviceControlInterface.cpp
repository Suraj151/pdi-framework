/*********************** Device Control Interface *****************************
This file is part of the Ewings Esp Stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st Jan 2024
******************************************************************************/

#include "DeviceControlInterface.h"

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
 * write digital/analog to gpio
 */
void DeviceControlInterface::gpioWrite(GPIO_MODE mode, gpio_id_t pin, gpio_val_t value)
{
    switch (mode)
    {
    case DIGITAL_WRITE:
        digitalWrite(pin, value);
        break;
    case ANALOG_WRITE:
        analogWrite(pin, value);
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
    gpio_val_t value = -1;

    switch (mode)
    {
    case DIGITAL_READ:
        value = digitalRead(pin);
        break;
    case ANALOG_READ:
        value = analogRead(pin);
        break;
    default:
        break;
    }

    return value;
}

/**
 * reset the device
 */
void DeviceControlInterface::resetDevice()
{
    ESP.reset();
}

/**
 * restart the device
 */
void DeviceControlInterface::restartDevice()
{
    ESP.restart();
}

/**
 * erase device config if any
 */
void DeviceControlInterface::eraseConfig()
{
    ESP.eraseConfig();
}

/**
 * get device id if any
 */
uint32_t DeviceControlInterface::getDeviceId()
{
    return ESP.getChipId();
}

/**
 * device watchdog feed
 */
void DeviceControlInterface::deviceWdtFeed()
{
    ESP.wdtFeed();
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

DeviceControlInterface __i_dvc_ctrl;
