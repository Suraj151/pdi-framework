/********************************* Mkfs Command *******************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Makes a filesystem on a disk or partition, the way linux mkfs does:
mkfs -t vfat|littlefs <node>. A whole disk that carries a partition table is
refused, as mkfs.vfat refuses one without -I; wipefs -a clears it first.

Author          : Suraj I.
created Date    : 23rd Oct 2026
******************************************************************************/
#ifndef _MKFS_COMMAND_H_
#define _MKFS_COMMAND_H_

#include "CommandCommon.h"

#if defined(ENABLE_DRIVER_CORE) && defined(ENABLE_STORAGE_SERVICE) && defined(ENABLE_DEVFS) && defined(ENABLE_DISK_FS)

#include <interface/pdi/impl/modules/storage/DiskTools.h>
#ifdef ENABLE_FAT_FS
#include <interface/pdi/impl/modules/storage/FatFsInterfaceImpl.h>
#endif
#ifdef ENABLE_LFS_DISK
#include <interface/pdi/impl/modules/storage/LfsDiskInterfaceImpl.h>
#endif

struct MkfsCommand : public CommandBase {

	MkfsCommand(){
		Clear();
		SetCommand(CMD_NAME_MKFS);
		setAcceptArgsOptions(true);
		setCmdOptionSeparator(CMD_OPTION_SEPERATOR_SPACE);
	}

	static void RegisterCommand(){
		CommandBase::RegisterCommand(CMD_NAME_MKFS, [](void *arg)->void *{
			return pdiutil::safe_new<MkfsCommand>();
		});
	}

	const char* getUsage() const override {
		return RODT_ATTR("mkfs -t vfat|littlefs <node>  make a filesystem on a disk or partition");
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
			return CMD_ERROR_NOTTY;
		}

		const CommandOption *flag = &m_options[0];
		const CommandOption *type = &m_options[1];
		if( nullptr == flag->optionval || 2 != flag->optionvalsize || 0 != strncmp(flag->optionval, "-t", 2) ||
			nullptr == type->optionval || type->optionvalsize <= 0 ||
			nullptr == m_options[2].optionval || m_options[2].optionvalsize <= 0 ){
			return CMD_ERROR_ARGS_MISSING;
		}
		pdiutil::string device = resolveArgPath(&m_options[2]);
		pdiutil::string fstype(type->optionval, (pdiutil::string::size_type)type->optionvalsize);

		uint32_t first = 0, span = 0;
		iBlockDriverInterface *disk = DiskTools::open(device.c_str(), first, span);
		pdi_err_t refused = PDI_OK;
		formatter_t format = formatter(fstype);
		if( nullptr == format ){
			refused = PDI_ERR_NOT_SUPPORTED;
		}else if( nullptr == disk ){
			refused = PDI_ERR_NOT_FOUND;
		}
		else if( !SessionManager::isRoot() ){
			refused = PDI_ERR_PERM;
		}
		else if( DiskTools::inUse(disk) ){
			refused = PDI_ERR_BUSY;
		}else if( 0 == first && BLOCK_TABLE_NONE != disk->tableType() ){
			refused = PDI_ERR_EXISTS;
		}
		if( PDI_OK != refused ){
			return reportFailure(device, refused);
		}

		m_terminal->putln();
		m_terminal->write_ro(RODT_ATTR("mkfs: formatting "));
		m_terminal->write(device.c_str());
		m_terminal->write_ro(RODT_ATTR(" as "));
		m_terminal->write(fstype.c_str());
		m_terminal->writeln_ro(RODT_ATTR(", this may take some time"));

		pdi_err_t made = format(device.c_str());
		if( PDI_OK != made ){
			return reportFailure(device, made);
		}
		return PDI_OK;
	}

private:

	typedef pdi_err_t (*formatter_t)(const char *device);

	/**
	 * What makes the filesystem the type names, nullptr for one it cannot make.
	 */
	static formatter_t formatter(const pdiutil::string &fstype){
#ifdef ENABLE_FAT_FS
		pdiutil::string vfat = CHARPTR_WRAP(BLOCK_FS_NAME_VFAT);
		pdiutil::string fat = CHARPTR_WRAP(BLOCK_FS_NAME_FAT);
		pdiutil::string msdos = CHARPTR_WRAP(BLOCK_FS_NAME_MSDOS);
		if( fstype == vfat || fstype == fat || fstype == msdos ){
			return &FatFsInterfaceImpl::formatDisk;
		}
#endif
#ifdef ENABLE_LFS_DISK
		pdiutil::string littlefs = CHARPTR_WRAP(BLOCK_FS_NAME_LITTLEFS);
		if( fstype == littlefs ){
			return &LfsDiskInterfaceImpl::formatDisk;
		}
#endif
		return nullptr;
	}

	/**
	 * Says which node failed and with what code.
	 */
	pdi_err_t reportFailure(const pdiutil::string &device, pdi_err_t code){
		m_terminal->putln();
		writeFailure(RODT_ATTR("mkfs: "), device.c_str(), (int32_t)code);
		m_terminal->putln();
		return CMD_ERROR_FAILED;
	}
};

#endif

#endif
