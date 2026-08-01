/******************************* Config helper ********************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st Aug 2026
******************************************************************************/

#include <config/Config.h>

#if defined(ENABLE_STORAGE_SERVICE)

#include "ConfigHelper.h"

/* generic config file support functions */

/**
 * @brief Parse a "key value" style config file into key/value pairs.
 *
 * Reads the file line by line. Blank lines and lines starting with '#' are
 * skipped. The first whitespace-separated token is the key, the remainder of
 * the line (leading whitespace trimmed) is the value.
 *
 * @param path Absolute path of the config file.
 * @param out Vector receiving the parsed pairs.
 * @return True if the file exists and was read, false otherwise.
 */
bool loadConfigFile(const char *path, pdiutil::vector<config_kv_t> &out)
{
  if (nullptr == path || !__i_fs.isFileExist(path))
  {
    return false;
  }

  int64_t fs = __i_fs.getFileSize(path);
  if (fs <= 0)
  {
    return false;
  }

  uint64_t offset = 0;
  pdiutil::string linedata;

  while (offset < (uint64_t)fs)
  {
    linedata.clear();
    int bytes = __i_fs.readFile(path, 128, [&](char *data, uint32_t size) -> bool {
      linedata += pdiutil::string(data, size);
      return true;
    }, offset, "\n");
    if (bytes < 0) break;

    offset += (uint64_t)bytes + 1;

    if (!linedata.empty() && linedata.back() == '\r')
    {
      linedata.pop_back();
    }

    size_t start = 0;
    while (start < linedata.length() && (linedata[start] == ' ' || linedata[start] == '\t')) start++;
    if (start >= linedata.length() || linedata[start] == '#')
    {
      __i_dvc_ctrl.yield();
      continue;
    }

    size_t kend = start;
    while (kend < linedata.length() && linedata[kend] != ' ' && linedata[kend] != '\t') kend++;

    size_t vstart = kend;
    while (vstart < linedata.length() && (linedata[vstart] == ' ' || linedata[vstart] == '\t')) vstart++;
    size_t vend = linedata.length();
    while (vend > vstart && (linedata[vend - 1] == ' ' || linedata[vend - 1] == '\t')) vend--;

    config_kv_t kv;
    kv.m_key = linedata.substr(start, kend - start);
    kv.m_value = linedata.substr(vstart, vend - vstart);
    out.push_back(kv);

    __i_dvc_ctrl.yield();
  }

  return true;
}

#endif
