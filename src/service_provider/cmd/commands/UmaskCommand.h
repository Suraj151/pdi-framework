/******************************* Umask Command ********************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 20th July 2026
******************************************************************************/

#ifndef _UMASK_COMMAND_H_
#define _UMASK_COMMAND_H_

#include "CommandCommon.h"

#ifdef ENABLE_STORAGE_SERVICE

/**
 * umask command — get or set the current session's umask (POSIX octal).
 * Umask is applied at file/dir creation as (default_perms & ~umask).
 *
 * e.g. umask         print current umask
 *      umask m=0022  set new umask
 */
struct UmaskCommand : public CommandBase {

	UmaskCommand(){
		Clear();
		SetCommand(CMD_NAME_UMASK);
		setArgs(RODT_ATTR(""), 1);
		setAcceptArgsOptions(true);
		setCmdOptionSeparator(CMD_OPTION_SEPERATOR_SPACE);
	}

	static void RegisterCommand(){
		CommandBase::RegisterCommand(CMD_NAME_UMASK, [](void *arg)->void *{
			return pdiutil::safe_new<UmaskCommand>();
		});
	}

	const char* getUsage() const override {
		return RODT_ATTR("umask [<octal>]  print or set current session's umask");
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

		if( nullptr == m_terminal ) return CMD_ERROR_FAILED;

		CommandOption *mopt = &m_options[0];
		if( optionGiven(mopt) ){
			uint32_t newMask = 0;
			if( !ParseOctal(mopt->optionval, (uint16_t)mopt->optionvalsize, 0777, newMask) ){
				m_terminal->writeln();
				m_terminal->write_ro(RODT_ATTR("umask: invalid mode : "));
				m_terminal->write(mopt->optionval, (uint32_t)mopt->optionvalsize);
				return CMD_ERROR_INVAL;
			}
			SessionManager::setCurrentUmask((uint16_t)newMask);
		}

		uint16_t cur = SessionManager::getCurrentUmask();
		char buf[6];
		buf[0] = '0';
		buf[1] = (char)('0' + ((cur >> 6) & 7));
		buf[2] = (char)('0' + ((cur >> 3) & 7));
		buf[3] = (char)('0' + (cur & 7));
		buf[4] = '\0';
		m_terminal->writeln();
		m_terminal->writeln(buf);

		return PDI_OK;
	}
};

#endif

#endif
