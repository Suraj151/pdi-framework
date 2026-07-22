/*****************************  Copy File Command *****************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st June 2019
******************************************************************************/

#ifndef _COPY_FILE_SYSTEM_COMMAND_H_
#define _COPY_FILE_SYSTEM_COMMAND_H_

#include "CommandCommon.h"

#ifdef ENABLE_STORAGE_SERVICE
/**
 * copy file command
 * e.g. if we want to copy a file or dir, we can execute command as below
 * copy <src_file_path> <dst_file_path>
 */
struct CopyFSCommand : public CommandBase {

	/* Constructor */
	CopyFSCommand(){
		Clear();
		SetCommand(CMD_NAME_COPY);
		setAcceptArgsOptions(true);
		setCmdOptionSeparator(CMD_OPTION_SEPERATOR_SPACE);
	}

	/**
     * @brief Register the command.
     */
    static void RegisterCommand(){
		CommandBase::RegisterCommand(CMD_NAME_COPY, [](void *arg)->void *{
			return new CopyFSCommand();
		});
	}

	const char* getUsage() const override {
		return RODT_ATTR("cp <src> <dst>  copy a file");
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
			// Get first option which must be the old path and second option which must be the new path
			CommandOption *cmdoptn1 = &m_options[0];
			CommandOption *cmdoptn2 = &m_options[1];
			if( nullptr != cmdoptn1 && nullptr != cmdoptn1->optionval && cmdoptn1->optionvalsize > 0 &&
				nullptr != cmdoptn2 && nullptr != cmdoptn2->optionval && cmdoptn2->optionvalsize > 0){

				pdiutil::string srcpath = resolveArgPath(cmdoptn1);
				pdiutil::string dstpath = resolveArgPath(cmdoptn2);

				if( !srcpath.empty() && !dstpath.empty() &&
					!__are_str_equals(srcpath.c_str(), dstpath.c_str()) ){

					int iStatus = 0;

					if( !__i_fs.isFileExist(srcpath.c_str()) || __i_fs.isFileExist(dstpath.c_str()) ){
						result = CMD_RESULT_FAILED;
						m_terminal->putln();
						m_terminal->writeln_ro(RODT_ATTR("src not exist OR dst exist"));
					}else{
						iStatus = __i_fs.copyFile(srcpath.c_str(), dstpath.c_str());
					}

					if (iStatus < 0) {
						result = CMD_RESULT_FAILED;
						m_terminal->putln();
						m_terminal->write_ro(RODT_ATTR("Failed to copy file: "));
						m_terminal->write(srcpath.c_str());
						m_terminal->write_ro(RODT_ATTR(" : "));
						m_terminal->write(dstpath.c_str());
						m_terminal->write_ro(RODT_ATTR(" : "));
						m_terminal->write((int32_t)iStatus);
					}
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
