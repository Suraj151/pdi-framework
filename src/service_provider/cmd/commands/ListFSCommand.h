/*************************** List File System Command *************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st June 2019
******************************************************************************/

#ifndef _LIST_FILE_SYSTEM_COMMAND_H_
#define _LIST_FILE_SYSTEM_COMMAND_H_

#include "CommandCommon.h"

#ifdef ENABLE_STORAGE_SERVICE
/**
 * list file system command
 * 
 * e.g. if we want to list the files & directories in the current directory, we can execute command as below
 * ls
 */
struct ListFSCommand : public CommandBase {

	/* Constructor */
	ListFSCommand(){
		Clear();
		SetCommand(CMD_NAME_LS);
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

			pdiutil::vector<file_info_t> itemlist;
			int resultCode = __i_fs.getDirFileList(__i_fs.pwd()->c_str(), itemlist);
			
			if(resultCode < 0){
				result = CMD_RESULT_FAILED;
			}

			// Print the directory and file list
			m_terminal->putln();

			m_terminal->write_ro(RODT_ATTR("Used : "));
			m_terminal->write((int64_t)__i_fs.getUsedSize());
			m_terminal->write_ro(RODT_ATTR(", Free : "));
			m_terminal->write((int64_t)__i_fs.getFreeSize());
			m_terminal->putln();

			m_terminal->writeln_ro(RODT_ATTR("Type Name        Size"));

			for (file_info_t item : itemlist) {
				if( item.type == FILE_TYPE_DIR ){
					m_terminal->write_ro(RODT_ATTR("D    "));
				}else {
					m_terminal->write_ro(RODT_ATTR("F    "));
				}
				m_terminal->write_pad(item.name, 12);
				m_terminal->write((int64_t)item.size);
				m_terminal->writeln_ro(RODT_ATTR(" bytes"));
				// deallocates memory for items
				delete[] item.name;
			}

			itemlist.clear();
		}

		return result;
	}
};
#endif


#endif
