/************************** User Store Service ********************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 18th July 2026
******************************************************************************/

#include <config/Config.h>

#if defined(ENABLE_AUTH_SERVICE)

#include "UserStoreService.h"
#include <utility/crypto/hash/sha256.h>
#include <utility/DataTypeConversions.h>
#include <service_provider/database/DatabaseServiceProvider.h>
#include <stdlib.h>
#include <string.h>

UserStoreService::UserStoreService(): m_initStatus(false), ServiceProvider(SERVICE_USER_STORE, RODT_ATTR("UserStore"))
{
}

UserStoreService::~UserStoreService()
{
}

bool UserStoreService::initService(void *arg)
{
  m_initStatus = true;

#ifdef ENABLE_STORAGE_SERVICE
  bootstrapFromLoginTable();
#endif

  return (m_initStatus & ServiceProvider::initService(arg));
}

#ifdef ENABLE_STORAGE_SERVICE
void UserStoreService::bootstrapFromLoginTable()
{
  if (__i_fs.isFileExist(USER_STORE_SHADOW_PATH)) {
    __i_fs.setFilePermissions(USER_STORE_SHADOW_PATH, 0600);
  }

  if (__i_fs.isFileExist(USER_STORE_PASSWD_PATH)) return;

  if (!__i_fs.isDirectory(USER_STORE_ETC_DIR)) {
    if (__i_fs.createDirectory(USER_STORE_ETC_DIR) < 0) return;
  }

  login_credential_table creds;
  if (!__database_service.get_login_credential_table(&creds)) return;
  if (0 == creds.username[0]) return;

  user_record_t root;
  root.m_uid = USER_STORE_ROOT_UID;
  root.m_gid = USER_STORE_ROOT_GID;
  root.m_username = creds.username;
  root.m_home = __i_fs.getHomeDirectory();
  root.m_shell = USER_STORE_DEFAULT_SHELL;

  addUser(root, creds.password);
}
#endif

bool UserStoreService::findUserByName(const char *username, user_record_t &out)
{
  if (nullptr == username || 0 == username[0]) return false;
  return scanPasswdFile(false, 0, username, out);
}

bool UserStoreService::findUserByUid(uint16_t uid, user_record_t &out)
{
  return scanPasswdFile(true, uid, nullptr, out);
}

bool UserStoreService::addUser(const user_record_t &record, const char *password)
{
  if (record.m_username.empty()) return false;

  user_record_t existing;
  if (findUserByName(record.m_username.c_str(), existing)) return false;

  pdiutil::string line;
  serializePasswdLine(record, line);
  line += '\n';

  if (!__i_fs.isFileExist(USER_STORE_PASSWD_PATH)) {
    if (__i_fs.createFile(USER_STORE_PASSWD_PATH, "") < 0) return false;
  }

  int iStatus = __i_fs.writeFile(USER_STORE_PASSWD_PATH, (char*)line.c_str(), line.size(), true);
  if (iStatus < 0) return false;

  if (nullptr != password && 0 != password[0]) {
    if (!setPassword(record.m_username.c_str(), password)) {
      removeLineByUsername(USER_STORE_PASSWD_PATH, record.m_username.c_str());
      return false;
    }
  }
  return true;
}

bool UserStoreService::removeUser(const char *username)
{
  if (nullptr == username || 0 == username[0]) return false;
  bool passwdRemoved = removeLineByUsername(USER_STORE_PASSWD_PATH, username);
  removeLineByUsername(USER_STORE_SHADOW_PATH, username);
  return passwdRemoved;
}

bool UserStoreService::parsePasswdLine(const pdiutil::string &line, user_record_t &out)
{
  if (line.empty() || line[0] == '#') return false;

  pdiutil::string fields[6];
  pdiutil::string::size_type start = 0;
  uint8_t fi = 0;
  for (pdiutil::string::size_type i = 0; i <= line.size() && fi < 6; i++) {
    if (i == line.size() || line[i] == USER_STORE_FIELD_SEP) {
      fields[fi++] = line.substr(start, i - start);
      start = i + 1;
    }
  }
  if (fi < 6) return false;

  out.clear();
  out.m_username = fields[0];
  out.m_uid = (uint16_t)StringToUint32(fields[2].c_str(), (uint8_t)fields[2].size());
  out.m_gid = (uint16_t)StringToUint32(fields[3].c_str(), (uint8_t)fields[3].size());
  out.m_home = fields[4];
  out.m_shell = fields[5];
  return true;
}

void UserStoreService::serializePasswdLine(const user_record_t &record, pdiutil::string &out)
{
  char numbuf[8];
  out.clear();
  out += record.m_username;
  out += USER_STORE_FIELD_SEP;
  out += 'x';
  out += USER_STORE_FIELD_SEP;
  Uint32ToString((uint32_t)record.m_uid, numbuf, sizeof(numbuf));
  out += numbuf;
  out += USER_STORE_FIELD_SEP;
  Uint32ToString((uint32_t)record.m_gid, numbuf, sizeof(numbuf));
  out += numbuf;
  out += USER_STORE_FIELD_SEP;
  out += record.m_home;
  out += USER_STORE_FIELD_SEP;
  out += record.m_shell;
}

bool UserStoreService::scanPasswdFile(bool matchByUid, uint16_t uid, const char *username, user_record_t &out)
{
  if (!__i_fs.isFileExist(USER_STORE_PASSWD_PATH)) return false;
  int64_t fs = __i_fs.getFileSize(USER_STORE_PASSWD_PATH);
  if (fs <= 0) return false;

  uint64_t offset = 0;
  pdiutil::string linedata;
  bool found = false;

  while (offset < (uint64_t)fs && !found) {
    linedata.clear();
    int bytes = __i_fs.readFile(USER_STORE_PASSWD_PATH, 128, [&](char *data, uint32_t size) -> bool {
      linedata += pdiutil::string(data, size);
      return true;
    }, offset, "\n");
    if (bytes < 0) break;

    if (!linedata.empty() && linedata.back() == '\r') {
      linedata.pop_back();
    }

    user_record_t candidate;
    if (parsePasswdLine(linedata, candidate)) {
      if (matchByUid) {
        if (candidate.m_uid == uid) {
          out = candidate;
          found = true;
        }
      } else if (nullptr != username && candidate.m_username == username) {
        out = candidate;
        found = true;
      }
    }

    offset += (uint64_t)bytes + 1;
    __i_dvc_ctrl.yield();
  }

  return found;
}

bool UserStoreService::removeLineByUsername(const char *filepath, const char *username)
{
  if (!__i_fs.isFileExist(filepath)) return false;

  const char *tempdir = __i_fs.getTempDirectory();
  pdiutil::string temppath = pdiutil::string(tempdir) + __i_fs.basename(filepath);
  if (__i_fs.isFileExist(temppath.c_str())) {
    __i_fs.deleteFile(temppath.c_str());
  }

  int64_t fs = __i_fs.getFileSize(filepath);
  if (fs <= 0) return false;

  uint64_t offset = 0;
  bool removed = false;
  pdiutil::string linedata;
  size_t unlen = strlen(username);

  while (offset < (uint64_t)fs) {
    linedata.clear();
    int bytes = __i_fs.readFile(filepath, 128, [&](char *data, uint32_t size) -> bool {
      linedata += pdiutil::string(data, size);
      return true;
    }, offset, "\n");
    if (bytes < 0) break;

    bool matches = ( linedata.size() > unlen &&
                     linedata.compare(0, unlen, username) == 0 &&
                     linedata[unlen] == USER_STORE_FIELD_SEP );

    if (matches) {
      removed = true;
    } else if (!linedata.empty()) {
      pdiutil::string outline = linedata;
      outline += '\n';
      __i_fs.writeFile(temppath.c_str(), (char*)outline.c_str(), outline.size(), true);
    }

    offset += (uint64_t)bytes + 1;
    __i_dvc_ctrl.yield();
  }

  if (removed) {
    __i_fs.deleteFile(filepath);
    __i_fs.moveFile(temppath.c_str(), filepath);
  } else {
    __i_fs.deleteFile(temppath.c_str());
  }

  return removed;
}

bool UserStoreService::verifyPassword(const char *username, const char *password)
{
  if (nullptr == username || nullptr == password) return false;

  // /etc/shadow is 0600; auth is a system operation so bracket the read window
  // in a privileged scope (setuid-analog). Kept as narrow as possible.
  __i_fs.beginPrivileged();
  uint8_t storedHash[32];
  uint8_t salt[USER_STORE_SALT_LEN];
  bool haveRec = readShadowRecord(username, storedHash, salt);
  __i_fs.endPrivileged();
  if (!haveRec) return false;

  uint8_t computedHash[32];
  hashPassword(password, salt, USER_STORE_SALT_LEN, computedHash);

  uint8_t diff = 0;
  for (uint8_t i = 0; i < 32; i++) {
    diff |= (uint8_t)(storedHash[i] ^ computedHash[i]);
  }
  return (diff == 0);
}

bool UserStoreService::setPassword(const char *username, const char *password)
{
  if (nullptr == username || 0 == username[0] || nullptr == password) return false;

  uint8_t salt[USER_STORE_SALT_LEN];
  uint8_t hash[32];
  generateSalt(salt, USER_STORE_SALT_LEN);
  hashPassword(password, salt, USER_STORE_SALT_LEN, hash);

  char hexhash[65];
  char hexsalt[USER_STORE_SALT_LEN * 2 + 1];
  BytesToHexString(hash, 32, hexhash);
  BytesToHexString(salt, USER_STORE_SALT_LEN, hexsalt);

  pdiutil::string line = username;
  line += USER_STORE_FIELD_SEP;
  line += hexhash;
  line += USER_STORE_FIELD_SEP;
  line += hexsalt;
  line += '\n';

  __i_fs.beginPrivileged();
  removeLineByUsername(USER_STORE_SHADOW_PATH, username);

  if (!__i_fs.isFileExist(USER_STORE_SHADOW_PATH)) {
    if (__i_fs.createFile(USER_STORE_SHADOW_PATH, "") < 0) {
      __i_fs.endPrivileged();
      return false;
    }
    __i_fs.setFilePermissions(USER_STORE_SHADOW_PATH, 0600);
  }

  int iStatus = __i_fs.writeFile(USER_STORE_SHADOW_PATH, (char*)line.c_str(), line.size(), true);
  __i_fs.endPrivileged();
  return (iStatus >= 0);
}

bool UserStoreService::readShadowRecord(const char *username, uint8_t hashOut[32], uint8_t *saltOut)
{
  if (nullptr == username || nullptr == hashOut || nullptr == saltOut) return false;
  if (!__i_fs.isFileExist(USER_STORE_SHADOW_PATH)) return false;

  int64_t fs = __i_fs.getFileSize(USER_STORE_SHADOW_PATH);
  if (fs <= 0) return false;

  size_t unlen = strlen(username);
  uint64_t offset = 0;
  pdiutil::string linedata;
  bool found = false;

  while (offset < (uint64_t)fs && !found) {
    linedata.clear();
    int bytes = __i_fs.readFile(USER_STORE_SHADOW_PATH, 128, [&](char *data, uint32_t size) -> bool {
      linedata += pdiutil::string(data, size);
      return true;
    }, offset, "\n");
    if (bytes < 0) break;

    if (!linedata.empty() && linedata.back() == '\r') {
      linedata.pop_back();
    }

    pdiutil::string::size_type sep1 = linedata.find(USER_STORE_FIELD_SEP);
    if (sep1 != pdiutil::string::npos && sep1 == unlen &&
        linedata.compare(0, unlen, username) == 0) {

      pdiutil::string::size_type sep2 = linedata.find(USER_STORE_FIELD_SEP, sep1 + 1);
      if (sep2 != pdiutil::string::npos) {
        pdiutil::string::size_type hashlen = sep2 - sep1 - 1;
        pdiutil::string::size_type saltlen = linedata.size() - sep2 - 1;
        if (hashlen == 64 && saltlen == USER_STORE_SALT_LEN * 2) {
          if (HexStringToBytes(linedata.c_str() + sep1 + 1, 32, hashOut) &&
              HexStringToBytes(linedata.c_str() + sep2 + 1, USER_STORE_SALT_LEN, saltOut)) {
            found = true;
          }
        }
      }
    }

    offset += (uint64_t)bytes + 1;
    __i_dvc_ctrl.yield();
  }

  return found;
}

void UserStoreService::generateSalt(uint8_t *salt, uint8_t saltlen)
{
  static bool s_seeded = false;
  if (!s_seeded) {
    srand((unsigned)__i_dvc_ctrl.millis_now());
    s_seeded = true;
  }
  for (uint8_t i = 0; i < saltlen; i++) {
    salt[i] = (uint8_t)(rand() & 0xFF);
  }
}

void UserStoreService::hashPassword(const char *password, const uint8_t *salt, uint8_t saltlen, uint8_t out[32])
{
  sha256_context ctx;
  sha256_init(&ctx);
  if (nullptr != salt && saltlen > 0) {
    sha256_update(&ctx, salt, saltlen);
  }
  if (nullptr != password) {
    sha256_update(&ctx, (const unsigned char*)password, (unsigned int)strlen(password));
  }
  sha256_final(&ctx, out);
}

UserStoreService __user_store_service;

#endif
