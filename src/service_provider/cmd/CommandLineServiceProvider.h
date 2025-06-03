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
#include "commands/FileFSCommand.h"
#include "commands/ClearScreenCommand.h"
#include "commands/SchedulerTaskCommand.h"

/**
 * CommandLineServiceProvider class
 */
class CommandLineServiceProvider : public ServiceProvider
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

	void processTerminalInput(iTerminalInterface *terminal);
	cmd_result_t executeCommand(pdiutil::string *cmd = nullptr, cmd_term_inseq_t inseq = CMD_TERM_INSEQ_NONE);
	static void startInteraction();
	cmd_t* getCommandByName(char* _cmd);
    void useTerminal(iTerminalInterface *terminal);


private:

	/**
	 * @var	pdiutil::vector<cmd_t>	m_cmdlist
	 */
    pdiutil::vector<cmd_t*> m_cmdlist;

	/**
	 * @var	pdiutil::string	m_termrecvdata
	 */
    pdiutil::string m_termrecvdata;
};

extern CommandLineServiceProvider __cmd_service;

#endif
