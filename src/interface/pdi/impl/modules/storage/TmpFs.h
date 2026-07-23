/********************************** TmpFS *************************************
This file is part of the PDI Stack.

This is free software. You can redistribute it and/or modify it but without any
warranty.

TmpFS is a RAM-backed read/write filesystem. Unlike the synthetic proc/sys/dev
backends it actually stores file content and directories in the heap. Mounted at
/tmp for scratch space. Node count and total byte budget are bounded by
TmpFsConfig.h; everything is lost on reboot.

Author          : Suraj I.
Created Date    : 23rd July 2026
******************************************************************************/

#ifndef _TMP_FS_H
#define _TMP_FS_H

#include <config/Config.h>

#ifdef ENABLE_TMPFS

#include <config/TmpFsConfig.h>
#include <interface/pdi/modules/storage/iFileSystemInterface.h>
#include <interface/pdi/modules/storage/iStorageInterface.h>

struct tmpfs_node_t {
  pdiutil::string m_path; ///< normalized, no leading '/', e.g. "x/a.txt"
  pdiutil::string m_data; ///< file content (regular files only)
  file_type_t m_type;
  uint16_t m_perms;
  uint16_t m_uid;
  uint16_t m_gid;
  uint32_t m_ctime;
  uint32_t m_mtime;
};

class TmpFs : public iFileSystemInterface {
public:
  TmpFs();
  virtual ~TmpFs() {}

  int init() override { return 0; }

  int createFile(const char *path, const char *content, int64_t size = -1) override;
  int editFile(const char *path, uint64_t offset, const char *content, uint32_t size) override;
  int writeFile(const char *path, const char *content, uint32_t size, bool append = false) override;
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

  int createDirectory(const char *path) override;
  int deleteDirectory(const char *path) override;
  int rename(const char *oldPath, const char *newPath) override;
  int copyFile(const char *sourcePath, const char *destPath) override;
  int moveFile(const char *oldPath, const char *newPath) override;
  int deleteFile(const char *path) override;

  int64_t getFileSize(const char *path) override;
  int getDirFileList(const char *path, pdiutil::vector<file_info_t> &items,
                     const char *pattern = nullptr) override;
  bool isFileExist(const char *path) override;
  bool isDirExist(const char *path) override;
  bool isDirectory(const char *path) override;

  uint64_t getTotalSize() override { return TMPFS_MAX_BYTES; }
  uint64_t getUsedSize() override { return usedBytes(); }
  uint64_t getFreeSize() override {
    uint64_t used = usedBytes();
    return (used >= TMPFS_MAX_BYTES) ? 0 : (TMPFS_MAX_BYTES - used);
  }

  pdiutil::string getPWD() const override { return pdiutil::string("/tmp"); }
  bool setPWD(const char *path) override { return false; }
  pdiutil::string getLastPWD() const override { return pdiutil::string("/tmp"); }

  void appendFileSeparator(char *path) override {}
  void appendFileSeparator(pdiutil::string &path) override {}
  bool updatePathNotations(const char *path, pdiutil::string &updatedpath) override { return false; }
  bool changeDirectory(const char *path) override { return false; }
  const char *getRootDirectory() const override { return "/tmp"; }
  const char *getHomeDirectory() const override { return "/tmp"; }
  const char *getTempDirectory() const override { return "/tmp"; }
  bool setHomeDirectory(pdiutil::string &homedir) override { return false; }

  mimetype_t getFileMimeType(const pdiutil::string &path) override { return MIME_TYPE_TEXT_PLAIN; }
  pdiutil::string basename(const char *path) override;
  void applyFileSizeLimit(pdiutil::string &name, uint32_t sizelimit = FILE_NAME_MAX_SIZE) override {}

  int setFileAttr(const char *path, uint8_t type, const void *buffer, uint32_t size) override;
  int getFileAttr(const char *path, uint8_t type, void *buffer, uint32_t size) override;
  int removeFileAttr(const char *path, uint8_t type) override { return -1; }
  int getFileMeta(const char *path, file_info_t &out) override;
  int setFilePermissions(const char *path, uint16_t perms) override;
  int setFileOwner(const char *path, uint16_t uid, uint16_t gid) override;
  int touch(const char *path) override;

protected:
  uint32_t nowEpoch() override;
  void currentOwner(uint16_t &uid, uint16_t &gid) override;
  uint16_t currentUmask() override;

private:
  pdiutil::vector<tmpfs_node_t> m_nodes;

  pdiutil::string normalize(const char *path) const;
  int findNode(const pdiutil::string &norm) const;
  pdiutil::string parentOf(const pdiutil::string &norm) const;
  bool parentExists(const pdiutil::string &norm) const;
  uint32_t usedBytes() const;
  void stampNew(tmpfs_node_t &n, uint16_t defperms);
  int putContent(const pdiutil::string &norm, const char *content, uint32_t size, bool append);
};

extern TmpFs __i_tmpfs;

#endif // ENABLE_TMPFS

#endif
