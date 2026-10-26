/****************************** Device Manager ********************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

The devices the serial service knows of and the drivers bound to them. Each
scan looks on every serial interface the port made, takes in what is declared
in /etc/devices.conf, binds a driver to what turned up and lets go of what has
gone, announcing each on the event bus.

Author          : Suraj I.
created Date    : 19th Oct 2026
******************************************************************************/

#ifndef _DEVICE_MANAGER_H_
#define _DEVICE_MANAGER_H_

#include <config/Config.h>

#ifdef ENABLE_DRIVER_CORE

#include <config/DriverConfig.h>
#include <interface/pdi/modules/driver/iDriverInterface.h>

/**
 * DeviceManager class
 */
class DeviceManager
{
public:

  /**
   * DeviceManager constructor.
   */
  DeviceManager();

  /**
   * Lets go of every bound device, giving a busy driver up to DEVICE_BUSY_WAIT_MS
   * to finish first.
   */
  ~DeviceManager();

  /**
   * Adds a driver to the ones every manager matches devices against.
   */
  static void registerDriver(const driver_prop_t &prop);

  /**
   * Every driver compiled in, created on first use so a driver registering
   * itself during static start up always finds it.
   */
  static pdiutil::vector<driver_prop_t> &drivers();

  /**
   * One pass: reread changed declarations, scan every interface, bind what appeared
   * and release what has gone.
   */
  void scan();

  /**
   * Rereads the declarations, gives each unbound device another chance, restores
   * every left out device, then scans.
   */
  void trigger();

  /**
   * How many devices are known.
   */
  uint16_t count() const;

  /**
   * One known device, nullptr past the end.
   */
  const driver_device_t *at(uint16_t index) const;

  /**
   * The device at a target on an interface, nullptr when none is known.
   */
  const driver_device_t *find(serial_iface_t bus, uint32_t target) const;

  /**
   * The first driver bound that presents a device of the class, nullptr when none
   * is. Valid only until the next scan.
   */
  iDriverInterface *bound(uint8_t cls) const;

  /**
   * The name a device goes by under /sys/bus, the way linux names it.
   */
  static bool deviceId(const driver_device_t &dev, pdiutil::string &out);

  /**
   * The name an interface goes by in /etc/devices.conf and under /sys/bus.
   */
  static bool busName(serial_iface_t bus, pdiutil::string &out);

  /**
   * Reads one /etc/devices.conf line into a declared device, false for a
   * comment, a blank line or one that does not parse.
   */
  static bool parseLine(const char *line, uint16_t len, driver_device_t &out);

  /**
   * A whole number setting the declaration gave the device, false when it gave
   * none by that name.
   */
  static bool param(const driver_device_t &dev, const char *key, uint32_t &value);

  /**
   * Whether a line in /etc/devices.conf holds the hardware pin for its bus.
   */
  bool holdsPin(uint8_t hwpin) const;

  /**
   * What an interface's bus line came to: PDI_OK, the refusing code, or
   * PDI_ERR_NOT_FOUND when there is none.
   */
  pdi_err_t busResult(serial_iface_t bus) const;

  /**
   * The hardware pins a line uses on its bus: the ones it names, or the bus's
   * own; how many.
   */
  static uint8_t linePins(const driver_device_t &line, iSerialInterface *iface, uint8_t *out);

private:

  /**
   * Puts each bus on its line's pins, or its own, and works out which pins the
   * lines hold, refusing a line whose pins another holds.
   */
  void holdPins();

  /**
   * Puts the interface on its bus line's settings: PDI_OK, or the code that
   * names why not.
   */
  pdi_err_t applyBusLine(const driver_device_t &line, iSerialInterface *iface, uint64_t others);

  /**
   * Puts a bus whose pins the framework hands out on the pins and clock given;
   * PDI_ERR_NOT_SUPPORTED for any other.
   */
  static pdi_err_t configureBus(iSerialInterface *iface, serial_iface_t family, const int16_t *pins, uint32_t clock);

  /**
   * Whether a scan may look on the interface: one with framework pins only while a
   * line holds them.
   */
  bool lookable(serial_iface_t bus, iSerialInterface *iface) const;

  /**
   * Whether any bound driver is in the middle of an operation on its device.
   */
  bool anyBusy() const;

  /**
   * The device at a target on an interface, made known when it was not.
   */
  driver_device_t *note(serial_iface_t bus, uint32_t target);

  /**
   * Finds what is on one interface now: addresses a driver answers to, devices the
   * port reports and devices declared on it.
   */
  void look(serial_iface_t bus, iSerialInterface *iface);

  /**
   * Takes a device whose probe held the scan too long out of later scans, until
   * udevadm trigger probes it again.
   */
  void leaveOutOfScan(driver_device_t &dev, uint32_t took);

  /**
   * Binds what turned up since the last pass and lets go of what has gone.
   */
  void reconcile();

  /**
   * Whether the driver a declared device names registered to be rescanned.
   */
  static bool rescanned(const driver_device_t &dev);

  /**
   * Offers each missing or refused declared device to its rescanned driver again,
   * so it binds with no trigger.
   */
  void retryMissing();

  /**
   * Offers the device to each driver that could drive it until one confirms it,
   * leaving it unbound when none does.
   */
  void bind(driver_device_t &dev, bool quiet = false);

  /**
   * Lets the bound driver go of the device.
   */
  void unbind(driver_device_t &dev);

  /**
   * Tells the event bus and the system log a device came or went.
   */
  void announce(event_name_t event, driver_device_t &dev);

  /**
   * Logs a newly bound disk's partition table and each partition in it, as linux
   * does.
   */
  void logLayout(driver_device_t &dev);

#ifdef ENABLE_STORAGE_SERVICE
  /**
   * Whether the declarations changed since last read, judged by the file's size and
   * modification time.
   */
  bool confChanged();

  /**
   * Takes the declarations in, adding what is new and forgetting what is no longer
   * declared.
   */
  void loadConf();

  /**
   * Puts declared devices in file order ahead of found ones, so the earlier line
   * keeps a pin two lines want.
   */
  void orderAsDeclared(const pdiutil::vector<driver_device_t> &declared);

  /**
   * Says which declaration was passed over.
   */
  void logRefusedLine(const pdiutil::string &line);
#endif

  pdiutil::vector<driver_device_t> m_devices;
  pdiutil::vector<driver_device_t> m_bus_lines;
  uint64_t m_held_pins;
  uint32_t m_held_buses;
  uint64_t m_conf_size;
  uint32_t m_conf_mtime;
  bool m_conf_known;
  uint32_t m_probe_ms;
#if defined(ENABLE_DISK_FS) && defined(ENABLE_DEVFS)
  int16_t m_fstab_listener;
#endif
};

#endif

#endif
