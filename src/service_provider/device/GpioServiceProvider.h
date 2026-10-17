/****************************** Gpio service **********************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st June 2019
******************************************************************************/

#ifndef _GPIO_SERVICE_PROVIDER_H_
#define _GPIO_SERVICE_PROVIDER_H_

#include <service_provider/ServiceProvider.h>
#include <service_provider/database/DatabaseServiceProvider.h>

#ifdef ENABLE_EMAIL_SERVICE
#include <service_provider/email/EmailServiceProvider.h>
#endif

#ifdef ENABLE_HTTP_CLIENT
#include <transports/http/HTTPClient.h>
#endif

/**
 * GpioServiceProvider class
 */
class GpioServiceProvider : public ServiceProvider
{

public:
  /**
   * GpioServiceProvider constructor.
   */
  GpioServiceProvider();
  /**
   * GpioServiceProvider destructor
   */
  ~GpioServiceProvider();

  bool initService(void *arg = nullptr) override;

  /**
   * Forget the http post task id and ask for the pin table to be reloaded, the
   * way a service that has never run yet asks for it.
   */
  void resetServiceState() override;

  /**
   * Needs its pin configuration before it can come up.
   */
  uint8_t getServiceDependencies(service_t *_out, uint8_t _max) const override {
    uint8_t _n = 0;
    if( _max > _n ) _out[_n++] = SERVICE_DATABASE;
    return _n;
  }

  void enable_update_gpio_table_from_copy(void);

  /**
   * Sets a pin's mode in the working copy when it is one the service knows,
   * leaving the pin as it was otherwise.
   */
  bool setGpioModeIfValid(uint8_t _pin, uint16_t _mode);

  /**
   * Sets a pin's value in the working copy when its mode can hold it, leaving
   * the pin as it was otherwise.
   */
  bool setGpioValueIfValid(uint8_t _pin, uint16_t _value);

  bool isAllowedGpioPin(uint8_t _pin, pdiutil::vector<pdiutil::string> *allowedlist);
  void appendGpioJsonPayload(pdiutil::string &_payload, bool isEventPost = false, pdiutil::vector<pdiutil::string> *allowedlist = nullptr);
  void applyGpioJsonPayload(char *_payload, uint16_t _payload_length, pdiutil::vector<pdiutil::string> *allowedlist = nullptr);
  #ifndef ENABLE_GPIO_BASIC_ONLY
  void applyGpioEventJsonPayload(char *_payload, uint16_t _payload_length, pdiutil::vector<pdiutil::string> *allowedlist = nullptr);
  #endif
  #ifndef ENABLE_GPIO_BASIC_ONLY
  void setHttpHost(const char* _host);
  #endif
  #ifdef ENABLE_EMAIL_SERVICE
  bool handleGpioEventOverEmail(void);
#endif
  void handleGpioOperations(void);
  void handleGpioModes(int _gpio_config_type = GPIO_MODE_CONFIG);
#ifdef ENABLE_HTTP_CLIENT
  bool handleGpioHttpRequest(bool isEventPost = false);
#endif
  void printConfigToTerminal(iTerminalInterface *terminal) override;

  /**
   * @var gpio_config_table m_gpio_config_copy
   */
  gpio_config_table m_gpio_config_copy;
  /**
   * @var	int|0 m_gpio_http_request_cb_id
   */
  pdiutil::task_id_t m_gpio_http_request_cb_id;
  /**
   * @var	bool|true m_update_gpio_table_from_copy
   */
  bool m_update_gpio_table_from_copy;
  /**
   * @var	int8_t|-1 m_pending_event_post
   */
  int8_t m_pending_event_post;

protected:

#ifdef ENABLE_HTTP_CLIENT
  /**
   * @var	Http_Client  *m_http_client
   */
  Http_Client *m_http_client;
#endif

  iGpioBlinkerInterface *m_digital_blinker[MAX_DIGITAL_GPIO_PINS];
};

extern GpioServiceProvider __gpio_service;

#endif
