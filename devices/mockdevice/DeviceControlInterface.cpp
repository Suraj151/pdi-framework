/*********************** Device Control Interface *****************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 16th Aug 2026
******************************************************************************/

#include "DeviceControlInterface.h"
#ifdef ENABLE_NETWORK_SERVICE
#include "PingInterface.h"
#include "UdpInterface.h"
#endif
#ifdef ENABLE_SERIAL_SERVICE
#include "SerialInterface.h"
#include <utility/EventUtil.h>
#endif
#include <malloc.h>
#include <stdio.h>
#include <time.h>

/**
 * a mac that stays the same across runs so expectations can hardcode it
 */
static const char MOCKDEVICE_MAC[] = "02:00:00:AB:CD:EF";
static const uint32_t MOCKDEVICE_ID = 0x00C0FFEE;
static const uint32_t MOCKDEVICE_HEAP_SIZE = 4194304;

/**
 * DeviceControlInterface constructor.
 */
DeviceControlInterface::DeviceControlInterface() : m_boot_micros(0),
                                                   m_virtual_micros(0),
                                                   m_virtual_clock(false),
                                                   m_random_state(0x1F2E3D4Cu),
                                                   m_reset_count(0),
                                                   m_restart_count(0)
{
    m_boot_micros = host_micros();

    for (uint8_t i = 0; i < MAX_DIGITAL_GPIO_PINS; i++)
    {
        m_digital_value[i] = 0;
        m_pin_mode[i] = OFF;
    }

    for (uint8_t i = 0; i < MAX_ANALOG_GPIO_PINS; i++)
    {
        m_analog_value[i] = 0;
    }
}

/**
 * DeviceControlInterface destructor.
 */
DeviceControlInterface::~DeviceControlInterface()
{
}

/**
 * monotonic microseconds from the host, independent of wall clock changes
 */
uint64_t DeviceControlInterface::host_micros() const
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ((uint64_t)ts.tv_sec * 1000000ULL) + ((uint64_t)ts.tv_nsec / 1000ULL);
}

void DeviceControlInterface::gpioMode(GPIO_MODE mode, gpio_id_t pin)
{
    if (pin < MAX_DIGITAL_GPIO_PINS)
    {
        m_pin_mode[pin] = mode;
    }
}

void DeviceControlInterface::gpioWrite(GPIO_MODE mode, gpio_id_t pin, gpio_val_t value)
{
    if (ANALOG_WRITE == mode)
    {
        if (pin < MAX_ANALOG_GPIO_PINS)
        {
            m_analog_value[pin] = value;
        }
        return;
    }

    if (pin < MAX_DIGITAL_GPIO_PINS)
    {
        m_digital_value[pin] = value;
    }
}

gpio_val_t DeviceControlInterface::gpioRead(GPIO_MODE mode, gpio_id_t pin)
{
    if (ANALOG_READ == mode)
    {
        return pin < MAX_ANALOG_GPIO_PINS ? m_analog_value[pin] : 0;
    }

    return pin < MAX_DIGITAL_GPIO_PINS ? m_digital_value[pin] : 0;
}

gpio_id_t DeviceControlInterface::gpioFromPinMap(gpio_id_t pin, bool isAnalog)
{
    return pin;
}

bool DeviceControlInterface::isExceptionalGpio(gpio_id_t pin)
{
    for (uint8_t i = 0; i < sizeof(EXCEPTIONAL_GPIO_PINS); i++)
    {
        if (EXCEPTIONAL_GPIO_PINS[i] == pin)
        {
            return true;
        }
    }
    return false;
}

iGpioBlinkerInterface *DeviceControlInterface::createGpioBlinkerInstance(gpio_id_t pin, gpio_val_t duration)
{
    return pdiutil::safe_new<GpioBlinkerInterface>(pin, duration);
}

void DeviceControlInterface::releaseGpioBlinkerInstance(iGpioBlinkerInterface *instance)
{
    pdiutil::safe_delete(instance);
}

void DeviceControlInterface::initDeviceSpecificFeatures()
{
    if (nullptr != getTerminal(TERMINAL_TYPE_SERIAL))
    {
        getTerminal(TERMINAL_TYPE_SERIAL)->open();
    }
}

/**
 * a host process has nothing to reset, so the call is recorded for the caller
 * to observe instead of being carried out
 */
void DeviceControlInterface::resetDevice()
{
    m_reset_count++;
}

void DeviceControlInterface::restartDevice()
{
    m_restart_count++;
}

/**
 * nothing calls into a host descriptor or socket on its own, so the serve loop
 * is where waiting terminal input and udp datagrams get picked up
 */
void DeviceControlInterface::handleEvents()
{
#ifdef ENABLE_SERIAL_SERVICE
    if (__serial_uart.available())
    {
        serial_event_t e(SERIAL_IFACE_UART, SERIAL_IFACE_CMD, &__serial_uart);
        __utl_event.execute_event(EVENT_SERIAL_AVAILABLE, &e);
    }
#endif

#ifdef ENABLE_NETWORK_SERVICE
    UdpInterface::serviceAll();
    __i_ping.service();
#endif
}

void DeviceControlInterface::enableWdt(uint8_t mode_if_any)
{
}

void DeviceControlInterface::disableWdt()
{
}

void DeviceControlInterface::feedWdt()
{
}

void DeviceControlInterface::eraseConfig()
{
}

uint32_t DeviceControlInterface::getDeviceId()
{
    return MOCKDEVICE_ID;
}

pdiutil::string DeviceControlInterface::getDeviceMac()
{
    return pdiutil::string(MOCKDEVICE_MAC);
}

bool DeviceControlInterface::isDeviceFactoryRequested()
{
    return false;
}

iTerminalInterface *DeviceControlInterface::getTerminal(terminal_types_t terminal)
{
#ifdef ENABLE_SERIAL_SERVICE
    if (TERMINAL_TYPE_MAX > terminal)
    {
        return &__serial_uart;
    }
#endif
    return nullptr;
}

/**
 * hold execution for the given duration. a virtual clock moves forward instead
 * of sleeping, so a caller driving time never waits in real seconds.
 */
void DeviceControlInterface::wait(double timeoutms)
{
    if (timeoutms <= 0)
    {
        return;
    }

    if (m_virtual_clock)
    {
        m_virtual_micros += (uint64_t)(timeoutms * 1000.0);
        return;
    }

    struct timespec req;
    req.tv_sec = (time_t)(timeoutms / 1000.0);
    req.tv_nsec = (long)((timeoutms - ((double)req.tv_sec * 1000.0)) * 1000000.0);
    nanosleep(&req, nullptr);
}

uint32_t DeviceControlInterface::millis_now()
{
    return (uint32_t)(micros_now() / 1000ULL);
}

uint64_t DeviceControlInterface::micros_now()
{
    if (m_virtual_clock)
    {
        return m_virtual_micros;
    }
    return host_micros() - m_boot_micros;
}

/**
 * a seeded xorshift keeps a run reproducible, which a hardware source cannot
 */
uint32_t DeviceControlInterface::random_now()
{
    m_random_state ^= m_random_state << 13;
    m_random_state ^= m_random_state >> 17;
    m_random_state ^= m_random_state << 5;
    return m_random_state;
}

uint32_t DeviceControlInterface::get_free_heap()
{
    struct mallinfo2 info = mallinfo2();
    uint32_t used = (uint32_t)info.uordblks;
    return used < MOCKDEVICE_HEAP_SIZE ? (MOCKDEVICE_HEAP_SIZE - used) : 0;
}

uint32_t DeviceControlInterface::get_max_free_block()
{
    return get_free_heap();
}

/**
 * the terminal writes straight to its descriptor, so this path is flushed as it
 * goes or a captured session comes out in the wrong order
 */
void DeviceControlInterface::log(logger_type_t log_type, const char *content)
{
    if (nullptr != content)
    {
        fputs(content, stdout);
        fflush(stdout);
    }
}

/**
 * a caller that waits on an asynchronous result yields while it waits, and on
 * the sdk backed ports that is where the echo callbacks get their turn
 */
void DeviceControlInterface::yield()
{
#ifdef ENABLE_NETWORK_SERVICE
    __i_ping.service();
#endif
}

#ifdef ENABLE_OTA_SERVICE
upgrade_status_t DeviceControlInterface::Upgrade(const char *path, const char *version, void *client)
{
    return UPGRADE_STATUS_IGNORE;
}
#endif

void DeviceControlInterface::useVirtualClock(bool enable)
{
    if (enable && !m_virtual_clock)
    {
        m_virtual_micros = host_micros() - m_boot_micros;
    }
    m_virtual_clock = enable;
}

void DeviceControlInterface::advanceVirtualClock(uint64_t microseconds)
{
    if (m_virtual_clock)
    {
        m_virtual_micros += microseconds;
    }
}

void DeviceControlInterface::seedRandom(uint32_t seed)
{
    m_random_state = (0 == seed) ? 0xA5A5A5A5u : seed;
}

uint32_t DeviceControlInterface::getResetCount() const
{
    return m_reset_count;
}

uint32_t DeviceControlInterface::getRestartCount() const
{
    return m_restart_count;
}

void DeviceControlInterface::clearControlCounters()
{
    m_reset_count = 0;
    m_restart_count = 0;
}

void DeviceControlInterface::setAnalogGpioValue(gpio_id_t pin, gpio_val_t value)
{
    if (pin < MAX_ANALOG_GPIO_PINS)
    {
        m_analog_value[pin] = value;
    }
}

GPIO_MODE DeviceControlInterface::getGpioMode(gpio_id_t pin) const
{
    return pin < MAX_DIGITAL_GPIO_PINS ? m_pin_mode[pin] : OFF;
}

/**
 * GpioBlinkerInterface constructor.
 */
GpioBlinkerInterface::GpioBlinkerInterface(gpio_id_t pin, gpio_val_t duration) : m_pin(pin),
                                                                                m_duration(duration),
                                                                                m_running(false)
{
    this->start();
}

/**
 * GpioBlinkerInterface destructor.
 */
GpioBlinkerInterface::~GpioBlinkerInterface()
{
    this->stop();
}

void GpioBlinkerInterface::setConfig(gpio_id_t pin, gpio_val_t duration)
{
    m_pin = pin;
    m_duration = duration;
}

void GpioBlinkerInterface::updateConfig(gpio_id_t pin, gpio_val_t duration)
{
    bool wasrunning = m_running;
    this->stop();
    this->setConfig(pin, duration);
    if (wasrunning)
    {
        this->start();
    }
}

void GpioBlinkerInterface::start()
{
    m_running = true;
}

void GpioBlinkerInterface::stop()
{
    m_running = false;
}

bool GpioBlinkerInterface::isRunning()
{
    return m_running;
}

DeviceControlInterface __i_dvc_ctrl;
