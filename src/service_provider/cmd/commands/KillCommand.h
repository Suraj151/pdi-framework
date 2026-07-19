/********************************* Kill Command *******************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 19th July 2026
******************************************************************************/

#ifndef _KILL_COMMAND_H_
#define _KILL_COMMAND_H_

#include "CommandCommon.h"

#if defined(ENABLE_AUTH_SERVICE) && defined(ENABLE_STORAGE_SERVICE)
#include <service_provider/user/UserStoreService.h>
#endif

/**
 * kill command — deliver a signal to a scheduler task (PID = scheduler task id).
 *
 * e.g.
 *   kill p=5        deliver SIG_TERM (15) — polite reap
 *   kill p=5 s=9    deliver SIG_KILL — uncatchable reap
 *   kill p=5 s=15   explicit SIG_TERM
 *
 * Permission:
 *   - root (uid=0) may kill any task
 *   - other users may only kill tasks owned by their session
 *   - kernel-owned tasks (owner=0) are only killable by root
 */
struct KillCommand : public CommandBase {

	KillCommand(){
		Clear();
		SetCommand(CMD_NAME_KILL);
		AddOption(CMD_OPTION_NAME_P);
		AddOption(CMD_OPTION_NAME_S);
		setCmdOptionSeparator(CMD_OPTION_SEPERATOR_SPACE);
	}

	static void RegisterCommand(){
		CommandBase::RegisterCommand(CMD_NAME_KILL, [](void *arg)->void *{
			return new KillCommand();
		});
	}

	const char* getUsage() const override {
		return RODT_ATTR("kill p=<pid> [s=<sig>]  signal a scheduler task (9/15/18/19)");
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

		CommandOption *pOpt = RetrieveOption(CMD_OPTION_NAME_P);
		CommandOption *sOpt = RetrieveOption(CMD_OPTION_NAME_S);

		if( nullptr == pOpt || nullptr == pOpt->optionval || 0 == pOpt->optionvalsize ){
			// Usage line is printed by CommandBase::ResultToTerminal.
			return CMD_RESULT_ARGS_MISSING;
		}

		pdiutil::task_id_t pid = (pdiutil::task_id_t)StringToUint16(pOpt->optionval, pOpt->optionvalsize);
		signal_t sig = SIG_TERM;
		if( nullptr != sOpt && nullptr != sOpt->optionval && sOpt->optionvalsize > 0 ){
			sig = (signal_t)StringToUint16(sOpt->optionval, sOpt->optionvalsize);
		}
		if( sig != SIG_TERM && sig != SIG_KILL && sig != SIG_STOP && sig != SIG_CONT ){
			m_terminal->writeln_ro(RODT_ATTR("\nunsupported signal (9/15/18/19 only)"));
			return CMD_RESULT_FAILED;
		}

		task_t *t = __task_scheduler.get_task(pid);
		if( nullptr == t || t->m_state == TASK_STATE_ZOMBIE ){
			m_terminal->writeln_ro(RODT_ATTR("\nno such pid"));
			return CMD_RESULT_FAILED;
		}
		uint8_t task_owner = t->m_owner;

#if defined(ENABLE_AUTH_SERVICE) && defined(ENABLE_STORAGE_SERVICE)
		bool isRoot = false;
		const char *me = __auth_service.getUsername();
		if( nullptr != me && me[0] ){
			user_record_t meRec;
			if( __user_store_service.findUserByName(me, meRec) && meRec.m_uid == USER_STORE_ROOT_UID ){
				isRoot = true;
			}
		}
		if( !isRoot ){
			session_t *cur = SessionManager::current();
			uint8_t cur_sid = (nullptr != cur) ? cur->m_sid : 0;
			if( task_owner != cur_sid ){
				m_terminal->writeln_ro(RODT_ATTR("\nnot owner"));
				return CMD_RESULT_FAILED;
			}
		}
#endif

		if( !__task_scheduler.sendSignal(pid, sig) ){
			m_terminal->writeln_ro(RODT_ATTR("\nsignal delivery failed"));
			return CMD_RESULT_FAILED;
		}

		return CMD_RESULT_OK;
	}
};

#endif
