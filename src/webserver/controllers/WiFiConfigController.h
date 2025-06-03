/************************** WiFi Config Controller ****************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st June 2019
******************************************************************************/

#ifndef _WEB_SERVER_WIFI_CONFIG_CONTROLLER_
#define _WEB_SERVER_WIFI_CONFIG_CONTROLLER_

#include "Controller.h"
#include <webserver/pages/WiFiConfigPage.h>

/**
 * WiFiConfigController class
 */
class WiFiConfigController : public Controller
{

protected:
  /**
   * @var	wifi_config_table  wifi_configs
   */
  wifi_config_table wifi_configs;

public:
  /**
   * WiFiConfigController constructor
   */
  WiFiConfigController() : Controller("wifi")
  {
  }

  /**
   * WiFiConfigController destructor
   */
  ~WiFiConfigController()
  {
  }

  /**
   * register wifi config controller
   *
   */
  void boot(void)
  {
    if (nullptr != this->m_web_resource && nullptr != this->m_web_resource->m_db_conn)
    {
      this->m_web_resource->m_db_conn->get_wifi_config_table(&this->wifi_configs);
    }
    if (nullptr != this->m_route_handler)
    {
      this->m_route_handler->register_route(
          WEB_SERVER_WIFI_CONFIG_ROUTE, [&]()
          { this->handleWiFiConfigRoute(); },
          AUTH_MIDDLEWARE);
    }
  }

  /**
   * build wifi config html.
   *
   * @param	char*	_page
   * @param	bool|false	_is_error
   * @param	bool|false	_enable_flash
   * @param	int|PAGE_HTML_MAX_SIZE	_max_size
   */
  void build_wifi_config_html(char *_page, bool _is_error = false, bool _enable_flash = false, int _max_size = PAGE_HTML_MAX_SIZE)
  {

    memset(_page, 0, _max_size);
    char _ip_address[20];
    strcat_ro(_page, WEB_SERVER_HEADER_HTML);
    strcat_ro(_page, WEB_SERVER_WIFI_CONFIG_PAGE_TOP);

#ifdef ALLOW_WIFI_CONFIG_MODIFICATION

    concat_tr_input_html_tags(_page, RODT_ATTR("WiFi Name:"), RODT_ATTR("sta_ssid"), this->wifi_configs.sta_ssid, WIFI_CONFIGS_BUF_SIZE - 1);
    concat_tr_input_html_tags(_page, RODT_ATTR("WiFi Password:"), RODT_ATTR("sta_pswd"), this->wifi_configs.sta_password, WIFI_CONFIGS_BUF_SIZE - 1);

    __int_ip_to_str(_ip_address, this->wifi_configs.sta_local_ip, 20);
    concat_tr_input_html_tags(_page, RODT_ATTR("WiFi Local Ip:"), RODT_ATTR("sta_lip"), _ip_address);
    __int_ip_to_str(_ip_address, this->wifi_configs.sta_gateway, 20);
    concat_tr_input_html_tags(_page, RODT_ATTR("WiFi Gateway:"), RODT_ATTR("sta_gip"), _ip_address);
    __int_ip_to_str(_ip_address, this->wifi_configs.sta_subnet, 20);
    concat_tr_input_html_tags(_page, RODT_ATTR("WiFi Subnet:"), RODT_ATTR("sta_sip"), _ip_address);

    concat_tr_input_html_tags(_page, RODT_ATTR("Access Name:"), RODT_ATTR("ap_ssid"), this->wifi_configs.ap_ssid, WIFI_CONFIGS_BUF_SIZE - 1);
    concat_tr_input_html_tags(_page, RODT_ATTR("Access Password:"), RODT_ATTR("ap_pswd"), this->wifi_configs.ap_password, WIFI_CONFIGS_BUF_SIZE - 1);

    __int_ip_to_str(_ip_address, this->wifi_configs.ap_local_ip, 20);
    concat_tr_input_html_tags(_page, RODT_ATTR("Access Local Ip:"), RODT_ATTR("ap_lip"), _ip_address);
    __int_ip_to_str(_ip_address, this->wifi_configs.ap_gateway, 20);
    concat_tr_input_html_tags(_page, RODT_ATTR("Access Gateway:"), RODT_ATTR("ap_gip"), _ip_address);
    __int_ip_to_str(_ip_address, this->wifi_configs.ap_subnet, 20);
    concat_tr_input_html_tags(_page, RODT_ATTR("Access Subnet:"), RODT_ATTR("ap_sip"), _ip_address);

    strcat_ro(_page, WEB_SERVER_WIFI_CONFIG_PAGE_BOTTOM);

#else

#ifdef ALLOW_WIFI_SSID_PASSKEY_CONFIG_MODIFICATION_ONLY

    concat_tr_input_html_tags(_page, RODT_ATTR("WiFi Name:"), RODT_ATTR("sta_ssid"), this->wifi_configs.sta_ssid, WIFI_CONFIGS_BUF_SIZE - 1);
    concat_tr_input_html_tags(_page, RODT_ATTR("WiFi Password:"), RODT_ATTR("sta_pswd"), this->wifi_configs.sta_password, WIFI_CONFIGS_BUF_SIZE - 1);

#else

    concat_tr_input_html_tags(_page, RODT_ATTR("WiFi Name:"), RODT_ATTR("sta_ssid"), this->wifi_configs.sta_ssid, WIFI_CONFIGS_BUF_SIZE - 1, HTML_INPUT_TEXT_TAG_TYPE, false, true);
    concat_tr_input_html_tags(_page, RODT_ATTR("WiFi Password:"), RODT_ATTR("sta_pswd"), this->wifi_configs.sta_password, WIFI_CONFIGS_BUF_SIZE - 1, HTML_INPUT_TEXT_TAG_TYPE, false, true);

#endif

    __int_ip_to_str(_ip_address, this->wifi_configs.sta_local_ip, 20);
    concat_tr_input_html_tags(_page, RODT_ATTR("WiFi Local Ip:"), RODT_ATTR("sta_lip"), _ip_address, HTML_INPUT_TAG_DEFAULT_MAXLENGTH, HTML_INPUT_TEXT_TAG_TYPE, false, true);
    __int_ip_to_str(_ip_address, this->wifi_configs.sta_gateway, 20);
    concat_tr_input_html_tags(_page, RODT_ATTR("WiFi Gateway:"), RODT_ATTR("sta_gip"), _ip_address, HTML_INPUT_TAG_DEFAULT_MAXLENGTH, HTML_INPUT_TEXT_TAG_TYPE, false, true);
    __int_ip_to_str(_ip_address, this->wifi_configs.sta_subnet, 20);
    concat_tr_input_html_tags(_page, RODT_ATTR("WiFi Subnet:"), RODT_ATTR("sta_sip"), _ip_address, HTML_INPUT_TAG_DEFAULT_MAXLENGTH, HTML_INPUT_TEXT_TAG_TYPE, false, true);

    concat_tr_input_html_tags(_page, RODT_ATTR("Access Name:"), RODT_ATTR("ap_ssid"), this->wifi_configs.ap_ssid, WIFI_CONFIGS_BUF_SIZE - 1, HTML_INPUT_TEXT_TAG_TYPE, false, true);
    concat_tr_input_html_tags(_page, RODT_ATTR("Access Password:"), RODT_ATTR("ap_pswd"), this->wifi_configs.ap_password, WIFI_CONFIGS_BUF_SIZE - 1, HTML_INPUT_TEXT_TAG_TYPE, false, true);

    __int_ip_to_str(_ip_address, this->wifi_configs.ap_local_ip, 20);
    concat_tr_input_html_tags(_page, RODT_ATTR("Access Local Ip:"), RODT_ATTR("ap_lip"), _ip_address, HTML_INPUT_TAG_DEFAULT_MAXLENGTH, HTML_INPUT_TEXT_TAG_TYPE, false, true);
    __int_ip_to_str(_ip_address, this->wifi_configs.ap_gateway, 20);
    concat_tr_input_html_tags(_page, RODT_ATTR("Access Gateway:"), RODT_ATTR("ap_gip"), _ip_address, HTML_INPUT_TAG_DEFAULT_MAXLENGTH, HTML_INPUT_TEXT_TAG_TYPE, false, true);
    __int_ip_to_str(_ip_address, this->wifi_configs.ap_subnet, 20);
    concat_tr_input_html_tags(_page, RODT_ATTR("Access Subnet:"), RODT_ATTR("ap_sip"), _ip_address, HTML_INPUT_TAG_DEFAULT_MAXLENGTH, HTML_INPUT_TEXT_TAG_TYPE, false, true);

#ifdef ALLOW_WIFI_SSID_PASSKEY_CONFIG_MODIFICATION_ONLY

    strcat_ro(_page, WEB_SERVER_WIFI_CONFIG_PAGE_BOTTOM);

#endif

#endif

    if (_enable_flash)
      concat_flash_message_div(_page, _is_error ? RODT_ATTR("Invalid length error(3-20)") : RODT_ATTR("Config saved Successfully..applying new configs."), _is_error ? ALERT_DANGER : ALERT_SUCCESS);
    strcat_ro(_page, WEB_SERVER_FOOTER_HTML);
  }

  /**
   * build and send wifi config page.
   * when posted, get wifi configs from client and set them in database.
   */
  void handleWiFiConfigRoute(void)
  {

    LogI("Handling WiFi Config route\n");

    if (nullptr == this->m_web_resource ||
        nullptr == this->m_web_resource->m_db_conn ||
        nullptr == this->m_web_resource->m_server ||
        nullptr == this->m_route_handler)
    {
      return;
    }

    this->m_web_resource->m_db_conn->get_wifi_config_table(&this->wifi_configs);

    bool _is_posted = false;
    bool _is_error = true;

#if defined(ALLOW_WIFI_CONFIG_MODIFICATION) || defined(ALLOW_WIFI_SSID_PASSKEY_CONFIG_MODIFICATION_ONLY)
    if (this->m_web_resource->m_server->hasArg("sta_ssid") && this->m_web_resource->m_server->hasArg("sta_pswd"))
    {

      pdiutil::string _sta_ssid = this->m_web_resource->m_server->arg("sta_ssid");
      pdiutil::string _sta_pswd = this->m_web_resource->m_server->arg("sta_pswd");
      pdiutil::string _sta_lip = this->m_web_resource->m_server->arg("sta_lip");
      pdiutil::string _sta_gip = this->m_web_resource->m_server->arg("sta_gip");
      pdiutil::string _sta_sip = this->m_web_resource->m_server->arg("sta_sip");

      pdiutil::string _ap_ssid = this->m_web_resource->m_server->arg("ap_ssid");
      pdiutil::string _ap_pswd = this->m_web_resource->m_server->arg("ap_pswd");
      pdiutil::string _ap_lip = this->m_web_resource->m_server->arg("ap_lip");
      pdiutil::string _ap_gip = this->m_web_resource->m_server->arg("ap_gip");
      pdiutil::string _ap_sip = this->m_web_resource->m_server->arg("ap_sip");

      LogI("\nSubmitted info :\n");
      LogFmtI("sta ssid : %s\n", _sta_ssid.c_str());
      LogFmtI("sta password : %s\n", _sta_pswd.c_str());
      LogFmtI("sta local ip : %s\n", _sta_lip.c_str());
      LogFmtI("sta gateway : %s\n", _sta_gip.c_str());
      LogFmtI("sta subnet : %s\n", _sta_sip.c_str());
      LogFmtI("ap ssid : %s\n", _ap_ssid.c_str());
      LogFmtI("ap password : %s\n", _ap_pswd.c_str());
      LogFmtI("ap local ip : %s\n", _ap_lip.c_str());
      LogFmtI("ap gateway : %s\n", _ap_gip.c_str());
      LogFmtI("ap subnet : %s\n\n", _ap_sip.c_str());

      if (_sta_ssid.size() <= WIFI_CONFIGS_BUF_SIZE && _sta_pswd.size() <= WIFI_CONFIGS_BUF_SIZE

#ifdef ALLOW_WIFI_CONFIG_MODIFICATION
          && _ap_ssid.size() <= WIFI_CONFIGS_BUF_SIZE && _ap_pswd.size() <= WIFI_CONFIGS_BUF_SIZE &&
          _sta_ssid.size() > MIN_ACCEPTED_ARG_SIZE && _ap_ssid.size() > MIN_ACCEPTED_ARG_SIZE
#endif

      )
      {

        char _ip_address[20];
        memset(_ip_address, 0, 20);

#ifdef ALLOW_WIFI_CONFIG_MODIFICATION

        _ClearObject(&this->wifi_configs);

        strncpy(this->wifi_configs.sta_ssid, _sta_ssid.c_str(), _sta_ssid.size());
        strncpy(this->wifi_configs.sta_password, _sta_pswd.c_str(), _sta_pswd.size());
        strncpy(_ip_address, _sta_lip.c_str(), _sta_lip.size());
        __str_ip_to_int(_ip_address, this->wifi_configs.sta_local_ip, 20);
        strncpy(_ip_address, _sta_gip.c_str(), _sta_gip.size());
        __str_ip_to_int(_ip_address, this->wifi_configs.sta_gateway, 20);
        strncpy(_ip_address, _sta_sip.c_str(), _sta_sip.size());
        __str_ip_to_int(_ip_address, this->wifi_configs.sta_subnet, 20);

        strncpy(this->wifi_configs.ap_ssid, _ap_ssid.c_str(), _ap_ssid.size());
        strncpy(this->wifi_configs.ap_password, _ap_pswd.c_str(), _ap_pswd.size());
        strncpy(_ip_address, _ap_lip.c_str(), _ap_lip.size());
        __str_ip_to_int(_ip_address, this->wifi_configs.ap_local_ip, 20);
        strncpy(_ip_address, _ap_gip.c_str(), _ap_gip.size());
        __str_ip_to_int(_ip_address, this->wifi_configs.ap_gateway, 20);
        strncpy(_ip_address, _ap_sip.c_str(), _ap_sip.size());
        __str_ip_to_int(_ip_address, this->wifi_configs.ap_subnet, 20);

#else

        strncpy(this->wifi_configs.sta_ssid, _sta_ssid.c_str(), _sta_ssid.size());
        strncpy(this->wifi_configs.sta_password, _sta_pswd.c_str(), _sta_pswd.size());

#endif

        this->m_web_resource->m_db_conn->set_wifi_config_table(&this->wifi_configs);
        // this->set_wifi_config_table( &this->wifi_configs );
        _is_error = false;
      }
      _is_posted = true;
    }
#endif

    char *_page = new char[PAGE_HTML_MAX_SIZE];
    this->build_wifi_config_html(_page, _is_error, _is_posted);

    if (_is_posted && !_is_error)
    {
      this->m_route_handler->send_inactive_session_headers();
    }
    this->m_web_resource->m_server->send(HTTP_OK, TEXT_HTML_CONTENT, _page);
    delete[] _page;
    if (_is_posted && !_is_error)
    {
      __i_dvc_ctrl.wait(100);
      __i_dvc_ctrl.restartDevice();
    }
  }
};

#endif
