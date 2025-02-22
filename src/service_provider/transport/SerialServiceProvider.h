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

	bool initService() override;
	void processSerial(serial_event_t *se);
};

extern SerialServiceProvider __serial_service;

#endif
