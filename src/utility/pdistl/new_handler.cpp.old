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

const pdistd::nothrow_t pdistd::nothrow = { };

//Name selected to be compatable with g++ code
pdistd::new_handler __new_handler;

_UCXXEXPORT pdistd::new_handler pdistd::set_new_handler(pdistd::new_handler new_p) _UCXX_USE_NOEXCEPT{
	pdistd::new_handler retval = __new_handler;
	__new_handler = new_p;
	return retval;
}
