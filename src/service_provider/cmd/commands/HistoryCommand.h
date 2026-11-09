/******************************* History Command ******************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Lists the lines the shell keeps in its history, numbered from the oldest the
way bash numbers them; history N lists only the last N.

Author          : Suraj I.
created Date    : 7th Nov 2026
******************************************************************************/
#ifndef _HISTORY_COMMAND_H_
#define _HISTORY_COMMAND_H_

#include "CommandCommon.h"

#ifdef ENABLE_STORAGE_SERVICE

#define HISTORY_COL_NUMBER    5       /* line number column */

struct HistoryCommand : public CommandBase {

	HistoryCommand(){
		Clear();
		SetCommand(CMD_NAME_HISTORY);
		setArgs(RODT_ATTR(""), 1);
		setAcceptArgsOptions(true);
		setCmdOptionSeparator(CMD_OPTION_SEPERATOR_SPACE);
	}

	static void RegisterCommand(){
		CommandBase::RegisterCommand(CMD_NAME_HISTORY, [](void *arg)->void *{
			return pdiutil::safe_new<HistoryCommand>();
		});
	}

	const char* getUsage() const override {
		return RODT_ATTR("history [n]  list the lines typed before, numbered, or only the last n");
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

		uint32_t wanted = UINT32_MAX;
		if( optionGiven(&m_options[0]) ){
			pdi_err_t taken = takeNumber(RODT_ATTR("history: invalid count : "), m_options[0].optionval,
										 m_options[0].optionvalsize, 0, INT32_MAX, wanted);
			if( PDI_OK != taken ){
				return taken;
			}
		}

		pdiutil::string path = CHARPTR_WRAP(CMD_TERMINAL_HISTORY_FILEPATH);
		if( !__i_fs.isFileExist(path.c_str()) ){
			return PDI_OK;
		}

		uint32_t total = 0;
		readCommandLines(path, m_terminal, [&](const pdiutil::string &line)->bool{
			total++;
			return true;
		});

		uint32_t skip = (wanted < total) ? (total - wanted) : 0;
		uint32_t number = 0;
		char digits[12];
		int rc = readCommandLines(path, m_terminal, [&](const pdiutil::string &line)->bool{
			if( ++number <= skip ){
				return true;
			}
			memset(digits, 0, sizeof(digits));
			Uint32ToString(number, digits, sizeof(digits));
			m_terminal->putln();
			m_terminal->write_pad(digits, HISTORY_COL_NUMBER, true);
			m_terminal->write("  ");
			m_terminal->write(line.c_str());
			return true;
		});
		return (rc < 0) ? (pdi_err_t)CMD_ERROR_FAILED : stoppedResult(PDI_OK);
	}
};

#endif

#endif
