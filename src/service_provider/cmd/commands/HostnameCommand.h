/******************************* Hostname Command ******************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Prints the host name, the way linux hostname does with no argument: the name in
/etc/hostname, which mDNS writes from the board's mac when it starts.

Author          : Suraj I.
created Date    : 26th Oct 2026
******************************************************************************/
#ifndef _HOSTNAME_COMMAND_H_
#define _HOSTNAME_COMMAND_H_

#include "CommandCommon.h"

struct HostnameCommand : public CommandBase {

	HostnameCommand(){
		Clear();
		SetCommand(CMD_NAME_HOSTNAME);
		setAcceptArgsOptions(true);
		setCmdOptionSeparator(CMD_OPTION_SEPERATOR_SPACE);
	}

	static void RegisterCommand(){
		CommandBase::RegisterCommand(CMD_NAME_HOSTNAME, [](void *arg)->void *{
			return pdiutil::safe_new<HostnameCommand>();
		});
	}

	const char* getUsage() const override {
		return RODT_ATTR("hostname  print the host name");
	}

	pdi_err_t execute(cmd_term_inseq_t terminputaction){

		if( nullptr == m_terminal ){
			return CMD_ERROR_NOTTY;
		}

		for( uint8_t i = 0; i < CMD_OPTION_MAX; i++ ){
			if( nullptr != m_options[i].optionval && 0 != m_options[i].optionvalsize ){
				return CMD_ERROR_OPT;
			}
		}

		pdiutil::string key = CHARPTR_WRAP(ENV_KEY_HOSTNAME);
		pdiutil::string host;
		Environment::get(key.c_str(), host);
		if( host.empty() ){
			return CMD_ERROR_FAILED;
		}

		m_terminal->putln();
		m_terminal->write(host.c_str());
		return PDI_OK;
	}
};

#endif
