/******************************* INMP441 Driver *******************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 19th Oct 2026
******************************************************************************/

#include "Inmp441Driver.h"

#ifdef ENABLE_I2S_BUS

#include <service_provider/transport/DeviceManager.h>
#include <interface/pdi.h>

namespace {

const uint16_t s_probe_block_bytes = 128;

/**
 * Makes a driver for a device that turned up.
 */
iDriverInterface *createInmp441()
{
  return pdiutil::safe_new<Inmp441Driver>();
}

/**
 * Whether a block of 16 bit samples holds more than one value.
 */
bool samplesChange(const uint8_t *buf, uint32_t size)
{
  if( size < 4 ){
    return false;
  }
  int16_t first = (int16_t)(buf[0] | (buf[1] << 8));
  for( uint32_t i = 2; i + 1 < size; i += 2 ){
    if( (int16_t)(buf[i] | (buf[i + 1] << 8)) != first ){
      return true;
    }
  }
  return false;
}

/**
 * A pin the declaration gave, false when it gave none or one no board has.
 */
bool declaredPin(const driver_device_t &dev, const char *key, int8_t &pin)
{
  uint32_t value = 0;
  if( !DeviceManager::param(dev, key, value) || value > 127 ){
    return false;
  }
  pin = (int8_t)value;
  return true;
}

}

/**
 * Inmp441Driver constructor.
 */
Inmp441Driver::Inmp441Driver() : m_bus(nullptr), m_rate(SOUND_DEFAULT_RATE)
{
}

/**
 * Frees the bus when the driver goes.
 */
Inmp441Driver::~Inmp441Driver()
{
  if( nullptr != m_bus ){
    m_bus->release();
    m_bus = nullptr;
  }
}

/**
 * Adds the driver to the ones the serial service binds with.
 */
void Inmp441Driver::registerDriver()
{
  DeviceManager::registerDriver(driver_prop_t(RODT_ATTR(DRIVER_NAME_INMP441), SERIAL_IFACE_I2S,
                                              nullptr, 0, nullptr, createInmp441));
}

/**
 * Wires the bus to the declared pins and confirms the mic by its samples.
 */
pdi_err_t Inmp441Driver::probe(driver_device_t &dev, iSerialInterface *bus)
{
  if( 0 != dev.m_target || nullptr == bus || SERIAL_IFACE_I2S != serialFamily(bus->type()) ){
    return PDI_ERR_NOT_SUPPORTED;
  }

  serial_i2s_config_t config;
  pdiutil::string sck = CHARPTR_WRAP(DRIVER_PARAM_SCK), ws = CHARPTR_WRAP(DRIVER_PARAM_WS), sd = CHARPTR_WRAP(DRIVER_PARAM_SD);
  uint32_t unused = 0;
  bool named = DeviceManager::param(dev, sck.c_str(), unused) || DeviceManager::param(dev, ws.c_str(), unused) ||
               DeviceManager::param(dev, sd.c_str(), unused);

  if( named ){
    if( !declaredPin(dev, sck.c_str(), config.m_bclk) || !declaredPin(dev, ws.c_str(), config.m_ws) ||
        !declaredPin(dev, sd.c_str(), config.m_din) ){
      return PDI_ERR_INVALID_ARG;
    }
  }else{
    uint8_t pins[3] = {0, 0, 0};
    if( 3 != bus->defaultPins(pins, 3) ){
      return PDI_ERR_INVALID_ARG;
    }
    config.m_bclk = (int8_t)pins[0];
    config.m_ws = (int8_t)pins[1];
    config.m_din = (int8_t)pins[2];
  }

  uint32_t rate = SOUND_DEFAULT_RATE;
  pdiutil::string ratekey = CHARPTR_WRAP(DRIVER_PARAM_RATE);
  DeviceManager::param(dev, ratekey.c_str(), rate);
  config.m_rate = rate;
  config.m_bits = 16;
  config.m_capture = true;

  iI2sSerialInterface *i2s = static_cast<iI2sSerialInterface *>(bus);
  pdi_err_t configured = i2s->configure(config);
  if( PDI_OK != configured ){
    return configured;
  }

  uint8_t block[s_probe_block_bytes];
  bool alive = false;
  int32_t got = 0;
  uint32_t began = __i_dvc_ctrl.millis_now();
  i2s->setTimeout(SOUND_PROBE_WAIT_MS);
  while( !alive && got >= 0 && (__i_dvc_ctrl.millis_now() - began) < SOUND_PROBE_WAIT_MS ){
    got = i2s->read(block, sizeof(block));
    alive = (got > 0) && samplesChange(block, (uint32_t)got);
    __i_dvc_ctrl.yield();
  }
  i2s->setTimeout(0);

  if( !alive ){
    i2s->release();
    return (got < 0) ? (pdi_err_t)got : PDI_ERR_NOT_FOUND;
  }

  m_bus = i2s;
  m_rate = rate;
  return PDI_OK;
}

/**
 * Stops the bus and frees it.
 */
void Inmp441Driver::remove(driver_device_t &dev)
{
  if( nullptr != m_bus ){
    m_bus->release();
    m_bus = nullptr;
  }
}

/**
 * The capture node under /dev.
 */
bool Inmp441Driver::nodeName(pdiutil::string &out) const
{
  out = CHARPTR_WRAP(DEVICE_NODE_PCM_CAPTURE);
  return true;
}

/**
 * PCM bytes captured from the mic.
 */
int32_t Inmp441Driver::readNode(uint8_t *buf, uint32_t size)
{
  if( nullptr == m_bus ){
    return PDI_ERR_NOT_INITIALIZED;
  }
  return m_bus->read(buf, size);
}

/**
 * Samples per second, as declared or the default.
 */
uint32_t Inmp441Driver::rate() const
{
  return m_rate;
}

#endif
