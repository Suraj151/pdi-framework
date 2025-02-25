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
    CMD_RESULT_OK = 0,
    CMD_RESULT_ARGS_ERROR,
    CMD_RESULT_ARGS_MISSING,
    CMD_RESULT_NOT_FOUND,
    CMD_RESULT_INVALID,
    CMD_RESULT_INVALID_OPTION,
    CMD_RESULT_NEED_AUTH,
    CMD_RESULT_INCOMPLETE, // may need more input from user
    CMD_RESULT_WRONG_CREDENTIAL,
    CMD_RESULT_MAX
} cmd_result_t;

/* command status */
typedef enum {
    CMD_STATUS_IDLE = 0,
    CMD_STATUS_ACTIVE,
    CMD_STATUS_INACTIVE,
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
		bool holdingoptn;

		/* Constructor */
		CommandOption() : optionval(nullptr), optionvalsize(0), holdingoptn(false){
			Clear(true);
		}

		/* Clear */
		void Clear(bool deep=false){
			if(holdingoptn && nullptr != optionval && optionvalsize){
				delete[] optionval;
			}
			optionval = nullptr;
			optionvalsize = 0;
			holdingoptn = false;
			if(deep){
				memset(option, 0, CMD_OPTION_SIZE_MAX);
			}
		}
	};
	
	/* members */
	char m_cmd[CMD_SIZE_MAX];
	CommandOption m_options[CMD_OPTION_MAX];
	uint8_t m_optionindx;
	int8_t m_waitingoptionindx;
	iTerminalInterface *m_terminal;
	cmd_status_t m_status;
	cmd_result_t m_result;


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
				memcpy(m_cmd, _cmd, cmdsize);
				return true;
			}
		}
		return false;
	}

	/* Add available option for this command */
	bool AddOption(const char* _optn){

		if(nullptr != _optn){

			int16_t optnsize = strlen(_optn);

			if( m_optionindx < CMD_OPTION_MAX && optnsize < CMD_OPTION_SIZE_MAX ){
				memcpy(m_options[m_optionindx].option, _optn, optnsize);
				m_optionindx++;
				return true;
			}
		}
		return false;
	}

	/* retrieve available option from parsed command line data */
	CommandOption* RetrieveOption(const char* _optn){

		if( nullptr != _optn ){

			for (uint8_t i = 0; i < CMD_OPTION_MAX; i++)
			{
				if (m_options[i].optionval != nullptr && m_options[i].optionvalsize > 0 &&
					__are_str_equals(m_options[i].option, _optn, CMD_OPTION_SIZE_MAX)
				){

					return &m_options[i];
				}
			}
		}
		return nullptr;
	}

	/* check whether passed argument is matching with current/this command */
	bool isValidCommand(char* _cmd){
		// return ((nullptr != _cmd) && __are_str_equals(cmd, _cmd, CMD_SIZE_MAX));
		return ((nullptr != _cmd) && __are_arrays_equal(m_cmd, _cmd, strlen(m_cmd)));
	}

	/* check whether passed argument is matching with available options and return its index */
	int8_t getOptionIndex(const char* _optn){

		for (uint8_t i = 0; nullptr != _optn && i < CMD_OPTION_MAX; i++){

			if(__are_str_equals(m_options[i].option, _optn, CMD_OPTION_SIZE_MAX)){
				return i;
			}
		}
		return -1;
	}

	/* check whether passed argument is valid option */
	bool isValidOption(const char* _optn){
		return (getOptionIndex(_optn) != -1);
	}

	/* return if command is waiting for option */
	bool isWaitingForOption(){
		return (m_waitingoptionindx != -1);
	}

	/* Set flag which indicates that command need more input from user */
	void setWaitingForOption(const char* _optn){
		m_waitingoptionindx = getOptionIndex(_optn);
	}

	/* hold the option value if provided */
	bool holdOptionValue(const char* _optn){

		bool bStatus = false;
		int8_t optindx = getOptionIndex(_optn);

		if( optindx != -1 && optindx < m_optionindx ){

			if( !m_options[optindx].holdingoptn &&
				nullptr != m_options[optindx].optionval && 
				m_options[optindx].optionvalsize ){

				char *val = new char[m_options[optindx].optionvalsize+2]();
				memset(val, 0, m_options[optindx].optionvalsize+2);
				memcpy(val, m_options[optindx].optionval, m_options[optindx].optionvalsize);
				m_options[optindx].optionval = val;
				m_options[optindx].holdingoptn = true;
			}

			bStatus = m_options[optindx].holdingoptn;
		}

		return bStatus;
	}

	/* parse passed command and argument is matching with available options */
	cmd_result_t executeCommand(char* _args, int16_t _len, bool _waiting_option = false){

		m_result = CMD_RESULT_MAX;

		if(_args != nullptr){

			if( !_waiting_option ){

				int16_t cmd_max_len = _len;
				int16_t cmd_start_indx = __strstr(_args, m_cmd);
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

						if( optn_end_indx != -1 && m_optionindx > 0 ){

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

									int8_t validoptnindex = getOptionIndex(argoptntrimmed);
									if( validoptnindex != -1 ){

										m_options[validoptnindex].optionval = __strtrim(_args+optn_val_start_index);
										m_options[validoptnindex].optionvalsize = optn_val_end_index - optn_val_start_index;
										m_result = CMD_RESULT_OK;
									}else{
										m_result = CMD_RESULT_INVALID_OPTION;
										break;
									}
									
									// next option start index will start with last option value end index
									optn_start_indx = optn_val_end_index+strlen(CMD_OPTION_SEPERATOR);
									// optn_start_indx += optn_start_indx != -1 ? (optn_val_end_index+strlen(CMD_OPTION_SEPERATOR)) : 0;

									optn_end_indx = __strstr(_args+optn_start_indx, CMD_OPTION_ASSIGN_OPERATOR);
									optn_end_indx += optn_end_indx != -1 ? optn_start_indx : 0;
								} while ( optn_start_indx > 0 && optn_end_indx > 0 && optn_end_indx < cmd_max_len && optn_start_indx < optn_end_indx);

							}else{
								m_result = CMD_RESULT_ARGS_ERROR;
							}
						}else{
							// if command dont have any options by default
							m_result = CMD_RESULT_OK;
						}
					}else{
						m_result = CMD_RESULT_INVALID;
					}
				}else{
					m_result = CMD_RESULT_NOT_FOUND;
				}
			}else{

				if( m_waitingoptionindx != -1 && m_waitingoptionindx < m_optionindx ){

					m_options[m_waitingoptionindx].optionval = _args;
					m_options[m_waitingoptionindx].optionvalsize = _len;
					m_waitingoptionindx = -1;
					m_result = CMD_RESULT_OK;
				}else{

				}
			}
		}

		/* execute command if format is ok */
		if( CMD_RESULT_OK == m_result ){
			
			// if( nullptr != m_terminal ){
			// 	m_terminal->write_ro(RODT_ATTR("Executing cmd : "));
			// 	m_terminal->write(m_cmd);
			// 	m_terminal->write(RODT_ATTR("\n"));
			// }
			
			m_status = CMD_STATUS_ACTIVE;
			m_result = execute();
		}

		if( CMD_RESULT_INCOMPLETE != m_result ){
			
			m_status = CMD_STATUS_INACTIVE;
			ResultToTerminal(m_result);

			// once executed clear the options
			ClearOptions();
		}

		return m_result;
	}

	/* Clear */
	void Clear(){
		memset(m_cmd, 0, CMD_SIZE_MAX);
		ClearOptions(true);
		m_optionindx = 0;
		m_terminal = nullptr;
		m_status = CMD_STATUS_MAX;
		m_result = CMD_RESULT_MAX;
	}

	/* Clear options */
	void ClearOptions(bool deep=false){
		for (uint8_t i = 0; i < CMD_OPTION_MAX; i++){
			m_options[i].Clear(deep);
		}
		if(deep)
			m_waitingoptionindx = -1;
	}

	/* put final result on terminal */
	void ResultToTerminal(cmd_result_t res){

		if( nullptr != m_terminal && CMD_RESULT_INCOMPLETE != res && !isWaitingForOption() ){

			m_terminal->writeln();

			switch (res)
			{
			case CMD_RESULT_ARGS_ERROR:
				m_terminal->write_ro(RODT_ATTR("Arg Error"));
				break;
			case CMD_RESULT_ARGS_MISSING:
				m_terminal->write_ro(RODT_ATTR("Arg Missing"));
				break;
			case CMD_RESULT_NOT_FOUND:
				m_terminal->write_ro(RODT_ATTR("CMD Not Found"));
				break;
			case CMD_RESULT_INVALID:
				m_terminal->write_ro(RODT_ATTR("CMD invalid"));
				break;
			case CMD_RESULT_INVALID_OPTION:
				m_terminal->write_ro(RODT_ATTR("Option invalid"));
				break;
			case CMD_RESULT_NEED_AUTH:
				m_terminal->write_ro(RODT_ATTR("Required login"));
				break;
			case CMD_RESULT_WRONG_CREDENTIAL:
				m_terminal->write_ro(RODT_ATTR("Wrong Credential"));
				break;
			case CMD_RESULT_MAX:
				m_terminal->write_ro(RODT_ATTR("Unknown"));
				break;
			case CMD_RESULT_OK:
				m_terminal->write_ro(RODT_ATTR("Success"));
				break;
			default:
				break;
			}

			m_terminal->writeln();
		}
	}

	/* derived command can implement auth necesity */
	virtual bool needauth() { return false; }

	/* derived command should implement execute */
	virtual cmd_result_t execute() = 0;

} cmd_t;

#endif
