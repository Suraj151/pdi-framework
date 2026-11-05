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
		setArgs(RODT_ATTR(""), 2);
		setAcceptArgsOptions(true);
		setCmdOptionSeparator(CMD_OPTION_SEPERATOR_SPACE);
	}

	/**
     * @brief Register the command.
     */
    static void RegisterCommand(){
		CommandBase::RegisterCommand(CMD_NAME_MOVE, [](void *arg)->void *{
			return pdiutil::safe_new<MoveFSCommand>();
		});
	}

	const char* getUsage() const override {
		return RODT_ATTR("mv <src> <dst>  move or rename a file/directory (into a directory keeps its name)");
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
			// Get first option which must be the old path and second option which must be the new path
			CommandOption *cmdoptn1 = &m_options[0];
			CommandOption *cmdoptn2 = &m_options[1];
			if( nullptr != cmdoptn1 && nullptr != cmdoptn1->optionval && cmdoptn1->optionvalsize > 0 &&
				nullptr != cmdoptn2 && nullptr != cmdoptn2->optionval && cmdoptn2->optionvalsize > 0){
				pdiutil::string oldname = resolveArgPath(cmdoptn1);
				pdiutil::string newname = resolveArgPath(cmdoptn2);
				if( __i_fs.isDirectory(newname.c_str()) ){
					__i_fs.appendFileSeparator(newname);
					newname += __i_fs.basename(oldname.c_str());
				}

				if( __are_str_equals(oldname.c_str(), newname.c_str()) ){
					m_terminal->putln();
					m_terminal->write_ro(RODT_ATTR("mv: "));
					m_terminal->write(oldname.c_str());
					m_terminal->write_ro(RODT_ATTR(" and "));
					m_terminal->write(newname.c_str());
					m_terminal->write_ro(RODT_ATTR(" are the same file"));
					return CMD_ERROR_FAILED;
				}

				const char *failed = oldname.c_str();
				int iStatus = PDI_OK;
				if( !__i_fs.isFileExist(oldname.c_str()) && !__i_fs.isDirectory(oldname.c_str()) ){
					iStatus = PDI_ERR_NOT_FOUND;
				}else if( __i_fs.isFileExist(newname.c_str()) || __i_fs.isDirectory(newname.c_str()) ){
					failed = newname.c_str();
					iStatus = PDI_ERR_EXISTS;
				}else{
					failed = newname.c_str();
					iStatus = __i_fs.rename(oldname.c_str(), newname.c_str());
				}
				if (iStatus < 0) {
					result = CMD_ERROR_FAILED;
					m_terminal->putln();
					writeFailure(RODT_ATTR("mv: "), failed, (int32_t)iStatus);
				}
			}else{
				result = CMD_ERROR_INVAL;
			}
		}

		return result;
	}
};
#endif


#endif
