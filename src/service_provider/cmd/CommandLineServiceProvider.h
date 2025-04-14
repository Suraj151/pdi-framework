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
#include "commands/FSCommand.h"

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
    bool initService() override;

	void processTerminalInput(iTerminalInterface *terminal);
	cmd_result_t executeCommand(pdiutil::string *cmd = nullptr);
	static void startInteraction();
	cmd_t* getCommandByName(char* _cmd);

	/**
	 * @var	iTerminalInterface*	m_cmdterminal
	 */
    static iTerminalInterface	*m_cmdterminal;

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
