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
		setArgs(RODT_ATTR("r"), 1);
		setAcceptArgsOptions(true);
		setCmdOptionSeparator(CMD_OPTION_SEPERATOR_SPACE);
	}

	/**
     * @brief Register the command.
     */
    static void RegisterCommand(){
		CommandBase::RegisterCommand(CMD_NAME_RM, [](void *arg)->void *{
			return pdiutil::safe_new<RemoveFSCommand>();
		});
	}

	const char* getUsage() const override {
		return RODT_ATTR("rm [-r] <file_or_dir>  remove a file or an empty directory (-r a directory with everything in it)");
	}

#ifdef ENABLE_AUTH_SERVICE
	/* override the necesity of required permission */
	bool needauth() override { return true; }
#endif

	/* execute command with provided options */
	pdi_err_t execute(cmd_term_inseq_t terminputaction){

#ifdef ENABLE_AUTH_SERVICE
		// return in case authentication needed and not authorized yet
		if( needauth() && !__auth_service.getAuthorized()){
			return CMD_ERROR_PERM;
		}
#endif

		pdi_err_t result = PDI_OK;

		if(nullptr != m_terminal){
			bool recursive = false;
			CommandOption *cmdoptn = nullptr;
			for( uint8_t i = 0; i < CMD_OPTION_MAX && optionGiven(&m_options[i]); i++ ){
				if( m_options[i].optionvalsize > 1 && '-' == m_options[i].optionval[0] ){
					recursive = true;
				}else{
					cmdoptn = &m_options[i];
				}
			}
			pdiutil::string fileOrDir = (nullptr != cmdoptn) ? resolveArgPath(cmdoptn) : pdiutil::string();
			if( !fileOrDir.empty() ){

				int iStatus = 0;
				if( __i_fs.isDirectory(fileOrDir.c_str()) ){
					iStatus = recursive ? 0 : emptyDirectory(fileOrDir);
					if( 0 == iStatus ){
						iStatus = __i_fs.deleteDirectory(fileOrDir.c_str());
					}
				}else{
					iStatus = __i_fs.deleteFile(fileOrDir.c_str());
				}

				if(iStatus < 0){
					result = CMD_ERROR_FAILED;
					m_terminal->putln();
					writeFailure(RODT_ATTR("Failed to delete : "), fileOrDir.c_str(), (int32_t)iStatus);
				}
			}else{
				result = CMD_ERROR_INVAL;
			}
		}

		return result;
	}

private:

	/**
	 * 0 when a directory holds nothing but its dot entries,
	 * STORAGE_ERROR_NOT_EMPTY when it holds more, or the code its listing failed with.
	 */
	static int emptyDirectory(const pdiutil::string &dir){
		pdiutil::vector<file_info_t> items;
		int listed = __i_fs.getDirFileList(dir.c_str(), items);
		int result = ( listed < 0 ) ? listed : 0;
		for( uint16_t i = 0; i < items.size(); i++ ){
			const char *name = items[i].m_name;
			if( 0 == result && nullptr != name && 0 != strcmp(name, ".") && 0 != strcmp(name, "..") ){
				result = STORAGE_ERROR_NOT_EMPTY;
			}
			pdiutil::safe_delete_array(items[i].m_name);
		}
		return result;
	}
};
#endif


#endif
