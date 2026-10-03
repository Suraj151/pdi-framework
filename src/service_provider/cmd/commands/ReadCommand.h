/******************************** Read Command ********************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 27th Sep 2026
******************************************************************************/
#ifndef _READ_COMMAND_H_
#define _READ_COMMAND_H_

#include "CommandCommon.h"
#include <service_provider/session/Environment.h>

#ifdef ENABLE_STORAGE_SERVICE

struct ReadCommand : public CommandBase {

	ReadCommand(){
		Clear();
		SetCommand(CMD_NAME_READ);
		setAcceptArgsOptions(true);
		setCmdOptionSeparator(CMD_OPTION_SEPERATOR_SPACE);
	}

	static void RegisterCommand(){
		CommandBase::RegisterCommand(CMD_NAME_READ, [](void *arg)->void *{
			return pdiutil::safe_new<ReadCommand>();
		});
	}

	const char* getUsage() const override {
		return RODT_ATTR("read <name> [<name>..]  take a line of input into variables");
	}

#ifdef ENABLE_AUTH_SERVICE
	bool needauth() override { return true; }
#endif

	pdi_err_t execute(cmd_term_inseq_t terminputaction){

#ifdef ENABLE_AUTH_SERVICE
		if( needauth() && !__auth_service.getAuthorized() ){
			return CMD_ERROR_PERM;
		}
#endif

		if( nullptr == m_terminal ){
			return CMD_ERROR_NOTTY;
		}

		int16_t namelen[CMD_OPTION_MAX] = {0};
		uint8_t namecount = 0;

		while( namecount < CMD_OPTION_MAX && nullptr != m_options[namecount].optionval &&
			   0 < m_options[namecount].optionvalsize ){

			const char *arg = m_options[namecount].optionval;
			int16_t len = 0;
			while( len < m_options[namecount].optionvalsize && '\0' != arg[len] ) len++;

			if( 0 == len ){
				break;
			}

			namelen[namecount] = len;
			namecount++;
		}

		if( 0 == namecount ){
			m_terminal->putln();
			m_terminal->writeln_ro(RODT_ATTR("give it a name to read into"));
			return CMD_ERROR_ARGS_MISSING;
		}

		if( !isInputRedirected() ){
			m_terminal->putln();
			m_terminal->writeln_ro(RODT_ATTR("nothing is feeding it, give it a pipe or a file"));
			return CMD_ERROR_ARGS_MISSING;
		}

		pdiutil::string line;
		bool sawbyte = false;

		line.reserve(ENV_VALUE_MAX + 1);

		while( m_terminal->available() > 0 ){

			uint8_t c = m_terminal->read();
			sawbyte = true;

			if( '\n' == c ){
				break;
			}

			if( line.size() < ENV_VALUE_MAX ){
				line += (char)c;
			}
		}

		if( !sawbyte ){
			return CMD_RESULT_FALSE;
		}

		if( !line.empty() && '\r' == line.back() ){
			line.pop_back();
		}

		pdi_err_t res = PDI_OK;
		pdiutil::string::size_type at = 0;

		for( uint8_t i = 0; i < namecount; i++ ){

			pdiutil::string name(m_options[i].optionval, (pdiutil::string::size_type)namelen[i]);
			pdiutil::string value;

			while( at < line.size() && __is_blank(line[at]) ) at++;

			if( at < line.size() ){

				pdiutil::string::size_type end = line.size();

				if( (i + 1) < namecount ){
					end = at;
					while( end < line.size() && !__is_blank(line[end]) ) end++;
				}else{
					while( end > at && __is_blank(line[end - 1]) ) end--;
				}

				value = line.substr(at, end - at);
				at = end;
			}

			pdi_err_t set = Environment::set(name.c_str(), value.c_str());

			if( PDI_OK != set ){
				res = set;
			}
		}

		if( CMD_ERROR_PERM == res ){
			m_terminal->putln();
			m_terminal->writeln_ro(RODT_ATTR("the session answers for that name, it cannot be set"));
		}else if( CMD_ERROR_INVAL == res ){
			m_terminal->putln();
			m_terminal->writeln_ro(RODT_ATTR("a name is a letter or underscore, then letters, digits or underscores"));
		}else if( PDI_OK != res ){
			m_terminal->putln();
			m_terminal->writeln_ro(RODT_ATTR("this session holds no more variables"));
		}

		return res;
	}
};

#endif

#endif
