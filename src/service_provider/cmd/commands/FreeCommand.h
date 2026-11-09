/******************************** Free Command ********************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Prints the free memory and the largest piece of it as a table: in KiB with
no option, bytes with -b, KiB with -k and short human sizes with -h; the last
of them given wins.

Author          : Suraj I.
created Date    : 7th Nov 2026
******************************************************************************/
#ifndef _FREE_COMMAND_H_
#define _FREE_COMMAND_H_

#include "CommandCommon.h"

#define FREE_COL_LABEL    7       /* row label column */
#define FREE_COL_VALUE    12      /* each value column */

struct FreeCommand : public CommandBase {

	FreeCommand(){
		Clear();
		SetCommand(CMD_NAME_FREE);
		setArgs(RODT_ATTR("bkh"), 0);
		setAcceptArgsOptions(true);
		setCmdOptionSeparator(CMD_OPTION_SEPERATOR_SPACE);
	}

	static void RegisterCommand(){
		CommandBase::RegisterCommand(CMD_NAME_FREE, [](void *arg)->void *{
			return pdiutil::safe_new<FreeCommand>();
		});
	}

	const char* getUsage() const override {
		return RODT_ATTR("free [-b|-k|-h]  free memory and its largest block (KiB, -b bytes, -h human)");
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

		char unit = 'k';
		for( uint8_t i = 0; i < CMD_OPTION_MAX; i++ ){
			const CommandOption *option = &m_options[i];
			if( nullptr == option->optionval || 0 == option->optionvalsize ){
				continue;
			}
			if( '-' != option->optionval[0] || option->optionvalsize < 2 ){
				return CMD_ERROR_OPT;
			}
			unit = option->optionval[option->optionvalsize - 1];
		}

		uint32_t freebytes = __i_dvc_ctrl.get_free_heap();
		uint32_t largest = __i_dvc_ctrl.get_max_free_block();

		m_terminal->putln();
		m_terminal->write_pad_ro(RODT_ATTR(""), 0, FREE_COL_LABEL);
		m_terminal->write_pad_ro(RODT_ATTR("free"), 4, FREE_COL_VALUE, true);
		m_terminal->write_pad_ro(RODT_ATTR("largest"), 7, FREE_COL_VALUE, true);
		m_terminal->putln();
		m_terminal->write_pad_ro(RODT_ATTR("Mem:"), 4, FREE_COL_LABEL);
		writeAmount(freebytes, unit);
		writeAmount(largest, unit);
		return PDI_OK;
	}

private:

	/**
	 * Writes one amount right aligned in its column, in the unit the options
	 * asked for.
	 */
	void writeAmount(uint32_t bytes, char unit){
		char text[16];
		memset(text, 0, sizeof(text));
		if( 'h' == unit ){
			BytesToSizeString(bytes, true, text, sizeof(text));
		}else{
			Uint32ToString(('b' == unit) ? bytes : (bytes / 1024), text, sizeof(text));
		}
		m_terminal->write_pad(text, FREE_COL_VALUE, true);
	}
};

#endif
