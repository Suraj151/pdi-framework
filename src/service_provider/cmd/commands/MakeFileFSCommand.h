/****************************** Make File Command *****************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st June 2019
******************************************************************************/

#ifndef _MKF_FILE_SYSTEM_COMMAND_H_
#define _MKF_FILE_SYSTEM_COMMAND_H_

#include "CommandCommon.h"

#ifdef ENABLE_STORAGE_SERVICE
/**
 * make file command
 * e.g. if we want to create a new file, we can execute command as below
 * mkf <file_name>
 */
struct MakeFileFSCommand : public CommandBase {

	/* Constructor */
	MakeFileFSCommand(){
		Clear();
		SetCommand(CMD_NAME_MKF);
		setAcceptArgsOptions(true);
	}

#ifdef ENABLE_AUTH_SERVICE
	/* override the necesity of required permission */
	bool needauth() override { return true; }
#endif

	/* execute command with provided options */
	cmd_result_t execute(cmd_term_inseq_t terminputaction){

#ifdef ENABLE_AUTH_SERVICE
		// return in case authentication needed and not authorized yet
		if( needauth() && !__auth_service.getAuthorized()){
			return CMD_RESULT_NEED_AUTH;
		}
#endif

		cmd_result_t result = CMD_RESULT_OK;

		if(nullptr != m_terminal){
			// Get first option which must be the path
			CommandOption *cmdoptn = &m_options[0];
			if( nullptr != cmdoptn && nullptr != cmdoptn->optionval && cmdoptn->optionvalsize > 0 ){
				char *filename = new char[cmdoptn->optionvalsize+__i_fs.getPWD().size()+2]();
				if( nullptr != filename ){
					memset(filename, 0, cmdoptn->optionvalsize+__i_fs.getPWD().size()+2);
					memcpy(filename, __i_fs.getPWD().c_str(), __i_fs.getPWD().size());
					__i_fs.appendFileSeparator(filename);
					strncat(filename, cmdoptn->optionval, cmdoptn->optionvalsize);
					int bStatus = __i_fs.createFile(filename, "");
					if (bStatus < 0) {
						result = CMD_RESULT_FAILED;
						m_terminal->putln();
						m_terminal->write_ro(RODT_ATTR("Failed to create file: "));
						m_terminal->write(filename);
						m_terminal->write_ro(RODT_ATTR(" : "));
						m_terminal->write((int32_t)bStatus);
					}
					delete[] filename;	
				}
			}
		}

		return result;
	}
};
#endif


#endif
