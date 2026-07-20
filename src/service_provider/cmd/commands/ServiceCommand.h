/******************************* Service Command *****************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st June 2019
******************************************************************************/

#ifndef _SERVICE_COMMAND_H_
#define _SERVICE_COMMAND_H_

#include "CommandCommon.h"

#if defined(ENABLE_AUTH_SERVICE) && defined(ENABLE_STORAGE_SERVICE)
#include <service_provider/user/UserStoreService.h>
#endif

/**
 * srvc — service supervisor (systemd-lite, step E1).
 *
 *   srvc list                 List every registered service with its state.
 *   srvc status <name>        Show config + status + tracked task ids for one.
 *   srvc start <name>         Root-only. SIG_CONT every tracked task of the service.
 *   srvc stop <name>          Root-only. SIG_STOP every tracked task.
 *   srvc restart <name>       Root-only. STOP then CONT.
 *
 * State column (derived from tracked task states — see ServiceProvider::countServiceTasks):
 *   inactive  no tasks tracked (service never registered anything, or fully reaped)
 *   active    at least one task running (READY/RUNNING/SLEEPING)
 *   stopped   all tasks STOPPED (frozen by SIG_STOP)
 *   dead      tasks tracked but all in ZOMBIE (killed externally)
 *
 * Legacy `srvc s=<id> q=<query>` numeric form: removed in E1. Use subverbs.
 */
struct ServiceCommand : public CommandBase {

	enum ServiceSubverb : uint8_t {
		SVC_VERB_LIST = 0,
		SVC_VERB_STATUS,
		SVC_VERB_START,
		SVC_VERB_STOP,
		SVC_VERB_RESTART,
		SVC_VERB_UNKNOWN
	};

	ServiceCommand(){
		Clear();
		SetCommand(CMD_NAME_SERVICE);
		setAcceptArgsOptions(true);
		setCmdOptionSeparator(CMD_OPTION_SEPERATOR_SPACE);
	}

	static void RegisterCommand(){
		CommandBase::RegisterCommand(CMD_NAME_SERVICE, [](void *arg)->void *{
			return new ServiceCommand();
		});
	}

	const char* getUsage() const override {
		return RODT_ATTR("srvc list|status <name>|start <name>|stop <name>|restart <name>  service supervisor");
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

		CommandOption *verbOpt = &m_options[0];
		if( nullptr == verbOpt || nullptr == verbOpt->optionval || 0 == verbOpt->optionvalsize ){
			// Usage line is printed by CommandBase::ResultToTerminal.
			return CMD_RESULT_ARGS_MISSING;
		}

		ServiceSubverb verb = parseVerb(verbOpt->optionval, verbOpt->optionvalsize);
		if( SVC_VERB_UNKNOWN == verb ){
			return CMD_RESULT_INVALID_OPTION;
		}

		if( SVC_VERB_LIST == verb ){
			printList();
			return CMD_RESULT_OK;
		}

		CommandOption *nameOpt = &m_options[1];
		if( nullptr == nameOpt || nullptr == nameOpt->optionval || 0 == nameOpt->optionvalsize ){
			return CMD_RESULT_ARGS_MISSING;
		}
		char reqName[24];
		uint16_t nlen = nameOpt->optionvalsize < sizeof(reqName) ? nameOpt->optionvalsize : (sizeof(reqName) - 1);
		memcpy(reqName, nameOpt->optionval, nlen);
		reqName[nlen] = '\0';

		ServiceProvider *svc = findServiceByName(reqName);
		if( nullptr == svc ){
			m_terminal->writeln_ro(RODT_ATTR("\nno such service"));
			return CMD_RESULT_FAILED;
		}

		if( SVC_VERB_STATUS == verb ){
			printStatus(svc);
			return CMD_RESULT_OK;
		}

		if( !requireRoot() ){
			return CMD_RESULT_FAILED;
		}

		uint16_t hits = 0;
		switch( verb ){
			case SVC_VERB_STOP:
				hits = svc->signalAllServiceTasks(SIG_STOP);
				m_terminal->write_ro(RODT_ATTR("\nstopped "));
				break;
			case SVC_VERB_START:
				hits = svc->signalAllServiceTasks(SIG_CONT);
				m_terminal->write_ro(RODT_ATTR("\nstarted "));
				break;
			case SVC_VERB_RESTART:
				svc->signalAllServiceTasks(SIG_STOP);
				hits = svc->signalAllServiceTasks(SIG_CONT);
				m_terminal->write_ro(RODT_ATTR("\nrestarted "));
				break;
			default: return CMD_RESULT_FAILED;
		}
		char buf[8];
		Int32ToString((int32_t)hits, buf, 8, 0);
		m_terminal->write(buf);
		m_terminal->writeln_ro(RODT_ATTR(" task(s)"));
		return CMD_RESULT_OK;
	}

private:

	ServiceSubverb parseVerb(const char *raw, uint16_t len){
		// Verb literals live in RO/PROGMEM; strncmp_ro (arg2 = flash) reads them
		// safely on AVR/ESP. Length gate first — same-length false-prefixes fail.
		if( len == 4 && 0 == strncmp_ro(raw, RODT_ATTR("list"),    4) ) return SVC_VERB_LIST;
		if( len == 6 && 0 == strncmp_ro(raw, RODT_ATTR("status"),  6) ) return SVC_VERB_STATUS;
		if( len == 5 && 0 == strncmp_ro(raw, RODT_ATTR("start"),   5) ) return SVC_VERB_START;
		if( len == 4 && 0 == strncmp_ro(raw, RODT_ATTR("stop"),    4) ) return SVC_VERB_STOP;
		if( len == 7 && 0 == strncmp_ro(raw, RODT_ATTR("restart"), 7) ) return SVC_VERB_RESTART;
		return SVC_VERB_UNKNOWN;
	}

	/// Find a service by its m_service_name. Compare with strncmp_ro so PROGMEM
	/// (F()-wrapped) service names are read safely on AVR/ESP.
	ServiceProvider* findServiceByName(const char *name){
		size_t match_len = strlen(name) + 1;
		for (uint8_t i = 0; i < SERVICE_MAX; i++){
			ServiceProvider *s = ServiceProvider::getService((service_t)i);
			if( nullptr == s || nullptr == s->m_service_name ) continue;
			if( 0 == strncmp_ro(name, s->m_service_name, match_len) ) return s;
		}
		return nullptr;
	}

	/// Returned pointers live in RO/PROGMEM — callers must use write_ro, not write.
	static const char* stateLabel(uint16_t running, uint16_t stopped, uint16_t zombie){
		if( 0 == running + stopped + zombie ) return RODT_ATTR("inactive");
		if( 0 == running && 0 == stopped ) return RODT_ATTR("dead");
		if( 0 == running && stopped > 0 ) return RODT_ATTR("stopped");
		return RODT_ATTR("active");
	}

	// Column widths for `srvc list` — kept in one place so header and rows stay in sync.
	static constexpr uint8_t COL_SERVICE = 15;
	static constexpr uint8_t COL_STATE   = 10;
	static constexpr uint8_t COL_TASKS   = 6;

	void printList(){
		m_terminal->writeln();
		m_terminal->write_pad_ro(RODT_ATTR("SERVICE"), 7, COL_SERVICE);
		m_terminal->write_pad_ro(RODT_ATTR("STATE"),   5, COL_STATE);
		m_terminal->write_pad_ro(RODT_ATTR("TASKS"),   5, COL_TASKS);
		m_terminal->writeln_ro(RODT_ATTR("R/S/Z"));

		char buf[16];
		for (uint8_t i = 0; i < SERVICE_MAX; i++){
			ServiceProvider *s = ServiceProvider::getService((service_t)i);
			if( nullptr == s ) continue;
			uint16_t running, stopped, zombie;
			s->countServiceTasks(running, stopped, zombie);
			const char *svc_name = (s->m_service_name != nullptr) ? s->m_service_name : RODT_ATTR("-");
			m_terminal->write_pad_ro(svc_name, (uint32_t)strlen_ro(svc_name), COL_SERVICE);
			const char *state = stateLabel(running, stopped, zombie);
			m_terminal->write_pad_ro(state, (uint32_t)strlen_ro(state), COL_STATE);
			Int32ToString((int32_t)s->getServiceTaskCount(), buf, 16, COL_TASKS);
			m_terminal->write(buf);
			Int32ToString((int32_t)running, buf, 16, 0); m_terminal->write(buf);
			m_terminal->write_ro(RODT_ATTR("/"));
			Int32ToString((int32_t)stopped, buf, 16, 0); m_terminal->write(buf);
			m_terminal->write_ro(RODT_ATTR("/"));
			Int32ToString((int32_t)zombie,  buf, 16, 0); m_terminal->write(buf);
			m_terminal->writeln();
		}
	}

	void printStatus(ServiceProvider *svc){
		uint16_t running, stopped, zombie;
		svc->countServiceTasks(running, stopped, zombie);
		m_terminal->writeln();
		m_terminal->write_ro(RODT_ATTR("service : "));
		m_terminal->write_ro(svc->m_service_name != nullptr ? svc->m_service_name : "-");
		m_terminal->writeln();
		m_terminal->write_ro(RODT_ATTR("state   : "));
		m_terminal->writeln_ro(stateLabel(running, stopped, zombie));
		m_terminal->write_ro(RODT_ATTR("tasks   : "));
		char buf[16];
		Int32ToString((int32_t)svc->getServiceTaskCount(), buf, 16, 0);
		m_terminal->writeln(buf);
		m_terminal->write_ro(RODT_ATTR("pids    :"));
		for (uint8_t i = 0; i < svc->getServiceTaskCount(); i++){
			m_terminal->write_ro(RODT_ATTR(" "));
			Int32ToString((int32_t)svc->getServiceTaskId(i), buf, 16, 0);
			m_terminal->write(buf);
		}
		m_terminal->writeln();
	}

	bool requireRoot(){
#if defined(ENABLE_AUTH_SERVICE) && defined(ENABLE_STORAGE_SERVICE)
		const char *me = __auth_service.getUsername();
		if( nullptr != me && me[0] ){
			user_record_t meRec;
			if( __user_store_service.findUserByName(me, meRec) && meRec.m_uid == USER_STORE_ROOT_UID ){
				return true;
			}
		}
		m_terminal->writeln_ro(RODT_ATTR("\nroot required"));
		return false;
#else
		return true;
#endif
	}
};

#endif
