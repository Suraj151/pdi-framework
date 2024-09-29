/********************** String Operations Utility *****************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st June 2019
******************************************************************************/

#ifndef __STRINGOPERATIONS_H__
#define __STRINGOPERATIONS_H__

#include "DataTypeDef.h"

int __strstr(char *str, const char *substr, int _len = 300);
char *__strtrim(char *str, uint16_t _overflow_limit = 300);
char *__strtrim_val(char *str, char _val, uint16_t _overflow_limit = 300);
bool __are_str_equals(const char *str1, const char *str2, uint16_t _overflow_limit = 300);
bool __are_arrays_equal(char *array1, char *array2, uint16_t len = 300);
void __appendUintToBuff(char *_str, const char *_format, uint32_t _value, int _len);
void __int_ip_to_str(char *_str, uint8_t *_ip, int _len = 15);
void __str_ip_to_int(char *_str, uint8_t *_ip, int _len = 15, bool _clear_str_after_done = true);
void __find_and_replace(char *_str, const char *_find_str, const char *_replace_with, int _occurence);
bool __get_from_json(char *_str, char *_key, char *_value, int _max_value_len);

#endif
