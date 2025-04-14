/***************************** File System Command ****************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st June 2019
******************************************************************************/

#ifndef _FILE_SYSTEM_COMMAND_H_
#define _FILE_SYSTEM_COMMAND_H_

#include "CommandCommon.h"

#ifdef ENABLE_STORAGE_SERVICE
/**
 * file system command
 * 
 * e.g. if we want to create a file with name 'example.txt' and size 500 bytes, we can execute command as below
 * fs create filename=example.txt, size=500
 */
struct FileSystemCommand : public CommandBase {

	/* file system query options */
	enum FSCommandQuery{
		FS_COMMAND_QUERY_MIN = 0,
		FS_COMMAND_QUERY_PWD,
		FS_COMMAND_QUERY_LIST,
		FS_COMMAND_QUERY_MAKE_FILE,
		FS_COMMAND_QUERY_MAKE_DIR,
		FS_COMMAND_QUERY_MAX
	};
	
	/* Constructor */
	FileSystemCommand(){
		Clear();
		SetCommand(CMD_NAME_FS);
		AddOption(CMD_OPTION_NAME_Q); // query of operation
		AddOption(CMD_OPTION_NAME_V); // value for option
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
		FSCommandQuery fsq = FS_COMMAND_QUERY_MAX;

		CommandOption *cmdoptn = RetrieveOption(CMD_OPTION_NAME_Q);
		if( nullptr != cmdoptn ){
			fsq = (FSCommandQuery)StringToUint16(cmdoptn->optionval, cmdoptn->optionvalsize);
		}

		if( nullptr != m_terminal && fsq > FS_COMMAND_QUERY_MIN && fsq < FS_COMMAND_QUERY_MAX ){

			if( fsq == FS_COMMAND_QUERY_PWD ){

				m_terminal->writeln(__i_fs.pwd()->c_str());
			}else if( fsq == FS_COMMAND_QUERY_LIST ){

				pdiutil::vector<file_info_t> itemlist;
				int resultCode = __i_fs.getDirFileList(__i_fs.pwd()->c_str(), itemlist);	

				// Print the directory and file list
				m_terminal->write_ro(RODT_ATTR("\nType Name        Size"));

				for (file_info_t item : itemlist) {
					if( item.type == FILE_TYPE_DIR ){
						m_terminal->write_ro(RODT_ATTR("\nD    "));
					}else {
						m_terminal->write_ro(RODT_ATTR("\nF    "));
					}
					m_terminal->write_pad(item.name, 12);
					m_terminal->write((int64_t)item.size);
					m_terminal->write_ro(RODT_ATTR(" bytes"));
					// deallocates memory for items
					delete[] item.name;
				}

				itemlist.clear();
			}else if( fsq == FS_COMMAND_QUERY_MAKE_FILE ){

				CommandOption *cmdoptn = RetrieveOption(CMD_OPTION_NAME_V);
				if( nullptr != cmdoptn ){
					char *filename = new char[cmdoptn->optionvalsize+__i_fs.pwd()->size()+2]();
					if( nullptr != filename ){
						memset(filename, 0, cmdoptn->optionvalsize+__i_fs.pwd()->size()+2);
						memcpy(filename, __i_fs.pwd()->c_str(), __i_fs.pwd()->size());
						__i_fs.appendFileSeparator(filename);
						strncat(filename, cmdoptn->optionval, cmdoptn->optionvalsize);
						int bStatus = __i_fs.createFile(filename, "");
						if (bStatus < 0) {
							result = CMD_RESULT_FAILED;
							m_terminal->write_ro(RODT_ATTR("\nFailed to create file: "));
							m_terminal->write(filename);
							m_terminal->write_ro(RODT_ATTR(" : "));
							m_terminal->write(bStatus);
						}
						delete[] filename;	
					}
				}
			}else if( fsq == FS_COMMAND_QUERY_MAKE_DIR ){
				CommandOption *cmdoptn = RetrieveOption(CMD_OPTION_NAME_V);
				if( nullptr != cmdoptn ){
					char *dirname = new char[cmdoptn->optionvalsize+__i_fs.pwd()->size()+2]();
					if( nullptr != dirname ){
						memset(dirname, 0, cmdoptn->optionvalsize+__i_fs.pwd()->size()+2);
						memcpy(dirname, __i_fs.pwd()->c_str(), __i_fs.pwd()->size());
						__i_fs.appendFileSeparator(dirname);
						strncat(dirname, cmdoptn->optionval, cmdoptn->optionvalsize);
						int bStatus = __i_fs.createDirectory(dirname);
						if (bStatus < 0) {
							result = CMD_RESULT_FAILED;
							m_terminal->write_ro(RODT_ATTR("\nFailed to create directory: "));
							m_terminal->write(dirname);
							m_terminal->write_ro(RODT_ATTR(" : "));
							m_terminal->write(bStatus);
						}
						delete[] dirname;	
					}
				}
			}

		}

		// LogI("\n");
		return result;
	}
};
#endif


#endif
