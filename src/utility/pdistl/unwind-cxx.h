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

#ifndef _UNWIND_CXX_H
#define _UNWIND_CXX_H 1

// Level 2: C++ ABI

#include "typeinfo"
#include "exception"
#include "cstddef"
#include "unwind.h"

#ifdef __aarch64__
typedef long _Atomic_word;
#elif defined __cris__
typedef int _Atomic_word __attribute__ ((__aligned__ (4)));
#else
typedef int _Atomic_word;
#endif

#pragma GCC visibility push(default)

namespace __cxxabiv1
{

// A primary C++ exception object consists of a header, which is a wrapper
// around an unwind object header with additional C++ specific information,
// followed by the exception object itself.

struct __cxa_exception
{ 
  // Manage the exception object itself.
  pdistd::type_info *exceptionType;
  void (*exceptionDestructor)(void *); 

  // The C++ standard has entertaining rules wrt calling set_terminate
  // and set_unexpected in the middle of the exception cleanup process.
  pdistd::unexpected_handler unexpectedHandler;
  pdistd::terminate_handler terminateHandler;

  // The caught exception stack threads through here.
  __cxa_exception *nextException;

  // How many nested handlers have caught this exception.  A negated
  // value is a signal that this object has been rethrown.
  int handlerCount;

  // Cache parsed handler data from the personality routine Phase 1
  // for Phase 2 and __cxa_call_unexpected.
  int handlerSwitchValue;
  const unsigned char *actionRecord;
  const unsigned char *languageSpecificData;
  _Unwind_Ptr catchTemp;
  void *adjustedPtr;

  // The generic exception header.  Must be last.
  _Unwind_Exception unwindHeader;
};

struct __cxa_refcounted_exception
{
  // Manage this header.
  _Atomic_word referenceCount;
  // __cxa_exception must be last, and no padding can be after it.
  __cxa_exception exc;
};

// A dependent C++ exception object consists of a header, which is a wrapper
// around an unwind object header with additional C++ specific information,
// followed by the exception object itself.
struct __cxa_dependent_exception
{
  // The primary exception
  void *primaryException;

  // The C++ standard has entertaining rules wrt calling set_terminate
  // and set_unexpected in the middle of the exception cleanup process.
  pdistd::unexpected_handler unexpectedHandler;
  pdistd::terminate_handler terminateHandler;

  // The caught exception stack threads through here.
  __cxa_exception *nextException;

  // How many nested handlers have caught this exception.  A negated
  // value is a signal that this object has been rethrown.
  int handlerCount;

  // Cache parsed handler data from the personality routine Phase 1
  // for Phase 2 and __cxa_call_unexpected.
  int handlerSwitchValue;
  const unsigned char *actionRecord;
  const unsigned char *languageSpecificData;
  _Unwind_Ptr catchTemp;
  void *adjustedPtr;

  // The generic exception header.  Must be last.
  _Unwind_Exception unwindHeader;
};


// Each thread in a C++ program has access to a __cxa_eh_globals object.
struct __cxa_eh_globals
{
  __cxa_exception *caughtExceptions;
  unsigned int uncaughtExceptions;
};


// The __cxa_eh_globals for the current thread can be obtained by using
// either of the following functions.  The "fast" version assumes at least
// one prior call of __cxa_get_globals has been made from the current
// thread, so no initialization is necessary.
extern "C" __cxa_eh_globals *__cxa_get_globals () _UCXX_USE_NOEXCEPT;
extern "C" __cxa_eh_globals *__cxa_get_globals_fast () _UCXX_USE_NOEXCEPT;

// Allocate memory for the primary exception plus the thrown object.
extern "C" void *__cxa_allocate_exception(pdistd::size_t thrown_size) _UCXX_USE_NOEXCEPT;
// Allocate memory for dependent exception.
extern "C" __cxa_dependent_exception *__cxa_allocate_dependent_exception() _UCXX_USE_NOEXCEPT;

// Free the space allocated for the primary exception.
extern "C" void __cxa_free_exception(void *thrown_exception) _UCXX_USE_NOEXCEPT;
// Free the space allocated for the dependent exception.
extern "C" void __cxa_free_dependent_exception(__cxa_dependent_exception *dependent_exception) _UCXX_USE_NOEXCEPT;

// Throw the exception.
extern "C" void __cxa_throw (void *thrown_exception,
			     pdistd::type_info *tinfo,
			     void (*dest) (void *))
     __attribute__((noreturn));

// Used to implement exception handlers.
extern "C" void *__cxa_begin_catch (void *) _UCXX_USE_NOEXCEPT;
extern "C" void __cxa_end_catch ();
extern "C" void __cxa_rethrow () __attribute__((noreturn));

// These facilitate code generation for recurring situations.
extern "C" void __cxa_bad_cast ();
extern "C" void __cxa_bad_typeid ();

// @@@ These are not directly specified by the IA-64 C++ ABI.

// Handles re-checking the exception specification if unexpectedHandler
// throws, and if bad_exception needs to be thrown.  Called from the
// compiler.
extern "C" void __cxa_call_unexpected (void *) __attribute__((noreturn));

// Invokes given handler, dying appropriately if the user handler was
// so inconsiderate as to return.
extern void __terminate(pdistd::terminate_handler) _UCXX_USE_NOEXCEPT __attribute__((noreturn));
extern void __unexpected(pdistd::unexpected_handler) __attribute__((noreturn));

// The current installed user handlers.
extern pdistd::terminate_handler __terminate_handler;
extern pdistd::unexpected_handler __unexpected_handler;

// These are explicitly GNU C++ specific.

// This is the exception class we report -- "GNUCC++\0".
const _Unwind_Exception_Class __gxx_exception_class
#ifndef __ARM_EABI_UNWINDER__
= ((((((((_Unwind_Exception_Class) 'G' 
	 << 8 | (_Unwind_Exception_Class) 'N')
	<< 8 | (_Unwind_Exception_Class) 'U')
       << 8 | (_Unwind_Exception_Class) 'C')
      << 8 | (_Unwind_Exception_Class) 'C')
     << 8 | (_Unwind_Exception_Class) '+')
    << 8 | (_Unwind_Exception_Class) '+')
   << 8 | (_Unwind_Exception_Class) '\0');
#else
= "GNUCC++";
#endif

// GNU C++ personality routine, Version 0.
extern "C" _Unwind_Reason_Code __gxx_personality_v0
     (int, _Unwind_Action, _Unwind_Exception_Class,
      struct _Unwind_Exception *, struct _Unwind_Context *);

// GNU C++ sjlj personality routine, Version 0.
extern "C" _Unwind_Reason_Code __gxx_personality_sj0
     (int, _Unwind_Action, _Unwind_Exception_Class,
      struct _Unwind_Exception *, struct _Unwind_Context *);

// Acquire the C++ exception header from the C++ object.
static inline __cxa_exception *
__get_exception_header_from_obj (void *ptr)
{
  return reinterpret_cast<__cxa_exception *>(ptr) - 1;
}

// Acquire the C++ exception header from the generic exception header.
static inline __cxa_exception *
__get_exception_header_from_ue (_Unwind_Exception *exc)
{
  return reinterpret_cast<__cxa_exception *>(exc + 1) - 1;
}

// Acquire the C++ refcounted exception header from the C++ object.
static inline __cxa_refcounted_exception *
__get_refcounted_exception_header_from_obj (void *ptr)
{
  return reinterpret_cast<__cxa_refcounted_exception *>(ptr) - 1;
}

// Acquire the C++ refcounted exception header from the generic exception
// header.
static inline __cxa_refcounted_exception *
__get_refcounted_exception_header_from_ue (_Unwind_Exception *exc)
{
  return reinterpret_cast<__cxa_refcounted_exception *>(exc + 1) - 1;
}

} /* namespace __cxxabiv1 */

#pragma GCC visibility pop

#endif // _UNWIND_CXX_H
