/********************************  Reboot Command *****************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st June 2019
******************************************************************************/

#ifndef _REBOOT_COMMAND_H_
#define _REBOOT_COMMAND_H_

#include "CommandCommon.h"

/**
 * Reboot command
 * e.g. if we want to restart the device, we can execute command as below
 * reboot
 */
struct RebootCommand : public CommandBase {

	/* Constructor */
	RebootCommand(){
		Clear();
		SetCommand(CMD_NAME_REBOOT);
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

		if(nullptr != m_terminal){
			m_terminal->putln();
			m_terminal->writeln_ro(RODT_ATTR("Rebooting Now..."));
			__i_dvc_ctrl.restartDevice();
		}

		return result;
	}
};


#endif
