/***************************** Session Handler ********************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st June 2019
******************************************************************************/

#ifndef _EW_SERVER_SESSION_HANDLER_
#define _EW_SERVER_SESSION_HANDLER_

#include <webserver/resources/WebResource.h>

/**
 * @define	max buf size for client cookies.
 */
#define EW_COOKIE_BUFF_MAX_SIZE 60

/**
 * EwSessionHandler class
 */
class EwSessionHandler
{

public:
  /**
   * EwSessionHandler constructor
   */
  EwSessionHandler(void)
  {
  }

  /**
   * EwSessionHandler destructor
   */
  ~EwSessionHandler()
  {
  }

  /**
   * send inactive session to client. this will remove any login cache from client side.
   */
  void send_inactive_session_headers(void)
  {
    char _session_cookie[EW_COOKIE_BUFF_MAX_SIZE];
    this->build_session_cookie(_session_cookie, false, EW_COOKIE_BUFF_MAX_SIZE, true, 0);

    if (nullptr != __web_resource.m_server)
    {
      __web_resource.m_server->sendHeader("Cache-Control", "no-cache");
      __web_resource.m_server->sendHeader("Set-Cookie", _session_cookie);
    }
  }

  /**
   * build session cookies for login user with defined max age.
   *
   * @param	char*	_str
   * @param	bool  _stat
   * @param	int   _max_size
   * @param	bool|false	_enable_max_age
   * @param	uint32_t|SERVER_COOKIE_MAX_AGE  _max_age
   */
  void build_session_cookie(char *_str, bool _stat, int _max_size, bool _enable_max_age = false, uint32_t _max_age = SERVER_COOKIE_MAX_AGE)
  {

    if (nullptr == __web_resource.m_db_conn)
    {
      return;
    }

    login_credential_table _login_credentials;
    __web_resource.m_db_conn->get_login_credential_table(&_login_credentials);
    memset(_str, 0, _max_size);
    strcat(_str, _login_credentials.session_name);
    strcat(_str, _stat ? "=1" : "=0");
    if (_enable_max_age)
    {
      strcat(_str, ";Max-Age=");
      __appendUintToBuff(_str, "%08ld", _max_age, 8);
    }
  }

  /**
   * check whether client has active valid session
   *
   * @return  bool
   */
  bool has_active_session(void)
  {
    LogI("checking active session\n");

    if (nullptr == __web_resource.m_server)
    {
      return false;
    }

    if (__web_resource.m_server->hasHeader("Cookie"))
    {

      std::string cookie = __web_resource.m_server->header("Cookie");
      char _session_cookie[EW_COOKIE_BUFF_MAX_SIZE];
      this->build_session_cookie(_session_cookie, true, EW_COOKIE_BUFF_MAX_SIZE);

      LogFmtI("Found cookie: %s\n", cookie.c_str());

      if (cookie.find(_session_cookie) != std::string::npos)
      {
        LogI("active session found\n");
        return true;
      }
    }

    LogW("active session not found\n");
    return false;
  }

  /**
   * check whether client has inactive session
   *
   * @return  bool
   */
  bool has_inactive_session(void)
  {
    LogI("checking inactive session\n");

    if (nullptr == __web_resource.m_server)
    {
      false;
    }

    if (__web_resource.m_server->hasHeader("Cookie"))
    {
      std::string cookie = __web_resource.m_server->header("Cookie");
      char _session_cookie[EW_COOKIE_BUFF_MAX_SIZE];
      this->build_session_cookie(_session_cookie, false, EW_COOKIE_BUFF_MAX_SIZE);

      LogFmtI("Found cookie: %s\n", cookie.c_str());

      if (cookie.find(_session_cookie) != std::string::npos)
      {
        LogI("inactive session found\n");
        return true;
      }
    }

    LogI("inactive session not found\n");
    return false;
  }
};

#endif
