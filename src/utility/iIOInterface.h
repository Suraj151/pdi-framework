/************************** Input/Output Interface ***************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st Jan 2024
******************************************************************************/

#ifndef _I_IO_INTERFACE_H_
#define _I_IO_INTERFACE_H_

#include "DataTypeDef.h"

/**
 * iIOInterface class
 */
class iIOInterface
{

public:
  /**
   * iIOInterface constructor.
   */
  iIOInterface() {}
  /**
   * iIOInterface destructor.
   */
  virtual ~iIOInterface() {}

  // connect/disconnect api
  virtual int16_t connect(const uint8_t *host, uint16_t port) { return -1; }
  virtual int16_t connect(uint16_t port, uint64_t speed) { return -1; }
  virtual int16_t disconnect() = 0;
  virtual int16_t open(uint16_t port=0, uint64_t speed=115200) { return connect(port, speed); }
  virtual int16_t close() { return disconnect(); }

  // data sending api
  virtual uint32_t write(uint8_t c) = 0;
  virtual uint32_t write(const uint8_t *c_str) = 0;
  virtual uint32_t write(const uint8_t *c_str, uint32_t size) = 0;

  virtual uint32_t write(char c) { return write((uint8_t)c); }
  virtual uint32_t write(const char *c_str) { return write((const uint8_t*)c_str); }
  virtual uint32_t write(const char *c_str, uint32_t size) { return write((const uint8_t*)c_str, size); }

  virtual uint32_t putln() { return write("\r\n"); }
  virtual uint32_t writeln(char c) { return (write(c) + putln()); }
  virtual uint32_t writeln(const char *c_str="") { return (write(c_str) + putln()); }
  virtual uint32_t writeln(const char *c_str, uint32_t size) { return (write(c_str, size) + putln()); }

  virtual uint32_t write(int32_t val){
    char tembuff[25];
 		memset(tembuff, 0, 25);
		sprintf(tembuff, "%d", val);
    return write(tembuff);
  }

  virtual uint32_t write(int64_t val){
    char tembuff[25];
 		memset(tembuff, 0, 25);
		sprintf(tembuff, "%ld", val);
    return write(tembuff);
  }

  virtual uint32_t write(uint32_t val, bool hex=false, bool cap=false){
    char tembuff[25];
 		memset(tembuff, 0, 25);
		sprintf(tembuff, hex ? ( cap ? "%X" : "%x") : "%u", val);
    return write(tembuff);
  }

  virtual uint32_t write(double val){
    char tembuff[25];
 		memset(tembuff, 0, 25);
		sprintf(tembuff, "%f", val);
    return write(tembuff);
  }

  virtual uint32_t writeln(int32_t val) { return (write(val) + putln()); }
  virtual uint32_t writeln(int64_t val) { return (write(val) + putln()); }
  virtual uint32_t writeln(double val) { return (write(val) + putln()); }
  virtual uint32_t writeln(uint32_t val, bool hex=false, bool cap=false){ return (write(val,hex,cap) + putln()); }

  virtual uint32_t write_ro(const char *c_str) { return -1; }
  virtual uint32_t writeln_ro(const char *c_str) { return (write_ro(c_str) + putln()); }

  // received data read api
  virtual uint8_t read() = 0;
  virtual uint32_t read(uint8_t *buf, uint32_t size) = 0;

  // useful api
  virtual int32_t available() = 0;
  virtual int8_t connected() = 0;
  virtual int8_t isopen() { return connected(); }
  virtual void flush() = 0;
};

/**
 * iTerminalInterface class
 */
using iTerminalInterface = iIOInterface;

#endif
