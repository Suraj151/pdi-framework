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

#ifdef ENABLE_STORAGE_SERVICE
  static pdiutil::string getPWD();
  static pdiutil::string getLastPWD();
  static bool setPWD(const char *path);
  static bool changeDirectory(const char *path);
#endif

private:

  static session_t m_sessions[PDI_MAX_SESSIONS];
  static session_t *m_current;
};

#endif
