/*********************** Make Dir File System Command *************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st June 2019
******************************************************************************/

#ifndef _MKD_FILE_SYSTEM_COMMAND_H_
#define _MKD_FILE_SYSTEM_COMMAND_H_

#include "CommandCommon.h"

#ifdef ENABLE_STORAGE_SERVICE
/**
 * make directory file system command
 * 
 * e.g. if we want to create a new directory, we can execute command as below
 * mkd <directory_name>
 */
struct MakeDirFSCommand : public CommandBase {

	/* Constructor */
	MakeDirFSCommand(){
		Clear();
		SetCommand(CMD_NAME_MKD);
		setAcceptArgsOptions(true);
	}

#ifdef ENABLE_AUTH_SERVICE
	/* override the necesity of required permission */
	bool needauth() override { return true; }
#endif

	/* execute command with provided options */
	cmd_result_t execute(){

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
				char *dirname = new char[cmdoptn->optionvalsize+__i_fs.pwd()->size()+2]();
				if( nullptr != dirname ){
					memset(dirname, 0, cmdoptn->optionvalsize+__i_fs.pwd()->size()+2);
					memcpy(dirname, __i_fs.pwd()->c_str(), __i_fs.pwd()->size());
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
			}
		}

		return result;
	}
};
#endif


#endif
