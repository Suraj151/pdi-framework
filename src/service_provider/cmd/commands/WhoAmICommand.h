/******************************** WhoAmI Command ******************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 18th July 2026
******************************************************************************/
#ifndef _WHOAMI_COMMAND_H_
#define _WHOAMI_COMMAND_H_

#include "CommandCommon.h"

#ifdef ENABLE_AUTH_SERVICE

struct WhoAmICommand : public CommandBase {

	WhoAmICommand(){
		Clear();
		SetCommand(CMD_NAME_WHOAMI);
		setArgs(RODT_ATTR(""), 0);
	}

	static void RegisterCommand(){
		CommandBase::RegisterCommand(CMD_NAME_WHOAMI, [](void *arg)->void *{
			return pdiutil::safe_new<WhoAmICommand>();
		});
	}

	const char* getUsage() const override {
		return RODT_ATTR("whoami  print the current session's username");
	}

	bool needauth() override { return true; }

	pdi_err_t execute(cmd_term_inseq_t terminputaction){

		if( needauth() && !__auth_service.getAuthorized() ){
			return CMD_ERROR_PERM;
		}

		if( nullptr != m_terminal ){
			m_terminal->putln();
			const char *u = __auth_service.getUsername();
			if( nullptr != u && 0 != u[0] ){
				m_terminal->write(u);
			}
		}
		return PDI_OK;
	}
};

#endif

#endif
