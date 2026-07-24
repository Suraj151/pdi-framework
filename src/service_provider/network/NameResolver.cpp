/******************************** Name Resolver *******************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 23rd July 2026
******************************************************************************/

#include "NameResolver.h"

#ifdef ENABLE_NETWORK_SERVICE

void NameResolver::ensureHostsFile()
{
#ifdef ENABLE_STORAGE_SERVICE
  if (__i_fs.isFileExist(HOSTS_FILE_PATH)) return;
  __i_fs.createFile(HOSTS_FILE_PATH, HOSTS_FILE_SEED);
#endif
}

bool NameResolver::parseIpLiteral(const char *str, ipaddress_t &out)
{
  if (nullptr == str || '\0' == str[0]) return false;

  for (const char *p = str; *p; ++p) {
    if ((*p < '0' || *p > '9') && *p != '.') return false;
  }

  ipaddress_t ip(str);
  if (ip.isSet() || 0 == strcmp(str, "0.0.0.0")) {
    out = ip;
    return true;
  }
  return false;
}

bool NameResolver::lookupHostsFile(const char *hostname, ipaddress_t &out)
{
#ifdef ENABLE_STORAGE_SERVICE
  if (nullptr == hostname || !__i_fs.isFileExist(HOSTS_FILE_PATH)) return false;
  int64_t fs = __i_fs.getFileSize(HOSTS_FILE_PATH);
  if (fs <= 0) return false;

  size_t hlen = strlen(hostname);
  uint64_t offset = 0;
  bool found = false;

  while (offset < (uint64_t)fs && !found) {
    pdiutil::string linedata;
    int bytes = __i_fs.readFile(HOSTS_FILE_PATH, 128, [&](char *data, uint32_t size) -> bool {
      linedata += pdiutil::string(data, size);
      return true;
    }, offset, "\n");
    if (bytes < 0) break;

    if (!linedata.empty() && linedata.back() == '\r') {
      linedata.pop_back();
    }

    const char *line = linedata.c_str();
    int p = 0;
    while (line[p] == ' ' || line[p] == '\t') p++;

    // skip blank lines and comments
    if (line[p] != '#' && line[p] != '\0') {
      int ipstart = p;
      while (line[p] != '\0' && line[p] != ' ' && line[p] != '\t') p++;
      pdiutil::string ipstr(line + ipstart, p - ipstart);

      while (line[p] != '\0') {
        while (line[p] == ' ' || line[p] == '\t') p++;
        if (line[p] == '\0' || line[p] == '#') break;
        int nstart = p;
        while (line[p] != '\0' && line[p] != ' ' && line[p] != '\t') p++;
        if ((size_t)(p - nstart) == hlen && 0 == strncmp(line + nstart, hostname, hlen)) {
          out = ipaddress_t(ipstr.c_str());
          found = true;
          break;
        }
      }
    }

    offset += (uint64_t)bytes + 1;
    __i_dvc_ctrl.yield();
  }

  return found;
#else
  return false;
#endif
}

bool NameResolver::resolve(const char *hostname, ipaddress_t &out, uint32_t timeout_ms)
{
  if (nullptr == hostname || '\0' == hostname[0]) return false;

  if (parseIpLiteral(hostname, out)) return true;
  if (lookupHostsFile(hostname, out)) return true;

  int status = __i_wifi.hostByName(hostname, out, timeout_ms);
  return (status == 1) && out.isSet();
}

#endif
