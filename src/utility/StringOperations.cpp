/********************** String Operations Utility *****************************
This file is part of the PDI stack.

This is free software. You can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
Created Date    : 1st June 2019
******************************************************************************/

#include "DataTypeConversions.h"
#include "StringOperations.h"

/**
 * @brief Finds the first occurrence of a substring in a string.
 * 
 * This function searches for the first occurrence of a substring within a given string.
 * If the substring is found, it returns the index of the first occurrence; otherwise, it returns -1.
 *
 * @param str The main string to search in.
 * @param substr The substring to search for.
 * @param _len The maximum length to search (default is 300).
 * @return The index of the first occurrence of the substring, or -1 if not found.
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
        char *pattern = (char *)substr;

        while (*str && *pattern && *str == *pattern)
        {
            str++;
            pattern++;
        }

        if (!*pattern)
            return n;

        str = Begin + 1;
        n++;
    }

    return -1;
}

/**
 * @brief Trims a specific character from both ends of a string.
 * 
 * This function removes occurrences of a specified character from the beginning and end of a string.
 *
 * @param str The string to trim.
 * @param _val The character to trim.
 * @param _overflow_limit The maximum length of the string (default is 300).
 * @return A pointer to the trimmed string.
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
 * @brief Trims whitespace from both ends of a string.
 * 
 * This function removes leading and trailing whitespace from a string.
 *
 * @param str The string to trim.
 * @param _overflow_limit The maximum length of the string (default is 300).
 * @return A pointer to the trimmed string.
 */
char *__strtrim(char *str, uint16_t _overflow_limit)
{
    return __strtrim_val(str, ' ', _overflow_limit);
}

/**
 * @brief Compares two strings for equality.
 * 
 * This function checks whether two strings are equal.
 *
 * @param str1 The first string to compare.
 * @param str2 The second string to compare.
 * @param _overflow_limit The maximum length of the strings (default is 300).
 * @return True if the strings are equal, false otherwise.
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
 * @brief Compares two character arrays for equality.
 * 
 * This function checks whether two character arrays are equal.
 *
 * @param array1 The first array to compare.
 * @param array2 The second array to compare.
 * @param len The length of the arrays to compare.
 * @return True if the arrays are equal, false otherwise.
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
 * @brief Appends an unsigned integer to a string buffer using a specified format.
 * 
 * This function appends a formatted unsigned integer to a string buffer.
 *
 * @param _str The string buffer to append to.
 * @param _format The format string (e.g., "%u").
 * @param _value The unsigned integer value to append.
 * @param _len The maximum length of the string buffer.
 */
void __appendUintToBuff(char *_str, const char *_format, uint32_t _value, int _len)
{
    char value[20];
    memset(value, 0, 20);
    sprintf(value, (const char *)_format, _value);
    strncat(_str, value, _len);
}

/**
 * @brief Converts an IP address from integer format to string format.
 * 
 * This function converts an IP address represented as an array of 4 bytes into a string.
 *
 * @param _str The string buffer to store the IP address.
 * @param _ip The IP address in integer format (4 bytes).
 * @param _len The maximum length of the string buffer (default is 15).
 */
void __int_ip_to_str(char *_str, uint8_t *_ip, int _len)
{
    memset(_str, 0, _len);
    sprintf(_str, "%d.%d.%d.%d", _ip[0], _ip[1], _ip[2], _ip[3]);
}

/**
 * @brief Converts an IP address from string format to integer format.
 * 
 * This function converts an IP address represented as a string into an array of 4 bytes.
 *
 * @param _str The IP address in string format.
 * @param _ip The buffer to store the IP address in integer format (4 bytes).
 * @param _len The maximum length of the string (default is 15).
 * @param _clear_str_after_done If true, clears the string after conversion (default is true).
 */
void __str_ip_to_int(char *_str, uint8_t *_ip, int _len, bool _clear_str_after_done)
{
    _ip[0] = StringToUint8(_str, 3);
    for (uint8_t i = 0, _ip_index = 1; i < strlen(_str) && i < _len; i++)
    {
        if (_str[i] == '.' && _ip_index < 4)
        {
            _ip[_ip_index++] = StringToUint8(&_str[i + 1], 3);
        }
    }
    if (_clear_str_after_done)
    {
        memset(_str, 0, _len);
    }
}

/**
 * @brief Finds and replaces a substring in a string.
 * 
 * This function searches for a substring in a string and replaces it with another substring.
 *
 * @param _str The main string to modify.
 * @param _find_str The substring to find.
 * @param _replace_with The substring to replace with.
 * @param _occurence The number of occurrences to replace.
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
 * @brief Extracts a value from a JSON string based on a key.
 * 
 * This function parses a JSON string and retrieves the value associated with a specified key.
 *
 * @param _str The JSON string to parse.
 * @param _key The key to search for.
 * @param _value The buffer to store the extracted value.
 * @param _max_value_len The maximum length of the value buffer.
 * @return True if the key-value pair was found, false otherwise.
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
        int no_of_double_quote = 0;
        bool foundsemicolon = false;

        while (j < _str_len)
        {
            if (_str[_occur_index + _key_str_len + j] == ',')
            {
                no_of_commas++;
            }
            else if (_str[_occur_index + _key_str_len + j] == ':')
            {
                foundsemicolon = true;
            }
            else if (foundsemicolon && (_str[_occur_index + _key_str_len + j] == '"'))
            {
                no_of_double_quote++;
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

            if ((no_of_commas > 0 && (no_of_opening_curly_bracket +
                                      no_of_closing_curly_bracket + 
                                      no_of_opening_square_bracket + 
                                      no_of_closing_square_bracket) == 0)
                                    && (no_of_double_quote%2 == 0))
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
