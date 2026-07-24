/********************************* Host Command *******************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 23rd July 2026
******************************************************************************/

#ifndef _HOST_COMMAND_H_
#define _HOST_COMMAND_H_

#include "CommandCommon.h"

#ifdef ENABLE_NETWORK_SERVICE

#include <service_provider/network/NameResolver.h>

/**
 * host command
 *
 * Resolves a hostname to an IP address (IP literal / /etc/hosts / DNS):
 *   host example.com
 */
struct HostCommand : public CommandBase {

	/* Constructor */
	HostCommand(){
		Clear();
		SetCommand(CMD_NAME_HOST);
		setAcceptArgsOptions(true);
	}

	/**
     * @brief Register the command.
     */
    static void RegisterCommand(){
		CommandBase::RegisterCommand(CMD_NAME_HOST, [](void *arg)->void *{
			return new HostCommand();
		});
	}

	const char* getUsage() const override {
		return RODT_ATTR("host <name>  resolve a hostname to an IP (/etc/hosts, DNS)");
	}

#ifdef ENABLE_AUTH_SERVICE
	/* override the necesity of required permission */
	bool needauth() override { return true; }
#endif

	/* execute command with provided options */
	cmd_result_t execute(cmd_term_inseq_t terminputaction){

#ifdef ENABLE_AUTH_SERVICE
		// return in case authentication needed and not authorized yet
		if( needauth() && !__auth_service.getAuthorized()){
			return CMD_RESULT_NEED_AUTH;
		}
#endif

		if( nullptr == m_terminal ){
			return CMD_RESULT_TERMINAL_ERR;
		}

		CommandOption *cmdoptn = &m_options[0];
		if( nullptr == cmdoptn->optionval || cmdoptn->optionvalsize <= 0 ){
			return CMD_RESULT_ARGS_MISSING;
		}

		const char *blob = cmdoptn->optionval;
		int16_t nlen = 0;
		while( nlen < cmdoptn->optionvalsize && blob[nlen] != '\0' && blob[nlen] != ' ' ) nlen++;
		if( nlen == 0 ){
			return CMD_RESULT_ARGS_MISSING;
		}

		pdiutil::string name(blob, nlen);

		ipaddress_t ip;
		m_terminal->putln();
		if( NameResolver::resolve(name.c_str(), ip) ){
			pdiutil::string ipstr = ip;
			m_terminal->write(name.c_str());
			m_terminal->write_ro(RODT_ATTR(" has address "));
			m_terminal->writeln(ipstr.c_str());
		}else{
			m_terminal->write_ro(RODT_ATTR("host not found: "));
			m_terminal->writeln(name.c_str());
		}

		return CMD_RESULT_OK;
	}
};

#endif

#endif
