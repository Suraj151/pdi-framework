/****************************** Reset Factory ********************************
This file is part of the Ewings Esp Stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st June 2019
******************************************************************************/

#ifndef _DEVICE_FACTORY_RESET_SERVICE_H_
#define _DEVICE_FACTORY_RESET_SERVICE_H_

#include <service_provider/ServiceProvider.h>

/**
 * DeviceFactoryReset class
 */
class DeviceFactoryReset : public ServiceProvider
{

public:
	/**
	 * DeviceFactoryReset constructor
	 */
	DeviceFactoryReset();
	/**
	 * DeviceFactoryReset destructor
	 */
	~DeviceFactoryReset();

	bool initService() override;
	void factory_reset(void);
	void check_device_factory_request(void);

protected:
	/**
	 * @var	uint8_t|0 m_flash_key_pressed
	 */
	uint8_t m_flash_key_pressed;
};

extern DeviceFactoryReset __factory_reset;

#endif
