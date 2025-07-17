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

				char *srcpath = new char[cmdoptn1->optionvalsize+__i_fs.getPWD().size()+2]();
				char *dstpath = new char[cmdoptn2->optionvalsize+__i_fs.getPWD().size()+2]();

				if( nullptr != srcpath && nullptr != dstpath ){
					memset(srcpath, 0, cmdoptn1->optionvalsize+__i_fs.getPWD().size()+2);
					memset(dstpath, 0, cmdoptn2->optionvalsize+__i_fs.getPWD().size()+2);

					memcpy(srcpath, __i_fs.getPWD().c_str(), __i_fs.getPWD().size());
					memcpy(dstpath, __i_fs.getPWD().c_str(), __i_fs.getPWD().size());

					__i_fs.appendFileSeparator(srcpath);
					__i_fs.appendFileSeparator(dstpath);
					strncat(srcpath, cmdoptn1->optionval, cmdoptn1->optionvalsize);
					strncat(dstpath, cmdoptn2->optionval, cmdoptn2->optionvalsize);

					if( !__are_str_equals(srcpath, dstpath) ){

						int iStatus = 0;

						if( !__i_fs.isFileExist(srcpath) || __i_fs.isFileExist(dstpath) ){
							result = CMD_RESULT_FAILED;
							m_terminal->putln();
							m_terminal->writeln_ro(RODT_ATTR("src not exist OR dst exist"));
						}else{
							iStatus = __i_fs.copyFile(srcpath, dstpath);
						}

						if (iStatus < 0) {
							result = CMD_RESULT_FAILED;
							m_terminal->putln();
							m_terminal->write_ro(RODT_ATTR("Failed to copy file: "));
							m_terminal->write(srcpath);
							m_terminal->write_ro(RODT_ATTR(" : "));
							m_terminal->write(dstpath);
							m_terminal->write_ro(RODT_ATTR(" : "));
							m_terminal->write((int32_t)iStatus);
						}
					}
				}

				if(nullptr != srcpath){
					delete[] srcpath;	
				}
				if(nullptr != dstpath){
					delete[] dstpath;	
				}
			}
		}

		return result;
	}
};
#endif


#endif
