/******************************* Email service *********************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st June 2019
******************************************************************************/

#include <config/Config.h>

#if defined(ENABLE_EMAIL_SERVICE)

#include "EmailServiceProvider.h"

/**
 * EmailServiceProvider constructor.
 */
EmailServiceProvider::EmailServiceProvider() : m_mail_handler_cb_id(0),
                                               m_client(nullptr),
                                               ServiceProvider(SERVICE_EMAIL)
{
}

/**
 * EmailServiceProvider destructor
 */
EmailServiceProvider::~EmailServiceProvider()
{
  this->m_client = nullptr;
  this->m_smtp.end();
}

/**
 * begin email service with client
 *
 * @param iClientInterface*	      _client
 */
void EmailServiceProvider::begin(iClientInterface *_client)
{
  this->m_client = _client;
  this->m_mail_handler_cb_id = 0;
}

/**
 * handle emails
 *
 */
void EmailServiceProvider::handleEmail()
{
  this->m_mail_handler_cb_id = __task_scheduler.updateInterval(
      this->m_mail_handler_cb_id,
      [&]()
      {
        LogI("Handling email\n");

        pdiutil::string *_payload = new pdiutil::string();

        if (nullptr != _payload)
        {

#ifdef ENABLE_GPIO_SERVICE

          __gpio_service.appendGpioJsonPayload(*_payload);
#endif

          *_payload += "\n\nRegards\n";
          *_payload += __i_dvc_ctrl.getDeviceMac().c_str();

          if (this->sendMail(*_payload))
          {
            __task_scheduler.clearInterval(this->m_mail_handler_cb_id);
            this->m_mail_handler_cb_id = 0;
          }

          delete _payload;
        }
      },
      180000);
}

// /**
//  * This template send email and return the result of operation
//  *
//  * @param	  T*	mail_body
//  * @return  bool
//  */
// template <typename T> bool EmailServiceProvider::sendMail ( T mail_body ){
//
//   email_config_table _email_config;
//   __database_service.get_email_config_table(&_email_config);
//
//   bool ret = false;
//
//   if(
//     strlen(_email_config.mail_host) > 0 && strlen(_email_config.sending_domain) > 0 &&
//     strlen(_email_config.mail_from) > 0 && strlen(_email_config.mail_to) > 0
//   ){
//
//     ret = this->m_smtp.begin( this->m_client, _email_config.mail_host, _email_config.mail_port );
//     if( ret ){
//       ret = this->m_smtp.sendHello( _email_config.sending_domain );
//     }
//     if( ret ){
//       ret = this->m_smtp.sendAuthLogin( _email_config.mail_username, _email_config.mail_password );
//     }
//     if( ret ){
//       ret = this->m_smtp.sendFrom( _email_config.mail_from );
//     }
//     if( ret ){
//       ret = this->m_smtp.sendTo( _email_config.mail_to );
//     }
//     if( ret ){
//       ret = this->m_smtp.sendDataCommand();
//     }
//     if( ret ){
//       this->m_smtp.sendDataHeader( _email_config.mail_from_name, _email_config.mail_to, _email_config.mail_subject );
//       ret = this->m_smtp.sendDataBody( mail_body );
//     }
//     if( ret ){
//       ret = this->m_smtp.sendQuit();
//     }
//
//     this->m_smtp.end();
//   }
//
//   _ClearObject(&_email_config);
//   return ret;
// }

/**
 * send email and return the result of operation
 *
 * @param	  pdiutil::string*	mail_body
 * @return  bool
 */
bool EmailServiceProvider::sendMail(pdiutil::string &mail_body)
{

  email_config_table _email_config;
  __database_service.get_email_config_table(&_email_config);

  bool ret = false;

  if (
      strlen(_email_config.mail_host) > 0 && strlen(_email_config.sending_domain) > 0 &&
      strlen(_email_config.mail_from) > 0 && strlen(_email_config.mail_to) > 0)
  {
    ret = this->m_smtp.begin(this->m_client, _email_config.mail_host, _email_config.mail_port);
    if (ret)
    {
      ret = this->m_smtp.sendHello(_email_config.sending_domain);
    }
    if (ret)
    {
      ret = this->m_smtp.sendAuthLogin(_email_config.mail_username, _email_config.mail_password);
    }
    if (ret)
    {
      ret = this->m_smtp.sendFrom(_email_config.mail_from);
    }
    if (ret)
    {
      ret = this->m_smtp.sendTo(_email_config.mail_to);
    }
    if (ret)
    {
      ret = this->m_smtp.sendDataCommand();
    }
    if (ret)
    {
      this->m_smtp.sendDataHeader(_email_config.mail_from_name, _email_config.mail_to, _email_config.mail_subject);
      ret = this->m_smtp.sendDataBody(mail_body);
    }
    if (ret)
    {
      ret = this->m_smtp.sendQuit();
    }

    this->m_smtp.end();
  }

  _ClearObject(&_email_config);
  return ret;
}

/**
 * send email and return the result of operation
 *
 * @param	  char*	mail_body
 * @return  bool
 */
bool EmailServiceProvider::sendMail(char *mail_body)
{

  email_config_table _email_config;
  __database_service.get_email_config_table(&_email_config);

  bool ret = false;

  if (
      strlen(_email_config.mail_host) > 0 && strlen(_email_config.sending_domain) > 0 &&
      strlen(_email_config.mail_from) > 0 && strlen(_email_config.mail_to) > 0)
  {
    ret = this->m_smtp.begin(this->m_client, _email_config.mail_host, _email_config.mail_port);
    if (ret)
    {
      ret = this->m_smtp.sendHello(_email_config.sending_domain);
    }
    if (ret)
    {
      ret = this->m_smtp.sendAuthLogin(_email_config.mail_username, _email_config.mail_password);
    }
    if (ret)
    {
      ret = this->m_smtp.sendFrom(_email_config.mail_from);
    }
    if (ret)
    {
      ret = this->m_smtp.sendTo(_email_config.mail_to);
    }
    if (ret)
    {
      ret = this->m_smtp.sendDataCommand();
    }
    if (ret)
    {
      this->m_smtp.sendDataHeader(_email_config.mail_from_name, _email_config.mail_to, _email_config.mail_subject);
      ret = this->m_smtp.sendDataBody(mail_body);
    }
    if (ret)
    {
      ret = this->m_smtp.sendQuit();
    }

    this->m_smtp.end();
  }

  _ClearObject(&_email_config);
  return ret;
}

/**
 * send email and return the result of operation
 *
 * @param	  const char *	mail_body
 * @return  bool
 */
bool EmailServiceProvider::sendMail(const char * mail_body)
{

  email_config_table _email_config;
  __database_service.get_email_config_table(&_email_config);

  bool ret = false;

  if (
      strlen(_email_config.mail_host) > 0 && strlen(_email_config.sending_domain) > 0 &&
      strlen(_email_config.mail_from) > 0 && strlen(_email_config.mail_to) > 0)
  {
    ret = this->m_smtp.begin(this->m_client, _email_config.mail_host, _email_config.mail_port);
    if (ret)
    {
      ret = this->m_smtp.sendHello(_email_config.sending_domain);
    }
    if (ret)
    {
      ret = this->m_smtp.sendAuthLogin(_email_config.mail_username, _email_config.mail_password);
    }
    if (ret)
    {
      ret = this->m_smtp.sendFrom(_email_config.mail_from);
    }
    if (ret)
    {
      ret = this->m_smtp.sendTo(_email_config.mail_to);
    }
    if (ret)
    {
      ret = this->m_smtp.sendDataCommand();
    }
    if (ret)
    {
      this->m_smtp.sendDataHeader(_email_config.mail_from_name, _email_config.mail_to, _email_config.mail_subject);
      ret = this->m_smtp.sendDataBody(mail_body);
    }
    if (ret)
    {
      ret = this->m_smtp.sendQuit();
    }

    this->m_smtp.end();
  }

  _ClearObject(&_email_config);
  return ret;
}

/**
 * print Email configs to terminal
 */
void EmailServiceProvider::printConfigToTerminal(iTerminalInterface *terminal)
{
  if( nullptr != terminal ){

    email_config_table _email_config;
    __database_service.get_email_config_table(&_email_config);

    terminal->write_ro(RODT_ATTR("\nEmail Configs :\n"));

    terminal->write(_email_config.sending_domain); terminal->write(RODT_ATTR("\t"));
    terminal->write(_email_config.mail_host); terminal->write(RODT_ATTR("\t"));
    terminal->write(_email_config.mail_port); terminal->write(RODT_ATTR("\t"));
    terminal->write(_email_config.mail_username); terminal->write(RODT_ATTR("\t"));
    terminal->write(_email_config.mail_password); terminal->write(RODT_ATTR("\n"));

    terminal->write(_email_config.mail_from); terminal->write(RODT_ATTR("\t"));
    terminal->write(_email_config.mail_from_name); terminal->write(RODT_ATTR("\t"));
    terminal->write(_email_config.mail_to); terminal->write(RODT_ATTR("\t"));
    terminal->write(_email_config.mail_subject); terminal->write(RODT_ATTR("\t"));
    terminal->write(_email_config.mail_frequency); terminal->write(RODT_ATTR("\n"));
  }
}

EmailServiceProvider __email_service;

#endif
