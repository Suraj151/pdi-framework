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

#ifdef ENABLE_HTTP_CLIENT
#include <transports/http/HTTPClient.h>
#endif

#include <Update.h>
#include <esp_timer.h>

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
  // ESP32-C3: 22 GPIOs. 11-17=flash, 18-19=USB-JTAG, 20-21=UART0.
  // 8 digital + 4 analog = 12 user pins. Some boot-strap pins (2/8/9) included
  // because C3 doesn't have enough fully-safe pins to reach 8 digital otherwise.
  switch ( pin ) {
    case 0:
      mapped_pin = isAnalog ? 0 : 5;    // D0 or A0 (ADC1_CH0)
      break;
    case 1:
      mapped_pin = isAnalog ? 1 : 6;    // D1 or A1 (ADC1_CH1)
      break;
    case 2:
      mapped_pin = isAnalog ? 3 : 7;    // D2 or A2 (ADC1_CH3)
      break;
    case 3:
      mapped_pin = isAnalog ? 4 : 10;   // D3 or A3 (ADC1_CH4)
      break;
    case 4:
      mapped_pin = 8;                    // D4 (boot-strap)
      break;
    case 5:
      mapped_pin = 9;                    // D5 (boot-strap)
      break;
    case 6:
      mapped_pin = 2;                    // D6 (boot-strap)
      break;
    case 7:
      mapped_pin = 1;                    // D7
      break;
    case 8:
      mapped_pin = 0;                    // A0 via total-index (ADC1_CH0)
      break;
    case 9:
      mapped_pin = 1;                    // A1 via total-index (ADC1_CH1)
      break;
    case 10:
      mapped_pin = 3;                    // A2 via total-index (ADC1_CH3)
      break;
    case 11:
      mapped_pin = 4;                    // A3 via total-index (ADC1_CH4)
      break;
    default:
      mapped_pin = INVALID_GPIO_NUMBER;
  }
#elif defined(CONFIG_IDF_TARGET_ESP32S2)
  // ESP32-S2: 43 GPIOs. 26-32=flash, 19-20=USB, boot-strap 0/45/46.
  // ADC1 = GPIO 1-10. 12 digital + 4 analog = 16 user pins.
  switch ( pin ) {
    case 0:
      mapped_pin = isAnalog ? 1 : 17;   // D0 or A0 (ADC1_CH0)
      break;
    case 1:
      mapped_pin = isAnalog ? 2 : 18;   // D1 or A1 (ADC1_CH1)
      break;
    case 2:
      mapped_pin = isAnalog ? 3 : 21;   // D2 or A2 (ADC1_CH2)
      break;
    case 3:
      mapped_pin = isAnalog ? 4 : 33;   // D3 or A3 (ADC1_CH3)
      break;
    case 4:
      mapped_pin = 34;                   // D4
      break;
    case 5:
      mapped_pin = 35;                   // D5
      break;
    case 6:
      mapped_pin = 36;                   // D6
      break;
    case 7:
      mapped_pin = 37;                   // D7
      break;
    case 8:
      mapped_pin = 38;                   // D8
      break;
    case 9:
      mapped_pin = 39;                   // D9
      break;
    case 10:
      mapped_pin = 40;                   // D10
      break;
    case 11:
      mapped_pin = 41;                   // D11
      break;
    case 12:
      mapped_pin = 1;                    // A0 via total-index (ADC1_CH0)
      break;
    case 13:
      mapped_pin = 2;                    // A1 via total-index (ADC1_CH1)
      break;
    case 14:
      mapped_pin = 3;                    // A2 via total-index (ADC1_CH2)
      break;
    case 15:
      mapped_pin = 4;                    // A3 via total-index (ADC1_CH3)
      break;
    default:
      mapped_pin = INVALID_GPIO_NUMBER;
  }
#elif defined(CONFIG_IDF_TARGET_ESP32S3)
  // ESP32-S3: 45 GPIOs. 26-32=flash, 33-37=octal PSRAM, 19-20=USB-JTAG,
  // boot-strap 0/45/46. ADC1 = GPIO 1-10. 12 digital + 4 analog = 16 user pins.
  switch ( pin ) {
    case 0:
      mapped_pin = isAnalog ? 1 : 4;    // D0 or A0 (ADC1_CH0)
      break;
    case 1:
      mapped_pin = isAnalog ? 2 : 5;    // D1 or A1 (ADC1_CH1)
      break;
    case 2:
      mapped_pin = isAnalog ? 3 : 6;    // D2 or A2 (ADC1_CH2)
      break;
    case 3:
      mapped_pin = isAnalog ? 7 : 8;    // D3 or A3 (ADC1_CH6)
      break;
    case 4:
      mapped_pin = 9;                    // D4
      break;
    case 5:
      mapped_pin = 10;                   // D5
      break;
    case 6:
      mapped_pin = 11;                   // D6
      break;
    case 7:
      mapped_pin = 12;                   // D7
      break;
    case 8:
      mapped_pin = 13;                   // D8
      break;
    case 9:
      mapped_pin = 14;                   // D9
      break;
    case 10:
      mapped_pin = 15;                   // D10
      break;
    case 11:
      mapped_pin = 16;                   // D11
      break;
    case 12:
      mapped_pin = 1;                    // A0 via total-index
      break;
    case 13:
      mapped_pin = 2;                    // A1 via total-index
      break;
    case 14:
      mapped_pin = 3;                    // A2 via total-index
      break;
    case 15:
      mapped_pin = 7;                    // A3 via total-index (ADC1_CH6)
      break;
    default:
      mapped_pin = INVALID_GPIO_NUMBER;
  }
#elif defined(CONFIG_IDF_TARGET_ESP32C6)
  // ESP32-C6: 31 GPIOs. Flash pins (module dependent), 12-13=USB, boot-strap 8/9/15.
  // ADC1 = GPIO 0-6. 12 digital + 4 analog = 16 user pins.
  switch ( pin ) {
    case 0:
      mapped_pin = isAnalog ? 0 : 4;    // D0 or A0 (ADC1_CH0)
      break;
    case 1:
      mapped_pin = isAnalog ? 1 : 5;    // D1 or A1 (ADC1_CH1)
      break;
    case 2:
      mapped_pin = isAnalog ? 2 : 6;    // D2 or A2 (ADC1_CH2)
      break;
    case 3:
      mapped_pin = isAnalog ? 3 : 7;    // D3 or A3 (ADC1_CH3)
      break;
    case 4:
      mapped_pin = 10;                   // D4
      break;
    case 5:
      mapped_pin = 11;                   // D5
      break;
    case 6:
      mapped_pin = 16;                   // D6
      break;
    case 7:
      mapped_pin = 17;                   // D7
      break;
    case 8:
      mapped_pin = 18;                   // D8
      break;
    case 9:
      mapped_pin = 19;                   // D9
      break;
    case 10:
      mapped_pin = 20;                   // D10
      break;
    case 11:
      mapped_pin = 21;                   // D11
      break;
    case 12:
      mapped_pin = 0;                    // A0 via total-index
      break;
    case 13:
      mapped_pin = 1;                    // A1 via total-index
      break;
    case 14:
      mapped_pin = 2;                    // A2 via total-index
      break;
    case 15:
      mapped_pin = 3;                    // A3 via total-index
      break;
    default:
      mapped_pin = INVALID_GPIO_NUMBER;
  }
#elif defined(CONFIG_IDF_TARGET_ESP32H2)
  // ESP32-H2: ~19 GPIOs. Flash pins module-dependent, 26-27=USB-JTAG.
  // ADC1 = GPIO 1-5. 8 digital + 4 analog = 12 user pins.
  switch ( pin ) {
    case 0:
      mapped_pin = isAnalog ? 1 : 10;   // D0 or A0 (ADC1_CH0)
      break;
    case 1:
      mapped_pin = isAnalog ? 2 : 11;   // D1 or A1 (ADC1_CH1)
      break;
    case 2:
      mapped_pin = isAnalog ? 3 : 12;   // D2 or A2 (ADC1_CH2)
      break;
    case 3:
      mapped_pin = isAnalog ? 4 : 13;   // D3 or A3 (ADC1_CH3)
      break;
    case 4:
      mapped_pin = 14;                   // D4
      break;
    case 5:
      mapped_pin = 0;                    // D5 (boot-strap)
      break;
    case 6:
      mapped_pin = 8;                    // D6 (boot-strap)
      break;
    case 7:
      mapped_pin = 9;                    // D7 (boot-strap)
      break;
    case 8:
      mapped_pin = 1;                    // A0 via total-index
      break;
    case 9:
      mapped_pin = 2;                    // A1 via total-index
      break;
    case 10:
      mapped_pin = 3;                    // A2 via total-index
      break;
    case 11:
      mapped_pin = 4;                    // A3 via total-index
      break;
    default:
      mapped_pin = INVALID_GPIO_NUMBER;
  }
#else
  // ESP32 WROOM base variant. 12 digital + 4 analog user pins.
  // Cases 0-3 keep isAnalog conditional so the framework's re-indexed analog
  // access (pin = total_index - MAX_DIGITAL_GPIO_PINS) resolves to ADC hw pins.
  switch ( pin ) {

    case 0:
      mapped_pin = isAnalog ? 32 : 4;   // D0 or A0 (ADC1_CH4)
      break;
    case 1:
      mapped_pin = isAnalog ? 33 : 5;   // D1 or A1 (ADC1_CH5)
      break;
    case 2:
      mapped_pin = isAnalog ? 34 : 13;  // D2 or A2 (ADC1_CH6, input-only)
      break;
    case 3:
      mapped_pin = isAnalog ? 35 : 14;  // D3 or A3 (ADC1_CH7, input-only)
      break;
    case 4:
      mapped_pin = 16;                  // D4
      break;
    case 5:
      mapped_pin = 17;                  // D5
      break;
    case 6:
      mapped_pin = 18;                  // D6 (VSPI CLK default)
      break;
    case 7:
      mapped_pin = 19;                  // D7 (VSPI MISO default)
      break;
    case 8:
      mapped_pin = 21;                  // D8 (I2C SDA default)
      break;
    case 9:
      mapped_pin = 22;                  // D9 (I2C SCL default)
      break;
    case 10:
      mapped_pin = 23;                  // D10 (VSPI MOSI default)
      break;
    case 11:
      mapped_pin = 25;                  // D11 (DAC1)
      break;
    case 12:
      mapped_pin = 32;                  // A0 via total-index (ADC1_CH4)
      break;
    case 13:
      mapped_pin = 33;                  // A1 via total-index (ADC1_CH5)
      break;
    case 14:
      mapped_pin = 34;                  // A2 via total-index (ADC1_CH6, input-only)
      break;
    case 15:
      mapped_pin = 35;                  // A3 via total-index (ADC1_CH7, input-only)
      break;
    default:
      mapped_pin = INVALID_GPIO_NUMBER; // out-of-range user pin
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
 * return current time in micro second (64-bit, monotonic since boot —
 * esp_timer_get_time() is the ESP-IDF high-resolution monotonic timer)
 */
uint64_t DeviceControlInterface::micros_now()
{
    return (uint64_t)esp_timer_get_time();
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
upgrade_status_t DeviceControlInterface::Upgrade(const char *path, const char *version, void *client)
{
    (void)version;

    if (nullptr == path) {
        return UPGRADE_STATUS_FAILED;
    }

#if defined(MAKE_STREAM_DIRECT_OTA_UPGRADE)

    Http_Client *http = (Http_Client *)client;
    if (nullptr == http) {
        LogE("DEVICE_UPGRADE_NO_CLIENT\n");
        return UPGRADE_STATUS_FAILED;
    }

    bool begin_called = false;
    int64_t got = http->DownloadStream(path, [&begin_called](const uint8_t *buf, uint32_t sz) -> bool {
        if (!begin_called) {
            begin_called = true;
            LogFmtI("OTA Size  : %u\n", sz);
            // uint32_t maxSketchSpace = (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;
            size_t begin_size = (nullptr == buf && sz > 0) ? (size_t)sz : (size_t)UPDATE_SIZE_UNKNOWN;
            if (!Update.begin(begin_size, U_FLASH)) {
                LogFmtE("DEVICE_UPGRADE_BEGIN_FAILED : %d\n", (int)Update.getError());
                return false;
            }
            if (nullptr == buf) return true;
        }
        __i_dvc_ctrl.yield();
        size_t written = Update.write((uint8_t*)buf, sz);
        if (written != sz) {
            LogFmtE("DEVICE_UPGRADE_WRITE_SHORT : %u/%u err=%d\n",
                    (unsigned)written, (unsigned)sz, (int)Update.getError());
            return false;
        }
        return true;
    });

    if (got <= 0 || !Update.end(false)) {
        LogFmtE("DEVICE_UPGRADE_END_FAILED : got=%d err=%d\n", (int)got, (int)Update.getError());
        return UPGRADE_STATUS_FAILED;
    }

    LogFmtS("DEVICE_UPGRADE_OK size=%d\n", (int)got);
    return UPGRADE_STATUS_SUCCESS;

#elif defined(MAKE_STORAGE_DEPENDENT_OTA_UPGRADE)

    Http_Client *http = (Http_Client *)client;
    if (nullptr == http) {
        LogE("DEVICE_UPGRADE_NO_CLIENT\n");
        return UPGRADE_STATUS_FAILED;
    }

    pdiutil::string tmp_path = __i_fs.getTempDirectory();
    if (tmp_path.size() == 0 || tmp_path[tmp_path.size()-1] != '/') tmp_path += '/';
    tmp_path += "fw.bin";

    __i_fs.deleteFile(tmp_path.c_str());

    int64_t got = http->DownloadFile(path, tmp_path.c_str());
    if (got <= 0) {
        LogFmtE("DEVICE_UPGRADE_DOWNLOAD_FAILED : %d\n", (int)got);
        __i_fs.deleteFile(tmp_path.c_str());
        return UPGRADE_STATUS_FAILED;
    }

    if (!Update.begin((size_t)got, U_FLASH)) {
        LogFmtE("DEVICE_UPGRADE_BEGIN_FAILED : %d\n", (int)Update.getError());
        __i_fs.deleteFile(tmp_path.c_str());
        return UPGRADE_STATUS_FAILED;
    }

    bool write_ok = true;
    int64_t bytes_read = __i_fs.readFile(tmp_path.c_str(), 1024, [&](char *data, uint32_t sz) -> bool {
        __i_dvc_ctrl.yield();
        size_t written = Update.write((uint8_t*)data, sz);
        if (written != sz) { write_ok = false; return false; }
        return true;
    });

    if (bytes_read != got) {
        LogFmtE("DEVICE_UPGRADE_READ_SHORT : %d/%d\n", (int)bytes_read, (int)got);
        write_ok = false;
    }

    __i_fs.deleteFile(tmp_path.c_str());

    if (!write_ok || !Update.end(false)) {
        LogFmtE("DEVICE_UPGRADE_END_FAILED : %d\n", (int)Update.getError());
        return UPGRADE_STATUS_FAILED;
    }

    LogS("DEVICE_UPGRADE_OK\n");
    return UPGRADE_STATUS_SUCCESS;

#endif
}

DeviceControlInterface __i_dvc_ctrl;
