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
		setArgs(RODT_ATTR(""), 2);
		setAcceptArgsOptions(true);
		setCmdOptionSeparator(CMD_OPTION_SEPERATOR_SPACE);
	}

	static void RegisterCommand(){
		CommandBase::RegisterCommand(CMD_NAME_CHMOD, [](void *arg)->void *{
			return pdiutil::safe_new<ChmodCommand>();
		});
	}

	const char* getUsage() const override {
		return RODT_ATTR("chmod <octal> <path>  set permission bits (owner or root)");
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
		CommandOption *popt = &m_options[1];

		if( nullptr == mopt || nullptr == mopt->optionval || 0 >= mopt->optionvalsize ||
		    nullptr == popt || nullptr == popt->optionval || 0 >= popt->optionvalsize ){
			return CMD_ERROR_ARGS_MISSING;
		}

		uint32_t perms = 0;
		if( !ParseOctal(mopt->optionval, (uint16_t)mopt->optionvalsize, 0777, perms) ){
			m_terminal->writeln();
			m_terminal->write_ro(RODT_ATTR("chmod: invalid mode : "));
			m_terminal->write(mopt->optionval, (uint32_t)mopt->optionvalsize);
			return CMD_ERROR_INVAL;
		}

		pdiutil::string path = resolveArgPath(popt);

		int rc = __i_fs.setFilePermissions(path.c_str(), (uint16_t)perms);
		if( rc < 0 ){
			m_terminal->writeln();
			writeFailure(RODT_ATTR("chmod: "), path.c_str(), (int32_t)rc);
			return CMD_ERROR_FAILED;
		}
		return PDI_OK;
	}
};

#endif

#endif
