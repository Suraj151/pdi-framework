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
 * set the gpio mode
 */
void DeviceControlInterface::gpioMode(GPIO_MODE mode, gpio_id_t pin)
{
    switch (mode)
    {
    case DIGITAL_WRITE:
        pinMode(pin, OUTPUT);
        break;
    case DIGITAL_READ:
        pinMode(pin, INPUT);
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
 * device watchdog feed
 */
void DeviceControlInterface::feedWdt()
{
    ESP.wdtFeed();
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
 * get device mac id if any
 */
std::string DeviceControlInterface::getDeviceMac()
{
    return std::string( WiFi.macAddress().c_str() );
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
    String binary_path = path;
    upgrade_status_t status = UPGRADE_STATUS_MAX;

    ESPhttpUpdate.rebootOnUpdate(false);
    ESPhttpUpdate.followRedirects(true);
    ESPhttpUpdate.setAuthorization("ota", __i_dvc_ctrl.getDeviceMac().c_str());
    t_httpUpdate_return ret = ESPhttpUpdate.update( *(__i_wifi_client.getWiFiClient()), binary_path );

    if( ret == HTTP_UPDATE_FAILED ){

        LogE("DEVICE_UPGRADE_FAILD\n");
        status = UPGRADE_STATUS_FAILED;
    }else if( ret == HTTP_UPDATE_NO_UPDATES ){

        LogI("DEVICE_UPGRADE_NO_UPDATES\n");
        status = UPGRADE_STATUS_IGNORE;
    }else if( ret == HTTP_UPDATE_OK ){

        LogS("DEVICE_UPGRADE_OK\n");
        status = UPGRADE_STATUS_SUCCESS;
    }else{

        LogW("DEVICE_UPGRADE_UNKNOWN_RETURN\n");
        status = UPGRADE_STATUS_MAX;
    }

    return status;
}

DeviceControlInterface __i_dvc_ctrl;
