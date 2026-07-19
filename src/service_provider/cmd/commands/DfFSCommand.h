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
		return RODT_ATTR("df  print filesystem total/used(%)/free bytes");
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

		if(nullptr != m_terminal){

			uint64_t total = __i_fs.getTotalSize();
			uint64_t used = __i_fs.getUsedSize();
			uint64_t free = __i_fs.getFreeSize();
			uint64_t usedpct = (total > 0) ? ((used * 100) / total) : 0;

			m_terminal->putln();
			m_terminal->write_ro(RODT_ATTR("Total : "));
			m_terminal->write((int64_t)total);
			m_terminal->putln();
			m_terminal->write_ro(RODT_ATTR("Used  : "));
			m_terminal->write((int64_t)used);
			m_terminal->write_ro(RODT_ATTR(" ("));
			m_terminal->write((int64_t)usedpct);
			m_terminal->write_ro(RODT_ATTR("%)"));
			m_terminal->putln();
			m_terminal->write_ro(RODT_ATTR("Free  : "));
			m_terminal->write((int64_t)free);
		}

		return CMD_RESULT_OK;
	}
};

#endif

#endif
