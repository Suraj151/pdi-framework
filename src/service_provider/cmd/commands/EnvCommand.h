/********************************* Env Command ********************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 5th Sep 2026
******************************************************************************/
#ifndef _ENV_COMMAND_H_
#define _ENV_COMMAND_H_

#include "CommandCommon.h"
#include <service_provider/session/Environment.h>

struct EnvCommand : public CommandBase {

	EnvCommand(){
		Clear();
		SetCommand(CMD_NAME_ENV);
		setArgs(RODT_ATTR(""), 0);
	}

	static void RegisterCommand(){
		CommandBase::RegisterCommand(CMD_NAME_ENV, [](void *arg)->void *{
			return pdiutil::safe_new<EnvCommand>();
		});
	}

	const char* getUsage() const override {
		return RODT_ATTR("env  print the variables this session can see");
	}

#ifdef ENABLE_AUTH_SERVICE
	bool needauth() override { return true; }
#endif

	pdi_err_t execute(cmd_term_inseq_t terminputaction){

#ifdef ENABLE_AUTH_SERVICE
		if( needauth() && !__auth_service.getAuthorized() ){
			return CMD_ERROR_PERM;
		}
#endif

		if( nullptr == m_terminal ){
			return CMD_ERROR_NOTTY;
		}

		pdiutil::vector<config_kv_t> all;
		Environment::list(all);

		m_terminal->putln();

		for( uint32_t i = 0; i < all.size(); i++ ){
			m_terminal->write(all[i].m_key.c_str());
			m_terminal->write(KEY_VALUE_SEPARATOR_CHAR);
			m_terminal->writeln(all[i].m_value.c_str());
		}

		return PDI_OK;
	}
};

#endif
