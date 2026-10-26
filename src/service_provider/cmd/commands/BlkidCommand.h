/******************************** Blkid Command *******************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Names what each disk and partition carries, the way linux blkid does: the
filesystem as TYPE and the partition table as PTTYPE, one line per node that
carries either. blkid <node> names only that one.

Author          : Suraj I.
created Date    : 21st Oct 2026
******************************************************************************/
#ifndef _BLKID_COMMAND_H_
#define _BLKID_COMMAND_H_

#include "CommandCommon.h"

#if defined(ENABLE_DRIVER_CORE) && defined(ENABLE_STORAGE_SERVICE)

#include <service_provider/transport/SerialServiceProvider.h>
#include <interface/pdi/modules/driver/iBlockDriverInterface.h>
#include <interface/pdi/impl/modules/storage/DevFs.h>

struct BlkidCommand : public CommandBase {

	BlkidCommand(){
		Clear();
		SetCommand(CMD_NAME_BLKID);
		setAcceptArgsOptions(true);
	}

	static void RegisterCommand(){
		CommandBase::RegisterCommand(CMD_NAME_BLKID, [](void *arg)->void *{
			return pdiutil::safe_new<BlkidCommand>();
		});
	}

	const char* getUsage() const override {
		return RODT_ATTR("blkid [node]  name the filesystem and partition table each disk carries");
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

		pdiutil::string only;
		if( nullptr != m_options[0].optionval && m_options[0].optionvalsize > 0 ){
			only = resolveArgPath(&m_options[0]);
		}

		m_terminal->putln();
		DeviceManager *mgr = __serial_service.devices();
		pdiutil::string node;

		for( uint16_t i = 0; nullptr != mgr && i < mgr->count(); i++ ){
			const driver_device_t *dev = mgr->at(i);
			iBlockDriverInterface *disk = iBlockDriverInterface::diskOf(dev);
			if( nullptr == disk || !disk->nodeName(node) ){
				continue;
			}
			writeLine(node, only, disk->fsType(), disk->tableType());

			for( uint8_t n = 1; n <= disk->partitionCount(); n++ ){
				if( nullptr == disk->partition(n) ){
					continue;
				}
				pdiutil::string name = blockPartitionName(node.c_str(), n);
				writeLine(name, only, disk->partition(n)->m_fs, BLOCK_TABLE_NONE);
			}
		}

		return PDI_OK;
	}

private:

	/**
	 * One line for a node that carries a filesystem or a partition table, when
	 * no node was asked for or this is the one asked for.
	 */
	void writeLine(const pdiutil::string &name, const pdiutil::string &only, uint8_t fs, uint8_t table){
		pdiutil::string path = DevFs::pathOf(name.c_str());
		if( (!only.empty() && only != path) || (BLOCK_FS_NONE == fs && BLOCK_TABLE_NONE == table) ){
			return;
		}

		pdiutil::string line = path;
		line += ':';
		if( BLOCK_FS_NONE != fs ){
			line += ' ';
			line += CHARPTR_WRAP(BLKID_TAG_TYPE);
			line += '"';
			line += blockFsTypeName(fs);
			line += '"';
		}
		if( BLOCK_TABLE_NONE != table ){
			line += ' ';
			line += CHARPTR_WRAP(BLKID_TAG_PTTYPE);
			line += '"';
			line += blockTableName(table);
			line += '"';
		}
		m_terminal->writeln(line.c_str());
	}
};

#endif

#endif
