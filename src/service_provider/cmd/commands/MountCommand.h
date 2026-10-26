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
#include <helpers/ProcHelper.h>

#ifdef ENABLE_STORAGE_SERVICE

#ifdef ENABLE_DISK_FS
#include <interface/pdi/impl/modules/storage/DiskTools.h>
#endif

/**
 * mount command — list active VFS mount points, mount the filesystem a disk
 * node carries on an existing directory, or mount what /etc/fstab lists.
 *
 * e.g.
 *   mount   PREFIX  TYPE     NAME
 *           /       littlefs rootfs
 *   mount /dev/mmcblk0p1 /mnt
 *   mount -a
 */
struct MountCommand : public CommandBase {

	MountCommand(){
		Clear();
		SetCommand(CMD_NAME_MOUNT);
#ifdef ENABLE_DISK_FS
		setAcceptArgsOptions(true);
		setCmdOptionSeparator(CMD_OPTION_SEPERATOR_SPACE);
#endif
	}

	static void RegisterCommand(){
		CommandBase::RegisterCommand(CMD_NAME_MOUNT, [](void *arg)->void *{
			return pdiutil::safe_new<MountCommand>();
		});
	}

	const char* getUsage() const override {
#ifdef ENABLE_DISK_FS
		return RODT_ATTR("mount [-a | device dir]  list VFS mount points, mount the filesystem on device at dir, or all of /etc/fstab");
#else
		return RODT_ATTR("mount  list VFS mount points (prefix, type, backend name)");
#endif
	}

#ifdef ENABLE_AUTH_SERVICE
	bool needauth() override { return true; }
#endif

	pdi_err_t execute(cmd_term_inseq_t terminputaction){

#ifdef ENABLE_AUTH_SERVICE
		if( needauth() && !__auth_service.getAuthorized() ){
			return CMD_ERROR_PERM;
		}
#endif

		if( nullptr == m_terminal ){
			return CMD_ERROR_FAILED;
		}

#ifdef ENABLE_DISK_FS
		if( 2 == m_options[0].optionvalsize && 0 == strncmp(m_options[0].optionval, "-a", 2) ){
			return mountAll();
		}
		if( nullptr != m_options[0].optionval && m_options[0].optionvalsize > 0 ){
			return mountDevice();
		}
#endif

		m_terminal->writeln();
		m_terminal->write_pad_ro(RODT_ATTR("PREFIX"), 6, COL_PREFIX);
		m_terminal->write_pad_ro(RODT_ATTR("TYPE"),   4, COL_TYPE);
		m_terminal->writeln_ro(RODT_ATTR("NAME"));

#ifdef ENABLE_PROCFS
		pdiutil::string name, prefix, type;

		readProcLines(PROC_MOUNT_PREFIX "/mounts", [&](pdiutil::string &line) -> bool {
			if( !__line_field(line, 0, name) ) return true;
			__line_field(line, 1, prefix);
			__line_field(line, 2, type);
			m_terminal->write_pad(prefix.c_str(), COL_PREFIX);
			m_terminal->write_pad(type.c_str(), COL_TYPE);
			m_terminal->writeln(name.c_str());
			return true;
		});
#else
		for( uint8_t i = 0; i < __i_fs.getMountCount(); i++ ){
			const vfs_mount_t *m = __i_fs.getMount(i);
			if( nullptr == m ) continue;
			m_terminal->write_pad(m->m_prefix, COL_PREFIX);
			const char *tlabel = VfsTypeToString(m->m_type);
			m_terminal->write_pad_ro(tlabel, (uint32_t)strlen_ro(tlabel), COL_TYPE);
			m_terminal->writeln(m->m_name);
		}
#endif

		return PDI_OK;
	}

private:

#ifdef ENABLE_DISK_FS
	/**
	 * Mounts the filesystem the first argument's node carries on the directory
	 * the second names, for root only.
	 */
	pdi_err_t mountDevice(){
		pdiutil::string device = resolveArgPath(&m_options[0]);
		pdiutil::string dir = resolveArgPath(&m_options[1]);
		if( device.empty() || dir.empty() ){
			return CMD_ERROR_ARGS_MISSING;
		}
		__strip_trailing(dir, PATH_SEPARATOR_CHAR);

		pdi_err_t mounted = PDI_ERR_PERM;
		if( SessionManager::isRoot() ){
			mounted = DiskTools::mount(device.c_str(), dir.c_str());
		}
		if( PDI_OK != mounted ){
			reportFailure(device, mounted);
			return CMD_ERROR_FAILED;
		}
		return PDI_OK;
	}

	/**
	 * Mounts what /etc/fstab lists that is not mounted yet, for root only,
	 * naming each line that fails.
	 */
	pdi_err_t mountAll(){
		if( !SessionManager::isRoot() ){
			pdiutil::string fstab = CHARPTR_WRAP(FSTAB_PATH);
			reportFailure(fstab, PDI_ERR_PERM);
			return CMD_ERROR_FAILED;
		}
		pdi_err_t result = DiskTools::mountFstab([&](const pdiutil::string &device, pdi_err_t code){
			reportFailure(device, code);
		});
		return (PDI_OK == result) ? PDI_OK : (pdi_err_t)CMD_ERROR_FAILED;
	}

	/**
	 * Says which device failed and with what code.
	 */
	void reportFailure(const pdiutil::string &device, pdi_err_t code){
		m_terminal->putln();
		writeFailure(RODT_ATTR("mount: "), device.c_str(), (int32_t)code);
		m_terminal->putln();
	}
#endif

	static constexpr uint8_t COL_PREFIX = 12;
	static constexpr uint8_t COL_TYPE   = 10;
};

#endif

#endif
