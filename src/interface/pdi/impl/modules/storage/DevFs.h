/********************************** DevFS **************************************
This file is part of the PDI Stack.

This is free software. You can redistribute it and/or modify it but without any
warranty.

DevFS is a synthetic filesystem mounted at /dev exposing byte-stream device
nodes. /dev/null discards writes and reads as EOF; /dev/zero reads as 0x00;
/dev/random and /dev/urandom read as random bytes. Unbounded reads are capped at
DEVFS_STREAM_READ_MAX bytes per call so `cat` cannot spin forever on an MCU.

Author          : Suraj I.
Created Date    : 23rd July 2026
******************************************************************************/

#ifndef _DEV_FS_H
#define _DEV_FS_H

#include <config/Config.h>

#ifdef ENABLE_DEVFS

#include <interface/pdi/modules/storage/iFileSystemInterface.h>
#include <interface/pdi/modules/storage/iStorageInterface.h>

class iBlockDriverInterface;

class DevFs : public iFileSystemInterface {
public:
  DevFs();
  virtual ~DevFs() {}

  pdi_err_t init() override { return 0; }

  /**
   * The node a path under /dev names, e.g. mmcblk0p1 for /dev/mmcblk0p1;
   * nullptr for a path anywhere else.
   */
  static const char *nodeOf(const char *path);

  /**
   * The path under /dev of a node, e.g. /dev/mmcblk0p1 for mmcblk0p1.
   */
  static pdiutil::string pathOf(const char *node);

#ifdef ENABLE_DRIVER_CORE
  /**
   * The disk a disk or partition node reaches and the blocks it covers, nullptr
   * when the node names neither.
   */
  iBlockDriverInterface *blockWindow(const char *path, uint32_t &first, uint32_t &span) const;
#endif

  int createFile(const char *path, const char *content,
                 int64_t size = -1) override {
    return PDI_ERR_NOT_SUPPORTED;
  }
  int editFile(const char *path, uint64_t offset, const char *content,
               uint32_t size) override;
  int writeFile(const char *path, const char *content, uint32_t size,
                bool append = false) override;
  int readFile(const char *path, uint64_t size,
               pdiutil::function<bool(char *, uint32_t)> readbackfn,
               uint64_t offset = 0, const char *readUntilMatchStr = nullptr,
               bool *didmatchfound = nullptr) override;

  int64_t getOffsetFromLineNumber(const char *path, int linenumber,
                                  CallBackVoidArgFn yield = nullptr) override { return PDI_ERR_NOT_SUPPORTED; }
  int64_t getLineNumberFromOffset(const char *path, int64_t offset,
                                  CallBackVoidArgFn yield = nullptr) override { return PDI_ERR_NOT_SUPPORTED; }
  int findInFile(const char *path, const char *findStr,
                 pdiutil::vector<uint32_t> *findindices, int maxindices = -1,
                 int everynthindice = 1, int64_t offset = 0,
                 CallBackVoidArgFn yield = nullptr) override { return PDI_ERR_NOT_SUPPORTED; }
  int getLineNumbersInFile(const char *path,
                           pdiutil::vector<uint32_t> &linenumberindices,
                           int maxlinenumbers = -1, int linenumberoffset = 0,
                           CallBackVoidArgFn yield = nullptr) override { return PDI_ERR_NOT_SUPPORTED; }
  int readLineInFile(const char *path, int32_t linenumber,
                     pdiutil::string &linedata, const char *pattern = nullptr,
                     CallBackVoidArgFn yield = nullptr) override { return PDI_ERR_NOT_SUPPORTED; }

  pdi_err_t createDirectory(const char *path) override { return PDI_ERR_NOT_SUPPORTED; }
  pdi_err_t deleteDirectory(const char *path) override { return PDI_ERR_NOT_SUPPORTED; }
  pdi_err_t rename(const char *oldPath, const char *newPath) override { return PDI_ERR_NOT_SUPPORTED; }
  pdi_err_t copyFile(const char *sourcePath, const char *destPath, CallBackVoidArgBoolRetFn keepgoing = nullptr) override {
    return PDI_ERR_NOT_SUPPORTED;
  }
  pdi_err_t moveFile(const char *oldPath, const char *newPath) override { return PDI_ERR_NOT_SUPPORTED; }
  pdi_err_t deleteFile(const char *path) override { return PDI_ERR_NOT_SUPPORTED; }

  int64_t getFileSize(const char *path) override;
  int getDirFileList(const char *path, pdiutil::vector<file_info_t> &items,
                     const char *pattern = nullptr) override;
  bool isFileExist(const char *path) override;
  bool isDirExist(const char *path) override;
  bool isDirectory(const char *path) override;

  uint64_t getTotalSize() override { return 0; }
  uint64_t getUsedSize() override { return 0; }
  uint64_t getFreeSize() override { return 0; }

  pdiutil::string getPWD() const override { return pdiutil::string(DEV_MOUNT_PREFIX); }
  bool setPWD(const char *path) override { return false; }
  pdiutil::string getLastPWD() const override { return pdiutil::string(DEV_MOUNT_PREFIX); }

  void appendFileSeparator(char *path) override {}
  void appendFileSeparator(pdiutil::string &path) override {}
  bool updatePathNotations(const char *path, pdiutil::string &updatedpath) override { return false; }
  bool changeDirectory(const char *path) override { return false; }
  const char *getRootDirectory() const override { return DEV_MOUNT_PREFIX; }
  const char *getHomeDirectory() const override { return DEV_MOUNT_PREFIX; }
  const char *getTempDirectory() const override { return DEV_MOUNT_PREFIX; }
  bool setHomeDirectory(pdiutil::string &homedir) override { return false; }

  mimetype_t getFileMimeType(const pdiutil::string &path) override { return MIME_TYPE_TEXT_PLAIN; }
  pdiutil::string basename(const char *path) override;
  void applyFileSizeLimit(pdiutil::string &name, uint32_t sizelimit = FILE_NAME_MAX_SIZE) override {}

  int setFileAttr(const char *path, uint8_t type, const void *buffer,
                  uint32_t size) override {
    return PDI_ERR_NOT_SUPPORTED;
  }
  int getFileAttr(const char *path, uint8_t type, void *buffer,
                  uint32_t size) override;
  pdi_err_t removeFileAttr(const char *path, uint8_t type) override { return PDI_ERR_NOT_SUPPORTED; }
  pdi_err_t getFileMeta(const char *path, file_info_t &out) override;
  int setFilePermissions(const char *path, uint16_t perms) override { return PDI_ERR_NOT_SUPPORTED; }
  int setFileOwner(const char *path, uint16_t uid, uint16_t gid) override { return PDI_ERR_NOT_SUPPORTED; }
  pdi_err_t touch(const char *path) override { return PDI_ERR_NOT_SUPPORTED; }

protected:
  uint32_t nowEpoch() override { return 0; }

private:
  enum NodeKind : uint8_t {
    DEV_INVALID = 0,
    DEV_ROOT,
    DEV_NULL,
    DEV_ZERO,
    DEV_RANDOM,
    DEV_URANDOM,
    DEV_DRIVER_DIR,
    DEV_DRIVER_NODE,
    DEV_PARTITION
  };

  NodeKind classify(const char *path) const;

#ifdef ENABLE_DRIVER_CORE
  /**
   * The bound device whose node the path names, or the first one under the
   * directory it names; -1 when it names neither.
   */
  int16_t driverNode(const char *p, bool &isdir) const;

  /**
   * Adds the next path segment of each bound driver's node under a driver directory
   * or the root, a directory when more follows.
   */
  void listDriverNodes(const char *p, pdiutil::vector<file_info_t> &items) const;

  /**
   * Delivers a bounded run of bytes read from a driver's node, capped so a reader
   * always reaches an end.
   */
  int driverRead(int16_t device, uint64_t size,
                 pdiutil::function<bool(char *, uint32_t)> readbackfn,
                 uint64_t offset);

  /**
   * Whether the node names a disk or one of its partitions.
   */
  bool isDisk(const char *path) const;

  /**
   * The disk a partition node names and the partition's number, -1 when the
   * path names none.
   */
  int16_t partitionNode(const char *p, uint8_t &number) const;

  /**
   * The disk a disk or partition node reaches and the blocks it covers.
   */
  bool diskWindow(const char *path, int16_t &device, uint32_t &first, uint32_t &span) const;

  /**
   * Delivers a disk window from the offset a block at a time.
   */
  int diskRead(int16_t device, uint32_t first, uint32_t span, uint64_t size,
               pdiutil::function<bool(char *, uint32_t)> readbackfn,
               uint64_t offset, const char *readUntilMatchStr, bool *didmatchfound);

  /**
   * Stores bytes in a disk window from the offset, a block at a time.
   */
  int diskWrite(int16_t device, uint32_t first, uint32_t span, uint64_t offset, const char *content, uint32_t size);

  /**
   * The bytes a disk or partition node holds, 0 for any other node.
   */
  uint64_t diskSize(const char *path) const;
#endif
  const char *normalizePath(const char *path) const;
  int streamFill(bool random, uint64_t size,
                 pdiutil::function<bool(char *, uint32_t)> readbackfn,
                 uint64_t offset, const char *readUntilMatchStr = nullptr,
                 bool *didmatchfound = nullptr);
};

extern DevFs __i_devfs;

#endif // ENABLE_DEVFS

#endif
