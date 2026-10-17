/******************************* False Command ********************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 17th Oct 2026
******************************************************************************/
#ifndef _FALSE_COMMAND_H_
#define _FALSE_COMMAND_H_

#include "CommandCommon.h"

struct FalseCommand : public CommandBase {

	FalseCommand(){
		Clear();
		SetCommand(CMD_NAME_FALSE);
		setAcceptArgsOptions(true);
	}

	static void RegisterCommand(){
		CommandBase::RegisterCommand(CMD_NAME_FALSE, [](void *arg)->void *{
			return pdiutil::safe_new<FalseCommand>();
		});
	}

	const char* getUsage() const override {
		return RODT_ATTR("false");
	}

	/**
	 * Answers no and prints nothing, for a condition that never holds.
	 */
	pdi_err_t execute(cmd_term_inseq_t terminputaction){
		return CMD_RESULT_FALSE;
	}
};

#endif
