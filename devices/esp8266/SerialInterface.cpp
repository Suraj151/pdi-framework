/****************************** Serial Interface ******************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st June 2019
******************************************************************************/

#include "SerialInterface.h"
#include <utility/DataTypeConversions.h>


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
{
}

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

  m_hwserial.begin(speed);

  uint32_t last_seen = millis();
  while ((millis() - last_seen) < SERIAL_BOOT_RX_QUIET_MS) {
    if (m_hwserial.available() > 0) {
      m_hwserial.read();
      last_seen = millis();
    }
    yield();
  }

  m_connected = true;
  return 1;
}

/**
 * disconnect
 */
int16_t UARTSerial::disconnect()
{
  m_hwserial.end();
  m_connected = false;
  return 0;
}

/**
 * write
 */
int32_t UARTSerial::write(uint8_t c)
{
  #ifdef ENABLE_CONTEXTUAL_EXECUTION
  m_mutex.critical_lock();
  #endif

  int32_t ret = m_hwserial.write(c);

  #ifdef ENABLE_CONTEXTUAL_EXECUTION
  m_mutex.critical_unlock();
  #endif

  return ret;
}

/**
 * write
 */
int32_t UARTSerial::write(const uint8_t *c_str)
{
  #ifdef ENABLE_CONTEXTUAL_EXECUTION
  m_mutex.critical_lock();
  #endif

  int32_t ret = m_hwserial.write((const char*)c_str);

  #ifdef ENABLE_CONTEXTUAL_EXECUTION
  m_mutex.critical_unlock();
  #endif

  return ret;
}

/**
 * write
 */
int32_t UARTSerial::write(const uint8_t *c_str, uint32_t size)
{
  #ifdef ENABLE_CONTEXTUAL_EXECUTION
  m_mutex.critical_lock();
  #endif

  int32_t ret = m_hwserial.write(c_str, size);

  #ifdef ENABLE_CONTEXTUAL_EXECUTION
  m_mutex.critical_unlock();
  #endif

  return ret;
}

/**
 * write
 */
int32_t UARTSerial::write_ro(const char *c_str)
{
  #ifdef ENABLE_CONTEXTUAL_EXECUTION
  m_mutex.critical_lock();
  #endif

  int32_t ret = m_hwserial.print(c_str);
  
  #ifdef ENABLE_CONTEXTUAL_EXECUTION
  m_mutex.critical_unlock();
  #endif

  return ret;
}

/**
 * read
 */
uint8_t UARTSerial::read()
{
  #ifdef ENABLE_CONTEXTUAL_EXECUTION
  m_mutex.critical_lock();
  #endif

  uint8_t ret = m_hwserial.read();

  #ifdef ENABLE_CONTEXTUAL_EXECUTION
  m_mutex.critical_unlock();
  #endif
  
  return ret;
}

/**
 * read
 */
int32_t UARTSerial::read(uint8_t *buf, uint32_t size)
{
  uint32_t count = 0;

  #ifdef ENABLE_CONTEXTUAL_EXECUTION
  m_mutex.critical_lock();
  #endif

  for (; count < size && m_hwserial.available(); ++count)
  {
    buf[count] = m_hwserial.read();
  }

  #ifdef ENABLE_CONTEXTUAL_EXECUTION
  m_mutex.critical_unlock();
  #endif

  return count;
}

/**
 * available
 */
int32_t UARTSerial::available()
{
  #ifdef ENABLE_CONTEXTUAL_EXECUTION
  m_mutex.critical_lock();
  #endif

  int32_t ret = m_hwserial.available();

  #ifdef ENABLE_CONTEXTUAL_EXECUTION
  m_mutex.critical_unlock();
  #endif

  return ret;
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
  #ifdef ENABLE_CONTEXTUAL_EXECUTION
  m_mutex.critical_lock();
  #endif

  if (IsFlushTx(flushtype)) m_hwserial.flush();
  if (IsFlushRx(flushtype)) while (m_hwserial.available() > 0) m_hwserial.read();

  #ifdef ENABLE_CONTEXTUAL_EXECUTION
  m_mutex.critical_unlock();
  #endif  
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
  MicrosToTimeString(micros64(), tembuff, sizeof(tembuff));

  write('[');
  write_pad(tembuff, 15, true);
  write(']');

  return this;
}

UARTSerial __serial_uart(Serial);
UARTSerial __serial_uart1(Serial1);

#ifdef ENABLE_I2S_BUS

#include <core_esp8266_i2s.h>
#include "DeviceControlInterface.h"

/**
 * I2sSerial constructor.
 */
I2sSerial::I2sSerial() : m_running(false)
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
 * Captures 16 bit slots of 32 clocks a frame on the fixed I2S input pins; other
 * pins and playback are refused.
 */
pdi_err_t I2sSerial::configure(const serial_i2s_config_t &config)
{
  release();

  if( !config.m_capture ){
    return PDI_ERR_NOT_SUPPORTED;
  }

  if( I2S_GPIO_PINS[0] != config.m_bclk || I2S_GPIO_PINS[1] != config.m_ws || I2S_GPIO_PINS[2] != config.m_din ){
    return PDI_ERR_INVALID_ARG;
  }

  if( !i2s_rxtx_begin(true, false) ){
    return PDI_ERR_NO_MEM;
  }
  i2s_set_rate(config.m_rate);

  m_running = true;
  return PDI_OK;
}

/**
 * Stops the bus and frees its buffers.
 */
void I2sSerial::release()
{
  if( m_running ){
    i2s_end();
    m_running = false;
  }
}

/**
 * Playback is not supported.
 */
int32_t I2sSerial::write(const uint8_t *c_str, uint32_t size)
{
  return 0;
}

/**
 * 16 bit samples, least significant byte first, from the upper half of each word,
 * waiting no longer than the timeout for more.
 */
int32_t I2sSerial::read(uint8_t *buf, uint32_t size)
{
  if( nullptr == buf ){
    return PDI_ERR_NULL_PTR;
  }

  if( !m_running ){
    return PDI_ERR_NOT_INITIALIZED;
  }

  uint32_t wanted = size / 2;
  uint32_t done = 0;
  uint32_t began = millis();

  while( done < wanted ){

    int16_t left = 0;
    int16_t right = 0;
    if( !i2s_read_sample(&left, &right, false) ){
      if( (millis() - began) < m_timeout ){
        yield();
        continue;
      }
      break;
    }

    buf[done * 2] = (uint8_t)(right & 0xFF);
    buf[done * 2 + 1] = (uint8_t)((right >> 8) & 0xFF);
    done++;
  }

  return (int32_t)(done * 2);
}

/**
 * The pins the hardware runs the bus on: SCK, WS, SD.
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

#include <twi.h>
#include "DeviceControlInterface.h"

namespace {

const uint8_t s_twi_ok = 0;
const uint8_t s_twi_address_refused = 2;
const uint8_t s_twi_data_refused = 3;

/**
 * The framework code for what the bus reported.
 */
pdi_err_t fromTwiStatus(uint8_t status)
{
  switch( status ){
    case s_twi_address_refused: return PDI_ERR_NOT_FOUND;
    case s_twi_data_refused:    return PDI_ERR_IO;
    default:                    return PDI_ERR_BUSY;
  }
}

}

/**
 * I2cSerial constructor.
 */
I2cSerial::I2cSerial() : m_started(false)
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
 * Puts the software driven bus on the pins and clock given, -1 and 0 for its own;
 * it restarts on the new pins when next used.
 */
pdi_err_t I2cSerial::configure(int16_t sda, int16_t scl, uint32_t clock)
{
  int16_t pins[2] = {sda, scl};
  for( uint8_t p = 0; p < 2; p++ ){
    if( pins[p] < 0 ){
      continue;
    }
    if( pins[p] >= 16 ){
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

  m_started = false;
  return iI2cSerialInterface::configure(sda, scl, clock);
}

/**
 * Starts the bus on its pins and clock, unless it already runs.
 */
void I2cSerial::start()
{
  if( m_started || sda() < 0 || scl() < 0 ){
    return;
  }
  twi_init((unsigned char)sda(), (unsigned char)scl());
  twi_setClock(clock());
  m_started = true;
}

/**
 * Whether a chip answers at the address; a bus whose lines do not both rest high is
 * not clocked at all.
 */
bool I2cSerial::probe(uint32_t target)
{
  if( target > 0x7F ){
    return false;
  }
  #ifdef ENABLE_CONTEXTUAL_EXECUTION
  m_mutex.lock();
  #endif
  start();
  bool idle = m_started && HIGH == digitalRead(sda()) && HIGH == digitalRead(scl());
  bool answered = idle && s_twi_ok == twi_writeTo((unsigned char)target, nullptr, 0, true);
  #ifdef ENABLE_CONTEXTUAL_EXECUTION
  m_mutex.unlock();
  #endif
  return answered;
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
 * Writes tx to the address, then reads rx after a repeated start, holding the bus
 * for the whole exchange.
 */
int32_t I2cSerial::transferTo(uint32_t target, const uint8_t *tx, uint32_t txlen, uint8_t *rx, uint32_t rxlen)
{
  #ifdef ENABLE_CONTEXTUAL_EXECUTION
  m_mutex.lock();
  #endif
  int32_t result = exchange(target, tx, txlen, rx, rxlen);
  #ifdef ENABLE_CONTEXTUAL_EXECUTION
  m_mutex.unlock();
  #endif
  return result;
}

/**
 * One exchange with the address, with the bus already held: how many bytes
 * came back, or were sent when none were asked for.
 */
int32_t I2cSerial::exchange(uint32_t target, const uint8_t *tx, uint32_t txlen, uint8_t *rx, uint32_t rxlen)
{
  if( (txlen > 0 && nullptr == tx) || (rxlen > 0 && nullptr == rx) ){
    return PDI_ERR_NULL_PTR;
  }
  if( target > 0x7F ){
    return PDI_ERR_INVALID_ARG;
  }
  start();
  if( !m_started ){
    return PDI_ERR_NOT_INITIALIZED;
  }

  unsigned char address = (unsigned char)target;

  if( txlen > 0 ){
    uint8_t status = twi_writeTo(address, (unsigned char *)tx, txlen, (0 == rxlen));
    if( s_twi_ok != status ){
      return fromTwiStatus(status);
    }
  }

  if( 0 == rxlen ){
    return (int32_t)txlen;
  }

  uint8_t status = twi_readFrom(address, rx, rxlen, true);
  if( s_twi_ok != status ){
    return fromTwiStatus(status);
  }
  return (int32_t)rxlen;
}

I2cSerial __serial_i2c;

#endif

#ifdef ENABLE_SPI_BUS

#include <SPI.h>
#include "DeviceControlInterface.h"

namespace {

const uint8_t s_spi_modes[4] = {SPI_MODE0, SPI_MODE1, SPI_MODE2, SPI_MODE3};

}

/**
 * SpiSerial constructor.
 */
SpiSerial::SpiSerial() : m_started(false), m_open(false), m_cs(-1)
{
}

/**
 * The pins the hardware runs the bus on: SCK, MISO, MOSI.
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
 * Takes the top clock given. The hardware fixes the pins, so naming any but
 * its own is refused.
 */
pdi_err_t SpiSerial::configure(int16_t sck, int16_t miso, int16_t mosi, uint32_t clock)
{
  int16_t pins[3] = {sck, miso, mosi};
  for( uint8_t p = 0; p < 3; p++ ){
    if( pins[p] >= 0 && pins[p] != SPI_GPIO_PINS[p] ){
      return PDI_ERR_NOT_SUPPORTED;
    }
  }
  return iSpiSerialInterface::configure(-1, -1, -1, clock);
}

/**
 * Takes the bus at the clock and mode given and pulls the chip select low, holding
 * the bus until end().
 */
pdi_err_t SpiSerial::begin(int16_t cs, uint32_t clock, uint8_t mode)
{
  if( cs > 16 ){
    return PDI_ERR_INVALID_ARG;
  }
  for( uint8_t j = 0; cs >= 0 && j < sizeof(EXCEPTIONAL_GPIO_PINS); j++ ){
    if( EXCEPTIONAL_GPIO_PINS[j] == cs ){
      return PDI_ERR_INVALID_ARG;
    }
  }

  #ifdef ENABLE_CONTEXTUAL_EXECUTION
  m_mutex.lock();
  #endif

  if( !m_started ){
    SPI.begin();
    m_started = true;
  }

  SPI.beginTransaction(SPISettings(clockFor(clock), MSBFIRST, s_spi_modes[mode & 0x03]));
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
  SPI.endTransaction();
  m_cs = -1;
  m_open = false;

  #ifdef ENABLE_CONTEXTUAL_EXECUTION
  m_mutex.unlock();
  #endif
}

/**
 * Clocks the bytes out and in at once; 0xFF goes out for a missing buffer out, and
 * a missing buffer in drops what arrives.
 */
int32_t SpiSerial::exchange(const uint8_t *tx, uint8_t *rx, uint32_t len)
{
  if( !m_open ){
    return PDI_ERR_NOT_INITIALIZED;
  }
  SPI.transferBytes(tx, rx, len);
  return (int32_t)len;
}

SpiSerial __serial_spi;

#endif

/*
  SerialEvent occurs whenever a new data comes in the hardware serial RX. This
  routine is run between each time loop() runs, so using delay inside loop can
  delay response. Multiple bytes of data may be available.
  Now polled via DeviceControlInterface::handleEvents() because Arduino esp8266
  core does not reliably invoke serialEvent across all core versions.
*/
// void serialEvent() {
//
//   // check for uart serial data available if any
//   if (__serial_uart.available()) {
//     serial_event_t e(SERIAL_IFACE_UART, SERIAL_IFACE_CMD, &__serial_uart);
//     __utl_event.execute_event(EVENT_SERIAL_AVAILABLE, &e);
//   }
// }