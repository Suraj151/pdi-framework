/******************************** Auth Services *******************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st June 2019
******************************************************************************/

#include <config/Config.h>

#if defined(ENABLE_AUTH_SERVICE)

#include "AuthServiceProvider.h"

/**
 * AuthServiceProvider constructor
 */
AuthServiceProvider::AuthServiceProvider(): m_initStatus(false), m_isAuthorized(false), ServiceProvider(SERVICE_AUTH, "Auth")
{
}

/**
 * AuthServiceProvider destructor
 */
AuthServiceProvider::~AuthServiceProvider()
{
}

/**
 * initialize the auth service
 *
 * @return bool status
 */
bool AuthServiceProvider::initService(void *arg)
{
  m_initStatus = __database_service.get_login_credential_table(&m_login_credentials);

  return (m_initStatus & ServiceProvider::initService(arg));
}

/**
 * handle auth check
 *
 * @param const char* username
 * @param const char* password
 */
bool AuthServiceProvider::isAuthorized(const char *username, const char *password)
{
  if( m_initStatus &&
      __are_str_equals(username, m_login_credentials.username, LOGIN_CONFIGS_BUF_SIZE) &&
      __are_str_equals(password, m_login_credentials.password, LOGIN_CONFIGS_BUF_SIZE) 
  ){
    m_isAuthorized = true;
  }else{
    return false;
  }
  return m_isAuthorized;
}

/**
 * set authorised status
 *
 * @param bool auth
 */
void AuthServiceProvider::setAuthorized(bool auth)
{
  m_isAuthorized = auth;
}

/**
 * get authorised status
 *
 * @return bool
 */
bool AuthServiceProvider::getAuthorized()
{
  return m_isAuthorized;
}

/**
 * get username
 *
 * @return const char*
 */
const char *AuthServiceProvider::getUsername()
{
    return m_login_credentials.username;
}

/**
 * print login configs to terminal
 */
void AuthServiceProvider::printConfigToTerminal(iTerminalInterface *terminal)
{
  if( nullptr != terminal && m_initStatus ){

    terminal->writeln();
    terminal->writeln_ro(RODT_ATTR("Login Configs :"));
    terminal->write(m_login_credentials.username); terminal->write_ro(RODT_ATTR("\t"));
    terminal->writeln(m_login_credentials.password);
  }
}


AuthServiceProvider __auth_service;

#endif