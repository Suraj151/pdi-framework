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
#include <service_provider/session/SessionManager.h>
#include <service_provider/user/UserStoreService.h>

/**
 * AuthServiceProvider constructor
 */
AuthServiceProvider::AuthServiceProvider(): m_initStatus(false), ServiceProvider(SERVICE_AUTH, RODT_ATTR("Auth"))
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
  if( nullptr == username || nullptr == password ) return false;

  bool ok = false;

#ifdef ENABLE_STORAGE_SERVICE
  if( __i_fs.isFileExist(USER_STORE_SHADOW_PATH) ){
    ok = __user_store_service.verifyPassword(username, password);
  }else
#endif
  {
    ok = ( m_initStatus &&
        __are_str_equals(username, m_login_credentials.username, LOGIN_CONFIGS_BUF_SIZE) &&
        __are_str_equals(password, m_login_credentials.password, LOGIN_CONFIGS_BUF_SIZE)
    );
  }

  if( ok ){
    m_lastVerifiedUsername = username;
  }
  return ok;
}

/**
 * set authorised status
 *
 * @param bool auth
 */
void AuthServiceProvider::setAuthorized(bool auth)
{
  session_t *s = SessionManager::current();
  if( nullptr == s ) return;
  s->m_isAuthorized = auth;
  if( auth ){
    if( !m_lastVerifiedUsername.empty() ){
      s->m_username = m_lastVerifiedUsername;
    }else{
      s->m_username = m_login_credentials.username;
    }
    s->m_loginAt = (uint32_t)__i_dvc_ctrl.millis_now();
    s->m_lastActivityAt = s->m_loginAt;

    s->m_uid = 0;
    s->m_gid = 0;
#ifdef ENABLE_STORAGE_SERVICE
    user_record_t rec;
    if( __user_store_service.findUserByName(s->m_username.c_str(), rec) ){
      s->m_uid = rec.m_uid;
      s->m_gid = rec.m_gid;
    }
    s->m_umask = FILE_UMASK_DEFAULT;
#endif
  }else{
    s->m_username.clear();
    s->m_loginAt = 0;
    s->m_lastActivityAt = 0;
    s->m_uid = 0;
    s->m_gid = 0;
#ifdef ENABLE_STORAGE_SERVICE
    s->m_umask = FILE_UMASK_DEFAULT;
#endif
    m_lastVerifiedUsername.clear();
  }
}

/**
 * get authorised status
 *
 * @return bool
 */
bool AuthServiceProvider::getAuthorized()
{
  session_t *s = SessionManager::current();
  return ( nullptr != s && s->m_isAuthorized );
}

/**
 * get username
 *
 * @return const char*
 */
const char *AuthServiceProvider::getUsername()
{
  session_t *s = SessionManager::current();
  if( nullptr != s && !s->m_username.empty() ){
    return s->m_username.c_str();
  }
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