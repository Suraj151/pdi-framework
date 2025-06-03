/***************************** Session Handler ********************************
This file is part of the PDI stack.

This is free software. You can redistribute it and/or modify it but without any
warranty.

The `SessionHandler.h` file defines the `EwSessionHandler` class, which provides
methods to manage user sessions for the web server. It includes functionality
to create, validate, and invalidate session cookies, ensuring secure and efficient
session handling.

Author          : Suraj I.
Created Date    : 1st June 2019
******************************************************************************/

#ifndef _WEB_SERVER_SESSION_HANDLER_
#define _WEB_SERVER_SESSION_HANDLER_

#include <webserver/resources/WebResource.h>

/**
 * @define EW_COOKIE_BUFF_MAX_SIZE
 * @brief Defines the maximum buffer size for client cookies.
 */
#define EW_COOKIE_BUFF_MAX_SIZE 60

/**
 * @class EwSessionHandler
 * @brief Handles session management for the web server.
 *
 * The `EwSessionHandler` class provides methods to manage user sessions,
 * including creating session cookies, validating active sessions, and
 * invalidating sessions.
 */
class EwSessionHandler
{

public:
  /**
   * @brief Constructor for the `EwSessionHandler` class.
   *
   * Initializes the session handler.
   */
  EwSessionHandler(void) {}

  /**
   * @brief Destructor for the `EwSessionHandler` class.
   *
   * Cleans up resources used by the session handler.
   */
  ~EwSessionHandler() {}

  /**
   * @brief Sends inactive session headers to the client.
   *
   * This method invalidates the session on the client side by sending
   * headers that remove any login cache.
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
   * @brief Builds a session cookie for the logged-in user.
   *
   * Generates a session cookie with the specified attributes, including
   * an optional maximum age.
   *
   * @param _str The buffer to store the session cookie.
   * @param _stat The session status (true for active, false for inactive).
   * @param _max_size The maximum size of the session cookie buffer.
   * @param _enable_max_age Whether to include the Max-Age attribute.
   * @param _max_age The maximum age of the session cookie (default: SERVER_COOKIE_MAX_AGE).
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
   * @brief Checks whether the client has an active session.
   *
   * Validates the presence of an active session cookie in the client's
   * request headers.
   *
   * @return `true` if an active session is found, `false` otherwise.
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
      pdiutil::string cookie = __web_resource.m_server->header("Cookie");
      char _session_cookie[EW_COOKIE_BUFF_MAX_SIZE];
      this->build_session_cookie(_session_cookie, true, EW_COOKIE_BUFF_MAX_SIZE);

      LogFmtI("Found cookie: %s\n", cookie.c_str());

      if (cookie.find(_session_cookie) != pdiutil::string::npos)
      {
        LogI("active session found\n");
        return true;
      }
    }

    LogW("active session not found\n");
    return false;
  }

  /**
   * @brief Checks whether the client has an inactive session.
   *
   * Validates the presence of an inactive session cookie in the client's
   * request headers.
   *
   * @return `true` if an inactive session is found, `false` otherwise.
   */
  bool has_inactive_session(void)
  {
    LogI("checking inactive session\n");

    if (nullptr == __web_resource.m_server)
    {
      return false;
    }

    if (__web_resource.m_server->hasHeader("Cookie"))
    {
      pdiutil::string cookie = __web_resource.m_server->header("Cookie");
      char _session_cookie[EW_COOKIE_BUFF_MAX_SIZE];
      this->build_session_cookie(_session_cookie, false, EW_COOKIE_BUFF_MAX_SIZE);

      LogFmtI("Found cookie: %s\n", cookie.c_str());

      if (cookie.find(_session_cookie) != pdiutil::string::npos)
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
