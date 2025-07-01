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

#include "Base64.h"
#include "DataTypeDef.h"

// Character set of the Base64 encoding scheme
static char char_set[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

/**
 * @brief Encodes a string into Base64 format.
 * 
 * This function takes an input string and encodes it into Base64 format. It processes
 * the input string in chunks of 3 bytes, converts them into 4 Base64 characters, and
 * appends padding characters (`=`) if necessary.
 *
 * @param input_str The input string to encode.
 * @param len_str The length of the input string.
 * @param res_str The buffer to store the resulting Base64-encoded string.
 * @return True if the encoding was successful, false otherwise.
 */
bool base64Encode(char input_str[], int len_str, char *res_str)
{
  // Check if the result buffer is valid
  if (nullptr == res_str)
  {
    return false;
  }

  int index, no_of_bits = 0, padding = 0, val = 0, count = 0, temp;
  int i, j, k = 0;

  // Process the input string in chunks of 3 bytes
  for (i = 0; i < len_str; i += 3)
  {
    val = 0, count = 0, no_of_bits = 0;

    // Combine up to 3 bytes into a single integer value
    for (j = i; j < len_str && j <= i + 2; j++)
    {
      // Shift the current value 8 bits to the left
      val = val << 8;

      // Add the current byte to the value
      val = val | input_str[j];

      // Increment the count of processed bytes
      count++;
    }

    // Calculate the total number of bits in the current chunk
    no_of_bits = count * 8;

    // Calculate the number of padding characters needed
    padding = no_of_bits % 3;

    // Extract 6 bits at a time and map them to Base64 characters
    while (no_of_bits != 0)
    {
      if (no_of_bits >= 6)
      {
        temp = no_of_bits - 6;

        // Extract the top 6 bits
        index = (val >> temp) & 63;
        no_of_bits -= 6;
      }
      else
      {
        temp = 6 - no_of_bits;

        // Append zeros to the right if bits are less than 6
        index = (val << temp) & 63;
        no_of_bits = 0;
      }
      res_str[k++] = char_set[index];
    }
  }

  // Add padding characters (`=`) if necessary
  for (i = 1; i <= padding; i++)
  {
    res_str[k++] = '=';
  }

  // Null-terminate the resulting string
  res_str[k] = '\0';

  return true;
}

/**
 * @brief Generates a unique key of specified length.
 * 
 * This function generates a random unique key consisting of alphanumeric characters.
 * The key is generated using a predefined character set and is null-terminated.
 *
 * @param key Pointer to the buffer where the unique key will be stored.
 * @param len Length of the unique key to be generated (including null terminator).
 */
void genUniqueKey(char *key, int len){

  for (int i = 0; i < len; i++){

    key[i] = char_set[rand() % (sizeof(char_set) - 1)];
  }
}
