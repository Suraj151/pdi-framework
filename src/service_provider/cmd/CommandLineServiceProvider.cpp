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
  #ifdef ENABLE_AUTH_SERVICE
  LoginCommand *logincmd = new LoginCommand();
  m_cmdlist.push_back(logincmd);

  LogoutCommand *logoutcmd = new LogoutCommand();
  m_cmdlist.push_back(logoutcmd);
  #endif

  #ifdef ENABLE_GPIO_SERVICE
  GpioCommand *gpiocmd = new GpioCommand();
  m_cmdlist.push_back(gpiocmd);
  #endif

  ServiceCommand *svccmd = new ServiceCommand();
  m_cmdlist.push_back(svccmd);

  #ifdef ENABLE_STORAGE_SERVICE
  FileSystemCommand *fscmd = new FileSystemCommand();
  m_cmdlist.push_back(fscmd);
  #endif
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

		m_cmdterminal->write_ro(RODT_ATTR("\n\nStarted PDI Stack CMD\r\nRelease : "));
    m_cmdterminal->writeln(RELEASE);

    startInteraction();

    return true;
  }

  return false;
}

/**
 * execute command provided or in list available
 *
 * @param iTerminalInterface* terminal
 * @return None
 */
void CommandLineServiceProvider::processTerminalInput(iTerminalInterface *terminal)
{
    if( nullptr == terminal ) return;

    bool isEnteredLF = false;

    while (terminal->available())
    {
      char c = terminal->read();
      terminal->write(c);  

      // break command on line ending
      if (c == '\n' || c == '\r') {
        
        isEnteredLF = true;
        break;
      }else{

        m_termrecvdata += c;
      }
      __i_dvc_ctrl.wait(1);
    }

    // 
    if( !isEnteredLF ){
      return;
    }

    cmd_result_t result = executeCommand(&m_termrecvdata);

    // flush stored string
    m_termrecvdata = "";
}

/**
 * execute command provided or in list available
 *
 * @param pdiutil::string* cmd
 * @return cmd_result_t command result status
 */
cmd_result_t CommandLineServiceProvider::executeCommand(pdiutil::string *cmd)
{
  cmd_result_t res = CMD_RESULT_NOT_FOUND;
  bool is_executing_lastcommand = false;

  if( nullptr != cmd && cmd->size() ){

    // first check whether any last command is incomplete and waiting for user input
    for (int16_t i = 0; i < m_cmdlist.size(); i++){

      if(nullptr != m_cmdlist[i] && m_cmdlist[i]->isWaitingForOption()){

        is_executing_lastcommand = true;
        res = m_cmdlist[i]->executeCommand((char*)cmd->data(), cmd->size(), true);
        break;
      }
    }

    for (int16_t i = 0; !is_executing_lastcommand && i < m_cmdlist.size(); i++){

      if(nullptr != m_cmdlist[i] && m_cmdlist[i]->isValidCommand((char*)cmd->data())){

        res = m_cmdlist[i]->executeCommand((char*)cmd->data(), cmd->size());

        // if( CMD_RESULT_OK == res ){
          break;
        // }
      }
    }
  }

  if( !is_executing_lastcommand || CMD_RESULT_OK == res ){
    // start new interaction
    startInteraction();
  }

  return res;
}

/**
 * Make command terminal ready for interaction 
 *
 */
void CommandLineServiceProvider::startInteraction()
{
  if( nullptr != m_cmdterminal ){

    m_cmdterminal->putln();

    #ifdef ENABLE_AUTH_SERVICE
    if( __auth_service.getAuthorized() ){
      m_cmdterminal->write(__auth_service.getUsername());
		  m_cmdterminal->write_ro(RODT_ATTR("@"));
      m_cmdterminal->write(__i_dvc_ctrl.getDeviceId());
		  m_cmdterminal->write_ro(RODT_ATTR(": "));
    }else{
      m_cmdterminal->write(CMD_NAME_LOGIN);
		  m_cmdterminal->write_ro(RODT_ATTR(": "));

      cmd_t *logincmd = __cmd_service.getCommandByName(CMD_NAME_LOGIN);
      if( nullptr != logincmd ){
        logincmd->setWaitingForOption(CMD_OPTION_NAME_U);
      }
    }
    #else
      m_cmdterminal->write(__i_dvc_ctrl.getDeviceId());
		  m_cmdterminal->write_ro(RODT_ATTR(": "));
    #endif
  }
}

/**
 * Return command by provided name tag 
 *
 */
cmd_t *CommandLineServiceProvider::getCommandByName(char *_cmd)
{
  for (int16_t i = 0; i < m_cmdlist.size(); i++){

    if(nullptr != m_cmdlist[i] && m_cmdlist[i]->isValidCommand(_cmd)){

      return m_cmdlist[i];
    }
  }

  return nullptr;
}

CommandLineServiceProvider __cmd_service;

#endif