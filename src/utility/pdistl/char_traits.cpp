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

#define __UCLIBCXX_COMPILE_CHAR_TRAITS__ 1


#include "basic_definitions"
#include "char_traits"

namespace pdistd{

_UCXXEXPORT const char_traits<char>::char_type* char_traits<char>::find(const char_type* s, int n, const char_type& a){
	for(int i=0; i < n; i++){
		if(eq(s[i], a)){
			return (s+i);
		}
	}
	return 0;
}

_UCXXEXPORT bool char_traits<char>::eq(const char_type& c1, const char_type& c2){
	if(strncmp(&c1, &c2, 1) == 0){
		return true;
	}
	return false;
}

_UCXXEXPORT char_traits<char>::char_type char_traits<char>::to_char_type(const int_type & i){
	if(i > 0 && i <= 255){
		return (char)(unsigned char)i;
	}

	//Out of range
	return 0;
}



#ifdef __UCLIBCXX_HAS_WCHAR__

_UCXXEXPORT const char_traits<wchar_t>::char_type* char_traits<wchar_t>::find(const char_type* s, int n, const char_type& a){
	for(int i=0; i < n; i++){
		if(eq(s[i], a)){
			return (s+i);
		}
	}
	return 0;
}

#endif

}
