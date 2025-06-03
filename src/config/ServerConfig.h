/*************************** Server Config page *******************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st June 2019
******************************************************************************/
#ifndef _SERVER_CONFIG_H_
#define _SERVER_CONFIG_H_

#include "Common.h"

#define DEFAULT_LOGIN_USERNAME     USER
#define DEFAULT_LOGIN_PASSWORD     PASSPHRASE

#ifdef ENABLE_HTTP_SERVER
#define SERVER_SESSION_NAME     "pdi_session"
#endif
#define SERVER_COOKIE_MAX_AGE   300

#define LOGIN_CONFIGS_BUF_SIZE 25

struct login_credential {

  // Default Constructor
  login_credential(){
    clear();
  }

  // Clear members method
  void clear(){
    memset(username, 0, LOGIN_CONFIGS_BUF_SIZE);
    memset(password, 0, LOGIN_CONFIGS_BUF_SIZE);
    #ifdef ENABLE_HTTP_SERVER
    memset(session_name, 0, LOGIN_CONFIGS_BUF_SIZE);
    #endif

    memcpy(username, DEFAULT_LOGIN_USERNAME, sizeof(DEFAULT_LOGIN_USERNAME));
    memcpy(password, DEFAULT_LOGIN_PASSWORD, sizeof(DEFAULT_LOGIN_PASSWORD));
    #ifdef ENABLE_HTTP_SERVER
    memcpy(session_name, SERVER_SESSION_NAME, sizeof(SERVER_SESSION_NAME));
    #endif

    cookie_max_age = SERVER_COOKIE_MAX_AGE;
  }

  char username[LOGIN_CONFIGS_BUF_SIZE];
  char password[LOGIN_CONFIGS_BUF_SIZE];
  #ifdef ENABLE_HTTP_SERVER
  char session_name[LOGIN_CONFIGS_BUF_SIZE];
  #endif
  uint16_t cookie_max_age;
};

// const login_credential PROGMEM _login_credential_defaults = {
//   DEFAULT_LOGIN_USERNAME,  DEFAULT_LOGIN_PASSWORD, SERVER_SESSION_NAME, SERVER_COOKIE_MAX_AGE
// };

const int login_credential_size = sizeof(login_credential) + 5;

using login_credential_table = login_credential;

#endif
