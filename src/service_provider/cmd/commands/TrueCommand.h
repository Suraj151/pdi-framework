/******************************** True Command ********************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 17th Oct 2026
******************************************************************************/
#ifndef _TRUE_COMMAND_H_
#define _TRUE_COMMAND_H_

#include "CommandCommon.h"

struct TrueCommand : public CommandBase {

	TrueCommand(){
		Clear();
		SetCommand(CMD_NAME_TRUE);
		setAcceptArgsOptions(true);
	}

	static void RegisterCommand(){
		CommandBase::RegisterCommand(CMD_NAME_TRUE, [](void *arg)->void *{
			return pdiutil::safe_new<TrueCommand>();
		});
	}

	const char* getUsage() const override {
		return RODT_ATTR("true");
	}

	/**
	 * Answers yes and prints nothing, for a condition that always holds.
	 */
	pdi_err_t execute(cmd_term_inseq_t terminputaction){
		return PDI_OK;
	}
};

#endif
