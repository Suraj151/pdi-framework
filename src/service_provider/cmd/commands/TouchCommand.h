/******************************* Touch Command ********************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st June 2019
******************************************************************************/

#ifndef _TOUCH_COMMAND_H_
#define _TOUCH_COMMAND_H_

#include "CommandCommon.h"

#ifdef ENABLE_STORAGE_SERVICE
/**
 * touch command — create the file empty if missing; bump mtime if it exists.
 * e.g. touch <file>
 */
struct TouchCommand : public CommandBase {

	TouchCommand(){
		Clear();
		SetCommand(CMD_NAME_TOUCH);
		setAcceptArgsOptions(true);
	}

	static void RegisterCommand(){
		CommandBase::RegisterCommand(CMD_NAME_TOUCH, [](void *arg)->void *{
			return new TouchCommand();
		});
	}

	const char* getUsage() const override {
		return RODT_ATTR("touch <file>  create empty file or bump mtime (no spaces in name)");
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

		cmd_result_t result = CMD_RESULT_OK;

		if(nullptr != m_terminal){
			CommandOption *cmdoptn = &m_options[0];
			pdiutil::string filename = resolveArgPath(cmdoptn);
			if( !filename.empty() ){
				int bStatus = __i_fs.touch(filename.c_str());
				if (bStatus < 0) {
					result = CMD_RESULT_FAILED;
					m_terminal->putln();
					m_terminal->write_ro(RODT_ATTR("Failed to touch: "));
					m_terminal->write(filename.c_str());
					m_terminal->write_ro(RODT_ATTR(" : "));
					m_terminal->write((int32_t)bStatus);
				}
			}else{
				result = CMD_RESULT_ARGS_ERROR;
			}
		}

		return result;
	}
};
#endif


#endif
