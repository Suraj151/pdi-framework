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

#define __UCLIBCXX_COMPILE_VECTOR__ 1


#include "vector"

namespace pdistd{


#ifdef __UCLIBCXX_EXPAND_VECTOR_BASIC__

#ifdef __UCLIBCXX_EXPAND_CONSTRUCTORS_DESTRUCTORS__

	template _UCXXEXPORT vector<char, allocator<char> >::vector(const allocator<char>& al);
	template _UCXXEXPORT vector<char, allocator<char> >::vector(size_type n, const char & value, const allocator<char> & al);

	template _UCXXEXPORT vector<char, allocator<char> >::~vector();
	template _UCXXEXPORT vector<unsigned char, allocator<unsigned char> >::~vector();

#endif //__UCLIBCXX_EXPAND_CONSTRUCTORS_DESTRUCTORS__

	template _UCXXEXPORT void vector<char, allocator<char> >::reserve(size_type n);
	template _UCXXEXPORT void vector<unsigned char, allocator<unsigned char> >::reserve(size_type n);
	template _UCXXEXPORT void vector<short int, allocator<short int> >::reserve(size_type n);
	template _UCXXEXPORT void vector<unsigned short int, allocator<unsigned short int> >::reserve(size_type n);
	template _UCXXEXPORT void vector<int, allocator<int> >::reserve(size_type n);
	template _UCXXEXPORT void vector<unsigned int, allocator<unsigned int> >::reserve(size_type n);
	template _UCXXEXPORT void vector<long int, allocator<long int> >::reserve(size_type n);
	template _UCXXEXPORT void vector<unsigned long int, allocator<unsigned long int> >::reserve(size_type n);
	template _UCXXEXPORT void vector<float, allocator<float> >::reserve(size_type n);
	template _UCXXEXPORT void vector<double, allocator<double> >::reserve(size_type n);
	template _UCXXEXPORT void vector<bool, allocator<bool> >::reserve(size_type n);

	template _UCXXEXPORT void vector<char, allocator<char> >::resize(size_type sz, const char & c);
	template _UCXXEXPORT void vector<unsigned char, allocator<unsigned char> >::resize(size_type sz, const unsigned char & c);
	template _UCXXEXPORT void vector<short int, allocator<short int> >::resize(size_type sz, const short & c);
	template _UCXXEXPORT void vector<unsigned short int, allocator<unsigned short int> >
		::resize(size_type sz, const unsigned short int & c);
	template _UCXXEXPORT void vector<int, allocator<int> >::resize(size_type sz, const int & c);
	template _UCXXEXPORT void vector<unsigned int, allocator<unsigned int> >::resize(size_type sz, const unsigned int & c);
	template _UCXXEXPORT void vector<long int, allocator<long int> >::resize(size_type sz, const long int & c);
	template _UCXXEXPORT void vector<unsigned long int, allocator<unsigned long int> >::
		resize(size_type sz, const unsigned long int & c);
	template _UCXXEXPORT void vector<float, allocator<float> >::resize(size_type sz, const float & c);
	template _UCXXEXPORT void vector<double, allocator<double> >::resize(size_type sz, const double & c);
	template _UCXXEXPORT void vector<bool, allocator<bool> >::resize(size_type sz, const bool & c);

#elif defined __UCLIBCXX_EXPAND_STRING_CHAR__


#ifdef __UCLIBCXX_EXPAND_CONSTRUCTORS_DESTRUCTORS__
	template _UCXXEXPORT vector<char, allocator<char> >::vector(const allocator<char>& al);
	template _UCXXEXPORT vector<char, allocator<char> >::vector(size_type n, const char & value, const allocator<char> & al);
	template _UCXXEXPORT vector<char, allocator<char> >::~vector();
#endif // __UCLIBCXX_EXPAND_CONSTRUCTORS_DESTRUCTORS__

	template _UCXXEXPORT void vector<char, allocator<char> >::reserve(size_type n);
	template _UCXXEXPORT void vector<char, allocator<char> >::resize(size_type sz, const char & c);

#endif




}
