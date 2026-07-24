/****************************** Timedatectl Command ***************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 23rd July 2026
******************************************************************************/

#ifndef _TIMEDATECTL_COMMAND_H_
#define _TIMEDATECTL_COMMAND_H_

#include "CommandCommon.h"
#include <utility/DataTypeConversions.h>
#include <interface/pdi/middlewares/iNtpInterface.h>

/**
 * timedatectl command
 *
 * Read-only clock status: local time, UTC, time zone, NTP sync state
 * and the configured NTP server.
 */
struct TimedatectlCommand : public CommandBase {

	/* Constructor */
	TimedatectlCommand(){
		Clear();
		SetCommand(CMD_NAME_TIMEDATECTL);
	}

	/**
     * @brief Register the command.
     */
    static void RegisterCommand(){
		CommandBase::RegisterCommand(CMD_NAME_TIMEDATECTL, [](void *arg)->void *{
			return new TimedatectlCommand();
		});
	}

	const char* getUsage() const override {
		return RODT_ATTR("tdctl  timedatectl: show clock status (local/UTC time, zone, NTP sync)");
	}

	/* execute command with provided options */
	cmd_result_t execute(cmd_term_inseq_t terminputaction){

		if( nullptr == m_terminal ){
			return CMD_RESULT_TERMINAL_ERR;
		}

		bool synced = __i_ntp.is_valid_ntptime();
		uint32_t utcEpoch = synced ? (uint32_t)__i_ntp.get_ntp_time() : 0;
		uint32_t localEpoch = synced ? utcEpoch + (uint32_t)TZ_SEC : 0;

		char localbuf[24], utcbuf[24];
		EpochToDateTimeString(localEpoch, localbuf, sizeof(localbuf));
		EpochToDateTimeString(utcEpoch, utcbuf, sizeof(utcbuf));

		m_terminal->putln();
		m_terminal->write_ro(RODT_ATTR("Local time      : "));
		m_terminal->writeln(localbuf);
		m_terminal->write_ro(RODT_ATTR("Universal time  : "));
		m_terminal->writeln(utcbuf);
		m_terminal->write_ro(RODT_ATTR("Time zone       : "));
		m_terminal->writeln_ro(RODT_ATTR(TZ_Asia_Kolkata));
		m_terminal->write_ro(RODT_ATTR("NTP synchronized: "));
		m_terminal->writeln_ro(synced ? RODT_ATTR("yes") : RODT_ATTR("no"));
		m_terminal->write_ro(RODT_ATTR("NTP server      : "));
		m_terminal->writeln_ro(RODT_ATTR(NTP_SERVER1));

		return CMD_RESULT_OK;
	}
};

#endif
