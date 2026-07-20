/******************************* Mkdir Command ********************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st June 2019
******************************************************************************/

#ifndef _MKDIR_COMMAND_H_
#define _MKDIR_COMMAND_H_

#include "CommandCommon.h"

#ifdef ENABLE_STORAGE_SERVICE
/**
 * mkdir command — create a directory.
 * e.g. mkdir <dir_name>
 */
struct MkdirCommand : public CommandBase {

	MkdirCommand(){
		Clear();
		SetCommand(CMD_NAME_MKDIR);
		setAcceptArgsOptions(true);
	}

	static void RegisterCommand(){
		CommandBase::RegisterCommand(CMD_NAME_MKDIR, [](void *arg)->void *{
			return new MkdirCommand();
		});
	}

	const char* getUsage() const override {
		return RODT_ATTR("mkdir <dir>  create directory (no spaces in name)");
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
				char *dirname = new char[cmdoptn->optionvalsize+SessionManager::getPWD().size()+2]();
				if( nullptr != dirname ){
					memset(dirname, 0, cmdoptn->optionvalsize+SessionManager::getPWD().size()+2);
					memcpy(dirname, SessionManager::getPWD().c_str(), SessionManager::getPWD().size());
					__i_fs.appendFileSeparator(dirname);
					strncat(dirname, cmdoptn->optionval, cmdoptn->optionvalsize);
					int bStatus = __i_fs.createDirectory(dirname);
					if (bStatus < 0) {
						result = CMD_RESULT_FAILED;
						m_terminal->putln();
						m_terminal->write_ro(RODT_ATTR("Failed to create directory: "));
						m_terminal->write(dirname);
						m_terminal->write_ro(RODT_ATTR(" : "));
						m_terminal->write((int32_t)bStatus);
					}
					delete[] dirname;
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
