/**************************** PWD File System Command *************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st June 2019
******************************************************************************/

#ifndef _PWD_FILE_SYSTEM_COMMAND_H_
#define _PWD_FILE_SYSTEM_COMMAND_H_

#include "CommandCommon.h"

#ifdef ENABLE_STORAGE_SERVICE
/**
 * pwd file system command
 * 
 * e.g. if we want to print the current working directory, we can execute command as below
 * pwd
 */
struct PWDFSCommand  : public CommandBase {

	/* Constructor */
	PWDFSCommand (){
		Clear();
		SetCommand(CMD_NAME_PWD);
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
			m_terminal->putln();
			m_terminal->write(__i_fs.pwd()->c_str());
		}

		return result;
	}
};
#endif


#endif
