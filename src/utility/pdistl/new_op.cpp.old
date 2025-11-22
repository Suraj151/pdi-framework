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

// Arduino 1.0 contains an implementation for this.
#if ARDUINO < 100

#include "new"
#include "cstdlib"
#include "func_exception"

_UCXXEXPORT void* operator new(pdistd::size_t numBytes) _UCXX_THROW(pdistd::bad_alloc)
{
	//C++ stardard 5.3.4.8 requires that a valid pointer be returned for
	//a call to new(0). Thus:
	if(numBytes == 0){
		numBytes = 1;
	}
	void * p = malloc(numBytes);
	if(p == 0){
		pdistd::__throw_bad_alloc();
	}
	return p;
}

#endif
