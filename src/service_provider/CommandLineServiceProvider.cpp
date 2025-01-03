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

/**
 * CommandLineServiceProvider constructor
 */
CommandLineServiceProvider::CommandLineServiceProvider()
{
  // Add commands in list
  GpioCommand *gpiocmd = new GpioCommand();
  LoginCommand *logincmd = new LoginCommand();
  LogoutCommand *logoutcmd = new LogoutCommand();

  m_cmdlist.push_back(gpiocmd);
  m_cmdlist.push_back(logincmd);
  m_cmdlist.push_back(logoutcmd);
}

/**
 * CommandLineServiceProvider destructor
 */
CommandLineServiceProvider::~CommandLineServiceProvider()
{
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

      res = m_cmdlist[i]->parseCmdOptions((char*)cmd->data());

      if( CMD_STATUS_OK == res ){
        break;
      }
    }
  }

  return res;
}

/**
 * Command status to console string 
 *
 * @param cmd_status_t status
 */
void CommandLineServiceProvider::CMDStatusToSerial(cmd_status_t status)
{
  switch (status)
  {
  case CMD_STATUS_ARGS_ERROR:
    LogE("\nArg Error\n");
    break;
  case CMD_STATUS_ARGS_MISSING:
    LogE("\nArg Missing\n");
    break;
  case CMD_STATUS_NOT_FOUND:
    LogE("\nCMD Not Found\n");
    break;
  case CMD_STATUS_INVALID:
    LogE("\nCMD invalid\n");
    break;
  case CMD_STATUS_INVALID_OPTION:
    LogE("\nOption invalid\n");
    break;
  case CMD_STATUS_EMPTY:
    LogE("\nCMD empty\n");
    break;
  case CMD_STATUS_NEED_AUTH:
    LogE("\nRequired login\n");
    break;
  case CMD_STATUS_WRONG_CREDENTIAL:
    LogE("\nWrong Credential\n");
    break;
  case CMD_STATUS_MAX:
    LogE("\nUnknown\n");
    break;
  case CMD_STATUS_OK:
    LogS("\nSuccess\n");
    break;
  default:
    break;
  }
}

CommandLineServiceProvider __cmd_service;

#endif