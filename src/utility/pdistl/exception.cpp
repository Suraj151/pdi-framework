/***************************** PDI STD File ***********************************
This file is taken from external sources. this is taken from open source uClibc++ 
library available on github. below is reference link. Thanking to author for
providing this .

This is free software. you can redistribute it and/or modify it but without any
warranty.

referred from   : https://github.com/mike-matera/ArduinoSTL
added Date      : 1st Dec 2024
added by        : Suraj I.
******************************************************************************/

#include "exception"

//We can't do this yet because gcc is too stupid to be able to handle
//different implementations of exception class.

#undef __UCLIBCXX_EXCEPTION_SUPPORT__

#ifdef __UCLIBCXX_EXCEPTION_SUPPORT__

namespace pdistd{
	_UCXXEXPORT static char * __std_exception_what_value = "exception";

	//We are providing our own versions to be sneaky


	_UCXXEXPORT exception::~exception() _UCXX_USE_NOEXCEPT{
		//Empty function
	}

	_UCXXEXPORT const char* exception::what() const _UCXX_USE_NOEXCEPT{
		return __std_exception_what_value;
	}

	_UCXXEXPORT bad_exception::~bad_exception() _UCXX_USE_NOEXCEPT{

	}


}


#endif
