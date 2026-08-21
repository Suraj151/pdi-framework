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
#include <service_provider/session/SessionManager.h>
#include <MountedStack.h>
#include <pditest.h>

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

TEST(vfsperm, a_missing_file_is_not_refused_so_it_can_be_created)
{
    VfsDispatcher *fs = mountedVfs();
    removeIfPresent(fs, "/fresh.txt");

    {
        ScopedSession anyone(7, 7);
        ASSERT_GE(fs->createFile("/fresh.txt", "made by a user"), 0);
    }

    fs->deleteFile("/fresh.txt");
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
