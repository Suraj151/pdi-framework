/******************************** Head Command ********************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 28th May 2026
******************************************************************************/

#ifndef _HEAD_FS_COMMAND_H_
#define _HEAD_FS_COMMAND_H_

#include "CommandCommon.h"

#ifdef ENABLE_STORAGE_SERVICE

struct HeadFSCommand : public CommandBase {

	HeadFSCommand(){
		Clear();
		SetCommand(CMD_NAME_HEAD);
		setAcceptArgsOptions(true);
		setCmdOptionSeparator(CMD_OPTION_SEPERATOR_SPACE);
	}

	static void RegisterCommand(){
		CommandBase::RegisterCommand(CMD_NAME_HEAD, [](void *arg)->void *{
			return new HeadFSCommand();
		});
	}

#ifdef ENABLE_AUTH_SERVICE
	bool needauth() override { return true; }
#endif

	cmd_result_t execute(cmd_term_inseq_t terminputaction){

#ifdef ENABLE_AUTH_SERVICE
		if( needauth() && !__auth_service.getAuthorized()){
			return CMD_RESULT_NEED_AUTH;
		}
#endif

		cmd_result_t result = CMD_RESULT_OK;

		if(nullptr != m_terminal){
			CommandOption *fileoptn = &m_options[0];
			CommandOption *countoptn = &m_options[1];

			if(nullptr != fileoptn && nullptr != fileoptn->optionval && fileoptn->optionvalsize > 0){

				uint32_t count = 10;
				if(nullptr != countoptn && nullptr != countoptn->optionval && countoptn->optionvalsize > 0){
					uint32_t parsed = StringToUint32(countoptn->optionval, countoptn->optionvalsize);
					if(parsed > 0) count = parsed;
				}

				char *filename = new char[fileoptn->optionvalsize + SessionManager::getPWD().size() + 2]();
				if(nullptr != filename){
					memcpy(filename, SessionManager::getPWD().c_str(), SessionManager::getPWD().size());
					__i_fs.appendFileSeparator(filename);
					strncat(filename, fileoptn->optionval, fileoptn->optionvalsize);

					m_terminal->putln();

					if(!__i_fs.isFileExist(filename)){
						result = CMD_RESULT_FAILED;
						m_terminal->write_ro(RODT_ATTR("Not found : "));
						m_terminal->write(filename);
					}else{
						int64_t fs = __i_fs.getFileSize(filename);
						if(fs > 0){
							uint64_t offset = 0;
							uint32_t printed = 0;
							pdiutil::string linedata;
							while(offset < (uint64_t)fs && printed < count){
								linedata.clear();
								int bytes = __i_fs.readFile(filename, 200, [&](char *data, uint32_t size) -> bool {
									linedata += pdiutil::string(data, size);
									return true;
								}, offset, "\n");
								if(bytes < 0) break;

								if(!linedata.empty() && linedata.back() == '\r'){
									linedata.pop_back();
								}

								m_terminal->write(linedata.c_str());
								m_terminal->putln();

								offset += (uint64_t)bytes + 1;
								printed++;
								__i_dvc_ctrl.yield();
							}
						}
					}

					delete[] filename;
				}
			}else{
				result = CMD_RESULT_ARGS_MISSING;
			}
		}

		return result;
	}
};

#endif

#endif
