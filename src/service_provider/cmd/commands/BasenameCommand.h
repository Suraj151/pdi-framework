/****************************** Basename Command ******************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Splits a path on the text alone without
looking at any file: basename prints the last part, less a suffix when one is
given and the name is longer than it; dirname prints everything before it, "."
when there is no directory part and "/" for the root.

Author          : Suraj I.
created Date    : 7th Nov 2026
******************************************************************************/
#ifndef _BASENAME_COMMAND_H_
#define _BASENAME_COMMAND_H_

#include "CommandCommon.h"

struct BasenameCommand : public CommandBase {

	BasenameCommand(){
		Clear();
		SetCommand(CMD_NAME_BASENAME);
		setArgs(RODT_ATTR(""), 2);
		setAcceptArgsOptions(true);
		setCmdOptionSeparator(CMD_OPTION_SEPERATOR_SPACE);
	}

	static void RegisterCommand(){
		CommandBase::RegisterCommand(CMD_NAME_BASENAME, [](void *arg)->void *{
			return pdiutil::safe_new<BasenameCommand>();
		});
	}

	const char* getUsage() const override {
		return RODT_ATTR("basename <path> [suffix]  last part of a path, less the suffix");
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
		if( !optionGiven(&m_options[0]) ){
			return CMD_ERROR_ARGS_MISSING;
		}

		pdiutil::string name(m_options[0].optionval, (size_t)m_options[0].optionvalsize);
		__strip_trailing(name, FILE_SEPARATOR[0]);
		if( name.size() > 1 ){
			pdiutil::string::size_type slash = name.find_last_of(FILE_SEPARATOR);
			if( pdiutil::string::npos != slash ){
				name = name.substr(slash + 1);
			}
			if( optionGiven(&m_options[1]) ){
				pdiutil::string suffix(m_options[1].optionval, (size_t)m_options[1].optionvalsize);
				if( !suffix.empty() && name.size() > suffix.size() &&
					0 == memcmp(name.c_str() + name.size() - suffix.size(), suffix.c_str(), suffix.size()) ){
					name = name.substr(0, name.size() - suffix.size());
				}
			}
		}

		m_terminal->putln();
		m_terminal->write(name.c_str());
		return PDI_OK;
	}
};

struct DirnameCommand : public CommandBase {

	DirnameCommand(){
		Clear();
		SetCommand(CMD_NAME_DIRNAME);
		setArgs(RODT_ATTR(""), 1);
		setAcceptArgsOptions(true);
		setCmdOptionSeparator(CMD_OPTION_SEPERATOR_SPACE);
	}

	static void RegisterCommand(){
		CommandBase::RegisterCommand(CMD_NAME_DIRNAME, [](void *arg)->void *{
			return pdiutil::safe_new<DirnameCommand>();
		});
	}

	const char* getUsage() const override {
		return RODT_ATTR("dirname <path>  the path less its last part");
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
		if( !optionGiven(&m_options[0]) ){
			return CMD_ERROR_ARGS_MISSING;
		}

		pdiutil::string path(m_options[0].optionval, (size_t)m_options[0].optionvalsize);
		__strip_trailing(path, FILE_SEPARATOR[0]);
		pdiutil::string::size_type slash = path.find_last_of(FILE_SEPARATOR);
		if( pdiutil::string::npos == slash ){
			path = ".";
		}else if( path.size() > 1 ){
			path = path.substr(0, slash);
			if( path.empty() ){
				path += FILE_SEPARATOR;
			}
			__strip_trailing(path, FILE_SEPARATOR[0]);
		}

		m_terminal->putln();
		m_terminal->write(path.c_str());
		return PDI_OK;
	}
};

#endif
