/********************** String Operations Utility *****************************
This file is part of the PDI stack.

This is free software. You can redistribute it and/or modify it but without any
warranty.

The StringOperations utility provides a collection of helper functions for
manipulating and processing strings. These functions include searching,
trimming, comparing, and converting strings, as well as performing operations
like finding and replacing substrings.

Author          : Suraj I.
Created Date    : 1st June 2019
******************************************************************************/

#ifndef __STRING_OPERATIONS_H__
#define __STRING_OPERATIONS_H__

#include "DataTypeDef.h"

/**
 * @brief Finds the first occurrence of a substring in a string.
 * @param str The main string to search in.
 * @param substr The substring to search for.
 * @param _len The maximum length to search (default is 300).
 * @return The index of the first occurrence of the substring, or -1 if not found.
 */
int __strstr(char *str, const char *substr, int _len = 300);

/**
 * @brief Trims leading and trailing whitespace from a string.
 * @param str The string to trim.
 * @param _overflow_limit The maximum length of the string (default is 300).
 * @return A pointer to the trimmed string.
 */
char *__strtrim(char *str, uint16_t _overflow_limit = 300);

/**
 * @brief Trims leading and trailing occurrences of a specific character from a string.
 * @param str The string to trim.
 * @param _val The character to trim.
 * @param _overflow_limit The maximum length of the string (default is 300).
 * @return A pointer to the trimmed string.
 */
char *__strtrim_val(char *str, char _val, uint16_t _overflow_limit = 300);

/**
 * @brief Compares two strings for equality.
 * @param str1 The first string to compare.
 * @param str2 The second string to compare.
 * @param _overflow_limit The maximum length of the strings (default is 300).
 * @return True if the strings are equal, false otherwise.
 */
bool __are_str_equals(const char *str1, const char *str2, uint16_t _overflow_limit = 300);

/**
 * @brief Compares two character arrays for equality.
 * @param array1 The first array to compare.
 * @param array2 The second array to compare.
 * @param len The length of the arrays to compare (default is 300).
 * @return True if the arrays are equal, false otherwise.
 */
bool __are_arrays_equal(char *array1, char *array2, uint16_t len = 300);

/**
 * @brief Appends an unsigned integer to a string buffer using a specified format.
 * @param _str The string buffer to append to.
 * @param _format The format string (e.g., "%u").
 * @param _value The unsigned integer value to append.
 * @param _len The maximum length of the string buffer.
 */
void __appendUintToBuff(char *_str, const char *_format, uint32_t _value, int _len);

/**
 * @brief Converts an IP address from integer format to string format.
 * @param _str The string buffer to store the IP address.
 * @param _ip The IP address in integer format (4 bytes).
 * @param _len The maximum length of the string buffer (default is 15).
 */
void __int_ip_to_str(char *_str, uint8_t *_ip, int _len = 15);

/**
 * @brief Converts an IP address from string format to integer format.
 * @param _str The IP address in string format.
 * @param _ip The buffer to store the IP address in integer format (4 bytes).
 * @param _len The maximum length of the string (default is 15).
 * @param _clear_str_after_done If true, clears the string after conversion (default is true).
 */
void __str_ip_to_int(char *_str, uint8_t *_ip, int _len = 15, bool _clear_str_after_done = true);

/**
 * @brief Finds and replaces a substring in a string.
 * @param _str The main string to modify.
 * @param _find_str The substring to find.
 * @param _replace_with The substring to replace with.
 * @param _occurence The number of occurrences to replace.
 */
void __find_and_replace(char *_str, const char *_find_str, const char *_replace_with, int _occurence);

/**
 * @brief Extracts a value from a JSON string based on a key.
 * @param _str The JSON string to parse.
 * @param _key The key to search for.
 * @param _value The buffer to store the extracted value.
 * @param _max_value_len The maximum length of the value buffer.
 * @return True if the key-value pair was found, false otherwise.
 */
bool __get_from_json(char *_str, char *_key, char *_value, int _max_value_len);

#endif
