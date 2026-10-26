/*********************** Device Control Interface *****************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 16th Aug 2026
******************************************************************************/

#include "DeviceControlInterface.h"

#ifdef ENABLE_NETWORK_SERVICE
#include "NetStackInterface.h"
#include <interface/pdi/impl/modules/netif/NetifRegistry.h>
#endif

#include <unistd.h>
#include <sys/utsname.h>

#ifndef MOCK_DEVICE_TEST
#include <stdlib.h>

namespace {
  char **g_saved_argv = nullptr;
}

/**
 * Remembers the argument vector so a restart can re-exec with it. A host that
 * never calls this simply exits instead of re-execing.
 */
void pdi_posix_save_argv(char **argv)
{
    g_saved_argv = argv;
}

char **pdi_posix_saved_argv()
{
    return g_saved_argv;
}
#endif
#ifdef ENABLE_NETWORK_SERVICE
#include "PingInterface.h"
#include "UdpInterface.h"
#endif
#ifdef ENABLE_SERIAL_SERVICE
#include "SerialInterface.h"
#include <utility/EventUtil.h>
#endif
#ifdef ENABLE_DRIVER_CORE
#include <interface/pdi/impl/drivers/BuiltInDrivers.h>
#endif
#include <malloc.h>
#include <stdio.h>
#include <time.h>
#include <string.h>

/**
 * a mac that stays the same across runs so expectations can hardcode it
 */
static const char PDI_POSIX_MAC[] = "02:00:00:AB:CD:EF";
static const uint32_t PDI_POSIX_ID = 0x00C0FFEE;
static const uint32_t PDI_POSIX_HEAP_SIZE = 4194304;

/**
 * DeviceControlInterface constructor.
 */
DeviceControlInterface::DeviceControlInterface() : m_boot_micros(0),
                                                   m_virtual_micros(0),
                                                   m_virtual_clock(false),
                                                   m_random_state(0x1F2E3D4Cu),
                                                   m_free_heap_cap(0),
                                                   m_max_block_cap(0)
#ifdef MOCK_DEVICE_TEST
                                                   , m_reset_count(0)
                                                   , m_restart_count(0)
#endif
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
    if (pin < MAX_DIGITAL_GPIO_PINS)
    {
        m_digital_value[pin] = value;
    }
}

gpio_val_t DeviceControlInterface::gpioRead(GPIO_MODE mode, gpio_id_t pin)
{
    gpio_val_t injected = 0;

    if (ANALOG_READ == mode)
    {
        if (readGpioInputFile('A', pin, injected))
        {
            return injected;
        }
        return pin < MAX_ANALOG_GPIO_PINS ? m_analog_value[pin] : 0;
    }

    if (DIGITAL_READ == mode && readGpioInputFile('D', pin, injected))
    {
        return injected;
    }

    return pin < MAX_DIGITAL_GPIO_PINS ? m_digital_value[pin] : 0;
}

const char *DeviceControlInterface::s_gpio_input_file = nullptr;

/**
 * @brief Takes input pin levels from a file of `D4 1` / `A0 512` lines, re-read on every input sample.
 */
void DeviceControlInterface::setGpioInputFile(const char *path)
{
    s_gpio_input_file = path;
}

const char *DeviceControlInterface::s_heap_input_file = nullptr;
uint32_t DeviceControlInterface::s_heap_file_free = 0;
uint32_t DeviceControlInterface::s_heap_file_block = 0;
uint64_t DeviceControlInterface::s_heap_file_read_at = 0;

/**
 * @brief Takes heap limits from a file of `free N` / `block N` lines, re-read at most every 50 ms.
 */
void DeviceControlInterface::setHeapInputFile(const char *path)
{
    s_heap_input_file = path;
    s_heap_file_read_at = 0;
}

/**
 * @brief Takes the heap limits the input file holds, once its re-read interval has passed.
 */
void DeviceControlInterface::readHeapInputFile()
{
    if (nullptr == s_heap_input_file)
    {
        return;
    }

    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    uint64_t millis = (uint64_t)now.tv_sec * 1000u + (uint64_t)(now.tv_nsec / 1000000);
    if (0 != s_heap_file_read_at && (millis - s_heap_file_read_at) < 50u)
    {
        return;
    }
    s_heap_file_read_at = millis;

    uint32_t free_limit = 0;
    uint32_t block_limit = 0;
    FILE *file = fopen(s_heap_input_file, "r");
    if (nullptr != file)
    {
        char key[8] = {0};
        unsigned long value = 0;
        while (2 == fscanf(file, " %7s %lu", key, &value))
        {
            if (0 == strcmp(key, "free"))
            {
                free_limit = (uint32_t)value;
            }
            else if (0 == strcmp(key, "block"))
            {
                block_limit = (uint32_t)value;
            }
        }
        fclose(file);
    }
    s_heap_file_free = free_limit;
    s_heap_file_block = block_limit;
}

/**
 * @brief The level the input file gives a pin, or false when it names none.
 */
bool DeviceControlInterface::readGpioInputFile(char kind, gpio_id_t pin, gpio_val_t &value)
{
    if (nullptr == s_gpio_input_file)
    {
        return false;
    }

    FILE *file = fopen(s_gpio_input_file, "r");
    if (nullptr == file)
    {
        return false;
    }

    bool found = false;
    char label = 0;
    unsigned int number = 0;
    unsigned int level = 0;
    while (3 == fscanf(file, " %c%u %u", &label, &number, &level))
    {
        if ((label == kind || label == kind + 32) && number == pin)
        {
            value = (gpio_val_t)level;
            found = true;
        }
    }

    fclose(file);
    return found;
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
#ifdef ENABLE_DRIVER_CORE
    if (isDriverGpio(gpioFromPinMap(pin)))
    {
        return true;
    }
#endif
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

#ifdef ENABLE_NETWORK_SERVICE
    __netif_registry.registerStack(&__i_net_stack);
#endif

#ifdef ENABLE_DRIVER_CORE
    registerBuiltInDrivers();
#endif
}

/**
 * a host process has nothing to reset, so the call is recorded for the caller
 * to observe instead of being carried out
 */
void DeviceControlInterface::resetDevice()
{
#ifdef MOCK_DEVICE_TEST
    m_reset_count++;
#else
    restartDevice();
#endif
}

void DeviceControlInterface::restartDevice()
{
#ifdef MOCK_DEVICE_TEST
    m_restart_count++;
#else
    fflush(nullptr);

    // a host process restarts by becoming itself again, so the arguments it
    // was started with carry over the way a reboot preserves a board's setup
    char **argv = pdi_posix_saved_argv();
    if (nullptr != argv)
    {
        execv("/proc/self/exe", argv);
    }

    // execv only returns having failed, and a caller that asked for a restart
    // is not expecting to be handed control back
    _exit(0);
#endif
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
    return PDI_POSIX_ID;
}

pdiutil::string DeviceControlInterface::getDeviceMac()
{
    return pdiutil::string(PDI_POSIX_MAC);
}

/**
 * Fills in what this port can report about the hardware it runs on, leaving
 * untouched whatever it has no answer for.
 */
void DeviceControlInterface::getDeviceInfo(device_info_t &_out)
{
    struct utsname host;
    memset(&host, 0, sizeof(host));

    if (0 == uname(&host)) {
        _out.m_model = pdiutil::string(host.machine);
        _out.m_platform_version = pdiutil::string(host.sysname);
        _out.m_platform_version += CHARPTR_WRAP(" ");
        _out.m_platform_version += pdiutil::string(host.release);
    }

    int32_t cores = (int32_t)sysconf(_SC_NPROCESSORS_ONLN);
    _out.m_cores = cores > 0 ? (uint8_t)cores : 1;
    _out.m_restart_reason = CHARPTR_WRAP("process start");
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
    uint32_t free_heap = used < PDI_POSIX_HEAP_SIZE ? (PDI_POSIX_HEAP_SIZE - used) : 0;
    readHeapInputFile();
    if (0 != s_heap_file_free && free_heap > s_heap_file_free)
    {
        free_heap = s_heap_file_free;
    }
    return (0 != m_free_heap_cap && free_heap > m_free_heap_cap) ? m_free_heap_cap : free_heap;
}

uint32_t DeviceControlInterface::get_max_free_block()
{
    uint32_t block = get_free_heap();
    if (0 != s_heap_file_block && block > s_heap_file_block)
    {
        block = s_heap_file_block;
    }
    return (0 != m_max_block_cap && block > m_max_block_cap) ? m_max_block_cap : block;
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

/**
 * Caps the free heap this port reports, so a test can drive the heap check.
 */
void DeviceControlInterface::capFreeHeap(uint32_t bytes)
{
    m_free_heap_cap = bytes;
}

/**
 * @brief Report at most bytes as the largest free block, as a fragmented heap does; 0 reports the free heap.
 */
void DeviceControlInterface::capMaxFreeBlock(uint32_t bytes)
{
    m_max_block_cap = bytes;
}

#ifdef MOCK_DEVICE_TEST

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

#endif

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
