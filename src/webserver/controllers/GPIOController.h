/******************************* GPIO Controller ******************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st June 2019
******************************************************************************/

#ifndef _WEB_SERVER_GPIO_CONTROLLER_
#define _WEB_SERVER_GPIO_CONTROLLER_

#include "Controller.h"
#include <webserver/pages/GpioConfigPage.h>
#include <service_provider/device/GpioServiceProvider.h>

/**
 * GpioController class
 */
class GpioController : public Controller
{

protected:
	/**
	 * @var	gpio_config_table	gpio_configs
	 */
	// gpio_config_table gpio_configs;

	/**
	 * @var	last_gpio_monitor_point	_last_monitor_point
	 */
	last_gpio_monitor_point _last_monitor_point;

public:
	/**
	 * GpioController constructor
	 */
	GpioController() : Controller("gpio")
	{
	}

	/**
	 * GpioController destructor
	 */
	~GpioController()
	{
	}

	/**
	 * register gpio controller
	 *
	 */
	void boot(void)
	{
		if (nullptr != this->m_route_handler)
		{
			this->m_route_handler->register_route(
				WEB_SERVER_GPIO_MANAGE_CONFIG_ROUTE, [&]()
				{ this->handleGpioManageRoute(); },
				AUTH_MIDDLEWARE);
			this->m_route_handler->register_route(
				WEB_SERVER_GPIO_SERVER_CONFIG_ROUTE, [&]()
				{ this->handleGpioServerConfigRoute(); },
				AUTH_MIDDLEWARE);
			this->m_route_handler->register_route(
				WEB_SERVER_GPIO_MODE_CONFIG_ROUTE, [&]()
				{ this->handleGpioModeConfigRoute(); },
				AUTH_MIDDLEWARE);
			this->m_route_handler->register_route(
				WEB_SERVER_GPIO_WRITE_CONFIG_ROUTE, [&]()
				{ this->handleGpioWriteConfigRoute(); },
				AUTH_MIDDLEWARE);
			this->m_route_handler->register_route(
				WEB_SERVER_GPIO_ALERT_CONFIG_ROUTE, [&]()
				{ this->handleGpioAlertConfigRoute(); },
				AUTH_MIDDLEWARE);
			this->m_route_handler->register_route(
				WEB_SERVER_GPIO_MONITOR_ROUTE, [&]()
				{ this->handleGpioMonitorRoute(); },
				AUTH_MIDDLEWARE);
			this->m_route_handler->register_route(
				WEB_SERVER_GPIO_ANALOG_MONITOR_ROUTE, [&]()
				{ this->handleAnalogMonitor(); });
		}
		// this->m_web_resource->m_db_conn->get_gpio_config_table(&this->gpio_configs);
	}

	/**
	 * handle adc pin monitor calls.
	 * it provides live analog readings mapped to graph height.
	 */
	void handleAnalogMonitor(void)
	{
		LogI("Handling analog monitor route\n");

		if (nullptr == this->m_web_resource ||
			nullptr == this->m_web_resource->m_server ||
			nullptr == this->m_route_handler)
		{
			return;
		}

		int y1 = this->_last_monitor_point.y,
			y2 = map(
				__gpio_service.m_gpio_config_copy.gpio_readings[MAX_DIGITAL_GPIO_PINS], 0, ANALOG_GPIO_RESOLUTION,
				GPIO_GRAPH_TOP_MARGIN, GPIO_MAX_GRAPH_HEIGHT - GPIO_GRAPH_BOTTOM_MARGIN),
			x1 = this->_last_monitor_point.x < GPIO_MAX_GRAPH_WIDTH ? this->_last_monitor_point.x : GPIO_MAX_GRAPH_WIDTH - GPIO_GRAPH_ADJ_POINT_DISTANCE,
			x2 = x1 + GPIO_GRAPH_ADJ_POINT_DISTANCE;

		y2 = GPIO_MAX_GRAPH_HEIGHT - y2;

		pdiutil::string *_response = new pdiutil::string();

		if (nullptr != _response)
		{
			*_response = "{\"x1\":";
			*_response += pdiutil::to_string(x1);
			*_response += ",\"y1\":";
			*_response += pdiutil::to_string(y1);
			*_response += ",\"x2\":";
			*_response += pdiutil::to_string(x2);
			*_response += ",\"y2\":";
			*_response += pdiutil::to_string(y2);
			*_response += ",\"r\":";
			*_response += pdiutil::to_string(!this->m_route_handler->has_active_session());
			*_response += ",\"d\":";
			__gpio_service.appendGpioJsonPayload(*_response);
			*_response += ",\"md\":[\"OFF\", \"DOUT\", \"DIN\", \"BLINK\", \"AOUT\", \"AIN\"]";
			*_response += "}";

			this->_last_monitor_point.x = x2;
			this->_last_monitor_point.y = y2;
			this->m_web_resource->m_server->sendHeader("Cache-Control", "no-cache");
			this->m_web_resource->m_server->send(HTTP_OK, TEXT_HTML_CONTENT, _response->c_str());

			delete _response;
		}
	}

	/**
	 * build and send gpio manage page to client
	 */
	void handleGpioManageRoute(void)
	{
		LogI("Handling Gpio Manage route\n");

		if (nullptr == this->m_web_resource ||
			nullptr == this->m_web_resource->m_server)
		{
			return;
		}

		char *_page = new char[PAGE_HTML_MAX_SIZE];
		memset(_page, 0, PAGE_HTML_MAX_SIZE);

		strcat_ro(_page, WEB_SERVER_HEADER_HTML);
		strcat_ro(_page, WEB_SERVER_MENU_CARD_PAGE_WRAP_TOP);

		concat_svg_menu_card(_page, WEB_SERVER_GPIO_MENU_TITLE_MODES, SVG_ICON48_PATH_TUNE, WEB_SERVER_GPIO_MODE_CONFIG_ROUTE);
		concat_svg_menu_card(_page, WEB_SERVER_GPIO_MENU_TITLE_CONTROL, SVG_ICON48_PATH_GAME_ASSET, WEB_SERVER_GPIO_WRITE_CONFIG_ROUTE);
		concat_svg_menu_card(_page, WEB_SERVER_GPIO_MENU_TITLE_SERVER, SVG_ICON48_PATH_COMPUTER, WEB_SERVER_GPIO_SERVER_CONFIG_ROUTE);
		concat_svg_menu_card(_page, WEB_SERVER_GPIO_MENU_TITLE_MONITOR, SVG_ICON48_PATH_EYE, WEB_SERVER_GPIO_MONITOR_ROUTE);
		concat_svg_menu_card(_page, WEB_SERVER_GPIO_MENU_TITLE_ALERT, SVG_ICON48_PATH_NOTIFICATION, WEB_SERVER_GPIO_ALERT_CONFIG_ROUTE);

		strcat_ro(_page, WEB_SERVER_MENU_CARD_PAGE_WRAP_BOTTOM);
		strcat_ro(_page, WEB_SERVER_FOOTER_HTML);

		this->m_web_resource->m_server->send(HTTP_OK, TEXT_HTML_CONTENT, _page);
		delete[] _page;
	}

	/**
	 * build gpio monitor html page
	 *
	 * @param	char*	_page
	 * @param	int|PAGE_HTML_MAX_SIZE	_max_size
	 */
	void build_gpio_monitor_html(char *_page, int _max_size = PAGE_HTML_MAX_SIZE)
	{
		memset(_page, 0, _max_size);
		strcat_ro(_page, WEB_SERVER_HEADER_HTML);
		strcat_ro(_page, WEB_SERVER_GPIO_MONITOR_PAGE_TOP);

		char *_gpio_monitor_table_heading[] = {"Pin", "Mode", "value"};
		strcat_ro(_page, HTML_TABLE_OPEN_TAG);
		concat_style_attribute(_page, RODT_ATTR("width:92%"));
		strcat_ro(_page, HTML_TAG_CLOSE_BRACKET);
		concat_table_heading_row(_page, _gpio_monitor_table_heading, 3, nullptr, nullptr, RODT_ATTR("btn"), nullptr);

		char _name[5];
		char *_gpio_monitor_table_data[] = {_name, "", ""};

		for (uint8_t _pin = 0; _pin < MAX_DIGITAL_GPIO_PINS; _pin++)
		{
			if (!__i_dvc_ctrl.isExceptionalGpio(_pin))
			{
				memset(_name, 0, 5);
				__appendUintToBuff(_name, "D%d", _pin, 4);

				concat_table_data_row(_page, _gpio_monitor_table_data, 3, nullptr, nullptr, RODT_ATTR("btnd"), nullptr);
			}
		}

		for (uint8_t _pin = 0; _pin < MAX_ANALOG_GPIO_PINS; _pin++)
		{
			if (!__i_dvc_ctrl.isExceptionalGpio(_pin))
			{
				memset(_name, 0, 5);
				__appendUintToBuff(_name, "A%d", _pin, 4);

				concat_table_data_row(_page, _gpio_monitor_table_data, 3, nullptr, nullptr, RODT_ATTR("btnd"), nullptr);
			}
		}
		strcat_ro(_page, HTML_TABLE_CLOSE_TAG);

		strcat_ro(_page, HTML_DIV_OPEN_TAG);
		concat_style_attribute(_page, RODT_ATTR("display:inline-flex;margin-top:25px;"));
		strcat_ro(_page, HTML_TAG_CLOSE_BRACKET);
		concat_graph_axis_title_div(_page, (char *)"A0 ( 0 - 1024 )", (char *)"writing-mode:vertical-lr");
		strcat_ro(_page, WEB_SERVER_GPIO_MONITOR_SVG_ELEMENT);
		strcat_ro(_page, HTML_DIV_CLOSE_TAG);
		concat_graph_axis_title_div(_page, (char *)"Time");
		strcat_ro(_page, WEB_SERVER_FOOTER_WITH_ANALOG_MONITOR_HTML);
	}

	/**
	 * build and send gpio monitor config page to client
	 */
	void handleGpioMonitorRoute(void)
	{
		LogI("Handling Gpio Monitor Config route\n");

		if (nullptr == this->m_web_resource ||
			nullptr == this->m_web_resource->m_server)
		{
			return;
		}

		char *_page = new char[PAGE_HTML_MAX_SIZE];
		this->build_gpio_monitor_html(_page);

		this->_last_monitor_point.x = 0;
		this->_last_monitor_point.y = GPIO_MAX_GRAPH_HEIGHT - GPIO_GRAPH_BOTTOM_MARGIN;
		this->m_web_resource->m_server->send(HTTP_OK, TEXT_HTML_CONTENT, _page);
		delete[] _page;
	}

	/**
	 * build gpio server config html.
	 *
	 * @param	char*		_page
	 * @param	bool|false	_enable_flash
	 * @param	int|PAGE_HTML_MAX_SIZE	_max_size
	 */
	void build_gpio_server_config_html(char *_page, bool _enable_flash = false, int _max_size = PAGE_HTML_MAX_SIZE)
	{
		memset(_page, 0, _max_size);
		strcat_ro(_page, WEB_SERVER_HEADER_HTML);
		strcat_ro(_page, WEB_SERVER_GPIO_SERVER_PAGE_TOP);

		char _port[10], _freq[10];
		memset(_port, 0, 10);
		memset(_freq, 0, 10);
		__appendUintToBuff(_port, "%d", __gpio_service.m_gpio_config_copy.gpio_port, 8);
		__appendUintToBuff(_freq, "%d", __gpio_service.m_gpio_config_copy.gpio_post_frequency, 8);

		concat_tr_input_html_tags(_page, RODT_ATTR("Host Address:"), RODT_ATTR("hst"), __gpio_service.m_gpio_config_copy.gpio_host, GPIO_HOST_BUF_SIZE - 1);
		concat_tr_input_html_tags(_page, RODT_ATTR("Host Port:"), RODT_ATTR("prt"), _port);
		concat_tr_input_html_tags(_page, RODT_ATTR("Post Frequency:"), RODT_ATTR("frq"), _freq);

		strcat_ro(_page, WEB_SERVER_WIFI_CONFIG_PAGE_BOTTOM);
		if (_enable_flash)
			concat_flash_message_div(_page, HTML_SUCCESS_FLASH, ALERT_SUCCESS);
		strcat_ro(_page, WEB_SERVER_FOOTER_HTML);
	}

	/**
	 * build and send gpio server config page.
	 * when posted, get gpio server configs from client and set them in database.
	 */
	void handleGpioServerConfigRoute(void)
	{
		LogI("Handling Gpio Server Config route\n");

		if (nullptr == this->m_web_resource ||
			nullptr == this->m_web_resource->m_db_conn ||
			nullptr == this->m_web_resource->m_server)
		{
			return;
		}

		bool _is_posted = false;

		if (this->m_web_resource->m_server->hasArg("hst") && this->m_web_resource->m_server->hasArg("prt"))
		{
			pdiutil::string _gpio_host = this->m_web_resource->m_server->arg("hst");
			pdiutil::string _gpio_port = this->m_web_resource->m_server->arg("prt");
			pdiutil::string _post_freq = this->m_web_resource->m_server->arg("frq");

			LogI("\nSubmitted info :\n");
			LogFmtI("gpio host : %s\n", _gpio_host.c_str());
			LogFmtI("gpio port : %s\n", _gpio_port.c_str());
			LogFmtI("post freq : %s\n\n", _post_freq.c_str());

			strncpy(__gpio_service.m_gpio_config_copy.gpio_host, _gpio_host.c_str(), _gpio_host.size());
			__gpio_service.m_gpio_config_copy.gpio_port = StringToUint16(_gpio_port.c_str());
			__gpio_service.m_gpio_config_copy.gpio_post_frequency = StringToUint32(_post_freq.c_str()) < 0 ? GPIO_DATA_POST_FREQ : StringToUint32(_post_freq.c_str());
			this->m_web_resource->m_db_conn->set_gpio_config_table(&__gpio_service.m_gpio_config_copy);

			_is_posted = true;
		}

		char *_page = new char[PAGE_HTML_MAX_SIZE];
		this->build_gpio_server_config_html(_page, _is_posted);

		this->m_web_resource->m_server->send(HTTP_OK, TEXT_HTML_CONTENT, _page);
		delete[] _page;
		if (_is_posted)
		{
			__gpio_service.handleGpioModes(GPIO_SERVER_CONFIG);
		}
	}

	/**
	 * build gpio mode config html.
	 *
	 * @param	char*	_page
	 * @param	bool|false	_enable_flash
	 * @param	int|PAGE_HTML_MAX_SIZE	_max_size
	 */
	void build_gpio_mode_config_html(char *_page, bool _enable_flash = false, int _max_size = PAGE_HTML_MAX_SIZE)
	{
		memset(_page, 0, _max_size);
		strcat_ro(_page, WEB_SERVER_HEADER_HTML);
		strcat_ro(_page, WEB_SERVER_GPIO_CONFIG_PAGE_TOP);
		char *_gpio_mode_general_options[] = {"OFF", "DOUT", "DIN", "BLINK", "AOUT"};
		char *_gpio_mode_analog_options[] = {"OFF", "", "", "", "", "AIN"};
		int _gpio_mode_general_options_size = sizeof(_gpio_mode_general_options) / sizeof(_gpio_mode_general_options[0]);
		int _gpio_mode_analog_options_size = sizeof(_gpio_mode_analog_options) / sizeof(_gpio_mode_analog_options[0]);

		char _name[6], _label[6];
		int _exception = -1;

		for (uint8_t _pin = 0; _pin < MAX_DIGITAL_GPIO_PINS; _pin++)
		{
			memset(_name, 0, 6);
			memset(_label, 0, 6);
			__appendUintToBuff(_name, "D%d:", _pin, 5);
			__appendUintToBuff(_label, "d%d", _pin, 5);
			_exception = _pin == 0 ? ANALOG_WRITE : -1;
			if (!__i_dvc_ctrl.isExceptionalGpio(_pin))
			{
				concat_tr_select_html_tags(_page, _name, _label, _gpio_mode_general_options, _gpio_mode_general_options_size, (int)__gpio_service.m_gpio_config_copy.gpio_mode[_pin], _exception);
			}
		}

		for (uint8_t _pin = 0; _pin < MAX_ANALOG_GPIO_PINS; _pin++)
		{
			memset(_name, 0, 6);
			memset(_label, 0, 6);
			__appendUintToBuff(_name, "A%d:", _pin, 5);
			__appendUintToBuff(_label, "a%d", _pin, 5);
			if (!__i_dvc_ctrl.isExceptionalGpio(_pin))
			{
				concat_tr_select_html_tags(_page, _name, _label, _gpio_mode_analog_options, _gpio_mode_analog_options_size, (int)__gpio_service.m_gpio_config_copy.gpio_mode[MAX_DIGITAL_GPIO_PINS + _pin]);
			}
		}

		strcat_ro(_page, WEB_SERVER_WIFI_CONFIG_PAGE_BOTTOM);
		if (_enable_flash)
			concat_flash_message_div(_page, HTML_SUCCESS_FLASH, ALERT_SUCCESS);
		strcat_ro(_page, WEB_SERVER_FOOTER_HTML);
	}

	/**
	 * build and send gpio mode config page.
	 * when posted, get gpio mode configs from client and set them in database.
	 */
	void handleGpioModeConfigRoute(void)
	{
		LogI("Handling Gpio Mode Config route\n");

		if (nullptr == this->m_web_resource ||
			nullptr == this->m_web_resource->m_db_conn ||
			nullptr == this->m_web_resource->m_server)
		{
			return;
		}

		bool _is_posted = false;

		if (this->m_web_resource->m_server->hasArg("d0") && this->m_web_resource->m_server->hasArg("a0"))
		{
			LogI("\nSubmitted info :\n");

			char _label[6];

			for (uint8_t _pin = 0; _pin < MAX_DIGITAL_GPIO_PINS; _pin++)
			{
				memset(_label, 0, 6);
				__appendUintToBuff(_label, "d%d", _pin, 5);

				uint8_t _val = StringToUint8(this->m_web_resource->m_server->arg(_label).c_str());
				__gpio_service.m_gpio_config_copy.gpio_mode[_pin] = !__i_dvc_ctrl.isExceptionalGpio(_pin) ? _val : 0;

				LogFmtI("Pin D%d : %d\n", _pin, _val);

				if (__gpio_service.m_gpio_config_copy.gpio_mode[_pin] == OFF || __gpio_service.m_gpio_config_copy.gpio_mode[_pin] == DIGITAL_WRITE || __gpio_service.m_gpio_config_copy.gpio_mode[_pin] == ANALOG_WRITE)
				{
					__gpio_service.m_gpio_config_copy.gpio_alert_comparator[_pin] = EQUAL;
					__gpio_service.m_gpio_config_copy.gpio_alert_channel[_pin] = NO_ALERT;
					__gpio_service.m_gpio_config_copy.gpio_alert_values[_pin] = OFF;
				}
			}

			for (uint8_t _pin = 0; _pin < MAX_ANALOG_GPIO_PINS; _pin++)
			{
				memset(_label, 0, 6);
				__appendUintToBuff(_label, "a%d", _pin, 5);

				uint16_t _val = StringToUint16(this->m_web_resource->m_server->arg(_label).c_str());
				__gpio_service.m_gpio_config_copy.gpio_mode[MAX_DIGITAL_GPIO_PINS + _pin] = !__i_dvc_ctrl.isExceptionalGpio(_pin) ? _val : 0;

				LogFmtI("Pin A%d : %d\n", _pin, _val);

				if (__gpio_service.m_gpio_config_copy.gpio_mode[MAX_DIGITAL_GPIO_PINS + _pin] == OFF)
				{
					__gpio_service.m_gpio_config_copy.gpio_alert_comparator[MAX_DIGITAL_GPIO_PINS + _pin] = EQUAL;
					__gpio_service.m_gpio_config_copy.gpio_alert_channel[MAX_DIGITAL_GPIO_PINS + _pin] = NO_ALERT;
					__gpio_service.m_gpio_config_copy.gpio_alert_values[MAX_DIGITAL_GPIO_PINS + _pin] = OFF;
				}
			}

			this->m_web_resource->m_db_conn->set_gpio_config_table(&__gpio_service.m_gpio_config_copy);

			_is_posted = true;
		}

		char *_page = new char[PAGE_HTML_MAX_SIZE];
		this->build_gpio_mode_config_html(_page, _is_posted);

		this->m_web_resource->m_server->send(HTTP_OK, TEXT_HTML_CONTENT, _page);
		delete[] _page;
		if (_is_posted)
		{
			__gpio_service.handleGpioModes(GPIO_MODE_CONFIG);
		}
	}

	/**
	 * build gpio write config html.
	 *
	 * @param	char*	_page
	 * @param	bool|false	_enable_flash
	 * @param	int|PAGE_HTML_MAX_SIZE	_max_size
	 */
	void build_gpio_write_config_html(char *_page, bool _enable_flash = false, int _max_size = PAGE_HTML_MAX_SIZE)
	{

		memset(_page, 0, _max_size);
		strcat_ro(_page, WEB_SERVER_HEADER_HTML);
		strcat_ro(_page, WEB_SERVER_GPIO_WRITE_PAGE_TOP);
		char *_gpio_digital_write_options[] = {"LOW", "HIGH"};
		int _gpio_digital_write_options_size = sizeof(_gpio_digital_write_options) / sizeof(_gpio_digital_write_options[0]);

		char _input_value[10], _name[6], _label[6];

		bool _added_options = false;
		for (uint8_t _pin = 0; _pin < MAX_DIGITAL_GPIO_PINS; _pin++)
		{
			memset(_name, 0, 6);
			memset(_label, 0, 6);
			__appendUintToBuff(_name, "D%d:", _pin, 5);
			__appendUintToBuff(_label, "d%d", _pin, 5);

			if (!__i_dvc_ctrl.isExceptionalGpio(_pin))
			{
				if (__gpio_service.m_gpio_config_copy.gpio_mode[_pin] == DIGITAL_WRITE)
				{
					_added_options = true;
					concat_tr_select_html_tags(_page, _name, _label, _gpio_digital_write_options, _gpio_digital_write_options_size, (int)__gpio_service.m_gpio_config_copy.gpio_readings[_pin]);
				}
				if (__gpio_service.m_gpio_config_copy.gpio_mode[_pin] == ANALOG_WRITE ||
					__gpio_service.m_gpio_config_copy.gpio_mode[_pin] == DIGITAL_BLINK)
				{
					_added_options = true;
					memset(_input_value, 0, 10);
					__appendUintToBuff(_input_value, "%d", __gpio_service.m_gpio_config_copy.gpio_readings[_pin], 8);

					if (DIGITAL_BLINK == __gpio_service.m_gpio_config_copy.gpio_mode[_pin])
					{
						concat_tr_input_html_tags(_page, _name, _label, _input_value, 10, HTML_INPUT_TEXT_TAG_TYPE, false, false);
					}
					else
					{
						concat_tr_input_html_tags(_page, _name, _label, _input_value, 0, HTML_INPUT_RANGE_TAG_TYPE, false, false);
					}
				}
			}
		}

		if (_added_options)
		{
			strcat_ro(_page, WEB_SERVER_WIFI_CONFIG_PAGE_BOTTOM);
		}
		else
		{
			strcat_ro(_page, WEB_SERVER_GPIO_WRITE_EMPTY_MESSAGE);
		}
		if (_enable_flash)
			concat_flash_message_div(_page, HTML_SUCCESS_FLASH, ALERT_SUCCESS);
		strcat_ro(_page, WEB_SERVER_FOOTER_HTML);
	}

	/**
	 * build and send gpio write config page.
	 * when posted, get gpio write configs from client and set them in database.
	 */
	void handleGpioWriteConfigRoute(void)
	{
		LogI("Handling Gpio Write Config route\n");

		if (nullptr == this->m_web_resource ||
			nullptr == this->m_web_resource->m_db_conn ||
			nullptr == this->m_web_resource->m_server)
		{
			return;
		}

		bool _is_posted = false;

		if (true)
		{
			char _label[6];

			for (uint8_t _pin = 0; _pin < MAX_DIGITAL_GPIO_PINS; _pin++)
			{
				memset(_label, 0, 6);
				__appendUintToBuff(_label, "d%d", _pin, 5);

				uint16_t _val = StringToUint16(this->m_web_resource->m_server->arg(_label).c_str());
				if (this->m_web_resource->m_server->hasArg(_label))
				{
					__gpio_service.m_gpio_config_copy.gpio_readings[_pin] = __i_dvc_ctrl.isExceptionalGpio(_pin) ? 0 : _val;

					LogFmtI("Pin %d : %d\n", _pin, _val);
					_is_posted = true;
				}
			}

			if (_is_posted)
			{
				this->m_web_resource->m_db_conn->set_gpio_config_table(&__gpio_service.m_gpio_config_copy);
			}
		}

		char *_page = new char[PAGE_HTML_MAX_SIZE];
		this->build_gpio_write_config_html(_page, _is_posted);

		this->m_web_resource->m_server->send(HTTP_OK, TEXT_HTML_CONTENT, _page);
		delete[] _page;
		if (_is_posted)
		{
			__gpio_service.handleGpioModes(GPIO_WRITE_CONFIG);
		}
	}

	/**
	 * build gpio alert config html.
	 *
	 * @param	char*	_page
	 * @param	bool|false	_enable_flash
	 * @param	int|PAGE_HTML_MAX_SIZE	_max_size
	 */
	void build_gpio_alert_config_html(char *_page, bool _enable_flash = false, int _max_size = PAGE_HTML_MAX_SIZE)
	{
		memset(_page, 0, _max_size);
		strcat_ro(_page, WEB_SERVER_HEADER_HTML);
		strcat_ro(_page, WEB_SERVER_GPIO_ALERT_PAGE_TOP);
		char *_gpio_digital_alert_options[] = {"LOW", "HIGH"};
#ifdef ENABLE_EMAIL_SERVICE
		char *_gpio_alert_channels[] = {"NOALERT", "MAIL", "HTTPSERVER"};
#else
		char *_gpio_alert_channels[] = {"NOALERT", "HTTPSERVER"};
#endif
		char *_gpio_analog_alert_comparators[] = {"=", ">", "<"};

		int _gpio_digital_alert_options_size = sizeof(_gpio_digital_alert_options) / sizeof(_gpio_digital_alert_options[0]);
		int _gpio_alert_channels_size = sizeof(_gpio_alert_channels) / sizeof(_gpio_alert_channels[0]);
		int _gpio_analog_alert_comparators_size = sizeof(_gpio_analog_alert_comparators) / sizeof(_gpio_analog_alert_comparators[0]);

		char _analog_value[10], _name[8], _label[8], _alert_label[8], _alert_value[8];

		bool _added_options = false;
		for (uint8_t _pin = 0; _pin < MAX_DIGITAL_GPIO_PINS; _pin++)
		{
			memset(_name, 0, 8);
			memset(_label, 0, 8);
			memset(_alert_label, 0, 8);
			__appendUintToBuff(_name, "D%d", _pin, 7);
			__appendUintToBuff(_label, "d%d", _pin, 7);
			__appendUintToBuff(_alert_label, "al%d", _pin, 7);

			if (!__i_dvc_ctrl.isExceptionalGpio(_pin) && __gpio_service.m_gpio_config_copy.gpio_mode[_pin] == DIGITAL_READ)
			{
				_added_options = true;
				strcat_ro(_page, HTML_TR_OPEN_TAG);
				strcat_ro(_page, HTML_TAG_CLOSE_BRACKET);
				concat_td_select_html_tags(_page, _name, _label, _gpio_digital_alert_options, _gpio_digital_alert_options_size, (int)__gpio_service.m_gpio_config_copy.gpio_alert_values[_pin]);
				concat_td_select_html_tags(_page, (char *)" ? ", _alert_label, _gpio_alert_channels, _gpio_alert_channels_size, (int)__gpio_service.m_gpio_config_copy.gpio_alert_channel[_pin]);
				strcat_ro(_page, HTML_TR_CLOSE_TAG);
			}
		}

		for (uint8_t _pin = 0; _pin < MAX_ANALOG_GPIO_PINS; _pin++)
		{
			memset(_name, 0, 8);
			memset(_label, 0, 8);
			memset(_alert_label, 0, 8);
			memset(_alert_value, 0, 8);
			__appendUintToBuff(_name, "A%d", _pin, 7);
			__appendUintToBuff(_label, "a%d", _pin, 7);
			__appendUintToBuff(_alert_label, "anlt%d", _pin, 7);
			__appendUintToBuff(_alert_value, "aval%d", _pin, 7);

			if (__gpio_service.m_gpio_config_copy.gpio_mode[MAX_DIGITAL_GPIO_PINS + _pin] == ANALOG_READ)
			{
				_added_options = true;
				memset(_analog_value, 0, 10);
				__appendUintToBuff(_analog_value, "%d", __gpio_service.m_gpio_config_copy.gpio_alert_values[MAX_DIGITAL_GPIO_PINS + _pin], 8);
				strcat_ro(_page, HTML_TR_OPEN_TAG);
				strcat_ro(_page, HTML_TAG_CLOSE_BRACKET);
				strcat_ro(_page, HTML_TD_OPEN_TAG);
				strcat_ro(_page, HTML_TAG_CLOSE_BRACKET);
				strcat(_page, _name);
				strcat_ro(_page, HTML_TD_CLOSE_TAG);
				strcat_ro(_page, HTML_TD_OPEN_TAG);
				strcat_ro(_page, HTML_STYLE_ATTR);
				strcat(_page, (char *)"'display:flex;'");
				strcat_ro(_page, HTML_TAG_CLOSE_BRACKET);
				concat_select_html_tag(_page, _label, _gpio_analog_alert_comparators, _gpio_analog_alert_comparators_size, (int)__gpio_service.m_gpio_config_copy.gpio_alert_comparator[MAX_DIGITAL_GPIO_PINS + _pin]);
				concat_input_html_tag(_page, _alert_value, _analog_value);
				strcat_ro(_page, HTML_TD_CLOSE_TAG);
				concat_td_select_html_tags(_page, (char *)" ? ", _alert_label, _gpio_alert_channels, _gpio_alert_channels_size, (int)__gpio_service.m_gpio_config_copy.gpio_alert_channel[MAX_DIGITAL_GPIO_PINS + _pin]);
				strcat_ro(_page, HTML_TR_CLOSE_TAG);
			}
		}

		if (_added_options)
		{
			strcat_ro(_page, WEB_SERVER_WIFI_CONFIG_PAGE_BOTTOM);
		}
		else
		{
			strcat_ro(_page, WEB_SERVER_GPIO_ALERT_EMPTY_MESSAGE);
		}
		if (_enable_flash)
			concat_flash_message_div(_page, HTML_SUCCESS_FLASH, ALERT_SUCCESS);
		strcat_ro(_page, WEB_SERVER_FOOTER_HTML);
	}

	/**
	 * build and send gpio alert config page.
	 * when posted, get gpio alert configs from client and set them in database.
	 */
	void handleGpioAlertConfigRoute(void)
	{
		LogI("Handling Gpio Alert Config route\n");

		if (nullptr == this->m_web_resource ||
			nullptr == this->m_web_resource->m_db_conn ||
			nullptr == this->m_web_resource->m_server)
		{
			return;
		}

		bool _is_posted = false;

		if (true)
		{
			char _label[8], _alert_label[8], _alert_value[8];

			for (uint8_t _pin = 0; _pin < MAX_DIGITAL_GPIO_PINS; _pin++)
			{
				memset(_label, 0, 8);
				memset(_alert_label, 0, 8);
				__appendUintToBuff(_label, "d%d", _pin, 7);
				__appendUintToBuff(_alert_label, "al%d", _pin, 7);

				if (this->m_web_resource->m_server->hasArg(_label) && this->m_web_resource->m_server->hasArg(_alert_label))
				{
					__gpio_service.m_gpio_config_copy.gpio_alert_comparator[_pin] = EQUAL;
					__gpio_service.m_gpio_config_copy.gpio_alert_values[_pin] = StringToUint16(this->m_web_resource->m_server->arg(_label).c_str());
					__gpio_service.m_gpio_config_copy.gpio_alert_channel[_pin] = StringToUint8(this->m_web_resource->m_server->arg(_alert_label).c_str());

					LogFmtI("Pin D%d : %d : %d\n", _pin, 
					__gpio_service.m_gpio_config_copy.gpio_alert_values[_pin], 
					__gpio_service.m_gpio_config_copy.gpio_alert_channel[_pin]);

					_is_posted = true;
				}
			}

			for (uint8_t _pin = 0; _pin < MAX_ANALOG_GPIO_PINS; _pin++)
			{
				memset(_label, 0, 8);
				memset(_alert_label, 0, 8);
				memset(_alert_value, 0, 8);
				__appendUintToBuff(_label, "a%d", _pin, 7);
				__appendUintToBuff(_alert_label, "anlt%d", _pin, 7);
				__appendUintToBuff(_alert_value, "aval%d", _pin, 7);

				if (this->m_web_resource->m_server->hasArg(_label) && this->m_web_resource->m_server->hasArg(_alert_value) && this->m_web_resource->m_server->hasArg(_alert_label))
				{
					__gpio_service.m_gpio_config_copy.gpio_alert_comparator[MAX_DIGITAL_GPIO_PINS + _pin] = StringToUint8(this->m_web_resource->m_server->arg(_label).c_str());
					__gpio_service.m_gpio_config_copy.gpio_alert_values[MAX_DIGITAL_GPIO_PINS + _pin] = StringToUint16(this->m_web_resource->m_server->arg(_alert_value).c_str());
					__gpio_service.m_gpio_config_copy.gpio_alert_channel[MAX_DIGITAL_GPIO_PINS + _pin] = StringToUint8(this->m_web_resource->m_server->arg(_alert_label).c_str());

					LogFmtI("Pin A%d : %d : %d\n", _pin, 
					__gpio_service.m_gpio_config_copy.gpio_alert_values[MAX_DIGITAL_GPIO_PINS + _pin], 
					__gpio_service.m_gpio_config_copy.gpio_alert_channel[MAX_DIGITAL_GPIO_PINS + _pin]);

					_is_posted = true;
				}
			}

			if (_is_posted)
			{
				this->m_web_resource->m_db_conn->set_gpio_config_table(&__gpio_service.m_gpio_config_copy);
			}
		}

		char *_page = new char[PAGE_HTML_MAX_SIZE];
		this->build_gpio_alert_config_html(_page, _is_posted);

		this->m_web_resource->m_server->send(HTTP_OK, TEXT_HTML_CONTENT, _page);
		delete[] _page;
		if (_is_posted)
		{
			__gpio_service.handleGpioModes(GPIO_ALERT_CONFIG);
		}
	}
};

#endif
