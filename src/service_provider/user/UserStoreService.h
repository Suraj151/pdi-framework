/************************** User Store Service ********************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 18th July 2026
******************************************************************************/
#ifndef _USER_STORE_SERVICE_H_
#define _USER_STORE_SERVICE_H_

#include <service_provider/ServiceProvider.h>

#ifdef ENABLE_AUTH_SERVICE

/**
 * UserStoreService class
 */
class UserStoreService : public ServiceProvider {

public:

  UserStoreService();
  ~UserStoreService();

  bool initService(void *arg = nullptr) override;

  bool findUserByName(const char *username, user_record_t &out);
  bool findUserByUid(uint16_t uid, user_record_t &out);
  bool addUser(const user_record_t &record, const char *password);
  bool removeUser(const char *username);

  bool verifyPassword(const char *username, const char *password);
  bool setPassword(const char *username, const char *password);

  /**
   * @brief Resolves an owning uid and gid to display names.
   *
   * Falls back to the numeric id when no record matches, and reuses the owner
   * name for the group when the two ids are equal, which is the common case
   * and halves the lookups.
   */
  void resolveOwnerNames(uint16_t uid, uint16_t gid, pdiutil::string &owner, pdiutil::string &group);

private:

  void generateSalt(uint8_t *salt, uint8_t saltlen);
  void hashPassword(const char *password, const uint8_t *salt, uint8_t saltlen, uint8_t out[32]);

  bool parsePasswdLine(const pdiutil::string &line, user_record_t &out);
  void serializePasswdLine(const user_record_t &record, pdiutil::string &out);
  bool scanPasswdFile(bool matchByUid, uint16_t uid, const char *username, user_record_t &out);
  bool removeLineByUsername(const char *filepath, const char *username);
  bool readShadowRecord(const char *username, uint8_t hashOut[32], uint8_t *saltOut);
  void resolveNameByUid(uint16_t uid, pdiutil::string &out);
#ifdef ENABLE_STORAGE_SERVICE
  void bootstrapFromLoginTable();
#endif

  bool m_initStatus;
};

extern UserStoreService __user_store_service;

#endif

#endif
