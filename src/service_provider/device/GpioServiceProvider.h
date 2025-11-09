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

#define GPIO_PAYLOAD_DATA_KEY "data"
#define GPIO_PAYLOAD_MODE_KEY "mode"
#define GPIO_PAYLOAD_VALUE_KEY "val"
#define GPIO_PAYLOAD_MAC_KEY "mac"
#define GPIO_ALERT_PIN_KEY "alrtkey"

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
  void enable_update_gpio_table_from_copy(void);
  bool isAllowedGpioPin(uint8_t _pin, pdiutil::vector<pdiutil::string> *allowedlist);
  void appendGpioJsonPayload(pdiutil::string &_payload, bool isAlertPost = false, pdiutil::vector<pdiutil::string> *allowedlist = nullptr);
  void applyGpioJsonPayload(char *_payload, uint16_t _payload_length, pdiutil::vector<pdiutil::string> *allowedlist = nullptr);
#ifdef ENABLE_EMAIL_SERVICE
  bool handleGpioEmailAlert(void);
#endif
  void handleGpioOperations(void);
  void handleGpioModes(int _gpio_config_type = GPIO_MODE_CONFIG);
#ifdef ENABLE_HTTP_CLIENT
  bool handleGpioHttpRequest(bool isAlertPost = false);
#endif
  void printConfigToTerminal(iTerminalInterface *terminal) override;


  /**
   * @var gpio_config_table m_gpio_config_copy
   */
  gpio_config_table m_gpio_config_copy;
  /**
   * @var	int|0 m_gpio_http_request_cb_id
   */
  int m_gpio_http_request_cb_id;
  /**
   * @var	bool|true m_update_gpio_table_from_copy
   */
  bool m_update_gpio_table_from_copy;

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
