/********************** String Operations Utility *****************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st June 2019
******************************************************************************/

#include "DataTypeConversions.h"
#include "StringOperations.h"

/**
 * This function finds and returns substring index from given string if present
 * or returns null if not found.
 *
 * @param   char* str
 * @param   char* substr
 * @param   int|300	_len
 * @return  int
 */
int __strstr(char *str, const char *substr, int _len)
{
	if (nullptr == str || nullptr == substr || 0 == strlen(str) || 0 == strlen(substr))
	{
		return -1;
	}

	int n = 0;

	while (*str && n < _len)
	{
		char *Begin = str;
		char *pattern = (char*)substr;

		while (*str && *pattern && *str == *pattern)
		{
			str++;
			pattern++;
		}
		// if (!*pattern) return Begin;
		if (!*pattern)
			return n;

		str = Begin + 1;
		n++;
	}
	// return NULL;
	return -1;
}

/**
 * This function remove/trim given char from both ends of given string if present.
 *
 * @param   char* str
 * @param   char	_val
 * @param   uint16_t|300	_overflow_limit
 * @return  char*
 */
char *__strtrim_val(char *str, char _val, uint16_t _overflow_limit)
{

	if (nullptr == str)
	{
		return nullptr;
	}

	uint16_t n = 0;
	uint16_t len = strlen(str);

	if (0 == len)
	{
		return nullptr;
	}

	while (*str && n < _overflow_limit)
	{

		char *_begin = str;

		while (*_begin && *_begin == _val && n < len && n < _overflow_limit)
		{
			_begin++;
			n++;
		}
		while (*(str + len - 1) && len > 0 && n < _overflow_limit)
		{
			if (*(str + len - 1) == _val)
			{
				*(str + len - 1) = 0;
				n++;
				len--;
			}
			else
			{
				break;
			}
		}

		return _begin;
	}
	
	return nullptr;
}

/**
 * This function remove/trim blank spaces from both ends of given string if present.
 *
 * @param   char* str
 * @param   uint16_t|300	_overflow_limit
 * @return  char*
 */
char *__strtrim(char *str, uint16_t _overflow_limit)
{
	return __strtrim_val( str, ' ', _overflow_limit);
}


/**
 * This function checks whether given strings are equals or not.
 *
 * @param   char* str1
 * @param   char* str2
 * @param   uint16_t|300	_overflow_limit
 * @return  bool
 */
bool __are_str_equals(const char *str1, const char *str2, uint16_t _overflow_limit)
{

	if (nullptr == str1 || nullptr == str2)
	{
		return false;
	}

	uint16_t len = strlen(str1);
	if (len != strlen(str2))
	{
		return false;
	}

	for (uint16_t i = 0; i < len && i < _overflow_limit; i++)
	{

		if (str1[i] != str2[i])
		{
			return false;
		}
	}
	return true;
}

/**
 * This function checks whether given arrays are equals or not.
 *
 * @param   char* array1
 * @param   char* array2
 * @param   uint16_t|300	_overflow_limit
 * @return  bool
 */
bool __are_arrays_equal(char *array1, char *array2, uint16_t len)
{

	if (nullptr == array1 || nullptr == array2)
	{
		return false;
	}

	for (uint16_t i = 0; i < len; i++)
	{

		if (array1[i] != array2[i])
		{
			return false;
		}
	}
	return true;
}

/**
 * This function appends uint data to char buffer in given format.
 *
 * @param	char* _str
 * @param 	const char* _format
 * @param 	uint32_t	_value
 * @param 	int	_len
 */
void __appendUintToBuff(char *_str, const char *_format, uint32_t _value, int _len)
{

	char value[20];
	memset(value, 0, 20);
	sprintf(value, (const char *)_format, _value);
	strncat(_str, value, _len);
}

/**
 * This function convert int ip address to char string.
 *
 * @param	char* _str
 * @param 	uint8_t*	_ip
 * @param 	int|15	_len
 */
void __int_ip_to_str(char *_str, uint8_t *_ip, int _len)
{
	memset(_str, 0, _len);
	sprintf(_str, "%d.%d.%d.%d", _ip[0], _ip[1], _ip[2], _ip[3]);
}

/**
 * This function convert char string ip address to int.
 *
 * @param	char* _str
 * @param 	uint8_t*	_ip
 * @param 	int|15	_len
 * @param 	bool|true	_clear_str_after_done
 */
void __str_ip_to_int(char *_str, uint8_t *_ip, int _len, bool _clear_str_after_done)
{
	// char _buf[4];
	// memset( _buf, 0, 4 );
	_ip[0] = StringToUint8(_str, 3);
	for (uint8_t i = 0, _ip_index = 1; i < strlen(_str) && i < _len; i++)
	{

		if (_str[i] == '.' && _ip_index < 4)
		{

			_ip[_ip_index++] = StringToUint8(&_str[i + 1], 3);
		}
		// if( _buf_index < 4 ) _buf[_buf_index++] = _str[i];
	}
	if (_clear_str_after_done)
	{
		memset(_str, 0, _len);
	}
}

/**
 * This function finds substring from main string and replace it with given string.
 *
 * @param	char* _str
 * @param	char* _find_str
 * @param	char* _replace_with
 * @param 	int	_occurence
 */
void __find_and_replace(char *_str, const char *_find_str, const char *_replace_with, int _occurence)
{

	if (nullptr == _str || nullptr == _find_str || nullptr == _replace_with)
	{
		return;
	}

	int _str_len = strlen(_str);
	int _find_str_len = strlen(_find_str);
	int _replace_str_len = strlen(_replace_with);

	int _total_len = _str_len + (_replace_str_len * _occurence);
	char *_buf = new char[_total_len];

	if (nullptr == _buf)
	{
		return;
	}

	memset(_buf, 0, _total_len);

	int j = 0, o = 0;
	for (; j < _str_len && o < _occurence;)
	{

		int _occur_index = __strstr(&_str[j], _find_str, _str_len);
		if (_occur_index >= 0)
		{

			strncat(_buf, &_str[j], _occur_index);
			strncat(_buf, _replace_with, _replace_str_len);
			j += _occur_index + _find_str_len;
			o++;
		}
		else
		{
			break;
		}
	}

	if (j < _str_len)
		strncat(_buf, &_str[j], (_str_len - j));
	int _fin_len = _str_len - (o * _find_str_len) + (o * _replace_str_len);

	if (strlen(_buf) > 0 && o > 0)
	{
		memset(_str, 0, _str_len);
		memcpy(_str, _buf, _fin_len + 1);
	}
	delete[] _buf;
}

/**
 * This function parse and find the value of given key from main json string.
 *
 * @param		char* _str
 * @param		char* _key
 * @param		char* _value
 * @param 		int	_max_value_len
 * @return		bool
 */
bool __get_from_json(char *_str, char *_key, char *_value, int _max_value_len)
{

	if (nullptr == _str || nullptr == _key)
	{
		return false;
	}

	int _str_len = strlen(_str);
	int _key_str_len = strlen(_key);

	char *_str_buf = new char[_str_len];

	if (nullptr == _str_buf)
	{
		return false;
	}

	memset(_str_buf, 0, _str_len);

	int _occur_index = __strstr(_str, _key, _str_len);
	if (_occur_index >= 0)
	{

		int j = 0;
		int no_of_commas = 0;
		int no_of_opening_curly_bracket = 0;
		int no_of_closing_curly_bracket = 0;
		int no_of_opening_square_bracket = 0;
		int no_of_closing_square_bracket = 0;

		while (j < _str_len)
		{

			if (_str[_occur_index + _key_str_len + j] == ',')
			{
				no_of_commas++;
			}
			else if (_str[_occur_index + _key_str_len + j] == '{')
			{
				no_of_opening_curly_bracket++;
			}
			else if (_str[_occur_index + _key_str_len + j] == '}')
			{
				no_of_closing_curly_bracket++;
			}
			else if (_str[_occur_index + _key_str_len + j] == '[')
			{
				no_of_opening_square_bracket++;
			}
			else if (_str[_occur_index + _key_str_len + j] == ']')
			{
				no_of_closing_square_bracket++;
			}
			else
			{
			}

			if (
				(no_of_commas > 0 && (no_of_opening_curly_bracket +
									  no_of_closing_curly_bracket + no_of_opening_square_bracket + no_of_closing_square_bracket) == 0))
			{
				break;
			}

			if (no_of_opening_curly_bracket > 0 && no_of_opening_curly_bracket == no_of_closing_curly_bracket)
			{
				break;
			}

			if (no_of_opening_curly_bracket == 0 && no_of_opening_square_bracket > 0 &&
				no_of_opening_square_bracket == no_of_closing_square_bracket)
			{
				break;
			}

			if (no_of_opening_curly_bracket == 0 && no_of_closing_curly_bracket > 0 &&
				no_of_opening_square_bracket == no_of_closing_square_bracket)
			{
				j--;
				break;
			}

			if (no_of_opening_square_bracket == 0 && no_of_closing_square_bracket > 0 &&
				no_of_opening_curly_bracket == no_of_closing_curly_bracket)
			{
				j--;
				break;
			}

			j++;
			//__i_dvc_ctrl.wait(0);
		}

		memcpy(_str_buf, &_str[_occur_index], _key_str_len + j + 1);
		__find_and_replace(_str_buf, "\n", "", 5);
		__find_and_replace(_str_buf, _key, "", 1);
		int _key_value_seperator = __strstr(_str_buf, ":", _str_len);
		memcpy(_str_buf, _str_buf + _key_value_seperator, _key_str_len + j + 1 - _key_value_seperator);
		memcpy(_str_buf, __strtrim_val(_str_buf, ':', _max_value_len), strlen(_str_buf));
		memcpy(_str_buf, __strtrim_val(_str_buf, ',', _max_value_len), strlen(_str_buf));
		memcpy(_str_buf, __strtrim(_str_buf, _max_value_len), strlen(_str_buf));
		memcpy(_str_buf, __strtrim_val(_str_buf, '"', _max_value_len), strlen(_str_buf));

		memset(_value, 0, _max_value_len);
		memcpy(_value, _str_buf, strlen(_str_buf));
	}

	delete[] _str_buf;
	return _occur_index >= 0;
}
