/*************************** Sound Driver Interface ***************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

A driver that captures sound, telling a recorder the shape of the samples its
/dev node gives.

Author          : Suraj I.
created Date    : 19th Oct 2026
******************************************************************************/

#ifndef _I_SOUND_DRIVER_INTERFACE_H_
#define _I_SOUND_DRIVER_INTERFACE_H_

#include "iDriverInterface.h"

/**
 * iSoundDriverInterface class
 */
class iSoundDriverInterface : public iDriverInterface
{
public:

  /**
   * iSoundDriverInterface destructor.
   */
  virtual ~iSoundDriverInterface() {}

  /**
   * Samples per second.
   */
  virtual uint32_t rate() const = 0;

  /**
   * Bits in one sample.
   */
  virtual uint8_t bits() const { return 16; }

  /**
   * Samples taken at once, one per channel.
   */
  virtual uint8_t channels() const { return 1; }

  /**
   * A sound driver presents a capture device.
   */
  uint8_t driverClass() const override { return DRIVER_CLASS_SOUND_CAPTURE; }
};

#endif
