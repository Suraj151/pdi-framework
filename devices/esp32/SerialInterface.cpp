/****************************** Serial Interface ******************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st June 2019
******************************************************************************/

#include "SerialInterface.h"
#include <utility/DataTypeConversions.h>
#include <esp_timer.h>


// extern int __heap_start, *__brkval; int v; 
// int ramavail = (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval); 
// LogI("available ram %d, %d\n", ramavail, sizeof(task_t));


// initialize instances with respective type or nullptr
iSerialInterface* iSerialInterface::instances[SERIAL_IFACE_MAX] = {
  &__serial_uart // SERIAL_IFACE_UART
  ,&__serial_uart1 // SERIAL_IFACE_UART1
#ifdef ENABLE_I2C_BUS
  ,&__serial_i2c // SERIAL_IFACE_I2C
  ,nullptr // SERIAL_IFACE_I2C1
#else
  ,nullptr // SERIAL_IFACE_I2C
  ,nullptr // SERIAL_IFACE_I2C1
#endif
#ifdef ENABLE_SPI_BUS
  ,&__serial_spi // SERIAL_IFACE_SPI
  ,nullptr // SERIAL_IFACE_SPI1
#else
  ,nullptr // SERIAL_IFACE_SPI
  ,nullptr // SERIAL_IFACE_SPI1
#endif
  ,nullptr // SERIAL_IFACE_CAN
  ,nullptr // SERIAL_IFACE_CAN1
  ,nullptr // SERIAL_IFACE_CMD
  ,nullptr // SERIAL_IFACE_IOT
#ifdef ENABLE_I2S_BUS
  ,&__serial_i2s // SERIAL_IFACE_I2S
#endif
};

/**
 * UARTSerial constructor.
 */
UARTSerial::UARTSerial(HardwareSerial& hwserial) : m_connected(false), m_port(0), m_speed(115200), m_hwserial(hwserial)
#if ARDUINO_USB_CDC_ON_BOOT
, m_cdcserial(nullptr)
#endif
{
}

#if ARDUINO_USB_CDC_ON_BOOT
UARTSerial::UARTSerial(HWCDC& cdcserial) : m_connected(false), m_port(0), m_speed(115200), m_hwserial(Serial0), m_cdcserial(&cdcserial)
{
}
#endif

/**
 * UARTSerial destructor.
 */
UARTSerial::~UARTSerial()
{
}

/**
 * connect
 */
int16_t UARTSerial::connect(uint16_t port, uint64_t speed)
{
  m_port = port;
  m_speed = speed;

#if ARDUINO_USB_CDC_ON_BOOT
  if (m_cdcserial) m_cdcserial->begin(speed);
  else
#endif
  m_hwserial.begin(speed);
  m_connected = true;
  return 1;
}

/**
 * disconnect
 */
int16_t UARTSerial::disconnect()
{
#if ARDUINO_USB_CDC_ON_BOOT
  if (m_cdcserial) m_cdcserial->end();
  else
#endif
  m_hwserial.end();
  m_connected = false;
  return 0;
}

/**
 * write
 */
int32_t UARTSerial::write(uint8_t c)
{
#if ARDUINO_USB_CDC_ON_BOOT
  if (m_cdcserial) return m_cdcserial->write(c);
#endif
  return m_hwserial.write(c);
}

/**
 * write
 */
int32_t UARTSerial::write(const uint8_t *c_str)
{
#if ARDUINO_USB_CDC_ON_BOOT
  if (m_cdcserial) return m_cdcserial->write((const char*)c_str);
#endif
  return m_hwserial.write((const char*)c_str);
}

/**
 * write
 */
int32_t UARTSerial::write(const uint8_t *c_str, uint32_t size)
{
#if ARDUINO_USB_CDC_ON_BOOT
  if (m_cdcserial) return m_cdcserial->write(c_str, size);
#endif
  return m_hwserial.write(c_str, size);
}

/**
 * write
 */
int32_t UARTSerial::write_ro(const char *c_str)
{
#if ARDUINO_USB_CDC_ON_BOOT
  if (m_cdcserial) return m_cdcserial->print(c_str);
#endif
  return m_hwserial.print(c_str);
}

/**
 * read
 */
uint8_t UARTSerial::read()
{
#if ARDUINO_USB_CDC_ON_BOOT
  if (m_cdcserial) return m_cdcserial->read();
#endif
  return m_hwserial.read();
}

/**
 * read
 */
int32_t UARTSerial::read(uint8_t *buf, uint32_t size)
{
  int32_t count = 0;
#if ARDUINO_USB_CDC_ON_BOOT
  if (m_cdcserial) {
    for (; count < size && m_cdcserial->available(); ++count)
    {
      buf[count] = m_cdcserial->read();
    }
    return count;
  }
#endif
  for (; count < size && m_hwserial.available(); ++count)
  {
    buf[count] = m_hwserial.read();
  }
  return count;
}

/**
 * available
 */
int32_t UARTSerial::available()
{
#if ARDUINO_USB_CDC_ON_BOOT
  if (m_cdcserial) return m_cdcserial->available();
#endif
  return m_hwserial.available();
}

/**
 * connected
 */
int8_t UARTSerial::connected()
{
  return m_connected;
}

/**
 * setTimeout
 */
void UARTSerial::setTimeout(uint32_t timeout)
{
}

/**
 * flush
 */
void UARTSerial::flush(int16_t flushtype)
{
#if ARDUINO_USB_CDC_ON_BOOT
  if (m_cdcserial) {
    if (IsFlushTx(flushtype)) m_cdcserial->flush();
    if (IsFlushRx(flushtype)) while (m_cdcserial->available() > 0) m_cdcserial->read();
    return;
  }
#endif
  if (IsFlushTx(flushtype)) m_hwserial.flush();
  if (IsFlushRx(flushtype)) while (m_hwserial.available() > 0) m_hwserial.read();
}

/**
 * @brief With timestamp will print timestamp first
 * derived class should implement this function to print timestamp
 * @param None
 * @return this
 */
iTerminalInterface* UARTSerial::with_timestamp()
{
  char tembuff[21];
  MicrosToTimeString((uint64_t)esp_timer_get_time(), tembuff, sizeof(tembuff));

  write('[');
  write_pad(tembuff, 15, true);
  write(']');

  return this;
}

UARTSerial __serial_uart(Serial);
UARTSerial __serial_uart1(Serial1);

#ifdef ENABLE_I2S_BUS

#include <ESP_I2S.h>
#include <esp32-hal-periman.h>
#include "DeviceControlInterface.h"

namespace {

const uint16_t s_i2s_raw_samples = 128;

/**
 * The framework code nearest to what the I2S driver reported.
 */
pdi_err_t fromEspError(esp_err_t err)
{
  switch( err ){
    case ESP_ERR_NO_MEM:        return PDI_ERR_NO_MEM;
    case ESP_ERR_INVALID_ARG:   return PDI_ERR_INVALID_ARG;
    case ESP_ERR_INVALID_STATE: return PDI_ERR_STATE;
    case ESP_ERR_NOT_FOUND:     return PDI_ERR_BUSY;
    case ESP_ERR_TIMEOUT:       return PDI_ERR_TIMEOUT;
    default:                    return PDI_ERR_IO;
  }
}

}

/**
 * I2sSerial constructor.
 */
I2sSerial::I2sSerial() : m_i2s(nullptr), m_raw(nullptr), m_bclk(-1), m_ws(-1), m_din(-1)
{
}

/**
 * I2sSerial destructor.
 */
I2sSerial::~I2sSerial()
{
  release();
}

/**
 * Takes the peripheral and captures 32 bit left channel slots, the frame an I2S
 * microphone sends; playback is not supported.
 */
pdi_err_t I2sSerial::configure(const serial_i2s_config_t &config)
{
  release();

  if( !config.m_capture ){
    return PDI_ERR_NOT_SUPPORTED;
  }

  if( 0 > config.m_din || 0 > config.m_bclk || 0 > config.m_ws ){
    return PDI_ERR_INVALID_ARG;
  }

  m_i2s = pdiutil::safe_new<I2SClass>();
  m_raw = pdiutil::safe_new_array<int32_t>(s_i2s_raw_samples);
  if( nullptr == m_i2s || nullptr == m_raw ){
    release();
    return PDI_ERR_NO_MEM;
  }

  m_i2s->setPins(config.m_bclk, config.m_ws, -1, config.m_din);
  if( !m_i2s->begin(I2S_MODE_STD, config.m_rate, I2S_DATA_BIT_WIDTH_32BIT, I2S_SLOT_MODE_MONO, I2S_STD_SLOT_LEFT) ){
    esp_err_t reason = (esp_err_t)m_i2s->lastError();
    release();
    return (ESP_OK == reason) ? PDI_ERR_BUSY : fromEspError(reason);
  }

  m_bclk = config.m_bclk;
  m_ws = config.m_ws;
  m_din = config.m_din;
  return PDI_OK;
}

/**
 * Stops the bus and hands the peripheral and its pins back.
 */
void I2sSerial::release()
{
  if( nullptr != m_i2s ){
    m_i2s->end();
  }
  pdiutil::safe_delete(m_i2s);
  pdiutil::safe_delete_array(m_raw);
  m_bclk = -1;
  m_ws = -1;
  m_din = -1;
}

/**
 * Playback is not supported.
 */
int32_t I2sSerial::write(const uint8_t *c_str, uint32_t size)
{
  return 0;
}

/**
 * 16 bit samples, least significant byte first, from what the bus gathered, waiting
 * no longer than the timeout for more.
 */
int32_t I2sSerial::read(uint8_t *buf, uint32_t size)
{
  if( nullptr == buf ){
    return PDI_ERR_NULL_PTR;
  }

  if( nullptr == m_i2s || nullptr == m_raw || nullptr == m_i2s->rxChan() ){
    return PDI_ERR_NOT_INITIALIZED;
  }

  if( ESP32_BUS_TYPE_I2S_STD_BCLK != perimanGetPinBusType(m_bclk) ||
      ESP32_BUS_TYPE_I2S_STD_WS != perimanGetPinBusType(m_ws) ||
      ESP32_BUS_TYPE_I2S_STD_DIN != perimanGetPinBusType(m_din) ){
    return PDI_ERR_BUSY;
  }

  uint32_t wanted = size / 2;
  uint32_t done = 0;

  while( done < wanted ){

    uint32_t ask = wanted - done;
    if( ask > s_i2s_raw_samples ){
      ask = s_i2s_raw_samples;
    }
    size_t got = 0;
    esp_err_t err = i2s_channel_read(m_i2s->rxChan(), m_raw, ask * sizeof(int32_t), &got, m_timeout);
    if( ESP_OK != err && ESP_ERR_TIMEOUT != err && 0 == got ){
      return (0 < done) ? (int32_t)(done * 2) : fromEspError(err);
    }

    uint32_t samples = got / sizeof(int32_t);
    for( uint32_t i = 0; i < samples; i++ ){
      int16_t sample = (int16_t)(m_raw[i] >> 16);
      buf[(done + i) * 2] = (uint8_t)(sample & 0xFF);
      buf[(done + i) * 2 + 1] = (uint8_t)((sample >> 8) & 0xFF);
    }
    done += samples;

    if( samples < ask ){
      break;
    }
  }

  return (int32_t)(done * 2);
}

/**
 * The pins the bus runs on when its line names none: SCK, WS, SD.
 */
uint8_t I2sSerial::defaultPins(uint8_t *out, uint8_t max) const
{
  uint8_t n = 0;
  for( ; n < sizeof(I2S_GPIO_PINS) && n < max && nullptr != out; n++ ){
    out[n] = I2S_GPIO_PINS[n];
  }
  return n;
}

I2sSerial __serial_i2s;

#endif

#ifdef ENABLE_I2C_BUS

#include <esp32-hal-i2c.h>
#include "DeviceControlInterface.h"

namespace {

const uint8_t s_i2c_port = 0;

}

/**
 * I2cSerial constructor.
 */
I2cSerial::I2cSerial()
{
}

/**
 * The pins the bus runs on when no bus line names its own: SDA, SCL.
 */
uint8_t I2cSerial::defaultPins(uint8_t *out, uint8_t max) const
{
  uint8_t n = 0;
  for( ; n < sizeof(I2C_GPIO_PINS) && n < max && nullptr != out; n++ ){
    out[n] = I2C_GPIO_PINS[n];
  }
  return n;
}

/**
 * Puts the bus on the pins and clock given, -1 and 0 for its own; a running bus
 * restarts on the new pins when next used.
 */
pdi_err_t I2cSerial::configure(int16_t sda, int16_t scl, uint32_t clock)
{
  int16_t pins[2] = {sda, scl};
  for( uint8_t p = 0; p < 2; p++ ){
    if( pins[p] < 0 ){
      continue;
    }
    if( !GPIO_IS_VALID_OUTPUT_GPIO(pins[p]) ){
      return PDI_ERR_INVALID_ARG;
    }
    for( uint8_t j = 0; j < sizeof(EXCEPTIONAL_GPIO_PINS); j++ ){
      if( EXCEPTIONAL_GPIO_PINS[j] == pins[p] ){
        return PDI_ERR_INVALID_ARG;
      }
    }
  }

  if( sda == m_sda && scl == m_scl && clock == m_clock ){
    return PDI_OK;
  }

  if( i2cIsInit(s_i2c_port) ){
    i2cDeinit(s_i2c_port);
  }
  return iI2cSerialInterface::configure(sda, scl, clock);
}

/**
 * Starts the bus on its pins and clock, unless it already runs.
 */
pdi_err_t I2cSerial::start()
{
  if( i2cIsInit(s_i2c_port) ){
    return PDI_OK;
  }
  if( sda() < 0 || scl() < 0 ){
    return PDI_ERR_NOT_SUPPORTED;
  }
  esp_err_t err = i2cInit(s_i2c_port, (int8_t)sda(), (int8_t)scl(), clock());
  if( ESP_OK == err ){
    return PDI_OK;
  }
  return (ESP_ERR_NO_MEM == err) ? PDI_ERR_NO_MEM : PDI_ERR_NOT_INITIALIZED;
}

/**
 * The framework code for a failed transaction, asking the address again to tell a
 * refused address from refused data.
 */
pdi_err_t I2cSerial::failure(int32_t err, uint32_t target)
{
  switch( err ){
    case ESP_ERR_TIMEOUT:     return PDI_ERR_TIMEOUT;
    case ESP_ERR_INVALID_ARG: return PDI_ERR_INVALID_ARG;
    case ESP_ERR_NO_MEM:      return PDI_ERR_NO_MEM;
    default:                  break;
  }
  return (ESP_OK == i2cWrite(s_i2c_port, (uint16_t)target, nullptr, 0, I2C_BUS_TIMEOUT_MS)) ? PDI_ERR_IO
                                                                                          : PDI_ERR_NOT_FOUND;
}

/**
 * Whether a chip answers at the address.
 */
bool I2cSerial::probe(uint32_t target)
{
  if( target > 0x7F || PDI_OK != start() ){
    return false;
  }
  return ESP_OK == i2cWrite(s_i2c_port, (uint16_t)target, nullptr, 0, I2C_BUS_TIMEOUT_MS);
}

/**
 * Writes tx to the selected address, then reads rx after a repeated start; how many
 * bytes came back, or were sent.
 */
int32_t I2cSerial::transfer(const uint8_t *tx, uint32_t txlen, uint8_t *rx, uint32_t rxlen)
{
  return transferTo(m_selected, tx, txlen, rx, rxlen);
}

/**
 * Writes tx to the address, then reads rx after a repeated start, under the
 * peripheral's own lock for the whole exchange.
 */
int32_t I2cSerial::transferTo(uint32_t target, const uint8_t *tx, uint32_t txlen, uint8_t *rx, uint32_t rxlen)
{
  if( (txlen > 0 && nullptr == tx) || (rxlen > 0 && nullptr == rx) ){
    return PDI_ERR_NULL_PTR;
  }
  if( target > 0x7F ){
    return PDI_ERR_INVALID_ARG;
  }
  pdi_err_t started = start();
  if( PDI_OK != started ){
    return started;
  }

  esp_err_t err = ESP_OK;
  size_t got = 0;
  uint16_t address = (uint16_t)target;

  if( 0 == rxlen ){
    err = i2cWrite(s_i2c_port, address, tx, txlen, I2C_BUS_TIMEOUT_MS);
  }else if( 0 == txlen ){
    err = i2cRead(s_i2c_port, address, rx, rxlen, I2C_BUS_TIMEOUT_MS, &got);
  }else{
    err = i2cWriteReadNonStop(s_i2c_port, address, tx, txlen, rx, rxlen, I2C_BUS_TIMEOUT_MS, &got);
  }

  if( ESP_OK != err ){
    return failure(err, target);
  }
  return (0 == rxlen) ? (int32_t)txlen : (int32_t)got;
}

I2cSerial __serial_i2c;

#endif

#ifdef ENABLE_SPI_BUS

#include <SPI.h>
#include "DeviceControlInterface.h"

namespace {

#if CONFIG_IDF_TARGET_ESP32
const uint8_t s_spi_port = VSPI;
#else
const uint8_t s_spi_port = FSPI;
#endif

const uint8_t s_spi_chunk = 64;

/**
 * Whether the pin exists and is not kept for the flash or the console, so the
 * bus may read a line on it.
 */
bool spiReadable(int16_t pin)
{
  if( pin < 0 || !GPIO_IS_VALID_GPIO(pin) ){
    return false;
  }
  for( uint8_t j = 0; j < sizeof(EXCEPTIONAL_GPIO_PINS); j++ ){
    if( EXCEPTIONAL_GPIO_PINS[j] == pin ){
      return false;
    }
  }
  return true;
}

/**
 * Whether the bus may also drive a line on the pin.
 */
bool spiDrivable(int16_t pin)
{
  return spiReadable(pin) && GPIO_IS_VALID_OUTPUT_GPIO(pin);
}

}

/**
 * SpiSerial constructor.
 */
SpiSerial::SpiSerial() : m_spi(nullptr), m_started(false), m_open(false), m_cs(-1)
{
}

/**
 * SpiSerial destructor.
 */
SpiSerial::~SpiSerial()
{
  if( m_started && nullptr != m_spi ){
    m_spi->end();
  }
  pdiutil::safe_delete(m_spi);
}

/**
 * The pins the bus runs on when no bus line names its own: SCK, MISO, MOSI.
 */
uint8_t SpiSerial::defaultPins(uint8_t *out, uint8_t max) const
{
  uint8_t n = 0;
  for( ; n < sizeof(SPI_GPIO_PINS) && n < max && nullptr != out; n++ ){
    out[n] = SPI_GPIO_PINS[n];
  }
  return n;
}

/**
 * Puts the bus on the pins and top clock given, -1 and 0 for its own; a running bus
 * restarts on the new pins when next used.
 */
pdi_err_t SpiSerial::configure(int16_t sck, int16_t miso, int16_t mosi, uint32_t clock)
{
  if( (sck >= 0 && !spiDrivable(sck)) || (mosi >= 0 && !spiDrivable(mosi)) || (miso >= 0 && !spiReadable(miso)) ){
    return PDI_ERR_INVALID_ARG;
  }

  if( sck == m_sck && miso == m_miso && mosi == m_mosi && clock == m_clock ){
    return PDI_OK;
  }

  if( m_open ){
    return PDI_ERR_BUSY;
  }

  if( m_started && nullptr != m_spi ){
    m_spi->end();
    m_started = false;
  }
  return iSpiSerialInterface::configure(sck, miso, mosi, clock);
}

/**
 * Starts the bus on its pins, unless it already runs.
 */
pdi_err_t SpiSerial::start()
{
  if( m_started ){
    return PDI_OK;
  }
  if( sck() < 0 || miso() < 0 || mosi() < 0 ){
    return PDI_ERR_NOT_SUPPORTED;
  }
  if( nullptr == m_spi ){
    m_spi = pdiutil::safe_new<SPIClass>(s_spi_port);
    if( nullptr == m_spi ){
      return PDI_ERR_NO_MEM;
    }
  }
  if( !m_spi->begin((int8_t)sck(), (int8_t)miso(), (int8_t)mosi(), -1) ){
    return PDI_ERR_BUSY;
  }
  m_started = true;
  return PDI_OK;
}

/**
 * Takes the bus at the clock and mode given and pulls the chip select low, holding
 * the peripheral's lock until end().
 */
pdi_err_t SpiSerial::begin(int16_t cs, uint32_t clock, uint8_t mode)
{
  if( cs >= 0 && !spiDrivable(cs) ){
    return PDI_ERR_INVALID_ARG;
  }
  pdi_err_t started = start();
  if( PDI_OK != started ){
    return started;
  }

  m_spi->beginTransaction(SPISettings(clockFor(clock), MSBFIRST, mode & 0x03));
  if( cs >= 0 ){
    pinMode(cs, OUTPUT);
    digitalWrite(cs, LOW);
  }
  m_cs = cs;
  m_open = true;
  return PDI_OK;
}

/**
 * Lets the chip select go high and hands the bus back.
 */
void SpiSerial::end()
{
  if( !m_open ){
    return;
  }
  if( m_cs >= 0 ){
    digitalWrite(m_cs, HIGH);
  }
  m_spi->endTransaction();
  m_cs = -1;
  m_open = false;
}

/**
 * Clocks the bytes out and in at once, through word aligned buffers.
 */
int32_t SpiSerial::exchange(const uint8_t *tx, uint8_t *rx, uint32_t len)
{
  if( !m_open ){
    return PDI_ERR_NOT_INITIALIZED;
  }

  uint32_t out[s_spi_chunk / sizeof(uint32_t)];
  uint32_t in[s_spi_chunk / sizeof(uint32_t)];
  uint32_t done = 0;

  while( done < len ){
    uint32_t n = (len - done < s_spi_chunk) ? len - done : s_spi_chunk;
    if( nullptr != tx ){
      memcpy(out, tx + done, n);
    }else{
      memset(out, 0xFF, n);
    }
    m_spi->transferBytes((const uint8_t *)out, (nullptr != rx) ? (uint8_t *)in : nullptr, n);
    if( nullptr != rx ){
      memcpy(rx + done, in, n);
    }
    done += n;
  }
  return (int32_t)done;
}

SpiSerial __serial_spi;

#endif

/*
  SerialEvent occurs whenever a new data comes in the hardware serial RX. This
  routine is run between each time loop() runs, so using delay inside loop can
  delay response. Multiple bytes of data may be available.
  Now polled via DeviceControlInterface::handleEvents() to also cover HWCDC
  (USB CDC on boot) where Arduino core does not auto-invoke serialEvent.
*/
// void serialEvent() {
//
//   // check for uart serial data available if any
//   if (__serial_uart.available()) {
//     serial_event_t e(SERIAL_IFACE_UART, SERIAL_IFACE_CMD, &__serial_uart);
//     __utl_event.execute_event(EVENT_SERIAL_AVAILABLE, &e);
//   }
// }