/******************************** Help Command ********************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 28th May 2026
******************************************************************************/

#ifndef _HELP_COMMAND_H_
#define _HELP_COMMAND_H_

#include "CommandCommon.h"

struct HelpCommand : public CommandBase {

	static constexpr uint8_t HELP_COL_NAME = 12;

	HelpCommand(){
		Clear();
		SetCommand(CMD_NAME_HELP);
	}

	static void RegisterCommand(){
		CommandBase::RegisterCommand(CMD_NAME_HELP, [](void *arg)->void *{
			return new HelpCommand();
		});
	}

	const char* getUsage() const override {
		return RODT_ATTR("help  list every registered command with a one-line usage");
	}

	cmd_result_t execute(cmd_term_inseq_t terminputaction){

		if(nullptr != m_terminal){
			m_terminal->putln();
			m_terminal->write_ro(RODT_ATTR("Registered commands ("));
			m_terminal->write((int32_t)CommandBase::m_cmd_registry.size());
			m_terminal->writeln_ro(RODT_ATTR("):"));

			for (uint16_t i = 0; i < CommandBase::m_cmd_registry.size(); i++){
				const char *cmdname = CommandBase::m_cmd_registry[i].cmdname;
				auto registrar = CommandBase::m_cmd_registry[i].cmdregistrar;

				m_terminal->write_ro(RODT_ATTR("  "));
				m_terminal->write(cmdname);
				// space-pad name to HELP_COL_NAME so descriptions align regardless
				// of terminal tab width; overflowing names still get one space gap.
				uint8_t nlen = (uint8_t)strlen(cmdname);
				if (nlen >= HELP_COL_NAME) {
					m_terminal->write_ro(RODT_ATTR(" "));
				} else {
					for (uint8_t j = nlen; j < HELP_COL_NAME; j++) m_terminal->write_ro(RODT_ATTR(" "));
				}

				// getUsage is a virtual, so we need a live instance to dispatch
				// on. Registry entries are factories — spin up a scratch command,
				// read its usage, delete it. Only runs on explicit `help` calls
				// so the transient heap churn is bounded and infrequent.
				const char *usage = nullptr;
				CommandBase *inst = nullptr;
				if (nullptr != registrar) {
					inst = (CommandBase*)registrar(nullptr);
					if (nullptr != inst) usage = inst->getUsage();
				}
				if (nullptr != usage) {
					m_terminal->write_ro(usage);
				}
				if (nullptr != inst) delete inst;
				m_terminal->writeln();
			}
		}

		return CMD_RESULT_OK;
	}
};

#endif
