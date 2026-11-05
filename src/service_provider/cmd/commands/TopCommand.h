/********************************** Top Command *******************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 19th July 2026
******************************************************************************/

#ifndef _TOP_COMMAND_H_
#define _TOP_COMMAND_H_

#include "CommandCommon.h"
#include <helpers/ProcHelper.h>

/**
 * top command — periodically refreshed ps view.
 *
 * e.g.
 *   top                     refresh every 2s, forever (until stopRunningInBackground)
 *   top i=1000              refresh every 1s
 *   top i=2000,n=5          5 refreshes, then stop
 *   top u=1                 filter by owner session id 1
 */
struct TopCommand : public CommandBase {

	int32_t m_toptaskid = -1;
	uint8_t m_filter_owner = 0xFF;

	TopCommand(){
		Clear();
		SetCommand(CMD_NAME_TOP);
		setArgs(RODT_ATTR(""), 3, true);
		AddOption(CMD_OPTION_NAME_I);
		AddOption(CMD_OPTION_NAME_N);
		AddOption(CMD_OPTION_NAME_U);
		setCmdOptionSeparator(CMD_OPTION_SEPERATOR_COMMA);
	}

	~TopCommand(){
		stopRunningInBackground();
	}

	static void RegisterCommand(){
		CommandBase::RegisterCommand(CMD_NAME_TOP, [](void *arg)->void *{
			return pdiutil::safe_new<TopCommand>();
		});
	}

	const char* getUsage() const override {
		return RODT_ATTR("top [i=<ms>,n=<iters>,u=<sid>]  refreshed ps view (i 500 or more); Ctrl+C to stop");
	}

#ifdef ENABLE_AUTH_SERVICE
	bool needauth() override { return true; }
#endif

	bool stopRunningInBackground() override {
		__task_scheduler.remove_task(m_toptaskid);
		m_toptaskid = -1;
		m_runinbackground = false;
		return CommandBase::stopRunningInBackground();
	}

	pdi_err_t execute(cmd_term_inseq_t terminputaction){

#ifdef ENABLE_AUTH_SERVICE
		if( needauth() && !__auth_service.getAuthorized() ){
			return CMD_ERROR_PERM;
		}
#endif

		if( nullptr == m_terminal ){
			return CMD_ERROR_FAILED;
		}

		uint32_t interval = 2000;
		int32_t iterations = -1;
		m_filter_owner = 0xFF;

		CommandOption *icmdoptn = RetrieveOption(CMD_OPTION_NAME_I);
		CommandOption *ncmdoptn = RetrieveOption(CMD_OPTION_NAME_N);
		CommandOption *ucmdoptn = RetrieveOption(CMD_OPTION_NAME_U);

		pdi_err_t taken = PDI_OK;
		uint32_t number = 0;
		if( optionGiven(icmdoptn) ){
			taken = takeNumber(RODT_ATTR("invalid interval : "), icmdoptn->optionval, icmdoptn->optionvalsize, 500, INT32_MAX, interval);
		}
		if( PDI_OK == taken && optionGiven(ncmdoptn) ){
			taken = takeNumber(RODT_ATTR("invalid count : "), ncmdoptn->optionval, ncmdoptn->optionvalsize, 1, INT32_MAX, number);
			iterations = (int32_t)number;
		}
		if( PDI_OK == taken && optionGiven(ucmdoptn) ){
			taken = takeNumber(RODT_ATTR("invalid sid : "), ucmdoptn->optionval, ucmdoptn->optionvalsize, 0, 0xFE, number);
			m_filter_owner = (uint8_t)number;
		}
		if( PDI_OK != taken ){
			return taken;
		}

		__task_scheduler.remove_task(m_toptaskid);

		m_toptaskid = __task_scheduler.register_task( [&]() {
			if( nullptr == m_terminal ){
				stopRunningInBackground();
				return;
			}
			runAsOwner(this, [&](){
				m_terminal->csi_erase_display();
				m_terminal->csi_cursor_home();
				printProcessTable(m_terminal, m_filter_owner);
				m_terminal->commit();
			});
		}, interval, DEFAULT_TASK_PRIORITY, __i_dvc_ctrl.millis_now(), iterations, CMD_NAME_TOP );

		if( m_toptaskid < 0 ){
			return CMD_ERROR_FAILED;
		}

		m_runinbackground = true;
		return PDI_OK;
	}
};

#endif
