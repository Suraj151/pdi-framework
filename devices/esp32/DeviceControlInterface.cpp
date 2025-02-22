/*********************** Device Control Interface *****************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st Jan 2024
******************************************************************************/

#include "DeviceControlInterface.h"
#include "ExceptionsNotifier.h"
#include "LoggerInterface.h"
#include "WiFiClientInterface.h"
#include "PingInterface.h"
#include "SerialInterface.h"

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
      mapped_pin = isAnalog ? 17 : 16;
      break;
    case 1:
      mapped_pin = 5;
      break;
    case 2:
      mapped_pin = 4;
      break;
    case 3:
      mapped_pin = 0;
      break;
    case 4:
      mapped_pin = 2;
      break;
    case 5:
      mapped_pin = 14;
      break;
    case 6:
      mapped_pin = 12;
      break;
    case 7:
      mapped_pin = 13;
      break;
    case 8:
      mapped_pin = 15;
      break;
    case 9:
      mapped_pin = 3;
      break;
    case 10:
      mapped_pin = 1;
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
    __i_ping.init_ping( &__i_wifi );

    #ifdef ENABLE_EXCEPTION_NOTIFIER
    beginCrashHandler();
    #endif

    // Open default serial terminal
    if( nullptr != getTerminal(TERMINAL_TYPE_SERIAL) ){
        getTerminal(TERMINAL_TYPE_SERIAL)->open();
    }
}

/**
 * reset the device
 */
void DeviceControlInterface::resetDevice()
{
    ESP.restart(); // currently using restart api only
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
    // ESP.wdtFeed();
}

/**
 * erase device config if any
 */
void DeviceControlInterface::eraseConfig()
{
    // ESP.eraseConfig();
}

/**
 * get device id if any
 */
uint32_t DeviceControlInterface::getDeviceId()
{
    return g_rom_flashchip.device_id;
}

/**
 * get device mac id if any
 */
pdiutil::string DeviceControlInterface::getDeviceMac()
{
    return pdiutil::string( WiFi.macAddress().c_str() );
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
 * get terminal interface to interact
 */
iTerminalInterface * DeviceControlInterface::getTerminal(terminal_types_t terminal)
{
    if( TERMINAL_TYPE_MAX > terminal ){
      return &__serial_uart;
    } 
    return nullptr; 
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
 * log helper for print task
 */
void DeviceControlInterface::printtasks(pdiutil::vector<task_t> &tasks, iTerminalInterface *terminal)
{
    if( nullptr != terminal ){

        terminal->write_ro(RODT_ATTR("\nTasks : \n"));
        terminal->write_ro(RODT_ATTR("id        ")); // max column size=10
        terminal->write_ro(RODT_ATTR("priority  ")); // max column size=10
        terminal->write_ro(RODT_ATTR("interval  ")); // max column size=10
        terminal->write_ro(RODT_ATTR("last_ms   ")); // max column size=10
        terminal->write_ro(RODT_ATTR("exc_ms    ")); // max column size=10
        terminal->write_ro(RODT_ATTR("max_attempts\n")); // max column size=14

        char content[20];

        for (int i = 0; i < tasks.size(); i++)
        {
            Int32ToString(tasks[i]._task_id, content, 20, 10);
            terminal->write(content);

            Int32ToString(tasks[i]._task_priority, content, 20, 10);
            terminal->write(content);

            Int64ToString(tasks[i]._duration, content, 20, 10);
            terminal->write(content);

            Int64ToString(tasks[i]._last_millis, content, 20, 10);
            terminal->write(content);

            Int64ToString(tasks[i]._task_exec_millis, content, 20, 10);
            terminal->write(content);

            Int32ToString(tasks[i]._max_attempts, content, 20, 14);
            terminal->write(content);
            terminal->write("\n");
        }
    }
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

    httpUpdate.rebootOnUpdate(false);
    httpUpdate.setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS);
    httpUpdate.setAuthorization("ota", __i_dvc_ctrl.getDeviceMac().c_str());
    t_httpUpdate_return ret = httpUpdate.update( *(__i_wifi_client.getWiFiClient()), binary_path );

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
