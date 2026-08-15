/***************************** Session Handler ********************************
This file is part of the PDI stack.

This is free software. You can redistribute it and/or modify it but without any
warranty.

The `SessionHandler.h` file defines the `EwSessionHandler` class, which binds an
HTTP request to a server side session held by `WebSessionManager`. The client
only ever holds an opaque random token, and the handler publishes the resolved
session through `SessionManager` so file operations run as the logged in user.

Author          : Suraj I.
Created Date    : 1st June 2019
******************************************************************************/

#ifndef _WEB_SERVER_SESSION_HANDLER_
#define _WEB_SERVER_SESSION_HANDLER_

#include <webserver/resources/WebResource.h>
#include <webserver/handlers/WebSessionManager.h>
#include <service_provider/session/SessionManager.h>

/**
 * @define EW_COOKIE_BUFF_MAX_SIZE
 * @brief Defines the maximum buffer size for client cookies.
 */
#define EW_COOKIE_BUFF_MAX_SIZE (LOGIN_CONFIGS_BUF_SIZE + WEB_SESSION_TOKEN_HEX_LEN + 72)

/**
 * @class EwSessionHandler
 * @brief Handles session management for the web server.
 *
 * The `EwSessionHandler` class resolves the session cookie of a request into a
 * server side session, issues new session cookies, and tears sessions down.
 */
class EwSessionHandler
{

public:
  /**
   * @brief Constructor for the `EwSessionHandler` class.
   */
  EwSessionHandler(void) : m_active_session(nullptr), m_previous_session(nullptr) {}

  /**
   * @brief Destructor for the `EwSessionHandler` class.
   */
  ~EwSessionHandler() {}

  /**
   * @brief Sends headers that clear the session cookie on the client.
   *
   * Also destroys the matching server side session so a captured cookie
   * cannot be replayed.
   */
  void send_inactive_session_headers(void)
  {
    if (nullptr != m_active_session)
    {
      __web_session_manager.destroy(m_active_session);
      m_active_session = nullptr;
    }

    release_request_session();

    if (nullptr != __web_resource.m_server)
    {
      char _session_cookie[EW_COOKIE_BUFF_MAX_SIZE];
      this->build_session_cookie(_session_cookie, nullptr, EW_COOKIE_BUFF_MAX_SIZE, true, 0);

      __web_resource.m_server->addHeader(CHARPTR_WRAP_RO(HTTP_HEADER_KEY_CACHE_CONTROL), CHARPTR_WRAP_RO(HTTP_HEADER_VALUE_NO_CACHE));
      __web_resource.m_server->addHeader(CHARPTR_WRAP_RO(HTTP_HEADER_KEY_SET_COOKIE), _session_cookie);
    }
  }

  /**
   * @brief Builds the Set-Cookie value carrying a session token.
   *
   * @param _str The buffer to store the session cookie.
   * @param _token The session token, or nullptr to clear the cookie.
   * @param _max_size The maximum size of the session cookie buffer.
   * @param _enable_max_age Whether to include the Max-Age attribute.
   * @param _max_age The maximum age of the session cookie.
   */
  void build_session_cookie(char *_str, const char *_token, int _max_size, bool _enable_max_age = false, uint32_t _max_age = SERVER_COOKIE_MAX_AGE)
  {
    memset(_str, 0, _max_size);

    pdiutil::string name = session_cookie_name();
    strcat(_str, name.c_str());
    strcat(_str, "=");

    if (nullptr != _token)
    {
      strcat(_str, _token);
    }

    if (_enable_max_age)
    {
      strcat(_str, ";Max-Age=");
      __appendUintToBuff(_str, "%u", _max_age, 10);
    }

    strcat(_str, ";Path=/;HttpOnly;SameSite=Strict");

    // a browser drops a Secure cookie arriving over plain http, so this follows
    // the transport that served the request and not the build configuration
    if (nullptr != __web_resource.m_server && __web_resource.m_server->isSecure())
    {
      strcat(_str, ";Secure");
    }
  }

  /**
   * @brief Resolves the request cookie into a live server side session.
   *
   * On success the session is published through `SessionManager` so that
   * downstream file operations resolve against the logged in user.
   *
   * @return `true` if an active session is found, `false` otherwise.
   */
  bool has_active_session(void)
  {
    if (nullptr != m_active_session)
    {
      return true;
    }

    if (nullptr == __web_resource.m_server)
    {
      return false;
    }

    if (!__web_resource.m_server->hasHeader(CHARPTR_WRAP_RO(HTTP_HEADER_KEY_COOKIE)))
    {
      LogW("active session not found\n");
      return false;
    }

    pdiutil::string cookie = __web_resource.m_server->header(CHARPTR_WRAP_RO(HTTP_HEADER_KEY_COOKIE));
    pdiutil::string token;

    if (!extract_cookie_value(cookie, session_cookie_name(), token))
    {
      LogW("active session not found\n");
      return false;
    }

    m_active_session = __web_session_manager.validate(token.c_str());

    if (nullptr == m_active_session)
    {
      LogW("active session not found\n");
      return false;
    }

    publish_request_session();
    LogI("active session found\n");
    return true;
  }

  /**
   * @brief Returns the session bound to the current request, if any.
   */
  web_session_t *active_session(void)
  {
    return m_active_session;
  }

  /**
   * @brief Publishes a freshly created session as the session of this request.
   */
  void adopt_session(web_session_t *session)
  {
    m_active_session = session;
    publish_request_session();
  }

  /**
   * @brief Detaches the request session, restoring the previously current one.
   *
   * Called once a request has been served so a web session never stays
   * current for terminal driven work.
   */
  void release_request_session(void)
  {
    if (SessionManager::current() == m_active_session && nullptr != m_active_session)
    {
      SessionManager::setCurrent(m_previous_session);
    }

    m_active_session = nullptr;
    m_previous_session = nullptr;
  }

  /**
   * @brief Verifies the csrf token submitted with a state changing request.
   */
  bool has_valid_csrf_token(void)
  {
    if (nullptr == m_active_session || nullptr == __web_resource.m_server)
    {
      return false;
    }

    if (!__web_resource.m_server->hasArg(CHARPTR_WRAP(WEB_CSRF_FIELD_NAME)))
    {
      return false;
    }

    pdiutil::string submitted = __web_resource.m_server->arg(CHARPTR_WRAP(WEB_CSRF_FIELD_NAME));

    if (submitted.size() != (size_t)WEB_SESSION_TOKEN_HEX_LEN)
    {
      return false;
    }

    uint8_t diff = 0;
    for (uint16_t i = 0; i < WEB_SESSION_TOKEN_HEX_LEN; i++)
    {
      diff |= (uint8_t)(m_active_session->m_csrf[i] ^ submitted[i]);
    }

    return (0 == diff);
  }

protected:

  /**
   * @brief Reads the configured session cookie name.
   */
  pdiutil::string session_cookie_name(void)
  {
    if (nullptr != __web_resource.m_db_conn)
    {
      login_credential_table _login_credentials;
      if (__web_resource.m_db_conn->get_login_credential_table(&_login_credentials) &&
          0 != _login_credentials.session_name[0])
      {
        return pdiutil::string(_login_credentials.session_name);
      }
    }

    return pdiutil::string(CHARPTR_WRAP(SERVER_SESSION_NAME));
  }

  /**
   * @brief Extracts one cookie value from a Cookie header by exact name.
   */
  bool extract_cookie_value(const pdiutil::string &cookie, const pdiutil::string &name, pdiutil::string &out)
  {
    size_t pos = 0;

    while (pos < cookie.size())
    {
      while (pos < cookie.size() && (cookie[pos] == ' ' || cookie[pos] == ';'))
      {
        pos++;
      }

      size_t eq = pos;
      while (eq < cookie.size() && cookie[eq] != '=' && cookie[eq] != ';')
      {
        eq++;
      }

      if (eq >= cookie.size() || cookie[eq] != '=')
      {
        break;
      }

      size_t end = eq + 1;
      while (end < cookie.size() && cookie[end] != ';')
      {
        end++;
      }

      if ((eq - pos) == name.size() && 0 == strncmp(cookie.c_str() + pos, name.c_str(), name.size()))
      {
        out.clear();
        for (size_t i = eq + 1; i < end; i++)
        {
          out += cookie[i];
        }
        return true;
      }

      pos = end;
    }

    return false;
  }

  /**
   * @brief Makes the request session current, remembering the previous one.
   */
  void publish_request_session(void)
  {
    if (nullptr == m_active_session)
    {
      return;
    }

    m_previous_session = SessionManager::current();
    SessionManager::setCurrent(m_active_session);
  }

  /**
   * @var web_session_t* m_active_session
   */
  web_session_t *m_active_session;

  /**
   * @var session_t* m_previous_session
   */
  session_t *m_previous_session;
};

#endif
