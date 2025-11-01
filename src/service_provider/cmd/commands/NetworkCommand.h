/*******************************  Network Command *****************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st June 2019
******************************************************************************/

#ifndef _NETWORK_COMMAND_H_
#define _NETWORK_COMMAND_H_

#include "CommandCommon.h"

#ifdef ENABLE_NETWORK_SERVICE

#define NETWORK_CMD_OPTION_IP			"ip"
#define NETWORK_CMD_OPTION_SCANSTA		"scansta"
#define NETWORK_CMD_OPTION_CONNSTA		"connsta"

#define NETWORK_CMD_OPTION_MAXLEN		10


/**
 * Network command
 * e.g. if we want to get device network detail then we can execute command as below
 * net <options> 
 */
struct NetworkCommand : public CommandBase {

	/* Constructor */
	NetworkCommand(){
		Clear();
		SetCommand(CMD_NAME_NETWORK);
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
			&& cmdoptn1->optionvalsize < NETWORK_CMD_OPTION_MAXLEN ){

			if( 
				cmdoptn1->optionvalsize == (sizeof(NETWORK_CMD_OPTION_IP)-1) && 
				0 == strncmp(cmdoptn1->optionval, NETWORK_CMD_OPTION_IP, strlen(NETWORK_CMD_OPTION_IP)) 
			){
				ServiceProvider *srvc = ServiceProvider::getService(SERVICE_WIFI);

				if(nullptr != srvc){
					
					srvc->printStatusToTerminal(m_terminal);
				}
			}else if( 
				cmdoptn1->optionvalsize == (sizeof(NETWORK_CMD_OPTION_SCANSTA)-1) && 
				0 == strncmp(cmdoptn1->optionval, NETWORK_CMD_OPTION_SCANSTA, (sizeof(NETWORK_CMD_OPTION_SCANSTA)-1)) 
			){
				m_terminal->putln();
				m_terminal->writeln_ro(RODT_ATTR("Scanning ....!"));

				int8_t foundnet = __i_wifi.scanNetworks();

				m_terminal->write_ro(RODT_ATTR("Scan completed. Found networks: "));
				m_terminal->writeln((int32_t)foundnet);
				m_terminal->putln();

				for(int8_t i=0; i<foundnet; i++){
					m_terminal->write_ro(RODT_ATTR("Network "));
					m_terminal->write((int32_t)(i+1));
					m_terminal->write_ro(RODT_ATTR(": "));
					m_terminal->write_ro(__i_wifi.SSID(i).c_str());
					m_terminal->write_ro(RODT_ATTR(", RSSI: "));
					m_terminal->write((int32_t)__i_wifi.RSSI(i));
					m_terminal->write_ro(RODT_ATTR(" dBm, BSSID: "));
					uint8_t *bssid = __i_wifi.BSSID(i);
					if(nullptr != bssid){
						char macstr[36] = {0};
						snprintf(macstr, sizeof(macstr), "%02X:%02X:%02X:%02X:%02X:%02X", bssid[0], bssid[1], bssid[2], bssid[3], bssid[4], bssid[5]);
						m_terminal->write(macstr);
					}
					m_terminal->write_ro(RODT_ATTR("\r\n"));
				}
			}else if( 
				cmdoptn1->optionvalsize == (sizeof(NETWORK_CMD_OPTION_CONNSTA)-1) && 
				0 == strncmp(cmdoptn1->optionval, NETWORK_CMD_OPTION_CONNSTA, (sizeof(NETWORK_CMD_OPTION_CONNSTA)-1)) 
			){
				CommandOption *cmdoptn2 = &m_options[1];
				CommandOption *cmdoptn3 = &m_options[2];

				wifi_config_table _wifi_credentials;
				__database_service.get_wifi_config_table( &_wifi_credentials );

				if( nullptr != cmdoptn2 && nullptr != cmdoptn2->optionval && cmdoptn2->optionvalsize > 0
					&& cmdoptn2->optionvalsize < WIFI_CONFIGS_BUF_SIZE ){
					memset(_wifi_credentials.sta_ssid, 0, WIFI_CONFIGS_BUF_SIZE);
					strncpy(_wifi_credentials.sta_ssid, cmdoptn2->optionval, cmdoptn2->optionvalsize);
				}else{
					// ssid is must
					return CMD_RESULT_ARGS_MISSING;
				}

				if( nullptr != cmdoptn3 && nullptr != cmdoptn3->optionval && cmdoptn3->optionvalsize > 0 
					&& cmdoptn3->optionvalsize < WIFI_CONFIGS_BUF_SIZE ){
					memset(_wifi_credentials.sta_password, 0, WIFI_CONFIGS_BUF_SIZE);
					strncpy(_wifi_credentials.sta_password, cmdoptn3->optionval, cmdoptn3->optionvalsize);
				}else{
					// password can be empty for open network
					memset(_wifi_credentials.sta_password, 0, WIFI_CONFIGS_BUF_SIZE);
				}

				m_terminal->putln();
				m_terminal->write_ro(RODT_ATTR("Connecting to "));
				m_terminal->writeln(_wifi_credentials.sta_ssid);

				__database_service.set_wifi_config_table( &_wifi_credentials );
				__wifi_service.stopService();
				__wifi_service.initService(&__i_wifi);

				m_terminal->writeln_ro(RODT_ATTR("Check Station Status after 10 second using 'net ip' command"));
			}
		}

		return result;
	}
};
#endif

#endif
