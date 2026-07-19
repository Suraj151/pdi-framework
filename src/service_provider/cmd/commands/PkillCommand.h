/****************************** Pkill / Killall *******************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 19th July 2026
******************************************************************************/

#ifndef _PKILL_COMMAND_H_
#define _PKILL_COMMAND_H_

#include "CommandCommon.h"

#if defined(ENABLE_AUTH_SERVICE) && defined(ENABLE_STORAGE_SERVICE)
#include <service_provider/user/UserStoreService.h>
#endif

/**
 * pkill / killall — deliver a signal to every scheduler task whose name
 * matches. Same permission model as `kill` (root can signal any task; other
 * users only tasks they own).
 *
 *   pkill n=<name>        SIG_TERM (default)
 *   pkill n=<name> s=9    SIG_KILL
 *   killall n=<name>      SIG_KILL (default — killall is the impolite one)
 *   killall n=<name> s=15 SIG_TERM
 *
 * Prints the number of tasks the signal reached.
 */
struct PkillCommand : public CommandBase {

	PkillCommand(){
		Clear();
		SetCommand(CMD_NAME_PKILL);
		AddOption(CMD_OPTION_NAME_N);
		AddOption(CMD_OPTION_NAME_S);
		setCmdOptionSeparator(CMD_OPTION_SEPERATOR_SPACE);
	}

	static void RegisterCommand(){
		CommandBase::RegisterCommand(CMD_NAME_PKILL, [](void *arg)->void *{
			return new PkillCommand();
		});
	}

	const char* getUsage() const override {
		return RODT_ATTR("pkill n=<name> [s=<sig>]  signal every task matching name (default TERM)");
	}

#ifdef ENABLE_AUTH_SERVICE
	bool needauth() override { return true; }
#endif

	/// Default signal when `s=` is omitted. Overridden by KillallCommand.
	virtual signal_t defaultSignal() const { return SIG_TERM; }

	cmd_result_t execute(cmd_term_inseq_t terminputaction){

#ifdef ENABLE_AUTH_SERVICE
		if( needauth() && !__auth_service.getAuthorized() ){
			return CMD_RESULT_NEED_AUTH;
		}
#endif

		if( nullptr == m_terminal ){
			return CMD_RESULT_FAILED;
		}

		CommandOption *nOpt = RetrieveOption(CMD_OPTION_NAME_N);
		CommandOption *sOpt = RetrieveOption(CMD_OPTION_NAME_S);

		if( nullptr == nOpt || nullptr == nOpt->optionval || 0 == nOpt->optionvalsize ){
			// Usage line is printed by CommandBase::ResultToTerminal.
			return CMD_RESULT_ARGS_MISSING;
		}

		signal_t sig = defaultSignal();
		if( nullptr != sOpt && nullptr != sOpt->optionval && sOpt->optionvalsize > 0 ){
			sig = (signal_t)StringToUint16(sOpt->optionval, sOpt->optionvalsize);
		}
		if( sig != SIG_TERM && sig != SIG_KILL && sig != SIG_STOP && sig != SIG_CONT ){
			m_terminal->writeln_ro(RODT_ATTR("\nunsupported signal (9/15/18/19 only)"));
			return CMD_RESULT_FAILED;
		}

		// Copy the name option into a local NUL-terminated buffer so
		// sendSignalByName can strlen it safely.
		char name[32];
		uint16_t nlen = nOpt->optionvalsize < sizeof(name) ? nOpt->optionvalsize : (sizeof(name) - 1);
		memcpy(name, nOpt->optionval, nlen);
		name[nlen] = '\0';

		bool isRoot = false;
		uint8_t cur_sid = 0;
#if defined(ENABLE_AUTH_SERVICE) && defined(ENABLE_STORAGE_SERVICE)
		const char *me = __auth_service.getUsername();
		if( nullptr != me && me[0] ){
			user_record_t meRec;
			if( __user_store_service.findUserByName(me, meRec) && meRec.m_uid == USER_STORE_ROOT_UID ){
				isRoot = true;
			}
		}
		session_t *cur = SessionManager::current();
		cur_sid = (nullptr != cur) ? cur->m_sid : 0;
#else
		isRoot = true; // no auth compiled in — no restriction
#endif

		uint16_t hits = __task_scheduler.sendSignalByName(name, sig, cur_sid, isRoot);

		m_terminal->write_ro(RODT_ATTR("\nsignaled "));
		char buf[8];
		Int32ToString((int32_t)hits, buf, 8, 0);
		m_terminal->write(buf);
		m_terminal->writeln_ro(RODT_ATTR(" task(s)"));

		return CMD_RESULT_OK;
	}
};

/**
 * killall — same as pkill but default signal is SIG_KILL.
 */
struct KillallCommand : public PkillCommand {

	KillallCommand(){
		Clear();
		SetCommand(CMD_NAME_KILLALL);
		AddOption(CMD_OPTION_NAME_N);
		AddOption(CMD_OPTION_NAME_S);
		setCmdOptionSeparator(CMD_OPTION_SEPERATOR_SPACE);
	}

	static void RegisterCommand(){
		CommandBase::RegisterCommand(CMD_NAME_KILLALL, [](void *arg)->void *{
			return new KillallCommand();
		});
	}

	const char* getUsage() const override {
		return RODT_ATTR("killall n=<name> [s=<sig>]  signal every task matching name (default KILL)");
	}

	signal_t defaultSignal() const override { return SIG_KILL; }
};

#endif
