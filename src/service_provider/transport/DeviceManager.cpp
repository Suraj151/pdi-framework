/****************************** Device Manager ********************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 19th Oct 2026
******************************************************************************/

#include "DeviceManager.h"

#ifdef ENABLE_DRIVER_CORE

#include <interface/pdi.h>
#include <interface/pdi/modules/serial/iUsbHostSerialInterface.h>
#include <interface/pdi/modules/serial/iI2cSerialInterface.h>
#include <interface/pdi/modules/serial/iSpiSerialInterface.h>
#include <utility/DataTypeConversions.h>
#include <interface/pdi/modules/driver/iBlockDriverInterface.h>
#if defined(ENABLE_DISK_FS) && defined(ENABLE_DEVFS)
#include <interface/pdi/impl/modules/storage/DiskTools.h>
#endif

namespace {

/**
 * Whether the character separates the fields of a declaration.
 */
bool isFieldBlank(char c)
{
  return ' ' == c || '\t' == c || '\r' == c;
}

/**
 * Whether the span is one setting: a name of lowercase letters, digits and
 * underscores, an equals sign, and a value.
 */
bool isSetting(const char *span, uint16_t len)
{
  uint16_t i = 0;
  while( i < len && (('a' <= span[i] && 'z' >= span[i]) || ('0' <= span[i] && '9' >= span[i]) || '_' == span[i]) ){
    i++;
  }
  return i > 0 && i + 1 < len && '=' == span[i];
}

/**
 * Whether the span reads exactly as the literal.
 */
bool spanIs(const char *span, uint16_t len, const char *literal)
{
  uint16_t n = (uint16_t)strlen(literal);
  return n == len && 0 == strncmp(span, literal, n);
}

/**
 * A whole number written in decimal, or in hexadecimal after 0x, that fits in
 * 32 bits; false for anything else.
 */
bool spanNumber(const char *span, uint16_t len, uint32_t &value)
{
  value = 0;

  if( len > 2 && '0' == span[0] && ('x' == span[1] || 'X' == span[1]) ){

    if( len - 2 > 8 ){
      return false;
    }

    for( uint16_t i = 2; i < len; i++ ){
      char c = span[i];
      uint8_t digit;
      if( c >= '0' && c <= '9' ) digit = (uint8_t)(c - '0');
      else if( c >= 'a' && c <= 'f' ) digit = (uint8_t)(10 + c - 'a');
      else if( c >= 'A' && c <= 'F' ) digit = (uint8_t)(10 + c - 'A');
      else return false;
      value = (value << 4) | digit;
    }
    return true;
  }

  if( 0 == len || len > 10 ){
    return false;
  }

  uint64_t wide = 0;
  for( uint16_t i = 0; i < len; i++ ){
    if( span[i] < '0' || span[i] > '9' ){
      return false;
    }
    wide = wide * 10 + (uint64_t)(span[i] - '0');
  }

  if( wide > 0xFFFFFFFFULL ){
    return false;
  }

  value = (uint32_t)wide;
  return true;
}

/**
 * Appends the value as hex digits, zero padded to the width given.
 */
void appendHex(pdiutil::string &out, uint32_t value, uint8_t width)
{
  char digits[9] = {0};
  Uint32ToHexString(value, digits, sizeof(digits), false, width);
  out += digits;
}

/**
 * Appends the value as decimal digits.
 */
void appendDecimal(pdiutil::string &out, uint32_t value)
{
  char digits[12] = {0};
  Uint32ToString(value, digits, sizeof(digits));
  out += digits;
}

/**
 * The interface a bus name such as i2c1 stands for, SERIAL_IFACE_MAX when it
 * names none.
 */
serial_iface_t busFromName(const char *span, uint16_t len)
{
  pdiutil::string name;
  for( uint8_t i = 0; i < SERIAL_IFACE_MAX; i++ ){
    if( !serialInterfaceName((serial_iface_t)i, name) ){
      continue;
    }
    uint16_t namelen = (uint16_t)name.size();
    if( len != namelen + 1 || 0 != strncmp(span, name.c_str(), namelen) ){
      continue;
    }
    if( span[namelen] == (char)('0' + serialInterfaceIndex((serial_iface_t)i)) ){
      return (serial_iface_t)i;
    }
  }
  return SERIAL_IFACE_MAX;
}

}

/**
 * DeviceManager constructor.
 */
DeviceManager::DeviceManager() : m_held_pins(0), m_held_buses(0), m_conf_size(0), m_conf_mtime(0), m_conf_known(false), m_probe_ms(0)
{
#if defined(ENABLE_DISK_FS) && defined(ENABLE_DEVFS)
  m_fstab_listener = __utl_event.add_event_listener(EVENT_DEVICE_ADDED, [](void *e){
    driver_device_t *dev = reinterpret_cast<driver_device_t *>(e);
    if( nullptr == iBlockDriverInterface::diskOf(dev) ){
      return;
    }
    DiskTools::mountFstab([](const pdiutil::string &device, pdi_err_t code){
      SysLogW("DEV: %s not mounted from fstab: %d\n", device.c_str(), (int)code);
    });
  });
#endif
}

/**
 * Lets go of every bound device, giving a busy driver up to DEVICE_BUSY_WAIT_MS to
 * finish first.
 */
DeviceManager::~DeviceManager()
{
#if defined(ENABLE_DISK_FS) && defined(ENABLE_DEVFS)
  __utl_event.remove_event_listener(m_fstab_listener);
#endif
  if( anyBusy() ){
    uint32_t began = __i_dvc_ctrl.millis_now();
    while( anyBusy() && (__i_dvc_ctrl.millis_now() - began) < DEVICE_BUSY_WAIT_MS ){
      __i_dvc_ctrl.yield();
    }
  }

  for( uint16_t i = 0; i < m_devices.size(); i++ ){
    unbind(m_devices[i]);
  }
  m_devices.clear();
}

/**
 * Adds a driver to the ones every manager matches devices against.
 */
void DeviceManager::registerDriver(const driver_prop_t &prop)
{
  if( nullptr == prop.m_name || nullptr == prop.m_create ){
    return;
  }

  pdiutil::vector<driver_prop_t> &registry = drivers();
  pdiutil::string name = CHARPTR_WRAP_RO(prop.m_name);
  for( uint16_t i = 0; i < registry.size(); i++ ){
    pdiutil::string known = CHARPTR_WRAP_RO(registry[i].m_name);
    if( known == name ){
      return;
    }
  }

  registry.push_back(prop);
}

/**
 * Every driver compiled in, created on first use so a driver registering
 * itself during static start up always finds it.
 */
pdiutil::vector<driver_prop_t> &DeviceManager::drivers()
{
  static pdiutil::vector<driver_prop_t> registry;
  return registry;
}

/**
 * One pass: reread changed declarations, scan every interface, bind what appeared
 * and release what has gone.
 */
void DeviceManager::scan()
{
#ifdef ENABLE_STORAGE_SERVICE
  if( !anyBusy() && confChanged() ){
    loadConf();
  }
#endif

  for( uint16_t i = 0; i < m_devices.size(); i++ ){
    m_devices[i].m_seen = false;
  }

  for( uint8_t i = 0; i < SERIAL_IFACE_MAX; i++ ){
    iSerialInterface *iface = iSerialInterface::instances[i];
    if( nullptr != iface ){
      look((serial_iface_t)i, iface);
    }
  }

  reconcile();
  retryMissing();
}

/**
 * Rereads the declarations, gives each unbound device another chance, restores
 * every left out device, then scans.
 */
void DeviceManager::trigger()
{
  m_conf_known = false;

  for( uint16_t i = 0; i < m_devices.size(); i++ ){
    driver_device_t &dev = m_devices[i];
    dev.m_unscanned = false;
    if( DEVICE_STATE_PRESENT == dev.m_state || DEVICE_STATE_FAILED == dev.m_state ){
      bind(dev);
    }
  }

  scan();
}

/**
 * How many devices are known.
 */
uint16_t DeviceManager::count() const
{
  return (uint16_t)m_devices.size();
}

/**
 * One known device, nullptr past the end.
 */
const driver_device_t *DeviceManager::at(uint16_t index) const
{
  return (index < m_devices.size()) ? &m_devices[index] : nullptr;
}

/**
 * The first driver bound that presents a device of the class, nullptr when none is.
 * Valid only until the next scan.
 */
iDriverInterface *DeviceManager::bound(uint8_t cls) const
{
  for( uint16_t i = 0; i < m_devices.size(); i++ ){
    const driver_device_t &dev = m_devices[i];
    if( DEVICE_STATE_BOUND == dev.m_state && nullptr != dev.m_instance && cls == dev.m_instance->driverClass() ){
      return dev.m_instance;
    }
  }
  return nullptr;
}

/**
 * The device at a target on an interface, nullptr when none is known.
 */
const driver_device_t *DeviceManager::find(serial_iface_t bus, uint32_t target) const
{
  for( uint16_t i = 0; i < m_devices.size(); i++ ){
    if( bus == m_devices[i].m_bus && target == m_devices[i].m_target ){
      return &m_devices[i];
    }
  }
  return nullptr;
}

/**
 * The name a device goes by under /sys/bus, the way linux names it.
 */
bool DeviceManager::deviceId(const driver_device_t &dev, pdiutil::string &out)
{
  out.clear();
  serial_iface_t family = serialFamily(dev.m_bus);

  if( SERIAL_IFACE_I2C == family ){
    appendDecimal(out, (SERIAL_IFACE_I2C1 == dev.m_bus) ? 1 : 0);
    out += '-';
    appendHex(out, dev.m_target, 4);
    return true;
  }

  if( SERIAL_IFACE_USB_HOST == family ){
    out += '1';
    out += '-';
    appendDecimal(out, dev.m_target);
    return true;
  }

  if( !busName(dev.m_bus, out) ){
    return false;
  }

  out += '.';
  appendDecimal(out, dev.m_target);
  return true;
}

/**
 * The name an interface goes by in /etc/devices.conf and under /sys/bus.
 */
bool DeviceManager::busName(serial_iface_t bus, pdiutil::string &out)
{
  if( !serialInterfaceName(bus, out) ){
    return false;
  }
  appendDecimal(out, serialInterfaceIndex(bus));
  return true;
}

/**
 * Reads one /etc/devices.conf line into a declared device, false for a
 * comment, a blank line or one that does not parse.
 */
bool DeviceManager::parseLine(const char *line, uint16_t len, driver_device_t &out)
{
  if( nullptr == line ){
    return false;
  }

  const char *field[3] = {nullptr, nullptr, nullptr};
  uint16_t fieldlen[3] = {0, 0, 0};
  uint8_t fields = 0;
  uint16_t i = 0;

  while( i < len && isFieldBlank(line[i]) ) i++;

  if( i >= len || DEVICES_CONF_COMMENT_CHAR == line[i] ){
    return false;
  }

  while( i < len && fields < 3 ){

    field[fields] = line + i;
    while( i < len && !isFieldBlank(line[i]) ) i++;
    fieldlen[fields] = (uint16_t)((line + i) - field[fields]);
    fields++;

    while( i < len && isFieldBlank(line[i]) ) i++;
  }

  if( 3 != fields ){
    return false;
  }

  pdiutil::string params;
  uint8_t settings = 0;

  while( i < len ){

    const char *setting = line + i;
    while( i < len && !isFieldBlank(line[i]) ) i++;
    uint16_t settinglen = (uint16_t)((line + i) - setting);

    if( !isSetting(setting, settinglen) || ++settings > DRIVER_PARAMS_MAX ){
      return false;
    }

    if( !params.empty() ){
      params += ' ';
    }
    params.append(setting, settinglen);

    while( i < len && isFieldBlank(line[i]) ) i++;
  }

  serial_iface_t bus = busFromName(field[0], fieldlen[0]);
  if( SERIAL_IFACE_MAX == bus ){
    return false;
  }

  uint32_t target = 0;
  serial_iface_t family = serialFamily(bus);
  pdiutil::string buskey = CHARPTR_WRAP(DEVICE_CONF_BUS);

  if( spanIs(field[1], fieldlen[1], buskey.c_str()) ){
    pdiutil::string familyname;
    if( !serialInterfaceName(family, familyname) || !spanIs(field[2], fieldlen[2], familyname.c_str()) ){
      return false;
    }
    target = DEVICE_TARGET_BUS;
  }else if( SERIAL_IFACE_SPI == family ){
    pdiutil::string chipselect = CHARPTR_WRAP(DEVICE_CONF_CHIP_SELECT);
    uint16_t cslen = (uint16_t)chipselect.size();
    if( fieldlen[1] <= cslen || 0 != strncmp(field[1], chipselect.c_str(), cslen) ||
        !spanNumber(field[1] + cslen, (uint16_t)(fieldlen[1] - cslen), target) ){
      return false;
    }
  }else if( SERIAL_IFACE_I2S == family ){
    pdiutil::string capture = CHARPTR_WRAP(DEVICE_CONF_CAPTURE), playback = CHARPTR_WRAP(DEVICE_CONF_PLAYBACK);
    if( spanIs(field[1], fieldlen[1], capture.c_str()) ){
      target = 0;
    }else if( spanIs(field[1], fieldlen[1], playback.c_str()) ){
      target = 1;
    }else{
      return false;
    }
  }else if( !spanNumber(field[1], fieldlen[1], target) ){
    return false;
  }

  if( fieldlen[2] > DRIVER_NAME_MAX ){
    return false;
  }

  out = driver_device_t();
  out.m_bus = bus;
  out.m_target = target;
  out.m_declared = true;
  out.m_driver = pdiutil::string(field[2], fieldlen[2]);
  out.m_params = params;
  return true;
}

/**
 * A whole number setting the declaration gave the device, false when it gave
 * none by that name.
 */
bool DeviceManager::param(const driver_device_t &dev, const char *key, uint32_t &value)
{
  if( nullptr == key ){
    return false;
  }

  uint16_t keylen = (uint16_t)strlen(key);
  const char *p = dev.m_params.c_str();

  while( '\0' != *p ){

    const char *setting = p;
    while( '\0' != *p && ' ' != *p ) p++;
    uint16_t settinglen = (uint16_t)(p - setting);

    if( settinglen > keylen && '=' == setting[keylen] && 0 == strncmp(setting, key, keylen) ){
      return spanNumber(setting + keylen + 1, (uint16_t)(settinglen - keylen - 1), value);
    }

    while( ' ' == *p ) p++;
  }

  return false;
}

/**
 * Whether a line in /etc/devices.conf holds the hardware pin for its bus.
 */
bool DeviceManager::holdsPin(uint8_t hwpin) const
{
  return hwpin < DRIVER_GPIO_HOLD_MAX && 0 != ((m_held_pins >> hwpin) & 1ULL);
}

/**
 * What an interface's bus line came to: PDI_OK, the refusing code, or
 * PDI_ERR_NOT_FOUND when there is none.
 */
pdi_err_t DeviceManager::busResult(serial_iface_t bus) const
{
  for( uint16_t i = 0; i < m_bus_lines.size(); i++ ){
    if( bus == m_bus_lines[i].m_bus ){
      return (pdi_err_t)m_bus_lines[i].m_result;
    }
  }
  return PDI_ERR_NOT_FOUND;
}

/**
 * The hardware pins a line uses on its bus: the ones it names, or the bus's
 * own; how many.
 */
uint8_t DeviceManager::linePins(const driver_device_t &line, iSerialInterface *iface, uint8_t *out)
{
  if( nullptr == iface || nullptr == out ){
    return 0;
  }

  serial_iface_t family = serialFamily(line.m_bus);
  uint8_t n = 0;

  if( SERIAL_IFACE_I2C == family ){
    iI2cSerialInterface *i2c = static_cast<iI2cSerialInterface *>(iface);
    if( i2c->sda() >= 0 ) out[n++] = (uint8_t)i2c->sda();
    if( i2c->scl() >= 0 ) out[n++] = (uint8_t)i2c->scl();
    return n;
  }

  if( SERIAL_IFACE_SPI == family ){
    iSpiSerialInterface *spi = static_cast<iSpiSerialInterface *>(iface);
    if( spi->sck() >= 0 ) out[n++] = (uint8_t)spi->sck();
    if( spi->miso() >= 0 ) out[n++] = (uint8_t)spi->miso();
    if( spi->mosi() >= 0 ) out[n++] = (uint8_t)spi->mosi();
    if( DEVICE_TARGET_BUS != line.m_target && line.m_target < DRIVER_GPIO_HOLD_MAX ) out[n++] = (uint8_t)line.m_target;
    return n;
  }

  if( SERIAL_IFACE_I2S == family ){
    pdiutil::string keys[3] = {CHARPTR_WRAP(DRIVER_PARAM_SCK), CHARPTR_WRAP(DRIVER_PARAM_WS), CHARPTR_WRAP(DRIVER_PARAM_SD)};
    bool named = false;
    for( uint8_t k = 0; k < 3; k++ ){
      uint32_t value = 0;
      if( param(line, keys[k].c_str(), value) ){
        named = true;
        if( value < DRIVER_GPIO_HOLD_MAX ){
          out[n++] = (uint8_t)value;
        }
      }
    }
    return named ? n : iface->defaultPins(out, DRIVER_PINS_MAX);
  }

  return 0;
}

/**
 * Puts the interface on its bus line's settings: PDI_OK, or the code that
 * names why not.
 */
pdi_err_t DeviceManager::applyBusLine(const driver_device_t &line, iSerialInterface *iface, uint64_t others)
{
  serial_iface_t family = serialFamily(line.m_bus);
  pdiutil::string keys[3];
  uint8_t keycount = 0;

  if( SERIAL_IFACE_I2C == family ){
    keys[0] = CHARPTR_WRAP(DRIVER_PARAM_SDA);
    keys[1] = CHARPTR_WRAP(DRIVER_PARAM_SCL);
    keycount = 2;
  }else if( SERIAL_IFACE_SPI == family ){
    keys[0] = CHARPTR_WRAP(DRIVER_PARAM_SCK);
    keys[1] = CHARPTR_WRAP(DRIVER_PARAM_MISO);
    keys[2] = CHARPTR_WRAP(DRIVER_PARAM_MOSI);
    keycount = 3;
  }else{
    return PDI_ERR_NOT_SUPPORTED;
  }

  int16_t pins[3] = {-1, -1, -1};
  uint8_t named = 0;
  uint64_t wanted = 0;
  for( uint8_t k = 0; k < keycount; k++ ){
    uint32_t value = 0;
    if( !param(line, keys[k].c_str(), value) ){
      continue;
    }
    if( value >= DRIVER_GPIO_HOLD_MAX || 0 != (wanted & (1ULL << value)) ){
      return PDI_ERR_INVALID_ARG;
    }
    pins[k] = (int16_t)value;
    wanted |= 1ULL << value;
    named++;
  }

  if( 0 != named && keycount != named ){
    return PDI_ERR_INVALID_ARG;
  }
  if( 0 != (others & wanted) ){
    return PDI_ERR_BUSY;
  }

  uint32_t clock = 0;
  pdiutil::string clockkey = CHARPTR_WRAP(DRIVER_PARAM_CLOCK);
  param(line, clockkey.c_str(), clock);

  pdi_err_t configured = configureBus(iface, family, pins, clock);
  if( PDI_OK != configured ){
    return configured;
  }

  uint8_t held[DRIVER_PINS_MAX];
  uint8_t count = linePins(line, iface, held);
  for( uint8_t p = 0; p < count; p++ ){
    if( held[p] < DRIVER_GPIO_HOLD_MAX && 0 != (others & (1ULL << held[p])) ){
      int16_t none[3] = {-1, -1, -1};
      configureBus(iface, family, none, 0);
      return PDI_ERR_BUSY;
    }
  }
  return PDI_OK;
}

/**
 * Puts a bus whose pins the framework hands out on the pins and clock given;
 * PDI_ERR_NOT_SUPPORTED for any other.
 */
pdi_err_t DeviceManager::configureBus(iSerialInterface *iface, serial_iface_t family, const int16_t *pins, uint32_t clock)
{
  if( SERIAL_IFACE_I2C == family ){
    return static_cast<iI2cSerialInterface *>(iface)->configure(pins[0], pins[1], clock);
  }
  if( SERIAL_IFACE_SPI == family ){
    return static_cast<iSpiSerialInterface *>(iface)->configure(pins[0], pins[1], pins[2], clock);
  }
  return PDI_ERR_NOT_SUPPORTED;
}

/**
 * Puts each bus on its line's pins, or its own, and works out which pins the lines
 * hold, refusing a line whose pins another holds.
 */
void DeviceManager::holdPins()
{
  uint64_t held[SERIAL_IFACE_MAX];
  bool configured[SERIAL_IFACE_MAX];
  for( uint8_t b = 0; b < SERIAL_IFACE_MAX; b++ ){
    held[b] = 0;
    configured[b] = false;
  }

  uint8_t pins[DRIVER_PINS_MAX];

  for( uint16_t i = 0; i < m_bus_lines.size(); i++ ){

    driver_device_t &line = m_bus_lines[i];
    iSerialInterface *iface = iSerialInterface::getSerialInstance(line.m_bus);
    uint64_t others = 0;
    for( uint8_t b = 0; b < SERIAL_IFACE_MAX; b++ ){
      others |= (b != line.m_bus) ? held[b] : 0;
    }

    if( nullptr == iface ){
      line.m_result = PDI_ERR_NOT_FOUND;
    }else if( configured[line.m_bus] ){
      line.m_result = PDI_ERR_EXISTS;
    }else{
      line.m_result = applyBusLine(line, iface, others);
    }

    if( PDI_OK == line.m_result ){
      configured[line.m_bus] = true;
      uint8_t count = linePins(line, iface, pins);
      for( uint8_t p = 0; p < count; p++ ){
        held[line.m_bus] |= (pins[p] < DRIVER_GPIO_HOLD_MAX) ? (1ULL << pins[p]) : 0;
      }
      continue;
    }

    pdiutil::string bus;
    busName(line.m_bus, bus);
    SysLogW("DEV: %s bus refused: %d\n", bus.c_str(), (int)line.m_result);
  }

  for( uint8_t b = 0; b < SERIAL_IFACE_MAX; b++ ){
    iSerialInterface *iface = iSerialInterface::instances[b];
    if( nullptr != iface && !configured[b] ){
      int16_t none[3] = {-1, -1, -1};
      configureBus(iface, serialFamily((serial_iface_t)b), none, 0);
    }
  }

  for( uint16_t i = 0; i < m_devices.size(); i++ ){

    driver_device_t &dev = m_devices[i];
    if( !dev.m_declared ){
      continue;
    }

    uint64_t mine = 0, others = 0;
    bool repeated = false;
    uint8_t count = linePins(dev, iSerialInterface::getSerialInstance(dev.m_bus), pins);
    for( uint8_t p = 0; p < count; p++ ){
      uint64_t bit = (pins[p] < DRIVER_GPIO_HOLD_MAX) ? (1ULL << pins[p]) : 0;
      repeated = repeated || (0 != (mine & bit));
      mine |= bit;
    }
    for( uint8_t b = 0; b < SERIAL_IFACE_MAX; b++ ){
      others |= (b != dev.m_bus) ? held[b] : 0;
    }

    dev.m_pin_clash = repeated || (0 != (mine & others));
    if( !dev.m_pin_clash ){
      held[dev.m_bus] |= mine;
    }
  }

  m_held_pins = 0;
  m_held_buses = 0;
  for( uint8_t b = 0; b < SERIAL_IFACE_MAX; b++ ){
    m_held_pins |= held[b];
    m_held_buses |= (0 != held[b]) ? (1UL << b) : 0;
  }

  for( uint16_t i = 0; i < m_devices.size(); i++ ){

    driver_device_t &dev = m_devices[i];
    if( !dev.m_declared ){
      continue;
    }

    if( dev.m_pin_clash && DEVICE_STATE_BOUND == dev.m_state ){
      unbind(dev);
      bind(dev);
    }else if( !dev.m_pin_clash && DEVICE_STATE_FAILED == dev.m_state && PDI_ERR_BUSY == dev.m_result ){
      bind(dev);
    }
  }
}

/**
 * Whether any bound driver is in the middle of an operation on its device.
 */
bool DeviceManager::anyBusy() const
{
  for( uint16_t i = 0; i < m_devices.size(); i++ ){
    if( nullptr != m_devices[i].m_instance && m_devices[i].m_instance->busy() ){
      return true;
    }
  }
  return false;
}

/**
 * Whether a scan may look on the interface: one with framework pins only while a
 * line holds them.
 */
bool DeviceManager::lookable(serial_iface_t bus, iSerialInterface *iface) const
{
  uint8_t pins[DRIVER_PINS_MAX];
  if( 0 == iface->defaultPins(pins, DRIVER_PINS_MAX) ){
    return true;
  }
  return 0 != (m_held_buses & (1UL << bus));
}

/**
 * The device at a target on an interface, made known when it was not.
 */
driver_device_t *DeviceManager::note(serial_iface_t bus, uint32_t target)
{
  for( uint16_t i = 0; i < m_devices.size(); i++ ){
    if( bus == m_devices[i].m_bus && target == m_devices[i].m_target ){
      m_devices[i].m_seen = true;
      return &m_devices[i];
    }
  }

  driver_device_t dev;
  dev.m_bus = bus;
  dev.m_target = target;
  dev.m_seen = true;
  m_devices.push_back(dev);

  if( m_devices.empty() || bus != m_devices.back().m_bus || target != m_devices.back().m_target ){
    return nullptr;
  }
  return &m_devices.back();
}

/**
 * Finds what is on one interface now: addresses a driver answers to, devices the
 * port reports and devices declared on it.
 */
void DeviceManager::look(serial_iface_t bus, iSerialInterface *iface)
{
  serial_iface_t family = serialFamily(bus);

  if( SERIAL_IFACE_I2C == family && lookable(bus, iface) ){
    pdiutil::vector<driver_prop_t> &registry = drivers();
    for( uint16_t d = 0; d < registry.size(); d++ ){
      if( SERIAL_IFACE_I2C != registry[d].m_family ){
        continue;
      }
      for( uint8_t a = 0; a < registry[d].m_address_count; a++ ){
        uint32_t address = registry[d].m_addresses[a];
        if( iface->probe(address) ){
          note(bus, address);
        }
      }
    }
  }

  if( SERIAL_IFACE_USB_HOST == family ){
    iUsbHostSerialInterface *usb = static_cast<iUsbHostSerialInterface *>(iface);
    usb_device_info_t info;
    for( uint8_t u = 0; u < usb->deviceCount(); u++ ){
      if( !usb->deviceAt(u, info) ){
        continue;
      }
      driver_device_t *dev = note(bus, info.m_address);
      if( nullptr != dev ){
        dev->m_vid = info.m_vid;
        dev->m_pid = info.m_pid;
        dev->m_class = info.m_class;
      }
    }
  }

  for( uint16_t i = 0; i < m_devices.size(); i++ ){
    driver_device_t &dev = m_devices[i];
    if( !dev.m_declared || bus != dev.m_bus || dev.m_seen ){
      continue;
    }
    bool detectable = (SERIAL_IFACE_I2C == family || SERIAL_IFACE_USB_HOST == family);
    bool busy = (nullptr != dev.m_instance) && dev.m_instance->busy();
    if( dev.m_unscanned ){
      dev.m_seen = (DEVICE_STATE_ABSENT != dev.m_state);
      continue;
    }
    if( !detectable && !busy && !dev.m_pin_clash && rescanned(dev) ){
      if( DEVICE_STATE_BOUND != dev.m_state || nullptr == dev.m_instance ){
        dev.m_seen = (DEVICE_STATE_ABSENT != dev.m_state);
        continue;
      }
      uint32_t asked = __i_dvc_ctrl.millis_now();
      dev.m_seen = dev.m_instance->stillThere();
      uint32_t answered = __i_dvc_ctrl.millis_now() - asked;
      if( answered > DEVICE_SCAN_PROBE_MAX_MS ){
        leaveOutOfScan(dev, answered);
      }
      continue;
    }
    if( !detectable || busy || dev.m_pin_clash ){
      dev.m_seen = true;
      continue;
    }
    uint32_t began = __i_dvc_ctrl.millis_now();
    dev.m_seen = iface->probe(dev.m_target);
    uint32_t took = __i_dvc_ctrl.millis_now() - began;
    if( took > DEVICE_SCAN_PROBE_MAX_MS ){
      leaveOutOfScan(dev, took);
    }
  }
}

/**
 * Takes a device whose probe held the scan too long out of later scans, until
 * udevadm trigger probes it again.
 */
void DeviceManager::leaveOutOfScan(driver_device_t &dev, uint32_t took)
{
  dev.m_unscanned = true;
  dev.m_seen = (DEVICE_STATE_ABSENT != dev.m_state);
  pdiutil::string id;
  pdiutil::string bus;
  deviceId(dev, id);
  busName(dev.m_bus, bus);
  SysLogW("DEV: %s %s probe took %d ms, left to udevadm trigger\n", bus.c_str(), id.c_str(), (int)took);
}

/**
 * Binds what turned up since the last pass and lets go of what has gone.
 */
void DeviceManager::reconcile()
{
  for( int32_t i = (int32_t)m_devices.size() - 1; i >= 0; i-- ){

    driver_device_t &dev = m_devices[i];

    if( dev.m_seen && DEVICE_STATE_ABSENT == dev.m_state ){
      dev.m_state = DEVICE_STATE_PRESENT;
      bind(dev);
      announce(EVENT_DEVICE_ADDED, dev);
      continue;
    }

    if( dev.m_seen || DEVICE_STATE_ABSENT == dev.m_state ){
      continue;
    }

    unbind(dev);
    announce(EVENT_DEVICE_REMOVED, dev);

    if( dev.m_declared ){
      dev.m_state = DEVICE_STATE_ABSENT;
    }else{
      m_devices.erase(m_devices.begin() + i);
    }
  }
}

/**
 * Whether the driver a declared device names registered to be rescanned.
 */
bool DeviceManager::rescanned(const driver_device_t &dev)
{
  pdiutil::vector<driver_prop_t> &registry = drivers();
  serial_iface_t family = serialFamily(dev.m_bus);
  for( uint16_t d = 0; d < registry.size(); d++ ){
    if( family != registry[d].m_family || !registry[d].m_rescan ){
      continue;
    }
    pdiutil::string name = CHARPTR_WRAP_RO(registry[d].m_name);
    if( name == dev.m_driver ){
      return true;
    }
  }
  return false;
}

/**
 * Offers each missing or refused declared device to its rescanned driver again, so
 * it binds with no trigger.
 */
void DeviceManager::retryMissing()
{
  for( uint16_t i = 0; i < m_devices.size(); i++ ){
    driver_device_t &dev = m_devices[i];
    bool waiting = (DEVICE_STATE_ABSENT == dev.m_state || DEVICE_STATE_FAILED == dev.m_state);
    if( !dev.m_declared || !waiting || dev.m_unscanned || dev.m_pin_clash ||
        nullptr == iSerialInterface::getSerialInstance(dev.m_bus) || !rescanned(dev) ){
      continue;
    }
    m_probe_ms = 0;
    bind(dev, true);
    if( DEVICE_STATE_BOUND == dev.m_state ){
      announce(EVENT_DEVICE_ADDED, dev);
    }else if( m_probe_ms > DEVICE_SCAN_PROBE_MAX_MS ){
      leaveOutOfScan(dev, m_probe_ms);
    }
  }
}

/**
 * Offers the device to each driver that could drive it until one confirms it,
 * leaving it unbound when none does.
 */
void DeviceManager::bind(driver_device_t &dev, bool quiet)
{
  int16_t before = dev.m_result;
  iSerialInterface *iface = iSerialInterface::getSerialInstance(dev.m_bus);
  serial_iface_t family = serialFamily(dev.m_bus);
  pdiutil::vector<driver_prop_t> &registry = drivers();

  if( dev.m_declared && dev.m_pin_clash ){
    pdiutil::string id;
    pdiutil::string bus;
    deviceId(dev, id);
    busName(dev.m_bus, bus);
    SysLogW("DEV: %s %s refused, a pin it needs is already in use: %d\n", bus.c_str(), id.c_str(), (int)PDI_ERR_BUSY);
    dev.m_result = PDI_ERR_BUSY;
    dev.m_state = DEVICE_STATE_FAILED;
    return;
  }

  dev.m_result = PDI_ERR_NOT_FOUND;

  for( uint16_t d = 0; d < registry.size() && nullptr != iface; d++ ){

    const driver_prop_t &prop = registry[d];

    if( family != prop.m_family ){
      continue;
    }

    pdiutil::string name = CHARPTR_WRAP_RO(prop.m_name);

    if( dev.m_declared ){
      if( dev.m_driver != name ){
        continue;
      }
    }else{
      bool listed = false;
      for( uint8_t a = 0; a < prop.m_address_count; a++ ){
        if( dev.m_target == prop.m_addresses[a] ){
          listed = true;
          break;
        }
      }
      bool matched = (nullptr != prop.m_match) && prop.m_match(dev);
      if( !listed && !matched ){
        continue;
      }
    }

    iDriverInterface *driver = prop.m_create();
    if( nullptr == driver ){
      continue;
    }

    uint32_t asked = __i_dvc_ctrl.millis_now();
    pdi_err_t probed = driver->probe(dev, iface);
    m_probe_ms += __i_dvc_ctrl.millis_now() - asked;
    dev.m_result = probed;
    if( PDI_OK == probed ){
      dev.m_instance = driver;
      dev.m_driver = name;
      dev.m_state = DEVICE_STATE_BOUND;
      return;
    }

    pdiutil::safe_delete(driver);

    if( dev.m_declared && !(quiet && probed == before) ){
      pdiutil::string id;
      pdiutil::string bus;
      deviceId(dev, id);
      busName(dev.m_bus, bus);
      SysLogW("DEV: %s %s refused by %s: %d\n", bus.c_str(), id.c_str(), name.c_str(), (int)probed);
    }
  }

  if( !dev.m_declared ){
    dev.m_state = DEVICE_STATE_PRESENT;
  }else{
    dev.m_state = (PDI_ERR_NOT_FOUND == dev.m_result && rescanned(dev)) ? DEVICE_STATE_ABSENT : DEVICE_STATE_FAILED;
  }
}

/**
 * Lets the bound driver go of the device.
 */
void DeviceManager::unbind(driver_device_t &dev)
{
  if( nullptr != dev.m_instance ){
    dev.m_instance->remove(dev);
    pdiutil::safe_delete(dev.m_instance);
  }

  if( !dev.m_declared ){
    dev.m_driver.clear();
  }

  if( DEVICE_STATE_ABSENT != dev.m_state ){
    dev.m_state = DEVICE_STATE_PRESENT;
  }
}

/**
 * Tells the event bus and the system log a device came or went.
 */
void DeviceManager::announce(event_name_t event, driver_device_t &dev)
{
  pdiutil::string id;
  pdiutil::string bus;
  deviceId(dev, id);
  busName(dev.m_bus, bus);

  if( EVENT_DEVICE_ADDED == event ){
    SysLogI("DEV: %s %s added, driver %s\n", bus.c_str(), id.c_str(),
            dev.m_driver.empty() ? "-" : dev.m_driver.c_str());
    logLayout(dev);
  }else{
    SysLogI("DEV: %s %s removed\n", bus.c_str(), id.c_str());
  }

  __utl_event.execute_event(event, &dev);
}

/**
 * Logs a newly bound disk's partition table and each partition in it, as linux
 * does.
 */
void DeviceManager::logLayout(driver_device_t &dev)
{
  pdiutil::string node;
  iBlockDriverInterface *disk = iBlockDriverInterface::diskOf(&dev);
  if( nullptr == disk || !disk->nodeName(node) ){
    return;
  }

  pdi_err_t read = disk->scanLayout();
  if( PDI_OK != read ){
    SysLogW("DEV: %s: layout not read: %d\n", node.c_str(), (int)read);
    return;
  }
  uint8_t table = disk->tableType();
  if( BLOCK_TABLE_NONE == table ){
    SysLogI("DEV: %s: no partition table\n", node.c_str());
    return;
  }

  pdiutil::string line = blockTableName(table);
  for( uint8_t n = 1; n <= disk->partitionCount(); n++ ){
    if( nullptr == disk->partition(n) ){
      continue;
    }
    line += ' ';
    line += blockPartitionName("", n);
  }
  SysLogI("DEV: %s: %s\n", node.c_str(), line.c_str());
}

#ifdef ENABLE_STORAGE_SERVICE
/**
 * Whether the declarations changed since last read, judged by the file's size and
 * modification time.
 */
bool DeviceManager::confChanged()
{
  pdiutil::string path = CHARPTR_WRAP(DEVICES_CONF_PATH);
  file_info_t meta;

  __i_fs.beginPrivileged();
  bool exists = (PDI_OK == __i_fs.getFileMeta(path.c_str(), meta));
  __i_fs.endPrivileged();

  if( !exists ){
    bool was = m_conf_known || m_conf_size > 0;
    m_conf_known = true;
    m_conf_size = 0;
    m_conf_mtime = 0;
    return was;
  }

  if( m_conf_known && meta.m_size == m_conf_size && meta.m_mtime == m_conf_mtime ){
    return false;
  }

  m_conf_known = true;
  m_conf_size = meta.m_size;
  m_conf_mtime = meta.m_mtime;
  return true;
}

/**
 * Takes the declarations in, adding what is new and forgetting what is no longer
 * declared.
 */
void DeviceManager::loadConf()
{
  pdiutil::vector<driver_device_t> declared;
  pdiutil::vector<driver_device_t> buslines;
  pdiutil::string path = CHARPTR_WRAP(DEVICES_CONF_PATH);
  pdiutil::string line;

  __i_fs.beginPrivileged();
  if( __i_fs.isFileExist(path.c_str()) ){
    __i_fs.readFile(path.c_str(), 64, [&](char *data, uint32_t size)->bool{
      for( uint32_t i = 0; i < size; i++ ){
        if( '\n' != data[i] ){
          line += data[i];
          continue;
        }
        driver_device_t dev;
        bool parsed = parseLine(line.c_str(), (uint16_t)line.size(), dev);
        if( parsed && DEVICE_TARGET_BUS == dev.m_target ){
          buslines.push_back(dev);
        }else if( parsed ){
          declared.push_back(dev);
        }else if( !line.empty() ){
          logRefusedLine(line);
        }
        line.clear();
      }
      return true;
    });
  }
  __i_fs.endPrivileged();

  driver_device_t last;
  bool parsed = parseLine(line.c_str(), (uint16_t)line.size(), last);
  if( parsed && DEVICE_TARGET_BUS == last.m_target ){
    buslines.push_back(last);
  }else if( parsed ){
    declared.push_back(last);
  }else if( !line.empty() ){
    logRefusedLine(line);
  }

  m_bus_lines = buslines;

  for( int32_t i = (int32_t)m_devices.size() - 1; i >= 0; i-- ){

    driver_device_t &dev = m_devices[i];
    if( !dev.m_declared ){
      continue;
    }

    bool kept = false;
    for( uint16_t k = 0; k < declared.size(); k++ ){
      if( declared[k].m_bus == dev.m_bus && declared[k].m_target == dev.m_target &&
          declared[k].m_driver == dev.m_driver && declared[k].m_params == dev.m_params ){
        kept = true;
        break;
      }
    }

    if( kept ){
      continue;
    }

    if( DEVICE_STATE_ABSENT != dev.m_state ){
      unbind(dev);
      announce(EVENT_DEVICE_REMOVED, dev);
    }
    m_devices.erase(m_devices.begin() + i);
  }

  for( uint16_t k = 0; k < declared.size(); k++ ){

    driver_device_t *dev = nullptr;
    for( uint16_t i = 0; i < m_devices.size(); i++ ){
      if( declared[k].m_bus == m_devices[i].m_bus && declared[k].m_target == m_devices[i].m_target ){
        dev = &m_devices[i];
        break;
      }
    }

    if( nullptr == dev ){
      m_devices.push_back(declared[k]);
      continue;
    }

    if( dev->m_declared ){
      continue;
    }

    if( DEVICE_STATE_ABSENT != dev->m_state ){
      unbind(*dev);
      announce(EVENT_DEVICE_REMOVED, *dev);
    }
    dev->m_declared = true;
    dev->m_driver = declared[k].m_driver;
    dev->m_params = declared[k].m_params;
    dev->m_state = DEVICE_STATE_ABSENT;
  }

  orderAsDeclared(declared);
  holdPins();
}

/**
 * Puts declared devices in file order ahead of found ones, so the earlier line
 * keeps a pin two lines want.
 */
void DeviceManager::orderAsDeclared(const pdiutil::vector<driver_device_t> &declared)
{
  pdiutil::vector<driver_device_t> ordered;
  pdiutil::vector<bool> placed(m_devices.size(), false);

  for( uint16_t k = 0; k < declared.size(); k++ ){
    for( uint16_t i = 0; i < m_devices.size(); i++ ){
      if( !placed[i] && m_devices[i].m_declared &&
          declared[k].m_bus == m_devices[i].m_bus && declared[k].m_target == m_devices[i].m_target ){
        ordered.push_back(m_devices[i]);
        placed[i] = true;
        break;
      }
    }
  }

  for( uint16_t i = 0; i < m_devices.size(); i++ ){
    if( !placed[i] ){
      ordered.push_back(m_devices[i]);
    }
  }

  m_devices = ordered;
}

/**
 * Says which declaration was passed over.
 */
void DeviceManager::logRefusedLine(const pdiutil::string &line)
{
  const char *p = line.c_str();
  while( isFieldBlank(*p) ) p++;
  if( '\0' == *p || DEVICES_CONF_COMMENT_CHAR == *p ){
    return;
  }
  pdiutil::string path = CHARPTR_WRAP(DEVICES_CONF_PATH);
  SysLogW("DEV: %s line not understood: %s\n", path.c_str(), p);
}
#endif

#endif
