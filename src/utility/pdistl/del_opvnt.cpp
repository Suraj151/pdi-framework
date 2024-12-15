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
#include "new"
#include "cstdlib"
#include "func_exception"

#ifndef NO_NOTHROW
_UCXXEXPORT void operator delete[](void* ptr, const pdistd::nothrow_t& ) _UCXX_USE_NOEXCEPT{
	free(ptr);
}
#endif
