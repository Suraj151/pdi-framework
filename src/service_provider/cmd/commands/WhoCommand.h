/******************************** Who Command *********************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 18th July 2026
******************************************************************************/
#ifndef _WHO_COMMAND_H_
#define _WHO_COMMAND_H_

#include "CommandCommon.h"
#include <utility/DataTypeConversions.h>

#ifdef ENABLE_AUTH_SERVICE

namespace {

	inline const char *SessionTtyName(terminal_types_t t){
		switch(t){
			case TERMINAL_TYPE_SERIAL: return "tty";
			case TERMINAL_TYPE_TELNET: return "tel";
			case TERMINAL_TYPE_SSH:    return "ssh";
			default: return "?";
		}
	}
}

struct WhoCommand : public CommandBase {

	WhoCommand(){
		Clear();
		SetCommand(CMD_NAME_WHO);
	}

	static void RegisterCommand(){
		CommandBase::RegisterCommand(CMD_NAME_WHO, [](void *arg)->void *{
			return new WhoCommand();
		});
	}

	const char* getUsage() const override {
		return RODT_ATTR("who  list authenticated sessions (USER TTY SID LOGIN IDLE)");
	}

	bool needauth() override { return true; }

	cmd_result_t execute(cmd_term_inseq_t terminputaction){

		if( needauth() && !__auth_service.getAuthorized() ){
			return CMD_RESULT_NEED_AUTH;
		}

		if( nullptr == m_terminal ) return CMD_RESULT_OK;

		m_terminal->putln();
		m_terminal->write_pad_ro(RODT_ATTR("USER"),  4, COL_USER);
		m_terminal->write_pad_ro(RODT_ATTR("TTY"),   3, COL_TTY);
		m_terminal->write_pad_ro(RODT_ATTR("SID"),   3, COL_SID);
		m_terminal->write_pad_ro(RODT_ATTR("LOGIN"), 5, COL_LOGIN);
		m_terminal->writeln_ro(RODT_ATTR("IDLE"));

		uint32_t now = (uint32_t)__i_dvc_ctrl.millis_now();
		char numbuf[12];

		for( uint8_t i = 0; i < SessionManager::maxSessions(); i++ ){

			session_t *s = SessionManager::getByIndex(i);
			if( nullptr == s || SESSION_STATE_FREE == s->m_state ) continue;
			if( !s->m_isAuthorized ) continue;

			m_terminal->write_pad(s->m_username.c_str(), COL_USER);
			const char *tty = SessionTtyName(nullptr != s->m_terminal ? s->m_terminal->get_terminal_type() : TERMINAL_TYPE_MAX);
			m_terminal->write_pad_ro(tty, (uint32_t)strlen_ro(tty), COL_TTY);
			Uint32ToString((uint32_t)s->m_sid, numbuf, sizeof(numbuf));
			m_terminal->write_pad(numbuf, COL_SID);
			uint32_t login_s = (now - s->m_loginAt) / 1000;
			Uint32ToString(login_s, numbuf, sizeof(numbuf));
			uint32_t login_len = (uint32_t)strlen(numbuf);
			if( login_len < sizeof(numbuf) - 1 ){ numbuf[login_len] = 's'; numbuf[login_len + 1] = '\0'; }
			m_terminal->write_pad(numbuf, COL_LOGIN);
			Uint32ToString((now - s->m_lastActivityAt) / 1000, numbuf, sizeof(numbuf));
			m_terminal->write(numbuf);
			m_terminal->write_ro(RODT_ATTR("s"));
			m_terminal->putln();
		}

		return CMD_RESULT_OK;
	}

private:

	static constexpr uint8_t COL_USER  = 10;
	static constexpr uint8_t COL_TTY   = 5;
	static constexpr uint8_t COL_SID   = 5;
	static constexpr uint8_t COL_LOGIN = 8;
};

#endif

#endif
