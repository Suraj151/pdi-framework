/******************************** Command base ********************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st June 2019
******************************************************************************/

#ifndef _COMMAND_BASE_H_
#define _COMMAND_BASE_H_

#include "StringOperations.h"
#include "iIOInterface.h"


/* command results */
typedef enum {
    CMD_STATUS_OK = 0,
    CMD_STATUS_ARGS_ERROR,
    CMD_STATUS_ARGS_MISSING,
    CMD_STATUS_NOT_FOUND,
    CMD_STATUS_INVALID,
    CMD_STATUS_INVALID_OPTION,
    CMD_STATUS_EMPTY,
    CMD_STATUS_NEED_AUTH,
    CMD_STATUS_WRONG_CREDENTIAL,
    CMD_STATUS_MAX
} cmd_status_t;

/* commands constants */
#define CMD_SIZE_MAX 				8
#define CMD_OPTION_MAX 				3
#define CMD_OPTION_SIZE_MAX 		3
#define CMD_OPTION_SEPERATOR 		","
#define CMD_OPTION_ASSIGN_OPERATOR 	"="

/* command base struct type */
typedef struct CommandBase {

	/* command option struct */
	struct CommandOption
	{
		char option[CMD_OPTION_SIZE_MAX];
		char *optionval;
		int16_t optionvalsize;

		/* Constructor */
		CommandOption(){
			Clear();
		}

		/* Clear */
		void Clear(){
			memset(option, 0, CMD_OPTION_SIZE_MAX);
			optionval = nullptr;
			optionvalsize = 0;
		}
	};
	
	/* members */
	char cmd[CMD_SIZE_MAX];
	CommandOption options[CMD_OPTION_MAX];
	uint8_t optionindx;
	iTerminalInterface *m_terminal;

	/* Constructor */
	CommandBase(){
		// Clear();
	}

	/* Set terminal type */
	void SetTerminal(iTerminalInterface *terminal){
		m_terminal = terminal;
	}

	/* Set command */
	bool SetCommand(const char* _cmd){

		if(nullptr != _cmd){

			int16_t cmdsize = strlen(_cmd);

			if( cmdsize < CMD_SIZE_MAX ){
				memcpy(cmd, _cmd, cmdsize);
				return true;
			}
		}
		return false;
	}

	/* Add available option for this command */
	bool AddOption(const char* _optn){

		if(nullptr != _optn){

			int16_t optnsize = strlen(_optn);

			if( optionindx < CMD_OPTION_MAX && optnsize < CMD_OPTION_SIZE_MAX ){
				memcpy(options[optionindx].option, _optn, optnsize);
				optionindx++;
				return true;
			}
		}
		return false;
	}

	/* check whether passed argument is matching with current/this command */
	bool isValidCommand(char* _cmd){
		// return ((nullptr != _cmd) && __are_str_equals(cmd, _cmd, CMD_SIZE_MAX));
		return ((nullptr != _cmd) && __are_arrays_equal(cmd, _cmd, strlen(cmd)));
	}

	/* check whether passed argument is matching with available options and return its index */
	int8_t isValidOption(char* _optn){

		for (uint8_t i = 0; nullptr != _optn && i < CMD_OPTION_MAX; i++){

			if(__are_str_equals(options[i].option, _optn, CMD_OPTION_SIZE_MAX)){
				return i;
			}
		}
		return -1;
	}

	/* parse passed command and argument is matching with available options */
	cmd_status_t parseCmdOptions(char* _args){

		cmd_status_t status = CMD_STATUS_EMPTY;

		if(_args != nullptr){

			int16_t cmd_max_len = strlen(_args);
			int16_t cmd_start_indx = __strstr(_args, cmd);
			int16_t cmd_end_indx = __strstr(_args+cmd_start_indx, " ");
			cmd_end_indx = cmd_end_indx < 0 ? cmd_max_len : (cmd_start_indx+cmd_end_indx);

			if( cmd_start_indx >= 0 && cmd_start_indx < cmd_end_indx && cmd_end_indx <= cmd_max_len ){

				char argcmd[CMD_SIZE_MAX];
				memset(argcmd, 0, CMD_SIZE_MAX);
				memcpy(argcmd, _args+cmd_start_indx, abs(cmd_end_indx-cmd_start_indx));

				if( isValidCommand(argcmd) ){

					char argoptn[CMD_OPTION_SIZE_MAX];
					
					// get the option start and end indices
					int16_t optn_start_indx = cmd_end_indx;
					int16_t optn_end_indx = __strstr(_args+optn_start_indx, CMD_OPTION_ASSIGN_OPERATOR);
					optn_end_indx += optn_end_indx != -1 ? optn_start_indx : 0;

					if( optn_end_indx != -1 && optionindx > 0 ){

						if( optn_end_indx < cmd_max_len && optn_start_indx < optn_end_indx ){

							do
							{
								memset(argoptn, 0, CMD_OPTION_SIZE_MAX);
								memcpy(argoptn, _args+optn_start_indx, optn_end_indx-optn_start_indx);

								// get the option value start and end indices
								int16_t optn_val_start_index = optn_end_indx+strlen(CMD_OPTION_ASSIGN_OPERATOR);
								int16_t optn_val_end_index = __strstr(_args+optn_val_start_index, CMD_OPTION_SEPERATOR);
								optn_val_end_index += optn_val_end_index != -1 ? optn_val_start_index : cmd_max_len+1;
								char *argoptntrimmed = __strtrim(argoptn);

								int8_t validoptnindex = isValidOption(argoptntrimmed);
								if( validoptnindex != -1 ){

									options[validoptnindex].optionval = __strtrim(_args+optn_val_start_index);
									options[validoptnindex].optionvalsize = optn_val_end_index - optn_val_start_index;
									status = CMD_STATUS_OK;
								}else{
									status = CMD_STATUS_INVALID_OPTION;
									break;
								}
								
								// next option start index will start with last option value end index
								optn_start_indx = optn_val_end_index+strlen(CMD_OPTION_SEPERATOR);
								// optn_start_indx += optn_start_indx != -1 ? (optn_val_end_index+strlen(CMD_OPTION_SEPERATOR)) : 0;

								optn_end_indx = __strstr(_args+optn_start_indx, CMD_OPTION_ASSIGN_OPERATOR);
								optn_end_indx += optn_end_indx != -1 ? optn_start_indx : 0;
							} while ( optn_start_indx > 0 && optn_end_indx > 0 && optn_end_indx < cmd_max_len && optn_start_indx < optn_end_indx);

						}else{
							status = CMD_STATUS_ARGS_ERROR;
						}
					}else{
						// if command dont have any options by default
						status = CMD_STATUS_OK;
					}
				}else{
					status = CMD_STATUS_INVALID;
				}
			}else{
				status = CMD_STATUS_NOT_FOUND;
			}
		}

		/* execute command if format is ok */
		if( CMD_STATUS_OK == status ){
			if( nullptr != m_terminal ){
				m_terminal->write_ro(RODT_ATTR("Exec command : "));
				m_terminal->write(cmd);
				m_terminal->write(RODT_ATTR("\n"));
			}
			status = execute();
		}

		// once executed clear the option value and their value
		for (uint8_t i = 0; i < CMD_OPTION_MAX; i++){
			options[i].optionval = nullptr;
			options[i].optionvalsize = 0;
		}

		return status;
	}

	/* Clear */
	void Clear(){
		memset(cmd, 0, CMD_SIZE_MAX);
		for (uint8_t i = 0; i < CMD_OPTION_MAX; i++){
			options[i].Clear();
		}
		optionindx = 0;
		m_terminal = nullptr;
	}

	/* derived command can implement auth necesity */
	virtual bool needauth() { return false; }

	/* derived command should implement execute */
	virtual cmd_status_t execute() = 0;

} cmd_t;

#endif
