/******************************* Lsblk Command ********************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Lists the disks the drivers bound to devices present, one row each with the
name of its node under /dev, its size in bytes and its type.

Author          : Suraj I.
created Date    : 20th Oct 2026
******************************************************************************/
#ifndef _LSBLK_COMMAND_H_
#define _LSBLK_COMMAND_H_

#include "CommandCommon.h"

#ifdef ENABLE_DRIVER_CORE

#include <service_provider/transport/SerialServiceProvider.h>
#include <interface/pdi/modules/driver/iBlockDriverInterface.h>

struct LsblkCommand : public CommandBase {

	LsblkCommand(){
		Clear();
		SetCommand(CMD_NAME_LSBLK);
	}

	static void RegisterCommand(){
		CommandBase::RegisterCommand(CMD_NAME_LSBLK, [](void *arg)->void *{
			return pdiutil::safe_new<LsblkCommand>();
		});
	}

	const char* getUsage() const override {
		return RODT_ATTR("lsblk  list disks with their size in bytes");
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

		m_terminal->putln();
		m_terminal->write_pad_ro(RODT_ATTR("NAME"), 4, COL_NAME);
		m_terminal->write_pad_ro(RODT_ATTR("SIZE"), 4, COL_SIZE);
		m_terminal->write_pad_ro(RODT_ATTR("TYPE"), 4, COL_TYPE);
		m_terminal->writeln_ro(RODT_ATTR("FSTYPE"));

		DeviceManager *mgr = __serial_service.devices();
		pdiutil::string node;
		pdiutil::string disktype = CHARPTR_WRAP(DEVICE_TYPE_DISK);
		pdiutil::string parttype = CHARPTR_WRAP(DEVICE_TYPE_PART);

		for( uint16_t i = 0; nullptr != mgr && i < mgr->count(); i++ ){
			const driver_device_t *dev = mgr->at(i);
			iBlockDriverInterface *disk = iBlockDriverInterface::diskOf(dev);
			if( nullptr == disk || !disk->nodeName(node) ){
				continue;
			}
			writeRow(node, (uint64_t)disk->blocks() * disk->blockSize(), disktype, disk->fsType());

			for( uint8_t n = 1; n <= disk->partitionCount(); n++ ){
				const block_partition_t *part = disk->partition(n);
				if( nullptr == part ){
					continue;
				}
				pdiutil::string name = blockPartitionName(node.c_str(), n);
				writeRow(name, (uint64_t)part->m_blocks * disk->blockSize(), parttype, part->m_fs);
			}
		}

		return PDI_OK;
	}

private:

	static constexpr uint8_t COL_NAME = 12;
	static constexpr uint8_t COL_SIZE = 14;
	static constexpr uint8_t COL_TYPE = 6;

	/**
	 * One row: the node's name, its size in bytes, its type and its filesystem.
	 */
	void writeRow(const pdiutil::string &name, uint64_t bytes, const pdiutil::string &type, uint8_t fs){
		char size[24];
		Int64ToString((int64_t)bytes, size, sizeof(size), 0);
		pdiutil::string fsname = blockFsTypeName(fs);
		m_terminal->write_pad(name.c_str(), COL_NAME);
		m_terminal->write_pad(size, COL_SIZE);
		m_terminal->write_pad(type.c_str(), COL_TYPE);
		m_terminal->writeln(fsname.c_str());
	}
};

#endif

#endif
