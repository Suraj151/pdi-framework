/*********************** Device Control Interface *****************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st Jan 2024
******************************************************************************/

#include "DeviceControlInterface.h"
#include "SerialInterface.h"
#include <avr/wdt.h>

/**
 * Turns off a watchdog left running by a watchdog reset, before anything else
 * runs, so the reset brings the board up instead of cutting every boot short.
 */
void pdiDisableWdtAtBoot(void) __attribute__((naked, used, section(".init3")));
void pdiDisableWdtAtBoot(void)
{
    MCUSR = 0;
    wdt_disable();
}


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
  static const uint8_t digital[MAX_DIGITAL_GPIO_PINS] = { 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 22, 23, 24, 25 };
  static const uint8_t analog[MAX_ANALOG_GPIO_PINS] = { A0, A1, A2, A3, A4, A5, A6, A7 };

  if( isAnalog ){
    mapped_pin = (pin < MAX_ANALOG_GPIO_PINS) ? analog[pin] : INVALID_GPIO_NUMBER;
  }else{
    mapped_pin = (pin < MAX_DIGITAL_GPIO_PINS) ? digital[pin] : INVALID_GPIO_NUMBER;
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
    return pdiutil::safe_new<GpioBlinkerInterface>(pin, duration);
}

/**
 * Delete GpioBlinkerInterface instance.
 */
void DeviceControlInterface::releaseGpioBlinkerInstance(iGpioBlinkerInterface *instance)
{
    pdiutil::safe_delete(instance);
}

/**
 * Init device specific features here
 */
void DeviceControlInterface::initDeviceSpecificFeatures()
{
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
    wdt_disable();
    wdt_enable(WDTO_15MS);
    while(1) {}
}

/**
 * restart the device
 */
void DeviceControlInterface::restartDevice()
{
    resetDevice();
}

/**
 * device watchdog feed
 */
void DeviceControlInterface::feedWdt()
{
    wdt_reset();
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
    return 328;
}

/**
 * get device mac id if any
 */
pdiutil::string DeviceControlInterface::getDeviceMac()
{
    return pdiutil::string( "AvrMega2560" );
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
        LogI("Flash Key pressed : %d\n", m_flash_key_pressed);
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
    delay(_timeoutms); // delay in milliseconds
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
 * return current time in micro second (64-bit). AVR's micros() is 32-bit and
 * wraps every ~71.6 min; track wraps here to keep the returned value monotonic
 * across long uptimes. Must be called from loop() context, not from an ISR.
 */
uint64_t DeviceControlInterface::micros_now()
{
    static uint32_t s_prev = 0;
    static uint32_t s_wraps = 0;
    uint32_t now = micros();
    if (now < s_prev) s_wraps++;
    s_prev = now;
    return ((uint64_t)s_wraps << 32) | (uint64_t)now;
}

/**
 * log helper for utility
 */
void DeviceControlInterface::log(logger_type_t log_type, const char *content)
{
  #if ( defined(ENABLE_CONSOLE_LOG_ALL) || defined(ENABLE_CONSOLE_LOG_INFO) || defined(ENABLE_CONSOLE_LOG_ERROR) || defined(ENABLE_CONSOLE_LOG_WARNING) || defined(ENABLE_CONSOLE_LOG_SUCCESS) )
  __log_manager.log(log_type, RODT_ATTR("%s"), content);
  #endif
}

/**
 * yield
 */
void DeviceControlInterface::yield()
{
    delay(0);
}

/**
 * @brief Check and start if possible of measure stack in use for future functions
 */
char* ptToBeMesaureFrom = nullptr;
char* ptToBeMesaureTill = nullptr;
bool DeviceControlInterface::can_measure_stack() {
  
    noInterrupts();

    ptToBeMesaureTill = (char*)SP; // Get the current stack pointer
    int maxlimit = 300;       // Maximum stack size to measure

    // // Ensure ptToBeMesaureFrom does not go below the valid stack range
    // extern char __stack_start; // Start of the stack (defined by linker script)
    ptToBeMesaureFrom = ptToBeMesaureTill - maxlimit;
    // if (ptToBeMesaureFrom < &__stack_start) {
    //     ptToBeMesaureFrom = &__stack_start; // Clamp to the start of the stack
    // }

    // Fill the stack region with 0x55
    for (char* pt = ptToBeMesaureFrom; pt < ptToBeMesaureTill; pt++) {
        *pt = 0x55;
    }

    interrupts();

    return true;
}

/**
 * @brief Get used stack size of last function. Must call can_measure_stack once 
 * before using this function
 */
int64_t DeviceControlInterface::measure_lastfn_stack(){

  noInterrupts();

  int64_t ustack = -1;

  if( nullptr != ptToBeMesaureFrom && nullptr != ptToBeMesaureTill ){

    for (char* pt = ptToBeMesaureFrom; pt < ptToBeMesaureTill; pt++) {
      if(*pt != 0x55){
        ustack = ptToBeMesaureTill - pt;
        break;
      }
    }
  }

  ptToBeMesaureFrom = nullptr;
  ptToBeMesaureTill = nullptr;

  interrupts();

  return ustack;
}

#ifdef ENABLE_OTA_SERVICE
/**
 * Upgrade device with provided binary path and new version
 */
upgrade_status_t DeviceControlInterface::Upgrade(const char *path, const char *version, void *client)
{
    (void)path;
    (void)version;
    (void)client;
    return UPGRADE_STATUS_MAX;
}
#endif

DeviceControlInterface __i_dvc_ctrl;
