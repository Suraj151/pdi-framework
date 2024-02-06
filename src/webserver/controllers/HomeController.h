/******************************* Home Controller ******************************
This file is part of the Ewings Esp Stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st June 2019
******************************************************************************/

#ifndef _EW_SERVER_HOME_CONTROLLER_
#define _EW_SERVER_HOME_CONTROLLER_

#include "Controller.h"
#include <webserver/pages/HomePage.h>

/**
 * HomeController class
 */
class HomeController : public Controller
{

public:
	/**
	 * HomeController constructor
	 */
	HomeController() : Controller("home")
	{
	}

	/**
	 * HomeController destructor
	 */
	~HomeController()
	{
	}

	/**
	 * register home controller
	 *
	 */
	void boot(void)
	{
		if (nullptr != this->m_route_handler)
		{
			this->m_route_handler->register_route(EW_SERVER_HOME_ROUTE, [&]()
												  { this->handleHomeRoute(); });
			this->m_route_handler->register_not_found_fn([&]()
														 { this->handleNotFound(); });
		}
	}

	/**
	 * build html page with header, middle and footer part.
	 *
	 * @param	char*	_page
	 * @param	PGM_P	_pgm_page
	 * @param	bool|false	_enable_flash
	 * @param	char*|""	_message
	 * @param	FLASH_MSG_TYPE|ALERT_SUCCESS	_alert_type
	 * @param	bool|true	_enable_header_footer
	 * @param	int|EW_HTML_MAX_SIZE	_max_size
	 */
	void build_html(
		char *_page,
		PGM_P _pgm_page,
		bool _enable_flash = false,
		char *_message = "",
		FLASH_MSG_TYPE _alert_type = ALERT_SUCCESS,
		bool _enable_header_footer = true,
		int _max_size = EW_HTML_MAX_SIZE)
	{
		memset(_page, 0, _max_size);

		if (_enable_header_footer)
			strcat_P(_page, EW_SERVER_HEADER_HTML);
		strcat_P(_page, _pgm_page);
		if (_enable_flash)
			concat_flash_message_div(_page, _message, _alert_type);
		if (_enable_header_footer)
			strcat_P(_page, EW_SERVER_FOOTER_HTML);
	}

	/**
	 * build and send not found page to client
	 */
	void handleNotFound(void)
	{
	  LogW("Handling 404 route\n");

	  if (nullptr == this->m_web_resource ||
		  nullptr == this->m_web_resource->m_server)
	  {
		  return;
	  }

	  char *_page = new char[EW_HTML_MAX_SIZE];
	  this->build_html(_page, EW_SERVER_404_PAGE);

	  this->m_web_resource->m_server->send(HTTP_NOT_FOUND, EW_HTML_CONTENT, _page);
	  delete[] _page;
	}

	/**
	 * build and send home page to client
	 */
	void handleHomeRoute(void)
	{
		LogI("Handling Home route\n");

		if (nullptr == this->m_web_resource ||
			nullptr == this->m_web_resource->m_server ||
			nullptr == this->m_route_handler)
		{
			return;
		}

		char *_page = new char[EW_HTML_MAX_SIZE];

		memset(_page, 0, EW_HTML_MAX_SIZE);
		strcat_P(_page, EW_SERVER_HEADER_HTML);

		if (this->m_route_handler->has_active_session())
		{

			strcat_P(_page, EW_SERVER_MENU_CARD_PAGE_WRAP_TOP);

			concat_svg_menu_card(_page, EW_SERVER_HOME_MENU_TITLE_LOGIN, SVG_ICON48_PATH_ACCOUNT_CIRCLE, EW_SERVER_LOGIN_CONFIG_ROUTE);
			concat_svg_menu_card(_page, EW_SERVER_HOME_MENU_TITLE_WIFI, SVG_ICON48_PATH_WIFI, EW_SERVER_WIFI_CONFIG_ROUTE);
			concat_svg_menu_card(_page, EW_SERVER_HOME_MENU_TITLE_OTA, SVG_ICON48_PATH_CLOUD_DOWNLOAD, EW_SERVER_OTA_CONFIG_ROUTE);
#ifdef ENABLE_MQTT_SERVICE
			concat_svg_menu_card(_page, EW_SERVER_HOME_MENU_TITLE_MQTT, SVG_ICON48_PATH_SEND, EW_SERVER_MQTT_MANAGE_CONFIG_ROUTE);
#endif
#ifdef ENABLE_GPIO_SERVICE
			concat_svg_menu_card(_page, EW_SERVER_HOME_MENU_TITLE_GPIO, SVG_ICON48_PATH_MEMORY, EW_SERVER_GPIO_MANAGE_CONFIG_ROUTE);
#endif
#ifdef ENABLE_EMAIL_SERVICE
			concat_svg_menu_card(_page, EW_SERVER_HOME_MENU_TITLE_EMAIL, SVG_ICON48_PATH_MAIL, EW_SERVER_EMAIL_CONFIG_ROUTE);
#endif
#ifdef ENABLE_DEVICE_IOT
			concat_svg_menu_card(_page, EW_SERVER_HOME_MENU_TITLE_DEVICE_REGISTER, SVG_ICON48_PATH_BEENHERE, EW_SERVER_DEVICE_REGISTER_CONFIG_ROUTE);
#endif
			concat_svg_menu_card(_page, EW_SERVER_HOME_MENU_TITLE_DASHBOARD, SVG_ICON48_PATH_DASHBOARD, EW_SERVER_DASHBOARD_ROUTE);
			concat_svg_menu_card(_page, EW_SERVER_HOME_MENU_TITLE_LOGOUT, SVG_ICON48_PATH_POWER, EW_SERVER_LOGOUT_ROUTE);

			strcat_P(_page, EW_SERVER_MENU_CARD_PAGE_WRAP_BOTTOM);
		}
		else
		{

			strcat_P(_page, EW_SERVER_HOME_PAGE);
		}

		strcat_P(_page, EW_SERVER_FOOTER_HTML);

		this->m_web_resource->m_server->send(HTTP_OK, EW_HTML_CONTENT, _page);
		delete[] _page;
	}
};

#endif
