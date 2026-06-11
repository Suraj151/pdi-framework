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
#include "PingInterface.h"
#include "SerialInterface.h"
#ifdef ENABLE_TLS_CERT_GENERATION
#include "TlsCertProvisioner.h"
#include "WiFiInterface.h"
#include <utility/EventUtil.h>
#endif

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

#if defined(CONFIG_IDF_TARGET_ESP32C3)
  switch ( pin ) {
    case 0:
      mapped_pin = isAnalog ? 0 : 2;
      break;
    case 1:
      mapped_pin = isAnalog ? 1 : 3;
      break;
    case 2:
      mapped_pin = isAnalog ? 4 : 4;
      break;
    case 3:
      mapped_pin = 5;
      break;
    case 4:
      mapped_pin = 6;
      break;
    case 5:
      mapped_pin = 7;
      break;
    case 6:
      mapped_pin = 21;
      break;
    case 7:
      mapped_pin = 20;
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
    default:
      mapped_pin = 0;
  }
#else
  switch ( pin ) {

    case 0:
      mapped_pin = isAnalog ? 32 : 4;
      break;
    case 1:
      mapped_pin = isAnalog ? 33 : 13;
      break;
    case 2:
      mapped_pin = isAnalog ? 34 : 14;
      break;
    case 3:
      mapped_pin = isAnalog ? 35 : 16;
      break;
    case 4:
      mapped_pin = 17;
      break;
    case 5:
      mapped_pin = 18;
      break;
    case 6:
      mapped_pin = 19;
      break;
    case 7:
      mapped_pin = 21;
      break;
    case 8:
      mapped_pin = 22;
      break;
    case 9:
      mapped_pin = 23;
      break;
    default:
      mapped_pin = 0;
  }
#endif

  return mapped_pin;
}

/**
 * return whether gpio is exceptional
 */
bool DeviceControlInterface::isExceptionalGpio(gpio_id_t pin)
{
  gpio_id_t hw = gpioFromPinMap(pin);
  for (uint8_t j = 0; j < sizeof(EXCEPTIONAL_GPIO_PINS); j++) {

    if( EXCEPTIONAL_GPIO_PINS[j] == hw )return true;
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

    #ifdef ENABLE_SERVER_TLS_CERT_GENERATION_AT_RUNTIME
    __utl_event.add_event_listener(EVENT_WIFI_STA_GOT_IP, [](void* arg) {
        uint32_t ip = __i_wifi.localIP();
        if (ip == 0) return;
        TlsCertProvisioner::ensureServerCert(
            TLS_DEFAULT_SERVER_CERT_PATH,
            TLS_DEFAULT_SERVER_KEY_PATH,
            ip,
            nullptr);
    });
    #endif
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
 * handle device specific events
 */
void DeviceControlInterface::handleEvents()
{
    if (__serial_uart.available()) {
        serial_event_t e(SERIAL_IFACE_UART, SERIAL_IFACE_CMD, &__serial_uart);
        __utl_event.execute_event(EVENT_SERIAL_AVAILABLE, &e);
    }
}

/**
 * device watchdog enable
 */
void DeviceControlInterface::enableWdt(uint8_t mode_if_any)
{
    // ESP.wdtEnable((uint32_t)0);
}

/**
 * device watchdog disable
 */
void DeviceControlInterface::disableWdt()
{
    // ESP.wdtDisable();
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

    if (FLASH_KEY_PIN >= 0 && __i_dvc_ctrl.gpioRead(DIGITAL_READ, FLASH_KEY_PIN) == LOW)
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

    #ifdef ENABLE_CONTEXTUAL_EXECUTION
    if(_timeoutms > 0){
        if (__i_cooperative_scheduler.can_sleep_from_othersched()) {
            __i_cooperative_scheduler.sleep_from_othersched(_timeoutms);
        } else {
            __i_cooperative_scheduler.sleep(_timeoutms);
        }
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
 * return currently free heap size in bytes
 */
uint32_t DeviceControlInterface::get_free_heap()
{
    return ESP.getFreeHeap();
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
    // run_scheduled_functions();
    // run_scheduled_recurrent_functions();
    // esp_schedule();
    vPortYield();
    optimistic_yield(1000);
    delay(0);
}

/**
 * Upgrade device with provided binary path and new version
 */
upgrade_status_t DeviceControlInterface::Upgrade(const char *path, const char *version)
{
    String binary_path = path;
    upgrade_status_t status = UPGRADE_STATUS_MAX;
    WiFiClient _wifi_client;

    httpUpdate.rebootOnUpdate(false);
    httpUpdate.setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS);
    httpUpdate.setAuthorization("ota", __i_dvc_ctrl.getDeviceMac().c_str());
    t_httpUpdate_return ret = httpUpdate.update( _wifi_client, binary_path );

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
