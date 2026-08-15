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
#include <service_provider/device/OtaServiceProvider.h>

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
	void build_ota_server_config_html(char *_page, bool _enable_flash = false, const char *_message = nullptr, FLASH_MSG_TYPE _alert_type = ALERT_SUCCESS, int _max_size = PAGE_HTML_MAX_SIZE)
	{
		if (nullptr == this->m_web_resource ||
			nullptr == this->m_web_resource->m_db_conn)
		{
			return;
		}

		// memset(_page, 0, _max_size);
		concat_header_html( _page );
		strcat_ro(_page, WEB_SERVER_OTA_CONFIG_PAGE_TOP);
		CONTINUE_SEND_IN_CHUNK(_page);

		ota_config_table _ota_configs;
		this->m_web_resource->m_db_conn->get_ota_config_table(&_ota_configs);

		char _port[10];
		memset(_port, 0, 10);
		__appendUintToBuff(_port, "%d", _ota_configs.ota_port, 8);

#ifdef ALLOW_OTA_CONFIG_MODIFICATION

		concat_tr_input_html_tags(_page, RODT_ATTR("OTA Host:"), RODT_ATTR("hst"), _ota_configs.ota_host, OTA_HOST_BUF_SIZE - 1);
		concat_tr_input_html_tags(_page, RODT_ATTR("OTA Port:"), RODT_ATTR("prt"), _port);

		concat_csrf_input_html_tag( _page );
		strcat_ro(_page, WEB_SERVER_WIFI_CONFIG_PAGE_BOTTOM);
#else

		concat_tr_input_html_tags(_page, RODT_ATTR("OTA Host:"), RODT_ATTR("hst"), _ota_configs.ota_host, OTA_HOST_BUF_SIZE - 1, HTML_INPUT_TEXT_TAG_TYPE, false, true);
		concat_tr_input_html_tags(_page, RODT_ATTR("OTA Port:"), RODT_ATTR("prt"), _port, HTML_INPUT_TAG_DEFAULT_MAXLENGTH, HTML_INPUT_TEXT_TAG_TYPE, false, true);

		strcat_ro(_page, HTML_TABLE_CLOSE_TAG);
		strcat_ro(_page, HTML_FORM_CLOSE_TAG);
#endif

#ifdef ENABLE_STORAGE_SERVICE
		this->build_local_flash_html(_page);
#endif

		if (_enable_flash)
			concat_flash_message_div(_page, nullptr != _message ? (char *)_message : HTML_SUCCESS_FLASH, _alert_type);
		strcat_ro(_page, WEB_SERVER_FOOTER_HTML);
		CONTINUE_SEND_IN_CHUNK(_page);
	}

#ifdef ENABLE_STORAGE_SERVICE
	/**
	 * build the local image flashing form listing images found on storage.
	 *
	 * @param	char*	_page
	 */
	void build_local_flash_html(char *_page)
	{
		strcat_ro(_page, WEB_SERVER_OTA_LOCAL_FLASH_TOP);
		CONTINUE_SEND_IN_CHUNK(_page);

		pdiutil::vector<pdiutil::string> _images;
		__ota_service.collectLocalImages(__i_fs.getHomeDirectory(), _images);

		if (_images.size() > 0)
		{
			const char **_options = pdiutil::safe_new_array<const char *>(_images.size());

			if (nullptr != _options)
			{
				for (uint32_t i = 0; i < _images.size(); i++)
				{
					_options[i] = _images[i].c_str();
				}

				concat_tr_select_html_tags(_page, RODT_ATTR("Image:"), RODT_ATTR("img"), _options, _images.size(), 0);
				pdiutil::safe_delete_array(_options);
			}

			concat_csrf_input_html_tag(_page);
			strcat_ro(_page, WEB_SERVER_OTA_LOCAL_FLASH_BOTTOM);
		}
		else
		{
			strcat_ro(_page, WEB_SERVER_OTA_NO_LOCAL_IMAGE);
		}

		CONTINUE_SEND_IN_CHUNK(_page);
	}
#endif

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
		bool _is_error = false;
		bool _is_flashed = false;
		pdiutil::string _message;

#ifdef ENABLE_STORAGE_SERVICE
		if (this->m_web_resource->m_server->hasArg("img"))
		{
			pdiutil::string _image = this->m_web_resource->m_server->arg("img");
			pdiutil::string _path;

			_is_posted = true;

			// only a name the scan itself produced is accepted, so a crafted
			// value cannot reach a file outside the image directory
			if (this->resolveLocalImage(_image, _path))
			{
				_is_flashed = (UPGRADE_STATUS_SUCCESS == __ota_service.flashFromFile(_path.c_str()));
				_is_error = !_is_flashed;
				_message = __i_fs.basename(_path.c_str());
				_message += _is_flashed ? CHARPTR_WRAP(" flashed. Restarting.")
				                        : CHARPTR_WRAP(" failed to flash. Firmware unchanged.");
			}
			else
			{
				_is_error = true;
				_message = CHARPTR_WRAP("Unknown image.");
			}
		}
#endif

#ifdef ALLOW_OTA_CONFIG_MODIFICATION
		if (this->m_web_resource->m_server->hasArg("hst") && this->m_web_resource->m_server->hasArg("prt"))
		{
			pdiutil::string _ota_host = this->m_web_resource->m_server->arg("hst");
			pdiutil::string _ota_port = this->m_web_resource->m_server->arg("prt");

			LogI("\nSubmitted info :\n");
			LogI("ota host : %s\n", _ota_host.c_str());
			LogI("ota port : %s\n\n", _ota_port.c_str());

			ota_config_table _ota_configs;
			// this->m_web_resource->m_db_conn->get_ota_config_table(&_ota_configs);

			strncpy(_ota_configs.ota_host, _ota_host.c_str(), _ota_host.size());
			_ota_configs.ota_port = StringToUint16(_ota_port.c_str());

			this->m_web_resource->m_db_conn->set_ota_config_table(&_ota_configs);

			_is_posted = true;
		}
#endif

		char *_page = pdiutil::safe_new_array<char>(PAGE_HTML_MAX_SIZE);
		if (nullptr == _page) return;

		BEGIN_SEND_IN_CHUNK(HTTP_RESP_OK, MIME_TYPE_TEXT_HTML, _page);
		this->build_ota_server_config_html(_page, _is_posted, _message.empty() ? nullptr : _message.c_str(),
										   _is_error ? ALERT_DANGER : ALERT_SUCCESS);
		END_SENDING_CHUNK();

		// this->m_web_resource->m_server->send(HTTP_RESP_OK, MIME_TYPE_TEXT_HTML, _page);
		pdiutil::safe_delete_array(_page);

#ifdef ENABLE_STORAGE_SERVICE
		if (_is_flashed)
		{
			__i_dvc_ctrl.wait(100);
			__i_dvc_ctrl.restartDevice();
		}
#endif
	}

#ifdef ENABLE_STORAGE_SERVICE
	/**
	 * resolve a submitted image name against the images actually on storage.
	 *
	 * @param	pdiutil::string&	_image
	 * @param	pdiutil::string&	_path
	 * @return	bool
	 */
	bool resolveLocalImage(const pdiutil::string &_image, pdiutil::string &_path)
	{
		if (_image.empty())
		{
			return false;
		}

		// the select carries the option index as its value, so anything that is
		// not a plain number never came from the rendered form
		for (uint32_t i = 0; i < _image.size(); i++)
		{
			if (_image[i] < '0' || _image[i] > '9')
			{
				return false;
			}
		}

		pdiutil::vector<pdiutil::string> _images;
		__ota_service.collectLocalImages(__i_fs.getHomeDirectory(), _images);

		uint32_t _index = StringToUint32(_image.c_str());
		if (_index >= _images.size())
		{
			return false;
		}

		_path = __i_fs.getHomeDirectory();
		__i_fs.appendFileSeparator(_path);
		_path += _images[_index];

		return true;
	}
#endif
};

#endif
