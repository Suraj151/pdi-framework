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
#ifdef ENABLE_AUTH_SERVICE
#include <service_provider/user/UserStoreService.h>
#endif

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
		setArgs(RODT_ATTR(""), 1);
		setAcceptArgsOptions(true);
	}

	/**
     * @brief Register the command.
     */
    static void RegisterCommand(){
		CommandBase::RegisterCommand(CMD_NAME_CD, [](void *arg)->void *{
			return pdiutil::safe_new<ChangeDirFSCommand>();
		});
	}

	const char* getUsage() const override {
		return RODT_ATTR("cd <dir>  change working directory");
	}

#ifdef ENABLE_AUTH_SERVICE
	/* override the necesity of required permission */
	bool needauth() override { return true; }
#endif

	/* execute command with provided options */
	/**
	 * @brief Why a directory could not be entered: not one, not there, or not allowed.
	 */
	int32_t failureOf(const pdiutil::string &dir){
		if( dir.empty() || (!__i_fs.isDirectory(dir.c_str()) && !__i_fs.isFileExist(dir.c_str())) ){
			return PDI_ERR_NOT_FOUND;
		}
		if( !__i_fs.isDirectory(dir.c_str()) ){
			return STORAGE_ERROR_NOT_A_DIRECTORY;
		}
		return PDI_ERR_PERM;
	}

	pdi_err_t execute(cmd_term_inseq_t terminputaction){

#ifdef ENABLE_AUTH_SERVICE
		// return in case authentication needed and not authorized yet
		if( needauth() && !__auth_service.getAuthorized()){
			return CMD_ERROR_PERM;
		}
#endif

		pdi_err_t result = PDI_OK;

		if(nullptr != m_terminal){
			// Get first option which must be the path
			CommandOption *cmdoptn = &m_options[0];
			if( nullptr != cmdoptn && nullptr != cmdoptn->optionval && cmdoptn->optionvalsize > 0 ){
				pdiutil::string dirname;

				// Check whether symbol was provided
				if( cmdoptn->optionvalsize == 1 && cmdoptn->optionval[0] == '~' ){

					homeDirectory(dirname);
				}else if( cmdoptn->optionvalsize == 1 && cmdoptn->optionval[0] == '-' ){

					dirname = SessionManager::getLastPWD();
				}else{

					dirname = resolveArgPath(cmdoptn);
				}

				bool bStatus = !dirname.empty() && SessionManager::changeDirectory(dirname.c_str());
				if(!bStatus){
					result = CMD_ERROR_FAILED;
					m_terminal->putln();
					writeFailure(RODT_ATTR("Failed to change directory: "), dirname.c_str(), failureOf(dirname));
				}
			}else{
				// POSIX-style: bare `cd` goes to the home directory.
				pdiutil::string homedir;
				homeDirectory(homedir);
				if( SessionManager::changeDirectory(homedir.c_str()) == false ){
					result = CMD_ERROR_FAILED;
					m_terminal->putln();
					writeFailure(RODT_ATTR("Failed to change directory: "), homedir.c_str(), failureOf(homedir));
				}
			}
		}

		return result;
	}

private:

	/**
	 * The logged in user's home, or the filesystem's own when the account
	 * names none that exists.
	 */
	void homeDirectory(pdiutil::string &out){
		out = __i_fs.getHomeDirectory();
#ifdef ENABLE_AUTH_SERVICE
		__user_store_service.homeOf(__auth_service.getUsername(), out);
#endif
	}
};
#endif


#endif
