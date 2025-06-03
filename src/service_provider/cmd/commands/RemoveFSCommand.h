/******************************* Remove Command ******************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st June 2019
******************************************************************************/

#ifndef _REMOVE_FILE_SYSTEM_COMMAND_H_
#define _REMOVE_FILE_SYSTEM_COMMAND_H_

#include "CommandCommon.h"

#ifdef ENABLE_STORAGE_SERVICE
/**
 * remove file/directory command
 * e.g. if we want to remove a file or directory, we can execute command as below
 * rm <file_or_dir_name>
 */
struct RemoveFSCommand : public CommandBase {

	/* Constructor */
	RemoveFSCommand(){
		Clear();
		SetCommand(CMD_NAME_RM);
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
				char *fileOrDir = new char[cmdoptn->optionvalsize+__i_fs.pwd()->size()+2]();
				if( nullptr != fileOrDir ){
					memset(fileOrDir, 0, cmdoptn->optionvalsize+__i_fs.pwd()->size()+2);
					memcpy(fileOrDir, __i_fs.pwd()->c_str(), __i_fs.pwd()->size());
					__i_fs.appendFileSeparator(fileOrDir);
					strncat(fileOrDir, cmdoptn->optionval, cmdoptn->optionvalsize);

					int iStatus = 0;
					if( __i_fs.isDirectory(fileOrDir) ){
						iStatus = __i_fs.deleteDirectory(fileOrDir);
					}else{
						iStatus = __i_fs.deleteFile(fileOrDir);
					}

					if(iStatus < 0){
						result = CMD_RESULT_FAILED;
						m_terminal->putln();
						m_terminal->write_ro(RODT_ATTR("Failed to delete : "));
						m_terminal->write(fileOrDir);
						m_terminal->write_ro(RODT_ATTR(" : "));
						m_terminal->write((int32_t)iStatus);
					}

					delete[] fileOrDir;	
				}
			}
		}

		return result;
	}
};
#endif


#endif
