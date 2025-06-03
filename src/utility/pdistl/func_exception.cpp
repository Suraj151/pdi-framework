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
#include "func_exception"
#include "stdexcept"
#include "cstdlib"

namespace pdistd{

#ifdef __UCLIBCXX_EXCEPTION_SUPPORT__

_UCXXEXPORT void __throw_bad_alloc(){
	throw bad_alloc();
}

_UCXXEXPORT void __throw_out_of_range( const char * message){
	if(message == 0){
		throw out_of_range();
	}
	throw out_of_range(message);
}

_UCXXEXPORT void __throw_overflow_error( const char * message){
	if(message == 0){
		throw overflow_error();
	}
	throw overflow_error(message);
}

_UCXXEXPORT void __throw_length_error(const char * message){
	if(message == 0){
		throw length_error();
	}
	throw length_error(message);
}

_UCXXEXPORT void __throw_invalid_argument(const char * message){
	if(message == 0){
		throw invalid_argument();
	}
	throw invalid_argument(message);
}

#else

_UCXXEXPORT void __throw_bad_alloc(){
	abort();
}

_UCXXEXPORT void __throw_out_of_range( const char * ){
	abort();
}

_UCXXEXPORT void __throw_overflow_error( const char * ){
	abort();
}

_UCXXEXPORT void __throw_length_error(const char * ){
	abort();
}

_UCXXEXPORT void __throw_invalid_argument(const char *){
	abort();
}

#endif



}
