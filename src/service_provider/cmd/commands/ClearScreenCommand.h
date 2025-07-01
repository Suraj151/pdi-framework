/****************************** Clear Screen Command *************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st June 2019
******************************************************************************/

#ifndef _CLEAR_SCREEN_COMMAND_H_
#define _CLEAR_SCREEN_COMMAND_H_

#include "CommandCommon.h"

/**
 * clear screen command
 * 
 * e.g. if we want to clear the terminal screen, we can execute command as below
 * cls
 */
struct ClearScreenCommand : public CommandBase {

	/* Constructor */
	ClearScreenCommand(){
		Clear();
		SetCommand(CMD_NAME_CLS);
	}

	/* execute command with provided options */
	cmd_result_t execute(cmd_term_inseq_t terminputaction){

		cmd_result_t result = CMD_RESULT_OK;

		if(nullptr != m_terminal){

			// clear the display
			m_terminal->csi_erase_display();
		}

		return result;
	}
};


#endif
