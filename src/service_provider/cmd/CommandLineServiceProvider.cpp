/******************************** CMD Service *********************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st June 2019
******************************************************************************/

#include <config/Config.h>

#if defined(ENABLE_CMD_SERVICE)

#include "CommandLineServiceProvider.h"


iTerminalInterface *CommandLineServiceProvider::m_cmdterminal = nullptr;

/**
 * CommandLineServiceProvider constructor
 */
CommandLineServiceProvider::CommandLineServiceProvider() : ServiceProvider(SERVICE_CMD)
{
  // Add commands in list
  #ifdef ENABLE_GPIO_SERVICE
  GpioCommand *gpiocmd = new GpioCommand();
  m_cmdlist.push_back(gpiocmd);
  #endif
  
  #ifdef ENABLE_AUTH_SERVICE
  LoginCommand *logincmd = new LoginCommand();
  m_cmdlist.push_back(logincmd);

  LogoutCommand *logoutcmd = new LogoutCommand();
  m_cmdlist.push_back(logoutcmd);
  #endif

  ServiceCommand *svccmd = new ServiceCommand();
  m_cmdlist.push_back(svccmd);
}

/**
 * CommandLineServiceProvider destructor
 */
CommandLineServiceProvider::~CommandLineServiceProvider()
{
}

/**
 * Initialize cmd service 
 *
 */
bool CommandLineServiceProvider::initService()
{

  m_cmdterminal = __i_dvc_ctrl.getTerminal();

  if( nullptr != m_cmdterminal ){

    for (int16_t i = 0; i < m_cmdlist.size(); i++){

      if( nullptr != m_cmdlist[i] ){

        // Set default terminal at start
        m_cmdlist[i]->SetTerminal(m_cmdterminal);
      }
    }

		m_cmdterminal->write_ro(RODT_ATTR("\n\nInitializing PDI Stack CMD\nRelease : "));
    m_cmdterminal->write(RELEASE);
		m_cmdterminal->write_ro(RODT_ATTR("\n\n"));
    startInteraction();

    return true;
  }

  return false;
}

/**
 * execute command provided or in list available
 *
 * @param pdiutil::string* cmd
 * @return cmd_status_t command result status
 */
cmd_status_t CommandLineServiceProvider::executeCommand(pdiutil::string *cmd)
{
  cmd_status_t res = CMD_STATUS_NOT_FOUND;

  for (int16_t i = 0; i < m_cmdlist.size(); i++){

    if(nullptr != m_cmdlist[i] && m_cmdlist[i]->isValidCommand((char*)cmd->data())){

      res = m_cmdlist[i]->executeCommand((char*)cmd->data());

      // if( CMD_STATUS_OK == res ){
        break;
      // }
    }
  }

  // start new interaction
  startInteraction();

  return res;
}

/**
 * Make command terminal ready for interaction 
 *
 * @param cmd_status_t status
 */
void CommandLineServiceProvider::startInteraction()
{
  if( nullptr != m_cmdterminal ){

		m_cmdterminal->write_ro(RODT_ATTR("\npdistack : "));
  }
}

CommandLineServiceProvider __cmd_service;

#endif