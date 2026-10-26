/******************************** Tail Command ********************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 28th May 2026
******************************************************************************/

#ifndef _TAIL_FS_COMMAND_H_
#define _TAIL_FS_COMMAND_H_

#include "CommandCommon.h"

#ifdef ENABLE_STORAGE_SERVICE

struct TailFSCommand : public CommandBase {

	TailFSCommand(){
		Clear();
		SetCommand(CMD_NAME_TAIL);
		setAcceptArgsOptions(true);
		setCmdOptionSeparator(CMD_OPTION_SEPERATOR_SPACE);
	}

	static void RegisterCommand(){
		CommandBase::RegisterCommand(CMD_NAME_TAIL, [](void *arg)->void *{
			return pdiutil::safe_new<TailFSCommand>();
		});
	}

	const char* getUsage() const override {
		return RODT_ATTR("tail <file> [N]  print last N lines (default 10)");
	}

#ifdef ENABLE_AUTH_SERVICE
	bool needauth() override { return true; }
#endif

	pdi_err_t execute(cmd_term_inseq_t terminputaction){

#ifdef ENABLE_AUTH_SERVICE
		if( needauth() && !__auth_service.getAuthorized()){
			return CMD_ERROR_PERM;
		}
#endif

		pdi_err_t result = PDI_OK;

		if(nullptr != m_terminal){
			CommandOption *fileoptn = &m_options[0];
			CommandOption *countoptn = &m_options[1];

			// a piped stage has no file to name, so its lone positional is the
			// count rather than a path
			if( isInputRedirected() ){

				CommandOption *streamcountoptn = ( nullptr != fileoptn && nullptr != fileoptn->optionval &&
												   fileoptn->optionvalsize > 0 ) ? fileoptn : countoptn;

				uint32_t count = 10;
				if(nullptr != streamcountoptn && nullptr != streamcountoptn->optionval && streamcountoptn->optionvalsize > 0){
					uint32_t parsed = StringToUint32(streamcountoptn->optionval, streamcountoptn->optionvalsize);
					if(parsed > 0) count = parsed;
				}

				// a stream cannot be rewound, so the last lines are the only
				// ones worth keeping while it drains
				if(count > PDI_STREAM_TAIL_LINES_MAX) count = PDI_STREAM_TAIL_LINES_MAX;

				pdiutil::vector<pdiutil::string> kept;

				readCommandLines(pdiutil::string(), m_terminal, [&](const pdiutil::string &line)->bool{
					if(kept.size() == (size_t)count){
						kept.erase(kept.begin());
					}
					kept.push_back(line);
					return true;
				});

				m_terminal->putln();

				for(size_t i = 0; i < kept.size(); i++){
					m_terminal->write(kept[i].c_str());
					m_terminal->putln();
					__i_dvc_ctrl.yield();
				}
			}else if(nullptr != fileoptn && nullptr != fileoptn->optionval && fileoptn->optionvalsize > 0){

				uint32_t count = 10;
				if(nullptr != countoptn && nullptr != countoptn->optionval && countoptn->optionvalsize > 0){
					uint32_t parsed = StringToUint32(countoptn->optionval, countoptn->optionvalsize);
					if(parsed > 0) count = parsed;
				}

				pdiutil::string filepath = resolveArgPath(fileoptn);
				if(!filepath.empty()){
					const char* filename = filepath.c_str();

					m_terminal->putln();

					if(!__i_fs.isFileExist(filename)){
						result = CMD_ERROR_FAILED;
						m_terminal->write_ro(RODT_ATTR("Not found : "));
						m_terminal->write(filename);
					}else{
						int64_t fs = __i_fs.getFileSize(filename);
						if(fs > 0){
							int64_t startOffset = __i_fs.getOffsetFromLineNumber(filename, -(int32_t)count, [&](void){
								__i_dvc_ctrl.yield();
							});
							if(startOffset < 0) startOffset = 0;

							uint64_t offset = (uint64_t)startOffset;
							pdiutil::string linedata;
							while(offset < (uint64_t)fs && !commandStopped(m_terminal)){
								linedata.clear();
								int bytes = __i_fs.readFile(filename, 200, [&](char *data, uint32_t size) -> bool {
									linedata += pdiutil::string(data, size);
									return !commandStopped(m_terminal);
								}, offset, "\n");
								if(bytes < 0) break;

								if(!linedata.empty() && linedata.back() == '\r'){
									linedata.pop_back();
								}

								m_terminal->write(linedata.c_str());
								m_terminal->putln();

								offset += (uint64_t)bytes + 1;
								__i_dvc_ctrl.yield();
							}
						}
					}
				}
			}else{
				result = CMD_ERROR_ARGS_MISSING;
			}
		}

		return stoppedResult(result);
	}
};

#endif

#endif
