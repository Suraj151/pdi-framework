/*************************** File System Interface ****************************
This file is part of the PDI Stack.

This is free software. You can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
Created Date    : 16th Aug 2026
******************************************************************************/

#include "FileSystemInterface.h"

/**
 * @brief Root-mount backend for the VFS dispatcher. The dispatcher itself lives
 * in VfsDispatcher.cpp and mounts this backend at "/" during PdiStack init.
 */
FileSystemInterface __i_rootfs;
