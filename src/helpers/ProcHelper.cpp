/******************************** Proc helper *********************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 27th Aug 2026
******************************************************************************/

#include "ProcHelper.h"
#include <utility/DataTypeConversions.h>
#include <utility/TaskRecord.h>

/**
 * Hands every line of a file to the callback in turn, stopping where it answers
 * false; false when nothing is readable.
 */
bool readProcLines(const char *path, pdiutil::function<bool(pdiutil::string &)> online)
{
#ifdef ENABLE_STORAGE_SERVICE
  if (nullptr == path || !online || !__i_fs.isFileExist(path))
  {
    return false;
  }

  int64_t total = __i_fs.getFileSize(path);
  if (total <= 0)
  {
    return false;
  }

  uint64_t offset = 0;
  pdiutil::string linedata;

  while (offset < (uint64_t)total)
  {
    linedata.clear();
    int bytes = __i_fs.readFile(path, 64, [&](char *data, uint32_t size) -> bool {
      linedata += pdiutil::string(data, size);
      return true;
    }, offset, "\n");
    if (bytes < 0) break;

    offset += (uint64_t)bytes + 1;

    if (!linedata.empty() && linedata.back() == '\r')
    {
      linedata.pop_back();
    }

    if (!linedata.empty() && !online(linedata)) break;

    __i_dvc_ctrl.yield();
  }

  return true;
#else
  (void)path; (void)online;
  return false;
#endif
}

#ifdef ENABLE_CMD_SERVICE

namespace {

/**
 * True where a directory entry names a task, which is to say a pid.
 */
bool isTaskEntry(const char *name)
{
  return (nullptr != name && name[0] >= '0' && name[0] <= '9');
}

#ifdef ENABLE_PROCFS
/**
 * Reads the one line a task's node holds, false where it holds nothing.
 */
bool readTaskStat(const char *entry, pdiutil::string &line)
{
  pdiutil::string path = CHARPTR_WRAP(PROC_MOUNT_PREFIX);
  path += "/";
  path += entry;
  path += CHARPTR_WRAP("/stat");

  line.clear();
  readProcLines(path.c_str(), [&line](pdiutil::string &read) -> bool {
    line = read;
    return false;
  });

  return !line.empty();
}
#endif

const uint8_t PS_COL_NARROW = 5;
const uint8_t PS_COL_CPU = 7;
const uint8_t PS_COL_WIDE = 10;

/**
 * Writes one column of a line, padded so the table stays in step.
 */
void writeColumn(iTerminalInterface *terminal, const pdiutil::string &value, uint8_t width)
{
  terminal->write_pad(value.c_str(), width);
}

/**
 * Writes the lifetime average share of a task, as a percentage to two places.
 */
void writeCpuShare(iTerminalInterface *terminal, uint64_t exec_us, uint64_t created_ms, uint64_t now_ms)
{
  uint32_t share = taskCpuShare(exec_us, created_ms, now_ms);

  char buf[24];
  Int64ToString((int64_t)(share / 100), buf, sizeof(buf), 0);

  pdiutil::string value(buf);
  value += ".";
  value += (char)('0' + (char)((share % 100) / 10));
  value += (char)('0' + (char)(share % 10));

  writeColumn(terminal, value, PS_COL_CPU);
}

/**
 * Writes the columns of one task, read out of the line a task node reads as.
 */
void writeStatRow(iTerminalInterface *terminal, const pdiutil::string &line, uint64_t now_ms)
{
  pdiutil::string field;

  taskStatField(line, 0, field);
  writeColumn(terminal, field, PS_COL_NARROW);
  taskStatField(line, 3, field);
  writeColumn(terminal, field, PS_COL_NARROW);
  taskStatField(line, 2, field);
  writeColumn(terminal, field, 4);
  taskStatField(line, 4, field);
  writeColumn(terminal, field, PS_COL_NARROW);
  taskStatField(line, 5, field);
  writeColumn(terminal, field, PS_COL_NARROW);
  taskStatField(line, 10, field);
  writeColumn(terminal, field, PS_COL_NARROW);

  writeCpuShare(terminal, (uint64_t)taskStatNumber(line, 7),
                (uint64_t)taskStatNumber(line, 8), now_ms);

  taskStatField(line, 6, field);
  writeColumn(terminal, field, PS_COL_WIDE);
  taskStatField(line, 9, field);
  writeColumn(terminal, field, PS_COL_WIDE);

  taskStatField(line, 1, field);
  terminal->writeln(field.c_str());
}

/**
 * Writes the header the table is read under, summary line first.
 */
void writeHeader(iTerminalInterface *terminal, uint64_t now_ms, uint16_t shown, uint32_t freeheap)
{
  char buf[24];

  terminal->writeln();
  terminal->write_ro(RODT_ATTR("top - up "));
  Int64ToString((int64_t)(now_ms / 1000), buf, sizeof(buf), 0);
  terminal->write(buf);
  terminal->write_ro(RODT_ATTR("s, "));
  Int32ToString((int32_t)shown, buf, sizeof(buf), 0);
  terminal->write(buf);
  terminal->write_ro(RODT_ATTR(" tasks, "));
  Int32ToString((int32_t)freeheap, buf, sizeof(buf), 0);
  terminal->write(buf);
  terminal->writeln_ro(RODT_ATTR(" bytes free heap"));

  terminal->write_ro(RODT_ATTR("PID  "));
  terminal->write_ro(RODT_ATTR("OWN  "));
  terminal->write_ro(RODT_ATTR("ST  "));
  terminal->write_ro(RODT_ATTR("PRI  "));
  terminal->write_ro(RODT_ATTR("NI   "));
  terminal->write_ro(RODT_ATTR("POL  "));
  terminal->write_ro(RODT_ATTR("%CPU   "));
  terminal->write_ro(RODT_ATTR("RUNS      "));
  terminal->write_ro(RODT_ATTR("INTVL     "));
  terminal->writeln_ro(RODT_ATTR("NAME"));
}

}

/**
 * Writes the process table a session may see, filtered to one owner or to all.
 */
void printProcessTable(iTerminalInterface *terminal, uint8_t filter_owner)
{
  if (nullptr == terminal) return;

  uint64_t now = __i_dvc_ctrl.millis_now();

#ifdef ENABLE_PROCFS
  pdiutil::vector<file_info_t> items;
  __i_fs.getDirFileList(PROC_MOUNT_PREFIX, items);

  pdiutil::string line;
  uint16_t shown = 0;

  for (uint32_t i = 0; i < items.size(); i++)
  {
    if (!isTaskEntry(items[i].m_name)) continue;

    // an unfiltered listing shows every entry, so only a filtered one pays for
    // reading each node twice rather than holding every line in memory at once
    if (0xFF != filter_owner)
    {
      if (!readTaskStat(items[i].m_name, line)) continue;
      if ((uint8_t)taskStatNumber(line, 3) != filter_owner) continue;
    }
    shown++;
  }

  writeHeader(terminal, now, shown, __i_dvc_ctrl.get_free_heap());

  for (uint32_t i = 0; i < items.size(); i++)
  {
    if (!isTaskEntry(items[i].m_name)) continue;
    if (!readTaskStat(items[i].m_name, line)) continue;
    if (0xFF != filter_owner && (uint8_t)taskStatNumber(line, 3) != filter_owner) continue;

    writeStatRow(terminal, line, now);
    __i_dvc_ctrl.yield();
  }

  for (uint32_t i = 0; i < items.size(); i++)
  {
    pdiutil::safe_delete_array(items[i].m_name);
  }
#else
  uint16_t shown = 0;
  for (uint16_t i = 0; i < __task_scheduler.getTaskSlots(); i++)
  {
    task_t *task = __task_scheduler.getTaskByIndex(i);
    if (nullptr == task) continue;
    if (filter_owner != 0xFF && task->m_owner != filter_owner) continue;
    shown++;
  }

  writeHeader(terminal, now, shown, __i_dvc_ctrl.get_free_heap());

  pdiutil::string line;

  for (uint16_t i = 0; i < __task_scheduler.getTaskSlots(); i++)
  {
    task_t *task = __task_scheduler.getTaskByIndex(i);
    if (nullptr == task) continue;
    if (filter_owner != 0xFF && task->m_owner != filter_owner) continue;

    taskStatLine(task, line);
    writeStatRow(terminal, line, now);
    __i_dvc_ctrl.yield();
  }
#endif
}

#endif
