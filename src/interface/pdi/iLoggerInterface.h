/*********************** Device logging Interface *****************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st Jan 2024
******************************************************************************/

#ifndef _I_LOGGER_INTERFACE_H_
#define _I_LOGGER_INTERFACE_H_

#include <interface/interface_includes.h>
#include <utility/iIOInterface.h>


/**
 * iLoggerInterface class
 */
class iLoggerInterface
{

public:
  /**
   * iLoggerInterface constructor.
   */
  iLoggerInterface() {}
  /**
   * iLoggerInterface destructor.
   */
  virtual ~iLoggerInterface() {}

  virtual void init(iIOInterface *io = nullptr) = 0;
  virtual void log(logger_type_t log_type, const char *format, ...) = 0;
};

#endif
