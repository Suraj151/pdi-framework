/*******************************  Network Command *****************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st June 2019
******************************************************************************/

#ifndef _NETWORK_COMMAND_H_
#define _NETWORK_COMMAND_H_

#include "CommandCommon.h"

#ifdef ENABLE_NETWORK_SERVICE

/**
 * Network command
 * e.g. if we want to get device network detail then we can execute command as below
 * net <options> 
 */
struct NetworkCommand : public CommandBase {

	/* Constructor */
	NetworkCommand(){
		Clear();
		SetCommand(CMD_NAME_NETWORK);
		setAcceptArgsOptions(true);
		setCmdOptionSeparator(CMD_OPTION_SEPERATOR_SPACE);
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

		cmd_result_t result = CMD_RESULT_OK;
		CommandOption *cmdoptn1 = &m_options[0];
		int maxoptionlen = 5;

		if( nullptr != cmdoptn1 && nullptr != cmdoptn1->optionval && cmdoptn1->optionvalsize > 0
			&& cmdoptn1->optionvalsize < maxoptionlen ){

			if(0 == strncmp(cmdoptn1->optionval, "ip", 2)){

				ServiceProvider *srvc = ServiceProvider::getService(SERVICE_WIFI);

				if(nullptr != srvc){
					
					srvc->printStatusToTerminal(m_terminal);
				}
			}
		}

		return result;
	}
};
#endif

#endif
