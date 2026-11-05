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
		setArgs(RODT_ATTR(""), 2);
		setAcceptArgsOptions(true);
		setCmdOptionSeparator(CMD_OPTION_SEPERATOR_SPACE);
	}

	static void RegisterCommand(){
		CommandBase::RegisterCommand(CMD_NAME_HEAD, [](void *arg)->void *{
			return pdiutil::safe_new<HeadFSCommand>();
		});
	}

	const char* getUsage() const override {
		return RODT_ATTR("head [<file>] [N]  print first N lines of a file or the piped input (default 10)");
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
				if( !takeCount(streamcountoptn, count) ){
					return CMD_ERROR_INVAL;
				}

				uint32_t printed = 0;
				m_terminal->putln();

				readCommandLines(pdiutil::string(), m_terminal, [&](const pdiutil::string &line)->bool{
					if( printed >= count ){
						return false;
					}
					m_terminal->write(line.c_str());
					m_terminal->putln();
					printed++;
					return printed < count;
				});
			}else if(nullptr != fileoptn && nullptr != fileoptn->optionval && fileoptn->optionvalsize > 0){

				uint32_t count = 10;
				if( !takeCount(countoptn, count) ){
					return CMD_ERROR_INVAL;
				}

				pdiutil::string filepath = resolveArgPath(fileoptn);
				if(!filepath.empty()){
					const char* filename = filepath.c_str();

					m_terminal->putln();

					if(__i_fs.isDirectory(filename)){
						result = CMD_ERROR_FAILED;
						writeFailure(RODT_ATTR(CMD_MSG_FAILED), filename, (int32_t)STORAGE_ERROR_NOT_A_FILE);
					}else if(!__i_fs.isFileExist(filename)){
						result = CMD_ERROR_FAILED;
						m_terminal->write_ro(RODT_ATTR(CMD_MSG_NOT_FOUND));
						writeName(filename);
					}else{
						int64_t fs = __i_fs.getFileSize(filename);
						if(fs > 0){
							uint64_t offset = 0;
							uint32_t printed = 0;
							while(offset < (uint64_t)fs && printed < count && !commandStopped(m_terminal)){
								int bytes = writeFileLine(m_terminal, filename, offset);
								if(bytes < 0) break;

								offset += (uint64_t)bytes + 1;
								printed++;
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

private:

	/**
	 * Reads the line count given, if one was; false, after saying why, when it is
	 * not a whole number that fits.
	 */
	bool takeCount(const CommandOption *opt, uint32_t &count){
		if( nullptr == opt || nullptr == opt->optionval || opt->optionvalsize <= 0 ){
			return true;
		}
		if( ParseUint32(opt->optionval, (uint16_t)opt->optionvalsize, count) ){
			return true;
		}
		m_terminal->putln();
		m_terminal->write_ro(RODT_ATTR("invalid count : "));
		m_terminal->write(opt->optionval, (uint32_t)opt->optionvalsize);
		return false;
	}
};

#endif

#endif
