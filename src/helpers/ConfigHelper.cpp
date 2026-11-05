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
#include <interface/pdi/impl/modules/storage/FileLines.h>

/* generic config file support functions */

static int readConfigLine(const char *path, uint64_t offset, pdiutil::string &linedata, bool &cut)
{
  int bytes = FileLines::readLineAt(__i_fs, path, offset, linedata, &cut);

  if (cut)
  {
    SysLogW("config: %s line at %u longer than the heap could hold, not taken\n", path, (unsigned)offset);
  }

  return bytes;
}

/**
 * Logs an option whose value was refused, so a value too long or unreadable is
 * not kept from the reader in silence.
 */
static void logRefusedConfigValue(const pdiutil::string &key, const pdiutil::string &value)
{
  if (!value.empty())
  {
    SysLogW("config: %s value refused, the old one kept\n", key.c_str());
  }
}

static bool splitConfigLine(const pdiutil::string &linedata, pdiutil::string &key, pdiutil::string &value)
{
  size_t start = 0;
  while (start < linedata.length() && (linedata[start] == ' ' || linedata[start] == '\t')) start++;
  if (start >= linedata.length() || linedata[start] == LINE_COMMENT_CHAR)
  {
    return false;
  }

  size_t kend = start;
  while (kend < linedata.length() && linedata[kend] != ' ' && linedata[kend] != '\t') kend++;

  size_t vstart = kend;
  while (vstart < linedata.length() && (linedata[vstart] == ' ' || linedata[vstart] == '\t')) vstart++;
  size_t vend = linedata.length();
  while (vend > vstart && (linedata[vend - 1] == ' ' || linedata[vend - 1] == '\t')) vend--;

  key = linedata.substr(start, kend - start);
  value = linedata.substr(vstart, vend - vstart);
  return true;
}

static bool buildConfigLine(pdiutil::string &out, const char *key, const char *value)
{
  out.clear();
  out += key;
  out += ' ';
  if (nullptr != value)
  {
    out += value;
  }
  out += TERMINAL_NEW_LINE;

  return out.size() == strlen(key) + 1 + (nullptr != value ? strlen(value) : 0) + strlen(TERMINAL_NEW_LINE);
}

static bool parseConfigNumber(const pdiutil::string &value, uint32_t maxvalue, uint32_t &out)
{
  uint32_t parsed = StringToUint32(value.c_str());

  if (parsed > maxvalue || configNumberAsValue(parsed) != value)
  {
    return false;
  }

  out = parsed;
  return true;
}

static void applyConfigMeta(const char *path, uint16_t perms, uint16_t uid, uint16_t gid, bool owner)
{
  __i_fs.beginPrivileged();
  __i_fs.setFilePermissions(path, perms);
  if (owner)
  {
    __i_fs.setFileOwner(path, uid, gid);
  }
  __i_fs.endPrivileged();
}

static void restoreConfigMeta(const char *path, const file_info_t &meta, bool owner)
{
  applyConfigMeta(path, meta.m_perms, meta.m_uid, meta.m_gid, owner);
}

/**
 * Build the config file path a feature is read from, so the service layer and
 * the feature tables cannot drift on where a config lives.
 */
void buildConfigPath(const char *name, pdiutil::string &out)
{
  out.clear();

  if (nullptr == name || 0 == name[0])
  {
    return;
  }

  out = CHARPTR_WRAP(SERVICE_CONFIG_DIR_ROOT);
  out += name;
  out += FILE_SEPARATOR;
  out += name;
  out += CHARPTR_WRAP(SERVICE_CONFIG_FILE_SUFFIX);
}

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
    bool cut = false;
    int bytes = readConfigLine(path, offset, linedata, cut);
    if (bytes < 0) break;

    offset += (uint64_t)bytes + 1;

    config_kv_t kv;
    if (!cut && splitConfigLine(linedata, kv.m_key, kv.m_value))
    {
      out.push_back(kv);
    }

    __i_dvc_ctrl.yield();
  }

  return true;
}

/**
 * Read a single option out of a config file without holding the rest of it.
 */
bool getConfigValue(const char *path, const char *key, pdiutil::string &out)
{
  if (nullptr == path || nullptr == key || 0 == key[0] || !__i_fs.isFileExist(path))
  {
    return false;
  }

  int64_t fs = __i_fs.getFileSize(path);
  if (fs <= 0)
  {
    return false;
  }

  uint64_t offset = 0;
  pdiutil::string linedata, linekey, linevalue;

  while (offset < (uint64_t)fs)
  {
    bool cut = false;
    int bytes = readConfigLine(path, offset, linedata, cut);
    if (bytes < 0) break;

    offset += (uint64_t)bytes + 1;

    if (!cut && splitConfigLine(linedata, linekey, linevalue) && linekey == key)
    {
      out = linevalue;
      return true;
    }

    __i_dvc_ctrl.yield();
  }

  return false;
}

/**
 * Persist one option, leaving every other line, comment and ordering intact.
 * The option is appended when the file does not already carry it.
 */
bool setConfigValue(const char *path, const char *key, const char *value)
{
  if (nullptr == path || nullptr == key || 0 == key[0])
  {
    return false;
  }

  pdiutil::string newline;
  if (!buildConfigLine(newline, key, value))
  {
    return false;
  }

  if (!__i_fs.isFileExist(path))
  {
    return __i_fs.createFile(path, newline.c_str(), newline.size()) >= 0;
  }

  file_info_t original;
  bool haveoriginal = (PDI_OK == __i_fs.getFileMeta(path, original));

  pdiutil::string temppath = pdiutil::string(path) + CONFIG_TEMP_SUFFIX;
  if (__i_fs.isFileExist(temppath.c_str()))
  {
    __i_fs.deleteFile(temppath.c_str());
  }
  if (__i_fs.createFile(temppath.c_str(), "", 0) < 0)
  {
    return false;
  }
  if (haveoriginal)
  {
    restoreConfigMeta(temppath.c_str(), original, false);
  }

  int64_t fs = __i_fs.getFileSize(path);
  uint64_t offset = 0;
  bool replaced = false;
  bool writeok = true;
  pdiutil::string linedata, linekey, linevalue;

  while (writeok && fs > 0 && offset < (uint64_t)fs)
  {
    bool cut = false;
    int bytes = readConfigLine(path, offset, linedata, cut);
    if (bytes < 0 || cut)
    {
      writeok = false;
      break;
    }

    offset += (uint64_t)bytes + 1;

    if (splitConfigLine(linedata, linekey, linevalue) && linekey == key)
    {
      if (!replaced)
      {
        writeok = (__i_fs.writeFile(temppath.c_str(), newline.c_str(), newline.size(), true) == (int)newline.size());
        replaced = true;
      }
    }
    else
    {
      writeok = (__i_fs.writeFile(temppath.c_str(), linedata.c_str(), linedata.size(), true) == (int)linedata.size() &&
                 __i_fs.writeFile(temppath.c_str(), TERMINAL_NEW_LINE, strlen(TERMINAL_NEW_LINE), true) == (int)strlen(TERMINAL_NEW_LINE));
    }

    __i_dvc_ctrl.yield();
  }

  if (writeok && !replaced)
  {
    writeok = (__i_fs.writeFile(temppath.c_str(), newline.c_str(), newline.size(), true) == (int)newline.size());
  }

  if (!writeok)
  {
    __i_fs.deleteFile(temppath.c_str());
    return false;
  }

  __i_fs.deleteFile(path);
  if (PDI_OK != __i_fs.moveFile(temppath.c_str(), path))
  {
    return false;
  }

  if (haveoriginal)
  {
    restoreConfigMeta(path, original, true);
  }

  return true;
}

/**
 * Write a whole config file from key/value pairs, keeping the permissions and
 * ownership the file already had. A file being created takes the given mode,
 * owned by root, so one carrying a secret is not left world readable.
 */
bool saveConfigFile(const char *path, const pdiutil::vector<config_kv_t> &kvs, const char *header, uint16_t mode)
{
  if (nullptr == path)
  {
    return false;
  }

  file_info_t original;
  bool haveoriginal = (__i_fs.isFileExist(path) && PDI_OK == __i_fs.getFileMeta(path, original));

  pdiutil::string temppath = pdiutil::string(path) + CONFIG_TEMP_SUFFIX;
  if (__i_fs.isFileExist(temppath.c_str()))
  {
    __i_fs.deleteFile(temppath.c_str());
  }

  if (__i_fs.createFile(temppath.c_str(), "", 0) < 0)
  {
    return false;
  }

  if (haveoriginal)
  {
    restoreConfigMeta(temppath.c_str(), original, false);
  }
  else if (0 != mode)
  {
    applyConfigMeta(temppath.c_str(), mode, FILE_OWNER_ROOT_UID, FILE_OWNER_ROOT_GID, false);
  }

  bool writeok = true;
  if (nullptr != header && 0 != header[0])
  {
    writeok = (__i_fs.writeFile(temppath.c_str(), header, strlen(header), true) == (int)strlen(header));
  }

  pdiutil::string line;
  for (size_t i = 0; writeok && i < kvs.size(); i++)
  {
    writeok = buildConfigLine(line, kvs[i].m_key.c_str(), kvs[i].m_value.c_str()) &&
              (__i_fs.writeFile(temppath.c_str(), line.c_str(), line.size(), true) == (int)line.size());
    __i_dvc_ctrl.yield();
  }

  if (!writeok)
  {
    __i_fs.deleteFile(temppath.c_str());
    return false;
  }

  if (haveoriginal)
  {
    __i_fs.deleteFile(path);
  }

  if (PDI_OK != __i_fs.moveFile(temppath.c_str(), path))
  {
    return false;
  }

  if (haveoriginal)
  {
    restoreConfigMeta(path, original, true);
  }
  else if (0 != mode)
  {
    applyConfigMeta(path, mode, FILE_OWNER_ROOT_UID, FILE_OWNER_ROOT_GID, true);
  }

  return true;
}

/**
 * Create the config file and its directory with the given defaults when it is
 * missing, so a feature always has somewhere to read from.
 */
bool ensureConfigFile(const char *path, const pdiutil::vector<config_kv_t> &defaults, const char *header, uint16_t mode)
{
  if (nullptr == path)
  {
    return false;
  }

  if (__i_fs.isFileExist(path))
  {
    return true;
  }

  pdiutil::string dir = path;
  pdiutil::string::size_type sep = dir.find_last_of(PATH_SEPARATOR_CHAR);
  if (sep != pdiutil::string::npos && sep > 0)
  {
    dir = dir.substr(0, sep);
    if (!__i_fs.isDirExist(dir.c_str()) && __i_fs.createDirectory(dir.c_str()) < 0)
    {
      return false;
    }
  }

  return saveConfigFile(path, defaults, header, mode);
}

/**
 * Bring the file to the given options, creating it when absent and otherwise
 * rewriting only the ones whose value actually changed.
 */
bool writeConfigValues(const char *path, const pdiutil::vector<config_kv_t> &kvs, const char *header, uint16_t mode)
{
  if (nullptr == path)
  {
    return false;
  }

  if (!__i_fs.isFileExist(path))
  {
    return ensureConfigFile(path, kvs, header, mode);
  }

  pdiutil::vector<config_kv_t> present;
  loadConfigFile(path, present);

  bool written = true;
  pdiutil::string current;

  for (size_t i = 0; i < kvs.size(); i++)
  {
    if (findConfigValue(present, kvs[i].m_key, current) && current == kvs[i].m_value)
    {
      continue;
    }

    written = setConfigValue(path, kvs[i].m_key.c_str(), kvs[i].m_value.c_str()) && written;
    __i_dvc_ctrl.yield();
  }

  if (0 != mode)
  {
    applyConfigMeta(path, mode, FILE_OWNER_ROOT_UID, FILE_OWNER_ROOT_GID, true);
  }

  return written;
}

/**
 * Add an option to a set being built, so a caller renders its settings without
 * knowing how a config line is spelled.
 */
void appendConfigValue(pdiutil::vector<config_kv_t> &kvs, const pdiutil::string &key, const pdiutil::string &value)
{
  config_kv_t kv;
  kv.m_key = key;
  kv.m_value = value;
  kvs.push_back(kv);
}

/**
 * Read one option out of a set already in hand, for a caller that would
 * otherwise walk the whole file once per key.
 */
bool findConfigValue(const pdiutil::vector<config_kv_t> &kvs, const pdiutil::string &key, pdiutil::string &out)
{
  for (size_t i = 0; i < kvs.size(); i++)
  {
    if (kvs[i].m_key == key)
    {
      out = kvs[i].m_value;
      return true;
    }
  }

  return false;
}

/**
 * Take an option into a fixed width text field, leaving the field alone when
 * the set does not carry the option or the value would not fit.
 */
bool takeConfigText(const pdiutil::vector<config_kv_t> &kvs, const pdiutil::string &key, char *field, uint16_t size)
{
  pdiutil::string value;
  if (nullptr == field || !findConfigValue(kvs, key, value))
  {
    return false;
  }

  if (value.size() >= size)
  {
    logRefusedConfigValue(key, value);
    return false;
  }

  memset(field, 0, size);
  memcpy(field, value.c_str(), value.size());
  return true;
}

/**
 * Take an option into a four octet address, leaving the field alone unless the
 * value reads back as the very address it spells.
 */
bool takeConfigAddress(const pdiutil::vector<config_kv_t> &kvs, const pdiutil::string &key, uint8_t *field)
{
  pdiutil::string value;
  if (nullptr == field || !findConfigValue(kvs, key, value))
  {
    return false;
  }

  ipaddress_t address(value.c_str());
  pdiutil::string rendered = address;
  if (rendered != value)
  {
    logRefusedConfigValue(key, value);
    return false;
  }

  memcpy(field, address.ip4, 4);
  return true;
}

/**
 * Take an option into a truth value, leaving the field alone when the set does
 * not carry the option or its text says nothing recognisable.
 */
bool takeConfigBool(const pdiutil::vector<config_kv_t> &kvs, const pdiutil::string &key, bool *field)
{
  pdiutil::string value;
  if (nullptr == field || !findConfigValue(kvs, key, value))
  {
    return false;
  }

  *field = configValueAsBool(value, *field);
  return true;
}

/**
 * Take an option into a truth value kept as a byte, for a record that spells a
 * flag as a number rather than as a bool.
 */
bool takeConfigBool(const pdiutil::vector<config_kv_t> &kvs, const pdiutil::string &key, uint8_t *field)
{
  pdiutil::string value;
  if (nullptr == field || !findConfigValue(kvs, key, value))
  {
    return false;
  }

  *field = configValueAsBool(value, 0 != *field) ? 1 : 0;
  return true;
}

/**
 * Take an option into a whole number, leaving the field alone unless every
 * character is a digit and the result is one the field is allowed to hold.
 */
bool takeConfigNumber(const pdiutil::vector<config_kv_t> &kvs, const pdiutil::string &key, uint8_t *field, uint8_t maxvalue)
{
  pdiutil::string value;
  uint32_t parsed = 0;

  if (nullptr == field || !findConfigValue(kvs, key, value))
  {
    return false;
  }

  if (!parseConfigNumber(value, maxvalue, parsed))
  {
    logRefusedConfigValue(key, value);
    return false;
  }

  *field = (uint8_t)parsed;
  return true;
}

/**
 * Take an option into a whole number, leaving the field alone unless every
 * character is a digit and the result is one the field is allowed to hold.
 */
bool takeConfigNumber(const pdiutil::vector<config_kv_t> &kvs, const pdiutil::string &key, uint16_t *field, uint16_t maxvalue)
{
  pdiutil::string value;
  uint32_t parsed = 0;

  if (nullptr == field || !findConfigValue(kvs, key, value))
  {
    return false;
  }

  if (!parseConfigNumber(value, maxvalue, parsed))
  {
    logRefusedConfigValue(key, value);
    return false;
  }

  *field = (uint16_t)parsed;
  return true;
}

/**
 * Render a four octet address the way a config file carries it.
 */
pdiutil::string configAddressAsValue(const uint8_t *octets)
{
  ipaddress_t address(octets[0], octets[1], octets[2], octets[3]);
  return address;
}

/**
 * Read an option's text as a truth value, falling back to the default when it
 * says nothing recognisable.
 */
bool configValueAsBool(const pdiutil::string &value, bool defaultval)
{
  char buf[8];
  uint8_t len = (uint8_t)value.size();
  if (0 == len || len > (sizeof(buf) - 3))
  {
    return defaultval;
  }

  buf[0] = CONFIG_BOOL_SEPERATOR;
  memcpy(buf + 1, value.c_str(), len);
  buf[len + 1] = CONFIG_BOOL_SEPERATOR;
  buf[len + 2] = '\0';
  __tolowercase(buf, sizeof(buf));

  pdiutil::string truevals = CHARPTR_WRAP(CONFIG_BOOL_TRUE_VALUES);
  if (__strstr(truevals.c_str(), buf, truevals.size()) >= 0)
  {
    return true;
  }

  pdiutil::string falsevals = CHARPTR_WRAP(CONFIG_BOOL_FALSE_VALUES);
  if (__strstr(falsevals.c_str(), buf, falsevals.size()) >= 0)
  {
    return false;
  }

  return defaultval;
}

/**
 * Render a truth value the way a config file carries it.
 */
pdiutil::string configBoolAsValue(bool value)
{
  pdiutil::string out;

  if (value)
  {
    out = CHARPTR_WRAP(CONFIG_BOOL_YES);
  }
  else
  {
    out = CHARPTR_WRAP(CONFIG_BOOL_NO);
  }

  return out;
}

/**
 * Render a whole number the way a config file carries it.
 */
pdiutil::string configNumberAsValue(uint32_t value)
{
  char buf[12];
  Uint32ToString(value, buf, sizeof(buf));
  return pdiutil::string(buf);
}

#endif
