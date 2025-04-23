/*****************************  Move File Command *****************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st June 2019
******************************************************************************/

#ifndef _MOVE_FILE_SYSTEM_COMMAND_H_
#define _MOVE_FILE_SYSTEM_COMMAND_H_

#include "CommandCommon.h"

#ifdef ENABLE_STORAGE_SERVICE
/**
 * move file command
 * e.g. if we want to move a file or dir, we can execute command as below
 * move <old_file_name> <new_file_name>
 */
struct MoveFSCommand : public CommandBase {

	/* Constructor */
	MoveFSCommand(){
		Clear();
		SetCommand(CMD_NAME_MOVE);
		setAcceptArgsOptions(true);
		setCmdOptionSeparator(CMD_OPTION_SEPERATOR_SPACE);
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
			// Get first option which must be the old path and second option which must be the new path
			CommandOption *cmdoptn1 = &m_options[0];
			CommandOption *cmdoptn2 = &m_options[1];
			if( nullptr != cmdoptn1 && nullptr != cmdoptn1->optionval && cmdoptn1->optionvalsize > 0 &&
				nullptr != cmdoptn2 && nullptr != cmdoptn2->optionval && cmdoptn2->optionvalsize > 0){
				char *oldname = new char[cmdoptn1->optionvalsize+__i_fs.pwd()->size()+2]();
				char *newname = new char[cmdoptn2->optionvalsize+__i_fs.pwd()->size()+2]();
				if( nullptr != oldname && nullptr != newname ){
					memset(oldname, 0, cmdoptn1->optionvalsize+__i_fs.pwd()->size()+2);
					memset(newname, 0, cmdoptn2->optionvalsize+__i_fs.pwd()->size()+2);

					memcpy(oldname, __i_fs.pwd()->c_str(), __i_fs.pwd()->size());
					memcpy(newname, __i_fs.pwd()->c_str(), __i_fs.pwd()->size());

					__i_fs.appendFileSeparator(oldname);
					__i_fs.appendFileSeparator(newname);

					strncat(oldname, cmdoptn1->optionval, cmdoptn1->optionvalsize);
					strncat(newname, cmdoptn2->optionval, cmdoptn2->optionvalsize);

					int iStatus = __i_fs.rename(oldname, newname);
					if (iStatus < 0) {
						result = CMD_RESULT_FAILED;
						m_terminal->putln();
						m_terminal->write_ro(RODT_ATTR("Failed to rename file: "));
						m_terminal->write(oldname);
						m_terminal->write_ro(RODT_ATTR(" : "));
						m_terminal->write(newname);
						m_terminal->write_ro(RODT_ATTR(" : "));
						m_terminal->write((int32_t)iStatus);
					}
				}

				if(nullptr != oldname){
					delete[] oldname;	
				}
				if(nullptr != newname){
					delete[] newname;	
				}
			}
		}

		return result;
	}
};
#endif


#endif
