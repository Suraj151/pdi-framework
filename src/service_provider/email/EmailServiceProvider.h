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

  /**
   * Starts sending a mail and returns at once: true when it was taken, false
   * when it was refused. done, when given, is called with whether the server
   * took the mail once the last step is answered.
   */
  bool sendMail(pdiutil::string &mail_body, CallBackBoolArgFn done = nullptr);
  bool sendMail(char *mail_body, CallBackBoolArgFn done = nullptr);
  bool sendMail(const char * mail_body, CallBackBoolArgFn done = nullptr);
  // template <typename T> bool sendMail( T mail_body );
  void handleEmail(void);

  /**
   * Ends a mail still going out when the service stops, and forgets the
   * periodic mail's task.
   */
  void resetServiceState() override;
  void printConfigToTerminal(iTerminalInterface *terminal) override;

  /**
   * @var	int|0 m_mail_handler_cb_id
   */
  pdiutil::task_id_t m_mail_handler_cb_id;

protected:
  /**
   * True when the settings name a server, a domain, a sender and a recipient.
   * Missing settings are logged.
   */
  bool canSend(email_config_table &_email_config);

  /**
   * Takes a copy of the mail and its settings, connects, and leaves the rest of
   * the conversation to stepMail.
   */
  bool startMail(const char *body, uint32_t size, CallBackBoolArgFn done);

  /**
   * Reads the reply to the step in flight and, once it is complete, sends the
   * next step or ends the mail with the step that was refused logged.
   */
  void stepMail(void);

  /**
   * Closes the connection, lets go of the mail and tells its sender the result.
   */
  void finishMail(bool sent);

  iClientInterface *m_client;
  iClientInterface *m_mail_client;
  email_config_table *m_mail_config;
  pdiutil::string m_mail_body;
  CallBackBoolArgFn m_mail_done;
  pdiutil::task_id_t m_mail_step_cb_id;
  email_step_t m_mail_step;
  SMTPClient m_smtp;
};

extern EmailServiceProvider __email_service;

#endif
