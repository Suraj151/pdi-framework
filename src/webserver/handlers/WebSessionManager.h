/**************************** Web Session Manager *****************************
This file is part of the PDI stack.

This is free software. You can redistribute it and/or modify it but without any
warranty.

The `WebSessionManager` class owns the server side session table for the web
portal. Each slot is a `web_session_t` (a `session_t` carrying an opaque random
token), so an authenticated request can be published through `SessionManager`
and file operations resolve against the logged in user's uid/gid.

Author          : Suraj I.
Created Date    : 14th August 2026
******************************************************************************/

#ifndef _WEB_SESSION_MANAGER_H_
#define _WEB_SESSION_MANAGER_H_

#include <config/Config.h>

#ifdef ENABLE_HTTP_SERVER

/**
 * @class WebSessionManager
 * @brief Server side session store for the web portal.
 */
class WebSessionManager
{

public:

  WebSessionManager();
  ~WebSessionManager();

  /**
   * @brief Allocates a session slot and generates its token and csrf secret.
   *
   * @param username The authenticated user name.
   * @param uid The resolved user id.
   * @param gid The resolved group id.
   * @return Pointer to the new session, or nullptr when the table is full.
   */
  web_session_t *create(const char *username, uint16_t uid, uint16_t gid);

  /**
   * @brief Looks up a session by token and enforces idle and absolute expiry.
   *
   * Refreshes the last activity stamp of a session that is still valid.
   *
   * @param token The token taken from the client cookie.
   * @return Pointer to the live session, or nullptr when unknown or expired.
   */
  web_session_t *validate(const char *token);

  /**
   * @brief Releases a single session slot.
   */
  void destroy(web_session_t *session);

  /**
   * @brief Releases every session belonging to a user.
   *
   * Used after a credential change so old cookies stop working.
   */
  void destroyByUsername(const char *username);

  /**
   * @brief Releases sessions that have passed idle or absolute expiry.
   */
  void collectExpired();

  /**
   * @brief Records a failed login and reports whether logins are now blocked.
   */
  void registerLoginFailure();

  /**
   * @brief Clears the failure counter after a successful login.
   */
  void clearLoginFailures();

  /**
   * @brief Reports whether login attempts are currently in cooldown.
   */
  bool isLoginBlocked();

private:

  bool isExpired(const web_session_t &session, uint32_t now);
  void generateToken(char *out);
  uint32_t idleMaxAge();

  web_session_t m_sessions[WEB_MAX_SESSIONS];
  uint8_t m_loginFailures;
  uint32_t m_lockoutStartedAt;
};

extern WebSessionManager __web_session_manager;

#endif

#endif
