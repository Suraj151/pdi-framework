/******************************** Uname Command *******************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Prints what the system is, the way linux uname does: the system name with no
option, and with -s -n -r -v -m the system name, host name, release, version
and machine, in that order whatever order they are given in; -a prints all of
them.

Author          : Suraj I.
created Date    : 26th Oct 2026
******************************************************************************/
#ifndef _UNAME_COMMAND_H_
#define _UNAME_COMMAND_H_

#include "CommandCommon.h"

struct UnameCommand : public CommandBase {

	UnameCommand(){
		Clear();
		SetCommand(CMD_NAME_UNAME);
		setAcceptArgsOptions(true);
		setCmdOptionSeparator(CMD_OPTION_SEPERATOR_SPACE);
	}

	static void RegisterCommand(){
		CommandBase::RegisterCommand(CMD_NAME_UNAME, [](void *arg)->void *{
			return pdiutil::safe_new<UnameCommand>();
		});
	}

	const char* getUsage() const override {
		return RODT_ATTR("uname [-a|-s|-n|-r|-v|-m]  print system name, host, release, version, machine");
	}

	pdi_err_t execute(cmd_term_inseq_t terminputaction){

		if( nullptr == m_terminal ){
			return CMD_ERROR_NOTTY;
		}

		uint8_t fields = 0;
		for( uint8_t i = 0; i < CMD_OPTION_MAX; i++ ){
			const CommandOption *option = &m_options[i];
			if( nullptr == option->optionval || 0 == option->optionvalsize ){
				continue;
			}
			if( '-' != option->optionval[0] || option->optionvalsize < 2 ){
				return CMD_ERROR_OPT;
			}
			for( uint16_t c = 1; c < option->optionvalsize; c++ ){
				uint8_t bit = fieldBit(option->optionval[c]);
				if( 0 == bit ){
					return CMD_ERROR_OPT;
				}
				fields |= bit;
			}
		}
		if( 0 == fields ){
			fields = UNAME_FIELD_SYSTEM;
		}

		m_terminal->putln();
		bool first = true;
		for( uint8_t bit = UNAME_FIELD_SYSTEM; bit <= UNAME_FIELD_MACHINE; bit <<= 1 ){
			if( 0 == (fields & bit) ){
				continue;
			}
			if( !first ){
				m_terminal->write(" ");
			}
			pdiutil::string value;
			fieldValue(bit, value);
			m_terminal->write(value.c_str());
			first = false;
		}
		return PDI_OK;
	}

private:

	/**
	 * The field an option letter names, as its bit; 0 for none, every field for a.
	 */
	uint8_t fieldBit(char letter) const {
		switch( letter ){
			case 'a': return UNAME_FIELD_ALL;
			case 's': return UNAME_FIELD_SYSTEM;
			case 'n': return UNAME_FIELD_HOST;
			case 'r': return UNAME_FIELD_RELEASE;
			case 'v': return UNAME_FIELD_VERSION;
			case 'm': return UNAME_FIELD_MACHINE;
			default: return 0;
		}
	}

	/**
	 * The text one field prints, `unknown` when the device cannot report it.
	 */
	void fieldValue(uint8_t bit, pdiutil::string &out) const {
		switch( bit ){
			case UNAME_FIELD_SYSTEM:
				out = CHARPTR_WRAP(UNAME_SYSTEM_NAME);
				break;
			case UNAME_FIELD_HOST: {
				pdiutil::string key = CHARPTR_WRAP(ENV_KEY_HOSTNAME);
				Environment::get(key.c_str(), out);
				break;
			}
			case UNAME_FIELD_RELEASE:
				out = CHARPTR_WRAP(RELEASE);
				break;
			case UNAME_FIELD_VERSION:
				out = CHARPTR_WRAP(CONFIG_VERSION);
				break;
			case UNAME_FIELD_MACHINE: {
				device_info_t info;
				__i_dvc_ctrl.getDeviceInfo(info);
				out = info.m_model;
				break;
			}
			default:
				break;
		}
		if( out.empty() ){
			out = CHARPTR_WRAP(UNAME_UNKNOWN);
		}
	}
};

#endif
