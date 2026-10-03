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
#include "commands/WhoAmICommand.h"
#include "commands/IdCommand.h"
#include "commands/WhoCommand.h"
#include "commands/SuCommand.h"
#include "commands/PasswdCommand.h"
#include "commands/UseraddCommand.h"
#include "commands/UserdelCommand.h"
#include "commands/GroupsCommand.h"
#include "commands/ServiceCommand.h"
#include "commands/ListFSCommand.h"
#include "commands/ChangeDirFSCommand.h"
#include "commands/PWDFSCommand.h"
#include "commands/MkdirCommand.h"
#include "commands/TouchCommand.h"
#include "commands/ChmodCommand.h"
#include "commands/ChownCommand.h"
#include "commands/UmaskCommand.h"
#include "commands/RemoveFSCommand.h"
#include "commands/MoveFSCommand.h"
#include "commands/CopyFSCommand.h"
#include "commands/FileFSCommand.h"
#include "commands/ClearScreenCommand.h"
#include "commands/PsCommand.h"
#include "commands/TopCommand.h"
#include "commands/KillCommand.h"
#include "commands/PkillCommand.h"
#include "commands/ReniceCommand.h"
#include "commands/SSHKeygenCommand.h"
#include "commands/TlsCommand.h"
#include "commands/RebootCommand.h"
#include "commands/NetworkCommand.h"
#include "commands/WatchCommand.h"
#include "commands/DeviceIotCommand.h"
#include "commands/HelpCommand.h"
#include "commands/UptimeCommand.h"
#include "commands/EchoCommand.h"
#include "commands/EnvCommand.h"
#include "commands/ExportCommand.h"
#include "commands/ReadCommand.h"
#include "commands/UnsetCommand.h"
#include "commands/TestCommand.h"
#ifdef ENABLE_SCRIPT_RUNNER
#include "commands/SourceCommand.h"
#endif
#include "commands/DateCommand.h"
#include "commands/TimedatectlCommand.h"
#include "commands/HostCommand.h"
#include "commands/PingCommand.h"
#include "commands/HexdumpCommand.h"
#include "commands/DfFSCommand.h"
#include "commands/DatabaseCommand.h"
#include "commands/MountCommand.h"
#include "commands/WcFSCommand.h"
#include "commands/HeadFSCommand.h"
#include "commands/TailFSCommand.h"
#include "commands/GrepFSCommand.h"
#include "commands/ExecCommand.h"
#include "commands/CrontabCommand.h"
#include "commands/WgetCommand.h"
#include "ShellParser.h"


#ifdef ENABLE_STORAGE_SERVICE
/**
 * Defines constants for cmd history file path.
 */
static const char 		CMD_TERMINAL_HISTORY_STATIC_FILEPATH   []PROG_RODT_ATTR = "/.term_history";
static const uint16_t 	CMD_TERMINAL_HISTORY_MAX_LINES     = 25;
static const uint16_t 	CMD_TERMINAL_HISTORY_ROTATE_SLACK  = 8;

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
    bool isEssentialService() const override { return true; }

	pdi_err_t processTerminalInput(iTerminalInterface *terminal);
	pdi_err_t executeCommand(pdiutil::string *cmd = nullptr, cmd_term_inseq_t inseq = CMD_TERM_INSEQ_NONE) override;
	static void startInteraction();
    bool useTerminal(iTerminalInterface *terminal);
	void releaseSession(session_t *session);
	bool isSessionBusy(session_t *session);
	bool getCommandExecutedFromHistory(pdiutil::string &cmdExec, int16_t index, const char* pattern = nullptr);

private:

	friend class ScriptRunner;

	/**
	 * @var	pdiutil::vector<cmd_t>	m_cmdlist
	 */
    pdiutil::vector<cmd_t*> m_cmdlist;

	#ifdef ENABLE_STORAGE_SERVICE
	/**
	 * @var	pdiutil::string	m_termhistoryfile
	 */
	pdiutil::string m_termhistoryfile;
	#endif

	int16_t getCommandWaitingForUserInput();
	cmd_t* getCommandToExecute(const char *cmdname);

	/**
	 * Names the first word of what was not found. A line can hold several
	 * commands, so the one that failed is not always the one the line starts
	 * with.
	 */
	void reportUnknownCommand(const char *text, uint16_t len);
#ifdef ENABLE_STORAGE_SERVICE
	/**
	 * Points the session output descriptor at the redirect target, resolved
	 * against the working directory. False when the file will not open.
	 */
	bool openRedirect(const pdiutil::string &target, bool append);

	/**
	 * Points the session input descriptor at the named file, resolved against
	 * the working directory. False when the file will not open.
	 */
	bool openSource(const pdiutil::string &source);

	/**
	 * Runs every stage in turn, carrying each one's output into the next
	 * through a pipe and sending the last one to the target or the terminal.
	 */
	pdi_err_t runPipeline(const char *line, const ShellParser::Line &parsed, uint16_t index);

#endif
	/**
	 * Parses one line and dispatches it, without the history, cleanup and
	 * prompt that an interactively typed line carries around it.
	 */
	pdi_err_t runLine(pdiutil::string &line, bool &named_noent);

	/**
	 * Releases every command that has finished, leaving the ones still waiting
	 * for input, running in background, or executing on the stack.
	 */
	void reapFinishedCommands();

	cmd_t* getActiveCommandByName(const char* _cmd);
};

extern CommandLineServiceProvider __cmd_service;

#endif
