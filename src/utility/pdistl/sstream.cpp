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


#define __UCLIBCXX_COMPILE_SSTREAM__ 1

#include "sstream"

namespace pdistd{

#ifdef __UCLIBCXX_EXPAND_SSTREAM_CHAR__

	typedef char_traits<char> tr_ch;
	typedef basic_stringbuf<char, tr_ch, allocator<char> > char_stringbuf;

#ifdef __UCLIBCXX_EXPAND_CONSTRUCTORS_DESTRUCTORS__

	template _UCXXEXPORT char_stringbuf::basic_stringbuf(ios_base::openmode which);
	template _UCXXEXPORT char_stringbuf::~basic_stringbuf();

#endif //__UCLIBCXX_EXPAND_CONSTRUCTORS_DESTRUCTORS__

	template _UCXXEXPORT basic_string<char, char_traits<char>, allocator<char> > char_stringbuf::str() const;
	template _UCXXEXPORT char_stringbuf::int_type char_stringbuf::pbackfail(char_stringbuf::int_type c);
	template _UCXXEXPORT char_stringbuf::int_type char_stringbuf::overflow(char_stringbuf::int_type c);
	template _UCXXEXPORT char_stringbuf::pos_type
		char_stringbuf::seekoff(char_stringbuf::off_type, ios_base::seekdir, ios_base::openmode);
	template _UCXXEXPORT char_stringbuf::int_type char_stringbuf::underflow ();
	template _UCXXEXPORT streamsize char_stringbuf::xsputn(const char* s, streamsize n);

#ifdef __UCLIBCXX_EXPAND_CONSTRUCTORS_DESTRUCTORS__

	template _UCXXEXPORT basic_stringstream<char, tr_ch, allocator<char> >::basic_stringstream(ios_base::openmode which);
	template _UCXXEXPORT basic_istringstream<char, tr_ch, allocator<char> >::~basic_istringstream();
	template _UCXXEXPORT basic_ostringstream<char, tr_ch, allocator<char> >::~basic_ostringstream();
	template _UCXXEXPORT basic_stringstream<char, tr_ch, allocator<char> >::~basic_stringstream();

#endif //__UCLIBCXX_EXPAND_CONSTRUCTORS_DESTRUCTORS__

#endif

}


