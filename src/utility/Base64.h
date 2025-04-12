/****************************** Base64 Utility ********************************
This file is part of the PDI stack.

This is free software. You can redistribute it and/or modify it but without any
warranty.

The Base64 utility provides functionality for encoding data into Base64 format.
It is useful for encoding binary data into a text-based format for transmission
or storage.

Author          : Suraj I.
Created Date    : 1st June 2019
******************************************************************************/

#ifndef _BASE64_H_
#define _BASE64_H_

/**
 * @brief Encodes a string into Base64 format.
 * 
 * This function takes an input string and encodes it into Base64 format.
 *
 * @param input_str The input string to encode.
 * @param len_str The length of the input string.
 * @param res_str The buffer to store the resulting Base64-encoded string.
 * @return True if the encoding was successful, false otherwise.
 */
bool base64Encode(char input_str[], int len_str, char *res_str);

#endif
