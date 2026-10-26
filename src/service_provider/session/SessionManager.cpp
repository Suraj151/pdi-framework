/**************************** Session Manager *********************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 18th July 2026
******************************************************************************/

#include "SessionManager.h"
#include <service_provider/ServiceProvider.h>
#ifdef ENABLE_CMD_SERVICE
#include <service_provider/cmd/CommandLineServiceProvider.h>
#include <utility/SafeAlloc.h>
#endif

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
#ifdef ENABLE_CMD_SERVICE
      // a slot freed without going through detach still holds its descriptors
      releaseFds(&m_sessions[i]);
#endif
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
#ifdef ENABLE_CMD_SERVICE
    // a command still waiting for input belongs to this session, not to the
    // next terminal that is handed the slot
    __cmd_service.releaseSession(s);
    releaseFds(s);
#endif
    s->clear();
    if (m_current == s) {
      m_current = nullptr;
    }
  }

  iTerminalInterface *console = __i_dvc_ctrl.getTerminal();
  if (terminal == ServiceProvider::getTerminal() && terminal != console) {
    ServiceProvider::setTerminal(console);
  }
}

void SessionManager::detachCurrent() {

  if (nullptr != m_current) {
#ifdef ENABLE_CMD_SERVICE
    __cmd_service.releaseSession(m_current);
    releaseFds(m_current);
#endif
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

/**
 * The scheduler's hooks around a service task: no session is current while it runs,
 * and the previous one is restored after.
 */
void *SessionManager::enterSystem() {
  session_t *previous = m_current;
  m_current = nullptr;
  return previous;
}

void SessionManager::leaveSystem(void *previous) {
  m_current = static_cast<session_t *>(previous);
}

/**
 * Makes the given session current for the scope's lifetime, or none when it
 * is null, and puts back the one that was current.
 */
SessionManager::Scope::Scope(session_t *s) : m_previous(m_current) {
  m_current = s;
}

SessionManager::Scope::~Scope() {
  leaveSystem(m_previous);
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

uint16_t SessionManager::getCurrentUmask() {
  session_t *s = current();
  return (nullptr != s) ? s->m_umask : (uint16_t)FILE_UMASK_DEFAULT;
}

void SessionManager::setCurrentUmask(uint16_t umask) {
  session_t *s = current();
  if (nullptr != s) s->m_umask = umask & 0777;
}

#endif

#ifdef ENABLE_AUTH_SERVICE

uint16_t SessionManager::getCurrentUid() {
  session_t *s = current();
  return (nullptr != s) ? s->m_uid : (uint16_t)0;
}

uint16_t SessionManager::getCurrentGid() {
  session_t *s = current();
  return (nullptr != s) ? s->m_gid : (uint16_t)0;
}

#endif

/**
 * Whether the current session is root, as every session is in a build
 * without accounts.
 */
bool SessionManager::isRoot() {
#ifdef ENABLE_AUTH_SERVICE
  return USER_STORE_ROOT_UID == getCurrentUid();
#else
  return true;
#endif
}

#ifdef ENABLE_CMD_SERVICE

/**
 * Result of the last command the current session ran to completion.
 */
pdi_err_t SessionManager::getLastExit() {

  session_t *s = current();
  return (nullptr != s) ? s->m_lastExit : (pdi_err_t)PDI_OK;
}

/**
 * Records a finished command's result; a still running one is ignored.
 */
void SessionManager::setLastExit(pdi_err_t status) {

  if (CMD_ERROR_AGAIN == status || CMD_ERROR_HOLD_BUFFER == status) {
    return;
  }

  session_t *s = current();
  if (nullptr != s) {
    s->m_lastExit = status;
  }
}

/**
 * The stream a descriptor resolves to, falling back to the session terminal
 * for the standard three when nothing has claimed them.
 */
iTerminalInterface *SessionManager::getFd(uint8_t fd, session_t *s) {

  if (nullptr == s) s = current();
  if (nullptr == s || fd >= PDI_MAX_FDS) {
    return nullptr;
  }

  if (nullptr != s->m_fdtable && nullptr != s->m_fdtable->m_fds[fd]) {
    return s->m_fdtable->m_fds[fd];
  }

  return (fd <= PDI_FD_STDERR) ? s->m_terminal : nullptr;
}

/**
 * Claims a descriptor, taking the table on first use. An owned stream is
 * deleted when the slot is reassigned or released.
 */
bool SessionManager::setFd(uint8_t fd, iTerminalInterface *stream, bool owned, session_t *s) {

  if (nullptr == s) s = current();
  if (nullptr == s || fd >= PDI_MAX_FDS) {
    return false;
  }

  closeFd(fd, s);

  if (nullptr == stream) {
    return true;
  }

  if (nullptr == s->m_fdtable) {
    s->m_fdtable = pdiutil::safe_new<fd_table_t>();
    if (nullptr == s->m_fdtable) {
      return false;
    }
  }

  s->m_fdtable->m_fds[fd] = stream;
  if (owned) {
    s->m_fdtable->m_owned |= (uint8_t)(1 << fd);
  }
  return true;
}

/**
 * Claims the first free slot above the standard three, or -1 when none is
 * left.
 */
int8_t SessionManager::allocFd(iTerminalInterface *stream, bool owned, session_t *s) {

  if (nullptr == s) s = current();
  if (nullptr == s || nullptr == stream) {
    return -1;
  }

  for (uint8_t fd = PDI_FD_STDERR + 1; fd < PDI_MAX_FDS; fd++) {
    if (nullptr == s->m_fdtable || nullptr == s->m_fdtable->m_fds[fd]) {
      return setFd(fd, stream, owned, s) ? (int8_t)fd : (int8_t)-1;
    }
  }
  return -1;
}

/**
 * Releases one descriptor, deleting its stream when owned, and drops the
 * table once the last slot clears.
 */
void SessionManager::closeFd(uint8_t fd, session_t *s) {

  if (nullptr == s) s = current();
  if (nullptr == s || fd >= PDI_MAX_FDS || nullptr == s->m_fdtable) {
    return;
  }

  fd_table_t *t = s->m_fdtable;

  if (t->m_owned & (uint8_t)(1 << fd)) {
    iTerminalInterface *owned_stream = t->m_fds[fd];
    t->m_owned &= (uint8_t)~(1 << fd);
    t->m_fds[fd] = nullptr;
    pdiutil::safe_delete(owned_stream);
  } else {
    t->m_fds[fd] = nullptr;
  }

  releaseTableIfIdle(s);
}

/**
 * Points the standard three back at the session terminal, releasing whatever
 * a redirect left behind.
 */
void SessionManager::resetStdio(session_t *s) {

  if (nullptr == s) s = current();
  if (nullptr == s || nullptr == s->m_fdtable) {
    return;
  }

  closeFd(PDI_FD_STDIN, s);
  closeFd(PDI_FD_STDOUT, s);
  closeFd(PDI_FD_STDERR, s);
}

/**
 * Frees the table and its adapter once no slot is claimed.
 */
void SessionManager::releaseTableIfIdle(session_t *s) {

  if (nullptr == s || nullptr == s->m_fdtable || !s->m_fdtable->isIdle()) {
    return;
  }

#ifdef ENABLE_CMD_SERVICE
  __cmd_service.retargetTerminal(s->m_fdtable->m_stdio, s->m_terminal);
#endif
  pdiutil::safe_delete(s->m_fdtable->m_stdio);
  pdiutil::safe_delete(s->m_fdtable);
}

/**
 * The adapter a command of this session writes through while a redirect is
 * live, or null when the terminal already serves directly.
 */
SessionStdio *SessionManager::stdioFor(session_t *s) {

  if (nullptr == s) s = current();

  if (nullptr == s || nullptr == s->m_fdtable) {
    return nullptr;
  }

  if (nullptr == s->m_fdtable->m_stdio) {
    s->m_fdtable->m_stdio = pdiutil::safe_new<SessionStdio>();
  }

  if (nullptr != s->m_fdtable->m_stdio) {
    s->m_fdtable->m_stdio->bind(s);
  }
  return s->m_fdtable->m_stdio;
}

/**
 * Releases every descriptor the session holds along with its table, so
 * nothing outlives the session.
 */
void SessionManager::releaseFds(session_t *s) {

  if (nullptr == s || nullptr == s->m_fdtable) {
    return;
  }

  for (uint8_t fd = 0; fd < PDI_MAX_FDS; fd++) {
    closeFd(fd, s);
  }

  if (nullptr != s->m_fdtable) {
    pdiutil::safe_delete(s->m_fdtable->m_stdio);
    pdiutil::safe_delete(s->m_fdtable);
  }
}

/**
 * Whether ctrl+c reached the current session's terminal since its line began,
 * polled at most every CMD_INTERRUPT_POLL_MS.
 */
bool SessionManager::interruptRequested() {

  session_t *s = current();
  if (nullptr == s || s->m_interrupted) {
    return (nullptr != s);
  }

  uint32_t now = (uint32_t)__i_dvc_ctrl.millis_now();
  if (0 != s->m_inputPolledAt && (now - s->m_inputPolledAt) < CMD_INTERRUPT_POLL_MS) {
    return false;
  }
  s->m_inputPolledAt = (0 == now) ? 1 : now;

  iTerminalInterface *t = s->m_terminal;
  while (nullptr != t && t->available() > 0) {
    uint8_t c = t->read();
    if (CMD_INTERRUPT_CHAR == c) {
      s->m_interrupted = true;
      s->m_typeahead.clear();
      if (__cmd_service.isSessionBusy(s)) {
        s->m_typeahead += (char)c;
      }
      break;
    }
    if (s->m_typeahead.size() < CMD_TYPEAHEAD_MAX) {
      s->m_typeahead += (char)c;
    }
  }
  return s->m_interrupted;
}

/**
 * Whether the current session was interrupted, without looking again.
 */
bool SessionManager::interrupted() {
  session_t *s = current();
  return (nullptr != s) && s->m_interrupted;
}

/**
 * Lets the current session's next line run with no interrupt pending.
 */
void SessionManager::clearInterrupt() {
  session_t *s = current();
  if (nullptr != s) {
    s->m_interrupted = false;
    s->m_inputPolledAt = 0;
  }
}

#endif

