/*************************** Email Config page ********************************
This file is part of the Ewings Esp Stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st June 2019
******************************************************************************/
#ifndef _EMAIL_CONFIG_H_
#define _EMAIL_CONFIG_H_

#include "Common.h"

/**
 * email configurations
 */

#define DEFAULT_SENDING_DOMAIN_MAX_SIZE   50
#define DEFAULT_MAIL_HOST_MAX_SIZE        DEFAULT_SENDING_DOMAIN_MAX_SIZE
#define DEFAULT_MAIL_USERNAME_MAX_SIZE    100
#define DEFAULT_MAIL_PASSWORD_MAX_SIZE    DEFAULT_MAIL_HOST_MAX_SIZE
#define DEFAULT_MAIL_SUBJECT_MAX_SIZE     DEFAULT_MAIL_HOST_MAX_SIZE
#define DEFAULT_MAIL_FROM_MAX_SIZE        DEFAULT_MAIL_HOST_MAX_SIZE
#define DEFAULT_MAIL_FROM_NAME_MAX_SIZE   DEFAULT_MAIL_HOST_MAX_SIZE
#define DEFAULT_MAIL_TO_MAX_SIZE          DEFAULT_MAIL_FROM_MAX_SIZE


#define DEFAULT_SENDING_DOMAIN            "www.example.com"
#define DEFAULT_MAIL_HOST                 "sandbox.smtp.mailtrap.io"
#define DEFAULT_MAIL_PORT                 2525
#define DEFAULT_MAIL_USERNAME             "b3ffe50b2430a0"
#define DEFAULT_MAIL_PASSWORD             "a3c7845387ed29"
#define DEFAULT_MAIL_FROM                 "surajinamdar151@gmail.com"
#define DEFAULT_MAIL_TO                   DEFAULT_MAIL_FROM
#define DEFAULT_MAIL_SUBJECT              "device status"
#define DEFAULT_MAIL_FREQUENCY            300
#define DEFAULT_MAIL_FROM_NAME            "esp8266"
// #define DEFAULT_MAIL_ENCRYPTION           "tls"


struct email_config {

  // Default Constructor
  email_config(){
    clear();
  }

  // Clear members method
  void clear(){
    memset(sending_domain, 0, DEFAULT_SENDING_DOMAIN_MAX_SIZE);
    memset(mail_host, 0, DEFAULT_MAIL_HOST_MAX_SIZE);
    memset(mail_username, 0, DEFAULT_MAIL_USERNAME_MAX_SIZE);
    memset(mail_password, 0, DEFAULT_MAIL_PASSWORD_MAX_SIZE);
    memset(mail_from, 0, DEFAULT_MAIL_FROM_MAX_SIZE);
    memset(mail_from_name, 0, DEFAULT_MAIL_FROM_NAME_MAX_SIZE);
    memset(mail_to, 0, DEFAULT_MAIL_TO_MAX_SIZE);
    memset(mail_subject, 0, DEFAULT_MAIL_SUBJECT_MAX_SIZE);

    memcpy(sending_domain, DEFAULT_SENDING_DOMAIN, sizeof(DEFAULT_SENDING_DOMAIN));
    memcpy(mail_host, DEFAULT_MAIL_HOST, sizeof(DEFAULT_MAIL_HOST));
    mail_port = DEFAULT_MAIL_PORT;
    memcpy(mail_username, DEFAULT_MAIL_USERNAME, sizeof(DEFAULT_MAIL_USERNAME));
    memcpy(mail_password, DEFAULT_MAIL_PASSWORD, sizeof(DEFAULT_MAIL_PASSWORD));
    memcpy(mail_from, DEFAULT_MAIL_FROM, sizeof(DEFAULT_MAIL_FROM));
    memcpy(mail_from_name, DEFAULT_MAIL_FROM_NAME, sizeof(DEFAULT_MAIL_FROM_NAME));
    memcpy(mail_to, DEFAULT_MAIL_TO, sizeof(DEFAULT_MAIL_TO));
    memcpy(mail_subject, DEFAULT_MAIL_SUBJECT, sizeof(DEFAULT_MAIL_SUBJECT));
    mail_frequency = DEFAULT_MAIL_FREQUENCY;
  }

  char sending_domain[DEFAULT_SENDING_DOMAIN_MAX_SIZE];
  char mail_host[DEFAULT_MAIL_HOST_MAX_SIZE];
  uint16_t mail_port;
  char mail_username[DEFAULT_MAIL_USERNAME_MAX_SIZE];
  char mail_password[DEFAULT_MAIL_PASSWORD_MAX_SIZE];
  char mail_from[DEFAULT_MAIL_FROM_MAX_SIZE];
  char mail_from_name[DEFAULT_MAIL_FROM_NAME_MAX_SIZE];
  char mail_to[DEFAULT_MAIL_TO_MAX_SIZE];
  char mail_subject[DEFAULT_MAIL_SUBJECT_MAX_SIZE];
  uint16_t mail_frequency;
};


// const email_config PROGMEM _email_config_defaults = {
//   DEFAULT_SENDING_DOMAIN, DEFAULT_MAIL_HOST, DEFAULT_MAIL_PORT, DEFAULT_MAIL_USERNAME, DEFAULT_MAIL_PASSWORD,
//   DEFAULT_MAIL_FROM, DEFAULT_MAIL_FROM_NAME, DEFAULT_MAIL_TO, DEFAULT_MAIL_SUBJECT, DEFAULT_MAIL_FREQUENCY
// };

const int email_config_size = sizeof(email_config) + 5;

using email_config_table = email_config;

#endif
