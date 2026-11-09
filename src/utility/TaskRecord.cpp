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

#include "TaskRecord.h"
#include "DataTypeConversions.h"

namespace {

/**
 * Append a decimal number, which every numeric field of the line is built from.
 */
void appendNumber(pdiutil::string &out, int64_t value) {
    char buf[24];
    Int64ToString(value, buf, sizeof(buf), 0);
    out += buf;
}

}

/**
 * The letter a lifecycle state reads as, ready where it is not a known one.
 */
char taskStateLetter(task_state_t state) {
    switch (state) {
        case TASK_STATE_RUNNING:  return 'R';
        case TASK_STATE_SLEEPING: return 'S';
        case TASK_STATE_STOPPED:  return 'T';
        case TASK_STATE_ZOMBIE:   return 'Z';
        default:                  return 'r';
    }
}

/**
 * The letter a scheduling policy reads as, fifo where it is not a known one.
 */
char taskPolicyLetter(task_policy_t policy) {
    switch (policy) {
        case TASK_POLICY_ROUNDROBIN: return 'R';
        case TASK_POLICY_DEADLINE:   return 'D';
        case TASK_POLICY_FAIRSHARE:  return 'S';
        default:                     return 'F';
    }
}

/**
 * The letter an execution mode reads as, inline where it is not a known one.
 */
char taskModeLetter(task_mode_t mode) {
#ifdef ENABLE_CONTEXTUAL_EXECUTION
    switch (mode) {
        case TASK_MODE_COOPERATIVE: return 'C';
        case TASK_MODE_PREEMPTIVE:  return 'P';
        default:                    return 'I';
    }
#else
    (void)mode;
    return 'I';
#endif
}

/**
 * A task's lifetime share of the clock, scaled by a hundred so it stays whole.
 */
uint32_t taskCpuShare(uint64_t exec_us, uint64_t created_ms, uint64_t now_ms) {
    uint64_t elapsed = (now_ms > created_ms) ? (now_ms - created_ms) : 1;
    uint64_t share = (exec_us * 10ULL) / elapsed;
    return (share > 99999ULL) ? 99999 : (uint32_t)share;
}

/**
 * A task's name as ordinary ram, since it is a read only pointer that may not
 * be dereferenced directly on every port.
 */
pdiutil::string taskDisplayName(const task_t *task) {
    if (nullptr == task || nullptr == task->m_name) {
        return pdiutil::string("-");
    }
    return pdiutil::string(CHARPTR_WRAP_RO(task->m_name));
}

/**
 * The one line a task reads as, leading with the pid, the bracketed name and
 * then the state.
 */
void taskStatLine(const task_t *task, pdiutil::string &out) {
    out.clear();
    if (nullptr == task) return;

    appendNumber(out, task->m_task_id);
    out += " (";
    out += taskDisplayName(task);
    out += ") ";
    out += taskStateLetter(task->m_state);
    out += " ";
    appendNumber(out, task->m_owner);
    out += " ";
    appendNumber(out, task->m_task_priority);
    out += " ";
    appendNumber(out, task->m_nice);
    out += " ";
    appendNumber(out, (int64_t)task->m_run_count);
    out += " ";
    appendNumber(out, (int64_t)task->m_total_exec_us);
    out += " ";
    appendNumber(out, (int64_t)task->m_created_ms);
    out += " ";
    appendNumber(out, (int64_t)task->m_duration);
    out += " ";
    out += taskPolicyLetter(task->m_task_policy);
    out += " ";
    out += taskModeLetter(task->m_task_mode);
    out += "\n";
}

/**
 * The nth field of such a line, counting the bracketed name as one field.
 */
bool taskStatField(const pdiutil::string &line, uint8_t index, pdiutil::string &out) {
    out.clear();

    pdiutil::string::size_type cursor = 0;
    uint8_t seen = 0;

    while (cursor < line.length()) {

        while (cursor < line.length() &&
               (line[cursor] == ' ' || line[cursor] == '\t' || line[cursor] == '\n')) {
            cursor++;
        }
        if (cursor >= line.length()) break;

        pdiutil::string::size_type start = cursor;
        pdiutil::string::size_type stop = cursor;

        if (line[cursor] == '(') {
            pdiutil::string::size_type close = cursor + 1;
            while (close < line.length() && line[close] != ')') close++;
            if (seen == index) {
                out = line.substr(cursor + 1, close - cursor - 1);
                return true;
            }
            stop = (close < line.length()) ? (close + 1) : close;
        } else {
            while (stop < line.length() && line[stop] != ' ' && line[stop] != '\t' &&
                   line[stop] != '\n') {
                stop++;
            }
            if (seen == index) {
                out = line.substr(start, stop - start);
                return true;
            }
        }

        cursor = stop;
        seen++;
    }

    return false;
}

/**
 * The same field read straight as a number, zero where it is not a number.
 */
int64_t taskStatNumber(const pdiutil::string &line, uint8_t index) {
    pdiutil::string field;
    if (!taskStatField(line, index, field) || field.empty()) return 0;

    bool negative = ('-' == field[0]);
    if (negative || '+' == field[0]) {
        field = field.substr(1, field.length() - 1);
    }
    if (field.empty()) return 0;

    int64_t value = (int64_t)StringToUint64(field.c_str(), (uint8_t)field.length());
    return negative ? -value : value;
}
