/******************************** File Lines **********************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 22nd Oct 2026
******************************************************************************/
#ifndef _FILE_LINES_H_
#define _FILE_LINES_H_

#include <interface/pdi/modules/storage/iFileSystemInterface.h>

/**
 * Finds lines and strings in a file through its filesystem's readFile and
 * getFileSize alone.
 */
struct FileLines {

  /**
   * Finds the string in a file, storing the offsets of every nth match up to
   * maxindices; how many, or an error code.
   */
  static int findInFile(iFileSystemInterface &fs, const char* path, const char* findStr, pdiutil::vector<uint32_t> *findindices, int maxindices, int everynthindice, int64_t offset, CallBackVoidArgFn yield);

  /**
   * The offset at which a line number starts in a file, or a negative error code.
   */
  static int64_t getOffsetFromLineNumber(iFileSystemInterface &fs, const char* path, int linenumber, CallBackVoidArgFn yield);

  /**
   * The line number holding an offset in a file, or a negative error code.
   */
  static int64_t getLineNumberFromOffset(iFileSystemInterface &fs, const char* path, int64_t offset, CallBackVoidArgFn yield);

  /**
   * Counts the lines in a file, storing up to maxlinenumbers line offsets; how
   * many, or a negative error code.
   */
  static int getLineNumbersInFile(iFileSystemInterface &fs, const char* path, pdiutil::vector<uint32_t> &linenumberindices, int maxlinenumbers, int linenumberoffset, CallBackVoidArgFn yield);

  /**
   * Reads a line of a file, negative numbers counting from the end, matched to
   * pattern when given; the bytes read.
   */
  static int readLineInFile(iFileSystemInterface &fs, const char* path, int32_t linenumber, pdiutil::string &linedata, const char* pattern, CallBackVoidArgFn yield);
};

#endif
