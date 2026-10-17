/********************************** ProcFS *************************************
This file is part of the PDI Stack.

This is free software. You can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
Created Date    : 21st July 2026
******************************************************************************/

#include <config/Config.h>

#ifdef ENABLE_PROCFS

#include "ProcFs.h"
#include <interface/pdi.h>
#include <utility/DataTypeConversions.h>
#include <utility/TaskRecord.h>

#ifdef ENABLE_NETWORK_SERVICE
#include <interface/pdi/impl/modules/netif/NetifRegistry.h>
#endif

namespace {

class ProcFsNullStorage : public iStorageInterface {
public:
    int64_t read(uint64_t, void*, uint64_t) override { return PDI_ERR_NOT_SUPPORTED; }
    int64_t write(uint64_t, const void*, uint64_t) override { return PDI_ERR_NOT_SUPPORTED; }
    bool erase(uint64_t, uint64_t) override { return false; }
    uint64_t size() const override { return 0; }
};

ProcFsNullStorage s_proc_null_storage;

// Plain string literals at namespace scope — RODT_ATTR expands to a GCC
// statement-expression via F()/PSTR() on esp8266, which is only legal inside a
// function body. String literals here already live in RODATA/IROM.
const char* const s_proc_files[] = {
    "uptime",
    "version",
    "meminfo",
    "mounts",
    "stat"
};

const uint8_t s_proc_file_count = sizeof(s_proc_files) / sizeof(s_proc_files[0]);

const char* const s_proc_task_files[] = {
    "stat",
    "cmdline",
    "status"
};

const uint8_t s_proc_task_file_count = sizeof(s_proc_task_files) / sizeof(s_proc_task_files[0]);

#ifdef ENABLE_NETWORK_SERVICE
const char* const s_proc_net_files[] = {
    "route",
    "dev",
    "tcp"
};

const uint8_t s_proc_net_file_count = sizeof(s_proc_net_files) / sizeof(s_proc_net_files[0]);
#endif

const uint8_t PROC_COL_KEY = 14;
const uint8_t PROC_COL_STATKEY = 15;
const uint8_t PROC_COL_MOUNT = 10;
const uint8_t PROC_COL_IFACE = 8;
const uint8_t PROC_COL_ADDR = 16;
const uint8_t PROC_COL_COUNT = 12;
const uint8_t PROC_COL_ENDPOINT = 22;

/**
 * Append a decimal number, which every field of a task node is built from.
 */
void appendNumber(pdiutil::string& out, int64_t value) {
    char buf[24];
    Int64ToString(value, buf, sizeof(buf), 0);
    out += buf;
}

/**
 * Append a key padded to the key column, so the values below line up whatever
 * the keys are named.
 */
void appendKey(pdiutil::string& out, const char* key) {
    pdiutil::string text = CHARPTR_WRAP_RO(key);
    text += ":";
    __append_padded(out, text.c_str(), PROC_COL_KEY);
}

/**
 * Append a keyed byte count, the shape every memory line is built from.
 */
void appendMemLine(pdiutil::string& out, const char* key, uint32_t bytes) {
    appendKey(out, key);
    appendNumber(out, (int64_t)bytes);
    out += " B" TERMINAL_NEW_LINE;
}

#ifdef ENABLE_NETWORK_SERVICE
/**
 * The name TCP itself gives a state.
 */
void appendSockState(pdiutil::string& out, net_sock_state_t state) {
    switch (state) {
        case NET_SOCK_LISTEN:      out += CHARPTR_WRAP("LISTEN"); break;
        case NET_SOCK_SYN_SENT:    out += CHARPTR_WRAP("SYN_SENT"); break;
        case NET_SOCK_SYN_RCVD:    out += CHARPTR_WRAP("SYN_RECV"); break;
        case NET_SOCK_ESTABLISHED: out += CHARPTR_WRAP("ESTABLISHED"); break;
        case NET_SOCK_FIN_WAIT_1:  out += CHARPTR_WRAP("FIN_WAIT1"); break;
        case NET_SOCK_FIN_WAIT_2:  out += CHARPTR_WRAP("FIN_WAIT2"); break;
        case NET_SOCK_CLOSE_WAIT:  out += CHARPTR_WRAP("CLOSE_WAIT"); break;
        case NET_SOCK_CLOSING:     out += CHARPTR_WRAP("CLOSING"); break;
        case NET_SOCK_LAST_ACK:    out += CHARPTR_WRAP("LAST_ACK"); break;
        case NET_SOCK_TIME_WAIT:   out += CHARPTR_WRAP("TIME_WAIT"); break;
        default:                   out += CHARPTR_WRAP("CLOSED"); break;
    }
}

/**
 * Append an address and port as one padded column.
 */
void appendEndpoint(pdiutil::string& out, const ipaddress_t& ip, uint16_t port) {
    pdiutil::string text = ip;
    char portbuf[8];

    Int32ToString((int32_t)port, portbuf, sizeof(portbuf), 0);
    text += ":";
    text += portbuf;

    __append_padded(out, text.c_str(), PROC_COL_ENDPOINT);
}

/**
 * One endpoint as a row, appended straight into the file being built.
 */
void appendSocketRow(const net_socket_t& sock, void* arg) {
    if (nullptr == arg) return;

    pdiutil::string& out = *(pdiutil::string*)arg;

    appendEndpoint(out, sock.m_localip, sock.m_localport);
    appendEndpoint(out, sock.m_remoteip, sock.m_remoteport);
    appendSockState(out, sock.m_state);
    out += TERMINAL_NEW_LINE;
}
#endif

}

ProcFs __i_procfs;

ProcFs::ProcFs() : SynthFs(s_proc_null_storage, PROC_MOUNT_PREFIX) {}

/**
 * Which node the path names, and for a task node which task it belongs to.
 */
ProcFs::proc_node_t ProcFs::classify(const char* path, int32_t& taskid_out,
                                     uint8_t& leaf_out) const {
    const char* p = normalizePath(path);
    taskid_out = -1;
    leaf_out = 0;

    if (*p == '\0') return PROC_ROOT;

    for (uint8_t i = 0; i < s_proc_file_count; ++i) {
        const char* cursor = p;
        if (matchSegment(cursor, s_proc_files[i]) && *cursor == '\0') {
            return PROC_TOPFILE;
        }
    }

#ifdef ENABLE_NETWORK_SERVICE
    {
        const char* cursor = p;
        if (matchSegment(cursor, "net")) {
            if (*cursor == '\0') return PROC_NETDIR;
            if (!nextSegment(cursor)) return PROC_INVALID;

            for (uint8_t i = 0; i < s_proc_net_file_count; ++i) {
                const char* leafcursor = cursor;
                if (matchSegment(leafcursor, s_proc_net_files[i]) && *leafcursor == '\0') {
                    leaf_out = i;
                    return PROC_NETFILE;
                }
            }

            return PROC_INVALID;
        }
    }
#endif

    int32_t id = numberSegment(p);
    if (id < 0 || nullptr == __task_scheduler.get_task((pdiutil::task_id_t)id)) {
        return PROC_INVALID;
    }
    taskid_out = id;

    if (*p == '\0') return PROC_TASKDIR;
    if (!nextSegment(p)) return PROC_INVALID;

    for (uint8_t i = 0; i < s_proc_task_file_count; ++i) {
        const char* cursor = p;
        if (matchSegment(cursor, s_proc_task_files[i]) && *cursor == '\0') {
            leaf_out = i;
            return PROC_TASKFILE;
        }
    }

    return PROC_INVALID;
}

SynthFs::synth_node_t ProcFs::resolve(const char* path) {
    int32_t taskid;
    uint8_t leaf;

    switch (classify(path, taskid, leaf)) {
        case PROC_ROOT:
        case PROC_TASKDIR:
        case PROC_NETDIR:
            return SYNTH_DIR;
        case PROC_TOPFILE:
        case PROC_TASKFILE:
        case PROC_NETFILE:
            return SYNTH_FILE;
        default:
            return SYNTH_NONE;
    }
}

pdiutil::string ProcFs::render(const char* path) {
    int32_t taskid;
    uint8_t leaf;
    proc_node_t node = classify(path, taskid, leaf);

    if (PROC_TOPFILE == node) {
        const char* norm = normalizePath(path);
        char buf[128];
        buf[0] = '\0';

        if (strcmp_ro(norm, RODT_ATTR("uptime")) == 0) {
            uint32_t ms = __i_dvc_ctrl.millis_now();
            uint32_t sec = ms / 1000UL;
            uint32_t frac = (ms % 1000UL) / 10;
            pdiutil::string fmt = CHARPTR_WRAP("%u.%02u %u.%02u" TERMINAL_NEW_LINE);
            __snprintf(buf, sizeof(buf), fmt.c_str(), sec, frac, sec, frac);
            return pdiutil::string(buf);
        }

        if (strcmp_ro(norm, RODT_ATTR("meminfo")) == 0) {
            return renderMemInfo();
        }

        if (strcmp_ro(norm, RODT_ATTR("mounts")) == 0) {
            return renderMounts();
        }

        if (strcmp_ro(norm, RODT_ATTR("stat")) == 0) {
            return renderStat();
        }

        // RELEASE / CONFIG_VERSION land in IROM on esp8266; __vsnprintf's %s
        // reads char-by-char with plain *p and faults there. Marshal to RAM
        // first via CHARPTR_WRAP (same trick already used for format strings).
        pdiutil::string fmt = CHARPTR_WRAP("PDI Stack version %s (%s)" TERMINAL_NEW_LINE);
        pdiutil::string rel = CHARPTR_WRAP(RELEASE);
        pdiutil::string cfg = CHARPTR_WRAP(CONFIG_VERSION);
        __snprintf(buf, sizeof(buf), fmt.c_str(), rel.c_str(), cfg.c_str());
        return pdiutil::string(buf);
    }

#ifdef ENABLE_NETWORK_SERVICE
    if (PROC_NETFILE == node) {
        if (0 == leaf) return renderNetRoute();
        return (1 == leaf) ? renderNetDev() : renderNetTcp();
    }
#endif

    if (PROC_TASKFILE != node) return pdiutil::string();

    task_t* task = __task_scheduler.get_task((pdiutil::task_id_t)taskid);
    if (nullptr == task) return pdiutil::string();

    pdiutil::string out;

    if (1 == leaf) {
        out += taskDisplayName(task);
        out += TERMINAL_NEW_LINE;
        return out;
    }

    pdiutil::string line;
    taskStatLine(task, line);

    if (0 == leaf) {
        return line;
    }

    pdiutil::string state, policy, mode;
    taskStatField(line, 2, state);
    taskStatField(line, 10, policy);
    taskStatField(line, 11, mode);

    appendKey(out, CHARPTR_WRAP("Name"));
    out += taskDisplayName(task);
    out += TERMINAL_NEW_LINE;
    appendKey(out, CHARPTR_WRAP("Pid"));
    appendNumber(out, task->m_task_id);
    out += TERMINAL_NEW_LINE;
    appendKey(out, CHARPTR_WRAP("State"));
    out += state;
    out += TERMINAL_NEW_LINE;
    appendKey(out, CHARPTR_WRAP("Owner"));
    appendNumber(out, task->m_owner);
    out += TERMINAL_NEW_LINE;
    appendKey(out, CHARPTR_WRAP("Prio"));
    appendNumber(out, task->m_task_priority);
    out += TERMINAL_NEW_LINE;
    appendKey(out, CHARPTR_WRAP("Nice"));
    appendNumber(out, task->m_nice);
    out += TERMINAL_NEW_LINE;
    appendKey(out, CHARPTR_WRAP("Policy"));
    out += policy;
    out += TERMINAL_NEW_LINE;
    appendKey(out, CHARPTR_WRAP("Mode"));
    out += mode;
    out += TERMINAL_NEW_LINE;
    appendKey(out, CHARPTR_WRAP("Runs"));
    appendNumber(out, (int64_t)task->m_run_count);
    out += TERMINAL_NEW_LINE;
    appendKey(out, CHARPTR_WRAP("ExecUs"));
    appendNumber(out, (int64_t)task->m_total_exec_us);
    out += TERMINAL_NEW_LINE;
    appendKey(out, CHARPTR_WRAP("IntvlMs"));
    appendNumber(out, (int64_t)task->m_duration);
    out += TERMINAL_NEW_LINE;

    return out;
}

/**
 * What the heap holds, and how much of it is in one piece. The gap between the
 * two is the fragmentation a long running board suffers from.
 */
pdiutil::string ProcFs::renderMemInfo() {
    pdiutil::string out;
    appendMemLine(out, RODT_ATTR("MemFree"), __i_dvc_ctrl.get_free_heap());
    appendMemLine(out, RODT_ATTR("MemMaxBlock"), __i_dvc_ctrl.get_max_free_block());
    appendKey(out, RODT_ATTR("AllocRefused"));
    appendNumber(out, (int64_t)pdiutil::safe_alloc_refusals());
    out += TERMINAL_NEW_LINE;
    return out;
}

/**
 * One line per mount, in the order the dispatcher searches them.
 */
pdiutil::string ProcFs::renderMounts() {
    pdiutil::string out;

    for (uint8_t i = 0; i < __i_fs.getMountCount(); ++i) {
        const vfs_mount_t* mount = __i_fs.getMount(i);
        if (nullptr == mount) continue;

        __append_padded(out, mount->m_name, PROC_COL_MOUNT);
        __append_padded(out, mount->m_prefix, PROC_COL_MOUNT);
        __append_padded(out, CHARPTR_WRAP_RO(VfsTypeToString(mount->m_type)), PROC_COL_MOUNT);
        out += CHARPTR_WRAP("rw 0 0" TERMINAL_NEW_LINE);
    }

    return out;
}

/**
 * Cumulative scheduler counters. The cpu figures are microseconds rather than
 * the jiffies linux counts in, because every reader turns them into a ratio.
 */
pdiutil::string ProcFs::renderStat() {
    uint64_t busy = 0;
    uint16_t running = 0;
    uint32_t switches = 0;
    uint16_t tasks = 0;

    for (uint16_t i = 0; i < __task_scheduler.getTaskSlots(); ++i) {
        task_t* task = __task_scheduler.getTaskByIndex(i);
        if (nullptr == task) continue;
        busy += task->m_total_exec_us;
        switches += task->m_run_count;
        tasks++;
        if (TASK_STATE_RUNNING == task->m_state) running++;
    }

    uint64_t elapsed = (uint64_t)__i_dvc_ctrl.micros_now();
    uint64_t idle = (elapsed > busy) ? (elapsed - busy) : 0;

    pdiutil::string out;

    __append_padded(out, CHARPTR_WRAP("cpu"), PROC_COL_STATKEY);
    out += CHARPTR_WRAP("0 0 ");
    appendNumber(out, (int64_t)busy);
    out += " ";
    appendNumber(out, (int64_t)idle);
    out += TERMINAL_NEW_LINE;

    __append_padded(out, CHARPTR_WRAP("ctxt"), PROC_COL_STATKEY);
    appendNumber(out, (int64_t)switches);
    out += TERMINAL_NEW_LINE;

    __append_padded(out, CHARPTR_WRAP("processes"), PROC_COL_STATKEY);
    appendNumber(out, (int64_t)tasks);
    out += TERMINAL_NEW_LINE;

    __append_padded(out, CHARPTR_WRAP("procs_running"), PROC_COL_STATKEY);
    appendNumber(out, (int64_t)running);
    out += TERMINAL_NEW_LINE;

    __append_padded(out, CHARPTR_WRAP("btime"), PROC_COL_STATKEY);
    out += CHARPTR_WRAP("0" TERMINAL_NEW_LINE);

    return out;
}

#ifdef ENABLE_NETWORK_SERVICE
/**
 * The routes the registered interfaces provide, each network they reach
 * directly and the default route wherever a gateway answers for one.
 */
pdiutil::string ProcFs::renderNetRoute() {
    pdiutil::string out;

    __append_padded(out, CHARPTR_WRAP("Iface"), PROC_COL_IFACE);
    __append_padded(out, CHARPTR_WRAP("Destination"), PROC_COL_ADDR);
    __append_padded(out, CHARPTR_WRAP("Gateway"), PROC_COL_ADDR);
    __append_padded(out, "Mask", PROC_COL_ADDR);
    out += CHARPTR_WRAP("Flags" TERMINAL_NEW_LINE);

    pdiutil::string anyaddr = CHARPTR_WRAP("0.0.0.0");

    for (uint8_t i = 0; i < __netif_registry.count(); ++i) {
        iNetifInterface* netif = __netif_registry.at(i);
        if (nullptr == netif) continue;

        netif_info_t info;
        if (!netif->getInfo(info) || !info.m_up) continue;

        if (IP4_ADDRESS_ANY != (uint32_t)info.m_netmask) {
            ipaddress_t network((uint32_t)info.m_ip & (uint32_t)info.m_netmask);

            __append_padded(out, netif->name(), PROC_COL_IFACE);
            __append_padded(out, pdiutil::string(network).c_str(), PROC_COL_ADDR);
            __append_padded(out, anyaddr.c_str(), PROC_COL_ADDR);
            __append_padded(out, pdiutil::string(info.m_netmask).c_str(), PROC_COL_ADDR);
            out += "U";
            out += TERMINAL_NEW_LINE;
        }

        if (info.m_gateway.isSet()) {
            __append_padded(out, netif->name(), PROC_COL_IFACE);
            __append_padded(out, anyaddr.c_str(), PROC_COL_ADDR);
            __append_padded(out, pdiutil::string(info.m_gateway).c_str(), PROC_COL_ADDR);
            __append_padded(out, anyaddr.c_str(), PROC_COL_ADDR);
            out += "UG";
            out += TERMINAL_NEW_LINE;
        }
    }

    return out;
}

/**
 * Per interface traffic, for the interfaces that can count it.
 */
pdiutil::string ProcFs::renderNetDev() {
    pdiutil::string out;

    __append_padded(out, CHARPTR_WRAP("Iface"), PROC_COL_IFACE);
    __append_padded(out, CHARPTR_WRAP("RxBytes"), PROC_COL_COUNT);
    __append_padded(out, CHARPTR_WRAP("RxPackets"), PROC_COL_COUNT);
    __append_padded(out, CHARPTR_WRAP("RxErrs"), PROC_COL_COUNT);
    __append_padded(out, CHARPTR_WRAP("TxBytes"), PROC_COL_COUNT);
    __append_padded(out, CHARPTR_WRAP("TxPackets"), PROC_COL_COUNT);
    out += CHARPTR_WRAP("TxErrs" TERMINAL_NEW_LINE);

    for (uint8_t i = 0; i < __netif_registry.count(); ++i) {
        iNetifInterface* netif = __netif_registry.at(i);
        if (nullptr == netif) continue;

        netif_counters_t counters;
        if (!netif->getCounters(counters)) continue;

        __append_padded(out, netif->name(), PROC_COL_IFACE);
        __append_padded_num(out, (int64_t)counters.m_rx_bytes, PROC_COL_COUNT);
        __append_padded_num(out, (int64_t)counters.m_rx_packets, PROC_COL_COUNT);
        __append_padded_num(out, (int64_t)counters.m_rx_errors, PROC_COL_COUNT);
        __append_padded_num(out, (int64_t)counters.m_tx_bytes, PROC_COL_COUNT);
        __append_padded_num(out, (int64_t)counters.m_tx_packets, PROC_COL_COUNT);
        appendNumber(out, (int64_t)counters.m_tx_errors);
        out += TERMINAL_NEW_LINE;
    }

    return out;
}

/**
 * Every TCP endpoint the stack holds, when the port can enumerate them.
 */
pdiutil::string ProcFs::renderNetTcp() {
    pdiutil::string out;

    __append_padded(out, CHARPTR_WRAP("Local"), PROC_COL_ENDPOINT);
    __append_padded(out, CHARPTR_WRAP("Remote"), PROC_COL_ENDPOINT);
    out += CHARPTR_WRAP("State" TERMINAL_NEW_LINE);

    iNetStackInterface* stack = __netif_registry.stack();
    if (nullptr != stack) {
        stack->eachTcpSocket(appendSocketRow, &out);
    }

    return out;
}
#endif

int ProcFs::listChildren(const char* path, pdiutil::vector<file_info_t>& items) {
    int32_t taskid;
    uint8_t leaf;
    proc_node_t node = classify(path, taskid, leaf);

    if (PROC_TASKDIR == node) {
        for (uint8_t i = 0; i < s_proc_task_file_count; ++i) {
            addEntry(items, s_proc_task_files[i], FILE_TYPE_REG, 0444);
        }
        return (int)items.size();
    }

#ifdef ENABLE_NETWORK_SERVICE
    if (PROC_NETDIR == node) {
        for (uint8_t i = 0; i < s_proc_net_file_count; ++i) {
            addEntry(items, s_proc_net_files[i], FILE_TYPE_REG, 0444);
        }
        return (int)items.size();
    }
#endif

    if (PROC_ROOT != node) return STORAGE_ERROR_NOT_A_DIRECTORY;

    for (uint8_t i = 0; i < s_proc_file_count; ++i) {
        addEntry(items, s_proc_files[i], FILE_TYPE_REG, 0444,
                 getFileSize(s_proc_files[i]));
    }

#ifdef ENABLE_NETWORK_SERVICE
    addEntry(items, "net", FILE_TYPE_DIR, 0555);
#endif

    char numbuf[12];
    for (uint16_t i = 0; i < __task_scheduler.getTaskSlots(); ++i) {
        task_t* task = __task_scheduler.getTaskByIndex(i);
        if (nullptr == task) continue;
        Uint32ToString((uint32_t)task->m_task_id, numbuf, sizeof(numbuf));
        addEntry(items, numbuf, FILE_TYPE_DIR, 0555);
    }

    return (int)items.size();
}

#endif
