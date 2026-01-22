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
#include "core/Espnow.h"
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

    #ifdef ENABLE_CONTEXTUAL_EXECUTION
    m_mutex.lock();
    #endif

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

    #ifdef ENABLE_CONTEXTUAL_EXECUTION
    m_mutex.unlock();
    #endif
}

/**
 * write digital/analog to gpio
 */
void DeviceControlInterface::gpioWrite(GPIO_MODE mode, gpio_id_t pin, gpio_val_t value)
{
    // get the hw pin from map
    gpio_id_t hwpin = gpioFromPinMap(pin, (ANALOG_READ==mode));

    #ifdef ENABLE_CONTEXTUAL_EXECUTION
    m_mutex.lock();
    #endif

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

    #ifdef ENABLE_CONTEXTUAL_EXECUTION
    m_mutex.unlock();
    #endif
}

/**
 * write digital/analog from gpio
 */
gpio_val_t DeviceControlInterface::gpioRead(GPIO_MODE mode, gpio_id_t pin)
{
    // get the hw pin from map
    gpio_id_t hwpin = gpioFromPinMap(pin, (ANALOG_READ==mode));
    gpio_val_t value = -1;

    #ifdef ENABLE_CONTEXTUAL_EXECUTION
    m_mutex.lock();
    #endif

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

    #ifdef ENABLE_CONTEXTUAL_EXECUTION
    m_mutex.unlock();
    #endif

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
    #ifdef ENABLE_CONTEXTUAL_EXECUTION
    m_mutex.lock();
    #endif

    iGpioBlinkerInterface * p = new GpioBlinkerInterface(pin, duration);

    #ifdef ENABLE_CONTEXTUAL_EXECUTION
    m_mutex.unlock();
    #endif
    
    return p;
}

/**
 * Delete GpioBlinkerInterface instance.
 */
void DeviceControlInterface::releaseGpioBlinkerInstance(iGpioBlinkerInterface *instance)
{
    #ifdef ENABLE_CONTEXTUAL_EXECUTION
    m_mutex.lock();
    #endif

    if (nullptr != instance)
    {
        delete instance;
    }

    #ifdef ENABLE_CONTEXTUAL_EXECUTION
    m_mutex.unlock();
    #endif
}

/**
 * Init device specific features here
 */
void DeviceControlInterface::initDeviceSpecificFeatures()
{
    #ifdef ENABLE_ESP_NOW
    __espnow.begin( &__i_wifi );
    #endif

    #ifdef ENABLE_NETWORK_SERVICE
    __i_ping.init_ping( &__i_wifi );
    #endif

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
 * device watchdog enable
 */
void DeviceControlInterface::enableWdt(uint8_t mode_if_any)
{
    ESP.wdtEnable((uint32_t)0);
}

/**
 * device watchdog disable
 */
void DeviceControlInterface::disableWdt()
{
    ESP.wdtDisable();
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
    #ifdef ENABLE_SERIAL_SERVICE
    if( TERMINAL_TYPE_MAX > terminal ){
      return &__serial_uart;
    } 
    #endif
    return nullptr; 
}

/**
 * keep hold on current execution
 */
void DeviceControlInterface::wait(double timeoutms)
{
    uint32_t _timeoutms = (uint32_t)timeoutms;
    uint32_t _remainingtimeoutus = (uint32_t)((timeoutms - _timeoutms) * 1000.0); // convert to microseconds

    // Using delay to run other contexual tasks if any
    // We can sleep from preemptive sched as well in case if we dont have cooperative tasks.
    // But if we have cooperative tasks then utilize this sleep for cooperative tasks 
    // Since preemptive tasks will get the scheduled slots soon if scheduled
    #ifdef ENABLE_CONTEXTUAL_EXECUTION
    if(_timeoutms > 0){
        __i_cooperative_scheduler.sleep_from_othersched(_timeoutms);
    }
    delay(0);
    #else
    delay(_timeoutms); // delay in milliseconds
    #endif
    if( _remainingtimeoutus > 0 )
        delayMicroseconds(_remainingtimeoutus); // delay in microseconds
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
    #if defined(LOGBEGIN) && ( defined(ENABLE_LOG_ALL) || defined(ENABLE_LOG_INFO) || defined(ENABLE_LOG_ERROR) || defined(ENABLE_LOG_WARNING) || defined(ENABLE_LOG_SUCCESS) )
    __i_logger.log(log_type, content);
    #endif
}

/**
 * yield
 */
void DeviceControlInterface::yield()
{
    #ifdef ENABLE_CONTEXTUAL_EXECUTION
    m_mutex.lock();
    #endif

    // run_scheduled_functions();
    // run_scheduled_recurrent_functions();
    // esp_schedule();
    esp_yield();
    optimistic_yield(1000);
    delay(0);

    #ifdef ENABLE_CONTEXTUAL_EXECUTION
    m_mutex.unlock();
    #endif
}

/**
 * Upgrade device with provided binary path and new version
 */
upgrade_status_t DeviceControlInterface::Upgrade(const char *path, const char *version)
{
    String binary_path = path;
    upgrade_status_t status = UPGRADE_STATUS_MAX;
    WiFiClient _wifi_client;

#ifdef ENABLE_WIFI_SERVICE
    ESPhttpUpdate.rebootOnUpdate(false);
    ESPhttpUpdate.followRedirects(true);
    ESPhttpUpdate.setAuthorization("ota", __i_dvc_ctrl.getDeviceMac().c_str());
    t_httpUpdate_return ret = ESPhttpUpdate.update( _wifi_client, binary_path );

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
#endif

    return status;
}

DeviceControlInterface __i_dvc_ctrl;
