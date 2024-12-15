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

#define __UCLIBCXX_COMPILE_STREAMBUF__ 1

#include "streambuf"

namespace pdistd{

#ifdef __UCLIBCXX_EXPAND_STREAMBUF_CHAR__

#ifdef __UCLIBCXX_EXPAND_CONSTRUCTORS_DESTRUCTORS__

	template _UCXXEXPORT streambuf::basic_streambuf();
	template _UCXXEXPORT streambuf::~basic_streambuf();

#endif

	template _UCXXEXPORT locale streambuf::pubimbue(const locale &loc);
	template _UCXXEXPORT streamsize streambuf::in_avail();
	template _UCXXEXPORT streambuf::int_type streambuf::sbumpc();
	template _UCXXEXPORT streambuf::int_type streambuf::snextc();
	template _UCXXEXPORT streambuf::int_type streambuf::sgetc();
	template _UCXXEXPORT streambuf::int_type streambuf::sputbackc(char_type c);
	template _UCXXEXPORT streambuf::int_type streambuf::sungetc();
	template _UCXXEXPORT streambuf::int_type streambuf::sputc(char_type c);

#endif


}


