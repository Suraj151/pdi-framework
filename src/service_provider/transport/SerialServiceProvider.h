/****************************** Serial Services *******************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st June 2019
******************************************************************************/

#ifndef _SERIAL_SERVICE_H_
#define _SERIAL_SERVICE_H_

#include <service_provider/ServiceProvider.h>

#ifdef ENABLE_DRIVER_CORE
#include "DeviceManager.h"
#endif

/**
 * SerialServiceProvider class
 */
class SerialServiceProvider : public ServiceProvider
{

public:
	/**
	 * SerialServiceProvider constructor
	 */
	SerialServiceProvider();

	/**
	 * SerialServiceProvider destructor
	 */
	~SerialServiceProvider();

	bool initService(void *arg = nullptr) override;
	bool isEssentialService() const override { return true; }
	terminal_types_t getServiceTerminalType() const override { return TERMINAL_TYPE_SERIAL; }
	void processSerial(serial_event_t *se);

	void appendSerialJsonPayload(pdiutil::string &_payload, pdiutil::vector<pdiutil::string> *allowedlist = nullptr);
	void applySerialJsonPayload(char *_payload, uint16_t _payload_length, pdiutil::vector<pdiutil::string> *allowedlist = nullptr);

#ifdef ENABLE_DRIVER_CORE
	/**
	 * The devices found on the serial interfaces and the drivers bound to
	 * them, nullptr until the service has started.
	 */
	DeviceManager *devices() { return m_devices; }

	/**
	 * Lets go of every device on a stop, so the next start finds them afresh
	 * and scans again.
	 */
	void resetServiceState() override;
#endif

private:

	bool isAllowedSerialPort(const char* serial, uint8_t _port, pdiutil::vector<pdiutil::string> *allowedlist = nullptr);

#ifdef ENABLE_DRIVER_CORE
	DeviceManager *m_devices;
#endif
};

extern SerialServiceProvider __serial_service;

#endif
