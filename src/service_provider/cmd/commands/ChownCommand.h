/******************************* Chown Command ********************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 20th July 2026
******************************************************************************/

#ifndef _CHOWN_COMMAND_H_
#define _CHOWN_COMMAND_H_

#include "CommandCommon.h"

#if defined(ENABLE_STORAGE_SERVICE) && defined(ENABLE_AUTH_SERVICE)

/**
 * chown command — change owning uid (and optionally gid) of a file/dir.
 * Root only (enforced by VfsDispatcher).
 *
 * e.g. chown 1001 /home/alice        gid defaults to uid
 *      chown 1001:1001 /home/alice   explicit gid via colon (POSIX)
 */
struct ChownCommand : public CommandBase {

	ChownCommand(){
		Clear();
		SetCommand(CMD_NAME_CHOWN);
		setAcceptArgsOptions(true);
		setCmdOptionSeparator(CMD_OPTION_SEPERATOR_SPACE);
	}

	static void RegisterCommand(){
		CommandBase::RegisterCommand(CMD_NAME_CHOWN, [](void *arg)->void *{
			return new ChownCommand();
		});
	}

	const char* getUsage() const override {
		return RODT_ATTR("chown <uid>[:<gid>] <path>  change ownership (root only; gid defaults to uid)");
	}

	bool needauth() override { return true; }

	cmd_result_t execute(cmd_term_inseq_t terminputaction){

		if( !__auth_service.getAuthorized() ) return CMD_RESULT_NEED_AUTH;

		if( nullptr == m_terminal ) return CMD_RESULT_FAILED;

		CommandOption *uopt = &m_options[0];
		CommandOption *popt = &m_options[1];

		if( nullptr == uopt || nullptr == uopt->optionval || 0 == uopt->optionvalsize ||
		    nullptr == popt || nullptr == popt->optionval || 0 == popt->optionvalsize ){
			return CMD_RESULT_ARGS_ERROR;
		}

		// Parse "<uid>" or "<uid>:<gid>". Colon is optional; if absent, gid = uid.
		uint16_t colonAt = 0;
		bool hasColon = false;
		for( uint16_t i = 0; i < uopt->optionvalsize; i++ ){
			if( uopt->optionval[i] == ':' ){ colonAt = i; hasColon = true; break; }
		}

		uint16_t uid = StringToUint16(uopt->optionval, hasColon ? (uint8_t)colonAt : (uint8_t)uopt->optionvalsize);
		uint16_t gid = uid;
		if( hasColon && (colonAt + 1) < uopt->optionvalsize ){
			gid = StringToUint16(uopt->optionval + colonAt + 1, (uint8_t)(uopt->optionvalsize - colonAt - 1));
		}

		char path[FILE_NAME_MAX_SIZE + 1];
		memset(path, 0, sizeof(path));
		uint16_t plen = popt->optionvalsize < sizeof(path) - 1 ? popt->optionvalsize : sizeof(path) - 1;
		memcpy(path, popt->optionval, plen);

		int rc = __i_fs.setFileOwner(path, uid, gid);
		if( rc < 0 ){
			m_terminal->writeln();
			m_terminal->write_ro(RODT_ATTR("chown: permission denied or not found"));
			return CMD_RESULT_FAILED;
		}
		return CMD_RESULT_OK;
	}
};

#endif

#endif
