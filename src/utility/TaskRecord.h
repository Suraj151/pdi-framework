/******************************** Task record *********************************
This file is part of the PDI stack.

This is free software. You can redistribute it and/or modify it but without any
warranty.

The text encoding of a scheduler task. One line carries every field a reader
needs, so a display and a generated node agree by construction rather than by
two pieces of code being kept in step.

Author          : Suraj I.
Created Date    : 27th Aug 2026
******************************************************************************/

#ifndef _TASK_RECORD_H_
#define _TASK_RECORD_H_

#include "TaskScheduler.h"

/**
 * A task's name as ordinary ram, since it is a read only pointer that may not
 * be dereferenced directly on every port.
 */
pdiutil::string taskDisplayName(const task_t *task);

/**
 * The letter a lifecycle state reads as, ready where it is not a known one.
 */
char taskStateLetter(task_state_t state);

/**
 * The letter a scheduling policy reads as, fifo where it is not a known one.
 */
char taskPolicyLetter(task_policy_t policy);

/**
 * The letter an execution mode reads as, inline where it is not a known one.
 */
char taskModeLetter(task_mode_t mode);

/**
 * A task's lifetime share of the clock, scaled by a hundred so it stays whole.
 */
uint32_t taskCpuShare(uint64_t exec_us, uint64_t created_ms, uint64_t now_ms);

/**
 * The one line a task reads as, leading with the pid, the bracketed name and
 * then the state.
 */
void taskStatLine(const task_t *task, pdiutil::string &out);

/**
 * The nth field of such a line, counting the bracketed name as one field.
 */
bool taskStatField(const pdiutil::string &line, uint8_t index, pdiutil::string &out);

/**
 * The same field read straight as a number, zero where it is not a number.
 */
int64_t taskStatNumber(const pdiutil::string &line, uint8_t index);

#endif
