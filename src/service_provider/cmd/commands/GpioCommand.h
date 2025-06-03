/********************************* GPIO Command *******************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st June 2019
******************************************************************************/

#ifndef _GPIO_COMMAND_H_
#define _GPIO_COMMAND_H_

#include "CommandCommon.h"

#ifdef ENABLE_GPIO_SERVICE
/**
 * gpio command
 * 
 * e.g. if we want to blink 4th gpio @500ms then we can execute command as below
 * gpio p=4, m=3, v=500
 */
struct GpioCommand : public CommandBase {

	/* Constructor */
	GpioCommand(){
		Clear();
		SetCommand(CMD_NAME_GPIO);
		AddOption(CMD_OPTION_NAME_P);
		AddOption(CMD_OPTION_NAME_M);
		AddOption(CMD_OPTION_NAME_V);
	}

#ifdef ENABLE_AUTH_SERVICE
	/* override the necesity of required permission */
	bool needauth() override { return true; }
#endif

	/* execute command with provided options */
	cmd_result_t execute(){

#ifdef ENABLE_AUTH_SERVICE
		// return in case authentication needed and not authorized yet
		if( needauth() && !__auth_service.getAuthorized()){
			return CMD_RESULT_NEED_AUTH;
		}
#endif

		cmd_result_t result = CMD_RESULT_OK;

		int16_t _pin = -1; 
		int16_t _mode = -1; 
		int32_t _value = -1; 
		// int32_t _strval = -1;

		CommandOption *cmdoptn = RetrieveOption(CMD_OPTION_NAME_P);		
		if( nullptr != cmdoptn ){
			_pin = StringToUint16(cmdoptn->optionval, cmdoptn->optionvalsize);
		}

		cmdoptn = RetrieveOption(CMD_OPTION_NAME_M);
		if( nullptr != cmdoptn ){
			_mode = StringToUint16(cmdoptn->optionval, cmdoptn->optionvalsize);
		}

		cmdoptn = RetrieveOption(CMD_OPTION_NAME_V);
		if( nullptr != cmdoptn ){
			_value = StringToUint16(cmdoptn->optionval, cmdoptn->optionvalsize);
		}

		if( _pin != -1 ){

			if( _mode != -1 ){
				
				__gpio_service.m_gpio_config_copy.gpio_mode[_pin] = _mode;
			}

			if( _value != -1 ){
				
				__gpio_service.m_gpio_config_copy.gpio_readings[_pin] = _value;
			}

			__database_service.set_gpio_config_table(&__gpio_service.m_gpio_config_copy);
			__gpio_service.handleGpioModes(GPIO_WRITE_CONFIG);
		}else{

			result = CMD_RESULT_ARGS_MISSING;
		}

		// LogI("\n");
		return result;
	}
};
#endif


#endif
