/******************************** Wc Command **********************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 28th May 2026
******************************************************************************/

#ifndef _WC_FS_COMMAND_H_
#define _WC_FS_COMMAND_H_

#include "CommandCommon.h"

#ifdef ENABLE_STORAGE_SERVICE

struct WcFSCommand : public CommandBase {

	WcFSCommand(){
		Clear();
		SetCommand(CMD_NAME_WC);
		setAcceptArgsOptions(true);
	}

	static void RegisterCommand(){
		CommandBase::RegisterCommand(CMD_NAME_WC, [](void *arg)->void *{
			return new WcFSCommand();
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
			CommandOption *cmdoptn = &m_options[0];
			if( nullptr != cmdoptn && nullptr != cmdoptn->optionval && cmdoptn->optionvalsize > 0 ){
				char *filename = new char[cmdoptn->optionvalsize + __i_fs.getPWD().size() + 2]();
				if( nullptr != filename ){
					memcpy(filename, __i_fs.getPWD().c_str(), __i_fs.getPWD().size());
					__i_fs.appendFileSeparator(filename);
					strncat(filename, cmdoptn->optionval, cmdoptn->optionvalsize);

					uint32_t bytes = 0;
					uint32_t lines = 0;
					uint32_t words = 0;
					bool in_word = false;

					int iStatus = __i_fs.readFile(filename, 250, [&](char *data, uint32_t size)->bool{
						for(uint32_t i = 0; i < size; i++){
							bytes++;
							char c = data[i];
							if(c == '\n') lines++;
							bool ws = (c == ' ' || c == '\t' || c == '\n' || c == '\r');
							if(!ws){
								if(!in_word){
									words++;
									in_word = true;
								}
							}else{
								in_word = false;
							}
						}
						return true;
					});

					m_terminal->putln();

					if(iStatus < 0){
						result = CMD_RESULT_FAILED;
						m_terminal->write_ro(RODT_ATTR("Failed : "));
						m_terminal->write(filename);
						m_terminal->write_ro(RODT_ATTR(" : "));
						m_terminal->write((int32_t)iStatus);
					}else{
						m_terminal->write((int32_t)lines);
						m_terminal->write(' ');
						m_terminal->write((int32_t)words);
						m_terminal->write(' ');
						m_terminal->write((int32_t)bytes);
						m_terminal->write(' ');
						m_terminal->write(filename);
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
