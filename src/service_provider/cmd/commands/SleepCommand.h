/******************************** Sleep Command *******************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Waits the number of whole seconds given before the next line runs, the way
POSIX sleep does.
The session and any script it runs wait; every other session and service keeps
running, and ctrl+c ends the wait. A scheduled job, the boot script, a pipe, a
capture or a redirect has no session to come back to, so there the board itself
waits, for at most SLEEP_DETACHED_MAX_MS.

Author          : Suraj I.
created Date    : 7th Nov 2026
******************************************************************************/
#ifndef _SLEEP_COMMAND_H_
#define _SLEEP_COMMAND_H_

#include "CommandCommon.h"

struct SleepCommand : public CommandBase {

	SleepCommand(){
		Clear();
		SetCommand(CMD_NAME_SLEEP);
		setArgs(RODT_ATTR(""), 1);
		setAcceptArgsOptions(true);
		setCmdOptionSeparator(CMD_OPTION_SEPERATOR_SPACE);
		reservePositionalSlots(1);
		m_taskid = -1;
		m_due = 0;
		m_tailnext = 0;
	}

	~SleepCommand(){
		stopRunningInBackground();
	}

	static void RegisterCommand(){
		CommandBase::RegisterCommand(CMD_NAME_SLEEP, [](void *arg)->void *{
			return pdiutil::safe_new<SleepCommand>();
		});
	}

	const char* getUsage() const override {
		return RODT_ATTR("sleep <seconds>  wait that many whole seconds");
	}

#ifdef ENABLE_AUTH_SERVICE
	bool needauth() override { return true; }
#endif

	bool holdsInput() override {
		return m_taskid >= 0;
	}

	void keepLineTail(const pdiutil::string &line, uint16_t next) override {
		m_tail = line;
		m_tailnext = next;
	}

	bool takeLineTail(pdiutil::string &line, uint16_t &next) override {
		if( m_tail.empty() ){
			return false;
		}
		line = m_tail;
		next = m_tailnext;
		m_tail.clear();
		return true;
	}

	bool stopRunningInBackground() override {
		__task_scheduler.clearTimeout(m_taskid);
		m_taskid = -1;
		return CommandBase::stopRunningInBackground();
	}

	pdi_err_t execute(cmd_term_inseq_t terminputaction){

#ifdef ENABLE_AUTH_SERVICE
		if( needauth() && !__auth_service.getAuthorized() ){
			return CMD_ERROR_PERM;
		}
#endif

		if( nullptr == m_terminal ){
			return CMD_ERROR_NOTTY;
		}

		if( m_taskid >= 0 ){
			if( CMD_TERM_INSEQ_CTRL_C == terminputaction || CMD_TERM_INSEQ_CTRL_Z == terminputaction ){
				stopRunningInBackground();
				return CMD_ERROR_CANCELED;
			}
			if( (int32_t)(__i_dvc_ctrl.millis_now() - m_due) < 0 ){
				setWaitingForOption((int8_t)0);
				return CMD_ERROR_AGAIN;
			}
			stopRunningInBackground();
			return PDI_OK;
		}

		if( !optionGiven(&m_options[0]) ){
			return CMD_ERROR_ARGS_MISSING;
		}
		uint32_t seconds = 0;
		pdi_err_t taken = takeNumber(RODT_ATTR("sleep: invalid time interval : "), m_options[0].optionval,
									 m_options[0].optionvalsize, 0, SLEEP_MAX_SECONDS, seconds);
		if( PDI_OK != taken ){
			return taken;
		}
		if( 0 == seconds ){
			return PDI_OK;
		}
		uint32_t wait = seconds * MILLISECOND_DURATION_1000;

		bool resumable = ( nullptr != m_owner && m_terminal == m_owner->m_terminal );
#ifdef ENABLE_SCRIPT_RUNNER
		resumable = resumable && !ScriptRunner::detached();
#endif
		if( !resumable ){
			return holdBoard(wait);
		}

		m_due = __i_dvc_ctrl.millis_now() + wait;
		m_taskid = __task_scheduler.setTimeout([&]() {
			if( nullptr != m_cmdexecinterface ){
				m_cmdexecinterface->wakeCommand(this);
			}
		}, wait, __i_dvc_ctrl.millis_now(), DEFAULT_TASK_PRIORITY, RODT_ATTR(CMD_NAME_SLEEP));
		if( m_taskid < 0 ){
			m_terminal->putln();
			m_terminal->write_ro(RODT_ATTR("sleep: no task free to wait on"));
			return CMD_ERROR_FAILED;
		}
		setWaitingForOption((int8_t)0);
		return CMD_ERROR_AGAIN;
	}

private:

	int32_t m_taskid;
	uint32_t m_due;
	pdiutil::string m_tail;
	uint16_t m_tailnext;

	/**
	 * Waits on the board itself, for a wait nothing can come back to: detached
	 * work, or output going to a pipe, a capture or a file. A wait past
	 * SLEEP_DETACHED_MAX_MS is refused and said in the log.
	 */
	pdi_err_t holdBoard(uint32_t wait){
		if( wait > SLEEP_DETACHED_MAX_MS ){
			SysLogW("sleep: %u ms refused where nothing can resume it, at most %u ms\n", (unsigned)wait, (unsigned)SLEEP_DETACHED_MAX_MS);
			m_terminal->putln();
			m_terminal->write_ro(RODT_ATTR("sleep: too long to wait here"));
			return CMD_ERROR_INVAL;
		}
		uint32_t start = __i_dvc_ctrl.millis_now();
		while( (__i_dvc_ctrl.millis_now() - start) < wait ){
			if( SessionManager::interruptRequested() ){
				return CMD_ERROR_CANCELED;
			}
			__i_dvc_ctrl.wait(10);
			__i_dvc_ctrl.yield();
		}
		return PDI_OK;
	}
};

#endif
