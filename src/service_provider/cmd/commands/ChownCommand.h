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
 * e.g. chown 1001 /notes.txt        gid defaults to uid
 *      chown 1001:1001 /notes.txt   explicit gid via colon (POSIX)
 */
struct ChownCommand : public CommandBase {

	ChownCommand(){
		Clear();
		SetCommand(CMD_NAME_CHOWN);
		setArgs(RODT_ATTR(""), 2);
		setAcceptArgsOptions(true);
		setCmdOptionSeparator(CMD_OPTION_SEPERATOR_SPACE);
	}

	static void RegisterCommand(){
		CommandBase::RegisterCommand(CMD_NAME_CHOWN, [](void *arg)->void *{
			return pdiutil::safe_new<ChownCommand>();
		});
	}

	const char* getUsage() const override {
		return RODT_ATTR("chown <uid>[:<gid>] <path>  change ownership (root only; gid defaults to uid)");
	}

	bool needauth() override { return true; }

	pdi_err_t execute(cmd_term_inseq_t terminputaction){

		if( !__auth_service.getAuthorized() ) return CMD_ERROR_PERM;

		if( nullptr == m_terminal ) return CMD_ERROR_FAILED;

		CommandOption *uopt = &m_options[0];
		CommandOption *popt = &m_options[1];

		if( nullptr == uopt || nullptr == uopt->optionval || 0 >= uopt->optionvalsize ||
		    nullptr == popt || nullptr == popt->optionval || 0 >= popt->optionvalsize ){
			return CMD_ERROR_ARGS_MISSING;
		}

		// Parse "<uid>" or "<uid>:<gid>". Colon is optional; if absent, gid = uid.
		uint16_t colonAt = 0;
		bool hasColon = false;
		for( uint16_t i = 0; i < uopt->optionvalsize; i++ ){
			if( uopt->optionval[i] == ':' ){ colonAt = i; hasColon = true; break; }
		}

		uint32_t uid = 0;
		uint32_t gid = 0;
		bool uidok = ParseUint32(uopt->optionval, hasColon ? colonAt : (uint16_t)uopt->optionvalsize, uid) && uid <= 0xFFFF;
		gid = uid;
		bool gidok = !hasColon || (colonAt + 1) >= uopt->optionvalsize ||
					 ( ParseUint32(uopt->optionval + colonAt + 1, (uint16_t)(uopt->optionvalsize - colonAt - 1), gid) && gid <= 0xFFFF );
		if( !uidok || !gidok ){
			m_terminal->writeln();
			m_terminal->write_ro(RODT_ATTR("chown: invalid user : "));
			m_terminal->write(uopt->optionval, (uint32_t)uopt->optionvalsize);
			return CMD_ERROR_INVAL;
		}

		pdiutil::string path = resolveArgPath(popt);

		int rc = __i_fs.setFileOwner(path.c_str(), (uint16_t)uid, (uint16_t)gid);
		if( rc < 0 ){
			m_terminal->writeln();
			writeFailure(RODT_ATTR("chown: "), path.c_str(), (int32_t)rc);
			return CMD_ERROR_FAILED;
		}
		return PDI_OK;
	}
};

#endif

#endif
