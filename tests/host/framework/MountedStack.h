/***************************** Mounted Stack **********************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

One mount for the whole binary. The unit tier reaches the root filesystem
directly and the system tier reaches it through the dispatcher, but there is
only ever one LittleFS on the emulated flash — mounting it twice leaks the
first mount's buffers and leaves two views of the same blocks.

Author          : Suraj I.
created Date    : 16th Aug 2026
******************************************************************************/

#ifndef _PDITEST_MOUNTED_STACK_H_
#define _PDITEST_MOUNTED_STACK_H_

#include <interface/pdi.h>
#ifdef ENABLE_PROCFS
#include <interface/pdi/impl/modules/storage/ProcFs.h>
#endif
#ifdef ENABLE_SYSFS
#include <interface/pdi/impl/modules/storage/SysFs.h>
#endif
#ifdef ENABLE_DEVFS
#include <interface/pdi/impl/modules/storage/DevFs.h>
#endif
#ifdef ENABLE_TMPFS
#include <interface/pdi/impl/modules/storage/TmpFs.h>
#endif

namespace pditest
{

/**
 * @brief The root filesystem, formatted and mounted on first use.
 * @return The mounted filesystem, or nullptr if it would not mount.
 */
inline FileSystemInterface *rootFs()
{
    static bool ready = false;
    static bool mounted = false;

    if (!ready)
    {
        ready = true;
        __i_storage.eraseAll();
        mounted = (PDI_OK == __i_rootfs.init());
    }

    return mounted ? &__i_rootfs : nullptr;
}

/**
 * @brief The dispatcher with the same mount table PdiStack brings up.
 *
 * The root filesystem is mounted through rootFs, and the synthetic backends
 * hold no storage, so nothing here mounts anything a second time.
 */
inline VfsDispatcher *mountedVfs()
{
    static bool ready = false;

    if (!ready)
    {
        ready = true;
        rootFs();
        __i_fs.mount(FILE_SEPARATOR, &__i_rootfs, "rootfs", VFS_TYPE_LITTLEFS);
#ifdef ENABLE_PROCFS
        __i_fs.mount("/proc", &__i_procfs, "procfs", VFS_TYPE_PROCFS);
#endif
#ifdef ENABLE_SYSFS
        __i_fs.mount("/sys", &__i_sysfs, "sysfs", VFS_TYPE_SYSFS);
#endif
#ifdef ENABLE_DEVFS
        __i_fs.mount("/dev", &__i_devfs, "devfs", VFS_TYPE_DEVFS);
#endif
#ifdef ENABLE_TMPFS
        __i_fs.mount("/tmp", &__i_tmpfs, "tmpfs", VFS_TYPE_TMPFS);
#endif
    }

    return &__i_fs;
}

} // namespace pditest

#endif // _PDITEST_MOUNTED_STACK_H_
