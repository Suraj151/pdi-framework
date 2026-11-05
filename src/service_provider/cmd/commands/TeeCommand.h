/********************************* Tee Command ********************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st Nov 2026
******************************************************************************/

#ifndef _TEE_COMMAND_H_
#define _TEE_COMMAND_H_

#include "CommandCommon.h"
#include <service_provider/session/FileWriteStream.h>

#ifdef ENABLE_STORAGE_SERVICE

struct TeeCommand : public CommandBase {

	TeeCommand(){
		Clear();
		SetCommand(CMD_NAME_TEE);
		setArgs(RODT_ATTR("a"), 255);
		setAcceptArgsOptions(true);
		setCmdOptionSeparator(CMD_OPTION_SEPERATOR_SPACE);
	}

	static void RegisterCommand(){
		CommandBase::RegisterCommand(CMD_NAME_TEE, [](void *arg)->void *{
			return pdiutil::safe_new<TeeCommand>();
		});
	}

	const char* getUsage() const override {
		return RODT_ATTR("tee [-a] [<file>...]  copy input to the output and to each file (-a append)");
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

		if( nullptr == m_terminal ){
			return PDI_OK;
		}

		bool append = false;
		pdiutil::vector<pdiutil::string> names;

		for( uint8_t i = 0; i < CMD_OPTION_MAX; i++ ){

			CommandOption *opt = &m_options[i];
			if( !optionGiven(opt) ){
				break;
			}

			if( opt->optionvalsize > 1 && '-' == opt->optionval[0] ){
				append = true;
			}else{
				names.push_back(pdiutil::string(opt->optionval, (pdiutil::string::size_type)opt->optionvalsize));
			}
		}

		m_terminal->putln();

		pdi_err_t result = PDI_OK;
		pdiutil::vector<FileWriteStream*> sinks;

		for( uint16_t i = 0; i < names.size(); i++ ){
			pdiutil::string path = resolveArgPathStr(names[i].c_str(), (int16_t)names[i].size());
			FileWriteStream *sink = nullptr;
			if( __i_fs.isDirectory(path.c_str()) ){
				writeFailure(RODT_ATTR("tee: "), names[i].c_str(), STORAGE_ERROR_NOT_A_FILE);
				m_terminal->putln();
				result = CMD_ERROR_FAILED;
			}else{
				sink = pdiutil::safe_new<FileWriteStream>(path.c_str(), append);
				if( nullptr == sink || !sink->isValid() ){
					pdiutil::safe_delete(sink);
					writeFailure(RODT_ATTR("tee: "), names[i].c_str(), PDI_ERR_NO_MEM);
					m_terminal->putln();
					result = CMD_ERROR_FAILED;
				}
			}
			sinks.push_back(sink);
		}

		char previous = '\n';
		if( isInputRedirected() ){
			session_t *session = SessionManager::current();
			bool onterminal = ( nullptr != session && m_terminal == session->m_terminal );

			readCommandInput(pdiutil::string(), m_terminal, [&](char *data, uint32_t size)->bool{
				if( onterminal ){
					writeShownBytes(m_terminal, data, size, previous);
				}else{
					m_terminal->writeData((const uint8_t *)data, size);
					previous = ( size > 0 ) ? data[size - 1] : previous;
				}
				for( uint16_t i = 0; i < sinks.size(); i++ ){
					if( nullptr != sinks[i] ){
						sinks[i]->write((const uint8_t *)data, size);
					}
				}
				return true;
			});
		}

		for( uint16_t i = 0; i < sinks.size(); i++ ){
			if( nullptr == sinks[i] ){
				continue;
			}
			int32_t closed = sinks[i]->disconnect();
			if( closed < 0 ){
				if( '\n' != previous ){
					m_terminal->putln();
				}
				writeFailure(RODT_ATTR("tee: "), names[i].c_str(), closed);
				previous = '\0';
				result = CMD_ERROR_FAILED;
			}
			pdiutil::safe_delete(sinks[i]);
		}

		return stoppedResult(result);
	}
};

#endif

#endif
