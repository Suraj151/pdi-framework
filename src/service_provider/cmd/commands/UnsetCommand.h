/******************************** Unset Command *******************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 5th Sep 2026
******************************************************************************/
#ifndef _UNSET_COMMAND_H_
#define _UNSET_COMMAND_H_

#include "CommandCommon.h"
#include <service_provider/session/Environment.h>

struct UnsetCommand : public CommandBase {

	UnsetCommand(){
		Clear();
		SetCommand(CMD_NAME_UNSET);
		setArgs(RODT_ATTR(""), 1);
		setAcceptArgsOptions(true);
	}

	static void RegisterCommand(){
		CommandBase::RegisterCommand(CMD_NAME_UNSET, [](void *arg)->void *{
			return pdiutil::safe_new<UnsetCommand>();
		});
	}

	const char* getUsage() const override {
		return RODT_ATTR("unset <name>  drop a variable from this session");
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

		CommandOption *cmdoptn = &m_options[0];
		if( nullptr == cmdoptn->optionval || 0 >= cmdoptn->optionvalsize ){
			m_terminal->putln();
			m_terminal->writeln_ro(RODT_ATTR("give it a name"));
			return CMD_ERROR_ARGS_MISSING;
		}

		pdiutil::string name(cmdoptn->optionval, cmdoptn->optionvalsize);

		if( Environment::isDerived(name.c_str()) ){
			m_terminal->putln();
			m_terminal->writeln_ro(RODT_ATTR("the session answers for that name, it cannot be dropped"));
			return CMD_ERROR_PERM;
		}

		if( !Environment::unset(name.c_str()) ){
			m_terminal->putln();
			m_terminal->writeln_ro(RODT_ATTR("this session does not carry that name"));
			return CMD_ERROR_FAILED;
		}

		return PDI_OK;
	}
};

#endif
