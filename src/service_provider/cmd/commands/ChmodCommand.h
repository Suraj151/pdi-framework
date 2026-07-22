/******************************* Chmod Command ********************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 20th July 2026
******************************************************************************/

#ifndef _CHMOD_COMMAND_H_
#define _CHMOD_COMMAND_H_

#include "CommandCommon.h"

#ifdef ENABLE_STORAGE_SERVICE

/**
 * chmod command — set POSIX permission bits on a file or directory.
 * Owner or root only (enforced by VfsDispatcher).
 *
 * e.g. chmod 0644 /etc/passwd
 */
struct ChmodCommand : public CommandBase {

	ChmodCommand(){
		Clear();
		SetCommand(CMD_NAME_CHMOD);
		setAcceptArgsOptions(true);
		setCmdOptionSeparator(CMD_OPTION_SEPERATOR_SPACE);
	}

	static void RegisterCommand(){
		CommandBase::RegisterCommand(CMD_NAME_CHMOD, [](void *arg)->void *{
			return new ChmodCommand();
		});
	}

	const char* getUsage() const override {
		return RODT_ATTR("chmod <octal> <path>  set permission bits (owner or root)");
	}

#ifdef ENABLE_AUTH_SERVICE
	bool needauth() override { return true; }
#endif

	cmd_result_t execute(cmd_term_inseq_t terminputaction){

#ifdef ENABLE_AUTH_SERVICE
		if( needauth() && !__auth_service.getAuthorized() ){
			return CMD_RESULT_NEED_AUTH;
		}
#endif

		if( nullptr == m_terminal ) return CMD_RESULT_FAILED;

		CommandOption *mopt = &m_options[0];
		CommandOption *popt = &m_options[1];

		if( nullptr == mopt || nullptr == mopt->optionval || 0 >= mopt->optionvalsize ||
		    nullptr == popt || nullptr == popt->optionval || 0 >= popt->optionvalsize ){
			return CMD_RESULT_ARGS_ERROR;
		}

		uint16_t perms = StringToOctalUint16(mopt->optionval, (uint8_t)mopt->optionvalsize) & 0777;

		pdiutil::string path = resolveArgPath(popt);

		int rc = __i_fs.setFilePermissions(path.c_str(), perms);
		if( rc < 0 ){
			m_terminal->writeln();
			m_terminal->write_ro(RODT_ATTR("chmod: permission denied or not found"));
			return CMD_RESULT_FAILED;
		}
		return CMD_RESULT_OK;
	}
};

#endif

#endif
