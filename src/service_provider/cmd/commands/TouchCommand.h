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
			if( nullptr != cmdoptn && nullptr != cmdoptn->optionval && cmdoptn->optionvalsize > 0 ){
				char *filename = new char[cmdoptn->optionvalsize+SessionManager::getPWD().size()+2]();
				if( nullptr != filename ){
					memset(filename, 0, cmdoptn->optionvalsize+SessionManager::getPWD().size()+2);
					memcpy(filename, SessionManager::getPWD().c_str(), SessionManager::getPWD().size());
					__i_fs.appendFileSeparator(filename);
					strncat(filename, cmdoptn->optionval, cmdoptn->optionvalsize);
					int bStatus = __i_fs.touch(filename);
					if (bStatus < 0) {
						result = CMD_RESULT_FAILED;
						m_terminal->putln();
						m_terminal->write_ro(RODT_ATTR("Failed to touch: "));
						m_terminal->write(filename);
						m_terminal->write_ro(RODT_ATTR(" : "));
						m_terminal->write((int32_t)bStatus);
					}
					delete[] filename;
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
