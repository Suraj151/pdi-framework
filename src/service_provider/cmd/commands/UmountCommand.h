/******************************** Umount Command ******************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Detaches a filesystem mounted at runtime: umount <dir>, for root only. A mount
made at boot, or one holding open files or a session's working directory, is
left as it is and the reason given by its code.

Author          : Suraj I.
created Date    : 21st Oct 2026
******************************************************************************/
#ifndef _UMOUNT_COMMAND_H_
#define _UMOUNT_COMMAND_H_

#include "CommandCommon.h"

#ifdef ENABLE_STORAGE_SERVICE

#ifdef ENABLE_AUTH_SERVICE
#include <service_provider/user/UserStoreService.h>
#endif

struct UmountCommand : public CommandBase {

	UmountCommand(){
		Clear();
		SetCommand(CMD_NAME_UMOUNT);
		setAcceptArgsOptions(true);
	}

	static void RegisterCommand(){
		CommandBase::RegisterCommand(CMD_NAME_UMOUNT, [](void *arg)->void *{
			return pdiutil::safe_new<UmountCommand>();
		});
	}

	const char* getUsage() const override {
		return RODT_ATTR("umount <dir>  detach the filesystem mounted at dir");
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

		pdiutil::string dir = resolveArgPath(&m_options[0]);
		if( dir.empty() ){
			return CMD_ERROR_ARGS_MISSING;
		}

		__strip_trailing(dir, PATH_SEPARATOR_CHAR);

		pdi_err_t detached = PDI_ERR_PERM;
		if( SessionManager::isRoot() )
		{
			detached = __i_fs.umount(dir.c_str());
		}
		if( PDI_OK != detached ){
			m_terminal->putln();
			writeFailure(RODT_ATTR("umount: "), dir.c_str(), (int32_t)detached);
			m_terminal->putln();
			return CMD_ERROR_FAILED;
		}
		return PDI_OK;
	}
};

#endif

#endif
