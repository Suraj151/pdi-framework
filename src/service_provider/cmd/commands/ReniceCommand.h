/******************************* Renice Command *******************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 19th July 2026
******************************************************************************/

#ifndef _RENICE_COMMAND_H_
#define _RENICE_COMMAND_H_

#include "CommandCommon.h"

#if defined(ENABLE_AUTH_SERVICE) && defined(ENABLE_STORAGE_SERVICE)
#include <service_provider/user/UserStoreService.h>
#endif

/**
 * renice — change the nice value of a live scheduler task.
 *
 *   renice <nice> <pid>
 *
 *   nice range: -20..19 (POSIX). Lower = higher priority. Clamped by the
 *   scheduler if out of range.
 *
 * Permission: same as kill. Root can renice any task; other users only tasks
 * owned by their session. Non-root cannot lower nice below the current value
 * (Linux behaviour) — deferred for now; today any allowed caller can set any
 * value in range.
 *
 * After updating the nice, calls rebaseAndRestartPrioTasks() so the reordered
 * priority takes effect on the next tick.
 */
struct ReniceCommand : public CommandBase {

	ReniceCommand(){
		Clear();
		SetCommand(CMD_NAME_RENICE);
		setAcceptArgsOptions(true);
		setCmdOptionSeparator(CMD_OPTION_SEPERATOR_SPACE);
	}

	static void RegisterCommand(){
		CommandBase::RegisterCommand(CMD_NAME_RENICE, [](void *arg)->void *{
			return new ReniceCommand();
		});
	}

	const char* getUsage() const override {
		return RODT_ATTR("renice <-20..19> <pid>  change a live task's POSIX nice");
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

		CommandOption *nOpt = &m_options[0];
		CommandOption *pOpt = &m_options[1];

		if( nullptr == nOpt || nullptr == nOpt->optionval || 0 == nOpt->optionvalsize ||
		    nullptr == pOpt || nullptr == pOpt->optionval || 0 == pOpt->optionvalsize ){
			// Usage line is printed by CommandBase::ResultToTerminal.
			return CMD_RESULT_ARGS_MISSING;
		}

		// Parse signed nice manually — StringToUintN doesn't accept a leading '-'.
		const char *nv = nOpt->optionval;
		uint16_t nvsize = nOpt->optionvalsize;
		bool negative = false;
		if( nv[0] == '-' ){
			negative = true;
			nv++;
			nvsize--;
		}
		int8_t nice = (int8_t)StringToUint8(nv, nvsize);
		if( negative ) nice = -nice;

		pdiutil::task_id_t pid = (pdiutil::task_id_t)StringToUint16(pOpt->optionval, pOpt->optionvalsize);

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

		if( !__task_scheduler.setTaskNice(pid, nice) ){
			m_terminal->writeln_ro(RODT_ATTR("\nsetTaskNice failed"));
			return CMD_RESULT_FAILED;
		}
		__task_scheduler.rebaseAndRestartPrioTasks();

		return CMD_RESULT_OK;
	}
};

#endif
