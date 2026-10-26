/********************************* AT24 Driver *********************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 20th Oct 2026
******************************************************************************/

#include "At24Driver.h"

#ifdef ENABLE_I2C_BUS

#include <service_provider/transport/DeviceManager.h>
#include <interface/pdi.h>

namespace {

/**
 * Makes a driver for a device that was declared.
 */
iDriverInterface *createAt24()
{
  return pdiutil::safe_new<At24Driver>();
}

}

/**
 * At24Driver constructor.
 */
At24Driver::At24Driver() : m_bus(nullptr), m_target(0), m_size(0), m_page(0), m_busy(false)
{
}

/**
 * Adds the driver to the ones the serial service binds with.
 */
void At24Driver::registerDriver()
{
  DeviceManager::registerDriver(driver_prop_t(RODT_ATTR(DRIVER_NAME_AT24), SERIAL_IFACE_I2C,
                                              nullptr, 0, nullptr, createAt24));
}

/**
 * Takes the declared size and page and confirms the chip answers; PDI_OK, or the
 * code that names what is wrong.
 */
pdi_err_t At24Driver::probe(driver_device_t &dev, iSerialInterface *bus)
{
  if( nullptr == bus || SERIAL_IFACE_I2C != serialFamily(bus->type()) ){
    return PDI_ERR_NOT_SUPPORTED;
  }

  uint32_t size = 0, page = 0;
  pdiutil::string sizekey = CHARPTR_WRAP(DRIVER_PARAM_SIZE), pagekey = CHARPTR_WRAP(DRIVER_PARAM_PAGE);
  if( !DeviceManager::param(dev, sizekey.c_str(), size) || !DeviceManager::param(dev, pagekey.c_str(), page) ){
    return PDI_ERR_INVALID_ARG;
  }

  if( 0 == size || 0 != (size & (size - 1)) || size > AT24_SIZE_MAX ||
      0 == page || page > size || page > I2C_TRANSFER_MAX ){
    return PDI_ERR_RANGE;
  }

  iI2cSerialInterface *i2c = static_cast<iI2cSerialInterface *>(bus);
  if( !i2c->probe(dev.m_target) ){
    return PDI_ERR_NOT_FOUND;
  }

  m_bus = i2c;
  m_target = dev.m_target;
  m_size = size;
  m_page = page;

  uint8_t first = 0;
  int32_t got = readAt(0, &first, 1);
  if( got < 0 ){
    m_bus = nullptr;
    return (pdi_err_t)got;
  }

  return PDI_OK;
}

/**
 * Lets go of the bus.
 */
void At24Driver::remove(driver_device_t &dev)
{
  m_bus = nullptr;
}

/**
 * Whether a read or write is under way, during which the chip may answer nothing
 * while it stores a page.
 */
bool At24Driver::busy() const
{
  return m_busy;
}

/**
 * Bytes the chip holds.
 */
uint32_t At24Driver::size() const
{
  return m_size;
}

/**
 * Gives the bus address of the block holding the offset and puts its word address
 * in the frame; the bytes it took.
 */
uint8_t At24Driver::address(uint32_t offset, uint8_t *frame, uint32_t &target) const
{
  if( m_size > AT24_ONE_BYTE_ADDRESS_MAX ){
    target = m_target + (offset >> 16);
    frame[0] = (uint8_t)((offset >> 8) & 0xFF);
    frame[1] = (uint8_t)(offset & 0xFF);
    return 2;
  }

  target = m_target + (offset >> 8);
  frame[0] = (uint8_t)(offset & 0xFF);
  return 1;
}

/**
 * Bytes read from the offset up to the end of the chip, busy throughout and
 * yielding between chunks.
 */
int32_t At24Driver::readAt(uint32_t offset, uint8_t *buf, uint32_t len)
{
  if( nullptr == buf ){
    return PDI_ERR_NULL_PTR;
  }
  if( nullptr == m_bus ){
    return PDI_ERR_NOT_INITIALIZED;
  }
  if( offset >= m_size ){
    return 0;
  }
  if( len > m_size - offset ){
    len = m_size - offset;
  }

  uint32_t reach = (m_size > AT24_ONE_BYTE_ADDRESS_MAX) ? 65536 : 256;
  uint32_t done = 0;
  bool wasbusy = m_busy;
  m_busy = true;

  while( done < len ){

    uint32_t at = offset + done;
    uint32_t chunk = len - done;
    if( chunk > I2C_TRANSFER_MAX ){
      chunk = I2C_TRANSFER_MAX;
    }
    if( chunk > reach - (at % reach) ){
      chunk = reach - (at % reach);
    }

    uint8_t frame[2];
    uint32_t target = 0;
    uint8_t used = address(at, frame, target);
    int32_t got = m_bus->transferTo(target, frame, used, buf + done, chunk);
    if( got < 0 || (uint32_t)got != chunk ){
      m_busy = wasbusy;
      return (got < 0) ? got : PDI_ERR_IO;
    }
    done += chunk;

    if( done < len ){
      __i_dvc_ctrl.yield();
    }
  }

  m_busy = wasbusy;
  return (int32_t)done;
}

/**
 * Bytes stored from the offset up to the end of the chip, a page at a time, busy
 * throughout and yielding between pages.
 */
int32_t At24Driver::writeAt(uint32_t offset, const uint8_t *buf, uint32_t len)
{
  if( nullptr == buf ){
    return PDI_ERR_NULL_PTR;
  }
  if( nullptr == m_bus ){
    return PDI_ERR_NOT_INITIALIZED;
  }
  if( offset >= m_size ){
    return PDI_ERR_NO_SPACE;
  }
  if( len > m_size - offset ){
    len = m_size - offset;
  }

  uint8_t frame[2 + I2C_TRANSFER_MAX];
  uint32_t done = 0;
  m_busy = true;

  while( done < len ){

    uint32_t at = offset + done;
    uint32_t chunk = len - done;
    if( chunk > m_page - (at % m_page) ){
      chunk = m_page - (at % m_page);
    }

    uint32_t target = 0;
    uint8_t used = address(at, frame, target);
    memcpy(frame + used, buf + done, chunk);

    int32_t sent = m_bus->transferTo(target, frame, used + chunk, nullptr, 0);
    if( sent < 0 || (uint32_t)sent != used + chunk ){
      m_busy = false;
      return (sent < 0) ? sent : PDI_ERR_IO;
    }

    pdi_err_t stored = waitStored(target);
    if( PDI_OK != stored ){
      m_busy = false;
      return stored;
    }
    done += chunk;

    if( done < len ){
      __i_dvc_ctrl.yield();
    }
  }

  m_busy = false;
  return (int32_t)done;
}

/**
 * Waits for the chip to answer again once it has stored a page, giving up after the
 * longest a page may take.
 */
pdi_err_t At24Driver::waitStored(uint32_t target)
{
  uint32_t began = __i_dvc_ctrl.millis_now();
  while( !m_bus->probe(target) ){
    if( (__i_dvc_ctrl.millis_now() - began) >= AT24_WRITE_TIMEOUT_MS ){
      return PDI_ERR_TIMEOUT;
    }
    __i_dvc_ctrl.yield();
  }
  return PDI_OK;
}

#endif
