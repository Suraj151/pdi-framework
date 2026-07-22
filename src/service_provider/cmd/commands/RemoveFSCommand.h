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

	/**
     * @brief Register the command.
     */
    static void RegisterCommand(){
		CommandBase::RegisterCommand(CMD_NAME_RM, [](void *arg)->void *{
			return new RemoveFSCommand();
		});
	}

	const char* getUsage() const override {
		return RODT_ATTR("rm <file_or_dir>  remove a file or directory");
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
			pdiutil::string fileOrDir = resolveArgPath(cmdoptn);
			if( !fileOrDir.empty() ){

				int iStatus = 0;
				if( __i_fs.isDirectory(fileOrDir.c_str()) ){
					iStatus = __i_fs.deleteDirectory(fileOrDir.c_str());
				}else{
					iStatus = __i_fs.deleteFile(fileOrDir.c_str());
				}

				if(iStatus < 0){
					result = CMD_RESULT_FAILED;
					m_terminal->putln();
					m_terminal->write_ro(RODT_ATTR("Failed to delete : "));
					m_terminal->write(fileOrDir.c_str());
					m_terminal->write_ro(RODT_ATTR(" : "));
					m_terminal->write((int32_t)iStatus);
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
