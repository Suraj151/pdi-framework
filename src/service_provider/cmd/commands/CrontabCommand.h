/****************************** Crontab Command *******************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 7th Sep 2026
******************************************************************************/
#ifndef _CRONTAB_COMMAND_H_
#define _CRONTAB_COMMAND_H_

#include "CommandCommon.h"

#ifdef ENABLE_CRON_SERVICE

#include <service_provider/cron/CronServiceProvider.h>
#include <service_provider/time/TimeServiceProvider.h>

struct CrontabCommand : public CommandBase {

	CrontabCommand(){
		Clear();
		SetCommand(CMD_NAME_CRONTAB);
		setArgs(RODT_ATTR(""), 0);
	}

	static void RegisterCommand(){
		CommandBase::RegisterCommand(CMD_NAME_CRONTAB, [](void *arg)->void *{
			return pdiutil::safe_new<CrontabCommand>();
		});
	}

	const char* getUsage() const override {
		return RODT_ATTR("crontab  show the jobs the table holds and what is due now");
	}

#ifdef ENABLE_AUTH_SERVICE
	bool needauth() override { return true; }
#endif

	pdi_err_t execute(cmd_term_inseq_t terminputaction){

#ifdef ENABLE_AUTH_SERVICE
		if( needauth() && !__auth_service.getAuthorized() ){
			return CMD_ERROR_PERM;
		}
#endif

		if( nullptr == m_terminal ){
			return CMD_ERROR_NOTTY;
		}

		pdiutil::string table = CHARPTR_WRAP(CRONTAB_FILE_PATH);

		m_terminal->putln();

		if( !__i_fs.isFileExist(table.c_str()) ){
			m_terminal->writeln_ro(RODT_ATTR("no crontab"));
			return PDI_OK;
		}

		const datetime_t &now = __time_service.now();

		if( !now.m_valid ){
			m_terminal->writeln_ro(RODT_ATTR("clock not valid, jobs are held"));
		}

		uint8_t listed = 0;

		for( int32_t n = 0; listed < CRON_MAX_ENTRIES; n++ ){

			pdiutil::string line;

			__i_dvc_ctrl.yield();

			if( __i_fs.readLineInFile(table.c_str(), n, line, nullptr, [](void){
					__i_dvc_ctrl.yield();
				}) < 0 ){
				break;
			}

			pdiutil::string fields[CRON_FIELD_COUNT];
			pdiutil::string command;

			if( !CronServiceProvider::parseRow(line, fields, command) ){
				continue;
			}

			listed++;

			pdiutil::string row;

			for( uint8_t i = 0; i < CRON_FIELD_COUNT; i++ ){
				row += fields[i];
				row += " ";
			}

			row += command;

			m_terminal->writeln(row.c_str());
		}

		if( 0 == listed ){
			m_terminal->writeln_ro(RODT_ATTR("no jobs"));
			return PDI_OK;
		}

		if( now.m_valid ){

			m_terminal->putln();
			m_terminal->writeln_ro(RODT_ATTR("due this minute:"));

			if( 0 == __cron_service.runDueJobs(now, true, m_terminal) ){
				m_terminal->writeln_ro(RODT_ATTR("  none"));
			}
		}

		return PDI_OK;
	}
};

#endif

#endif
