/******************************* Mount Command ********************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 20th July 2026
******************************************************************************/

#ifndef _MOUNT_COMMAND_H_
#define _MOUNT_COMMAND_H_

#include "CommandCommon.h"

#ifdef ENABLE_STORAGE_SERVICE

/**
 * mount command — list active VFS mount points.
 *
 * e.g.
 *   mount   PREFIX  TYPE     NAME
 *           /       littlefs rootfs
 */
struct MountCommand : public CommandBase {

	MountCommand(){
		Clear();
		SetCommand(CMD_NAME_MOUNT);
	}

	static void RegisterCommand(){
		CommandBase::RegisterCommand(CMD_NAME_MOUNT, [](void *arg)->void *{
			return new MountCommand();
		});
	}

	const char* getUsage() const override {
		return RODT_ATTR("mount  list VFS mount points (prefix, type, backend name)");
	}

#ifdef ENABLE_AUTH_SERVICE
	bool needauth() override { return true; }
#endif

	cmd_result_t execute(cmd_term_inseq_t terminputaction){

#ifdef ENABLE_AUTH_SERVICE
		if( needauth() && !__auth_service.getAuthorized() ){
			return CMD_RESULT_NEED_AUTH;
		}
#endif

		if( nullptr == m_terminal ){
			return CMD_RESULT_FAILED;
		}

		m_terminal->writeln();
		m_terminal->write_pad_ro(RODT_ATTR("PREFIX"), 6, COL_PREFIX);
		m_terminal->write_pad_ro(RODT_ATTR("TYPE"),   4, COL_TYPE);
		m_terminal->writeln_ro(RODT_ATTR("NAME"));

		for( uint8_t i = 0; i < __i_fs.getMountCount(); i++ ){
			const vfs_mount_t *m = __i_fs.getMount(i);
			if( nullptr == m ) continue;
			m_terminal->write_pad(m->m_prefix, COL_PREFIX);
			const char *tlabel = typeLabel(m->m_type);
			m_terminal->write_pad_ro(tlabel, (uint32_t)strlen_ro(tlabel), COL_TYPE);
			m_terminal->writeln(m->m_name);
		}

		return CMD_RESULT_OK;
	}

private:

	static constexpr uint8_t COL_PREFIX = 12;
	static constexpr uint8_t COL_TYPE   = 10;

	static const char* typeLabel(vfs_type_t t){
		switch(t){
			case VFS_TYPE_LITTLEFS: return RODT_ATTR("littlefs");
			case VFS_TYPE_SPIFFS:   return RODT_ATTR("spiffs");
			case VFS_TYPE_SD:       return RODT_ATTR("sd");
			case VFS_TYPE_TMPFS:    return RODT_ATTR("tmpfs");
			case VFS_TYPE_PROCFS:   return RODT_ATTR("procfs");
			case VFS_TYPE_SYSFS:    return RODT_ATTR("sysfs");
			case VFS_TYPE_DEVFS:    return RODT_ATTR("devfs");
			default:                return RODT_ATTR("unknown");
		}
	}
};

#endif

#endif
