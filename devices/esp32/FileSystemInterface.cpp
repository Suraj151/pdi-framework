/*************************** File System Interface ****************************
This file is part of the PDI Stack.

This is free software. You can redistribute it and/or modify it but without any
warranty.

This interface defines the basic operations required for interacting with
a file system, such as creating, reading, writing to files or directories.

Author          : Suraj I.
Created Date    : 6th Apr 2025
******************************************************************************/

#include "FileSystemInterface.h"


/**
 * @brief Global instance of the iFileSystemInterface class.
 * This instance is used to manage file system operations throughout the PDI stack.
 */
FileSystemInterface __i_fs;
