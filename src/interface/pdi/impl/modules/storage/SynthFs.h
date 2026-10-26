/******************************** Synthetic FS *********************************
This file is part of the PDI Stack.

This is free software. You can redistribute it and/or modify it but without any
warranty.

Shared base for the filesystems whose contents are generated rather than
stored. A subclass says what a path is, what it holds and what a directory
contains; everything else - the read loop, the metadata, the operations a
generated tree cannot support - is answered here.

Author          : Suraj I.
Created Date    : 27th Aug 2026
******************************************************************************/

#ifndef _SYNTH_FS_H
#define _SYNTH_FS_H

#include <config/Config.h>

#if defined(ENABLE_PROCFS) || defined(ENABLE_SYSFS)

#include <interface/pdi/modules/storage/iFileSystemInterface.h>
#include <interface/pdi/modules/storage/iStorageInterface.h>

class SynthFs : public iFileSystemInterface {
public:
  SynthFs(iStorageInterface &storage, const char *root);
  virtual ~SynthFs() {}

  pdi_err_t init() override { return 0; }

  int createFile(const char *path, const char *content,
                 int64_t size = -1) override {
    return PDI_ERR_NOT_SUPPORTED;
  }
  int editFile(const char *path, uint64_t offset, const char *content,
               uint32_t size) override {
    return PDI_ERR_NOT_SUPPORTED;
  }
  int writeFile(const char *path, const char *content, uint32_t size,
                bool append = false) override {
    return PDI_ERR_NOT_SUPPORTED;
  }
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
  pdi_err_t copyFile(const char *sourcePath, const char *destPath, CallBackVoidArgBoolRetFn keepgoing = nullptr) override { return PDI_ERR_NOT_SUPPORTED; }
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

  pdiutil::string getPWD() const override { return pdiutil::string(m_root); }
  bool setPWD(const char *path) override { return false; }
  pdiutil::string getLastPWD() const override { return pdiutil::string(m_root); }

  void appendFileSeparator(char *path) override {}
  void appendFileSeparator(pdiutil::string &path) override {}
  bool updatePathNotations(const char *path, pdiutil::string &updatedpath) override { return false; }
  bool changeDirectory(const char *path) override { return false; }
  const char *getRootDirectory() const override { return m_root; }
  const char *getHomeDirectory() const override { return m_root; }
  const char *getTempDirectory() const override { return m_root; }
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
  enum SynthNode : uint8_t {
    SYNTH_NONE = 0,
    SYNTH_DIR,
    SYNTH_FILE
  };
  typedef enum SynthNode synth_node_t;

  /**
   * What the path names in this tree, if anything. Called on every metadata
   * question, so a subclass keeps it cheap and free of side effects.
   */
  virtual synth_node_t resolve(const char *path) = 0;

  /**
   * The whole content of a file node, built fresh. An empty string means the
   * path names nothing readable.
   */
  virtual pdiutil::string render(const char *path) = 0;

  /**
   * Fill items with one entry per child of a directory node, using addEntry.
   */
  virtual int listChildren(const char *path,
                           pdiutil::vector<file_info_t> &items) = 0;

  /**
   * How long a file node reads. Rendering it is the honest default; a node
   * whose content is expensive to build overrides this and computes instead.
   */
  virtual int64_t sizeOf(const char *path);

  /**
   * The permission bits a node carries. Read-only trees keep the default.
   */
  virtual uint16_t permsFor(const char *path, synth_node_t kind);

  /**
   * The path with its leading separators removed, relative to the mount.
   */
  static const char *normalizePath(const char *path);

  /**
   * Consume one path segment when it equals the literal, else leave the cursor.
   */
  static bool matchSegment(const char *&cursor, const char *literal);

  /**
   * Consume a numeric path segment, or report why it is not one.
   */
  static int32_t numberSegment(const char *&cursor);

  /**
   * Step over the separator between two segments.
   */
  static bool nextSegment(const char *&cursor);

  /**
   * Append one directory entry, taking a heap copy of the name because the
   * callers that list a directory free it.
   */
  static void addEntry(pdiutil::vector<file_info_t> &items, const char *name,
                       file_type_t type, uint16_t perms, int64_t size = 0);

  uint32_t nowEpoch() override { return 0; }

  const char *m_root;
};

#endif // ENABLE_PROCFS || ENABLE_SYSFS

#endif
