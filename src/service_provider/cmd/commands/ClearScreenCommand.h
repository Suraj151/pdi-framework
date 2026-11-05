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
		setArgs(RODT_ATTR(""), 0);
	}

	/**
     * @brief Register the command.
     */
    static void RegisterCommand(){
		CommandBase::RegisterCommand(CMD_NAME_CLS, [](void *arg)->void *{
			return pdiutil::safe_new<ClearScreenCommand>();
		});
	}

	const char* getUsage() const override {
		return RODT_ATTR("cls  clear the terminal screen");
	}

	/* execute command with provided options */
	pdi_err_t execute(cmd_term_inseq_t terminputaction){

		pdi_err_t result = PDI_OK;

		if(nullptr != m_terminal){

			// clear the display
			m_terminal->csi_erase_display();
		}

		return result;
	}
};


#endif
