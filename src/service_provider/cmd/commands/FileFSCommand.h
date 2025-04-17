/******************************* File Command ********************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st June 2019
******************************************************************************/

#ifndef _FILE_OPERATION_FS_COMMAND_H_
#define _FILE_OPERATION_FS_COMMAND_H_

#include "CommandCommon.h"

/**
 * file operation command
 * 
 * e.g. if we want to operate on a file in the system then we can execute command as below
 * operate f={FILE_PATH}, m={MODES}, v={VALUE}
 * 
 * value can be given in subsequent enter i.e. first give only file and mode and enter then
 * we can provide value for file
 * 
 * This command allows users to modify files within the system, providing options for 
 * configuration and status checks.
 */
struct FileFSCommand : public CommandBase {

	/* Service options */
	enum FileFSCommandModes{
		FILE_FS_COMMAND_MODE_MIN = 0,
		FILE_FS_COMMAND_MODE_OVERWRITE,
		FILE_FS_COMMAND_MODE_APPEND,
		FILE_FS_COMMAND_MODE_READ,
		FILE_FS_COMMAND_MODE_MAX
	};

	/* Constructor */
	FileFSCommand(){
		Clear();
		SetCommand(CMD_NAME_FILE);
		AddOption(CMD_OPTION_NAME_F);
		AddOption(CMD_OPTION_NAME_M);
		AddOption(CMD_OPTION_NAME_V);
		setAcceptArgsOptions(true);
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
		FileFSCommandModes fmode = FILE_FS_COMMAND_MODE_MAX;

		CommandOption *filenamecmdoptn = RetrieveOption(CMD_OPTION_NAME_F);	
		CommandOption *editmodecmdoptn = RetrieveOption(CMD_OPTION_NAME_M);	
		CommandOption *valuecmdoptn = RetrieveOption(CMD_OPTION_NAME_V);	

		bool isFilenameProvided = ( nullptr != filenamecmdoptn && nullptr != filenamecmdoptn->optionval && filenamecmdoptn->optionvalsize );
		bool isEditModeProvided = ( nullptr != editmodecmdoptn && nullptr != editmodecmdoptn->optionval && editmodecmdoptn->optionvalsize );
		bool isFileValueProvided = ( nullptr != valuecmdoptn && nullptr != valuecmdoptn->optionval && valuecmdoptn->optionvalsize );

		if( isFilenameProvided ){
			if( !isEditModeProvided || !isFileValueProvided ){
				holdOptionValue(CMD_OPTION_NAME_F);
			}
		}else{
			setWaitingForOption(CMD_OPTION_NAME_F);
			if( nullptr != m_terminal ){
				m_terminal->putln();
				m_terminal->write_ro(RODT_ATTR("Enter Filename : "));
			}
			return CMD_RESULT_INCOMPLETE;
		}

		if( isEditModeProvided ){
			fmode = (FileFSCommandModes)StringToUint16(editmodecmdoptn->optionval, editmodecmdoptn->optionvalsize);
			if( !isFileValueProvided ){
				holdOptionValue(CMD_OPTION_NAME_M);
			}
		}else{
			setWaitingForOption(CMD_OPTION_NAME_M);
			if( nullptr != m_terminal ){
				m_terminal->putln();
				m_terminal->write_ro(RODT_ATTR("Enter mode : "));
			}
			return CMD_RESULT_INCOMPLETE;
		}

		if( !isFileValueProvided && fmode != FILE_FS_COMMAND_MODE_READ ){
			setWaitingForOption(CMD_OPTION_NAME_V);
			if( nullptr != m_terminal ){
				m_terminal->putln();
				m_terminal->write_ro(RODT_ATTR("Enter data : "));
			}
			return CMD_RESULT_INCOMPLETE;
		}

		if( isFilenameProvided && isEditModeProvided && (isFileValueProvided || fmode == FILE_FS_COMMAND_MODE_READ) ){
			char *filename = new char[filenamecmdoptn->optionvalsize+__i_fs.pwd()->size()+2]();
			if( nullptr != filename ){
				memset(filename, 0, filenamecmdoptn->optionvalsize+__i_fs.pwd()->size()+2);
				memcpy(filename, __i_fs.pwd()->c_str(), __i_fs.pwd()->size());
				__i_fs.appendFileSeparator(filename);
				strncat(filename, filenamecmdoptn->optionval, filenamecmdoptn->optionvalsize);

				int iStatus = 0;

				if( fmode == FILE_FS_COMMAND_MODE_READ ){
					// read file
					if( nullptr != m_terminal ){
						m_terminal->putln();
						m_terminal->write_ro(RODT_ATTR("Reading file : "));
						m_terminal->write(filename);
						m_terminal->putln();
						iStatus = __i_fs.readFile(filename, 5, [&](char* data, uint32_t size)->bool{
							m_terminal->write(data, size);
							// return true to continue reading
							return true;
                        });
					}
				}else if( fmode == FILE_FS_COMMAND_MODE_APPEND || 
				          fmode == FILE_FS_COMMAND_MODE_OVERWRITE ){
					iStatus = __i_fs.createFile(filename, valuecmdoptn->optionval, valuecmdoptn->optionvalsize);
				}

				if (iStatus < 0) {
					result = CMD_RESULT_FAILED;
					m_terminal->putln();
					m_terminal->write_ro(RODT_ATTR("Failed : "));
					m_terminal->write(filename);
					m_terminal->write_ro(RODT_ATTR(" : "));
					m_terminal->write(iStatus);
				}

				delete[] filename;	
			}
		}else{
			result = CMD_RESULT_ARGS_ERROR;
		}

		return result;
	}
};


#endif
