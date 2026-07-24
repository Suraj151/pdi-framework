/********************************* Date Command *******************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 23rd July 2026
******************************************************************************/

#ifndef _DATE_COMMAND_H_
#define _DATE_COMMAND_H_

#include "CommandCommon.h"
#include <utility/DataTypeConversions.h>
#include <interface/pdi/middlewares/iNtpInterface.h>

/**
 * date command
 *
 * Prints the current date/time from the NTP clock:
 *   date            local time, "%Y-%m-%d %H:%M:%S"
 *   date -u         UTC time
 *   date +%H:%M     custom format (see EpochToDateTimeString)
 *   date -u +%H:%M  UTC time in a custom format
 */
struct DateCommand : public CommandBase {

	/* Constructor */
	DateCommand(){
		Clear();
		SetCommand(CMD_NAME_DATE);
		setAcceptArgsOptions(true);
	}

	/**
     * @brief Register the command.
     */
    static void RegisterCommand(){
		CommandBase::RegisterCommand(CMD_NAME_DATE, [](void *arg)->void *{
			return new DateCommand();
		});
	}

	const char* getUsage() const override {
		return RODT_ATTR("date [-u] [-n] [-s <epoch>] [+format]  show, sync or set clock (NTP)");
	}

	/* execute command with provided options */
	cmd_result_t execute(cmd_term_inseq_t terminputaction){

		if( nullptr == m_terminal ){
			return CMD_RESULT_TERMINAL_ERR;
		}

		bool utc = false;
		bool customfmt = false;
		bool doset = false;
		bool dosync = false;
		uint32_t setepoch = 0;
		const char *fmt = "%Y-%m-%d %H:%M:%S";

		CommandOption *cmdoptn = &m_options[0];
		if( nullptr != cmdoptn->optionval && cmdoptn->optionvalsize > 0 ){
			const char *blob = cmdoptn->optionval;
			// the arg buffer is null-terminated, so a '+format' with spaces
			// stays intact as a single trailing token
			int16_t len = 0;
			while( len < cmdoptn->optionvalsize && blob[len] != '\0' ) len++;

			int16_t i = 0;
			while( i < len && blob[i] == ' ' ) i++;
			while( i+1 < len && blob[i] == '-' ){
				char flag = blob[i+1];
				if( flag == 'u' && (i+2 >= len || blob[i+2] == ' ') ){
					utc = true;
					i += 2;
				}else if( flag == 'n' && (i+2 >= len || blob[i+2] == ' ') ){
					dosync = true;
					i += 2;
				}else if( flag == 's' ){
					doset = true;
					i += 2;
					while( i < len && blob[i] == ' ' ) i++;
					int16_t start = i;
					while( i < len && blob[i] != ' ' ) i++;
					if( i > start ){
						setepoch = StringToUint32(blob + start, (uint8_t)(i - start));
					}
				}else{
					break;
				}
				while( i < len && blob[i] == ' ' ) i++;
			}
			if( i < len && blob[i] == '+' ){
				fmt = blob + i + 1;
				customfmt = true;
			}
		}

		if( dosync ){
#ifdef ENABLE_AUTH_SERVICE
			if( !__auth_service.getAuthorized() ){
				return CMD_RESULT_NEED_AUTH;
			}
#endif
			__i_ntp.init_ntp_time();
			m_terminal->putln();
			m_terminal->writeln_ro(RODT_ATTR("ntp sync triggered"));
			return CMD_RESULT_OK;
		}

		if( doset ){
#ifdef ENABLE_AUTH_SERVICE
			if( !__auth_service.getAuthorized() ){
				return CMD_RESULT_NEED_AUTH;
			}
#endif
			if( setepoch <= (uint32_t)LAUNCH_UNIX_TIME ){
				return CMD_RESULT_ARGS_ERROR;
			}
			if( !__i_ntp.set_ntp_time((pdiutil::epoch_time_t)setepoch) ){
				m_terminal->putln();
				m_terminal->writeln_ro(RODT_ATTR("failed to set time"));
				return CMD_RESULT_FAILED;
			}
		}

		if( !__i_ntp.is_valid_ntptime() ){
			m_terminal->putln();
			m_terminal->writeln_ro(RODT_ATTR("time not synced"));
			return CMD_RESULT_OK;
		}

		uint32_t epoch = (uint32_t)__i_ntp.get_ntp_time();
		if( !utc ){
			epoch += (uint32_t)TZ_SEC;
		}

		char datebuf[48];
		EpochToDateTimeString(epoch, datebuf, sizeof(datebuf), fmt);

		m_terminal->putln();
		m_terminal->write(datebuf);
		if( utc && !customfmt ){
			m_terminal->write_ro(RODT_ATTR(" UTC"));
		}
		m_terminal->writeln();
		return CMD_RESULT_OK;
	}
};

#endif
