/******************************** Useradd Command *****************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 18th July 2026
******************************************************************************/
#ifndef _USERADD_COMMAND_H_
#define _USERADD_COMMAND_H_

#include "CommandCommon.h"

#if defined(ENABLE_AUTH_SERVICE) && defined(ENABLE_STORAGE_SERVICE)
#include <service_provider/user/UserStoreService.h>

struct UseraddCommand : public CommandBase {

	UseraddCommand(){
		Clear();
		SetCommand(CMD_NAME_USERADD);
		AddOption(CMD_OPTION_NAME_U);
		AddOption(CMD_OPTION_NAME_P);
		setCmdOptionSeparator(CMD_OPTION_SEPERATOR_SPACE);
	}

	static void RegisterCommand(){
		CommandBase::RegisterCommand(CMD_NAME_USERADD, [](void *arg)->void *{
			return pdiutil::safe_new<UseraddCommand>();
		});
	}

	const char* getUsage() const override {
		return RODT_ATTR("useradd u=<user> p=<pass>  root-only; create a new user");
	}

	bool needauth() override { return true; }
	bool wantsMaskedInput() override { return isWaitingForOption(CMD_OPTION_NAME_P); }

	pdi_err_t execute(cmd_term_inseq_t terminputaction){

		if( needauth() && !__auth_service.getAuthorized() ){
			return CMD_ERROR_PERM;
		}

		if( terminputaction == CMD_TERM_INSEQ_CTRL_C ||
		    terminputaction == CMD_TERM_INSEQ_CTRL_Z ){
			return CMD_ERROR_CANCELED;
		}

		const char *me = __auth_service.getUsername();
		if( nullptr == me || 0 == me[0] ) return CMD_ERROR_PERM;
		if( !SessionManager::isRoot() ){
			if( nullptr != m_terminal ){
				m_terminal->putln();
				m_terminal->writeln_ro(RODT_ATTR(CMD_MSG_ROOT_REQUIRED));
			}
			return CMD_ERROR_FAILED;
		}

		CommandOption *uOpt = RetrieveOption(CMD_OPTION_NAME_U);
		CommandOption *pOpt = RetrieveOption(CMD_OPTION_NAME_P);

		bool hasU = ( nullptr != uOpt && nullptr != uOpt->optionval && uOpt->optionvalsize );
		bool hasP = ( nullptr != pOpt && nullptr != pOpt->optionval && pOpt->optionvalsize );

		if( !hasU ){
			setWaitingForOption(CMD_OPTION_NAME_U);
			if( nullptr != m_terminal ){
				m_terminal->putln();
				m_terminal->write_ro(RODT_ATTR("user: "));
			}
			return CMD_ERROR_AGAIN;
		}

		char uname[USER_STORE_MAX_USERNAME_LEN];
		memset(uname, 0, USER_STORE_MAX_USERNAME_LEN);
		uint16_t ulen = uOpt->optionvalsize < USER_STORE_MAX_USERNAME_LEN - 1 ? uOpt->optionvalsize : USER_STORE_MAX_USERNAME_LEN - 1;
		memcpy(uname, uOpt->optionval, ulen);

		if( ulen != uOpt->optionvalsize || !UserStoreService::isValidUsername(uname) ){
			if( nullptr != m_terminal ){
				m_terminal->putln();
				m_terminal->writeln_ro(RODT_ATTR("invalid user name"));
			}
			return CMD_ERROR_FAILED;
		}

		user_record_t existing;
		if( __user_store_service.findUserByName(uname, existing) ){
			if( nullptr != m_terminal ){
				m_terminal->putln();
				m_terminal->writeln_ro(RODT_ATTR("user exists"));
			}
			return CMD_ERROR_FAILED;
		}

		holdOptionValue(CMD_OPTION_NAME_U);

		if( !hasP ){
			setWaitingForOption(CMD_OPTION_NAME_P);
			if( nullptr != m_terminal ){
				m_terminal->putln();
				m_terminal->write_ro(RODT_ATTR("Pass : "));
			}
			return CMD_ERROR_AGAIN;
		}

		char pw[LOGIN_CONFIGS_BUF_SIZE];
		memset(pw, 0, LOGIN_CONFIGS_BUF_SIZE);
		uint16_t plen = pOpt->optionvalsize < LOGIN_CONFIGS_BUF_SIZE - 1 ? pOpt->optionvalsize : LOGIN_CONFIGS_BUF_SIZE - 1;
		memcpy(pw, pOpt->optionval, plen);

		uint16_t nextUid = USER_STORE_ROOT_UID + 1;
		user_record_t probe;
		while( __user_store_service.findUserByUid(nextUid, probe) ) nextUid++;

		user_record_t newRec;
		newRec.m_uid = nextUid;
		newRec.m_gid = nextUid;
		newRec.m_username = uname;
		UserStoreService::homePath(uname, newRec.m_home);
		newRec.m_shell = CHARPTR_WRAP(USER_STORE_DEFAULT_SHELL);

		if( __user_store_service.addUser(newRec, pw) ){
			pdi_err_t homed = __user_store_service.makeHome(newRec);
			if( homed < 0 && PDI_ERR_EXISTS != homed ){
				__user_store_service.removeUser(uname);
				if( nullptr != m_terminal ){
					m_terminal->putln();
					m_terminal->write_ro(RODT_ATTR("cannot create home "));
					m_terminal->write(newRec.m_home.c_str());
				}
				return homed;
			}
			if( nullptr != m_terminal ){
				m_terminal->putln();
				m_terminal->write_ro(RODT_ATTR("added "));
				m_terminal->write(uname);
				if( PDI_ERR_EXISTS == homed ){
					m_terminal->putln();
					m_terminal->write_ro(RODT_ATTR("home exists, kept as it is: "));
					m_terminal->write(newRec.m_home.c_str());
				}
			}
			return PDI_OK;
		}

		return CMD_ERROR_FAILED;
	}
};

#endif

#endif
