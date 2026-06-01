/******************************** Uptime Command ******************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 28th May 2026
******************************************************************************/

#ifndef _UPTIME_COMMAND_H_
#define _UPTIME_COMMAND_H_

#include "CommandCommon.h"

struct UptimeCommand : public CommandBase {

	UptimeCommand(){
		Clear();
		SetCommand(CMD_NAME_UPTIME);
	}

	static void RegisterCommand(){
		CommandBase::RegisterCommand(CMD_NAME_UPTIME, [](void *arg)->void *{
			return new UptimeCommand();
		});
	}

	cmd_result_t execute(cmd_term_inseq_t terminputaction){

		if(nullptr != m_terminal){
			uint32_t ms = __i_dvc_ctrl.millis_now();
			uint32_t days = ms / 86400000UL;
			uint32_t hours = (ms / 3600000UL) % 24;
			uint32_t mins = (ms / 60000UL) % 60;
			uint32_t secs = (ms / 1000UL) % 60;

			m_terminal->putln();
			m_terminal->write_ro(RODT_ATTR("up "));
			m_terminal->write((int32_t)days);
			m_terminal->write_ro(RODT_ATTR("d "));
			m_terminal->write((int32_t)hours);
			m_terminal->write_ro(RODT_ATTR("h "));
			m_terminal->write((int32_t)mins);
			m_terminal->write_ro(RODT_ATTR("m "));
			m_terminal->write((int32_t)secs);
			m_terminal->write_ro(RODT_ATTR("s"));
		}

		return CMD_RESULT_OK;
	}
};

#endif
