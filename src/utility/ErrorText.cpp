/******************************* Error Text *************************************
This file is part of the PDI stack.

This is free software. You can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
Created Date    : 5th Nov 2026
******************************************************************************/

#include "ErrorText.h"
#include "pdi_types.h"

/**
 * @brief The words for an error code: the generic codes, the storage codes and
 *        the LittleFS and FatFs codes carried through.
 * @param code The code.
 * @return Read only text, or nullptr for a code that has no words yet.
 */
const char *ErrorToText(int32_t code)
{
    switch (code)
    {
    case PDI_ERR_FAILURE:                 return RODT_ATTR("Failed");
    case PDI_ERR_INVALID_ARG:             return RODT_ATTR("Invalid argument");
    case PDI_ERR_NO_MEM:                  return RODT_ATTR("Out of memory");
    case PDI_ERR_TIMEOUT:                 return RODT_ATTR("Timed out");
    case PDI_ERR_NOT_FOUND:               return RODT_ATTR("Not found");
    case PDI_ERR_NOT_SUPPORTED:           return RODT_ATTR("Not supported");
    case PDI_ERR_BUSY:                    return RODT_ATTR("Busy");
    case PDI_ERR_PERM:                    return RODT_ATTR("Permission denied");
    case PDI_ERR_EXISTS:                  return RODT_ATTR("File exists");
    case PDI_ERR_NO_SPACE:                return RODT_ATTR("No space left on device");
    case PDI_ERR_IO:                      return RODT_ATTR("Input/output error");
    case PDI_ERR_RANGE:                   return RODT_ATTR("Out of range");
    case PDI_ERR_CORRUPT:                 return RODT_ATTR("Corrupt");
    case PDI_ERR_CRC:                     return RODT_ATTR("Checksum mismatch");
    case PDI_ERR_FULL:                    return RODT_ATTR("Full");
    case STORAGE_ERROR_BAD_PATH:          return RODT_ATTR("Bad path");
    case STORAGE_ERROR_NAME_TOO_LONG:     return RODT_ATTR("File name too long");
    case STORAGE_ERROR_NO_PARENT:         return RODT_ATTR("No such directory");
    case STORAGE_ERROR_NOT_A_FILE:        return RODT_ATTR("Is a directory");
    case STORAGE_ERROR_NOT_A_DIRECTORY:   return RODT_ATTR("Not a directory");
    case STORAGE_ERROR_READ_ONLY:         return RODT_ATTR("Read-only file system");
    case STORAGE_ERROR_NODE_LIMIT:        return RODT_ATTR("Too many files");
    case STORAGE_ERROR_NOT_MOUNTED:       return RODT_ATTR("Not mounted");
    case STORAGE_ERROR_NOT_EMPTY:         return RODT_ATTR("Directory not empty");
    case PDI_ERR_FROM_LFS(-5):          return RODT_ATTR("Input/output error");
    case PDI_ERR_FROM_LFS(-84):     return RODT_ATTR("File system corrupt");
    case PDI_ERR_FROM_LFS(-2):       return RODT_ATTR("No such file or directory");
    case PDI_ERR_FROM_LFS(-17):       return RODT_ATTR("File exists");
    case PDI_ERR_FROM_LFS(-20):      return RODT_ATTR("Not a directory");
    case PDI_ERR_FROM_LFS(-21):       return RODT_ATTR("Is a directory");
    case PDI_ERR_FROM_LFS(-39):    return RODT_ATTR("Directory not empty");
    case PDI_ERR_FROM_LFS(-27):        return RODT_ATTR("File too large");
    case PDI_ERR_FROM_LFS(-22):       return RODT_ATTR("Invalid argument");
    case PDI_ERR_FROM_LFS(-28):       return RODT_ATTR("No space left on device");
    case PDI_ERR_FROM_LFS(-12):       return RODT_ATTR("Out of memory");
    case PDI_ERR_FROM_LFS(-36): return RODT_ATTR("File name too long");
    case PDI_ERR_FROM_FAT(1):  return RODT_ATTR("Input/output error");
    case PDI_ERR_FROM_FAT(3):  return RODT_ATTR("Disk not ready");
    case PDI_ERR_FROM_FAT(4):  return RODT_ATTR("No such file or directory");
    case PDI_ERR_FROM_FAT(5):  return RODT_ATTR("No such file or directory");
    case PDI_ERR_FROM_FAT(6):  return RODT_ATTR("Invalid file name");
    case PDI_ERR_FROM_FAT(7):  return RODT_ATTR("Access denied or directory full");
    case PDI_ERR_FROM_FAT(8):  return RODT_ATTR("File exists");
    case PDI_ERR_FROM_FAT(10): return RODT_ATTR("Write protected");
    case PDI_ERR_FROM_FAT(13): return RODT_ATTR("No FAT file system");
    case PDI_ERR_FROM_FAT(17): return RODT_ATTR("Out of memory");
    case PDI_ERR_FROM_FAT(18): return RODT_ATTR("Too many open files");
    default:                   return nullptr;
    }
}
