/******************************* Email service *********************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st June 2019
******************************************************************************/

#ifndef _EMAIL_SERVICE_PROVIDER_H_
#define _EMAIL_SERVICE_PROVIDER_H_

#include <service_provider/ServiceProvider.h>
#ifdef ENABLE_GPIO_SERVICE
#include <service_provider/device/GpioServiceProvider.h>
#endif
#include <service_provider/database/DatabaseServiceProvider.h>
#include <transports/smtp/SMTPClient.h>

/**
 * EmailServiceProvider class
 */
class EmailServiceProvider : public ServiceProvider
{

public:
  /**
   * EmailServiceProvider constructor.
   */
  EmailServiceProvider();
  /**
   * EmailServiceProvider destructor
   */
  ~EmailServiceProvider();

  bool initService(void *arg = nullptr) override;

  /**
   * Needs its configuration, and the network it sends over, before it can
   * come up.
   */
  uint8_t getServiceDependencies(service_t *_out, uint8_t _max) const override {
    uint8_t _n = 0;
    if( _max > _n ) _out[_n++] = SERVICE_DATABASE;
  #ifdef ENABLE_WIFI_SERVICE
    if( _max > _n ) _out[_n++] = SERVICE_WIFI;
  #endif
    return _n;
  }

  bool sendMail(pdiutil::string &mail_body);
  bool sendMail(char *mail_body);
  bool sendMail(const char * mail_body);
  // template <typename T> bool sendMail( T mail_body );
  void handleEmail(void);
  void printConfigToTerminal(iTerminalInterface *terminal) override;

  /**
   * @var	int|0 m_mail_handler_cb_id
   */
  pdiutil::task_id_t m_mail_handler_cb_id;

protected:
  /**
   * True when no http request holds the shared client this service sends over.
   */
  bool isClientFree();

  iClientInterface *m_client;
  SMTPClient m_smtp;
};

extern EmailServiceProvider __email_service;

#endif
