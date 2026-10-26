/**************************** Session Manager *********************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 18th July 2026
******************************************************************************/
#ifndef _SESSION_MANAGER_H_
#define _SESSION_MANAGER_H_

#include <service_provider/ServiceProvider.h>
#include "SessionStdio.h"

/**
 * SessionManager
 */
class SessionManager {

public:

  static session_t *attach(iTerminalInterface *terminal);
  static void detach(iTerminalInterface *terminal);
  static void detachCurrent();
  static session_t *current();
  static session_t *getByIndex(uint8_t idx);
  static session_t *findByTerminal(iTerminalInterface *terminal);
  static void setCurrent(session_t *s) { m_current = s; }
  static uint8_t maxSessions() { return PDI_MAX_SESSIONS; }
  static uint8_t activeCount();

  /**
   * The scheduler's hooks around a service task: no session is current while it
   * runs, and the previous one is restored after.
   */
  static void *enterSystem();
  static void leaveSystem(void *previous);

  /**
   * Makes the given session current for the scope's lifetime, or none when it
   * is null, and puts back the one that was current.
   */
  class Scope {
  public:
    explicit Scope(session_t *s);
    ~Scope();
  private:
    session_t *m_previous;
  };

#ifdef ENABLE_STORAGE_SERVICE
  static pdiutil::string getPWD();
  static pdiutil::string getLastPWD();
  static bool setPWD(const char *path);
  static bool changeDirectory(const char *path);
  static uint16_t getCurrentUmask();
  static void setCurrentUmask(uint16_t umask);
#endif

#ifdef ENABLE_AUTH_SERVICE
  static uint16_t getCurrentUid();
  static uint16_t getCurrentGid();
#endif

  /**
   * Whether the current session is root, as every session is in a build
   * without accounts.
   */
  static bool isRoot();

#ifdef ENABLE_CMD_SERVICE
  /**
   * Result of the last command the current session ran to completion.
   */
  static pdi_err_t getLastExit();

  /**
   * Records a finished command's result; a still running one is ignored.
   */
  static void setLastExit(pdi_err_t status);

  /**
   * Descriptor table of the given session, or of the current one when null.
   * An owned stream is deleted when the slot is reassigned or released.
   */
  static iTerminalInterface *getFd(uint8_t fd, session_t *s = nullptr);
  static bool setFd(uint8_t fd, iTerminalInterface *stream, bool owned, session_t *s = nullptr);
  static int8_t allocFd(iTerminalInterface *stream, bool owned, session_t *s = nullptr);
  static void closeFd(uint8_t fd, session_t *s = nullptr);

  /**
   * Point stdin, stdout and stderr back at the session terminal, releasing
   * whatever a redirect left behind. Every command dispatch ends here.
   */
  static void resetStdio(session_t *s = nullptr);
  static void releaseFds(session_t *s);

  /**
   * The terminal a command of this session holds while a redirect is live.
   * Writes follow fd 1 and reads fd 0, so output moves without the prompt.
   * Null when nothing is claimed, meaning the session terminal serves directly.
   */
  static SessionStdio *stdioFor(session_t *s);

  /**
   * Whether ctrl+c reached the current session's terminal since its line began,
   * polled at most every CMD_INTERRUPT_POLL_MS.
   */
  static bool interruptRequested();

  /**
   * Whether the current session was interrupted, without looking again.
   */
  static bool interrupted();

  /**
   * Lets the current session's next line run with no interrupt pending.
   */
  static void clearInterrupt();
#endif

private:

#ifdef ENABLE_CMD_SERVICE
  static void releaseTableIfIdle(session_t *s);
#endif

  static session_t m_sessions[PDI_MAX_SESSIONS];
  static session_t *m_current;
};

#endif
