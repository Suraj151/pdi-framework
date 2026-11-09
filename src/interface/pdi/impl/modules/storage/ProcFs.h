/********************************** ProcFS *************************************
This file is part of the PDI Stack.

This is free software. You can redistribute it and/or modify it but without any
warranty.

ProcFS is a synthetic read-only filesystem mounted at /proc. Node contents are
generated dynamically on read, and the tree has depth: a directory per running
task sits alongside the flat nodes.

Author          : Suraj I.
Created Date    : 21st July 2026
******************************************************************************/

#ifndef _PROC_FS_H
#define _PROC_FS_H

#include <config/Config.h>

#ifdef ENABLE_PROCFS

#include "SynthFs.h"

class ProcFs : public SynthFs {
public:
  ProcFs();
  virtual ~ProcFs() {}

protected:
  synth_node_t resolve(const char *path) override;
  pdiutil::string render(const char *path) override;
  int listChildren(const char *path,
                   pdiutil::vector<file_info_t> &items) override;

private:
  enum ProcNode : uint8_t {
    PROC_INVALID = 0,
    PROC_ROOT,
    PROC_TOPFILE,
    PROC_TASKDIR,
    PROC_TASKFILE,
    PROC_NETDIR,
    PROC_NETFILE
  };
  typedef enum ProcNode proc_node_t;

  /**
   * Which node the path names, and for a task node which task it belongs to.
   */
  proc_node_t classify(const char *path, int32_t &taskid_out,
                       uint8_t &leaf_out) const;

  /**
   * What the heap holds, and how much of it is in one piece.
   */
  pdiutil::string renderMemInfo();

  /**
   * The processor the stack runs on, one block per core, then the program flash
   * and the platform beneath the stack; unknown for what the port cannot report.
   */
  pdiutil::string renderCpuInfo();

  /**
   * One line per mount, in the order the dispatcher searches them.
   */
  pdiutil::string renderMounts();

  /**
   * Cumulative scheduler counters, in microseconds rather than jiffies.
   */
  pdiutil::string renderStat();

  /**
   * The routes the registered interfaces provide, each network they reach
   * directly and the default route wherever a gateway answers for one.
   */
  pdiutil::string renderNetRoute();

  /**
   * Per interface traffic, for the interfaces that can count it.
   */
  pdiutil::string renderNetDev();

  /**
   * Every TCP endpoint the stack holds, when the port can enumerate them.
   */
  pdiutil::string renderNetTcp();
};

extern ProcFs __i_procfs;

#endif // ENABLE_PROCFS

#endif
