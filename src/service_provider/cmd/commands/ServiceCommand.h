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
 * service — start, stop and report the registered services.
 *
 *   service list             List every registered service with its state.
 *   service status <name>    Show config + status + tracked task ids for one.
 *   service start <name>     Root-only. startService() from cold; a no-op with a
 *                            message when the service is already running.
 *   service stop <name>      Root-only. Real teardown: stopService() releases the
 *                            listener, the connections and every tracked task.
 *   service restart <name>   Root-only. stopService() then startService().
 *   service enable <name>    Root-only. Persist "enabled yes" in the service conf.
 *   service disable <name>   Root-only. Persist "enabled no".
 *
 * No verb signals a task any more. enable and disable answer for the next boot
 * the way a real init system does, and leave what is running to start and stop.
 *
 * stop and restart refuse two services: an essential one, and the one carrying
 * the session the command was typed on — otherwise `service stop SSH` over ssh would
 * drop the connection mid-command.
 *
 * State column, recorded by the service's own start and stop rather than guessed
 * from what its tasks happen to be doing:
 *   inactive  never started, or stopped and released
 *   active    started, holding whatever it acquired
 *   failed    its own start reported that it did not come up
 *
 * The R/S/Z task counts stay beside it as detail. There is no activating or
 * deactivating: start and stop are synchronous here, so nothing could observe one.
 *
 * Legacy `service s=<id> q=<query>` numeric form: removed. Use subverbs.
 */
struct ServiceCommand : public CommandBase {

	enum ServiceSubverb : uint8_t {
		SVC_VERB_LIST = 0,
		SVC_VERB_STATUS,
		SVC_VERB_START,
		SVC_VERB_STOP,
		SVC_VERB_RESTART,
		SVC_VERB_ENABLE,
		SVC_VERB_DISABLE,
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
			return pdiutil::safe_new<ServiceCommand>();
		});
	}

	const char* getUsage() const override {
		return RODT_ATTR("service list|status|start|stop|restart|enable|disable [<name>]  service control");
	}

#ifdef ENABLE_AUTH_SERVICE
	bool needauth() override { return true; }
#endif

	/**
	 * The subcommand comes first and every argument after it names a service.
	 */
	cmd_complete_t completionFor(uint8_t argindex) const override {
		return 0 == argindex ? CMD_COMPLETE_NONE : CMD_COMPLETE_SERVICE;
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

		CommandOption *verbOpt = &m_options[0];
		if( nullptr == verbOpt || nullptr == verbOpt->optionval || 0 == verbOpt->optionvalsize ){
			// Usage line is printed by CommandBase::ResultToTerminal.
			return CMD_ERROR_ARGS_MISSING;
		}

		ServiceSubverb verb = parseVerb(verbOpt->optionval, verbOpt->optionvalsize);
		if( SVC_VERB_UNKNOWN == verb ){
			return CMD_ERROR_OPT;
		}

		if( SVC_VERB_LIST == verb ){
			printList();
			return PDI_OK;
		}

		CommandOption *nameOpt = &m_options[1];
		if( nullptr == nameOpt || nullptr == nameOpt->optionval || 0 == nameOpt->optionvalsize ){
			return CMD_ERROR_ARGS_MISSING;
		}
		char reqName[24];
		uint16_t nlen = nameOpt->optionvalsize < sizeof(reqName) ? nameOpt->optionvalsize : (sizeof(reqName) - 1);
		memcpy(reqName, nameOpt->optionval, nlen);
		reqName[nlen] = '\0';

		ServiceProvider *svc = findServiceByName(reqName);
		if( nullptr == svc ){
			m_terminal->putln();
			m_terminal->writeln_ro(RODT_ATTR("no such service"));
			return CMD_ERROR_FAILED;
		}

		if( SVC_VERB_STATUS == verb ){
			printStatus(svc);
			return PDI_OK;
		}

		if( !requireRoot() ){
			return CMD_ERROR_FAILED;
		}

		if( SVC_VERB_ENABLE == verb || SVC_VERB_DISABLE == verb ){
			return setEnabled(svc, SVC_VERB_ENABLE == verb);
		}

		switch( verb ){
			case SVC_VERB_STOP:    return stopOne(svc);
			case SVC_VERB_START:   return startOne(svc);
			case SVC_VERB_RESTART: return restartOne(svc);
			default: return CMD_ERROR_FAILED;
		}
	}

private:

	/**
	 * Tears the service down for real, refusing when it is the one carrying the
	 * line this command was typed on.
	 */
	pdi_err_t stopOne(ServiceProvider *svc){

		if( svc->isEssentialService() ){
			m_terminal->putln();
			m_terminal->writeln_ro(RODT_ATTR("the device cannot be recovered without this service"));
			return CMD_ERROR_PERM;
		}

		if( ownsThisSession(svc) ){
			m_terminal->putln();
			m_terminal->writeln_ro(RODT_ATTR("this service carries the session you are typing on"));
			return CMD_ERROR_PERM;
		}

		uint8_t before = svc->getServiceTaskCount();

		m_terminal->putln();
		if( !svc->stopService() ){
			m_terminal->putln();
			m_terminal->writeln_ro(RODT_ATTR("the service refused to stop"));
			return CMD_ERROR_FAILED;
		}

		m_terminal->write_ro(RODT_ATTR("stopped "));
		m_terminal->write_ro(svc->m_service_name);
		m_terminal->write_ro(RODT_ATTR(", released "));
		char buf[8];
		Int32ToString((int32_t)before, buf, 8, 0);
		m_terminal->write(buf);
		m_terminal->writeln_ro(RODT_ATTR(" task(s)"));
		return PDI_OK;
	}

	/**
	 * Brings the service up from cold, and says so rather than acting when it is
	 * already running.
	 */
	pdi_err_t startOne(ServiceProvider *svc){

		if( SERVICE_STATE_ACTIVE == svc->getServiceState() ){
			m_terminal->putln();
			m_terminal->write_ro(svc->m_service_name);
			m_terminal->writeln_ro(RODT_ATTR(" is already running"));
			return PDI_OK;
		}

		if( !svc->isServiceEnabled() ){
			m_terminal->putln();
			m_terminal->writeln_ro(RODT_ATTR("it is disabled, enable it first"));
			return CMD_ERROR_PERM;
		}

		ServiceProvider *unmet = svc->findUnmetServiceDependency();
		if( nullptr != unmet ){
			m_terminal->putln();
			m_terminal->write_ro(RODT_ATTR("it needs "));
			m_terminal->write_ro(unmet->m_service_name);
			m_terminal->writeln_ro(RODT_ATTR(" running first"));
			return CMD_ERROR_FAILED;
		}

		m_terminal->putln();
		if( !svc->startService() ){
			m_terminal->putln();
			m_terminal->writeln_ro(RODT_ATTR("the service did not start"));
			return CMD_ERROR_FAILED;
		}

		m_terminal->write_ro(RODT_ATTR("started "));
		m_terminal->writeln_ro(svc->m_service_name);
		return PDI_OK;
	}

	/**
	 * Stops the service and starts it again, refusing for the same reasons a stop
	 * refuses since the teardown is the same one.
	 */
	pdi_err_t restartOne(ServiceProvider *svc){

		if( svc->isEssentialService() ){
			m_terminal->putln();
			m_terminal->writeln_ro(RODT_ATTR("the device cannot be recovered without this service"));
			return CMD_ERROR_PERM;
		}

		if( ownsThisSession(svc) ){
			m_terminal->putln();
			m_terminal->writeln_ro(RODT_ATTR("this service carries the session you are typing on"));
			return CMD_ERROR_PERM;
		}

		if( !svc->isServiceEnabled() ){
			m_terminal->putln();
			m_terminal->writeln_ro(RODT_ATTR("it is disabled, enable it first"));
			return CMD_ERROR_PERM;
		}

		m_terminal->putln();
		svc->stopService();

		ServiceProvider *unmet = svc->findUnmetServiceDependency();
		if( nullptr != unmet ){
			m_terminal->putln();
			m_terminal->write_ro(RODT_ATTR("it needs "));
			m_terminal->write_ro(unmet->m_service_name);
			m_terminal->writeln_ro(RODT_ATTR(" running first"));
			return CMD_ERROR_FAILED;
		}

		if( !svc->startService() ){
			m_terminal->putln();
			m_terminal->writeln_ro(RODT_ATTR("the service stopped but did not start again"));
			return CMD_ERROR_FAILED;
		}

		m_terminal->write_ro(RODT_ATTR("restarted "));
		m_terminal->writeln_ro(svc->m_service_name);
		return PDI_OK;
	}

	/**
	 * Whether the service carries the transport this command arrived on, which
	 * is the one connection a stop must never take down under itself.
	 */
	bool ownsThisSession(ServiceProvider *svc){

		terminal_types_t owned = svc->getServiceTerminalType();
		if( TERMINAL_TYPE_MAX == owned ){
			return false;
		}

		session_t *cur = SessionManager::current();
		if( nullptr == cur || nullptr == cur->m_terminal ){
			return false;
		}

		return owned == cur->m_terminal->get_terminal_type();
	}

	pdi_err_t setEnabled(ServiceProvider *svc, bool enable){

		if( !enable && svc->isEssentialService() ){
			m_terminal->putln();
			m_terminal->writeln_ro(RODT_ATTR("the device cannot be recovered without this service"));
			return CMD_ERROR_PERM;
		}

		if( !svc->setServiceEnabled(enable) ){
			m_terminal->putln();
			m_terminal->writeln_ro(RODT_ATTR("could not persist the service state"));
			return CMD_ERROR_FAILED;
		}

		// enable and disable answer for the next boot, the way they do on a real
		// init system; what runs right now is what start and stop are for
		m_terminal->putln();
		m_terminal->write_ro(enable ? RODT_ATTR("enabled ") : RODT_ATTR("disabled "));
		m_terminal->write_ro(svc->m_service_name);
		m_terminal->writeln_ro(RODT_ATTR(" : takes effect on the next boot"));
		return PDI_OK;
	}

	ServiceSubverb parseVerb(const char *raw, uint16_t len){
		// Verb literals live in RO/PROGMEM; strncmp_ro (arg2 = flash) reads them
		// safely on AVR/ESP. Length gate first — same-length false-prefixes fail.
		if( len == 4 && 0 == strncmp_ro(raw, RODT_ATTR("list"),    4) ) return SVC_VERB_LIST;
		if( len == 6 && 0 == strncmp_ro(raw, RODT_ATTR("status"),  6) ) return SVC_VERB_STATUS;
		if( len == 5 && 0 == strncmp_ro(raw, RODT_ATTR("start"),   5) ) return SVC_VERB_START;
		if( len == 4 && 0 == strncmp_ro(raw, RODT_ATTR("stop"),    4) ) return SVC_VERB_STOP;
		if( len == 7 && 0 == strncmp_ro(raw, RODT_ATTR("restart"), 7) ) return SVC_VERB_RESTART;
		if( len == 6 && 0 == strncmp_ro(raw, RODT_ATTR("enable"),  6) ) return SVC_VERB_ENABLE;
		if( len == 7 && 0 == strncmp_ro(raw, RODT_ATTR("disable"), 7) ) return SVC_VERB_DISABLE;
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
	static const char* stateLabel(service_state_t state){
		switch( state ){
			case SERVICE_STATE_ACTIVE: return RODT_ATTR("active");
			case SERVICE_STATE_FAILED: return RODT_ATTR("failed");
			default:                   return RODT_ATTR("inactive");
		}
	}

	// Column widths for `service list` — kept in one place so header and rows stay in sync.
	static constexpr uint8_t COL_SERVICE = 15;
	static constexpr uint8_t COL_STATE   = 10;
	static constexpr uint8_t COL_ENABLED = 9;
	static constexpr uint8_t COL_TASKS   = 6;

	void printList(){
		m_terminal->writeln();
		m_terminal->write_pad_ro(RODT_ATTR("SERVICE"), 7, COL_SERVICE);
		m_terminal->write_pad_ro(RODT_ATTR("STATE"),   5, COL_STATE);
		m_terminal->write_pad_ro(RODT_ATTR("ENABLED"), 7, COL_ENABLED);
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
			const char *state = stateLabel(s->getServiceState());
			m_terminal->write_pad_ro(state, (uint32_t)strlen_ro(state), COL_STATE);
			const char *enabled = s->isServiceEnabled() ? RODT_ATTR("yes") : RODT_ATTR("no");
			m_terminal->write_pad_ro(enabled, (uint32_t)strlen_ro(enabled), COL_ENABLED);
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
		m_terminal->writeln_ro(stateLabel(svc->getServiceState()));
		m_terminal->write_ro(RODT_ATTR("enabled : "));
		m_terminal->writeln_ro(svc->isServiceEnabled() ? RODT_ATTR("yes") : RODT_ATTR("no"));
		m_terminal->write_ro(RODT_ATTR("config  : "));
		pdiutil::string cfgpath;
		svc->getServiceConfigPath(cfgpath);
		m_terminal->writeln(cfgpath.c_str());
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

		// common header for the service-specific detail each service appends
		// m_terminal->writeln_ro(RODT_ATTR("service info :"));
		svc->printStatusToTerminal(m_terminal);
	}

	bool requireRoot(){
#if defined(ENABLE_AUTH_SERVICE) && defined(ENABLE_STORAGE_SERVICE)
		if( SessionManager::isRoot() ){
			return true;
		}
		m_terminal->putln();
		m_terminal->writeln_ro(RODT_ATTR(CMD_MSG_ROOT_REQUIRED));
		return false;
#else
		return true;
#endif
	}
};

#endif
