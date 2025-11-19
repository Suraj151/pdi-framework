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
	void processSerial(serial_event_t *se);

	bool isAllowedSerialPort(uint8_t _port, pdiutil::vector<pdiutil::string> *allowedlist = nullptr);
	void appendSerialJsonPayload(pdiutil::string &_payload, pdiutil::vector<pdiutil::string> *allowedlist = nullptr);
	void applySerialJsonPayload(char *_payload, uint16_t _payload_length, pdiutil::vector<pdiutil::string> *allowedlist = nullptr);
};

extern SerialServiceProvider __serial_service;

#endif
