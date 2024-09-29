/**************************** Device IOT Controller ***************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st June 2019
******************************************************************************/

#ifndef _EW_SERVER_DEVICE_IOT_CONTROLLER_
#define _EW_SERVER_DEVICE_IOT_CONTROLLER_

#include "Controller.h"
#include <webserver/pages/WiFiConfigPage.h>
#include <webserver/pages/DeviceIotPage.h>
#include <service_provider/DeviceIotServiceProvider.h>

/**
 * DeviceIotController class
 */
class DeviceIotController : public Controller
{

public:
	/**
	 * DeviceIotController constructor
	 */
	DeviceIotController() : Controller("deviceiot")
	{
	}

	/**
	 * DeviceIotController destructor
	 */
	~DeviceIotController()
	{
	}

	/**
	 * boot device register controller
	 *
	 */
	void boot(void)
	{
		if (nullptr != this->m_route_handler)
		{
			this->m_route_handler->register_route(
				EW_SERVER_DEVICE_REGISTER_CONFIG_ROUTE, [&]()
				{ this->handleDeviceRegisterConfigRoute(); },
				AUTH_MIDDLEWARE);
		}
	}

	/**
	 * build device register config html.
	 *
	 * @param	char*	_page
	 * @param	int|EW_HTML_MAX_SIZE	_max_size
	 */
	void build_device_register_config_html(char *_page, int _max_size = EW_HTML_MAX_SIZE)
	{
		if (nullptr == this->m_web_resource || nullptr == this->m_web_resource->m_db_conn)
		{
			return;
		}

		memset(_page, 0, _max_size);
		strcat_P(_page, EW_SERVER_HEADER_HTML);
		strcat_P(_page, EW_SERVER_DEVICE_REGISTER_CONFIG_PAGE_TOP);

		device_iot_config_table _device_iot_configs;
		this->m_web_resource->m_db_conn->get_device_iot_config_table(&_device_iot_configs);

		concat_tr_input_html_tags(_page, PSTR("Registry Host:"), PSTR("dhst"), _device_iot_configs.device_iot_host, DEVICE_IOT_HOST_BUF_SIZE - 1);
		strcat_P(_page, EW_SERVER_FOOTER_WITH_OTP_MONITOR_HTML);
	}

	/**
	 * build and send device register config page.
	 * when posted, get device register configs from client and set them in database.
	 */
	void handleDeviceRegisterConfigRoute(void)
	{
		LogI("Handling device register Config route\n");

		if (nullptr == this->m_web_resource || nullptr == this->m_web_resource->m_server || nullptr == this->m_web_resource->m_db_conn)
		{
			return;
		}

		if (this->m_web_resource->m_server->hasArg("dhst"))
		{
			std::string _device_iot_host = this->m_web_resource->m_server->arg("dhst");

			LogI("\nSubmitted info :\n");
			LogFmtI("device reg. host : %s\n\n", _device_iot_host.c_str());

			device_iot_config_table _device_iot_configs;
			this->m_web_resource->m_db_conn->get_device_iot_config_table(&_device_iot_configs);
			strncpy(_device_iot_configs.device_iot_host, _device_iot_host.c_str(), _device_iot_host.size()); 
			this->m_web_resource->m_db_conn->set_device_iot_config_table(&_device_iot_configs);

			std::string _response = "";
			__device_iot_service.handleRegistrationOtpRequest(&_device_iot_configs, _response);

			this->m_web_resource->m_server->sendHeader("Cache-Control", "no-cache");
			this->m_web_resource->m_server->send(HTTP_OK, EW_HTML_CONTENT, _response.c_str());
		}
		else
		{
			char *_page = new char[EW_HTML_MAX_SIZE];
			this->build_device_register_config_html(_page);

			this->m_web_resource->m_server->send(HTTP_OK, EW_HTML_CONTENT, _page);
			delete[] _page;
		}
	}
};

#endif
