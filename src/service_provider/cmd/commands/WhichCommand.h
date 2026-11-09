/******************************** Which Command *******************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Tells whether a command exists, for scripts to test before running one. which
prints the name of each command it finds and nothing for one it does not; type
says what each name is, or that it was not found. Either succeeds only when
every name was found.

Author          : Suraj I.
created Date    : 7th Nov 2026
******************************************************************************/
#ifndef _WHICH_COMMAND_H_
#define _WHICH_COMMAND_H_

#include "CommandCommon.h"

/**
 * Whether the shell would run a command by this name: one word that is in the
 * command list.
 */
static inline bool isRunnableCommand(const CommandBase::CommandOption *opt, pdiutil::string &name){
	name = pdiutil::string(opt->optionval, (size_t)opt->optionvalsize);
	return !name.empty() && pdiutil::string::npos == name.find(' ') && CommandBase::IsCommandRegistered(name.c_str());
}

struct WhichCommand : public CommandBase {

	WhichCommand(){
		Clear();
		SetCommand(CMD_NAME_WHICH);
		setArgs(RODT_ATTR(""), CMD_OPTION_MAX - 1);
		setAcceptArgsOptions(true);
		setCmdOptionSeparator(CMD_OPTION_SEPERATOR_SPACE);
	}

	static void RegisterCommand(){
		CommandBase::RegisterCommand(CMD_NAME_WHICH, [](void *arg)->void *{
			return pdiutil::safe_new<WhichCommand>();
		});
	}

	const char* getUsage() const override {
		return RODT_ATTR("which <name>...  print each name that is a command; fails if any is not");
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

		bool any = false;
		bool all = true;
		pdiutil::string name;
		for( uint8_t i = 0; i < CMD_OPTION_MAX; i++ ){
			if( !optionGiven(&m_options[i]) ){
				continue;
			}
			any = true;
			if( !isRunnableCommand(&m_options[i], name) ){
				all = false;
				continue;
			}
			m_terminal->putln();
			m_terminal->write(name.c_str());
		}
		if( !any ){
			return CMD_ERROR_ARGS_MISSING;
		}
		return all ? PDI_OK : CMD_RESULT_FALSE;
	}
};

struct TypeCommand : public CommandBase {

	TypeCommand(){
		Clear();
		SetCommand(CMD_NAME_TYPE);
		setArgs(RODT_ATTR(""), CMD_OPTION_MAX - 1);
		setAcceptArgsOptions(true);
		setCmdOptionSeparator(CMD_OPTION_SEPERATOR_SPACE);
	}

	static void RegisterCommand(){
		CommandBase::RegisterCommand(CMD_NAME_TYPE, [](void *arg)->void *{
			return pdiutil::safe_new<TypeCommand>();
		});
	}

	const char* getUsage() const override {
		return RODT_ATTR("type <name>...  say whether each name is a command; fails if any is not");
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

		bool any = false;
		bool all = true;
		pdiutil::string name;
		for( uint8_t i = 0; i < CMD_OPTION_MAX; i++ ){
			if( !optionGiven(&m_options[i]) ){
				continue;
			}
			any = true;
			bool found = isRunnableCommand(&m_options[i], name);
			m_terminal->putln();
			if( found ){
				m_terminal->write(name.c_str());
				m_terminal->write_ro(RODT_ATTR(" is a shell builtin"));
			}else{
				m_terminal->write_ro(RODT_ATTR("type: "));
				writeName(name.c_str());
				m_terminal->write_ro(RODT_ATTR(": not found"));
				all = false;
			}
		}
		if( !any ){
			return CMD_ERROR_ARGS_MISSING;
		}
		return all ? PDI_OK : CMD_RESULT_FALSE;
	}
};

#endif
