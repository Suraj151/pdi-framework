/********************************* Watch Command *******************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st June 2019
******************************************************************************/

#ifndef _WATCH_COMMAND_H_
#define _WATCH_COMMAND_H_

#include "CommandCommon.h"

/**
 * watch command
 * 
 * e.g. watch c=<command to execute with its options periodically>; i=<interval in milliseconds>; n=<number of iterations>
 */
struct WatchCommand : public CommandBase {

	/* Members */
	int32_t m_watchtaskid = -1;
	pdiutil::string m_commandtoexec;

	/* Constructor */
	WatchCommand(){
		Clear();
		SetCommand(CMD_NAME_WATCH);
		AddOption(CMD_OPTION_NAME_C);
		AddOption(CMD_OPTION_NAME_I);
		AddOption(CMD_OPTION_NAME_N);
		setCmdOptionSeparator(CMD_OPTION_SEPERATOR_SEMICOLON);
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
		uint32_t interval = 1000; // default 1 second
		int32_t numberofiterations = -1; // default infinite

		CommandOption *commandcmdoptn = RetrieveOption(CMD_OPTION_NAME_C);	
		CommandOption *intervalcmdoptn = RetrieveOption(CMD_OPTION_NAME_I);	
		CommandOption *numberofiterationscmdoptn = RetrieveOption(CMD_OPTION_NAME_N);

		bool isCommandProvided = ( nullptr != commandcmdoptn && nullptr != commandcmdoptn->optionval && commandcmdoptn->optionvalsize );
		bool isIntervalProvided = ( nullptr != intervalcmdoptn && nullptr != intervalcmdoptn->optionval && intervalcmdoptn->optionvalsize );
		bool isNumberOfIterationsProvided = ( nullptr != numberofiterationscmdoptn && nullptr != numberofiterationscmdoptn->optionval && numberofiterationscmdoptn->optionvalsize );

		if( !isCommandProvided ){
			return CMD_RESULT_ARGS_MISSING;
		}

		m_commandtoexec = pdiutil::string(commandcmdoptn->optionval, commandcmdoptn->optionvalsize);

		if( isIntervalProvided ){
			interval = StringToUint32(intervalcmdoptn->optionval, intervalcmdoptn->optionvalsize);
			interval = ( interval < 1000 ) ? 1000 : interval; // minimum 1 second
		}

		if( isNumberOfIterationsProvided ){
			numberofiterations = StringToUint32(numberofiterationscmdoptn->optionval, numberofiterationscmdoptn->optionvalsize);
			numberofiterations = ( numberofiterations < 1 ) ? 1 : numberofiterations; // minimum 1 iteration, else infinite
		}

		if( 0 != numberofiterations ){

			// remove previous task if any
			__task_scheduler.remove_task(m_watchtaskid);

			// register new task
			m_watchtaskid = __task_scheduler.register_task( [&]() {

				// clear the display
				m_terminal->csi_erase_display();

				// m_terminal->putln();
				// m_terminal->write((int32_t)m_watchtaskid);
				// m_terminal->write_ro(RODT_ATTR(" : "));
				// m_terminal->write((int32_t)__i_dvc_ctrl.millis_now());
				// m_terminal->write_ro(RODT_ATTR(" : "));
				// m_terminal->writeln(m_commandtoexec.c_str());
				// m_terminal->putln();

				if( m_cmdexecinterface != nullptr ){

					cmd_result_t cmdres = m_cmdexecinterface->executeCommand(&m_commandtoexec);

					if( cmdres != CMD_RESULT_OK ){
						m_terminal->writeln();
						m_terminal->write_ro(RODT_ATTR("CmdErr : "));
						m_terminal->writeln((int32_t)cmdres);
						__task_scheduler.remove_task(m_watchtaskid);
					}
				}
				
			}, interval, 0, __i_dvc_ctrl.millis_now(), numberofiterations );

			if( m_watchtaskid < 0 ){
				result = CMD_RESULT_FAILED;
			}
		}

		return result;
	}
};


#endif
