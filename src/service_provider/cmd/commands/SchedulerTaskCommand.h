/***************************** Scheduler Task Command *************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st June 2019
******************************************************************************/

#ifndef _SCHED_TASK_COMMAND_H_
#define _SCHED_TASK_COMMAND_H_

#include "CommandCommon.h"

/**
 * Scheduler tasks command
 * 
 * e.g. if we want to list tasks then use below command
 * scht
 */
struct SchedulerTaskCommand : public CommandBase {

	/* Constructor */
	SchedulerTaskCommand(){
		Clear();
		SetCommand(CMD_NAME_SCHED_TASK);
		setAcceptArgsOptions(true);
	}

	/* execute command with provided options */
	cmd_result_t execute(cmd_term_inseq_t terminputaction){

		cmd_result_t result = CMD_RESULT_OK;

		if(nullptr != m_terminal){

			// options will be available at first index
			CommandOption *cmdoptn = &m_options[0];


			if(nullptr != cmdoptn && nullptr != cmdoptn->optionval && cmdoptn->optionvalsize > 0){

				// check if list option provided
				if( __strstr(cmdoptn->optionval, CMD_OPTION_NAME_L, cmdoptn->optionvalsize) != -1 ){

					__task_scheduler.printTasksToTerminal(m_terminal);
				}
			}
		}

		return result;
	}
};


#endif
