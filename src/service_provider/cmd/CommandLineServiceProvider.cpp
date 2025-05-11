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
CommandLineServiceProvider::CommandLineServiceProvider() : ServiceProvider(SERVICE_CMD, RODT_ATTR("CMD"))
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
  ListFSCommand *listfscmd = new ListFSCommand();
  m_cmdlist.push_back(listfscmd);

  ChangeDirFSCommand *chdircmd = new ChangeDirFSCommand();
  m_cmdlist.push_back(chdircmd);

  PWDFSCommand *pwdfscmd = new PWDFSCommand();
  m_cmdlist.push_back(pwdfscmd);

  MakeDirFSCommand *makedircmd = new MakeDirFSCommand();
  m_cmdlist.push_back(makedircmd);

  MakeFileFSCommand *makefilecmd = new MakeFileFSCommand();
  m_cmdlist.push_back(makefilecmd);

  RemoveFSCommand *removefdcmd = new RemoveFSCommand();
  m_cmdlist.push_back(removefdcmd);

  MoveFSCommand *movefscmd = new MoveFSCommand();
  m_cmdlist.push_back(movefscmd);

  FileReadCommand *fileReadcmd = new FileReadCommand();
  m_cmdlist.push_back(fileReadcmd);

  FileWriteCommand *fileWritecmd = new FileWriteCommand();
  m_cmdlist.push_back(fileWritecmd);
  #endif

  ClearScreenCommand *clearscreencmd = new ClearScreenCommand();
  m_cmdlist.push_back(clearscreencmd);

  SchedulerTaskCommand *schtaskcmd = new SchedulerTaskCommand();
  m_cmdlist.push_back(schtaskcmd);
}

/**
 * CommandLineServiceProvider destructor
 */
CommandLineServiceProvider::~CommandLineServiceProvider()
{
  for (auto cmd : m_cmdlist) {
    if(cmd) delete cmd;
  }
}

/**
 * Initialize cmd service 
 *
 */
bool CommandLineServiceProvider::initService(void *arg)
{

  if( nullptr != m_terminal ){

    for (int16_t i = 0; i < m_cmdlist.size(); i++){

      if( nullptr != m_cmdlist[i] ){

        // Set default terminal at start
        m_cmdlist[i]->SetTerminal(m_terminal);
      }
    }

    // startInteraction();

    return ServiceProvider::initService(arg);
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

    cmd_term_inseq_t inseq = CMD_TERM_INSEQ_NONE;
    static uint16_t terminalCursorIndex = 0;

    while (terminal->available())
    {
      char c = terminal->read();

      // break command on line ending
      if (c == '\n' || c == '\r') {
        
        terminal->write(c);  // echo
        inseq = CMD_TERM_INSEQ_ENTER;
        break;
      }else if (c == '\b' || c == 0x7F) {
        // backspace or delete char

        // remove cursor position char from string
        inseq = c == '\b' ? CMD_TERM_INSEQ_BACKSPACE_CHAR : CMD_TERM_INSEQ_DELETE_CHAR;
        if (m_termrecvdata.size() > 0 && terminalCursorIndex > 0) {

          terminal->write(c);  // echo

          pdiutil::string substr = m_termrecvdata.substr(terminalCursorIndex);
          if(substr.size() > 0){
            m_termrecvdata = m_termrecvdata.substr(0, terminalCursorIndex-1) + substr;
          }else{
            m_termrecvdata.pop_back();
          }

          terminalCursorIndex--;

          terminal->csi_erase_in_line(0);
          terminal->csi_cursor_move_left(terminalCursorIndex);
          terminal->write(m_termrecvdata.c_str());
          terminal->csi_cursor_move_left(m_termrecvdata.size() - terminalCursorIndex);  
        }
      }else if (c == 0x03) {
        // ctrl+c
        inseq = CMD_TERM_INSEQ_CTRL_C;
        terminal->putln();
      }else if (c == 0x1A) {
        // ctrl+z
        inseq = CMD_TERM_INSEQ_CTRL_Z;
        terminal->putln();
      }else if (c == 0x1B) {
        // Esc
        inseq = CMD_TERM_INSEQ_ESC;

        // wait a 2ms to get the next char
        __i_dvc_ctrl.wait(2);

        // check for ANSI escape sequence
        if(terminal->available()){

          char escseq[5];
          uint8_t seqlen = 0;
          memset(escseq, 0, 5);
          escseq[seqlen++] = c;
          escseq[seqlen++] = terminal->read();

          // there might be more escape sequences out there
          // but we will process only few of them as of now
          // other escape sequences might be valid but not 
          // considered here
          bool isEscSeqConsidered = true;
          bool shouldEcho = true;
  
          // wait a 2ms to get the next char
          __i_dvc_ctrl.wait(2);
          
          // Check for ANSI escape sequence
          if (escseq[1] == '[' && terminal->available()) {

            escseq[seqlen++] = terminal->read();

            // wait a 2ms to get the next char
            __i_dvc_ctrl.wait(2);

            // Check for xterm sequence
            if (escseq[2] == 'A') {
              // up arrow
              inseq = CMD_TERM_INSEQ_UP_ARROW;
              shouldEcho = false; // skip echo as of now
            }else if (escseq[2] == 'B') {
              // down arrow
              inseq = CMD_TERM_INSEQ_DOWN_ARROW;
              shouldEcho = false; // skip echo as of now
            }else if (escseq[2] == 'C') {
              // right arrow
              inseq = CMD_TERM_INSEQ_RIGHT_ARROW;
              if( terminalCursorIndex < m_termrecvdata.size() ){
                terminalCursorIndex++;
              }else{
                shouldEcho = false; // skip echo as of now
              }
            }else if (escseq[2] == 'D') {
              // left arrow
              inseq = CMD_TERM_INSEQ_LEFT_ARROW;
              if( terminalCursorIndex > 0 ){
                terminalCursorIndex--;
              }else{
                shouldEcho = false; // skip echo as of now
              }
            }else if (escseq[2] == 'H') {
              // home
              inseq = CMD_TERM_INSEQ_HOME;
              terminalCursorIndex = 0;
            }else if (escseq[2] == 'F') {
              // end
              inseq = CMD_TERM_INSEQ_END;
              terminalCursorIndex = m_termrecvdata.size();
            }else if (terminal->available()){

              escseq[seqlen++] = terminal->read();

              // check for vt sequence
              if( escseq[3] == '~' ){

                if (escseq[2] == '1' || escseq[2] == '7') {
                  // home
                  inseq = CMD_TERM_INSEQ_HOME;
                  terminalCursorIndex = 0;
                }else if (escseq[2] == '3') {
                  // delete
                  inseq = CMD_TERM_INSEQ_DELETE;

                  // remove char at cursor position
                  if (m_termrecvdata.size() > 0 && terminalCursorIndex < m_termrecvdata.size()) {

                    pdiutil::string substr = m_termrecvdata.substr(terminalCursorIndex+1);
                    if(substr.size() > 0){
                      m_termrecvdata = m_termrecvdata.substr(0, terminalCursorIndex) + substr;
                    }else{
                      m_termrecvdata = m_termrecvdata.substr(0, terminalCursorIndex);
                    }
                    
                    terminal->csi_erase_in_line(0);
                    terminal->csi_cursor_move_left(terminalCursorIndex);
                    terminal->write(m_termrecvdata.c_str());
                    terminal->csi_cursor_move_left(m_termrecvdata.size() - terminalCursorIndex);  

                    shouldEcho = false; // skip echo as of now                              
                  }          
                }else if (escseq[2] == '4' || escseq[2] == '8') {
                  // end
                  inseq = CMD_TERM_INSEQ_END;
                  terminalCursorIndex = m_termrecvdata.size();
                }else if (escseq[2] == '5') {
                  // page up
                  inseq = CMD_TERM_INSEQ_PAGE_UP;
                  shouldEcho = false; // skip echo as of now
                }else if (escseq[2] == '6') {
                  // page down
                  inseq = CMD_TERM_INSEQ_PAGE_DOWN;
                  shouldEcho = false; // skip echo as of now
                }else{
                  isEscSeqConsidered = false;
                }
              }else{
                isEscSeqConsidered = false;
              }
            }else{
              isEscSeqConsidered = false;
            }
          }else{
            isEscSeqConsidered = false;
          }

          // check if escape sequence is not considered
          if( isEscSeqConsidered ){

            // process as special escape sequence case

            if( shouldEcho ){
              // echo escape sequence
              terminal->write(escseq, seqlen);
            }
          }else{

            // todo : else process other chars as normal character
            // for (uint8_t i = 1; i < seqlen; i++){
            //   m_termrecvdata += escseq[i];
            // }
          }
        }
      }else{

        //terminal->write(c);  // echo

        pdiutil::string substr = m_termrecvdata.substr(terminalCursorIndex);
        if(substr.size() > 0){
          m_termrecvdata = m_termrecvdata.substr(0, terminalCursorIndex) + c + substr;
        }else{
          m_termrecvdata += c;
        }
        terminalCursorIndex++;

        terminal->csi_cursor_move_left(terminalCursorIndex-1);
        terminal->write(m_termrecvdata.c_str());
        terminal->csi_cursor_move_left(m_termrecvdata.size() - terminalCursorIndex);
      }

      __i_dvc_ctrl.wait(1);
    }

    // terminal->write_ro(RODT_ATTR(","));
    // terminal->write(m_termrecvdata.c_str());
    // terminal->write_ro(RODT_ATTR(":"));
    // terminal->write(m_termrecvdata.size());
    // terminal->write_ro(RODT_ATTR(":"));
    // terminal->write((int32_t)inseq);
    // terminal->write_ro(RODT_ATTR(":"));
    // terminal->write((int32_t)terminalCursorIndex);
    // terminal->write_ro(RODT_ATTR(":\r\n"));

    // check if line ending is entered
    if( 
      // true 
      inseq != CMD_TERM_INSEQ_ENTER &&
      inseq != CMD_TERM_INSEQ_CTRL_C &&
      inseq != CMD_TERM_INSEQ_CTRL_Z &&
      inseq != CMD_TERM_INSEQ_ESC 
    ){
      return;
    }

    // check if user trying to exit
    if(
      inseq == CMD_TERM_INSEQ_CTRL_C ||
      inseq == CMD_TERM_INSEQ_CTRL_Z
    ){
      // clear stored string
      m_termrecvdata.clear();
      m_termrecvdata = "";
      terminalCursorIndex = 0;
    }

    cmd_result_t result = executeCommand(&m_termrecvdata, inseq);

    // flush stored string
    m_termrecvdata.clear();
    m_termrecvdata = "";
    terminalCursorIndex = 0;
}

/**
 * execute command provided or in list available
 *
 * @param pdiutil::string* cmd
 * @param cmd_term_inseq_t inseq
 * @return cmd_result_t command result status
 */
cmd_result_t CommandLineServiceProvider::executeCommand(pdiutil::string *cmd, cmd_term_inseq_t inseq)
{
  cmd_result_t res = CMD_RESULT_NOT_FOUND;
  bool is_executing_lastcommand = false;

  if( nullptr != cmd && cmd->size() ){

    // first check whether any last command is incomplete and waiting for user input
    for (int16_t i = 0; i < m_cmdlist.size(); i++){

      if(nullptr != m_cmdlist[i] && m_cmdlist[i]->isWaitingForOption()){

        is_executing_lastcommand = true;
        res = m_cmdlist[i]->executeCommand((char*)cmd->c_str(), cmd->size(), true, inseq);
        break;
      }
    }

    for (int16_t i = 0; !is_executing_lastcommand && i < m_cmdlist.size(); i++){

      if(nullptr != m_cmdlist[i] && m_cmdlist[i]->isValidCommand((char*)cmd->c_str())){

        res = m_cmdlist[i]->executeCommand((char*)cmd->c_str(), cmd->size());

        // if( CMD_RESULT_OK == res ){
          break;
        // }
      }
    }

    // if command is incomplete then we are in continue execution mode
    if (CMD_RESULT_INCOMPLETE == res){
      is_executing_lastcommand = true;
    }
  }else{

    res = CMD_RESULT_MAX;

    // check if any command is waiting for user input
    // if any command is waiting for user input then we are in continue execution mode
    for (int16_t i = 0; i < m_cmdlist.size(); i++){

      if(nullptr != m_cmdlist[i] && m_cmdlist[i]->isWaitingForOption()){

        is_executing_lastcommand = true;
        res = CMD_RESULT_INCOMPLETE;

        /* Perform terminal input actions if any */
        if( inseq > CMD_TERM_INSEQ_ENTER && inseq < CMD_TERM_INSEQ_MAX ){
          res = m_cmdlist[i]->executeCommand((char*)cmd->c_str(), cmd->size(), true, inseq);
        }

        break;
      }
    }
  }

  #ifdef ENABLE_AUTH_SERVICE
  bool isWaitingForUser = false;
  if(!__auth_service.getAuthorized()){
    cmd_t *logincmd = __cmd_service.getCommandByName(CMD_NAME_LOGIN);
    if( nullptr != logincmd && logincmd->isWaitingForOption(CMD_OPTION_NAME_U) ){
      isWaitingForUser = true;
    }
  }
  #endif

  if( 
    !is_executing_lastcommand || 
    CMD_RESULT_OK == res || 
    CMD_RESULT_ABORTED == res 
    #ifdef ENABLE_AUTH_SERVICE
    || isWaitingForUser
    #endif
  ){
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
  if( nullptr != m_terminal ){

    m_terminal->putln();

    #ifdef ENABLE_AUTH_SERVICE
    if( __auth_service.getAuthorized() ){
      m_terminal->write(__auth_service.getUsername());
		  m_terminal->write_ro(RODT_ATTR("@"));
      m_terminal->write(__i_dvc_ctrl.getDeviceId());
      #ifdef ENABLE_STORAGE_SERVICE
		  m_terminal->write_ro(RODT_ATTR(":("));
      m_terminal->write(__i_fs.pwd()->c_str());
		  m_terminal->write_ro(RODT_ATTR("): "));
      #else
		  m_terminal->write_ro(RODT_ATTR(": "));
      #endif
    }else{
      m_terminal->write(CMD_NAME_LOGIN);
		  m_terminal->write_ro(RODT_ATTR(": "));

      cmd_t *logincmd = __cmd_service.getCommandByName(CMD_NAME_LOGIN);
      if( nullptr != logincmd ){
        logincmd->setWaitingForOption(CMD_OPTION_NAME_U);
      }
    }
    #else
      m_terminal->write(__i_dvc_ctrl.getDeviceMac().c_str());
		  m_terminal->write_ro(RODT_ATTR(": "));
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