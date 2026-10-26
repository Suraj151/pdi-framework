/**************************** Session Stdio ***********************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 25th Aug 2026
******************************************************************************/

#include "SessionStdio.h"

#ifdef ENABLE_CMD_SERVICE

#include "SessionManager.h"

namespace {
  const uint8_t RO_WINDOW = 32;
}

/**
 * Whatever holds the input descriptor, which reads are handed to.
 */
iTerminalInterface *SessionStdio::in() const {
  return SessionManager::getFd(PDI_FD_STDIN, m_session);
}

/**
 * Whatever holds the output descriptor, which writes are handed to.
 */
iTerminalInterface *SessionStdio::out() const {
  return SessionManager::getFd(PDI_FD_STDOUT, m_session);
}

/**
 * Closes the underlying terminal, since a descriptor has no connection of
 * its own to drop.
 */
int16_t SessionStdio::disconnect() {
  iTerminalInterface *t = (nullptr != m_session) ? m_session->m_terminal : nullptr;
  return (nullptr != t) ? t->disconnect() : (int16_t)-1;
}

/**
 * Follows the underlying terminal, not whatever a redirect has claimed.
 */
int8_t SessionStdio::connected() {
  iTerminalInterface *t = (nullptr != m_session) ? m_session->m_terminal : nullptr;
  return (nullptr != t) ? t->connected() : (int8_t)0;
}

/**
 * Commands open their output with a newline to leave the prompt line. That
 * is presentation, so it is dropped once when output has been redirected.
 */
uint32_t SessionStdio::leadOffset(const uint8_t *c_str, uint32_t size) {

  if (!m_droplead) {
    return 0;
  }

  m_droplead = false;

  if (nullptr == c_str || 0 == size) {
    return 0;
  }

  if ('\r' == c_str[0] && size > 1 && '\n' == c_str[1]) {
    return 2;
  }

  if ('\n' == c_str[0]) {
    return 1;
  }

  return 0;
}

/**
 * Hands bytes to the descriptor, dropping the carriage return of a line
 * ending while output is somewhere other than a terminal.
 */
int32_t SessionStdio::emit(iTerminalInterface *t, const uint8_t *c_str, uint32_t size) {

  if (!m_translate) {
    return t->write(c_str, size);
  }

  uint32_t seg = 0;

  for (uint32_t i = 0; i < size; i++) {

    if (m_heldcr) {
      m_heldcr = false;
      if ('\n' != c_str[i]) {
        const uint8_t cr = '\r';
        t->write(&cr, 1);
      }
    }

    if ('\r' != c_str[i]) {
      continue;
    }

    if (i + 1 < size) {
      if ('\n' == c_str[i + 1]) {
        if (i > seg) t->write(c_str + seg, i - seg);
        seg = i + 1;
      }
      continue;
    }

    if (i > seg) t->write(c_str + seg, i - seg);
    seg = size;
    m_heldcr = true;
  }

  if (seg < size) {
    t->write(c_str + seg, size - seg);
  }

  return (int32_t)size;
}

/**
 * Writes one byte to the output descriptor.
 */
int32_t SessionStdio::write(uint8_t c) {
  return write(&c, 1);
}

/**
 * Writes a terminated string to the output descriptor.
 */
int32_t SessionStdio::write(const uint8_t *c_str) {

  if (nullptr == c_str) {
    return 0;
  }

  uint32_t len = 0;
  while (c_str[len] != '\0') len++;

  return write(c_str, len);
}

/**
 * Writes a bounded run of bytes to the output descriptor.
 */
int32_t SessionStdio::write(const uint8_t *c_str, uint32_t size) {

  iTerminalInterface *t = out();
  if (nullptr == t) {
    return 0;
  }

  uint32_t skip = leadOffset(c_str, size);
  if (skip >= size) {
    return (int32_t)size;
  }

  return emit(t, c_str + skip, size - skip) + (int32_t)skip;
}

/**
 * Writes bytes exactly as given to the output descriptor, with no line end
 * tidied, as a copy of a file must arrive.
 */
int32_t SessionStdio::writeData(const uint8_t *c_str, uint32_t size) {

  iTerminalInterface *t = out();
  if (nullptr == t || nullptr == c_str) {
    return 0;
  }

  if (m_heldcr) {
    m_heldcr = false;
    const uint8_t cr = '\r';
    t->write(&cr, 1);
  }
  m_droplead = false;

  return t->write(c_str, size);
}

/**
 * Writes a read only string to the output descriptor.
 */
int32_t SessionStdio::write_ro(const char *c_str) {

  iTerminalInterface *t = out();
  if (nullptr == t || nullptr == c_str) {
    return 0;
  }

  if (!m_translate) {
    m_droplead = false;
    return t->write_ro(c_str);
  }

  uint32_t len = strlen_ro(c_str);
  uint32_t done = 0;
  char window[RO_WINDOW];

  while (done < len) {

    uint32_t chunk = len - done;
    if (chunk > RO_WINDOW) chunk = RO_WINDOW;

    memcpy_ro(window, c_str + done, chunk);

    int32_t put = write((const uint8_t *)window, chunk);
    if (put <= 0) {
      break;
    }
    done += (uint32_t)put;
  }

  return (int32_t)done;
}

/**
 * Takes one byte from the input descriptor.
 */
uint8_t SessionStdio::read() {
  iTerminalInterface *t = in();
  return (nullptr != t) ? t->read() : (uint8_t)0;
}

/**
 * Takes up to size bytes from the input descriptor.
 */
int32_t SessionStdio::read(uint8_t *buf, uint32_t size) {
  iTerminalInterface *t = in();
  return (nullptr != t) ? t->read(buf, size) : 0;
}

/**
 * Bytes waiting on the input descriptor.
 */
int32_t SessionStdio::available() {
  iTerminalInterface *t = in();
  return (nullptr != t) ? t->available() : 0;
}

/**
 * Whether the output descriptor can take size further bytes.
 */
bool SessionStdio::availableforwrite(uint32_t size) {
  iTerminalInterface *t = out();
  return (nullptr != t) ? t->availableforwrite(size) : false;
}

/**
 * Whether the output descriptor takes no further write.
 */
bool SessionStdio::writeRefused() const {
  iTerminalInterface *t = out();
  return (nullptr != t) && t->writeRefused();
}

/**
 * Sends each half of the flush to the descriptor that owns it, and passes
 * the type straight through when both resolve to the same stream.
 */
void SessionStdio::flush(int16_t flushtype) {

  iTerminalInterface *o = out();
  iTerminalInterface *i = in();

  if (o == i) {
    if (nullptr != o) o->flush(flushtype);
    return;
  }

  if (IsFlushTx(flushtype) && nullptr != o) {
    o->flush(FLUSH_TX);
  }

  if (IsFlushRx(flushtype) && nullptr != i) {
    i->flush(FLUSH_RX);
  }
}

#endif
