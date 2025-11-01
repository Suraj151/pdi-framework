/******************************** CMD Service *********************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st June 2019
******************************************************************************/

#ifndef _COMMANDLINE_SERVICE_H_
#define _COMMANDLINE_SERVICE_H_

#include <service_provider/ServiceProvider.h>

// commands
#include "commands/AuthCommand.h"
#include "commands/GpioCommand.h"
#include "commands/ServiceCommand.h"
#include "commands/ListFSCommand.h"
#include "commands/ChangeDirFSCommand.h"
#include "commands/PWDFSCommand.h"
#include "commands/MakeDirFSCommand.h"
#include "commands/MakeFileFSCommand.h"
#include "commands/RemoveFSCommand.h"
#include "commands/MoveFSCommand.h"
#include "commands/CopyFSCommand.h"
#include "commands/FileFSCommand.h"
#include "commands/ClearScreenCommand.h"
#include "commands/SchedulerTaskCommand.h"
#include "commands/SSHCommand.h"
#include "commands/RebootCommand.h"
#include "commands/NetworkCommand.h"
#include "commands/WatchCommand.h"


#ifdef ENABLE_STORAGE_SERVICE
/**
 * Defines constants for cmd history file path.
 */
static const char 		CMD_TERMINAL_HISTORY_STATIC_FILEPATH   []PROG_RODT_ATTR = "/.term_history";
static const uint16_t 	CMD_TERMINAL_HISTORY_MAX_LINES     = 25;

#endif


/**
 * CommandLineServiceProvider class
 */
class CommandLineServiceProvider : public ServiceProvider, public CommandExecutionInterface
{

public:
	/**
	 * CommandLineServiceProvider constructor
	 */
	CommandLineServiceProvider();

	/**
	 * CommandLineServiceProvider destructor
	 */
	~CommandLineServiceProvider();

    /**
     * init service
     */
    bool initService(void *arg = nullptr) override;

	cmd_result_t processTerminalInput(iTerminalInterface *terminal);
	cmd_result_t executeCommand(pdiutil::string *cmd = nullptr, cmd_term_inseq_t inseq = CMD_TERM_INSEQ_NONE) override;
	static void startInteraction();
	cmd_t* getCommandByName(char* _cmd);
    void useTerminal(iTerminalInterface *terminal);
	bool getCommandExecutedFromHistory(pdiutil::string &cmdExec, int16_t index, const char* pattern = nullptr);

private:

	/**
	 * @var	pdiutil::vector<cmd_t>	m_cmdlist
	 */
    pdiutil::vector<cmd_t*> m_cmdlist;

	/**
	 * @var	pdiutil::string	m_termrecvdata
	 */
    pdiutil::string m_termrecvdata;

	/**
	 * @var	uint16_t	m_terminalCursorIndex
	 */
    uint16_t m_terminalCursorIndex;

	#ifdef ENABLE_STORAGE_SERVICE
	/**
	 * @var	pdiutil::string	m_termhistoryfile
	 */
	pdiutil::string m_termhistoryfile;

	/**
	 * @var	int16_t	m_cmdHistoryIndex
	 */
    int16_t m_cmdHistoryIndex;
	#endif

	/**
	 * @var	int16_t	m_cmdAutoCompleteIndex
	 */
    int16_t m_cmdAutoCompleteIndex;

	int16_t getCommandWaitingForUserInput();
};

extern CommandLineServiceProvider __cmd_service;

#endif
