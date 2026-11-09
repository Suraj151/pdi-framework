/********************************* Seq Command ********************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Prints a run of whole numbers one per line: seq LAST
counts from 1, seq FIRST LAST and seq FIRST INCREMENT LAST step between them,
down as well as up. A run that cannot reach LAST prints nothing. ctrl+c or a
reader that takes no more ends it.

Author          : Suraj I.
created Date    : 7th Nov 2026
******************************************************************************/
#ifndef _SEQ_COMMAND_H_
#define _SEQ_COMMAND_H_

#include "CommandCommon.h"

#define SEQ_YIELD_EVERY   32      /* numbers printed between yields */

struct SeqCommand : public CommandBase {

	SeqCommand(){
		Clear();
		SetCommand(CMD_NAME_SEQ);
		setArgs(nullptr, 3);
		setAcceptArgsOptions(true);
		setCmdOptionSeparator(CMD_OPTION_SEPERATOR_SPACE);
	}

	static void RegisterCommand(){
		CommandBase::RegisterCommand(CMD_NAME_SEQ, [](void *arg)->void *{
			return pdiutil::safe_new<SeqCommand>();
		});
	}

	const char* getUsage() const override {
		return RODT_ATTR("seq [first [increment]] <last>  print whole numbers from first (1) to last, one a line");
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

		int64_t values[3] = {1, 1, 0};
		uint8_t count = 0;
		for( uint8_t i = 0; i < 3; i++ ){
			const CommandOption *option = &m_options[i];
			if( !optionGiven(option) ){
				break;
			}
			int64_t value = 0;
			if( option->optionvalsize <= 0 || !ParseInt64(option->optionval, (uint16_t)option->optionvalsize, value) ||
				value < INT32_MIN || value > INT32_MAX ){
				refuseArg(RODT_ATTR("seq: invalid number : "), option->optionval, option->optionvalsize, CMD_ERROR_INVAL);
				writeRange(INT32_MIN, INT32_MAX);
				return CMD_ERROR_INVAL;
			}
			values[count++] = value;
		}
		if( 0 == count ){
			return CMD_ERROR_ARGS_MISSING;
		}

		int64_t first = (count > 1) ? values[0] : 1;
		int64_t step = (3 == count) ? values[1] : 1;
		int64_t last = values[count - 1];
		if( 0 == step ){
			refuseArg(RODT_ATTR("seq: the increment cannot be : "), m_options[1].optionval, m_options[1].optionvalsize, CMD_ERROR_INVAL);
			return CMD_ERROR_INVAL;
		}

		uint32_t printed = 0;
		for( int64_t value = first; (step > 0) ? (value <= last) : (value >= last); value += step ){
			m_terminal->putln();
			m_terminal->write(value);
			if( 0 == (++printed % SEQ_YIELD_EVERY) ){
				__i_dvc_ctrl.yield();
			}
			if( SessionManager::interruptRequested() || m_terminal->writeRefused() ){
				return SessionManager::interrupted() ? (pdi_err_t)CMD_ERROR_CANCELED : (pdi_err_t)PDI_OK;
			}
		}
		return PDI_OK;
	}
};

#endif
