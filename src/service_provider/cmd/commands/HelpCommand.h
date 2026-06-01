/******************************** Help Command ********************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 28th May 2026
******************************************************************************/

#ifndef _HELP_COMMAND_H_
#define _HELP_COMMAND_H_

#include "CommandCommon.h"

struct HelpCommand : public CommandBase {

	HelpCommand(){
		Clear();
		SetCommand(CMD_NAME_HELP);
	}

	static void RegisterCommand(){
		CommandBase::RegisterCommand(CMD_NAME_HELP, [](void *arg)->void *{
			return new HelpCommand();
		});
	}

	cmd_result_t execute(cmd_term_inseq_t terminputaction){

		if(nullptr != m_terminal){
			m_terminal->putln();
			m_terminal->write_ro(RODT_ATTR("Registered commands ("));
			m_terminal->write((int32_t)CommandBase::m_cmd_registry.size());
			m_terminal->writeln_ro(RODT_ATTR("):"));

			for (uint16_t i = 0; i < CommandBase::m_cmd_registry.size(); i++){
				m_terminal->write_ro(RODT_ATTR("  "));
				m_terminal->writeln(CommandBase::m_cmd_registry[i].cmdname);
			}
		}

		return CMD_RESULT_OK;
	}
};

#endif
