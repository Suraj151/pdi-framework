/**************************** Web Session Manager *****************************
This file is part of the PDI stack.

This is free software. You can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
Created Date    : 14th August 2026
******************************************************************************/

#include "WebSessionManager.h"

#ifdef ENABLE_HTTP_SERVER

#include <webserver/resources/WebResource.h>
#include <service_provider/session/SessionManager.h>
#include <utility/DataTypeConversions.h>

WebSessionManager::WebSessionManager() : m_loginFailures(0), m_lockoutStartedAt(0)
{
}

WebSessionManager::~WebSessionManager()
{
}

uint32_t WebSessionManager::idleMaxAge()
{
  if (nullptr != __web_resource.m_db_conn)
  {
    login_credential_table creds;
    if (__web_resource.m_db_conn->get_login_credential_table(&creds) && creds.cookie_max_age > 0)
    {
      return (uint32_t)creds.cookie_max_age;
    }
  }
  return (uint32_t)SERVER_COOKIE_MAX_AGE;
}

void WebSessionManager::generateToken(char *out)
{
  uint8_t raw[WEB_SESSION_TOKEN_BYTES];

  for (uint8_t i = 0; i < WEB_SESSION_TOKEN_BYTES; i += 4)
  {
    uint32_t r = __i_dvc_ctrl.random_now();
    uint8_t remaining = (uint8_t)(WEB_SESSION_TOKEN_BYTES - i);
    uint8_t span = remaining < 4 ? remaining : 4;

    for (uint8_t b = 0; b < span; b++)
    {
      raw[i + b] = (uint8_t)((r >> (8 * b)) & 0xFF);
    }
  }

  memset(out, 0, WEB_SESSION_TOKEN_HEX_LEN + 1);
  BytesToHexString(raw, WEB_SESSION_TOKEN_BYTES, out);
}

bool WebSessionManager::isExpired(const web_session_t &session, uint32_t now)
{
  uint32_t idleseconds = (uint32_t)(now - session.m_lastActivityAt) / 1000;
  if (idleseconds > idleMaxAge())
  {
    return true;
  }

  uint32_t liveseconds = (uint32_t)(now - session.m_loginAt) / 1000;
  return (liveseconds > (uint32_t)WEB_SESSION_ABSOLUTE_MAX_AGE);
}

web_session_t *WebSessionManager::create(const char *username, uint16_t uid, uint16_t gid)
{
  if (nullptr == username || 0 == username[0])
  {
    return nullptr;
  }

  collectExpired();

  web_session_t *slot = nullptr;
  for (uint8_t i = 0; i < WEB_MAX_SESSIONS; i++)
  {
    if (SESSION_STATE_FREE == m_sessions[i].m_state)
    {
      slot = &m_sessions[i];
      break;
    }
  }

  if (nullptr == slot)
  {
    uint32_t now = (uint32_t)__i_dvc_ctrl.millis_now();
    uint32_t oldest = 0;

    for (uint8_t i = 0; i < WEB_MAX_SESSIONS; i++)
    {
      uint32_t age = (uint32_t)(now - m_sessions[i].m_lastActivityAt);
      if (nullptr == slot || age > oldest)
      {
        oldest = age;
        slot = &m_sessions[i];
      }
    }
  }

  if (nullptr == slot)
  {
    return nullptr;
  }

  slot->clear();
  slot->m_sid = (uint8_t)(slot - m_sessions) + 1;
  slot->m_state = SESSION_STATE_INTERACTIVE;
  slot->m_terminal = nullptr;
  slot->m_username = username;
  slot->m_loginAt = (uint32_t)__i_dvc_ctrl.millis_now();
  slot->m_lastActivityAt = slot->m_loginAt;
#ifdef ENABLE_AUTH_SERVICE
  slot->m_isAuthorized = true;
  slot->m_uid = uid;
  slot->m_gid = gid;
#endif
#ifdef ENABLE_STORAGE_SERVICE
  slot->m_umask = FILE_UMASK_DEFAULT;
  slot->m_cwd = __i_fs.getHomeDirectory();
#endif

  generateToken(slot->m_token);
  generateToken(slot->m_csrf);

  return slot;
}

web_session_t *WebSessionManager::validate(const char *token)
{
  if (nullptr == token)
  {
    return nullptr;
  }

  if (strlen(token) != (size_t)WEB_SESSION_TOKEN_HEX_LEN)
  {
    return nullptr;
  }

  uint32_t now = (uint32_t)__i_dvc_ctrl.millis_now();

  for (uint8_t i = 0; i < WEB_MAX_SESSIONS; i++)
  {
    if (SESSION_STATE_FREE == m_sessions[i].m_state)
    {
      continue;
    }

    uint8_t diff = 0;
    for (uint16_t c = 0; c < WEB_SESSION_TOKEN_HEX_LEN; c++)
    {
      diff |= (uint8_t)(m_sessions[i].m_token[c] ^ token[c]);
    }

    if (0 != diff)
    {
      continue;
    }

    if (isExpired(m_sessions[i], now))
    {
      m_sessions[i].clear();
      return nullptr;
    }

    m_sessions[i].m_lastActivityAt = now;
    return &m_sessions[i];
  }

  return nullptr;
}

void WebSessionManager::destroy(web_session_t *session)
{
  if (nullptr == session)
  {
    return;
  }

  if (SessionManager::current() == session)
  {
    SessionManager::setCurrent(nullptr);
  }

  session->clear();
}

void WebSessionManager::destroyByUsername(const char *username)
{
  if (nullptr == username || 0 == username[0])
  {
    return;
  }

  for (uint8_t i = 0; i < WEB_MAX_SESSIONS; i++)
  {
    if (SESSION_STATE_FREE != m_sessions[i].m_state &&
        m_sessions[i].m_username == username)
    {
      destroy(&m_sessions[i]);
    }
  }
}

void WebSessionManager::collectExpired()
{
  uint32_t now = (uint32_t)__i_dvc_ctrl.millis_now();

  for (uint8_t i = 0; i < WEB_MAX_SESSIONS; i++)
  {
    if (SESSION_STATE_FREE != m_sessions[i].m_state && isExpired(m_sessions[i], now))
    {
      destroy(&m_sessions[i]);
    }
  }
}

void WebSessionManager::registerLoginFailure()
{
  if (m_loginFailures < WEB_LOGIN_MAX_ATTEMPTS)
  {
    m_loginFailures++;
  }

  if (m_loginFailures >= WEB_LOGIN_MAX_ATTEMPTS)
  {
    m_lockoutStartedAt = (uint32_t)__i_dvc_ctrl.millis_now();
  }
}

void WebSessionManager::clearLoginFailures()
{
  m_loginFailures = 0;
  m_lockoutStartedAt = 0;
}

bool WebSessionManager::isLoginBlocked()
{
  if (m_loginFailures < WEB_LOGIN_MAX_ATTEMPTS)
  {
    return false;
  }

  uint32_t elapsed = (uint32_t)((uint32_t)__i_dvc_ctrl.millis_now() - m_lockoutStartedAt) / 1000;
  if (elapsed > (uint32_t)WEB_LOGIN_LOCKOUT_DURATION)
  {
    clearLoginFailures();
    return false;
  }

  return true;
}

WebSessionManager __web_session_manager;

#endif
