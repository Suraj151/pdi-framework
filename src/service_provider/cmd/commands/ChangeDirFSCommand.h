/***************************** CD File System Command *************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st June 2019
******************************************************************************/

#ifndef _CD_FILE_SYSTEM_COMMAND_H_
#define _CD_FILE_SYSTEM_COMMAND_H_

#include "CommandCommon.h"

#ifdef ENABLE_STORAGE_SERVICE
/**
 * change directory file system command
 * 
 * e.g. if we want to change the current directory, we can execute command as below
 * cd <directory_name>
 */
struct ChangeDirFSCommand : public CommandBase {

	/* Constructor */
	ChangeDirFSCommand(){
		Clear();
		SetCommand(CMD_NAME_CD);
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
				char *dirname = new char[cmdoptn->optionvalsize+200]();
				if( nullptr != dirname ){

					memset(dirname, 0, cmdoptn->optionvalsize+200);

					// Check whether symbol was provided
					if( cmdoptn->optionvalsize == 1 && cmdoptn->optionval[0] == '~' ){

						const char* homedir = __i_fs.getHomeDirectory();
						memcpy(dirname, homedir, strlen(homedir));
					}else if( cmdoptn->optionvalsize == 1 && cmdoptn->optionval[0] == '-' ){

						pdiutil::string lastpwd = __i_fs.getLastPWD();
						memcpy(dirname, lastpwd.c_str(), lastpwd.length());
					}else{

						memcpy(dirname, __i_fs.getPWD().c_str(), __i_fs.getPWD().size());
						__i_fs.appendFileSeparator(dirname);
						strncat(dirname, cmdoptn->optionval, cmdoptn->optionvalsize);
					}

					bool bStatus = __i_fs.changeDirectory(dirname);
					if(!bStatus){
						result = CMD_RESULT_FAILED;
						m_terminal->putln();
						m_terminal->write_ro(RODT_ATTR("Failed to change directory: "));
						m_terminal->write(dirname);
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
