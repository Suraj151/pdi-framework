/**************************** Exception Notifier ******************************
This file is part of the Ewings Esp Stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st June 2019
******************************************************************************/
#ifndef _ESPSAVECRASHSPIFFS_H_
#define _ESPSAVECRASHSPIFFS_H_

#include <utility/Utility.h>
#ifdef ENABLE_EMAIL_SERVICE
#include <service_provider/EmailServiceProvider.h>
#endif
#include <FS.h>
#include "user_interface.h"
#include <interface/pdi.h>


/**
 * Structure of the single crash data set
 *
 *  1. Crash time
 *  2. Restart reason
 *  3. Exception cause
 *  4. epc1
 *  5. epc2
 *  6. epc3
 *  7. excvaddr
 *  8. depc
 *  9. adress of stack start
 * 10. adress of stack end
 * 11. stack trace bytes
 *     ...
 */
#define CRASHFILEPATH "/crashdump.txt"
#define CRASHFILEMAXSIZE 102400
#define CRASH_HANDLER_DURATION 60000

void beginCrashHandler( void );
void handleCrashData( void );
void readCrashFileToBuffer(String &_filepath, String &_filedata, uint16_t &_size);
void saveCrashToSpiffs(struct rst_info *rst_info, uint32_t stack, uint32_t stack_end, Print& outputDev );
void clearCrashFile( void );
// #ifdef EW_SERIAL_LOG
// void dummyCrash( void );
// #endif

#endif
