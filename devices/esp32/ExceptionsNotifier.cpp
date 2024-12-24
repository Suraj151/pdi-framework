/**************************** Exception Notifier ******************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st June 2019
******************************************************************************/
#include "ExceptionsNotifier.h"
#include <config/Config.h>
#ifdef ENABLE_EMAIL_SERVICE
#include <service_provider/EmailServiceProvider.h>
#endif

#if defined(ENABLE_EXCEPTION_NOTIFIER)

/**
 * begin crash handler
 */
void beginCrashHandler()
{

	SPIFFS.begin();
	__task_scheduler.setInterval([&]()
								 { handleCrashData(); },
								 CRASH_HANDLER_DURATION, __i_dvc_ctrl.millis_now());
}

// void dummyCrash() {
//
// 	if (Serial.available() > 0){
//
//     char inChar = Serial.read();
//
//     if(inChar) {
//
//     switch (inChar) {
//
//       case '0':
//         Serial.println(F("Attempting to divide by zero ..."));
//         int result, zero;
//         zero = 0;
//         result = 1 / zero;
//         Serial.print(F("Result = "));
//         Serial.println(result);
//         break;
//       case 'e':
//         Serial.println(F("Attempting to read through a pointer to no object ..."));
//         int* nullPointer;
//         nullPointer = NULL;
//         // null pointer dereference - read
//         // attempt to read a value through a null pointer
//         Serial.print(*nullPointer);
//         break;
//       case 'c':
//         clearCrashFile();
//         Serial.println(F("Crash file cleared"));
//         break;
//       default:
//         break;
//       }
//     }
//   }
// }

/**
 * This function returns saved crashdump file as a string
 */
void handleCrashData()
{

	String _filename = "";

	File root = SPIFFS.open("/");
	if (!root) {
		// Serial.println("- failed to open directory");
		return;
	}
	if (!root.isDirectory()) {
		// Serial.println(" - not a directory");
		return;
	}

	File file = root.openNextFile();
	while (file)
	{
		_filename += file.name();
		_filename += String("\tsize : ");
		_filename += file.size();
		_filename += String("\n");
		file = root.openNextFile();
	}

	uint16_t _size = 0;
	String _filepath = CRASHFILEPATH;
	String _filedata = "";
	readCrashFileToBuffer(_filepath, _filedata, _size);
	if (_filename.length() && _size)
	{

		_filedata += "\n\nRegards,\n";
		_filedata += WiFi.macAddress();
		bool _candeletefile = false;
#ifdef ENABLE_EMAIL_SERVICE
		if (__email_service.sendMail(_filedata.c_str()))
		{
			_candeletefile = true;
		}
#endif
		if (_size > CRASHFILEMAXSIZE || _candeletefile)
		{

			clearCrashFile();
		}
	}
}

/**
 * This function returns saved crashdump file as a string
 */
void readCrashFileToBuffer(String &_filepath, String &_filedata, uint16_t &_size)
{

	File _file = SPIFFS.open(_filepath, "r");

	if (_file)
	{
		_size = _file.size();
		char *_buffer = new char[_size + 1];

		if (nullptr != _buffer)
		{
			_size = _file.readBytes(_buffer, _size);
			_file.close();
			_buffer[_size] = 0;

			_filedata = String(_buffer);
			delete[] _buffer;
		}
	}
}

/**
 * This function saves crashdump to spiff
 */
// void saveCrashToSpiffs(struct rst_info *rst_info, uint32_t stack, uint32_t stack_end, Print &outputDev)
// {

// 	__i_dvc_ctrl.feedWdt();
// 	outputDev.printf("\nRestart reason : %d\n", rst_info->reason);
// 	outputDev.printf("Exception (%d):\n", rst_info->exccause);
// 	outputDev.printf("epc1=0x%08x epc2=0x%08x epc3=0x%08x excvaddr=0x%08x depc=0x%08x\n\n", rst_info->epc1, rst_info->epc2, rst_info->epc3, rst_info->excvaddr, rst_info->depc);
// 	outputDev.println(">>>stack>>>\n");

// 	int16_t stackLength = stack_end - stack;
// 	uint32_t stackTrace;

// 	for (int16_t i = 0; i < stackLength; i += 0x10)
// 	{
// 		outputDev.printf("%08x: ", stack + i);
// 		for (byte j = 0; j < 4; j++)
// 		{
// 			uint32_t *byteptr = (uint32_t *)(stack + i + j * 4);
// 			stackTrace = *byteptr;
// 			outputDev.printf("%08x ", stackTrace);
// 		}
// 		outputDev.println();
// 	}
// 	outputDev.println("\n<<<stack<<<");
// }

/**
 * This function is called automatically if ESP suffers an exception
 * It should be kept quick / consise to be able to execute before hardware wdt may kick in
 */
// extern "C" void custom_crash_callback(struct rst_info *rst_info, uint32_t stack, uint32_t stack_end)
// {

// 	class StringPrinter2 : public Print
// 	{
// 	public:
// 		String str = "";
// 		StringPrinter2() {}
// 		virtual size_t write(const uint8_t character) { str += character; return 1; }
// 		virtual size_t write(const uint8_t *buffer, size_t size)
// 		{
// 			String buff = String((const char *)buffer);
// 			buff.remove(size);
// 			str += buff;
// 			return size;
// 		}
// 	} strprinter2;

// 	saveCrashToSpiffs(rst_info, stack, stack_end, strprinter2);
// 	File _file = SPIFFS.open(CRASHFILEPATH, "a");
// 	if (!_file)
// 	{
// 		_file = SPIFFS.open(CRASHFILEPATH, "w");
// 	}
// 	if (_file)
// 	{
// 		unsigned int w = _file.write((uint8_t *)strprinter2.str.c_str(), strprinter2.str.length());
// 		_file.close();
// 	}

// 	LogFmtI("\nStack Trace saved to file : %s\n", CRASHFILEPATH);
// }

/**
 * Clear crash information saved in SPIFFS
 * In fact only crash counter is cleared
 * The crash data are not deleted
 */
void clearCrashFile(void)
{
	String fn = String(F(CRASHFILEPATH));
	SPIFFS.remove(fn);
}

#endif
