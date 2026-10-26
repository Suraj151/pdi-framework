/**************************** Session Stdio ***********************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

A command holds one terminal pointer but reads and writes through separate
descriptors. SessionStdio is the terminal it holds: reads route to the session's
fd 0 and writes to fd 1, so a redirect moves output without moving the prompt.

Author          : Suraj I.
created Date    : 25th Aug 2026
******************************************************************************/
#ifndef _SESSION_STDIO_H_
#define _SESSION_STDIO_H_

#include <config/Config.h>
#include <utility/iIOInterface.h>

#ifdef ENABLE_CMD_SERVICE

class SessionStdio : public iTerminalInterface {

public:

  SessionStdio() : m_session(nullptr), m_droplead(false), m_translate(false),
                   m_heldcr(false) {}
  virtual ~SessionStdio() {}

  /**
   * Adopts the session and mirrors its terminal geometry, so a command that
   * lays out to the window still sees the real one while its output is away.
   */
  void bind(session_t *s) {
    m_session = s;
    if (nullptr != s && nullptr != s->m_terminal) {
      set_terminal_type(s->m_terminal->get_terminal_type());
      set_column_width(s->m_terminal->get_column_width());
      set_row_count(s->m_terminal->get_row_count());
    }
    m_translate = (nullptr != s && out() != s->m_terminal);
    m_droplead = m_translate;
    m_heldcr = false;
  }

  /**
   * The session this adapter currently speaks for.
   */
  session_t *boundTo() const { return m_session; }

  /**
   * Whatever holds the input descriptor, which reads are handed to.
   */
  iTerminalInterface *in() const;

  /**
   * Whatever holds the output descriptor, which writes are handed to.
   */
  iTerminalInterface *out() const;

  /**
   * Closes the underlying terminal, since a descriptor has no connection of
   * its own to drop.
   */
  int16_t disconnect() override;

  /**
   * Follows the underlying terminal, not whatever a redirect has claimed.
   */
  int8_t connected() override;

  using iTerminalInterface::write;
  using iTerminalInterface::read;

  /**
   * Writes one byte to the output descriptor.
   */
  int32_t write(uint8_t c) override;

  /**
   * Writes a terminated string to the output descriptor.
   */
  int32_t write(const uint8_t *c_str) override;

  /**
   * Writes a bounded run of bytes to the output descriptor.
   */
  int32_t write(const uint8_t *c_str, uint32_t size) override;

  /**
   * Writes a read only string to the output descriptor.
   */
  int32_t write_ro(const char *c_str) override;

  /**
   * Takes one byte from the input descriptor.
   */
  uint8_t read() override;

  /**
   * Takes up to size bytes from the input descriptor.
   */
  int32_t read(uint8_t *buf, uint32_t size) override;

  /**
   * Bytes waiting on the input descriptor.
   */
  int32_t available() override;

  /**
   * Whether the output descriptor can take size further bytes.
   */
  bool availableforwrite(uint32_t size) override;

  /**
   * Whether the output descriptor takes no further write.
   */
  bool writeRefused() const override;

  /**
   * Writes bytes exactly as given to the output descriptor, with no line end
   * tidied, as a copy of a file must arrive.
   */
  int32_t writeData(const uint8_t *c_str, uint32_t size) override;

  /**
   * Sends each half of the flush to the descriptor that owns it, and passes
   * the type straight through when both resolve to the same stream.
   */
  void flush(int16_t flushtype = FLUSH_TX) override;

private:

  /**
   * Commands open their output with a newline to leave the prompt line. That
   * is presentation, so it is dropped once when output has been redirected.
   */
  uint32_t leadOffset(const uint8_t *c_str, uint32_t size);

  /**
   * Hands bytes to the descriptor, dropping the carriage return of a line
   * ending while output is somewhere other than a terminal.
   */
  int32_t emit(iTerminalInterface *t, const uint8_t *c_str, uint32_t size);

  session_t *m_session;
  bool m_droplead;
  bool m_translate;
  bool m_heldcr;
};

#endif

#endif
