/*********************************** Ps Command *******************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 19th July 2026
******************************************************************************/

#ifndef _PS_COMMAND_H_
#define _PS_COMMAND_H_

#include "CommandCommon.h"

/**
 * ps command — snapshot of scheduler tasks.
 *
 * e.g.
 *   ps            list all tasks owned by any session (including kernel)
 *   ps <sid>    filter by owner session id
 */
struct PsCommand : public CommandBase {

	PsCommand(){
		Clear();
		SetCommand(CMD_NAME_PS);
		setAcceptArgsOptions(true);
		setCmdOptionSeparator(CMD_OPTION_SEPERATOR_SPACE);
	}

	static void RegisterCommand(){
		CommandBase::RegisterCommand(CMD_NAME_PS, [](void *arg)->void *{
			return new PsCommand();
		});
	}

	const char* getUsage() const override {
		return RODT_ATTR("ps [<sid>]  list scheduler tasks (POSIX-style; filter by owner sid)");
	}

#ifdef ENABLE_AUTH_SERVICE
	bool needauth() override { return true; }
#endif

	cmd_result_t execute(cmd_term_inseq_t terminputaction){

#ifdef ENABLE_AUTH_SERVICE
		if( needauth() && !__auth_service.getAuthorized() ){
			return CMD_RESULT_NEED_AUTH;
		}
#endif

		if( nullptr == m_terminal ){
			return CMD_RESULT_FAILED;
		}

		uint8_t filter_owner = 0xFF;
		CommandOption *ucmdoptn = &m_options[0];
		if( nullptr != ucmdoptn && nullptr != ucmdoptn->optionval && ucmdoptn->optionvalsize > 0 ){
			filter_owner = (uint8_t)StringToUint16(ucmdoptn->optionval, ucmdoptn->optionvalsize);
		}

		__task_scheduler.printPsToTerminal(m_terminal, filter_owner);
		return CMD_RESULT_OK;
	}
};

#endif
