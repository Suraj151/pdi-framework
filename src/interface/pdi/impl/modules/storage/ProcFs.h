/********************************** ProcFS *************************************
This file is part of the PDI Stack.

This is free software. You can redistribute it and/or modify it but without any
warranty.

ProcFS is a synthetic read-only filesystem mounted at /proc. Node contents
(/proc/uptime, /proc/meminfo, /proc/version, /proc/tasks, /proc/scheduler,
/proc/net) are generated dynamically on read.

Author          : Suraj I.
Created Date    : 21st July 2026
******************************************************************************/

#ifndef _PROC_FS_H
#define _PROC_FS_H

#include <config/Config.h>

#ifdef ENABLE_PROCFS

#include <interface/pdi/modules/storage/iFileSystemInterface.h>
#include <interface/pdi/modules/storage/iStorageInterface.h>

class ProcFs : public iFileSystemInterface {
public:
  ProcFs();
  virtual ~ProcFs() {}

  int init() override { return 0; }

  int createFile(const char *path, const char *content,
                 int64_t size = -1) override {
    return -1;
  }
  int editFile(const char *path, uint64_t offset, const char *content,
               uint32_t size) override {
    return -1;
  }
  int writeFile(const char *path, const char *content, uint32_t size,
                bool append = false) override {
    return -1;
  }
  int readFile(const char *path, uint64_t size,
               pdiutil::function<bool(char *, uint32_t)> readbackfn,
               uint64_t offset = 0, const char *readUntilMatchStr = nullptr,
               bool *didmatchfound = nullptr) override;

  int64_t getOffsetFromLineNumber(const char *path, int linenumber,
                                  CallBackVoidArgFn yield = nullptr) override { return -1; }
  int64_t getLineNumberFromOffset(const char *path, int64_t offset,
                                  CallBackVoidArgFn yield = nullptr) override { return -1; }
  int findInFile(const char *path, const char *findStr,
                 pdiutil::vector<uint32_t> *findindices, int maxindices = -1,
                 int everynthindice = 1, int64_t offset = 0,
                 CallBackVoidArgFn yield = nullptr) override { return -1; }
  int getLineNumbersInFile(const char *path,
                           pdiutil::vector<uint32_t> &linenumberindices,
                           int maxlinenumbers = -1, int linenumberoffset = 0,
                           CallBackVoidArgFn yield = nullptr) override { return -1; }
  int readLineInFile(const char *path, int32_t linenumber,
                     pdiutil::string &linedata, const char *pattern = nullptr,
                     CallBackVoidArgFn yield = nullptr) override { return -1; }

  int createDirectory(const char *path) override { return -1; }
  int deleteDirectory(const char *path) override { return -1; }
  int rename(const char *oldPath, const char *newPath) override { return -1; }
  int copyFile(const char *sourcePath, const char *destPath) override {
    return -1;
  }
  int moveFile(const char *oldPath, const char *newPath) override { return -1; }
  int deleteFile(const char *path) override { return -1; }

  int64_t getFileSize(const char *path) override;
  int getDirFileList(const char *path, pdiutil::vector<file_info_t> &items,
                     const char *pattern = nullptr) override;
  bool isFileExist(const char *path) override;
  bool isDirExist(const char *path) override;
  bool isDirectory(const char *path) override;

  uint64_t getTotalSize() override { return 0; }
  uint64_t getUsedSize() override { return 0; }
  uint64_t getFreeSize() override { return 0; }

  pdiutil::string getPWD() const override { return pdiutil::string("/proc"); }
  bool setPWD(const char *path) override { return false; }
  pdiutil::string getLastPWD() const override { return pdiutil::string("/proc"); }

  void appendFileSeparator(char *path) override {}
  void appendFileSeparator(pdiutil::string &path) override {}
  bool updatePathNotations(const char *path, pdiutil::string &updatedpath) override { return false; }
  bool changeDirectory(const char *path) override { return false; }
  const char *getRootDirectory() const override { return "/proc"; }
  const char *getHomeDirectory() const override { return "/proc"; }
  const char *getTempDirectory() const override { return "/proc"; }
  bool setHomeDirectory(pdiutil::string &homedir) override { return false; }

  mimetype_t getFileMimeType(const pdiutil::string &path) override { return MIME_TYPE_TEXT_PLAIN; }
  pdiutil::string basename(const char *path) override;
  void applyFileSizeLimit(pdiutil::string &name, uint32_t sizelimit = FILE_NAME_MAX_SIZE) override {}

  int setFileAttr(const char *path, uint8_t type, const void *buffer,
                  uint32_t size) override {
    return -1;
  }
  int getFileAttr(const char *path, uint8_t type, void *buffer,
                  uint32_t size) override;
  int removeFileAttr(const char *path, uint8_t type) override { return -1; }
  int getFileMeta(const char *path, file_info_t &out) override;
  int setFilePermissions(const char *path, uint16_t perms) override { return -1; }
  int setFileOwner(const char *path, uint16_t uid, uint16_t gid) override { return -1; }
  int touch(const char *path) override { return -1; }

protected:
  uint32_t nowEpoch() override { return 0; }

private:
  pdiutil::string generateContent(const char *path);
  const char *normalizePath(const char *path) const;
};

extern ProcFs __i_procfs;

#endif // ENABLE_PROCFS

#endif
