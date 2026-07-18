/*************************** User Store Config ********************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 18th July 2026
******************************************************************************/
#ifndef _USER_STORE_CONFIG_H_
#define _USER_STORE_CONFIG_H_

#include "Common.h"

#ifndef USER_STORE_ETC_DIR
#define USER_STORE_ETC_DIR "/etc"
#endif

#ifndef USER_STORE_PASSWD_PATH
#define USER_STORE_PASSWD_PATH "/etc/passwd"
#endif

#ifndef USER_STORE_SHADOW_PATH
#define USER_STORE_SHADOW_PATH "/etc/shadow"
#endif

#ifndef USER_STORE_MAX_USERNAME_LEN
#define USER_STORE_MAX_USERNAME_LEN 25
#endif

#ifndef USER_STORE_SALT_LEN
#define USER_STORE_SALT_LEN 8
#endif

#ifndef USER_STORE_ROOT_UID
#define USER_STORE_ROOT_UID 0
#endif

#ifndef USER_STORE_ROOT_GID
#define USER_STORE_ROOT_GID 0
#endif

#ifndef USER_STORE_DEFAULT_SHELL
#define USER_STORE_DEFAULT_SHELL "cmd"
#endif

#ifndef USER_STORE_FIELD_SEP
#define USER_STORE_FIELD_SEP ':'
#endif

#endif
