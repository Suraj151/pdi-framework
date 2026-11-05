/**************************** File Write Stream *******************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 25th Aug 2026
******************************************************************************/

#include "FileWriteStream.h"

#if defined(ENABLE_CMD_SERVICE) && defined(ENABLE_STORAGE_SERVICE)

#include <interface/pdi.h>
#include <utility/SafeAlloc.h>

namespace {
  const uint8_t RO_WINDOW = 32;
}

/**
 * Opens the named file, creating it when absent. Appending keeps what is
 * already there, otherwise the first block replaces it.
 */
FileWriteStream::FileWriteStream(const char *path, bool append) :
  m_buffer(nullptr), m_fill(0), m_written(0), m_append(append),
  m_started(false), m_failed(false), m_error(0), m_handle(-1) {

  if (nullptr != path) {
    m_path = path;
  }

  if (m_path.size() > 0) {
    m_buffer = pdiutil::safe_new_array<uint8_t>(PDI_FILE_STREAM_BUFFER);
    uint8_t flags = FILE_OPEN_WRITE | FILE_OPEN_CREATE;
    flags |= append ? FILE_OPEN_APPEND : FILE_OPEN_TRUNCATE;
    m_handle = __i_fs.openFile(m_path.c_str(), flags);
  }
}

FileWriteStream::~FileWriteStream() {
  commit();
  if (m_handle >= 0) {
    __i_fs.closeFile(m_handle);
    m_handle = -1;
  }
  pdiutil::safe_delete_array(m_buffer);
}

/**
 * Pushes whatever is buffered to storage. Zero when there was nothing to do
 * or the block landed, negative when storage refused it.
 */
int32_t FileWriteStream::commit() {

  if (nullptr == m_buffer || 0 == m_fill) {
    return 0;
  }

  bool append = m_append || m_started;

  int status = (m_handle >= 0)
    ? __i_fs.writeFileHandle(m_handle, (const char *)m_buffer, m_fill)
    : __i_fs.writeFile(m_path.c_str(), (const char *)m_buffer, m_fill, append);

  if (status < 0) {
    m_failed = true;
    m_error = (0 == m_error) ? (int16_t)status : m_error;
    m_fill = 0;
    return -1;
  }

  m_started = true;
  m_written += m_fill;
  m_fill = 0;
  return 0;
}

/**
 * Commits what is buffered and reports whether everything written landed:
 * zero, or the first error storage gave.
 */
int16_t FileWriteStream::disconnect() {

  int32_t status = commit();

  // littlefs holds the tail of a written file until the handle closes, so the
  // close belongs here, where a caller is still listening for the result
  if (m_handle >= 0) {
    int32_t closed = __i_fs.closeFile(m_handle);
    if (closed < 0) {
      m_failed = true;
      m_error = (0 == m_error) ? (int16_t)closed : m_error;
      status = -1;
    }
    m_handle = -1;
  }

  if (0 == status && !m_failed) {
    return 0;
  }
  return (0 != m_error) ? m_error : (int16_t)PDI_ERR_IO;
}

/**
 * Usable while it holds a path and a buffer and storage has not refused it.
 */
int8_t FileWriteStream::connected() {
  return (isValid() && !m_failed) ? (int8_t)1 : (int8_t)0;
}

/**
 * Buffers one byte, committing a block first when the buffer is full.
 */
int32_t FileWriteStream::write(uint8_t c) {
  return write(&c, 1);
}

/**
 * Buffers a terminated string, returning how much of it was taken.
 */
int32_t FileWriteStream::write(const uint8_t *c_str) {

  if (nullptr == c_str) {
    return 0;
  }

  uint32_t len = 0;
  while (c_str[len] != '\0') len++;

  return write(c_str, len);
}

/**
 * Buffers a bounded run of bytes, returning how much of it was taken.
 */
int32_t FileWriteStream::write(const uint8_t *c_str, uint32_t size) {

  if (nullptr == c_str || !isValid() || m_failed) {
    return 0;
  }

  uint32_t taken = 0;
  while (taken < size) {

    if (m_fill >= PDI_FILE_STREAM_BUFFER && 0 != commit()) {
      break;
    }

    uint32_t room = PDI_FILE_STREAM_BUFFER - m_fill;
    uint32_t chunk = size - taken;
    if (chunk > room) chunk = room;

    memcpy(m_buffer + m_fill, c_str + taken, chunk);
    m_fill += (uint16_t)chunk;
    taken += chunk;
  }

  return (int32_t)taken;
}

/**
 * Buffers a read only string, copied through a window so the source is
 * never addressed directly.
 */
int32_t FileWriteStream::write_ro(const char *c_str) {

  if (nullptr == c_str || !isValid() || m_failed) {
    return 0;
  }

  uint32_t len = strlen_ro(c_str);
  uint32_t done = 0;
  char window[RO_WINDOW];

  while (done < len) {
    uint32_t chunk = len - done;
    if (chunk > RO_WINDOW) chunk = RO_WINDOW;

    memcpy_ro(window, c_str + done, chunk);

    int32_t put = write((const uint8_t *)window, chunk);
    done += (uint32_t)put;

    if ((uint32_t)put < chunk) {
      break;
    }
  }

  return (int32_t)done;
}

/**
 * Write only, so this yields nothing.
 */
uint8_t FileWriteStream::read() {
  return 0;
}

/**
 * Write only, so this yields nothing.
 */
int32_t FileWriteStream::read(uint8_t *buf, uint32_t size) {
  return 0;
}

/**
 * Write only, so there is never anything to read.
 */
int32_t FileWriteStream::available() {
  return 0;
}

/**
 * Whether the stream is still able to take bytes.
 */
bool FileWriteStream::availableforwrite(uint32_t size) {
  return isValid() && !m_failed;
}

/**
 * A transmit flush commits the buffer without closing the stream.
 */
void FileWriteStream::flush(int16_t flushtype) {

  if (IsFlushTx(flushtype)) {
    commit();

    // a held handle keeps the tail in the backend's cache, so an explicit
    // flush has to reach storage for the bytes to be visible to a reader
    if (m_handle >= 0) {
      __i_fs.syncFile(m_handle);
    }
  }
}

#endif
