/******************************** Id Command **********************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 18th July 2026
******************************************************************************/
#ifndef _ID_COMMAND_H_
#define _ID_COMMAND_H_

#include "CommandCommon.h"
#include <utility/DataTypeConversions.h>

#ifdef ENABLE_AUTH_SERVICE
#ifdef ENABLE_STORAGE_SERVICE
#include <service_provider/user/UserStoreService.h>
#endif

struct IdCommand : public CommandBase {

	IdCommand(){
		Clear();
		SetCommand(CMD_NAME_ID);
	}

	static void RegisterCommand(){
		CommandBase::RegisterCommand(CMD_NAME_ID, [](void *arg)->void *{
			return pdiutil::safe_new<IdCommand>();
		});
	}

	const char* getUsage() const override {
		return RODT_ATTR("id  print uid=N(user) gid=N for the current session");
	}

	bool needauth() override { return true; }

	cmd_result_t execute(cmd_term_inseq_t terminputaction){

		if( needauth() && !__auth_service.getAuthorized() ){
			return CMD_RESULT_NEED_AUTH;
		}

		if( nullptr == m_terminal ) return CMD_RESULT_OK;

		const char *u = __auth_service.getUsername();
		m_terminal->putln();

		if( nullptr == u || 0 == u[0] ){
			return CMD_RESULT_OK;
		}

		char numbuf[8];

		m_terminal->write_ro(RODT_ATTR("uid="));
		Uint32ToString((uint32_t)SessionManager::getCurrentUid(), numbuf, sizeof(numbuf));
		m_terminal->write(numbuf);
		m_terminal->write_ro(RODT_ATTR("("));
		m_terminal->write(u);
		m_terminal->write_ro(RODT_ATTR(") gid="));
		Uint32ToString((uint32_t)SessionManager::getCurrentGid(), numbuf, sizeof(numbuf));
		m_terminal->write(numbuf);
		return CMD_RESULT_OK;
	}
};

#endif

#endif
