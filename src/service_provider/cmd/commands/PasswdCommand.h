/******************************** Passwd Command ******************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 18th July 2026
******************************************************************************/
#ifndef _PASSWD_COMMAND_H_
#define _PASSWD_COMMAND_H_

#include "CommandCommon.h"

#if defined(ENABLE_AUTH_SERVICE) && defined(ENABLE_STORAGE_SERVICE)
#include <service_provider/user/UserStoreService.h>

struct PasswdCommand : public CommandBase {

	PasswdCommand(){
		Clear();
		SetCommand(CMD_NAME_PASSWD);
		AddOption(CMD_OPTION_NAME_P);
		AddOption(CMD_OPTION_NAME_N);
		AddOption(CMD_OPTION_NAME_C);
		setCmdOptionSeparator(CMD_OPTION_SEPERATOR_SPACE);
	}

	static void RegisterCommand(){
		CommandBase::RegisterCommand(CMD_NAME_PASSWD, [](void *arg)->void *{
			return new PasswdCommand();
		});
	}

	const char* getUsage() const override {
		return RODT_ATTR("passwd [p=<curr> n=<new> c=<confirm>]  change own password (echo-off prompts)");
	}

	bool needauth() override { return true; }

	bool wantsMaskedInput() override {
		return isWaitingForOption(CMD_OPTION_NAME_P) ||
		       isWaitingForOption(CMD_OPTION_NAME_N) ||
		       isWaitingForOption(CMD_OPTION_NAME_C);
	}

	cmd_result_t execute(cmd_term_inseq_t terminputaction){

		if( needauth() && !__auth_service.getAuthorized() ){
			return CMD_RESULT_NEED_AUTH;
		}

		const char *username = __auth_service.getUsername();
		if( nullptr == username || 0 == username[0] ) return CMD_RESULT_FAILED;

		if( terminputaction == CMD_TERM_INSEQ_CTRL_C ||
		    terminputaction == CMD_TERM_INSEQ_CTRL_Z ){
			return CMD_RESULT_ABORTED;
		}

		CommandOption *pOpt = RetrieveOption(CMD_OPTION_NAME_P);
		CommandOption *nOpt = RetrieveOption(CMD_OPTION_NAME_N);
		CommandOption *cOpt = RetrieveOption(CMD_OPTION_NAME_C);

		bool hasP = ( nullptr != pOpt && nullptr != pOpt->optionval && pOpt->optionvalsize );
		bool hasN = ( nullptr != nOpt && nullptr != nOpt->optionval && nOpt->optionvalsize );
		bool hasC = ( nullptr != cOpt && nullptr != cOpt->optionval && cOpt->optionvalsize );

		if( !hasP ){
			setWaitingForOption(CMD_OPTION_NAME_P);
			if( nullptr != m_terminal ) m_terminal->write_ro(RODT_ATTR("\ncurrent: "));
			return CMD_RESULT_INCOMPLETE;
		}

		char curPw[LOGIN_CONFIGS_BUF_SIZE];
		memset(curPw, 0, LOGIN_CONFIGS_BUF_SIZE);
		uint16_t clen = pOpt->optionvalsize < LOGIN_CONFIGS_BUF_SIZE - 1 ? pOpt->optionvalsize : LOGIN_CONFIGS_BUF_SIZE - 1;
		memcpy(curPw, pOpt->optionval, clen);

		if( !__auth_service.isAuthorized(username, curPw) ){
			return CMD_RESULT_WRONG_CREDENTIAL;
		}

		holdOptionValue(CMD_OPTION_NAME_P);

		if( !hasN ){
			setWaitingForOption(CMD_OPTION_NAME_N);
			if( nullptr != m_terminal ) m_terminal->write_ro(RODT_ATTR("\nnew: "));
			return CMD_RESULT_INCOMPLETE;
		}

		holdOptionValue(CMD_OPTION_NAME_N);

		if( !hasC ){
			setWaitingForOption(CMD_OPTION_NAME_C);
			if( nullptr != m_terminal ) m_terminal->write_ro(RODT_ATTR("\nconfirm: "));
			return CMD_RESULT_INCOMPLETE;
		}

		if( nOpt->optionvalsize != cOpt->optionvalsize ||
		    0 != memcmp(nOpt->optionval, cOpt->optionval, nOpt->optionvalsize) ){
			if( nullptr != m_terminal ) m_terminal->writeln_ro(RODT_ATTR("\nmismatch"));
			return CMD_RESULT_FAILED;
		}

		char newPw[LOGIN_CONFIGS_BUF_SIZE];
		memset(newPw, 0, LOGIN_CONFIGS_BUF_SIZE);
		uint16_t nlen = nOpt->optionvalsize < LOGIN_CONFIGS_BUF_SIZE - 1 ? nOpt->optionvalsize : LOGIN_CONFIGS_BUF_SIZE - 1;
		memcpy(newPw, nOpt->optionval, nlen);

		if( __user_store_service.setPassword(username, newPw) ){
			if( nullptr != m_terminal ) m_terminal->writeln_ro(RODT_ATTR("\npassword updated"));
			return CMD_RESULT_OK;
		}

		return CMD_RESULT_FAILED;
	}
};

#endif

#endif
