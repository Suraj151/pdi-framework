/******************************** Df Command **********************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 28th May 2026
******************************************************************************/

#ifndef _DF_FS_COMMAND_H_
#define _DF_FS_COMMAND_H_

#include "CommandCommon.h"

#ifdef ENABLE_STORAGE_SERVICE

/**
 * df command — one line per mounted VFS backend with total/used/free bytes.
 */
struct DfFSCommand : public CommandBase {

	DfFSCommand(){
		Clear();
		SetCommand(CMD_NAME_DF);
	}

	static void RegisterCommand(){
		CommandBase::RegisterCommand(CMD_NAME_DF, [](void *arg)->void *{
			return new DfFSCommand();
		});
	}

	const char* getUsage() const override {
		return RODT_ATTR("df  list mounted filesystems with total/used/free bytes");
	}

#ifdef ENABLE_AUTH_SERVICE
	bool needauth() override { return true; }
#endif

	cmd_result_t execute(cmd_term_inseq_t terminputaction){

#ifdef ENABLE_AUTH_SERVICE
		if( needauth() && !__auth_service.getAuthorized()){
			return CMD_RESULT_NEED_AUTH;
		}
#endif

		if( nullptr == m_terminal ){
			return CMD_RESULT_FAILED;
		}

		m_terminal->writeln();
		m_terminal->write_pad_ro(RODT_ATTR("MOUNT"), 5, MOUNT_W);
		m_terminal->write_pad_ro(RODT_ATTR("NAME"),  4, NAME_W);
		m_terminal->write_pad_ro(RODT_ATTR("TOTAL"), 5, NUM_W);
		m_terminal->write_pad_ro(RODT_ATTR("USED"),  4, NUM_W);
		m_terminal->writeln_ro(RODT_ATTR("FREE"));

		char buf[24];
		for( uint8_t i = 0; i < __i_fs.getMountCount(); i++ ){
			const vfs_mount_t *m = __i_fs.getMount(i);
			if( nullptr == m || nullptr == m->m_backend ) continue;
			m_terminal->write_pad(m->m_prefix, MOUNT_W);
			m_terminal->write_pad(m->m_name,   NAME_W);
			Int64ToString((int64_t)m->m_backend->getTotalSize(), buf, sizeof(buf), 0);
			m_terminal->write_pad(buf, NUM_W);
			Int64ToString((int64_t)m->m_backend->getUsedSize(),  buf, sizeof(buf), 0);
			m_terminal->write_pad(buf, NUM_W);
			m_terminal->write((int64_t)m->m_backend->getFreeSize());
			m_terminal->putln();
		}

		return CMD_RESULT_OK;
	}

private:

	static constexpr uint8_t MOUNT_W = 10;
	static constexpr uint8_t NAME_W  = 10;
	static constexpr uint8_t NUM_W   = 12;
};

#endif

#endif
