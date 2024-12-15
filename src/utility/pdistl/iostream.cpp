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

#define __UCLIBCXX_COMPILE_IOSTREAM__ 1

#include "iostream"

namespace pdistd{

#ifdef __UCLIBCXX_EXPAND_OSTREAM_CHAR__
#ifdef __UCLIBCXX_EXPAND_ISTREAM_CHAR__

	template _UCXXEXPORT basic_iostream<char, char_traits<char> >::
		basic_iostream(basic_streambuf<char, char_traits<char> >* sb);
	template _UCXXEXPORT basic_iostream<char, char_traits<char> >::~basic_iostream();

#endif
#endif

}


