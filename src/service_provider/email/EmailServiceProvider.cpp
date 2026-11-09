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
#include <helpers/FeatureConfigFiles.h>
#ifdef ENABLE_HTTP_CLIENT
#include <transports/http/HTTPClient.h>
#endif

/**
 * EmailServiceProvider constructor.
 */
EmailServiceProvider::EmailServiceProvider() : m_mail_handler_cb_id(0),
                                               m_client(nullptr),
                                               m_mail_client(nullptr),
                                               m_mail_config(nullptr),
                                               m_mail_done(nullptr),
                                               m_mail_step_cb_id(0),
                                               m_mail_step(EMAIL_STEP_IDLE),
                                               ServiceProvider(SERVICE_EMAIL, RODT_ATTR("Email"))
{
}

/**
 * EmailServiceProvider destructor
 */
EmailServiceProvider::~EmailServiceProvider()
{
  this->finishMail(false);
  this->m_client = nullptr;
}

/**
 * Init email service. A client given here carries every mail; without one
 * each mail opens a client of its own for as long as it goes out.
 *
 * @param iClientInterface*	      _client
 */
bool EmailServiceProvider::initService(void *arg)
{
  this->finishMail(false);
  this->m_client = static_cast<iClientInterface*>(arg);

  this->m_mail_handler_cb_id = 0;

#ifdef ENABLE_EMAIL_CONFIG_FILE
  syncEmailConfigFile();
#endif

  return ServiceProvider::initService(arg);
}

/**
 * Ends a mail still going out when the service stops, and forgets the
 * periodic mail's task.
 */
void EmailServiceProvider::resetServiceState()
{
  this->m_mail_step_cb_id = 0;
  this->finishMail(false);
  this->m_mail_handler_cb_id = 0;
}

/**
 * handle emails
 *
 */
void EmailServiceProvider::handleEmail()
{
  this->m_mail_handler_cb_id = this->serviceUpdateInterval(
      this->m_mail_handler_cb_id,
      [&]()
      {
        if (EMAIL_STEP_IDLE != this->m_mail_step)
        {
          return;
        }

        LogI("Handling email\n");

        pdiutil::string *_payload = pdiutil::safe_new<pdiutil::string>();

        if (nullptr != _payload)
        {

#ifdef ENABLE_GPIO_SERVICE

          __gpio_service.appendGpioJsonPayload(*_payload);
#endif

          *_payload += CHARPTR_WRAP("\n\nRegards\n");
          *_payload += __i_dvc_ctrl.getDeviceMac().c_str();

          this->sendMail(*_payload, [this](bool sent)
          {
            if (sent && 0 != this->m_mail_handler_cb_id)
            {
              __task_scheduler.clearInterval(this->m_mail_handler_cb_id);
              this->m_mail_handler_cb_id = 0;
            }
          });

          pdiutil::safe_delete(_payload);
        }
      },
      180000, DEFAULT_TASK_PRIORITY, 0, -1);
}

/**
 * Starts sending a mail and returns at once: true when it was taken, false
 * when it was refused. done, when given, is called with whether the server
 * took the mail once the last step is answered.
 */
bool EmailServiceProvider::sendMail(pdiutil::string &mail_body, CallBackBoolArgFn done)
{
  return this->startMail(mail_body.c_str(), (uint32_t)mail_body.size(), done);
}

bool EmailServiceProvider::sendMail(char *mail_body, CallBackBoolArgFn done)
{
  return this->startMail(mail_body, (nullptr != mail_body) ? (uint32_t)strlen(mail_body) : 0, done);
}

bool EmailServiceProvider::sendMail(const char * mail_body, CallBackBoolArgFn done)
{
  return this->startMail(mail_body, (nullptr != mail_body) ? (uint32_t)strlen(mail_body) : 0, done);
}

/**
 * True when the settings name a server, a domain, a sender and a recipient.
 * Missing settings are logged.
 */
bool EmailServiceProvider::canSend(email_config_table &_email_config)
{
  if (0 == strlen(_email_config.mail_host) || 0 == strlen(_email_config.sending_domain) ||
      0 == strlen(_email_config.mail_from) || 0 == strlen(_email_config.mail_to))
  {
    SysLogE("Email: not sent, the mail server, domain, sender or recipient is not set\n");
    return false;
  }
  return true;
}

/**
 * Takes a copy of the mail and its settings, connects, and leaves the rest of
 * the conversation to stepMail.
 */
bool EmailServiceProvider::startMail(const char *body, uint32_t size, CallBackBoolArgFn done)
{
  if (EMAIL_STEP_IDLE != this->m_mail_step)
  {
    LogW("Email: not sent, a mail is already going out\n");
    return false;
  }

  this->m_mail_config = pdiutil::safe_new<email_config_table>();
  if (nullptr == this->m_mail_config)
  {
    SysLogE("Email: not sent, no memory for the mail\n");
    return false;
  }
  __database_service.get_email_config_table(this->m_mail_config);

  if (!this->canSend(*this->m_mail_config))
  {
    this->finishMail(false);
    return false;
  }

  if (nullptr != body && size > 0)
  {
    this->m_mail_body.assign(body, size);
  }
  this->m_mail_client = (nullptr != this->m_client) ? this->m_client : __i_instance.getNewTcpClientInstance();

  if (this->m_mail_body.size() != ((nullptr != body) ? size : 0) || nullptr == this->m_mail_client)
  {
    SysLogE("Email: not sent, no memory for the mail\n");
    this->finishMail(false);
    return false;
  }

  if (!this->m_smtp.begin(this->m_mail_client, this->m_mail_config->mail_host, this->m_mail_config->mail_port))
  {
    SysLogE("Email: not sent, no ready answer from %s:%u\n", this->m_mail_config->mail_host, (unsigned)this->m_mail_config->mail_port);
    this->finishMail(false);
    return false;
  }

  this->m_mail_step = EMAIL_STEP_GREETING;
  this->m_mail_done = done;
  this->m_mail_step_cb_id = this->serviceSetInterval([&]()
  {
    this->stepMail();
  }, EMAIL_STEP_INTERVAL_MS, __i_dvc_ctrl.millis_now());

  if (this->m_mail_step_cb_id <= 0)
  {
    SysLogE("Email: not sent, no task to send it with\n");
    this->m_mail_done = nullptr;
    this->finishMail(false);
    return false;
  }
  return true;
}

/**
 * Reads the reply to the step in flight and, once it is complete, sends the
 * next step or ends the mail with the step that was refused logged.
 */
void EmailServiceProvider::stepMail()
{
  smtp_command_status status = this->m_smtp.pollResponse();
  if (SMTP_RESPONSE_STARTING == status || SMTP_RESPONSE_WAITING == status)
  {
    return;
  }

  email_config_table &config = *this->m_mail_config;
  int16_t code = this->m_smtp.lastResponseCode();
  bool answered = (SMTP_RESPONSE_FINISHED == status);

  switch (this->m_mail_step)
  {
    case EMAIL_STEP_GREETING:
      if (answered && SMTP_STATUS_SERVER_READY == code)
      {
        this->m_smtp.startHello(config.sending_domain);
        this->m_mail_step = EMAIL_STEP_HELLO;
        return;
      }
      SysLogE("Email: not sent, no ready answer from %s:%u\n", config.mail_host, (unsigned)config.mail_port);
      break;

    case EMAIL_STEP_HELLO:
      if (answered && SMTP_STATUS_ACTION_COMPLETED == code)
      {
        if (strlen(config.mail_username) > 0)
        {
          this->m_smtp.startAuthLogin();
          this->m_mail_step = EMAIL_STEP_AUTH;
        }
        else
        {
          this->m_smtp.startFrom(config.mail_from);
          this->m_mail_step = EMAIL_STEP_FROM;
        }
        return;
      }
      SysLogE("Email: not sent, the server refused the greeting: %d\n", (int)code);
      break;

    case EMAIL_STEP_AUTH:
    case EMAIL_STEP_AUTH_USER:
      if (answered && SMTP_STATUS_AUTH_CONTINUE == code)
      {
        if (EMAIL_STEP_AUTH == this->m_mail_step)
        {
          this->m_smtp.startAuthUser(config.mail_username);
          this->m_mail_step = EMAIL_STEP_AUTH_USER;
        }
        else
        {
          this->m_smtp.startAuthPass(config.mail_password);
          this->m_mail_step = EMAIL_STEP_AUTH_PASS;
        }
        return;
      }
      SysLogE("Email: not sent, the server refused the login: %d\n", (int)code);
      break;

    case EMAIL_STEP_AUTH_PASS:
      if (answered && SMTP_STATUS_AUTH_SUCCEEDED == code)
      {
        this->m_smtp.startFrom(config.mail_from);
        this->m_mail_step = EMAIL_STEP_FROM;
        return;
      }
      SysLogE("Email: not sent, the server refused the login: %d\n", (int)code);
      break;

    case EMAIL_STEP_FROM:
      if (answered && SMTP_STATUS_ACTION_COMPLETED == code)
      {
        this->m_smtp.startTo(config.mail_to);
        this->m_mail_step = EMAIL_STEP_TO;
        return;
      }
      SysLogE("Email: not sent, the server refused the sender: %d\n", (int)code);
      break;

    case EMAIL_STEP_TO:
      if (answered && SMTP_STATUS_ACTION_COMPLETED == code)
      {
        this->m_smtp.startDataCommand();
        this->m_mail_step = EMAIL_STEP_DATA;
        return;
      }
      SysLogE("Email: not sent, the server refused the recipient: %d\n", (int)code);
      break;

    case EMAIL_STEP_DATA:
      if (answered && SMTP_STATUS_START_MAIL_INPUT == code)
      {
        this->m_smtp.startDataBody(config.mail_from_name, config.mail_to, config.mail_subject,
                                   this->m_mail_body.c_str(), (uint32_t)this->m_mail_body.size());
        this->m_mail_step = EMAIL_STEP_BODY;
        return;
      }
      SysLogE("Email: not sent, the server refused the message: %d\n", (int)code);
      break;

    case EMAIL_STEP_BODY:
      if (answered && SMTP_STATUS_ACTION_COMPLETED == code)
      {
        this->m_smtp.startQuit();
        this->m_mail_step = EMAIL_STEP_QUIT;
        return;
      }
      SysLogE("Email: not sent, the server did not take the message: %d\n", (int)code);
      break;

    case EMAIL_STEP_QUIT:
      if (answered && SMTP_STATUS_SERVER_ENDING_CON == code)
      {
        this->finishMail(true);
        return;
      }
      SysLogE("Email: the message was taken but the server did not end the session: %d\n", (int)code);
      break;

    default:
      break;
  }

  this->finishMail(false);
}

/**
 * Closes the connection, lets go of the mail and tells its sender the result.
 */
void EmailServiceProvider::finishMail(bool sent)
{
  if (this->m_mail_step_cb_id > 0)
  {
    __task_scheduler.clearInterval(this->m_mail_step_cb_id);
  }
  this->m_mail_step_cb_id = 0;

  this->m_smtp.end();
  if (nullptr != this->m_mail_client && this->m_mail_client != this->m_client)
  {
    pdiutil::safe_delete(this->m_mail_client);
  }
  this->m_mail_client = nullptr;

  if (nullptr != this->m_mail_config)
  {
    _ClearObject(this->m_mail_config);
    pdiutil::safe_delete(this->m_mail_config);
  }
  this->m_mail_body = pdiutil::string();

  bool going = (EMAIL_STEP_IDLE != this->m_mail_step);
  this->m_mail_step = EMAIL_STEP_IDLE;

  CallBackBoolArgFn done = this->m_mail_done;
  this->m_mail_done = nullptr;
  if (going && done)
  {
    done(sent);
  }
}

/**
 * print Email configs to terminal
 */
void EmailServiceProvider::printConfigToTerminal(iTerminalInterface *terminal)
{
  if( nullptr != terminal ){

    email_config_table _email_config;
    __database_service.get_email_config_table(&_email_config);

    terminal->writeln();
    terminal->writeln_ro(RODT_ATTR("Email Configs :"));

    terminal->write(_email_config.sending_domain); terminal->write_ro(RODT_ATTR("\t"));
    terminal->write(_email_config.mail_host); terminal->write_ro(RODT_ATTR("\t"));
    terminal->write((int32_t)_email_config.mail_port); terminal->write_ro(RODT_ATTR("\t"));
    terminal->write(_email_config.mail_username); terminal->write_ro(RODT_ATTR("\t"));
    terminal->writeln_ro(0 != _email_config.mail_password[0] ? RODT_ATTR("********") : RODT_ATTR("-"));

    terminal->write(_email_config.mail_from); terminal->write_ro(RODT_ATTR("\t"));
    terminal->write(_email_config.mail_from_name); terminal->write_ro(RODT_ATTR("\t"));
    terminal->write(_email_config.mail_to); terminal->write_ro(RODT_ATTR("\t"));
    terminal->write(_email_config.mail_subject); terminal->write_ro(RODT_ATTR("\t"));
    terminal->writeln((int32_t)_email_config.mail_frequency);
  }
}

EmailServiceProvider __email_service;

#endif
