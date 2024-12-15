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
#include <service_provider/EmailServiceProvider.h>
#include <service_provider/DatabaseServiceProvider.h>

#include <transports/http/HTTPClient.h>

#define GPIO_PAYLOAD_DATA_KEY "data"
#define GPIO_PAYLOAD_MODE_KEY "mode"
#define GPIO_PAYLOAD_VALUE_KEY "val"
#define GPIO_PAYLOAD_MAC_KEY "mac_id"
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

  void begin(iClientInterface *_client);
  void enable_update_gpio_table_from_copy(void);
  void appendGpioJsonPayload(pdiutil::string &_payload, bool isAlertPost = false);
  void applyGpioJsonPayload(char *_payload, uint16_t _payload_length);
#ifdef ENABLE_EMAIL_SERVICE
  bool handleGpioEmailAlert(void);
#endif
  void handleGpioOperations(void);
  void handleGpioModes(int _gpio_config_type = GPIO_MODE_CONFIG);
  bool handleGpioHttpRequest(bool isAlertPost = false);
  void printGpioConfigLogs(void);

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

  /**
   * @var	Http_Client  *m_http_client
   */
  Http_Client *m_http_client;

  iGpioBlinkerInterface *m_digital_blinker[MAX_DIGITAL_GPIO_PINS];
};

extern GpioServiceProvider __gpio_service;

#endif
