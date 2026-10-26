/****************************** Serial Interface ******************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 16th Aug 2026
******************************************************************************/

#include "SerialInterface.h"
#include "DeviceControlInterface.h"
#include <utility/DataTypeConversions.h>
#include <utility/Checksum.h>
#include <errno.h>
#include <time.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// initialize instances with respective type or nullptr
iSerialInterface *iSerialInterface::instances[SERIAL_IFACE_MAX] = {
    &__serial_uart  // SERIAL_IFACE_UART
    ,&__serial_uart1 // SERIAL_IFACE_UART1
    ,&__serial_i2c  // SERIAL_IFACE_I2C
    ,nullptr        // SERIAL_IFACE_I2C1
    ,&__serial_spi  // SERIAL_IFACE_SPI
    ,nullptr        // SERIAL_IFACE_SPI1
    ,nullptr        // SERIAL_IFACE_CAN
    ,nullptr        // SERIAL_IFACE_CAN1
    ,nullptr        // SERIAL_IFACE_CMD
    ,nullptr        // SERIAL_IFACE_IOT
    ,&__serial_i2s  // SERIAL_IFACE_I2S
    ,&__serial_usb_host // SERIAL_IFACE_USB_HOST
    ,nullptr        // SERIAL_IFACE_USB_DEVICE
};

/**
 * UARTSerial constructor.
 */
UARTSerial::UARTSerial(int readfd, int writefd) : m_connected(false),
                                                  m_port(0),
                                                  m_speed(0),
                                                  m_timeout(0),
                                                  m_readfd(readfd),
                                                  m_writefd(writefd),
                                                  m_peeked(-1)
{
}

/**
 * UARTSerial destructor.
 */
UARTSerial::~UARTSerial()
{
}

int16_t UARTSerial::connect(uint16_t port, uint64_t speed)
{
    m_port = port;
    m_speed = speed;
    m_connected = (m_readfd >= 0 || m_writefd >= 0);
    return m_connected ? 0 : -1;
}

int16_t UARTSerial::disconnect()
{
    m_connected = false;
    return 0;
}

int32_t UARTSerial::write(uint8_t c)
{
    return this->write(&c, 1);
}

int32_t UARTSerial::write(const uint8_t *c_str)
{
    if (nullptr == c_str)
    {
        return 0;
    }
    return this->write(c_str, (uint32_t)strlen((const char *)c_str));
}

int32_t UARTSerial::write(const uint8_t *c_str, uint32_t size)
{
    if (nullptr == c_str || 0 == size || m_writefd < 0)
    {
        return 0;
    }

    uint32_t written = 0;
    while (written < size)
    {
        ssize_t n = ::write(m_writefd, c_str + written, size - written);
        if (n <= 0)
        {
            if (n < 0 && EINTR == errno)
            {
                continue;
            }
            break;
        }
        written += (uint32_t)n;
    }

    return (int32_t)written;
}

int32_t UARTSerial::write_ro(const char *c_str)
{
    return this->write((const uint8_t *)c_str);
}

/**
 * pull one byte into the peek slot so available() can answer without consuming
 */
bool UARTSerial::fill_peek()
{
    if (m_peeked >= 0)
    {
        return true;
    }

    if (m_readfd < 0)
    {
        return false;
    }

    struct pollfd pfd;
    pfd.fd = m_readfd;
    pfd.events = POLLIN;
    pfd.revents = 0;

    if (poll(&pfd, 1, 0) <= 0)
    {
        return false;
    }

    uint8_t c = 0;
    ssize_t n = ::read(m_readfd, &c, 1);
    if (n != 1)
    {
        return false;
    }

    m_peeked = (int16_t)c;
    return true;
}

uint8_t UARTSerial::read()
{
    if (!fill_peek())
    {
        return 0;
    }

    uint8_t c = (uint8_t)m_peeked;
    m_peeked = -1;
    return c;
}

int32_t UARTSerial::read(uint8_t *buf, uint32_t size)
{
    if (nullptr == buf || 0 == size)
    {
        return 0;
    }

    uint32_t count = 0;
    while (count < size && fill_peek())
    {
        buf[count++] = (uint8_t)m_peeked;
        m_peeked = -1;
    }

    return (int32_t)count;
}

int32_t UARTSerial::available()
{
    return fill_peek() ? 1 : 0;
}

int8_t UARTSerial::connected()
{
    return m_connected ? 1 : 0;
}

void UARTSerial::setTimeout(uint32_t timeout)
{
    m_timeout = timeout;
}

/**
 * @brief With timestamp will print timestamp first
 * derived class should implement this function to print timestamp
 * @param None
 * @return this
 */
iTerminalInterface* UARTSerial::with_timestamp()
{
    static uint64_t s_started = 0;

    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);

    uint64_t elapsed = ((uint64_t)now.tv_sec * 1000000ULL) + ((uint64_t)now.tv_nsec / 1000ULL);
    if (0 == s_started) s_started = elapsed;

    char tembuff[21];
    MicrosToTimeString(elapsed - s_started, tembuff, sizeof(tembuff));

    write('[');
    write_pad(tembuff, 15, true);
    write(']');

    return this;
}

void UARTSerial::flush(int16_t flushtype)
{
    if (IsFlushTx(flushtype) && m_writefd >= 0)
    {
        fsync(m_writefd);
    }

    // modelled rather than skipped: a discard the host device does not perform
    // is a whole class of defect the host tier cannot see
    if (IsFlushRx(flushtype))
    {
        m_peeked = -1;
        while (available() > 0)
        {
            read();
        }
    }
}

void UARTSerial::setDescriptors(int readfd, int writefd)
{
    m_readfd = readfd;
    m_writefd = writefd;
    m_peeked = -1;
}

UARTSerial __serial_uart(STDIN_FILENO, STDOUT_FILENO);
UARTSerial __serial_uart1(-1, -1);

const char *SimulatedBus::s_input_file = nullptr;

/**
 * @brief Takes what is plugged in from this file from now on.
 */
void SimulatedBus::setInputFile(const char *path)
{
    s_input_file = path;
}

/**
 * @brief Hands each line naming the bus to the visitor, until it answers false.
 */
void SimulatedBus::eachLine(const char *bus, bool (*visit)(const char *rest, void *arg), void *arg)
{
    if (nullptr == s_input_file || nullptr == bus || nullptr == visit)
    {
        return;
    }

    FILE *file = fopen(s_input_file, "r");
    if (nullptr == file)
    {
        return;
    }

    size_t buslen = strlen(bus);
    char line[160];
    while (nullptr != fgets(line, sizeof(line), file))
    {
        const char *p = line;
        while (' ' == *p || '\t' == *p)
        {
            p++;
        }
        if (0 != strncmp(p, bus, buslen) || (' ' != p[buslen] && '\t' != p[buslen]))
        {
            continue;
        }
        if (!visit(p + buslen, arg))
        {
            break;
        }
    }

    fclose(file);
}

namespace
{

struct ChipQuery
{
    uint32_t m_address;
    bool m_found;
    uint8_t m_regs[64];
    uint8_t m_count;
};

/**
 * @brief Takes the chip's register bytes when the line names its address.
 */
bool findChip(const char *rest, void *arg)
{
    ChipQuery *query = static_cast<ChipQuery *>(arg);
    char *end = nullptr;
    unsigned long address = strtoul(rest, &end, 0);
    if (end == rest || address != query->m_address)
    {
        return true;
    }

    query->m_found = true;
    query->m_count = 0;
    const char *p = end;
    while (query->m_count < sizeof(query->m_regs))
    {
        unsigned long value = strtoul(p, &end, 16);
        if (end == p)
        {
            break;
        }
        query->m_regs[query->m_count++] = (uint8_t)value;
        p = end;
    }
    return false;
}

struct UsbQuery
{
    uint8_t m_index;
    uint8_t m_seen;
    usb_device_info_t m_info;
    bool m_found;
};

/**
 * @brief Counts each device line, and takes the one at the index asked for.
 */
bool findUsb(const char *rest, void *arg)
{
    UsbQuery *query = static_cast<UsbQuery *>(arg);
    unsigned int address = 0;
    unsigned int vid = 0;
    unsigned int pid = 0;
    unsigned int cls = 0;
    if (4 != sscanf(rest, " %u %x %x %x", &address, &vid, &pid, &cls))
    {
        return true;
    }

    if (query->m_seen == query->m_index)
    {
        query->m_info.m_address = (uint8_t)address;
        query->m_info.m_vid = (uint16_t)vid;
        query->m_info.m_pid = (uint16_t)pid;
        query->m_info.m_class = (uint8_t)cls;
        query->m_found = true;
    }
    query->m_seen++;
    return true;
}

}

/**
 * @brief I2cSerial constructor, for the bus with this number.
 */
I2cSerial::I2cSerial(uint8_t bus)
{
    snprintf(m_name, sizeof(m_name), "i2c%u", (unsigned)bus);
}

/**
 * @brief Whether the simulated bus file has a chip at the address.
 */
bool I2cSerial::probe(uint32_t target)
{
    ChipQuery query;
    query.m_address = target;
    query.m_found = false;
    query.m_count = 0;
    SimulatedBus::eachLine(m_name, findChip, &query);
    return query.m_found;
}

/**
 * @brief Reads the selected chip's registers from the one tx names; writes are taken and dropped.
 */
int32_t I2cSerial::transfer(const uint8_t *tx, uint32_t txlen, uint8_t *rx, uint32_t rxlen)
{
    ChipQuery query;
    query.m_address = m_selected;
    query.m_found = false;
    query.m_count = 0;
    SimulatedBus::eachLine(m_name, findChip, &query);
    if (!query.m_found)
    {
        return PDI_ERR_NOT_FOUND;
    }

    uint8_t reg = (nullptr != tx && txlen > 0) ? tx[0] : 0;
    for (uint32_t i = 0; i < rxlen && nullptr != rx; i++)
    {
        rx[i] = (reg + i < query.m_count) ? query.m_regs[reg + i] : 0xFF;
    }
    return (int32_t)(rxlen > 0 ? rxlen : txlen);
}

/**
 * @brief How many devices the simulated bus file plugs into the port.
 */
uint8_t UsbHostSerial::deviceCount()
{
    UsbQuery query;
    query.m_index = 0xFF;
    query.m_seen = 0;
    query.m_found = false;
    SimulatedBus::eachLine("usb0", findUsb, &query);
    return query.m_seen;
}

/**
 * @brief The descriptors the simulated bus file gives one device.
 */
bool UsbHostSerial::deviceAt(uint8_t index, usb_device_info_t &out)
{
    UsbQuery query;
    query.m_index = index;
    query.m_seen = 0;
    query.m_found = false;
    SimulatedBus::eachLine("usb0", findUsb, &query);
    if (query.m_found)
    {
        out = query.m_info;
    }
    return query.m_found;
}

/**
 * @brief A simulated device has no endpoints to move bytes on.
 */
int32_t UsbHostSerial::transfer(const uint8_t *tx, uint32_t txlen, uint8_t *rx, uint32_t rxlen)
{
    return PDI_ERR_NOT_SUPPORTED;
}

namespace
{

/**
 * @brief Notes whether the file plugs a microphone that gives changing samples.
 */
bool findNoise(const char *rest, void *arg)
{
    char word[16] = {0};
    if (1 == sscanf(rest, " %15s", word) && 0 == strcmp(word, "noise"))
    {
        *static_cast<bool *>(arg) = true;
        return false;
    }
    return true;
}

}

/**
 * @brief I2sSerial constructor.
 */
I2sSerial::I2sSerial() : m_configured(false), m_state(0x2545F491u)
{
}

/**
 * @brief Takes the configuration; a simulated bus can run any of them.
 */
pdi_err_t I2sSerial::configure(const serial_i2s_config_t &config)
{
    m_configured = true;
    return PDI_OK;
}

/**
 * @brief Stops the simulated bus.
 */
void I2sSerial::release()
{
    m_configured = false;
}

/**
 * @brief Played samples go nowhere.
 */
int32_t I2sSerial::write(const uint8_t *c_str, uint32_t size)
{
    return m_configured ? (int32_t)size : 0;
}

/**
 * @brief Samples from the simulated microphone: noise when the file says so, silence otherwise.
 */
int32_t I2sSerial::read(uint8_t *buf, uint32_t size)
{
    if (!m_configured || nullptr == buf)
    {
        return 0;
    }

    bool noise = false;
    SimulatedBus::eachLine("i2s0", findNoise, &noise);

    for (uint32_t i = 0; i < size; i++)
    {
        if (noise)
        {
            m_state ^= m_state << 13;
            m_state ^= m_state >> 17;
            m_state ^= m_state << 5;
        }
        buf[i] = noise ? (uint8_t)m_state : 0;
    }
    return (int32_t)size;
}

/**
 * @brief The pins the bus runs on when its line names none: SCK, WS, SD.
 */
uint8_t I2sSerial::defaultPins(uint8_t *out, uint8_t max) const
{
    uint8_t n = 0;
#if defined(ENABLE_DRIVER_CORE) && defined(ENABLE_I2S_BUS)
    for (; n < sizeof(I2S_GPIO_PINS) && n < max && nullptr != out; n++)
    {
        out[n] = I2S_GPIO_PINS[n];
    }
#endif
    return n;
}

/**
 * @brief The pins the bus runs on when no bus line names its own: SDA, SCL.
 */
uint8_t I2cSerial::defaultPins(uint8_t *out, uint8_t max) const
{
    uint8_t n = 0;
#if defined(ENABLE_DRIVER_CORE) && defined(ENABLE_I2C_BUS)
    for (; n < sizeof(I2C_GPIO_PINS) && n < max && nullptr != out; n++)
    {
        out[n] = I2C_GPIO_PINS[n];
    }
#endif
    return n;
}

namespace
{

const uint8_t s_sd_data_token = 0xFE;
const uint8_t s_sd_illegal_command = 0x04;
const uint8_t s_sd_command_crc_error = 0x08;
const uint8_t s_sd_parameter_error = 0x40;
const uint8_t s_sd_crc_rejected = 0x0B;
const uint8_t s_sd_accepted = 0x05;
const uint8_t s_sd_max_cards = 4;

/**
 * @brief The 16 bit CRC an SD card puts after a data block.
 */
uint16_t sdCrc16(const uint8_t *data, uint32_t len)
{
    return crc16CcittUpdate(0, data, len);
}

struct SdQuery
{
    int16_t m_cs;
    bool m_found;
    bool m_highcapacity;
    uint32_t m_kilobytes;
};

/**
 * @brief Takes the card's kind and size when the line names its chip select.
 */
bool findCard(const char *rest, void *arg)
{
    SdQuery *query = static_cast<SdQuery *>(arg);
    int cs = -1;
    char kind[4] = {0};
    char hc[4] = {0};
    unsigned int kilobytes = 0;
    if (4 != sscanf(rest, " %d %3s %3s %u", &cs, kind, hc, &kilobytes) || cs != query->m_cs ||
        0 != strcmp(kind, "sd"))
    {
        return true;
    }
    query->m_found = true;
    query->m_highcapacity = (0 == strcmp(hc, "hc"));
    query->m_kilobytes = kilobytes;
    return false;
}

struct SdSlot
{
    int16_t m_cs;
    bool m_highcapacity;
    uint32_t m_kilobytes;
    SdCardSim *m_card;
};

SdSlot s_sd_slots[s_sd_max_cards] = {};

}

/**
 * @brief SdCardSim constructor, a card of the kind and blocks given.
 */
SdCardSim::SdCardSim(bool highcapacity, uint32_t blocks)
    : m_present(true), m_v1(false), m_mmc(false), m_no_crc_command(false), m_wrong_voltage(false),
      m_ultra_capacity(false), m_never_ready(false), m_ready_after(2),
      m_read_error(0), m_bad_crc(false), m_write_response(s_sd_accepted), m_busy_bytes(3),
      m_never_done(false), m_silent_command(-1), m_silent_count(-1), m_refused_command(-1), m_bad_echo(false), m_no_token(false),
      m_no_write_response(false), m_fail_read_at(0), m_reads(0), m_writes(0), m_clocks_deselected(0),
      m_highcapacity(highcapacity), m_blocks(blocks), m_mem(nullptr), m_selected(false),
      m_asleep(false), m_idle(true), m_app(false), m_crc_on(false), m_state(SIM_COMMAND), m_cmdlen(0), m_outhead(0), m_outtail(0),
      m_inlen(0), m_write_offset(0), m_busy_left(0)
{
    m_store.assign((size_t)blocks * SD_BLOCK_SIZE, 0);
    m_mem = m_store.data();
}

/**
 * @brief SdCardSim destructor.
 */
SdCardSim::~SdCardSim()
{
}

/**
 * @brief The chip select going low or high; a card let go drops what it was half
 * saying or hearing.
 */
void SdCardSim::select(bool selected)
{
    m_selected = selected;
    if (!selected)
    {
        m_cmdlen = 0;
        m_outhead = m_outtail = 0;
        if (SIM_WRITE_TOKEN == m_state || SIM_WRITE_DATA == m_state)
        {
            m_state = SIM_COMMAND;
        }
    }
}

/**
 * The card put back in the slot, powered up afresh: idle, CRC off, needing to be
 * woken and reset again.
 */
void SdCardSim::reinsert()
{
    m_present = true;
    m_asleep = true;
    m_idle = true;
    m_app = false;
    m_crc_on = false;
    m_state = SIM_COMMAND;
    m_cmdlen = 0;
    m_outhead = m_outtail = 0;
}

/**
 * @brief Puts a byte on the queue the card answers from.
 */
void SdCardSim::queue(uint8_t b)
{
    if (m_outtail < sizeof(m_out))
    {
        m_out[m_outtail++] = b;
    }
}

/**
 * Puts a data block on the queue: its token, the bytes and their CRC, a placeholder
 * 0xFFFF until CRC is on.
 */
void SdCardSim::queueBlock(const uint8_t *block, uint32_t len, bool data)
{
    queue(s_sd_data_token);
    for (uint32_t i = 0; i < len; i++)
    {
        queue(block[i]);
    }
    uint16_t crc = (data && !m_crc_on) ? 0xFFFF : sdCrc16(block, len);
    if (m_bad_crc)
    {
        crc ^= 0xFFFF;
    }
    queue((uint8_t)(crc >> 8));
    queue((uint8_t)(crc & 0xFF));
}

/**
 * @brief Where in the card a read or write argument points, false past the
 *        end or, on a standard card, off a block edge.
 */
bool SdCardSim::blockAt(uint32_t arg, uint32_t &offset) const
{
    uint64_t at = m_highcapacity ? (uint64_t)arg * SD_BLOCK_SIZE : (uint64_t)arg;
    if (!m_highcapacity && 0 != (arg % SD_BLOCK_SIZE))
    {
        return false;
    }
    if (at + SD_BLOCK_SIZE > (uint64_t)m_blocks * SD_BLOCK_SIZE)
    {
        return false;
    }
    offset = (uint32_t)at;
    return true;
}

/**
 * @brief The card's CSD register, giving its size the way its kind does.
 */
void SdCardSim::csd(uint8_t *out) const
{
    memset(out, 0, 16);
    out[5] = 0x59;
    if (m_ultra_capacity)
    {
        out[0] = 0x80;
        return;
    }
    if (m_highcapacity)
    {
        uint32_t size = m_blocks / 1024 - 1;
        out[0] = 0x40;
        out[7] = (uint8_t)((size >> 16) & 0x3F);
        out[8] = (uint8_t)((size >> 8) & 0xFF);
        out[9] = (uint8_t)(size & 0xFF);
        return;
    }
    uint32_t size = m_blocks / 512 - 1;
    uint8_t mult = 7;
    out[6] = (uint8_t)((size >> 10) & 0x03);
    out[7] = (uint8_t)((size >> 2) & 0xFF);
    out[8] = (uint8_t)((size & 0x03) << 6);
    out[9] = (uint8_t)((mult >> 1) & 0x03);
    out[10] = (uint8_t)((mult & 0x01) << 7);
}

/**
 * @brief Answers the command just heard.
 */
void SdCardSim::command()
{
    uint8_t index = m_cmd[0] & 0x3F;
    uint32_t arg = ((uint32_t)m_cmd[1] << 24) | ((uint32_t)m_cmd[2] << 16) | ((uint32_t)m_cmd[3] << 8) | m_cmd[4];
    uint8_t r1 = m_idle ? 0x01 : 0x00;
    bool app = m_app;
    m_app = false;
    uint32_t offset = 0;

    queue(0xFF);

    if (m_crc_on && 0 != index && m_cmd[5] != (uint8_t)((crc7(m_cmd, 5) << 1) | 1))
    {
        queue(r1 | s_sd_command_crc_error);
        return;
    }

    if (m_asleep && 0 != index)
    {
        return;
    }

    if (m_silent_command == (int)index && 0 != m_silent_count)
    {
        if (m_silent_count > 0)
        {
            m_silent_count--;
        }
        return;
    }

    if (m_refused_command == (int)index)
    {
        queue(r1 | s_sd_parameter_error);
        return;
    }

    if (app && 41 == index)
    {
        if (!m_never_ready && --m_ready_after <= 0)
        {
            m_idle = false;
        }
        queue(m_idle ? 0x01 : 0x00);
        return;
    }

    switch (index)
    {
    case 0:
        m_idle = true;
        m_asleep = false;
        m_crc_on = false;
        queue(0x01);
        break;
    case 8:
        if (m_v1 || m_mmc)
        {
            queue(r1 | s_sd_illegal_command);
            break;
        }
        queue(r1);
        queue(0);
        queue(0);
        queue(m_wrong_voltage ? (uint8_t)0x00 : (uint8_t)((arg >> 8) & 0x0F));
        queue(m_bad_echo ? (uint8_t)~(arg & 0xFF) : (uint8_t)(arg & 0xFF));
        break;
    case 55:
        if (m_mmc)
        {
            queue(r1 | s_sd_illegal_command);
            break;
        }
        m_app = true;
        queue(r1);
        break;
    case 58:
    {
        uint32_t ocr = 0x00FF8000UL | (m_idle ? 0 : 0x80000000UL) | ((m_highcapacity && !m_idle) ? 0x40000000UL : 0);
        queue(r1);
        queue((uint8_t)(ocr >> 24));
        queue((uint8_t)(ocr >> 16));
        queue((uint8_t)(ocr >> 8));
        queue((uint8_t)ocr);
        break;
    }
    case 9:
    {
        uint8_t reg[16];
        csd(reg);
        queue(r1);
        queue(0xFF);
        if (!m_no_token)
        {
            queueBlock(reg, sizeof(reg), false);
        }
        break;
    }
    case 13:
        queue(r1);
        queue(0);
        break;
    case 16:
        queue((SD_BLOCK_SIZE == arg) ? r1 : (uint8_t)(r1 | s_sd_parameter_error));
        break;
    case 59:
        if (m_no_crc_command)
        {
            queue(r1 | s_sd_illegal_command);
            break;
        }
        m_crc_on = (0 != (arg & 0x01));
        queue(r1);
        break;
    case 17:
        if (!blockAt(arg, offset))
        {
            queue(r1 | s_sd_parameter_error);
            break;
        }
        queue(r1);
        queue(0xFF);
        m_reads++;
        if (0 != m_read_error || m_fail_read_at == m_reads)
        {
            queue((0 != m_read_error) ? m_read_error : (uint8_t)0x08);
            break;
        }
        if (!m_no_token)
        {
            queueBlock(m_mem + offset, SD_BLOCK_SIZE, true);
        }
        break;
    case 24:
        if (!blockAt(arg, offset))
        {
            queue(r1 | s_sd_parameter_error);
            break;
        }
        queue(r1);
        m_write_offset = offset;
        m_state = SIM_WRITE_TOKEN;
        break;
    default:
        queue(r1 | s_sd_illegal_command);
        break;
    }
}

/**
 * @brief One byte clocked in from the host, and the one clocked out to it.
 */
uint8_t SdCardSim::exchange(uint8_t in)
{
    if (!m_present)
    {
        return 0xFF;
    }
    if (!m_selected)
    {
        m_clocks_deselected++;
        return 0xFF;
    }

    uint8_t out = 0xFF;
    if (m_outhead < m_outtail)
    {
        out = m_out[m_outhead++];
        if (m_outhead == m_outtail)
        {
            m_outhead = m_outtail = 0;
        }
    }
    else if (SIM_BUSY == m_state)
    {
        if (m_never_done || m_busy_left > 0)
        {
            m_busy_left--;
            out = 0x00;
        }
        else
        {
            m_state = SIM_COMMAND;
        }
    }

    switch (m_state)
    {
    case SIM_COMMAND:
        if (0 == m_cmdlen && 0x40 != (in & 0xC0))
        {
            break;
        }
        m_cmd[m_cmdlen++] = in;
        if (sizeof(m_cmd) == m_cmdlen)
        {
            m_cmdlen = 0;
            command();
        }
        break;
    case SIM_WRITE_TOKEN:
        if (s_sd_data_token == in)
        {
            m_state = SIM_WRITE_DATA;
            m_inlen = 0;
        }
        break;
    case SIM_WRITE_DATA:
        m_in[m_inlen++] = in;
        if (sizeof(m_in) == m_inlen)
        {
            uint8_t response = m_write_response;
            if (m_crc_on && sdCrc16(m_in, SD_BLOCK_SIZE) != (uint16_t)((m_in[SD_BLOCK_SIZE] << 8) | m_in[SD_BLOCK_SIZE + 1]))
            {
                response = s_sd_crc_rejected;
            }
            m_state = SIM_COMMAND;
            if (m_no_write_response)
            {
                break;
            }
            queue(response);
            if (s_sd_accepted == (response & 0x1F))
            {
                memcpy(m_mem + m_write_offset, m_in, SD_BLOCK_SIZE);
                m_writes++;
                m_state = SIM_BUSY;
                m_busy_left = m_busy_bytes;
            }
        }
        break;
    default:
        break;
    }

    return out;
}

/**
 * The card the simulated bus file plugs in behind the chip select, made when its
 * line shows and dropped when it goes.
 */
SdCardSim *SpiSerial::card(int16_t cs)
{
    SdQuery query;
    query.m_cs = cs;
    query.m_found = false;
    query.m_highcapacity = false;
    query.m_kilobytes = 0;
    SimulatedBus::eachLine("spi0", findCard, &query);

    SdSlot *free = nullptr;
    for (uint8_t i = 0; i < s_sd_max_cards; i++)
    {
        SdSlot &slot = s_sd_slots[i];
        if (nullptr == slot.m_card)
        {
            free = (nullptr == free) ? &slot : free;
            continue;
        }
        if (slot.m_cs != cs)
        {
            continue;
        }
        if (query.m_found && slot.m_highcapacity == query.m_highcapacity && slot.m_kilobytes == query.m_kilobytes)
        {
            return slot.m_card;
        }
        pdiutil::safe_delete(slot.m_card);
        slot.m_card = nullptr;
        free = &slot;
    }

    if (!query.m_found || nullptr == free)
    {
        return nullptr;
    }

    free->m_cs = cs;
    free->m_highcapacity = query.m_highcapacity;
    free->m_kilobytes = query.m_kilobytes;
    free->m_card = pdiutil::safe_new<SdCardSim>(query.m_highcapacity, query.m_kilobytes * 1024 / SD_BLOCK_SIZE);
    return free->m_card;
}

/**
 * @brief Takes the bus for the card behind the chip select.
 */
pdi_err_t SpiSerial::begin(int16_t cs, uint32_t clock, uint8_t mode)
{
    m_cs = cs;
    m_card = (cs >= 0) ? card(cs) : nullptr;
    if (nullptr != m_card)
    {
        m_card->select(true);
    }
    return PDI_OK;
}

/**
 * @brief Lets the card's chip select go.
 */
void SpiSerial::end()
{
    if (nullptr != m_card)
    {
        m_card->select(false);
    }
    m_card = nullptr;
    m_cs = -1;
}

/**
 * @brief Clocks the bytes through the card behind the chip select, 0xFF in
 *        from an empty slot or with no chip select low.
 */
int32_t SpiSerial::exchange(const uint8_t *tx, uint8_t *rx, uint32_t len)
{
    for (uint32_t i = 0; i < len; i++)
    {
        uint8_t out = (nullptr != m_card) ? m_card->exchange((nullptr != tx) ? tx[i] : 0xFF) : 0xFF;
        if (nullptr != rx)
        {
            rx[i] = out;
        }
    }
    return (int32_t)len;
}

/**
 * @brief The pins the bus runs on when no bus line names its own: SCK, MISO, MOSI.
 */
uint8_t SpiSerial::defaultPins(uint8_t *out, uint8_t max) const
{
    uint8_t n = 0;
#if defined(ENABLE_DRIVER_CORE) && defined(ENABLE_SPI_BUS)
    for (; n < sizeof(SPI_GPIO_PINS) && n < max && nullptr != out; n++)
    {
        out[n] = SPI_GPIO_PINS[n];
    }
#endif
    return n;
}

I2cSerial __serial_i2c(0);
UsbHostSerial __serial_usb_host;
I2sSerial __serial_i2s;
SpiSerial __serial_spi;
