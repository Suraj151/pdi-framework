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

#define __UCLIBCXX_COMPILE_ISTREAM__ 1

#include "istream"


namespace pdistd{

#ifdef __UCLIBCXX_EXPAND_ISTREAM_CHAR__

	template <> _UCXXEXPORT string _readToken<char, char_traits<char> >(istream & stream)
	{
		string temp;
		char_traits<char>::int_type c;
		while(true){
			c = stream.rdbuf()->sgetc();
			if(c != char_traits<char>::eof() && isspace(c) == false){
				stream.rdbuf()->sbumpc();
				temp.append(1, char_traits<char>::to_char_type(c));
			}else{
				break;
			}
		}
		if (temp.size() == 0)
			stream.setstate(ios_base::eofbit|ios_base::failbit);

		return temp;
        }

	template _UCXXEXPORT istream::int_type istream::get();
	template _UCXXEXPORT istream & istream::get(char &c);

	template _UCXXEXPORT istream & istream::operator>>(bool &n);
	template _UCXXEXPORT istream & istream::operator>>(short &n);
	template _UCXXEXPORT istream & istream::operator>>(unsigned short &n);
	template _UCXXEXPORT istream & istream::operator>>(int &n);
	template _UCXXEXPORT istream & istream::operator>>(unsigned int &n);
	template _UCXXEXPORT istream & istream::operator>>(long unsigned &n);
	template _UCXXEXPORT istream & istream::operator>>(long int &n);
	template _UCXXEXPORT istream & istream::operator>>(void *& p);
	template _UCXXEXPORT istream & operator>>(istream & is, char & c);


#ifdef __UCLIBCXX_HAS_FLOATS__
	template _UCXXEXPORT istream & istream::operator>>(float &f);
	template _UCXXEXPORT istream & istream::operator>>(double &f);
	template _UCXXEXPORT istream & istream::operator>>(long double &f);
#endif

	template _UCXXEXPORT void __skipws(basic_istream<char, char_traits<char> >& is);

#endif


}

