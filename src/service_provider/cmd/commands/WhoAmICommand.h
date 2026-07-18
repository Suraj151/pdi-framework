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
	}

	static void RegisterCommand(){
		CommandBase::RegisterCommand(CMD_NAME_WHOAMI, [](void *arg)->void *{
			return new WhoAmICommand();
		});
	}

	bool needauth() override { return true; }

	cmd_result_t execute(cmd_term_inseq_t terminputaction){

		if( needauth() && !__auth_service.getAuthorized() ){
			return CMD_RESULT_NEED_AUTH;
		}

		if( nullptr != m_terminal ){
			m_terminal->putln();
			const char *u = __auth_service.getUsername();
			if( nullptr != u && 0 != u[0] ){
				m_terminal->write(u);
			}
		}
		return CMD_RESULT_OK;
	}
};

#endif

#endif
