/******************************** VFS Tests ***********************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Reaches the storage stack the way the shell does — through the dispatcher, with
every backend mounted where PdiStack mounts it — so what is under test is the
routing and the mount set, not one filesystem in isolation.

Author          : Suraj I.
created Date    : 16th Aug 2026
******************************************************************************/

#include <interface/pdi.h>
#include <interface/pdi/impl/modules/storage/DevFs.h>
#include <interface/pdi/impl/modules/storage/ProcFs.h>
#include <interface/pdi/impl/modules/storage/SysFs.h>
#include <interface/pdi/impl/modules/storage/TmpFs.h>
#include <helpers/ConfigHelper.h>
#include <service_provider/session/SessionManager.h>
#include <MountedStack.h>
#include <ShellHarness.h>
#include <utility/TaskScheduler.h>
#include <interface/pdi/impl/modules/netif/NetifRegistry.h>
#include <pditest.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

static VfsDispatcher *mountedVfs()
{
    return pditest::mountedVfs();
}

/**
 * Collect a whole file into a string through the read callback.
 */
static pdiutil::string slurp(VfsDispatcher *fs, const char *path)
{
    pdiutil::string out;
    fs->readFile(path, 64, [&out](char *chunk, uint32_t len) {
        for (uint32_t i = 0; i < len; i++)
        {
            out += chunk[i];
        }
        return true;
    });
    return out;
}

static void removeIfPresent(VfsDispatcher *fs, const char *path)
{
    if (fs->isFileExist(path))
    {
        fs->deleteFile(path);
    }
}

/**
 * A session the access checks will actually consult. With no session at all
 * getCurrentUid answers 0, which is root and waves everything through.
 */
struct ScopedSession
{
    session_t m_session;
    session_t *m_previous;

    ScopedSession(uint16_t uid, uint16_t gid) : m_previous(SessionManager::current())
    {
        m_session.m_sid = 99;
        m_session.m_state = SESSION_STATE_INTERACTIVE;
        m_session.m_uid = uid;
        m_session.m_gid = gid;
        SessionManager::setCurrent(&m_session);
    }

    ~ScopedSession()
    {
        SessionManager::setCurrent(m_previous);
    }
};

/* ------------------------------------------------------------------ mounts */

TEST(vfs, every_backend_is_mounted)
{
    VfsDispatcher *fs = mountedVfs();
    ASSERT_EQ(fs->getMountCount(), 5);
}

TEST(vfs, a_mount_reports_its_name_and_type)
{
    VfsDispatcher *fs = mountedVfs();
    const vfs_mount_t *root = fs->getMount(0);

    ASSERT_NOT_NULL(root);
    ASSERT_STREQ(root->m_name, "rootfs");
    ASSERT_EQ(root->m_type, VFS_TYPE_LITTLEFS);
}

TEST(vfs, a_mount_index_past_the_end_is_empty)
{
    VfsDispatcher *fs = mountedVfs();
    ASSERT_NULL(fs->getMount(fs->getMountCount()));
}

namespace
{

/**
 * A tmpfs that says it holds a file open, the way a disk filesystem in use does.
 */
class BusyTmpFs : public TmpFs
{
public:
    uint16_t openCount() const override { return 1; }
};

/**
 * A tmpfs that hands out a file handle and counts the reads through it.
 */
class HandleTmpFs : public TmpFs
{
public:
    int m_reads = 0;
    pdi_fhandle_t openFile(const char *path, uint8_t flags) override { return 7; }
    int readFileHandle(pdi_fhandle_t handle, char *buffer, uint32_t size) override
    {
        m_reads++;
        return 0;
    }
};

/**
 * A tmpfs that hands out a handle too big to carry its mount, and one that
 * counts what the table tells it.
 */
class HugeHandleTmpFs : public TmpFs
{
public:
    int m_closes = 0;
    int m_unmounts = 0;
    pdi_fhandle_t openFile(const char *path, uint8_t flags) override { return VFS_HANDLE_BACKEND_MASK + 1; }
    pdi_err_t closeFile(pdi_fhandle_t handle) override
    {
        m_closes++;
        return PDI_OK;
    }
    void unmounted() override { m_unmounts++; }
};

/**
 * A table with a tmpfs at the root and another mounted at runtime at /mnt/x.
 */
struct RuntimeTable
{
    VfsDispatcher m_table;
    TmpFs m_root;
    TmpFs m_extra;

    RuntimeTable()
    {
        m_table.mount("/", &m_root, "rootfs", VFS_TYPE_TMPFS);
        m_table.mount("/mnt/x", &m_extra, "extra", VFS_TYPE_TMPFS, true);
    }
};

} // namespace

TEST(vfs, an_open_is_refused_a_missing_path_no_mount_a_handle_too_big_and_no_access)
{
    VfsDispatcher table;
    HugeHandleTmpFs huge;
    table.mount("/mnt/x", &huge, "huge", VFS_TYPE_TMPFS, true);

    ASSERT_EQ((int)table.openFile(nullptr, FILE_OPEN_READ), (int)STORAGE_ERROR_BAD_PATH);
    ASSERT_EQ((int)table.openFile("", FILE_OPEN_READ), (int)STORAGE_ERROR_BAD_PATH);
    ASSERT_EQ((int)table.openFile("/elsewhere.txt", FILE_OPEN_READ), (int)STORAGE_ERROR_NOT_MOUNTED);
    ASSERT_EQ((int)table.openFile("/mnt/x/a.txt", FILE_OPEN_READ), (int)PDI_ERR_RANGE);
    ASSERT_EQ(huge.m_closes, 1);

    TmpFs root;
    table.mount("/", &root, "rootfs", VFS_TYPE_TMPFS);
    table.beginPrivileged();
    ASSERT_TRUE(table.createFile("/secret.txt", "s") >= 0);
    ASSERT_EQ(table.setFilePermissions("/secret.txt", 0600), 0);
    table.endPrivileged();
    ScopedSession stranger(7, 7);
    ASSERT_EQ((int)table.openFile("/secret.txt", FILE_OPEN_READ), (int)PDI_ERR_PERM);
    ASSERT_EQ((int)table.openFile("/secret.txt", FILE_OPEN_WRITE), (int)PDI_ERR_PERM);
}

TEST(vfs, a_backend_is_told_once_when_its_mount_is_gone_and_not_when_umount_is_refused)
{
    VfsDispatcher table;
    HugeHandleTmpFs backend;
    TmpFs fixed;
    table.mount("/", &fixed, "rootfs", VFS_TYPE_TMPFS);
    table.mount("/mnt/x", &backend, "told", VFS_TYPE_TMPFS, true);

    ASSERT_EQ((int)table.umount("/"), (int)PDI_ERR_PERM);
    ASSERT_EQ((int)table.umount("/mnt/x"), (int)PDI_OK);
    ASSERT_EQ(backend.m_unmounts, 1);
    ASSERT_EQ((int)table.umount("/mnt/x"), (int)PDI_ERR_NOT_FOUND);
    ASSERT_EQ(backend.m_unmounts, 1);
}

TEST(vfs, a_runtime_mount_is_reached_until_it_is_unmounted)
{
    RuntimeTable t;
    t.m_table.beginPrivileged();
    ASSERT_TRUE(t.m_table.createFile("/mnt/x/a.txt", "hi") >= 0);
    t.m_table.endPrivileged();
    ASSERT_TRUE(t.m_extra.isFileExist("/a.txt"));

    ASSERT_EQ((int)t.m_table.umount("/mnt/x"), (int)PDI_OK);

    ASSERT_NULL(t.m_table.getMount(1));
    ASSERT_STREQ(t.m_table.findMountForPath("/mnt/x/a.txt")->m_prefix, "/");
}

TEST(vfs, the_slot_an_unmount_empties_is_taken_by_the_next_mount)
{
    RuntimeTable t;
    TmpFs other;
    ASSERT_EQ((int)t.m_table.umount("/mnt/x"), (int)PDI_OK);

    ASSERT_EQ(t.m_table.mount("/mnt/y", &other, "other", VFS_TYPE_TMPFS, true), 0);

    ASSERT_EQ(t.m_table.getMountCount(), 2);
    ASSERT_STREQ(t.m_table.getMount(1)->m_prefix, "/mnt/y");
}

TEST(vfs, a_mount_made_at_boot_cannot_be_unmounted)
{
    RuntimeTable t;

    ASSERT_EQ((int)t.m_table.umount("/"), (int)PDI_ERR_PERM);
    ASSERT_NOT_NULL(t.m_table.getMount(0));
}

TEST(vfs, unmounting_a_prefix_nothing_is_mounted_at_is_not_found)
{
    RuntimeTable t;

    ASSERT_EQ((int)t.m_table.umount("/mnt/z"), (int)PDI_ERR_NOT_FOUND);
    ASSERT_EQ((int)t.m_table.umount("/mnt"), (int)PDI_ERR_NOT_FOUND);
}

TEST(vfs, a_filesystem_holding_open_files_is_busy)
{
    VfsDispatcher table;
    TmpFs root;
    BusyTmpFs busy;
    table.mount("/", &root, "rootfs", VFS_TYPE_TMPFS);
    table.mount("/mnt/x", &busy, "busy", VFS_TYPE_TMPFS, true);

    ASSERT_EQ((int)table.umount("/mnt/x"), (int)PDI_ERR_BUSY);
    ASSERT_NOT_NULL(table.getMount(1));
}

TEST(vfs, a_filesystem_a_session_is_working_inside_is_busy)
{
    RuntimeTable t;
    pditest::Shell sh;
    session_t *session = nullptr;
    for (uint8_t i = 0; i < SessionManager::maxSessions() && nullptr == session; i++)
    {
        session_t *s = SessionManager::getByIndex(i);
        if (nullptr != s && SESSION_STATE_FREE != s->m_state)
        {
            session = s;
        }
    }
    ASSERT_NOT_NULL(session);
    pdiutil::string saved = session->m_cwd;
    session->m_cwd = "/mnt/x/sub";

    pdi_err_t busy = t.m_table.umount("/mnt/x");
    session->m_cwd = "/mnt/xy";
    pdi_err_t freed = t.m_table.umount("/mnt/x");
    session->m_cwd = saved;

    ASSERT_EQ((int)busy, (int)PDI_ERR_BUSY);
    ASSERT_EQ((int)freed, (int)PDI_OK);
}

TEST(vfs, a_handle_into_an_unmounted_filesystem_is_refused)
{
    VfsDispatcher table;
    TmpFs root;
    HandleTmpFs extra;
    table.mount("/", &root, "rootfs", VFS_TYPE_TMPFS);
    table.mount("/mnt/x", &extra, "extra", VFS_TYPE_TMPFS, true);
    table.beginPrivileged();
    pdi_fhandle_t handle = table.openFile("/mnt/x/a.txt", FILE_OPEN_READ);
    table.endPrivileged();
    ASSERT_TRUE(handle >= 0);
    char buf[4];
    table.readFileHandle(handle, buf, sizeof(buf));
    ASSERT_EQ(extra.m_reads, 1);

    ASSERT_EQ((int)table.umount("/mnt/x"), (int)PDI_OK);

    ASSERT_EQ(table.readFileHandle(handle, buf, sizeof(buf)), (int)PDI_ERR_INVALID_ARG);
    ASSERT_EQ(extra.m_reads, 1);
}

TEST(vfs, umount_says_why_it_would_not)
{
    pditest::Shell sh;

    std::string boot = sh.run("umount /");
    std::string none = sh.run("umount /nowhere");

    ASSERT_TRUE(pditest::saw(boot, "umount: /"));
    ASSERT_TRUE(pditest::saw(boot, "-12"));
    ASSERT_TRUE(pditest::saw(none, "-6"));
    ASSERT_NOT_NULL(__i_fs.getMount(0));
}

TEST(vfs, mounting_the_same_prefix_twice_is_refused)
{
    VfsDispatcher table;
    ASSERT_EQ(table.mount("/proc", &__i_procfs, "procfs", VFS_TYPE_PROCFS), 0);
    ASSERT_EQ(table.mount("/proc", &__i_procfs, "again", VFS_TYPE_PROCFS), PDI_ERR_EXISTS);
    ASSERT_EQ(table.getMountCount(), 1);
}

TEST(vfs, a_mount_past_the_table_limit_is_refused)
{
    VfsDispatcher table;
    const char *prefixes[] = {"/a", "/b", "/c", "/d", "/e", "/f", "/g", "/h"};

    for (uint8_t i = 0; i < VFS_MAX_MOUNTS; i++)
    {
        ASSERT_EQ(table.mount(prefixes[i], &__i_tmpfs, "tmpfs", VFS_TYPE_TMPFS), 0);
    }

    ASSERT_EQ(table.getMountCount(), VFS_MAX_MOUNTS);
    ASSERT_EQ(table.mount("/z", &__i_tmpfs, "extra", VFS_TYPE_TMPFS), PDI_ERR_NO_SPACE);
}

TEST(vfs, a_mount_needs_a_prefix_and_a_backend)
{
    VfsDispatcher table;

    ASSERT_EQ(table.mount(nullptr, &__i_tmpfs, "x", VFS_TYPE_TMPFS), PDI_ERR_INVALID_ARG);
    ASSERT_EQ(table.mount("/x", nullptr, "x", VFS_TYPE_TMPFS), PDI_ERR_INVALID_ARG);
    ASSERT_EQ(table.mount("", &__i_tmpfs, "x", VFS_TYPE_TMPFS), PDI_ERR_INVALID_ARG);
    ASSERT_EQ(table.getMountCount(), 0);
}

TEST(vfs, a_prefix_longer_than_the_table_allows_is_refused)
{
    VfsDispatcher table;
    char toolong[VFS_MOUNT_PREFIX_MAX + 4];
    memset(toolong, 'a', sizeof(toolong));
    toolong[0] = '/';
    toolong[sizeof(toolong) - 1] = '\0';

    ASSERT_EQ(table.mount(toolong, &__i_tmpfs, "x", VFS_TYPE_TMPFS), PDI_ERR_INVALID_ARG);
}

TEST(vfs, a_path_with_no_matching_mount_resolves_to_nothing)
{
    VfsDispatcher table;
    ASSERT_EQ(table.mount("/tmp", &__i_tmpfs, "tmpfs", VFS_TYPE_TMPFS), 0);

    ASSERT_NOT_NULL(table.findMountForPath("/tmp/x"));
    ASSERT_NULL(table.findMountForPath("/var/x"));
}

/* ----------------------------------------------------------------- routing */

TEST(vfs, an_ordinary_path_routes_to_the_root_filesystem)
{
    VfsDispatcher *fs = mountedVfs();
    const vfs_mount_t *m = fs->findMountForPath("/var/log");

    ASSERT_NOT_NULL(m);
    ASSERT_STREQ(m->m_name, "rootfs");
}

TEST(vfs, the_longest_matching_prefix_wins)
{
    VfsDispatcher *fs = mountedVfs();
    const vfs_mount_t *m = fs->findMountForPath("/proc/uptime");

    ASSERT_NOT_NULL(m);
    ASSERT_STREQ(m->m_name, "procfs");
}

TEST(vfs, a_mount_point_itself_routes_to_its_backend)
{
    VfsDispatcher *fs = mountedVfs();
    const vfs_mount_t *m = fs->findMountForPath("/dev");

    ASSERT_NOT_NULL(m);
    ASSERT_STREQ(m->m_name, "devfs");
}

TEST(vfs, a_prefix_only_matches_a_whole_path_segment)
{
    VfsDispatcher *fs = mountedVfs();
    const vfs_mount_t *m = fs->findMountForPath("/procession/notes");

    ASSERT_NOT_NULL(m);
    ASSERT_STREQ(m->m_name, "rootfs");
}

TEST(vfs, each_mount_answers_for_its_own_tree)
{
    VfsDispatcher *fs = mountedVfs();

    ASSERT_STREQ(fs->findMountForPath("/sys/class/gpio")->m_name, "sysfs");
    ASSERT_STREQ(fs->findMountForPath("/tmp/scratch")->m_name, "tmpfs");
    ASSERT_STREQ(fs->findMountForPath("/dev/null")->m_name, "devfs");
}

TEST(vfs, size_reporting_comes_from_the_root_filesystem)
{
    VfsDispatcher *fs = mountedVfs();

    ASSERT_EQ(fs->getTotalSize(), __i_rootfs.getTotalSize());
    ASSERT_TRUE(fs->getFreeSize() <= fs->getTotalSize());
}

TEST(vfs, a_file_created_through_the_dispatcher_lands_on_the_root_filesystem)
{
    VfsDispatcher *fs = mountedVfs();
    removeIfPresent(fs, "/routed.txt");

    ASSERT_GE(fs->createFile("/routed.txt", "routed"), 0);
    ASSERT_TRUE(__i_rootfs.isFileExist("/routed.txt"));
    ASSERT_STREQ(slurp(fs, "/routed.txt").c_str(), "routed");

    fs->deleteFile("/routed.txt");
}

/* ------------------------------------------------------------------ procfs */

TEST(procfs, uptime_exists_and_reads)
{
    VfsDispatcher *fs = mountedVfs();

    ASSERT_TRUE(fs->isFileExist("/proc/uptime"));
    ASSERT_TRUE(slurp(fs, "/proc/uptime").length() > 0);
}

TEST(procfs, version_names_the_release)
{
    VfsDispatcher *fs = mountedVfs();
    pdiutil::string version = slurp(fs, "/proc/version");

    ASSERT_TRUE(version.find("PDI Stack version") != pdiutil::string::npos);
    ASSERT_TRUE(version.find(RELEASE) != pdiutil::string::npos);
}

TEST(procfs, uptime_advances_with_the_clock)
{
    VfsDispatcher *fs = mountedVfs();
    pdiutil::string first = slurp(fs, "/proc/uptime");
    __i_dvc_ctrl.wait(1100);
    pdiutil::string second = slurp(fs, "/proc/uptime");

    ASSERT_STRNE(first.c_str(), second.c_str());
}

TEST(procfs, a_node_reports_its_size)
{
    VfsDispatcher *fs = mountedVfs();
    int64_t size = fs->getFileSize("/proc/version");

    ASSERT_TRUE(size > 0);
    ASSERT_EQ((uint64_t)size, (uint64_t)slurp(fs, "/proc/version").length());
}

TEST(procfs, an_unknown_node_is_absent)
{
    VfsDispatcher *fs = mountedVfs();
    ASSERT_FALSE(fs->isFileExist("/proc/nosuchnode"));
}

TEST(procfs, a_write_is_refused)
{
    VfsDispatcher *fs = mountedVfs();
    ASSERT_EQ(fs->writeFile("/proc/uptime", "0", 1), PDI_ERR_NOT_SUPPORTED);
}

TEST(procfs, creating_a_node_is_refused)
{
    VfsDispatcher *fs = mountedVfs();
    ASSERT_EQ(fs->createFile("/proc/mine", "x"), PDI_ERR_NOT_SUPPORTED);
    ASSERT_FALSE(fs->isFileExist("/proc/mine"));
}

TEST(procfs, deleting_a_node_is_refused)
{
    VfsDispatcher *fs = mountedVfs();

    ASSERT_NE(fs->deleteFile("/proc/uptime"), (pdi_err_t)0);
    ASSERT_TRUE(fs->isFileExist("/proc/uptime"));
}

TEST(procfs, the_directory_lists_its_nodes)
{
    VfsDispatcher *fs = mountedVfs();
    pdiutil::vector<file_info_t> items;

    ASSERT_TRUE(fs->getDirFileList("/proc", items) >= 0);

    bool sawuptime = false;
    bool sawversion = false;
    for (file_info_t &item : items)
    {
        if (nullptr != item.m_name && 0 == strcmp(item.m_name, "uptime")) sawuptime = true;
        if (nullptr != item.m_name && 0 == strcmp(item.m_name, "version")) sawversion = true;
    }
    for (file_info_t &item : items)
    {
        pdiutil::safe_delete_array(item.m_name);
    }

    ASSERT_TRUE(sawuptime);
    ASSERT_TRUE(sawversion);
}

TEST(procfs, meminfo_reports_free_heap_and_the_largest_block)
{
    VfsDispatcher *fs = mountedVfs();
    pdiutil::string meminfo = slurp(fs, "/proc/meminfo");

    ASSERT_TRUE(meminfo.find("MemFree:") != pdiutil::string::npos);
    ASSERT_TRUE(meminfo.find("MemMaxBlock:") != pdiutil::string::npos);
    ASSERT_TRUE(meminfo.find("AllocRefused:") != pdiutil::string::npos);
    ASSERT_TRUE(meminfo.find(" B") != pdiutil::string::npos);
}

TEST(procfs, mounts_names_every_mounted_filesystem)
{
    VfsDispatcher *fs = mountedVfs();
    pdiutil::string mounts = slurp(fs, "/proc/mounts");

    for (uint8_t i = 0; i < fs->getMountCount(); i++)
    {
        const vfs_mount_t *mount = fs->getMount(i);
        ASSERT_TRUE(nullptr != mount);
        ASSERT_TRUE(mounts.find(mount->m_prefix) != pdiutil::string::npos);
        ASSERT_TRUE(mounts.find(mount->m_name) != pdiutil::string::npos);
    }
}

TEST(procfs, mounts_gives_each_line_the_type_the_mount_carries)
{
    VfsDispatcher *fs = mountedVfs();
    pdiutil::string mounts = slurp(fs, "/proc/mounts");

    // the fields sit in fixed width columns, so the assertion walks them in
    // order rather than pinning the padding between them
    size_t name = mounts.find("procfs");
    ASSERT_TRUE(name != pdiutil::string::npos);

    size_t prefix = mounts.find("/proc", name);
    ASSERT_TRUE(prefix != pdiutil::string::npos);

    size_t type = mounts.find("procfs", prefix);
    ASSERT_TRUE(type != pdiutil::string::npos);

    ASSERT_TRUE(mounts.find("rw 0 0", type) != pdiutil::string::npos);
}

TEST(procfs, a_read_stops_at_the_string_it_was_told_to_stop_at)
{
    VfsDispatcher *fs = mountedVfs();

    pdiutil::string first;
    bool matched = false;
    int bytes = fs->readFile("/proc/mounts", 64, [&first](char *chunk, uint32_t len) {
        for (uint32_t i = 0; i < len; i++)
        {
            first += chunk[i];
        }
        return true;
    }, 0, "\n", &matched);

    ASSERT_TRUE(matched);
    ASSERT_TRUE(bytes > 0);
    ASSERT_EQ((uint32_t)bytes, (uint32_t)first.length());
    ASSERT_TRUE(first.find('\n') == pdiutil::string::npos);
}

TEST(procfs, stepping_by_the_match_walks_every_line_once)
{
    VfsDispatcher *fs = mountedVfs();
    pdiutil::string whole = slurp(fs, "/proc/mounts");

    uint16_t lines = 0;
    pdiutil::string rebuilt;
    uint64_t offset = 0;

    while (offset < (uint64_t)whole.length() && lines < 32)
    {
        pdiutil::string line;
        int bytes = fs->readFile("/proc/mounts", 64, [&line](char *chunk, uint32_t len) {
            for (uint32_t i = 0; i < len; i++)
            {
                line += chunk[i];
            }
            return true;
        }, offset, "\n");

        if (bytes < 0) break;
        offset += (uint64_t)bytes + 1;

        rebuilt += line;
        rebuilt += "\n";
        lines++;
    }

    ASSERT_EQ((uint32_t)lines, (uint32_t)fs->getMountCount());
    ASSERT_STREQ(rebuilt.c_str(), whole.c_str());
}

TEST(procfs, a_read_with_no_match_in_it_delivers_the_rest)
{
    VfsDispatcher *fs = mountedVfs();
    pdiutil::string whole = slurp(fs, "/proc/version");

    pdiutil::string out;
    bool matched = false;
    fs->readFile("/proc/version", 64, [&out](char *chunk, uint32_t len) {
        for (uint32_t i = 0; i < len; i++)
        {
            out += chunk[i];
        }
        return true;
    }, 0, "@@nosuchmatch@@", &matched);

    ASSERT_FALSE(matched);
    ASSERT_STREQ(out.c_str(), whole.c_str());
}

TEST(procfs, stat_counts_the_tasks_the_scheduler_holds)
{
    VfsDispatcher *fs = mountedVfs();
    pditest::readyScheduler();

    pdiutil::task_id_t id = __task_scheduler.register_task([]() {}, 1000, DEFAULT_TASK_PRIORITY, 0, -1, "fsprobe");
    ASSERT_TRUE(id >= 0);

    pdiutil::string stat = slurp(fs, "/proc/stat");

    char count[16];
    __snprintf(count, sizeof(count), "%d", (int)__task_scheduler.getTaskCount());

    __task_scheduler.remove_task(id);

    // the keys sit in a fixed width column, so each is found and its value
    // looked for after it rather than pinning the padding between the two
    ASSERT_TRUE(stat.find("cpu") == 0);

    size_t procs = stat.find("processes");
    ASSERT_TRUE(procs != pdiutil::string::npos);
    ASSERT_TRUE(stat.find(count, procs) != pdiutil::string::npos);

    ASSERT_TRUE(stat.find("ctxt") != pdiutil::string::npos);
    ASSERT_TRUE(stat.find("procs_running") != pdiutil::string::npos);
}

TEST(procfs, stat_busy_and_idle_add_up_to_the_uptime)
{
    VfsDispatcher *fs = mountedVfs();
    pdiutil::string stat = slurp(fs, "/proc/stat");

    // "cpu<pad>0 0 <busy> <idle>" - the pair is a partition of the elapsed
    // time, which is what makes a ratio taken from it meaningful
    ASSERT_TRUE(stat.find("cpu") == 0);

    pdiutil::string::size_type at = stat.find("0 0 ");
    ASSERT_TRUE(at != pdiutil::string::npos);

    pdiutil::string rest = stat.substr(at + 4);
    pdiutil::string::size_type gap = rest.find(' ');
    ASSERT_TRUE(gap != pdiutil::string::npos);

    uint64_t busy = StringToUint64(rest.substr(0, gap).c_str());
    uint64_t idle = StringToUint64(rest.substr(gap + 1).c_str());

    ASSERT_TRUE((busy + idle) > 0);
}

TEST(procfs, a_task_gets_a_directory_of_its_own)
{
    VfsDispatcher *fs = mountedVfs();
    pditest::readyScheduler();

    pdiutil::task_id_t id = __task_scheduler.register_task([]() {}, 1000, DEFAULT_TASK_PRIORITY, 0, -1, "fsprobe");
    ASSERT_TRUE(id >= 0);

    char dir[32];
    __snprintf(dir, sizeof(dir), "/proc/%d", (int)id);

    ASSERT_TRUE(fs->isDirExist(dir));
    ASSERT_TRUE(fs->isDirectory(dir));
    ASSERT_FALSE(fs->isFileExist(dir));

    __task_scheduler.remove_task(id);
}

TEST(procfs, a_task_directory_holds_stat_cmdline_and_status)
{
    VfsDispatcher *fs = mountedVfs();
    pditest::readyScheduler();

    pdiutil::task_id_t id = __task_scheduler.register_task([]() {}, 1000, DEFAULT_TASK_PRIORITY, 0, -1, "fsprobe");
    ASSERT_TRUE(id >= 0);

    char dir[32];
    __snprintf(dir, sizeof(dir), "/proc/%d", (int)id);

    pdiutil::vector<file_info_t> items;
    ASSERT_TRUE(fs->getDirFileList(dir, items) >= 0);

    bool sawstat = false;
    bool sawcmdline = false;
    bool sawstatus = false;
    for (file_info_t &item : items)
    {
        if (nullptr == item.m_name) continue;
        if (0 == strcmp(item.m_name, "stat")) sawstat = true;
        if (0 == strcmp(item.m_name, "cmdline")) sawcmdline = true;
        if (0 == strcmp(item.m_name, "status")) sawstatus = true;
    }
    for (file_info_t &item : items)
    {
        pdiutil::safe_delete_array(item.m_name);
    }

    __task_scheduler.remove_task(id);

    ASSERT_TRUE(sawstat);
    ASSERT_TRUE(sawcmdline);
    ASSERT_TRUE(sawstatus);
}

TEST(procfs, stat_leads_with_the_pid_name_and_state)
{
    VfsDispatcher *fs = mountedVfs();
    pditest::readyScheduler();

    pdiutil::task_id_t id = __task_scheduler.register_task([]() {}, 1000, DEFAULT_TASK_PRIORITY, 0, -1, "fsprobe");
    ASSERT_TRUE(id >= 0);

    char path[40];
    __snprintf(path, sizeof(path), "/proc/%d/stat", (int)id);
    pdiutil::string stat = slurp(fs, path);

    char lead[24];
    __snprintf(lead, sizeof(lead), "%d (fsprobe)", (int)id);

    __task_scheduler.remove_task(id);

    ASSERT_TRUE(stat.find(lead) == 0);
}

TEST(procfs, cmdline_names_the_task)
{
    VfsDispatcher *fs = mountedVfs();
    pditest::readyScheduler();

    pdiutil::task_id_t id = __task_scheduler.register_task([]() {}, 1000, DEFAULT_TASK_PRIORITY, 0, -1, "fsprobe");
    ASSERT_TRUE(id >= 0);

    char path[40];
    __snprintf(path, sizeof(path), "/proc/%d/cmdline", (int)id);
    pdiutil::string cmdline = slurp(fs, path);

    __task_scheduler.remove_task(id);

    ASSERT_TRUE(cmdline.find("fsprobe") != pdiutil::string::npos);
}

TEST(procfs, status_reports_the_same_pid_as_the_directory)
{
    VfsDispatcher *fs = mountedVfs();
    pditest::readyScheduler();

    pdiutil::task_id_t id = __task_scheduler.register_task([]() {}, 1000, DEFAULT_TASK_PRIORITY, 0, -1, "fsprobe");
    ASSERT_TRUE(id >= 0);

    char path[40];
    __snprintf(path, sizeof(path), "/proc/%d/status", (int)id);
    pdiutil::string status = slurp(fs, path);

    char pidval[16];
    __snprintf(pidval, sizeof(pidval), "%d", (int)id);

    __task_scheduler.remove_task(id);

    // the keys sit in a fixed width column, so the assertions name the key and
    // its value rather than pinning the padding between them
    ASSERT_TRUE(status.find("Name:") == 0);
    ASSERT_TRUE(status.find("fsprobe") != pdiutil::string::npos);

    size_t pidkey = status.find("Pid:");
    ASSERT_TRUE(pidkey != pdiutil::string::npos);
    ASSERT_TRUE(status.find(pidval, pidkey) != pdiutil::string::npos);
}

TEST(procfs, the_root_lists_a_directory_per_running_task)
{
    VfsDispatcher *fs = mountedVfs();
    pditest::readyScheduler();

    pdiutil::task_id_t id = __task_scheduler.register_task([]() {}, 1000, DEFAULT_TASK_PRIORITY, 0, -1, "fsprobe");
    ASSERT_TRUE(id >= 0);

    char name[16];
    __snprintf(name, sizeof(name), "%d", (int)id);

    pdiutil::vector<file_info_t> items;
    ASSERT_TRUE(fs->getDirFileList("/proc", items) >= 0);

    bool sawtask = false;
    for (file_info_t &item : items)
    {
        if (nullptr != item.m_name && 0 == strcmp(item.m_name, name))
        {
            sawtask = (item.m_type == FILE_TYPE_DIR);
        }
    }
    for (file_info_t &item : items)
    {
        pdiutil::safe_delete_array(item.m_name);
    }

    __task_scheduler.remove_task(id);

    ASSERT_TRUE(sawtask);
}

TEST(procfs, a_removed_task_keeps_its_directory_until_it_is_reaped)
{
    VfsDispatcher *fs = mountedVfs();
    pditest::readyScheduler();

    pdiutil::task_id_t id = __task_scheduler.register_task([]() {}, 1000, DEFAULT_TASK_PRIORITY, 0, -1, "fsprobe");
    ASSERT_TRUE(id >= 0);

    char dir[32];
    char statpath[40];
    __snprintf(dir, sizeof(dir), "/proc/%d", (int)id);
    __snprintf(statpath, sizeof(statpath), "/proc/%d/stat", (int)id);
    ASSERT_TRUE(fs->isDirExist(dir));

    // removing marks the task a zombie rather than freeing its slot, and a
    // zombie is still something ps and /proc can be asked about
    __task_scheduler.remove_task(id);

    ASSERT_TRUE(fs->isDirExist(dir));
    pdiutil::string stat = slurp(fs, statpath);
    ASSERT_TRUE(stat.find(") Z ") != pdiutil::string::npos);

    __task_scheduler.remove_expired_tasks();

    ASSERT_FALSE(fs->isDirExist(dir));
    ASSERT_FALSE(fs->isFileExist(statpath));
}

TEST(procfs, an_unknown_leaf_under_a_task_is_absent)
{
    VfsDispatcher *fs = mountedVfs();
    pditest::readyScheduler();

    pdiutil::task_id_t id = __task_scheduler.register_task([]() {}, 1000, DEFAULT_TASK_PRIORITY, 0, -1, "fsprobe");
    ASSERT_TRUE(id >= 0);

    char path[40];
    __snprintf(path, sizeof(path), "/proc/%d/bogus", (int)id);

    bool exists = fs->isFileExist(path);
    __task_scheduler.remove_task(id);

    ASSERT_FALSE(exists);
}

TEST(procfs, a_pid_that_never_ran_is_absent)
{
    VfsDispatcher *fs = mountedVfs();

    ASSERT_FALSE(fs->isDirExist("/proc/60000"));
    ASSERT_FALSE(fs->isFileExist("/proc/60000/stat"));
    ASSERT_EQ(fs->getFileSize("/proc/60000/stat"), (int64_t)PDI_ERR_NOT_FOUND);
}

TEST(procfs, the_net_directory_holds_route_dev_and_tcp)
{
    VfsDispatcher *fs = mountedVfs();
    pditest::readyNetifs();

    ASSERT_TRUE(fs->isDirExist("/proc/net"));
    ASSERT_TRUE(fs->isFileExist("/proc/net/route"));
    ASSERT_TRUE(fs->isFileExist("/proc/net/dev"));
    ASSERT_TRUE(fs->isFileExist("/proc/net/tcp"));
    ASSERT_FALSE(fs->isFileExist("/proc/net/bogus"));
}

TEST(procfs, route_names_every_interface_that_has_somewhere_to_route)
{
    VfsDispatcher *fs = mountedVfs();
    pditest::readyNetifs();
    pdiutil::string route = slurp(fs, "/proc/net/route");

    ASSERT_TRUE(route.find("Iface") == 0);

    for (uint8_t i = 0; i < __netif_registry.count(); i++)
    {
        iNetifInterface *netif = __netif_registry.at(i);
        ASSERT_TRUE(nullptr != netif);

        netif_info_t info;
        if (!netif->getInfo(info) || !info.m_up) continue;

        // an interface can be up before it holds an address, and one with
        // neither a network nor a gateway has no route to report
        bool routable = (IP4_ADDRESS_ANY != (uint32_t)info.m_netmask) || info.m_gateway.isSet();
        bool named = (route.find(netif->name()) != pdiutil::string::npos);

        ASSERT_EQ(named, routable);
    }
}

TEST(procfs, route_carries_the_network_each_interface_reaches_directly)
{
    VfsDispatcher *fs = mountedVfs();
    pditest::readyNetifs();
    pdiutil::string route = slurp(fs, "/proc/net/route");

    ASSERT_TRUE(route.find("Flags") != pdiutil::string::npos);

    for (uint8_t i = 0; i < __netif_registry.count(); i++)
    {
        iNetifInterface *netif = __netif_registry.at(i);
        ASSERT_TRUE(nullptr != netif);

        netif_info_t info;
        if (!netif->getInfo(info) || !info.m_up) continue;
        if (IP4_ADDRESS_ANY == (uint32_t)info.m_netmask) continue;

        ipaddress_t network((uint32_t)info.m_ip & (uint32_t)info.m_netmask);
        ASSERT_TRUE(route.find(pdiutil::string(network).c_str()) != pdiutil::string::npos);
        ASSERT_TRUE(route.find(pdiutil::string(info.m_netmask).c_str()) != pdiutil::string::npos);
    }
}

TEST(procfs, route_gives_a_default_route_only_to_an_interface_with_a_gateway)
{
    VfsDispatcher *fs = mountedVfs();
    pditest::readyNetifs();
    pdiutil::string route = slurp(fs, "/proc/net/route");

    uint8_t defaults = 0;
    uint8_t gateways = 0;

    const char *scan = route.c_str();
    for (uint32_t i = 0; '\0' != scan[i] && '\0' != scan[i + 1]; i++)
    {
        if ('U' == scan[i] && 'G' == scan[i + 1]) defaults++;
    }

    for (uint8_t i = 0; i < __netif_registry.count(); i++)
    {
        iNetifInterface *netif = __netif_registry.at(i);
        ASSERT_TRUE(nullptr != netif);

        netif_info_t info;
        if (!netif->getInfo(info) || !info.m_up) continue;
        if (info.m_gateway.isSet()) gateways++;
    }

    ASSERT_EQ(defaults, gateways);
}

TEST(procfs, dev_lists_only_interfaces_that_can_count)
{
    VfsDispatcher *fs = mountedVfs();
    pditest::readyNetifs();
    pdiutil::string dev = slurp(fs, "/proc/net/dev");

    ASSERT_TRUE(dev.find("Iface") == 0);

    // an interface with no counters is left out rather than reported as idle
    for (uint8_t i = 0; i < __netif_registry.count(); i++)
    {
        iNetifInterface *netif = __netif_registry.at(i);
        ASSERT_TRUE(nullptr != netif);

        netif_counters_t counters;
        if (netif->getCounters(counters)) continue;

        ASSERT_TRUE(dev.find(netif->name()) == pdiutil::string::npos);
    }
}

TEST(procfs, tcp_lists_a_socket_the_process_is_listening_on)
{
    VfsDispatcher *fs = mountedVfs();
    pditest::readyNetifs();

    int listener = socket(AF_INET, SOCK_STREAM, 0);
    ASSERT_TRUE(listener >= 0);

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addr.sin_port = 0;

    ASSERT_EQ(bind(listener, (struct sockaddr *)&addr, sizeof(addr)), 0);
    ASSERT_EQ(listen(listener, 1), 0);

    socklen_t addrlen = sizeof(addr);
    ASSERT_EQ(getsockname(listener, (struct sockaddr *)&addr, &addrlen), 0);

    char portbuf[8];
    Int32ToString((int32_t)ntohs(addr.sin_port), portbuf, sizeof(portbuf), 0);

    pdiutil::string tcp = slurp(fs, "/proc/net/tcp");

    ASSERT_TRUE(tcp.find("Local") == 0);
    ASSERT_TRUE(tcp.find("127.0.0.1:") != pdiutil::string::npos);
    ASSERT_TRUE(tcp.find(portbuf) != pdiutil::string::npos);
    ASSERT_TRUE(tcp.find("LISTEN") != pdiutil::string::npos);

    close(listener);
}

TEST(procfs, tcp_is_a_header_and_nothing_else_when_the_port_cannot_answer)
{
    VfsDispatcher *fs = mountedVfs();
    pditest::readyNetifs();

    iNetStackInterface *had = __netif_registry.stack();
    __netif_registry.registerStack(nullptr);

    pdiutil::string tcp = slurp(fs, "/proc/net/tcp");

    // the columns still name themselves, so the reader can tell a port that
    // cannot enumerate from a device with nothing listening
    ASSERT_TRUE(tcp.find("Local") == 0);
    ASSERT_TRUE(tcp.find("LISTEN") == pdiutil::string::npos);

    __netif_registry.registerStack(had);
}

/* ------------------------------------------------------------------- sysfs */

TEST(sysfs, a_pin_value_node_exists)
{
    VfsDispatcher *fs = mountedVfs();
    ASSERT_TRUE(fs->isFileExist("/sys/class/gpio/2/value"));
}

TEST(sysfs, a_pin_past_the_end_has_no_node)
{
    VfsDispatcher *fs = mountedVfs();
    char path[48];
    snprintf(path, sizeof(path), "/sys/class/gpio/%d/value", MAX_GPIO_PINS);

    ASSERT_FALSE(fs->isFileExist(path));
}

TEST(sysfs, a_pin_the_device_reserves_has_no_node)
{
    VfsDispatcher *fs = mountedVfs();
    ASSERT_TRUE(__i_dvc_ctrl.isExceptionalGpio(3));

    ASSERT_FALSE(fs->isFileExist("/sys/class/gpio/3/value"));
    ASSERT_EQ(fs->writeFile("/sys/class/gpio/3/value", "1", 1), STORAGE_ERROR_READ_ONLY);
}

TEST(sysfs, an_unknown_leaf_has_no_node)
{
    VfsDispatcher *fs = mountedVfs();
    ASSERT_FALSE(fs->isFileExist("/sys/class/gpio/2/direction"));
}

/**
 * A write is accepted and reports the bytes it took. The value it leaves
 * behind is asserted in the system tier instead: SysFs::writeFile hands the
 * config to the database and then reloads it through handleGpioModes, so the
 * round trip only closes once the database service is running.
 */
TEST(sysfs, a_value_write_is_accepted)
{
    VfsDispatcher *fs = mountedVfs();

    ASSERT_EQ(fs->writeFile("/sys/class/gpio/2/value", "1", 1), 1);
    ASSERT_TRUE(slurp(fs, "/sys/class/gpio/2/value").length() > 0);
}

TEST(sysfs, a_mode_write_is_accepted)
{
    VfsDispatcher *fs = mountedVfs();

    ASSERT_EQ(fs->writeFile("/sys/class/gpio/4/mode", "1", 1), 1);
    ASSERT_TRUE(slurp(fs, "/sys/class/gpio/4/mode").length() > 0);
}

TEST(sysfs, a_leaf_reads_as_a_number_and_a_newline)
{
    VfsDispatcher *fs = mountedVfs();
    pdiutil::string value = slurp(fs, "/sys/class/gpio/2/value");

    ASSERT_TRUE(value.length() >= 2);
    ASSERT_EQ(value[value.length() - 1], '\n');
    ASSERT_TRUE(value[0] >= '0' && value[0] <= '9');
}

TEST(sysfs, a_mode_past_the_last_one_is_refused)
{
    VfsDispatcher *fs = mountedVfs();
    char mode[4];
    snprintf(mode, sizeof(mode), "%d", GPIO_MODE_MAX);

    ASSERT_EQ(fs->writeFile("/sys/class/gpio/4/mode", mode, strlen(mode)), PDI_ERR_RANGE);
}

TEST(sysfs, a_write_to_a_directory_is_refused)
{
    VfsDispatcher *fs = mountedVfs();
    ASSERT_EQ(fs->writeFile("/sys/class/gpio", "1", 1), STORAGE_ERROR_READ_ONLY);
}

TEST(sysfs, the_class_directory_lists_gpio)
{
    VfsDispatcher *fs = mountedVfs();
    pdiutil::vector<file_info_t> items;

    ASSERT_TRUE(fs->getDirFileList("/sys/class", items) >= 0);

    bool sawgpio = false;
    for (file_info_t &item : items)
    {
        if (nullptr != item.m_name && 0 == strcmp(item.m_name, "gpio")) sawgpio = true;
    }
    for (file_info_t &item : items)
    {
        pdiutil::safe_delete_array(item.m_name);
    }

    ASSERT_TRUE(sawgpio);
}

TEST(sysfs, a_pin_directory_lists_its_leaves)
{
    VfsDispatcher *fs = mountedVfs();
    pdiutil::vector<file_info_t> items;

    ASSERT_TRUE(fs->getDirFileList("/sys/class/gpio/2", items) >= 0);

    bool sawvalue = false;
    bool sawmode = false;
    for (file_info_t &item : items)
    {
        if (nullptr != item.m_name && 0 == strcmp(item.m_name, "value")) sawvalue = true;
        if (nullptr != item.m_name && 0 == strcmp(item.m_name, "mode")) sawmode = true;
    }
    for (file_info_t &item : items)
    {
        pdiutil::safe_delete_array(item.m_name);
    }

    ASSERT_TRUE(sawvalue);
    ASSERT_TRUE(sawmode);
}

/* ------------------------------------------------------------------- devfs */

TEST(devfs, the_three_nodes_exist)
{
    VfsDispatcher *fs = mountedVfs();

    ASSERT_TRUE(fs->isFileExist("/dev/null"));
    ASSERT_TRUE(fs->isFileExist("/dev/zero"));
    ASSERT_TRUE(fs->isFileExist("/dev/random"));
}

TEST(devfs, an_unknown_node_is_absent)
{
    VfsDispatcher *fs = mountedVfs();
    ASSERT_FALSE(fs->isFileExist("/dev/sda"));
}

TEST(devfs, null_swallows_a_write_and_reads_back_nothing)
{
    VfsDispatcher *fs = mountedVfs();

    ASSERT_TRUE(fs->writeFile("/dev/null", "discarded", 9) >= 0);
    ASSERT_EQ(slurp(fs, "/dev/null").length(), 0u);
}

TEST(devfs, zero_reads_as_zero_bytes)
{
    VfsDispatcher *fs = mountedVfs();

    uint32_t nonzero = 0;
    uint32_t seen = 0;
    fs->readFile("/dev/zero", 16, [&nonzero, &seen](char *chunk, uint32_t len) {
        for (uint32_t i = 0; i < len; i++)
        {
            if (chunk[i] != 0) nonzero++;
        }
        seen += len;
        return false;
    });

    ASSERT_TRUE(seen > 0);
    ASSERT_EQ(nonzero, 0u);
}

TEST(devfs, random_does_not_repeat_itself)
{
    VfsDispatcher *fs = mountedVfs();

    pdiutil::string first = slurp(fs, "/dev/random");
    pdiutil::string second = slurp(fs, "/dev/random");

    ASSERT_TRUE(first.length() > 0);
    ASSERT_STRNE(first.c_str(), second.c_str());
}

TEST(devfs, a_stream_delivers_all_of_itself_when_the_match_cannot_occur)
{
    VfsDispatcher *fs = mountedVfs();

    uint32_t seen = 0;
    bool matched = false;
    int bytes = fs->readFile("/dev/zero", 16, [&seen](char *chunk, uint32_t len) {
        seen += len;
        return true;
    }, 0, "\n", &matched);

    ASSERT_FALSE(matched);
    ASSERT_EQ(seen, (uint32_t)DEVFS_STREAM_READ_MAX);
    ASSERT_EQ((uint32_t)bytes, seen);
}

TEST(devfs, a_stream_stops_at_the_string_it_was_told_to_stop_at)
{
    VfsDispatcher *fs = mountedVfs();

    uint16_t matches = 0;
    for (uint16_t attempt = 0; attempt < 64; attempt++)
    {
        pdiutil::string out;
        bool matched = false;
        int bytes = fs->readFile("/dev/random", 16, [&out](char *chunk, uint32_t len) {
            for (uint32_t i = 0; i < len; i++)
            {
                out += chunk[i];
            }
            return true;
        }, 0, "\n", &matched);

        ASSERT_TRUE(bytes >= 0);
        ASSERT_EQ((uint32_t)bytes, (uint32_t)out.length());

        if (matched)
        {
            matches++;
            ASSERT_TRUE(out.length() < (uint32_t)DEVFS_STREAM_READ_MAX);
            for (uint32_t i = 0; i < out.length(); i++)
            {
                ASSERT_TRUE(out[i] != '\n');
            }
        }
        else
        {
            ASSERT_EQ(out.length(), (uint32_t)DEVFS_STREAM_READ_MAX);
        }
    }

    ASSERT_TRUE(matches > 0);
}

TEST(devfs, the_directory_lists_its_nodes)
{
    VfsDispatcher *fs = mountedVfs();
    pdiutil::vector<file_info_t> items;

    ASSERT_TRUE(fs->getDirFileList("/dev", items) >= 0);

    uint8_t found = 0;
    for (file_info_t &item : items)
    {
        if (nullptr == item.m_name) continue;
        if (0 == strcmp(item.m_name, "null")) found++;
        if (0 == strcmp(item.m_name, "zero")) found++;
        if (0 == strcmp(item.m_name, "random")) found++;
    }
    for (file_info_t &item : items)
    {
        pdiutil::safe_delete_array(item.m_name);
    }

    ASSERT_EQ(found, 3);
}

/* ------------------------------------------------------------------- tmpfs */

TEST(tmpfs, a_file_round_trips)
{
    VfsDispatcher *fs = mountedVfs();
    removeIfPresent(fs, "/tmp/note.txt");

    ASSERT_GE(fs->createFile("/tmp/note.txt", "in memory"), 0);
    ASSERT_TRUE(fs->isFileExist("/tmp/note.txt"));
    ASSERT_STREQ(slurp(fs, "/tmp/note.txt").c_str(), "in memory");

    fs->deleteFile("/tmp/note.txt");
}

TEST(tmpfs, its_content_never_reaches_the_flash)
{
    VfsDispatcher *fs = mountedVfs();
    removeIfPresent(fs, "/tmp/volatile.txt");

    ASSERT_GE(fs->createFile("/tmp/volatile.txt", "gone on reboot"), 0);
    ASSERT_FALSE(__i_rootfs.isFileExist("/tmp/volatile.txt"));
    ASSERT_FALSE(__i_rootfs.isFileExist("/volatile.txt"));

    fs->deleteFile("/tmp/volatile.txt");
}

TEST(tmpfs, a_deleted_file_is_gone)
{
    VfsDispatcher *fs = mountedVfs();
    fs->createFile("/tmp/short.txt", "brief");

    ASSERT_EQ(fs->deleteFile("/tmp/short.txt"), (pdi_err_t)0);
    ASSERT_FALSE(fs->isFileExist("/tmp/short.txt"));
}

TEST(tmpfs, it_reports_its_own_budget_not_the_flash)
{
    VfsDispatcher *fs = mountedVfs();

    ASSERT_EQ(__i_tmpfs.getTotalSize(), (uint64_t)TMPFS_MAX_BYTES);
    ASSERT_TRUE(__i_tmpfs.getTotalSize() < fs->getTotalSize());
}

TEST(tmpfs, free_space_falls_as_it_fills_and_returns_when_cleared)
{
    VfsDispatcher *fs = mountedVfs();
    uint64_t before = __i_tmpfs.getFreeSize();

    pdiutil::string payload(200, 'x');
    ASSERT_GE(fs->createFile("/tmp/filler.txt", payload.c_str()), 0);
    ASSERT_TRUE(__i_tmpfs.getFreeSize() < before);

    fs->deleteFile("/tmp/filler.txt");
    ASSERT_EQ(__i_tmpfs.getFreeSize(), before);
}

TEST(tmpfs, a_write_past_the_budget_is_refused)
{
    VfsDispatcher *fs = mountedVfs();
    removeIfPresent(fs, "/tmp/toobig.txt");

    pdiutil::string payload(TMPFS_MAX_BYTES + 64, 'x');
    ASSERT_EQ(fs->createFile("/tmp/toobig.txt", payload.c_str()), PDI_ERR_NO_SPACE);
    ASSERT_FALSE(fs->isFileExist("/tmp/toobig.txt"));
}

TEST(tmpfs, filling_it_leaves_the_root_filesystem_alone)
{
    VfsDispatcher *fs = mountedVfs();
    uint64_t rootfree = fs->getFreeSize();

    pdiutil::string payload(512, 'y');
    ASSERT_GE(fs->createFile("/tmp/bulk.txt", payload.c_str()), 0);
    ASSERT_EQ(fs->getFreeSize(), rootfree);

    fs->deleteFile("/tmp/bulk.txt");
}

TEST(tmpfs, a_read_stops_at_the_string_it_was_told_to_stop_at)
{
    VfsDispatcher *fs = mountedVfs();
    removeIfPresent(fs, "/tmp/lines.txt");
    ASSERT_GE(fs->createFile("/tmp/lines.txt", "first\nsecond\nthird\n"), 0);

    pdiutil::string first;
    bool matched = false;
    int bytes = fs->readFile("/tmp/lines.txt", 64, [&first](char *chunk, uint32_t len) {
        for (uint32_t i = 0; i < len; i++)
        {
            first += chunk[i];
        }
        return true;
    }, 0, "\n", &matched);

    ASSERT_TRUE(matched);
    ASSERT_EQ((uint32_t)bytes, (uint32_t)first.length());
    ASSERT_STREQ(first.c_str(), "first");

    fs->deleteFile("/tmp/lines.txt");
}

TEST(tmpfs, stepping_by_the_match_walks_every_line_once)
{
    VfsDispatcher *fs = mountedVfs();
    removeIfPresent(fs, "/tmp/lines.txt");
    ASSERT_GE(fs->createFile("/tmp/lines.txt", "first\nsecond\nthird\n"), 0);

    int64_t size = fs->getFileSize("/tmp/lines.txt");
    uint16_t lines = 0;
    pdiutil::string rebuilt;
    uint64_t offset = 0;

    while (offset < (uint64_t)size && lines < 16)
    {
        pdiutil::string line;
        int bytes = fs->readFile("/tmp/lines.txt", 64, [&line](char *chunk, uint32_t len) {
            for (uint32_t i = 0; i < len; i++)
            {
                line += chunk[i];
            }
            return true;
        }, offset, "\n");

        if (bytes < 0) break;
        offset += (uint64_t)bytes + 1;

        rebuilt += line;
        rebuilt += "\n";
        lines++;
    }

    ASSERT_EQ(lines, 3);
    ASSERT_STREQ(rebuilt.c_str(), "first\nsecond\nthird\n");

    fs->deleteFile("/tmp/lines.txt");
}

TEST(tmpfs, a_read_with_no_match_in_it_delivers_the_rest)
{
    VfsDispatcher *fs = mountedVfs();
    removeIfPresent(fs, "/tmp/lines.txt");
    ASSERT_GE(fs->createFile("/tmp/lines.txt", "first\nsecond\nthird\n"), 0);

    pdiutil::string out;
    bool matched = false;
    fs->readFile("/tmp/lines.txt", 64, [&out](char *chunk, uint32_t len) {
        for (uint32_t i = 0; i < len; i++)
        {
            out += chunk[i];
        }
        return true;
    }, 0, "@@nosuchmatch@@", &matched);

    ASSERT_FALSE(matched);
    ASSERT_STREQ(out.c_str(), "first\nsecond\nthird\n");

    fs->deleteFile("/tmp/lines.txt");
}

TEST(tmpfs, a_config_file_in_memory_parses_a_line_at_a_time)
{
    VfsDispatcher *fs = mountedVfs();
    removeIfPresent(fs, "/tmp/probe.conf");
    ASSERT_GE(fs->createFile("/tmp/probe.conf", "# a comment\nport 2222\nname in memory\n"), 0);

    pdiutil::vector<config_kv_t> pairs;
    ASSERT_TRUE(loadConfigFile("/tmp/probe.conf", pairs));

    ASSERT_EQ((uint32_t)pairs.size(), 2u);
    ASSERT_STREQ(pairs[0].m_key.c_str(), "port");
    ASSERT_STREQ(pairs[0].m_value.c_str(), "2222");
    ASSERT_STREQ(pairs[1].m_key.c_str(), "name");
    ASSERT_STREQ(pairs[1].m_value.c_str(), "in memory");

    fs->deleteFile("/tmp/probe.conf");
}

/* ------------------------------------------------------------- cross mount */

TEST(vfs, a_file_copies_from_the_flash_to_memory)
{
    VfsDispatcher *fs = mountedVfs();
    removeIfPresent(fs, "/source.txt");
    removeIfPresent(fs, "/tmp/copied.txt");
    fs->createFile("/source.txt", "carried across");

    ASSERT_EQ(fs->copyFile("/source.txt", "/tmp/copied.txt"), (pdi_err_t)0);
    ASSERT_TRUE(fs->isFileExist("/source.txt"));
    ASSERT_STREQ(slurp(fs, "/tmp/copied.txt").c_str(), "carried across");

    fs->deleteFile("/source.txt");
    fs->deleteFile("/tmp/copied.txt");
}

TEST(vfs, a_file_copies_from_memory_to_the_flash)
{
    VfsDispatcher *fs = mountedVfs();
    removeIfPresent(fs, "/tmp/origin.txt");
    removeIfPresent(fs, "/landed.txt");
    fs->createFile("/tmp/origin.txt", "the other way");

    ASSERT_EQ(fs->copyFile("/tmp/origin.txt", "/landed.txt"), (pdi_err_t)0);
    ASSERT_STREQ(slurp(fs, "/landed.txt").c_str(), "the other way");

    fs->deleteFile("/tmp/origin.txt");
    fs->deleteFile("/landed.txt");
}

TEST(vfs, a_move_across_mounts_leaves_nothing_behind)
{
    VfsDispatcher *fs = mountedVfs();
    removeIfPresent(fs, "/moving.txt");
    removeIfPresent(fs, "/tmp/moved.txt");
    fs->createFile("/moving.txt", "relocated");

    ASSERT_EQ(fs->moveFile("/moving.txt", "/tmp/moved.txt"), (pdi_err_t)0);
    ASSERT_FALSE(fs->isFileExist("/moving.txt"));
    ASSERT_STREQ(slurp(fs, "/tmp/moved.txt").c_str(), "relocated");

    fs->deleteFile("/tmp/moved.txt");
}

TEST(vfs, a_rename_across_mounts_carries_the_content)
{
    VfsDispatcher *fs = mountedVfs();
    removeIfPresent(fs, "/renaming.txt");
    removeIfPresent(fs, "/tmp/renamed.txt");
    fs->createFile("/renaming.txt", "new home");

    ASSERT_EQ(fs->rename("/renaming.txt", "/tmp/renamed.txt"), (pdi_err_t)0);
    ASSERT_FALSE(fs->isFileExist("/renaming.txt"));
    ASSERT_STREQ(slurp(fs, "/tmp/renamed.txt").c_str(), "new home");

    fs->deleteFile("/tmp/renamed.txt");
}

TEST(vfs, a_copy_within_one_mount_still_works)
{
    VfsDispatcher *fs = mountedVfs();
    removeIfPresent(fs, "/tmp/one.txt");
    removeIfPresent(fs, "/tmp/two.txt");
    fs->createFile("/tmp/one.txt", "same backend");

    ASSERT_EQ(fs->copyFile("/tmp/one.txt", "/tmp/two.txt"), (pdi_err_t)0);
    ASSERT_STREQ(slurp(fs, "/tmp/two.txt").c_str(), "same backend");

    fs->deleteFile("/tmp/one.txt");
    fs->deleteFile("/tmp/two.txt");
}

/* ------------------------------------------------------------- permissions */

TEST(vfsperm, root_is_not_stopped_by_the_mode_bits)
{
    VfsDispatcher *fs = mountedVfs();
    removeIfPresent(fs, "/locked.txt");
    fs->createFile("/locked.txt", "secret");
    fs->setFileOwner("/locked.txt", 5, 5);
    fs->setFilePermissions("/locked.txt", 0600);

    ScopedSession asroot(0, 0);
    ASSERT_STREQ(slurp(fs, "/locked.txt").c_str(), "secret");

    fs->deleteFile("/locked.txt");
}

TEST(vfsperm, another_user_is_refused_a_read)
{
    VfsDispatcher *fs = mountedVfs();
    removeIfPresent(fs, "/private.txt");
    fs->createFile("/private.txt", "not for you");
    fs->setFileOwner("/private.txt", 5, 5);
    fs->setFilePermissions("/private.txt", 0600);

    {
        ScopedSession stranger(7, 7);
        ASSERT_EQ(fs->readFile("/private.txt", 8, [](char *, uint32_t) { return true; }), PDI_ERR_PERM);
    }

    fs->deleteFile("/private.txt");
}

TEST(vfsperm, the_owner_gets_the_owner_bits)
{
    VfsDispatcher *fs = mountedVfs();
    removeIfPresent(fs, "/mine.txt");
    fs->createFile("/mine.txt", "readable by me");
    fs->setFileOwner("/mine.txt", 5, 5);
    fs->setFilePermissions("/mine.txt", 0600);

    {
        ScopedSession owner(5, 5);
        ASSERT_STREQ(slurp(fs, "/mine.txt").c_str(), "readable by me");
    }

    fs->deleteFile("/mine.txt");
}

TEST(vfsperm, the_group_gets_the_group_bits)
{
    VfsDispatcher *fs = mountedVfs();
    removeIfPresent(fs, "/shared.txt");
    fs->createFile("/shared.txt", "readable by us");
    fs->setFileOwner("/shared.txt", 5, 9);
    fs->setFilePermissions("/shared.txt", 0640);

    {
        ScopedSession member(7, 9);
        ASSERT_STREQ(slurp(fs, "/shared.txt").c_str(), "readable by us");
    }
    {
        ScopedSession outsider(7, 8);
        ASSERT_EQ(fs->readFile("/shared.txt", 8, [](char *, uint32_t) { return true; }), PDI_ERR_PERM);
    }

    fs->deleteFile("/shared.txt");
}

TEST(vfsperm, everyone_gets_the_other_bits)
{
    VfsDispatcher *fs = mountedVfs();
    removeIfPresent(fs, "/public.txt");
    fs->createFile("/public.txt", "readable by all");
    fs->setFileOwner("/public.txt", 5, 5);
    fs->setFilePermissions("/public.txt", 0644);

    {
        ScopedSession anyone(7, 7);
        ASSERT_STREQ(slurp(fs, "/public.txt").c_str(), "readable by all");
    }

    fs->deleteFile("/public.txt");
}

TEST(vfsperm, a_write_needs_the_write_bit)
{
    VfsDispatcher *fs = mountedVfs();
    removeIfPresent(fs, "/readonly.txt");
    fs->createFile("/readonly.txt", "fixed");
    fs->setFileOwner("/readonly.txt", 5, 5);
    fs->setFilePermissions("/readonly.txt", 0444);

    {
        ScopedSession anyone(7, 7);
        ASSERT_EQ(fs->writeFile("/readonly.txt", "changed", 7), PDI_ERR_PERM);
    }
    ASSERT_STREQ(slurp(fs, "/readonly.txt").c_str(), "fixed");

    fs->deleteFile("/readonly.txt");
}

TEST(vfsperm, only_the_owner_or_root_may_change_the_mode)
{
    VfsDispatcher *fs = mountedVfs();
    removeIfPresent(fs, "/chmodme.txt");
    fs->createFile("/chmodme.txt", "x");
    fs->setFileOwner("/chmodme.txt", 5, 5);
    fs->setFilePermissions("/chmodme.txt", 0644);

    {
        ScopedSession stranger(7, 7);
        ASSERT_EQ(fs->setFilePermissions("/chmodme.txt", 0777), PDI_ERR_PERM);
    }
    {
        ScopedSession owner(5, 5);
        ASSERT_EQ(fs->setFilePermissions("/chmodme.txt", 0640), 0);
    }

    fs->deleteFile("/chmodme.txt");
}

TEST(vfsperm, only_root_may_change_the_owner)
{
    VfsDispatcher *fs = mountedVfs();
    removeIfPresent(fs, "/chownme.txt");
    fs->createFile("/chownme.txt", "x");
    fs->setFileOwner("/chownme.txt", 5, 5);

    {
        ScopedSession owner(5, 5);
        ASSERT_EQ(fs->setFileOwner("/chownme.txt", 6, 6), PDI_ERR_PERM);
    }
    {
        ScopedSession asroot(0, 0);
        ASSERT_EQ(fs->setFileOwner("/chownme.txt", 6, 6), 0);
    }

    fs->deleteFile("/chownme.txt");
}

TEST(vfsperm, a_missing_file_is_created_where_the_user_may_write_the_directory)
{
    VfsDispatcher *fs = mountedVfs();
    removeIfPresent(fs, "/fresh.txt");
    {
        ScopedSession asroot(0, 0);
        if (fs->isDirExist("/fresh_dir")) fs->deleteDirectory("/fresh_dir");
        fs->createDirectory("/fresh_dir");
        fs->setFileOwner("/fresh_dir", 7, 7);
    }

    {
        ScopedSession anyone(7, 7);
        ASSERT_GE(fs->createFile("/fresh_dir/fresh.txt", "made by a user"), 0);
        ASSERT_EQ(fs->createFile("/fresh.txt", "made by a user"), PDI_ERR_PERM);
    }

    ASSERT_FALSE(fs->isFileExist("/fresh.txt"));
    {
        ScopedSession asroot(0, 0);
        fs->deleteDirectory("/fresh_dir");
    }
}

TEST(vfsperm, a_privileged_scope_reads_past_the_mode_bits)
{
    VfsDispatcher *fs = mountedVfs();
    removeIfPresent(fs, "/shadowlike.txt");
    fs->createFile("/shadowlike.txt", "hashes");
    fs->setFileOwner("/shadowlike.txt", 0, 0);
    fs->setFilePermissions("/shadowlike.txt", 0600);

    {
        ScopedSession anyone(7, 7);
        ASSERT_EQ(fs->readFile("/shadowlike.txt", 8, [](char *, uint32_t) { return true; }), PDI_ERR_PERM);

        fs->beginPrivileged();
        ASSERT_TRUE(fs->isPrivileged());
        ASSERT_STREQ(slurp(fs, "/shadowlike.txt").c_str(), "hashes");
        fs->endPrivileged();

        ASSERT_FALSE(fs->isPrivileged());
        ASSERT_EQ(fs->readFile("/shadowlike.txt", 8, [](char *, uint32_t) { return true; }), PDI_ERR_PERM);
    }

    fs->deleteFile("/shadowlike.txt");
}

TEST(vfsperm, nested_privileged_scopes_only_lift_at_the_outermost)
{
    VfsDispatcher *fs = mountedVfs();
    ScopedSession anyone(7, 7);

    fs->beginPrivileged();
    fs->beginPrivileged();
    fs->endPrivileged();
    ASSERT_TRUE(fs->isPrivileged());
    fs->endPrivileged();
    ASSERT_FALSE(fs->isPrivileged());
}

TEST(vfsperm, ending_a_scope_that_never_began_does_not_underflow)
{
    VfsDispatcher *fs = mountedVfs();

    fs->endPrivileged();
    ASSERT_FALSE(fs->isPrivileged());
}


/* ------------------------------------------------------------------- netif */

TEST(netif, the_registry_holds_the_wifi_interfaces)
{
    pditest::readyNetifs();

    ASSERT_TRUE(__netif_registry.count() >= 2);
    ASSERT_TRUE(nullptr != __netif_registry.find("wlan0"));
    ASSERT_TRUE(nullptr != __netif_registry.find("ap0"));
    ASSERT_TRUE(nullptr == __netif_registry.find("eth0"));
}

TEST(netif, a_name_already_taken_is_refused)
{
    pditest::readyNetifs();

    iNetifInterface *existing = __netif_registry.find("wlan0");
    ASSERT_TRUE(nullptr != existing);

    ASSERT_EQ(__netif_registry.registerNetif(existing), (int8_t)PDI_ERR_EXISTS);
}

TEST(netif, registering_nothing_is_refused)
{
    ASSERT_EQ(__netif_registry.registerNetif(nullptr), (int8_t)PDI_ERR_NULL_PTR);
}

TEST(netif, an_interface_answers_with_its_addresses)
{
    pditest::readyNetifs();

    iNetifInterface *wlan = __netif_registry.find("wlan0");
    ASSERT_TRUE(nullptr != wlan);

    netif_info_t info;
    ASSERT_TRUE(wlan->getInfo(info));
    ASSERT_EQ(info.m_kind, NETIF_KIND_WIFI_STA);
    ASSERT_TRUE(strlen(info.m_mac) > 0);
}

TEST(netif, an_interface_without_counters_says_so)
{
    pditest::readyNetifs();

    iNetifInterface *wlan = __netif_registry.find("wlan0");
    ASSERT_TRUE(nullptr != wlan);

    netif_counters_t counters;
    ASSERT_FALSE(wlan->getCounters(counters));
}

/* --------------------------------------------------------------- sysfs net */

TEST(sysfs, the_class_directory_lists_net_beside_gpio)
{
    VfsDispatcher *fs = mountedVfs();
    pditest::readyNetifs();
    pdiutil::vector<file_info_t> items;

    ASSERT_TRUE(fs->getDirFileList("/sys/class", items) >= 0);

    bool sawgpio = false;
    bool sawnet = false;
    for (file_info_t &item : items)
    {
        if (nullptr == item.m_name) continue;
        if (0 == strcmp(item.m_name, "gpio")) sawgpio = true;
        if (0 == strcmp(item.m_name, "net")) sawnet = true;
    }
    for (file_info_t &item : items)
    {
        pdiutil::safe_delete_array(item.m_name);
    }

    ASSERT_TRUE(sawgpio);
    ASSERT_TRUE(sawnet);
}

TEST(sysfs, the_net_directory_lists_every_registered_interface)
{
    VfsDispatcher *fs = mountedVfs();
    pditest::readyNetifs();
    pdiutil::vector<file_info_t> items;

    ASSERT_TRUE(fs->getDirFileList("/sys/class/net", items) >= 0);
    ASSERT_EQ((uint8_t)items.size(), __netif_registry.count());

    for (file_info_t &item : items)
    {
        pdiutil::safe_delete_array(item.m_name);
    }
}

TEST(sysfs, a_station_reports_its_association_and_an_access_point_does_not)
{
    VfsDispatcher *fs = mountedVfs();
    pditest::readyNetifs();

    ASSERT_TRUE(fs->isFileExist("/sys/class/net/wlan0/ssid"));
    ASSERT_TRUE(fs->isFileExist("/sys/class/net/wlan0/rssi"));

    // an access point has no association to describe, so those leaves are
    // absent rather than present and empty
    ASSERT_FALSE(fs->isFileExist("/sys/class/net/ap0/ssid"));
    ASSERT_FALSE(fs->isFileExist("/sys/class/net/ap0/rssi"));
    ASSERT_TRUE(fs->isFileExist("/sys/class/net/ap0/ip"));
}

TEST(sysfs, an_interface_that_is_not_registered_has_no_directory)
{
    VfsDispatcher *fs = mountedVfs();
    pditest::readyNetifs();

    ASSERT_FALSE(fs->isDirExist("/sys/class/net/eth0"));
    ASSERT_FALSE(fs->isFileExist("/sys/class/net/eth0/ip"));
}

TEST(sysfs, operstate_reads_up_or_down)
{
    VfsDispatcher *fs = mountedVfs();
    pditest::readyNetifs();
    pdiutil::string state = slurp(fs, "/sys/class/net/wlan0/operstate");

    ASSERT_TRUE(state == pdiutil::string("up" TERMINAL_NEW_LINE) ||
                state == pdiutil::string("down" TERMINAL_NEW_LINE));
}

TEST(sysfs, the_address_leaf_reads_the_interface_mac)
{
    VfsDispatcher *fs = mountedVfs();
    pditest::readyNetifs();
    pdiutil::string address = slurp(fs, "/sys/class/net/wlan0/address");

    netif_info_t info;
    ASSERT_TRUE(__netif_registry.find("wlan0")->getInfo(info));
    ASSERT_TRUE(address.find(info.m_mac) == 0);
}

TEST(sysfs, a_net_leaf_is_read_only)
{
    VfsDispatcher *fs = mountedVfs();
    pditest::readyNetifs();

    ASSERT_EQ(fs->writeFile("/sys/class/net/wlan0/ip", "1.2.3.4", 7),
              (int)STORAGE_ERROR_READ_ONLY);

    uint16_t perms = 0;
    ASSERT_TRUE(fs->getFileAttr("/sys/class/net/wlan0/ip", FILE_ATTR_PERMS,
                                &perms, sizeof(perms)) > 0);
    ASSERT_EQ(perms, (uint16_t)0444);
}

TEST(sysfs, a_gpio_leaf_is_still_writable)
{
    VfsDispatcher *fs = mountedVfs();

    uint16_t perms = 0;
    ASSERT_TRUE(fs->getFileAttr("/sys/class/gpio/2/value", FILE_ATTR_PERMS,
                                &perms, sizeof(perms)) > 0);
    ASSERT_EQ(perms, (uint16_t)0666);
}
