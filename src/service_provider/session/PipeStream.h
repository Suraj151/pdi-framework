/****************************** Pipe Stream ***********************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

A byte fifo that presents itself as a terminal, so one stage of a pipeline can
write into it and the next can read out of it. Capacity is fixed at construction
and the buffer is released with the object.

Author          : Suraj I.
created Date    : 25th Aug 2026
******************************************************************************/
#ifndef _PIPE_STREAM_H_
#define _PIPE_STREAM_H_

#include <config/Config.h>
#include <utility/iIOInterface.h>
#include <utility/queue/ringbuf.h>

#ifdef ENABLE_CMD_SERVICE

class PipeStream : public iTerminalInterface {

public:

  /**
   * Takes a buffer of the given capacity, which is released with the object.
   */
  PipeStream(uint16_t capacity = PDI_PIPE_CAPACITY);
  virtual ~PipeStream();

  /**
   * False when the buffer could not be taken, in which case every write is
   * refused rather than silently dropped.
   */
  bool isValid() const { return nullptr != m_buffer; }

  /**
   * True once a write has been refused for want of room. The caller decides
   * whether a truncated pipeline is worth reporting.
   */
  bool overflowed() const { return m_overflowed; }

  /**
   * A pipe that overflowed, or never got its buffer, takes no further write.
   */
  bool writeRefused() const override { return m_overflowed || !isValid(); }

  /**
   * Bytes the pipe can hold between a write and the read that drains it.
   */
  uint16_t capacity() const { return m_capacity; }

  /**
   * A pipe holds no connection of its own, so this always reports success.
   */
  int16_t disconnect() override;

  /**
   * Usable for as long as it holds a buffer.
   */
  int8_t connected() override;

  using iTerminalInterface::write;
  using iTerminalInterface::read;

  /**
   * Appends one byte, marking the pipe overflowed when it will not fit.
   */
  int32_t write(uint8_t c) override;

  /**
   * Appends a terminated string, returning how much of it fitted.
   */
  int32_t write(const uint8_t *c_str) override;

  /**
   * Appends a bounded run of bytes, returning how much of it fitted.
   */
  int32_t write(const uint8_t *c_str, uint32_t size) override;

  /**
   * Appends a read only string, copied through a window so the source is
   * never addressed directly.
   */
  int32_t write_ro(const char *c_str) override;

  /**
   * Takes the oldest byte, or zero when nothing is buffered.
   */
  uint8_t read() override;

  /**
   * Takes up to size bytes, returning how many came out.
   */
  int32_t read(uint8_t *buf, uint32_t size) override;

  /**
   * Bytes written into the pipe and not yet read back.
   */
  int32_t available() override;

  /**
   * Whether size further bytes would fit without overflowing.
   */
  bool availableforwrite(uint32_t size) override;

  /**
   * A receive flush discards whatever has not been read; there is nothing
   * buffered on the way out.
   */
  void flush(int16_t flushtype = FLUSH_TX) override;

private:

  RINGBUF m_ring;
  uint8_t *m_buffer;
  uint16_t m_capacity;
  bool m_overflowed;
};

#endif

#endif
