/***************************** Device Iot Command *****************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st June 2019
******************************************************************************/

#ifndef _DEVICE_IOT_COMMAND_H_
#define _DEVICE_IOT_COMMAND_H_

#include "CommandCommon.h"

#ifdef ENABLE_DEVICE_IOT

#define DEVICE_IOT_CMD_OPTION_SETDUID		"setid"
#define DEVICE_IOT_CMD_OPTION_GETDUID		"getid"
#define DEVICE_IOT_CMD_OPTION_SETHOST		"sethost"
#define DEVICE_IOT_CMD_OPTION_GETHOST		"gethost"

#define DEVICE_IOT_CMD_OPTION_MAXLEN		10

/**
 * set duid command
 *
 * e.g. if we want to get/set the device unique identifier, we can execute command as below
 * iot <options>
 */
struct DeviceIotCommand : public CommandBase {

	/* Constructor */
	DeviceIotCommand(){
		Clear();
		SetCommand(CMD_NAME_IOT);
		setAcceptArgsOptions(true);
		setCmdOptionSeparator(CMD_OPTION_SEPERATOR_COMMA);
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
		CommandOption *cmdoptn1 = &m_options[0];

		if( nullptr != cmdoptn1 && nullptr != cmdoptn1->optionval && cmdoptn1->optionvalsize > 0
			&& cmdoptn1->optionvalsize < DEVICE_IOT_CMD_OPTION_MAXLEN ){

			if( 
				cmdoptn1->optionvalsize == (sizeof(DEVICE_IOT_CMD_OPTION_GETDUID)-1) && 
				0 == strncmp(cmdoptn1->optionval, DEVICE_IOT_CMD_OPTION_GETDUID, strlen(DEVICE_IOT_CMD_OPTION_GETDUID)) 
			){

				device_iot_config_table _device_iot_configs;
				__database_service.get_device_iot_config_table(&_device_iot_configs);

				m_terminal->putln();
				m_terminal->write_ro(RODT_ATTR("DUID : "));
				m_terminal->writeln(_device_iot_configs.device_iot_duid);

			}else if( 
				cmdoptn1->optionvalsize == (sizeof(DEVICE_IOT_CMD_OPTION_SETDUID)-1) && 
				0 == strncmp(cmdoptn1->optionval, DEVICE_IOT_CMD_OPTION_SETDUID, strlen(DEVICE_IOT_CMD_OPTION_SETDUID)) 
			){
				CommandOption *cmdoptn2 = &m_options[1];

				if( nullptr != cmdoptn2 && nullptr != cmdoptn2->optionval && cmdoptn2->optionvalsize > 0
					&& cmdoptn2->optionvalsize < DEVICE_IOT_DUID_MAX_LENGTH ){

					device_iot_config_table _device_iot_configs;
					__database_service.get_device_iot_config_table(&_device_iot_configs);
					memset(_device_iot_configs.device_iot_duid, 0, DEVICE_IOT_DUID_MAX_LENGTH);
					strncpy(_device_iot_configs.device_iot_duid, cmdoptn2->optionval, cmdoptn2->optionvalsize);
					__database_service.set_device_iot_config_table(&_device_iot_configs);						
				}else{
					// ssid is must
					return CMD_RESULT_ARGS_MISSING;
				}
			}else if( 
				cmdoptn1->optionvalsize == (sizeof(DEVICE_IOT_CMD_OPTION_GETHOST)-1) && 
				0 == strncmp(cmdoptn1->optionval, DEVICE_IOT_CMD_OPTION_GETHOST, strlen(DEVICE_IOT_CMD_OPTION_GETHOST)) 
			){

				device_iot_config_table _device_iot_configs;
				__database_service.get_device_iot_config_table(&_device_iot_configs);

				m_terminal->putln();
				m_terminal->write_ro(RODT_ATTR("Host : "));
				m_terminal->writeln(_device_iot_configs.device_iot_host);
			}else if( 
				cmdoptn1->optionvalsize == (sizeof(DEVICE_IOT_CMD_OPTION_SETHOST)-1) && 
				0 == strncmp(cmdoptn1->optionval, DEVICE_IOT_CMD_OPTION_SETHOST, strlen(DEVICE_IOT_CMD_OPTION_SETHOST)) 
			){
				
				CommandOption *cmdoptn2 = &m_options[1];

				wifi_config_table _wifi_credentials;
				__database_service.get_wifi_config_table( &_wifi_credentials );

				if( nullptr != cmdoptn2 && nullptr != cmdoptn2->optionval && cmdoptn2->optionvalsize > 0
					&& cmdoptn2->optionvalsize < DEVICE_IOT_HOST_BUF_SIZE ){

					device_iot_config_table _device_iot_configs;
					__database_service.get_device_iot_config_table(&_device_iot_configs);
					memset(_device_iot_configs.device_iot_host, 0, DEVICE_IOT_HOST_BUF_SIZE);
					strncpy(_device_iot_configs.device_iot_host, cmdoptn2->optionval, cmdoptn2->optionvalsize);
					__database_service.set_device_iot_config_table(&_device_iot_configs);						
				}else{
					// ssid is must
					return CMD_RESULT_ARGS_MISSING;
				}
			}
		}

		return result;
	}
};
#endif


#endif
