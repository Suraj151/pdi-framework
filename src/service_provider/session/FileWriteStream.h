/**************************** File Write Stream *******************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

A file that presents itself as a terminal, so a redirect can hand a command
somewhere to write. Bytes gather in a buffer and reach storage a block at a
time, because writing through per byte costs one open and close per byte.

Author          : Suraj I.
created Date    : 25th Aug 2026
******************************************************************************/
#ifndef _FILE_WRITE_STREAM_H_
#define _FILE_WRITE_STREAM_H_

#include <config/Config.h>
#include <utility/iIOInterface.h>

#if defined(ENABLE_CMD_SERVICE) && defined(ENABLE_STORAGE_SERVICE)

class FileWriteStream : public iTerminalInterface {

public:

  /**
   * Opens the named file, creating it when absent. Appending keeps what is
   * already there, otherwise the first block replaces it.
   */
  FileWriteStream(const char *path, bool append);
  virtual ~FileWriteStream();

  /**
   * False when there is no path or no buffer, and every write is refused.
   */
  bool isValid() const { return nullptr != m_buffer && m_path.size() > 0; }

  /**
   * True once storage refused a block. Those bytes are gone, so a caller that
   * cares has to ask rather than trust the write count.
   */
  bool failed() const { return m_failed; }

  /**
   * Bytes that have reached storage, excluding anything still buffered.
   */
  uint32_t written() const { return m_written; }

  /**
   * Commits what is buffered and reports whether everything written landed:
   * zero, or the first error storage gave.
   */
  int16_t disconnect() override;

  /**
   * Usable while it holds a path and a buffer and storage has not refused it.
   */
  int8_t connected() override;

  using iTerminalInterface::write;
  using iTerminalInterface::read;

  /**
   * Buffers one byte, committing a block first when the buffer is full.
   */
  int32_t write(uint8_t c) override;

  /**
   * Buffers a terminated string, returning how much of it was taken.
   */
  int32_t write(const uint8_t *c_str) override;

  /**
   * Buffers a bounded run of bytes, returning how much of it was taken.
   */
  int32_t write(const uint8_t *c_str, uint32_t size) override;

  /**
   * Buffers a read only string, copied through a window so the source is
   * never addressed directly.
   */
  int32_t write_ro(const char *c_str) override;

  /**
   * Write only, so this yields nothing.
   */
  uint8_t read() override;

  /**
   * Write only, so this yields nothing.
   */
  int32_t read(uint8_t *buf, uint32_t size) override;

  /**
   * Write only, so there is never anything to read.
   */
  int32_t available() override;

  /**
   * Whether the stream is still able to take bytes.
   */
  bool availableforwrite(uint32_t size) override;

  /**
   * A transmit flush commits the buffer without closing the stream.
   */
  void flush(int16_t flushtype = FLUSH_TX) override;

  /**
   * Pushes whatever is buffered to storage. Zero when there was nothing to do
   * or the block landed, negative when storage refused it.
   */
  int32_t commit() override;

private:

  pdiutil::string m_path;
  uint8_t *m_buffer;
  uint16_t m_fill;
  uint32_t m_written;
  bool m_append;
  bool m_started;
  bool m_failed;
  int16_t m_error;

  // Held open for the life of the stream when the backend has handles, so a
  // block costs one write instead of an open, a seek and a close. Negative when
  // the backend has none and the path calls are used instead.
  pdi_fhandle_t m_handle;
};

#endif

#endif
