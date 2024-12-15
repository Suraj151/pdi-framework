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

#include "cstdlib"
#include "cstring"
#include "func_exception"

//This is a system-specific header which does all of the error-handling management
#include "unwind-cxx.h"

namespace __cxxabiv1
{

extern "C" void * __cxa_allocate_exception(pdistd::size_t thrown_size) _UCXX_USE_NOEXCEPT{
	void *e;
	// The sizeof crap is required by Itanium ABI because we need to
	// provide space for accounting information which is implementation
	// (gcc) defined.
	e = malloc (thrown_size + sizeof(__cxa_refcounted_exception));
	if (0 == e){
		pdistd::terminate();
	}
	memset (e, 0, sizeof(__cxa_refcounted_exception));
	return (void *)((unsigned char *)e + sizeof(__cxa_refcounted_exception));
}

extern "C" void __cxa_free_exception(void *vptr) _UCXX_USE_NOEXCEPT{
	free( (char *)(vptr) - sizeof(__cxa_refcounted_exception) );
}


extern "C" __cxa_dependent_exception * __cxa_allocate_dependent_exception() _UCXX_USE_NOEXCEPT{
	__cxa_dependent_exception *retval;
	// The sizeof crap is required by Itanium ABI because we need to
	// provide space for accounting information which is implementation
	// (gcc) defined.
	retval = static_cast<__cxa_dependent_exception*>(malloc (sizeof(__cxa_dependent_exception)));
	if (0 == retval){
		pdistd::terminate();
	}
	memset (retval, 0, sizeof(__cxa_dependent_exception));
	return retval;
}

extern "C" void __cxa_free_dependent_exception(__cxa_dependent_exception *vptr) _UCXX_USE_NOEXCEPT{
	free( (char *)(vptr) );
}

}  /* namespace __cxxabiv1 */
