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
#include <helpers/ProcHelper.h>

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
		setArgs(RODT_ATTR(""), 1);
		setAcceptArgsOptions(true);
		setCmdOptionSeparator(CMD_OPTION_SEPERATOR_SPACE);
	}

	static void RegisterCommand(){
		CommandBase::RegisterCommand(CMD_NAME_PS, [](void *arg)->void *{
			return pdiutil::safe_new<PsCommand>();
		});
	}

	const char* getUsage() const override {
		return RODT_ATTR("ps [<sid>]  list scheduler tasks (POSIX-style; filter by owner sid)");
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
			return CMD_ERROR_FAILED;
		}

		uint8_t filter_owner = 0xFF;
		CommandOption *ucmdoptn = &m_options[0];
		if( optionGiven(ucmdoptn) ){
			uint32_t sid = 0;
			pdi_err_t taken = takeNumber(RODT_ATTR("invalid sid : "), ucmdoptn->optionval, ucmdoptn->optionvalsize, 0, 0xFE, sid);
			if( PDI_OK != taken ){
				return taken;
			}
			filter_owner = (uint8_t)sid;
		}

		printProcessTable(m_terminal, filter_owner, false);
		return PDI_OK;
	}
};

#endif
