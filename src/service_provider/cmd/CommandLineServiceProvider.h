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
	cmd_status_t executeCommand(pdiutil::string *cmd = nullptr);
	static void startInteraction();

	/**
	 * @var	iTerminalInterface*	m_cmdterminal
	 */
    static iTerminalInterface	*m_cmdterminal;

private:

	/**
	 * @var	pdiutil::vector<cmd_t>	m_cmdlist
	 */
    pdiutil::vector<cmd_t*> m_cmdlist;
};

extern CommandLineServiceProvider __cmd_service;

#endif
