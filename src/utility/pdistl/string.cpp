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

#define __UCLIBCXX_COMPILE_STRING__ 1

#include "basic_definitions"
#include "char_traits"
#include "string"
#include "string_iostream"
#include <string.h>
#include "ostream"

namespace pdistd{

#ifdef __UCLIBCXX_EXPAND_STRING_CHAR__

#ifdef __UCLIBCXX_EXPAND_CONSTRUCTORS_DESTRUCTORS__

	template _UCXXEXPORT string::basic_string(const allocator<char> &);
	template _UCXXEXPORT string::basic_string(size_type n, char c, const allocator<char> & );
	template _UCXXEXPORT string::basic_string(const char* s, const allocator<char>& al);
	template _UCXXEXPORT string::basic_string(const basic_string& str, size_type pos, size_type n, const allocator<char>& al);
	template _UCXXEXPORT string::~basic_string();

#endif // __UCLIBCXX_EXPAND_CONSTRUCTORS_DESTRUCTORS__

	template _UCXXEXPORT string & string::append(const char * s, size_type n);

	template _UCXXEXPORT string::size_type string::find(const string & str, size_type pos) const;
	template _UCXXEXPORT string::size_type string::find(const char* s, size_type pos) const;
	template _UCXXEXPORT string::size_type string::find (char c, size_type pos) const;
	template _UCXXEXPORT string::size_type string::rfind(const string & str, size_type pos) const;
	template _UCXXEXPORT string::size_type string::rfind(char c, size_type pos) const;
	template _UCXXEXPORT string::size_type string::rfind(const char* s, size_type pos) const;

	template _UCXXEXPORT string::size_type string::find_first_of(const string &, size_type) const;
	template _UCXXEXPORT string::size_type string::find_first_of(const char *, size_type pos, size_type n) const;
	template _UCXXEXPORT string::size_type string::find_first_of(const char*, size_type pos) const;
	template _UCXXEXPORT string::size_type string::find_first_of(char c, size_type pos) const;

	template _UCXXEXPORT string::size_type string::find_last_of (const string & , size_type pos) const;
	template _UCXXEXPORT string::size_type string::find_last_of (const char* s, size_type pos, size_type n) const;
	template _UCXXEXPORT string::size_type string::find_last_of (const char* s, size_type pos) const;
	template _UCXXEXPORT string::size_type string::find_last_of (char c, size_type pos) const;

	template _UCXXEXPORT string::size_type string::find_first_not_of(const string &, size_type) const;
	template _UCXXEXPORT string::size_type string::find_first_not_of(const char*, size_type, size_type) const;
	template _UCXXEXPORT string::size_type string::find_first_not_of(const char*, size_type) const;
	template _UCXXEXPORT string::size_type string::find_first_not_of(char c, size_type) const;

	template _UCXXEXPORT int string::compare(const string & str) const;
//	template _UCXXEXPORT int string::compare(size_type pos1, size_type n1, const string & str) const;
	template _UCXXEXPORT int string::compare(
		size_type pos1, size_type n1, const string & str, size_type pos2, size_type n2) const;

	template _UCXXEXPORT string string::substr(size_type pos, size_type n) const;

	template _UCXXEXPORT string & string::operator=(const string & str);
	template _UCXXEXPORT string & string::operator=(const char * s);

	template _UCXXEXPORT bool operator==(const string & lhs, const string & rhs);
	template _UCXXEXPORT bool operator==(const char * lhs, const string & rhs);
	template _UCXXEXPORT bool operator==(const string & lhs, const char * rhs);

	template _UCXXEXPORT bool operator!=(const string & lhs, const string & rhs);
	template _UCXXEXPORT bool operator!=(const char * lhs, const string & rhs);
	template _UCXXEXPORT bool operator!=(const string & lhs, const char * rhs);

	template _UCXXEXPORT string operator+(const string & lhs, const char* rhs);
	template _UCXXEXPORT string operator+(const char* lhs, const string & rhs);
	template _UCXXEXPORT string operator+(const string & lhs,	const string & rhs);

	template _UCXXEXPORT bool operator> (const string & lhs, const string & rhs);
	template _UCXXEXPORT bool operator< (const string & lhs, const string & rhs);


//Functions dependent upon OSTREAM
#ifdef __UCLIBCXX_EXPAND_OSTREAM_CHAR__

template _UCXXEXPORT ostream & operator<<(ostream & os, const string & str);

#endif


//Functions dependent upon ISTREAM
#ifdef __UCLIBCXX_EXPAND_ISTREAM_CHAR__

template _UCXXEXPORT istream & operator>>(istream & is, string & str);


#endif


#endif

}
