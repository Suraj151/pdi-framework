/******************************* OTA Controller *******************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st June 2019
******************************************************************************/

#ifndef _WEB_SERVER_OTA_CONTROLLER_
#define _WEB_SERVER_OTA_CONTROLLER_

#include "Controller.h"
#include <webserver/pages/WiFiConfigPage.h>
#include <webserver/pages/OtaConfigPage.h>

/**
 * OtaController class
 */
class OtaController : public Controller
{

public:
	/**
	 * OtaController constructor
	 */
	OtaController() : Controller("ota")
	{
	}

	/**
	 * OtaController destructor
	 */
	~OtaController()
	{
	}

	/**
	 * register ota controller
	 *
	 */
	void boot(void)
	{
		if (nullptr != this->m_route_handler)
		{
			this->m_route_handler->register_route(
				WEB_SERVER_OTA_CONFIG_ROUTE, [&]()
				{ this->handleOtaServerConfigRoute(); },
				AUTH_MIDDLEWARE);
		}
	}

	/**
	 * build ota server config html.
	 *
	 * @param	char*	_page
	 * @param	bool|false	_enable_flash
	 * @param	int|PAGE_HTML_MAX_SIZE	_max_size
	 */
	void build_ota_server_config_html(char *_page, bool _enable_flash = false, int _max_size = PAGE_HTML_MAX_SIZE)
	{
		if (nullptr == this->m_web_resource ||
			nullptr == this->m_web_resource->m_db_conn)
		{
			return;
		}

		memset(_page, 0, _max_size);
		strcat_P(_page, WEB_SERVER_HEADER_HTML);
		strcat_P(_page, WEB_SERVER_OTA_CONFIG_PAGE_TOP);

		ota_config_table _ota_configs;
		this->m_web_resource->m_db_conn->get_ota_config_table(&_ota_configs);

		char _port[10];
		memset(_port, 0, 10);
		__appendUintToBuff(_port, "%d", _ota_configs.ota_port, 8);

#ifdef ALLOW_OTA_CONFIG_MODIFICATION

		concat_tr_input_html_tags(_page, RODT_ATTR("OTA Host:"), RODT_ATTR("hst"), _ota_configs.ota_host, OTA_HOST_BUF_SIZE - 1);
		concat_tr_input_html_tags(_page, RODT_ATTR("OTA Port:"), RODT_ATTR("prt"), _port);

		strcat_P(_page, WEB_SERVER_WIFI_CONFIG_PAGE_BOTTOM);
#else

		concat_tr_input_html_tags(_page, RODT_ATTR("OTA Host:"), RODT_ATTR("hst"), _ota_configs.ota_host, OTA_HOST_BUF_SIZE - 1, HTML_INPUT_TEXT_TAG_TYPE, false, true);
		concat_tr_input_html_tags(_page, RODT_ATTR("OTA Port:"), RODT_ATTR("prt"), _port, HTML_INPUT_TAG_DEFAULT_MAXLENGTH, HTML_INPUT_TEXT_TAG_TYPE, false, true);
#endif

		if (_enable_flash)
			concat_flash_message_div(_page, HTML_SUCCESS_FLASH, ALERT_SUCCESS);
		strcat_P(_page, WEB_SERVER_FOOTER_HTML);
	}

	/**
	 * build and send ota server config page.
	 * when posted, get ota server configs from client and set them in database.
	 */
	void handleOtaServerConfigRoute(void)
	{
		LogI("Handling OTA Server Config route\n");

		if (nullptr == this->m_web_resource ||
			nullptr == this->m_web_resource->m_db_conn ||
			nullptr == this->m_web_resource->m_server)
		{
			return;
		}

		bool _is_posted = false;

#ifdef ALLOW_OTA_CONFIG_MODIFICATION
		if (this->m_web_resource->m_server->hasArg("hst") && this->m_web_resource->m_server->hasArg("prt"))
		{
			pdiutil::string _ota_host = this->m_web_resource->m_server->arg("hst");
			pdiutil::string _ota_port = this->m_web_resource->m_server->arg("prt");

			LogI("\nSubmitted info :\n");
			LogFmtI("ota host : %s\n", _ota_host.c_str());
			LogFmtI("ota port : %s\n\n", _ota_port.c_str());

			ota_config_table _ota_configs;
			// this->m_web_resource->m_db_conn->get_ota_config_table(&_ota_configs);

			strncpy(_ota_configs.ota_host, _ota_host.c_str(), _ota_host.size());
			_ota_configs.ota_port = StringToUint16(_ota_port.c_str());

			this->m_web_resource->m_db_conn->set_ota_config_table(&_ota_configs);

			_is_posted = true;
		}
#endif

		char *_page = new char[PAGE_HTML_MAX_SIZE];
		this->build_ota_server_config_html(_page, _is_posted);

		this->m_web_resource->m_server->send(HTTP_OK, TEXT_HTML_CONTENT, _page);
		delete[] _page;
	}
};

#endif
