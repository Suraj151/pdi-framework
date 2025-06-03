/******************************* MQTT Controller ******************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st June 2019
******************************************************************************/

#ifndef _WEB_SERVER_MQTT_CONTROLLER_
#define _WEB_SERVER_MQTT_CONTROLLER_

#include "Controller.h"
#include <webserver/pages/MqttConfigPage.h>
#include <service_provider/transport/MqttServiceProvider.h>

/**
 * MqttController class
 */
class MqttController : public Controller
{

public:
  /**
   * MqttController constructor
   */
  MqttController() : Controller("mqtt")
  {
  }

  /**
   * MqttController destructor
   */
  ~MqttController()
  {
  }

  /**
   * register mqtt controller
   *
   */
  void boot(void)
  {
    if (nullptr != this->m_route_handler)
    {
      this->m_route_handler->register_route(
          WEB_SERVER_MQTT_MANAGE_CONFIG_ROUTE, [&]()
          { this->handleMqttManageRoute(); },
          AUTH_MIDDLEWARE);
      this->m_route_handler->register_route(
          WEB_SERVER_MQTT_GENERAL_CONFIG_ROUTE, [&]()
          { this->handleMqttGeneralConfigRoute(); },
          AUTH_MIDDLEWARE);
      this->m_route_handler->register_route(
          WEB_SERVER_MQTT_LWT_CONFIG_ROUTE, [&]()
          { this->handleMqttLWTConfigRoute(); },
          AUTH_MIDDLEWARE);
      this->m_route_handler->register_route(
          WEB_SERVER_MQTT_PUBSUB_CONFIG_ROUTE, [&]()
          { this->handleMqttPubSubConfigRoute(); },
          AUTH_MIDDLEWARE);
    }
  }

  /**
   * build and send mqtt manage page to client
   */
  void handleMqttManageRoute(void)
  {
    LogI("Handling Mqtt Manage route\n");

    if (nullptr == this->m_web_resource ||
        nullptr == this->m_web_resource->m_server)
    {
      return;
    }

    char *_page = new char[PAGE_HTML_MAX_SIZE];
    memset(_page, 0, PAGE_HTML_MAX_SIZE);

    strcat_ro(_page, WEB_SERVER_HEADER_HTML);
    strcat_ro(_page, WEB_SERVER_MENU_CARD_PAGE_WRAP_TOP);

    concat_svg_menu_card(_page, WEB_SERVER_MQTT_MENU_TITLE_GENERAL, SVG_ICON48_PATH_SETTINGS, WEB_SERVER_MQTT_GENERAL_CONFIG_ROUTE);
    concat_svg_menu_card(_page, WEB_SERVER_MQTT_MENU_TITLE_LWT, SVG_ICON48_PATH_BEENHERE, WEB_SERVER_MQTT_LWT_CONFIG_ROUTE);
    concat_svg_menu_card(_page, WEB_SERVER_MQTT_MENU_TITLE_PUBSUB, SVG_ICON48_PATH_IMPORT_EXPORT, WEB_SERVER_MQTT_PUBSUB_CONFIG_ROUTE);

    strcat_ro(_page, WEB_SERVER_MENU_CARD_PAGE_WRAP_BOTTOM);
    strcat_ro(_page, WEB_SERVER_FOOTER_HTML);

    this->m_web_resource->m_server->send(HTTP_OK, TEXT_HTML_CONTENT, _page);
    delete[] _page;
  }

  /**
   * build mqtt general config html.
   *
   * @param	char*	_page
   * @param	bool|false	_enable_flash
   * @param	int|PAGE_HTML_MAX_SIZE	_max_size
   */
  void build_mqtt_general_config_html(char *_page, bool _enable_flash = false, int _max_size = PAGE_HTML_MAX_SIZE)
  {
    if (nullptr == this->m_web_resource ||
        nullptr == this->m_web_resource->m_db_conn)
    {
      return;
    }

    memset(_page, 0, _max_size);
    strcat_ro(_page, WEB_SERVER_HEADER_HTML);
    strcat_ro(_page, WEB_SERVER_MQTT_GENERAL_PAGE_TOP);

    mqtt_general_config_table _mqtt_general_configs;
    this->m_web_resource->m_db_conn->get_mqtt_general_config_table(&_mqtt_general_configs);

    char _port[10], _keepalive[10];
    memset(_port, 0, 10);
    memset(_keepalive, 0, 10);
    __appendUintToBuff(_port, "%d", _mqtt_general_configs.port, 8);
    __appendUintToBuff(_keepalive, "%d", _mqtt_general_configs.keepalive, 8);

#ifdef ALLOW_MQTT_CONFIG_MODIFICATION

    concat_tr_input_html_tags(_page, RODT_ATTR("Host Address:"), RODT_ATTR("hst"), _mqtt_general_configs.host, MQTT_HOST_BUF_SIZE - 1);
    concat_tr_input_html_tags(_page, RODT_ATTR("Host Port:"), RODT_ATTR("prt"), _port);
    concat_tr_input_html_tags(_page, RODT_ATTR("Client Id:"), RODT_ATTR("clid"), _mqtt_general_configs.client_id, MQTT_CLIENT_ID_BUF_SIZE - 1);
    concat_tr_input_html_tags(_page, RODT_ATTR("Username:"), RODT_ATTR("usrn"), _mqtt_general_configs.username, MQTT_USERNAME_BUF_SIZE - 1);
    concat_tr_input_html_tags(_page, RODT_ATTR("Password:"), RODT_ATTR("pswd"), _mqtt_general_configs.password, MQTT_PASSWORD_BUF_SIZE - 1);
    concat_tr_input_html_tags(_page, RODT_ATTR("Keep Alive:"), RODT_ATTR("kpalv"), _keepalive);
    concat_tr_input_html_tags(_page, RODT_ATTR("Clean Session:"), RODT_ATTR("cln"), "clean", HTML_INPUT_TAG_DEFAULT_MAXLENGTH, HTML_INPUT_CHECKBOX_TAG_TYPE, _mqtt_general_configs.clean_session != 0);

    strcat_ro(_page, WEB_SERVER_WIFI_CONFIG_PAGE_BOTTOM);

#else

    concat_tr_input_html_tags(_page, RODT_ATTR("Host Address:"), RODT_ATTR("hst"), _mqtt_general_configs.host, MQTT_HOST_BUF_SIZE - 1, HTML_INPUT_TEXT_TAG_TYPE, false, true);
    concat_tr_input_html_tags(_page, RODT_ATTR("Host Port:"), RODT_ATTR("prt"), _port, HTML_INPUT_TAG_DEFAULT_MAXLENGTH, HTML_INPUT_TEXT_TAG_TYPE, false, true);
    concat_tr_input_html_tags(_page, RODT_ATTR("Client Id:"), RODT_ATTR("clid"), _mqtt_general_configs.client_id, MQTT_CLIENT_ID_BUF_SIZE - 1, HTML_INPUT_TEXT_TAG_TYPE, false, true);
    concat_tr_input_html_tags(_page, RODT_ATTR("Username:"), RODT_ATTR("usrn"), _mqtt_general_configs.username, MQTT_USERNAME_BUF_SIZE - 1, HTML_INPUT_TEXT_TAG_TYPE, false, true);
    concat_tr_input_html_tags(_page, RODT_ATTR("Password:"), RODT_ATTR("pswd"), _mqtt_general_configs.password, MQTT_PASSWORD_BUF_SIZE - 1, HTML_INPUT_TEXT_TAG_TYPE, false, true);
    concat_tr_input_html_tags(_page, RODT_ATTR("Keep Alive:"), RODT_ATTR("kpalv"), _keepalive, HTML_INPUT_TAG_DEFAULT_MAXLENGTH, HTML_INPUT_TEXT_TAG_TYPE, false, true);
    concat_tr_input_html_tags(_page, RODT_ATTR("Clean Session:"), RODT_ATTR("cln"), "clean", HTML_INPUT_TAG_DEFAULT_MAXLENGTH, HTML_INPUT_CHECKBOX_TAG_TYPE, _mqtt_general_configs.clean_session != 0, true);

#endif

    if (_enable_flash)
      concat_flash_message_div(_page, HTML_SUCCESS_FLASH, ALERT_SUCCESS);
    strcat_ro(_page, WEB_SERVER_FOOTER_HTML);
  }

  /**
   * build and send mqtt general config page.
   * when posted, get mqtt general configs from client and set them in database.
   */
  void handleMqttGeneralConfigRoute(void)
  {
    LogI("Handling Mqtt General Config route\n");

    if (nullptr == this->m_web_resource ||
        nullptr == this->m_web_resource->m_db_conn ||
        nullptr == this->m_web_resource->m_server)
    {
      return;
    }

    bool _is_posted = false;

#ifdef ALLOW_MQTT_CONFIG_MODIFICATION
    if (this->m_web_resource->m_server->hasArg("hst") && this->m_web_resource->m_server->hasArg("prt"))
    {
      pdiutil::string _mqtt_host = this->m_web_resource->m_server->arg("hst");
      pdiutil::string _mqtt_port = this->m_web_resource->m_server->arg("prt");
      pdiutil::string _client_id = this->m_web_resource->m_server->arg("clid");
      pdiutil::string _username = this->m_web_resource->m_server->arg("usrn");
      pdiutil::string _password = this->m_web_resource->m_server->arg("pswd");
      pdiutil::string _keep_alive = this->m_web_resource->m_server->arg("kpalv");
      pdiutil::string _clean_session = this->m_web_resource->m_server->arg("cln");

      LogI("\nSubmitted info :\n");
      LogFmtI("mqtt host : %s\n", _mqtt_host.c_str());
      LogFmtI("mqtt port : %s\n", _mqtt_port.c_str());
      LogFmtI("client id : %s\n", _client_id.c_str());
      LogFmtI("username : %s\n", _username.c_str());
      LogFmtI("password : %s\n", _password.c_str());
      LogFmtI("keep alive : %s\n", _keep_alive.c_str());
      LogFmtI("clean session : %s\n\n", _clean_session.c_str());

      mqtt_general_config_table *_mqtt_general_configs = new mqtt_general_config_table;
      memset((void *)_mqtt_general_configs, 0, sizeof(mqtt_general_config_table));
      // mqtt_general_config_table _mqtt_general_configs;
      // this->m_db_conn->get_mqtt_general_config_table(&_mqtt_general_configs);

      strncpy(_mqtt_general_configs->host, _mqtt_host.c_str(), _mqtt_host.size());
      _mqtt_general_configs->port = StringToUint16(_mqtt_port.c_str());
      strncpy(_mqtt_general_configs->client_id, _client_id.c_str(), _client_id.size());
      strncpy(_mqtt_general_configs->username, _username.c_str(), _username.size());
      strncpy(_mqtt_general_configs->password, _password.c_str(), _password.size());
      _mqtt_general_configs->keepalive = StringToUint16(_keep_alive.c_str());
      _mqtt_general_configs->clean_session = (int)(_clean_session == "clean");

      this->m_web_resource->m_db_conn->set_mqtt_general_config_table(_mqtt_general_configs);
      delete _mqtt_general_configs;

      _is_posted = true;
    }
#endif

    char *_page = new char[PAGE_HTML_MAX_SIZE];
    this->build_mqtt_general_config_html(_page, _is_posted);

    this->m_web_resource->m_server->send(HTTP_OK, TEXT_HTML_CONTENT, _page);
    delete[] _page;
    if (_is_posted)
    {
      __mqtt_service.handleMqttConfigChange(MQTT_GENERAL_CONFIG);
    }
  }

  /**
   * build mqtt lwt config html.
   *
   * @param	char*	_page
   * @param	bool|false	_enable_flash
   * @param	int|PAGE_HTML_MAX_SIZE	_max_size
   */
  void build_mqtt_lwt_config_html(char *_page, bool _enable_flash = false, int _max_size = PAGE_HTML_MAX_SIZE)
  {

    if (nullptr == this->m_web_resource ||
        nullptr == this->m_web_resource->m_db_conn)
    {
      return;
    }

    memset(_page, 0, _max_size);
    char _ip_address[20];
    strcat_ro(_page, WEB_SERVER_HEADER_HTML);
    strcat_ro(_page, WEB_SERVER_MQTT_LWT_PAGE_TOP);

    mqtt_lwt_config_table _mqtt_lwt_configs;
    this->m_web_resource->m_db_conn->get_mqtt_lwt_config_table(&_mqtt_lwt_configs);

    char *_qos_options[] = {"0", "1", "2"};

#ifdef ALLOW_MQTT_CONFIG_MODIFICATION

    concat_tr_input_html_tags(_page, RODT_ATTR("Will Topic:"), RODT_ATTR("wtpc"), _mqtt_lwt_configs.will_topic, MQTT_TOPIC_BUF_SIZE - 1);
    concat_tr_input_html_tags(_page, RODT_ATTR("Will Message:"), RODT_ATTR("wmsg"), _mqtt_lwt_configs.will_message, MQTT_TOPIC_BUF_SIZE - 1);
    concat_tr_select_html_tags(_page, RODT_ATTR("Will QoS:"), RODT_ATTR("wqos"), _qos_options, 3, _mqtt_lwt_configs.will_qos);
    concat_tr_input_html_tags(_page, RODT_ATTR("Will Retain:"), RODT_ATTR("wrtn"), "retain", HTML_INPUT_TAG_DEFAULT_MAXLENGTH, HTML_INPUT_CHECKBOX_TAG_TYPE, _mqtt_lwt_configs.will_retain != 0);

    strcat_ro(_page, WEB_SERVER_WIFI_CONFIG_PAGE_BOTTOM);

#else

    concat_tr_input_html_tags(_page, RODT_ATTR("Will Topic:"), RODT_ATTR("wtpc"), _mqtt_lwt_configs.will_topic, MQTT_TOPIC_BUF_SIZE - 1, HTML_INPUT_TEXT_TAG_TYPE, false, true);
    concat_tr_input_html_tags(_page, RODT_ATTR("Will Message:"), RODT_ATTR("wmsg"), _mqtt_lwt_configs.will_message, MQTT_TOPIC_BUF_SIZE - 1, HTML_INPUT_TEXT_TAG_TYPE, false, true);
    concat_tr_select_html_tags(_page, RODT_ATTR("Will QoS:"), RODT_ATTR("wqos"), _qos_options, 3, _mqtt_lwt_configs.will_qos, 0, true);
    concat_tr_input_html_tags(_page, RODT_ATTR("Will Retain:"), RODT_ATTR("wrtn"), "retain", HTML_INPUT_TAG_DEFAULT_MAXLENGTH, HTML_INPUT_CHECKBOX_TAG_TYPE, _mqtt_lwt_configs.will_retain != 0, true);

#endif

    if (_enable_flash)
      concat_flash_message_div(_page, HTML_SUCCESS_FLASH, ALERT_SUCCESS);
    strcat_ro(_page, WEB_SERVER_FOOTER_HTML);
  }

  /**
   * build and send mqtt lwt config page.
   * when posted, get mqtt lwt configs from client and set them in database.
   */
  void handleMqttLWTConfigRoute(void)
  {
    LogI("Handling Mqtt LWT Config route\n");

    if (nullptr == this->m_web_resource ||
        nullptr == this->m_web_resource->m_db_conn ||
        nullptr == this->m_web_resource->m_server)
    {
      return;
    }

    bool _is_posted = false;

#ifdef ALLOW_MQTT_CONFIG_MODIFICATION
    if (this->m_web_resource->m_server->hasArg("wtpc") && this->m_web_resource->m_server->hasArg("wmsg"))
    {
      pdiutil::string _will_topic = this->m_web_resource->m_server->arg("wtpc");
      pdiutil::string _will_message = this->m_web_resource->m_server->arg("wmsg");
      pdiutil::string _will_qos = this->m_web_resource->m_server->arg("wqos");
      pdiutil::string _will_retain = this->m_web_resource->m_server->arg("wrtn");

      LogI("\nSubmitted info :\n");
      LogFmtI("will topic : %s\n", _will_topic.c_str());
      LogFmtI("will message : %s\n", _will_message.c_str());
      LogFmtI("will qos : %s\n", _will_qos.c_str());
      LogFmtI("will retain : %s\n\n", _will_retain.c_str());

      mqtt_lwt_config_table *_mqtt_lwt_configs = new mqtt_lwt_config_table;
      memset((void *)_mqtt_lwt_configs, 0, sizeof(mqtt_lwt_config_table));

      strncpy(_mqtt_lwt_configs->will_topic, _will_topic.c_str(), _will_topic.size());
      strncpy(_mqtt_lwt_configs->will_message, _will_message.c_str(), _will_message.size());
      _mqtt_lwt_configs->will_qos = StringToUint16(_will_qos.c_str());
      _mqtt_lwt_configs->will_retain = (int)(_will_retain == "retain");

      this->m_web_resource->m_db_conn->set_mqtt_lwt_config_table(_mqtt_lwt_configs);

      delete _mqtt_lwt_configs;
      _is_posted = true;
    }
#endif

    char *_page = new char[PAGE_HTML_MAX_SIZE];
    this->build_mqtt_lwt_config_html(_page, _is_posted);

    this->m_web_resource->m_server->send(HTTP_OK, TEXT_HTML_CONTENT, _page);
    delete[] _page;
    if (_is_posted)
    {
      __mqtt_service.handleMqttConfigChange(MQTT_LWT_CONFIG);
    }
  }

  /**
   * build mqtt publish subscribe config html.
   *
   * @param	char*	_page
   * @param	bool|false	_enable_flash
   * @param	int|PAGE_HTML_MAX_SIZE	_max_size
   */
  void build_mqtt_pubsub_config_html(char *_page, bool _enable_flash = false, int _max_size = PAGE_HTML_MAX_SIZE)
  {

    if (nullptr == this->m_web_resource ||
        nullptr == this->m_web_resource->m_db_conn)
    {
      return;
    }

    memset(_page, 0, _max_size);
    char _ip_address[20];
    strcat_ro(_page, WEB_SERVER_HEADER_HTML);
    strcat_ro(_page, WEB_SERVER_MQTT_PUBSUB_PAGE_TOP);

    mqtt_pubsub_config_table _mqtt_pubsub_configs;
    this->m_web_resource->m_db_conn->get_mqtt_pubsub_config_table(&_mqtt_pubsub_configs);

    char *_qos_options[] = {"0", "1", "2"};
    char _topic_name[10], _topic_label[10];
    memset(_topic_name, 0, 10);
    memset(_topic_label, 0, 10);
    char _qos_name[10], _qos_label[10];
    memset(_qos_name, 0, 10);
    memset(_qos_label, 0, 10);
    char _retain_name[10], _retain_label[10];
    memset(_retain_name, 0, 10);
    memset(_retain_label, 0, 10);
    char _publish_freq[10];
    memset(_publish_freq, 0, 10);
    __appendUintToBuff(_publish_freq, "%d", _mqtt_pubsub_configs.publish_frequency, 8);
    strcpy(_topic_label, "Topic0:");
    strcpy(_topic_name, "ptpc0");
    strcpy(_qos_label, "QoS0:");
    strcpy(_qos_name, "pqos0");
    strcpy(_retain_label, "Retain0:");
    strcpy(_retain_name, "prtn0");

    concat_tr_heading_html_tags(_page, RODT_ATTR("Publish Topics"), 3, RODT_ATTR("2"));
    for (uint8_t i = 0; i < MQTT_MAX_PUBLISH_TOPIC; i++)
    {

      _topic_label[5] = (0x30 + i);
      _topic_name[4] = (0x30 + i);
      _qos_label[3] = (0x30 + i);
      _qos_name[4] = (0x30 + i);
      _retain_label[6] = (0x30 + i);
      _retain_name[4] = (0x30 + i);

#ifdef ALLOW_MQTT_CONFIG_MODIFICATION

      concat_tr_input_html_tags(_page, _topic_label, _topic_name, _mqtt_pubsub_configs.publish_topics[i].topic, MQTT_TOPIC_BUF_SIZE - 1);
      concat_tr_select_html_tags(_page, _qos_label, _qos_name, _qos_options, 3, _mqtt_pubsub_configs.publish_topics[i].qos);
      concat_tr_input_html_tags(_page, _retain_label, _retain_name, "retain", HTML_INPUT_TAG_DEFAULT_MAXLENGTH, HTML_INPUT_CHECKBOX_TAG_TYPE, _mqtt_pubsub_configs.publish_topics[i].retain != 0);

#else

      concat_tr_input_html_tags(_page, _topic_label, _topic_name, _mqtt_pubsub_configs.publish_topics[i].topic, MQTT_TOPIC_BUF_SIZE - 1, HTML_INPUT_TEXT_TAG_TYPE, false, true);
      concat_tr_select_html_tags(_page, _qos_label, _qos_name, _qos_options, 3, _mqtt_pubsub_configs.publish_topics[i].qos, 0, true);
      concat_tr_input_html_tags(_page, _retain_label, _retain_name, "retain", HTML_INPUT_TAG_DEFAULT_MAXLENGTH, HTML_INPUT_CHECKBOX_TAG_TYPE, _mqtt_pubsub_configs.publish_topics[i].retain != 0, true);

#endif
    }
#ifdef ALLOW_MQTT_CONFIG_MODIFICATION

    concat_tr_input_html_tags(_page, RODT_ATTR("Publish Frequency:"), RODT_ATTR("pfrq"), _publish_freq);

#else

    concat_tr_input_html_tags(_page, RODT_ATTR("Publish Frequency:"), RODT_ATTR("pfrq"), _publish_freq, HTML_INPUT_TAG_DEFAULT_MAXLENGTH, HTML_INPUT_TEXT_TAG_TYPE, false, true);

#endif

    _topic_name[0] = 's';
    _qos_name[0] = 's';
    concat_tr_heading_html_tags(_page, RODT_ATTR("Subscribe Topics"), 3, RODT_ATTR("2"));
    for (uint8_t i = 0; i < MQTT_MAX_SUBSCRIBE_TOPIC; i++)
    {

      _topic_label[5] = (0x30 + i);
      _topic_name[4] = (0x30 + i);
      _qos_label[3] = (0x30 + i);
      _qos_name[4] = (0x30 + i);

#ifdef ALLOW_MQTT_CONFIG_MODIFICATION

      concat_tr_input_html_tags(_page, _topic_label, _topic_name, _mqtt_pubsub_configs.subscribe_topics[i].topic, MQTT_TOPIC_BUF_SIZE - 1);
      concat_tr_select_html_tags(_page, _qos_label, _qos_name, _qos_options, 3, _mqtt_pubsub_configs.subscribe_topics[i].qos);

#else

      concat_tr_input_html_tags(_page, _topic_label, _topic_name, _mqtt_pubsub_configs.subscribe_topics[i].topic, MQTT_TOPIC_BUF_SIZE - 1, HTML_INPUT_TEXT_TAG_TYPE, false, true);
      concat_tr_select_html_tags(_page, _qos_label, _qos_name, _qos_options, 3, _mqtt_pubsub_configs.subscribe_topics[i].qos, 0, true);

#endif
    }

#ifdef ALLOW_MQTT_CONFIG_MODIFICATION
    strcat_ro(_page, WEB_SERVER_WIFI_CONFIG_PAGE_BOTTOM);
#endif

    if (_enable_flash)
      concat_flash_message_div(_page, HTML_SUCCESS_FLASH, ALERT_SUCCESS);
    strcat_ro(_page, WEB_SERVER_FOOTER_HTML);
  }

  /**
   * build and send mqtt pubsub config page.
   * when posted, get mqtt pubsub configs from client and set them in database.
   */
  void handleMqttPubSubConfigRoute(void)
  {
    LogI("Handling Mqtt LWT Config route\n");

    if (nullptr == this->m_web_resource ||
        nullptr == this->m_web_resource->m_db_conn ||
        nullptr == this->m_web_resource->m_server)
    {
      return;
    }

    bool _is_posted = false;

#ifdef ALLOW_MQTT_CONFIG_MODIFICATION
    if (this->m_web_resource->m_server->hasArg("ptpc0") && this->m_web_resource->m_server->hasArg("pqos0"))
    {

      mqtt_pubsub_config_table *_mqtt_pubsub_configs = new mqtt_pubsub_config_table;
      memset((void *)_mqtt_pubsub_configs, 0, sizeof(mqtt_pubsub_config_table));

      LogI("\nSubmitted info :\n\nPublish Topics\n");

      char _topic_name[10];
      memset(_topic_name, 0, 10);
      strcpy(_topic_name, "ptpc0");
      char _qos_name[10];
      memset(_qos_name, 0, 10);
      strcpy(_qos_name, "pqos0");
      char _retain_name[10];
      memset(_retain_name, 0, 10);
      strcpy(_retain_name, "prtn0");

      for (uint8_t i = 0; i < MQTT_MAX_PUBLISH_TOPIC; i++)
      {

        _topic_name[4] = (0x30 + i);
        _qos_name[4] = (0x30 + i);
        _retain_name[4] = (0x30 + i);

        pdiutil::string _topic = this->m_web_resource->m_server->arg(_topic_name);
        pdiutil::string _qos = this->m_web_resource->m_server->arg(_qos_name);
        pdiutil::string _retain = this->m_web_resource->m_server->arg(_retain_name);

        strncpy(_mqtt_pubsub_configs->publish_topics[i].topic, _topic.c_str(), _topic.size());
        _mqtt_pubsub_configs->publish_topics[i].qos = StringToUint8(_qos.c_str());
        _mqtt_pubsub_configs->publish_topics[i].retain = (int)(_retain == "retain");

        LogFmtI("%d : Topic(%s), Qos(%s), Retain(%s)\n", i, _topic.c_str(),_qos.c_str(),_retain.c_str());
      }

      LogI("\nSubscribe Topics\n");

      _topic_name[0] = 's';
      _qos_name[0] = 's';
      for (uint8_t i = 0; i < MQTT_MAX_SUBSCRIBE_TOPIC; i++)
      {

        _topic_name[4] = (0x30 + i);
        _qos_name[4] = (0x30 + i);

        pdiutil::string _topic = this->m_web_resource->m_server->arg(_topic_name);
        pdiutil::string _qos = this->m_web_resource->m_server->arg(_qos_name);

        strncpy(_mqtt_pubsub_configs->subscribe_topics[i].topic, _topic.c_str(), _topic.size());
        _mqtt_pubsub_configs->subscribe_topics[i].qos = StringToUint8(_qos.c_str());

        LogFmtI("%d : Topic(%s), Qos(%s), Retain(%s)\n", i, _topic.c_str(),_qos.c_str());
      }
      _mqtt_pubsub_configs->publish_frequency = StringToUint16(this->m_web_resource->m_server->arg("pfrq").c_str());

      this->m_web_resource->m_db_conn->set_mqtt_pubsub_config_table(_mqtt_pubsub_configs);

      delete _mqtt_pubsub_configs;
      _is_posted = true;
    }
#endif

    char *_page = new char[PAGE_HTML_MAX_SIZE];
    this->build_mqtt_pubsub_config_html(_page, _is_posted);

    this->m_web_resource->m_server->send(HTTP_OK, TEXT_HTML_CONTENT, _page);
    delete[] _page;
    if (_is_posted)
    {
      __mqtt_service.handleMqttConfigChange(MQTT_PUBSUB_CONFIG);
    }
  }
};

#endif
