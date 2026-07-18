/**************************** Session Manager *********************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 18th July 2026
******************************************************************************/

#include "SessionManager.h"

session_t SessionManager::m_sessions[PDI_MAX_SESSIONS];
session_t *SessionManager::m_current = nullptr;

session_t *SessionManager::attach(iTerminalInterface *terminal) {

  if (nullptr == terminal) {
    return nullptr;
  }

  session_t *existing = findByTerminal(terminal);
  if (nullptr != existing) {
    m_current = existing;
    return existing;
  }

  for (uint8_t i = 0; i < PDI_MAX_SESSIONS; i++) {
    if (SESSION_STATE_FREE == m_sessions[i].m_state) {
      m_sessions[i].clear();
      m_sessions[i].m_sid = i + 1;
      m_sessions[i].m_state = SESSION_STATE_PRELOGIN;
      m_sessions[i].m_terminal = terminal;
      m_current = &m_sessions[i];
      return m_current;
    }
  }

  return nullptr;
}

void SessionManager::detach(iTerminalInterface *terminal) {

  if (nullptr == terminal) {
    return;
  }

  session_t *s = findByTerminal(terminal);
  if (nullptr != s) {
    s->clear();
    if (m_current == s) {
      m_current = nullptr;
    }
  }
}

void SessionManager::detachCurrent() {

  if (nullptr != m_current) {
    m_current->clear();
    m_current = nullptr;
  }
}

session_t *SessionManager::current() {

  if (nullptr != m_current && SESSION_STATE_FREE != m_current->m_state) {
    return m_current;
  }
  return nullptr;
}

session_t *SessionManager::getByIndex(uint8_t idx) {

  if (idx >= PDI_MAX_SESSIONS) {
    return nullptr;
  }
  return &m_sessions[idx];
}

session_t *SessionManager::findByTerminal(iTerminalInterface *terminal) {

  if (nullptr == terminal) {
    return nullptr;
  }

  for (uint8_t i = 0; i < PDI_MAX_SESSIONS; i++) {
    if (SESSION_STATE_FREE != m_sessions[i].m_state && m_sessions[i].m_terminal == terminal) {
      return &m_sessions[i];
    }
  }
  return nullptr;
}

uint8_t SessionManager::activeCount() {

  uint8_t n = 0;
  for (uint8_t i = 0; i < PDI_MAX_SESSIONS; i++) {
    if (SESSION_STATE_FREE != m_sessions[i].m_state) {
      n++;
    }
  }
  return n;
}

#ifdef ENABLE_STORAGE_SERVICE

pdiutil::string SessionManager::getPWD() {

  session_t *s = current();
  if (nullptr != s && !s->m_cwd.empty()) {
    return s->m_cwd;
  }
  return __i_fs.getPWD();
}

pdiutil::string SessionManager::getLastPWD() {

  session_t *s = current();
  if (nullptr != s && !s->m_lastCwd.empty()) {
    return s->m_lastCwd;
  }
  return __i_fs.getLastPWD();
}

bool SessionManager::setPWD(const char *path) {

  session_t *s = current();
  if (nullptr == s) {
    return __i_fs.setPWD(path);
  }
  if (!__i_fs.isDirectory(path)) {
    return false;
  }
  s->m_lastCwd = s->m_cwd;
  s->m_cwd = path;
  if (path[strlen(path) - 1] != FILE_SEPARATOR[0]) {
    s->m_cwd += FILE_SEPARATOR[0];
  }
  return true;
}

bool SessionManager::changeDirectory(const char *path) {

  session_t *s = current();
  if (nullptr == s) {
    return __i_fs.changeDirectory(path);
  }
  if (s->m_cwd.empty()) {
    s->m_cwd = __i_fs.getPWD();
  }
  s->m_lastCwd = s->m_cwd;
  return __i_fs.updatePathNotations(path, s->m_cwd);
}

#endif

