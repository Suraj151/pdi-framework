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

/**
 * top command — periodically refreshed ps view.
 *
 * e.g.
 *   top                     refresh every 2s, forever (until stopRunningInBackground)
 *   top i=1000              refresh every 1s
 *   top i=2000; n=5         5 refreshes, then stop
 *   top u=1                 filter by owner session id 1
 */
struct TopCommand : public CommandBase {

	int32_t m_toptaskid = -1;
	uint8_t m_filter_owner = 0xFF;

	TopCommand(){
		Clear();
		SetCommand(CMD_NAME_TOP);
		AddOption(CMD_OPTION_NAME_I);
		AddOption(CMD_OPTION_NAME_N);
		AddOption(CMD_OPTION_NAME_U);
		setCmdOptionSeparator(CMD_OPTION_SEPERATOR_SEMICOLON);
	}

	~TopCommand(){
		stopRunningInBackground();
	}

	static void RegisterCommand(){
		CommandBase::RegisterCommand(CMD_NAME_TOP, [](void *arg)->void *{
			return new TopCommand();
		});
	}

	const char* getUsage() const override {
		return RODT_ATTR("top [i=<ms>; n=<iters>; u=<sid>]  refreshed ps view; Ctrl+C to stop");
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

	cmd_result_t execute(cmd_term_inseq_t terminputaction){

#ifdef ENABLE_AUTH_SERVICE
		if( needauth() && !__auth_service.getAuthorized() ){
			return CMD_RESULT_NEED_AUTH;
		}
#endif

		if( nullptr == m_terminal ){
			return CMD_RESULT_FAILED;
		}

		uint32_t interval = 2000;
		int32_t iterations = -1;
		m_filter_owner = 0xFF;

		CommandOption *icmdoptn = RetrieveOption(CMD_OPTION_NAME_I);
		CommandOption *ncmdoptn = RetrieveOption(CMD_OPTION_NAME_N);
		CommandOption *ucmdoptn = RetrieveOption(CMD_OPTION_NAME_U);

		if( nullptr != icmdoptn && nullptr != icmdoptn->optionval && icmdoptn->optionvalsize > 0 ){
			interval = StringToUint32(icmdoptn->optionval, icmdoptn->optionvalsize);
			if( interval < 500 ) interval = 500;
		}
		if( nullptr != ncmdoptn && nullptr != ncmdoptn->optionval && ncmdoptn->optionvalsize > 0 ){
			iterations = (int32_t)StringToUint32(ncmdoptn->optionval, ncmdoptn->optionvalsize);
			if( iterations < 1 ) iterations = 1;
		}
		if( nullptr != ucmdoptn && nullptr != ucmdoptn->optionval && ucmdoptn->optionvalsize > 0 ){
			m_filter_owner = (uint8_t)StringToUint16(ucmdoptn->optionval, ucmdoptn->optionvalsize);
		}

		__task_scheduler.remove_task(m_toptaskid);

		m_toptaskid = __task_scheduler.register_task( [&]() {
			if( nullptr == m_terminal ){
				stopRunningInBackground();
				return;
			}
			m_terminal->csi_erase_display();
			m_terminal->csi_cursor_home();
			__task_scheduler.printPsToTerminal(m_terminal, m_filter_owner);
		}, interval, 0, __i_dvc_ctrl.millis_now(), iterations, CMD_NAME_TOP );

		if( m_toptaskid < 0 ){
			return CMD_RESULT_FAILED;
		}

		m_runinbackground = true;
		return CMD_RESULT_OK;
	}
};

#endif
