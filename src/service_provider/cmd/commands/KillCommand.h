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
 *   kill 5           deliver SIG_TERM (15) — polite reap
 *   kill 9 5         deliver SIG_KILL — uncatchable reap
 *   kill 15 5        explicit SIG_TERM
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
		setAcceptArgsOptions(true);
		setCmdOptionSeparator(CMD_OPTION_SEPERATOR_SPACE);
	}

	static void RegisterCommand(){
		CommandBase::RegisterCommand(CMD_NAME_KILL, [](void *arg)->void *{
			return new KillCommand();
		});
	}

	const char* getUsage() const override {
		return RODT_ATTR("kill [<sig>] <pid>  signal a scheduler task (9[KILL]/15[TERM]/18[CONT]/19[STOP])");
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

		// Positional: <pid> OR <sig> <pid>. First slot is the signal only when a
		// second slot is populated too; otherwise it's the pid.
		CommandOption *a0 = &m_options[0];
		CommandOption *a1 = &m_options[1];
		bool have0 = ( nullptr != a0 && nullptr != a0->optionval && a0->optionvalsize > 0 );
		bool have1 = ( nullptr != a1 && nullptr != a1->optionval && a1->optionvalsize > 0 );

		if( !have0 ){
			return CMD_RESULT_ARGS_MISSING;
		}

		CommandOption *sOpt = have1 ? a0 : nullptr;
		CommandOption *pOpt = have1 ? a1 : a0;

		pdiutil::task_id_t pid = (pdiutil::task_id_t)StringToUint16(pOpt->optionval, pOpt->optionvalsize);
		signal_t sig = SIG_TERM;
		if( nullptr != sOpt ){
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
