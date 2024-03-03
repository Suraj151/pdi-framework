/******************************* Email service *********************************
This file is part of the Ewings Esp Stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st June 2019
******************************************************************************/

#ifndef _EMAIL_SERVICE_PROVIDER_H_
#define _EMAIL_SERVICE_PROVIDER_H_

#include <service_provider/ServiceProvider.h>
#ifdef ENABLE_GPIO_SERVICE
#include <service_provider/GpioServiceProvider.h>
#endif
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

  void begin(iClientInterface *_client);
  bool sendMail(String &mail_body);
  bool sendMail(char *mail_body);
  bool sendMail(PGM_P mail_body);
  // template <typename T> bool sendMail( T mail_body );
  void handleEmail(void);
  void printEmailConfigLogs(void);

  /**
   * @var	int|0 m_mail_handler_cb_id
   */
  int m_mail_handler_cb_id;

protected:
  iClientInterface *m_client;
  SMTPClient m_smtp;
};

extern EmailServiceProvider __email_service;

#endif
