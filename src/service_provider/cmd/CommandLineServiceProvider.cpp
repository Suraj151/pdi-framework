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
CommandLineServiceProvider::CommandLineServiceProvider() : 
  m_terminalCursorIndex(0),
  m_cmdHistoryIndex(-1),
  m_cmdAutoCompleteIndex(-1),
  ServiceProvider(SERVICE_CMD, RODT_ATTR("CMD"))
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

  CopyFSCommand *copyfscmd = new CopyFSCommand();
  m_cmdlist.push_back(copyfscmd);

  FileReadCommand *fileReadcmd = new FileReadCommand();
  m_cmdlist.push_back(fileReadcmd);

  FileWriteCommand *fileWritecmd = new FileWriteCommand();
  m_cmdlist.push_back(fileWritecmd);

  m_termhistoryfile = CHARPTR_WRAP_RO(CMD_TERMINAL_HISTORY_STATIC_FILEPATH);
  #endif

  ClearScreenCommand *clearscreencmd = new ClearScreenCommand();
  m_cmdlist.push_back(clearscreencmd);

  SchedulerTaskCommand *schtaskcmd = new SchedulerTaskCommand();
  m_cmdlist.push_back(schtaskcmd);

  #ifdef ENABLE_SSH_SERVICE
  SSHCommand *sshcmd = new SSHCommand();
  m_cmdlist.push_back(sshcmd);
  #endif

  RebootCommand *rebootcmd = new RebootCommand();
  m_cmdlist.push_back(rebootcmd);

  #ifdef ENABLE_NETWORK_SERVICE
  NetworkCommand *networkcmd = new NetworkCommand();
  m_cmdlist.push_back(networkcmd);
  #endif
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
 * @return Command result if valid command provided in terminal
 */
cmd_result_t CommandLineServiceProvider::processTerminalInput(iTerminalInterface *terminal)
{
    if( nullptr == terminal ) return CMD_RESULT_TERMINAL_ERR;

    cmd_term_inseq_t inseq = CMD_TERM_INSEQ_NONE;

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
        if (m_termrecvdata.size() > 0 && m_terminalCursorIndex > 0) {

          // putty can visualize 0x7F but not every terminal like linux.
          // so sending backspace irespective of incoming char
          terminal->write('\b');  // echo

          pdiutil::string substr = m_termrecvdata.substr(m_terminalCursorIndex);
          if(substr.size() > 0){
            m_termrecvdata = m_termrecvdata.substr(0, m_terminalCursorIndex-1) + substr;
          }else{
            m_termrecvdata.pop_back();
          }

          m_terminalCursorIndex--;

          terminal->csi_erase_in_line(0);
          terminal->csi_cursor_move_left(m_terminalCursorIndex);
          terminal->write(m_termrecvdata.c_str());
          terminal->csi_cursor_move_left(m_termrecvdata.size() - m_terminalCursorIndex);  
        }
      }else if (c == '\t') {
        // ctrl+c
        inseq = CMD_TERM_INSEQ_TAB;
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
              if( m_terminalCursorIndex < m_termrecvdata.size() ){
                m_terminalCursorIndex++;
              }else{
                shouldEcho = false; // skip echo as of now
              }
            }else if (escseq[2] == 'D') {
              // left arrow
              inseq = CMD_TERM_INSEQ_LEFT_ARROW;
              if( m_terminalCursorIndex > 0 ){
                m_terminalCursorIndex--;
              }else{
                shouldEcho = false; // skip echo as of now
              }
            }else if (escseq[2] == 'H') {
              // home
              inseq = CMD_TERM_INSEQ_HOME;
              m_terminalCursorIndex = 0;
            }else if (escseq[2] == 'F') {
              // end
              inseq = CMD_TERM_INSEQ_END;
              m_terminalCursorIndex = m_termrecvdata.size();
            }else if (terminal->available()){

              escseq[seqlen++] = terminal->read();

              // check for vt sequence
              if( escseq[3] == '~' ){

                if (escseq[2] == '1' || escseq[2] == '7') {
                  // home
                  inseq = CMD_TERM_INSEQ_HOME;
                  m_terminalCursorIndex = 0;
                }else if (escseq[2] == '3') {
                  // delete
                  inseq = CMD_TERM_INSEQ_DELETE;

                  // remove char at cursor position
                  if (m_termrecvdata.size() > 0 && m_terminalCursorIndex < m_termrecvdata.size()) {

                    pdiutil::string substr = m_termrecvdata.substr(m_terminalCursorIndex+1);
                    if(substr.size() > 0){
                      m_termrecvdata = m_termrecvdata.substr(0, m_terminalCursorIndex) + substr;
                    }else{
                      m_termrecvdata = m_termrecvdata.substr(0, m_terminalCursorIndex);
                    }
                    
                    terminal->csi_erase_in_line(0);
                    terminal->csi_cursor_move_left(m_terminalCursorIndex);
                    terminal->write(m_termrecvdata.c_str());
                    terminal->csi_cursor_move_left(m_termrecvdata.size() - m_terminalCursorIndex);  

                    shouldEcho = false; // skip echo as of now                              
                  }          
                }else if (escseq[2] == '4' || escseq[2] == '8') {
                  // end
                  inseq = CMD_TERM_INSEQ_END;
                  m_terminalCursorIndex = m_termrecvdata.size();
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

        pdiutil::string substr = m_termrecvdata.substr(m_terminalCursorIndex);
        if(substr.size() > 0){
          m_termrecvdata = m_termrecvdata.substr(0, m_terminalCursorIndex) + c + substr;
        }else{
          m_termrecvdata += c;
        }
        m_terminalCursorIndex++;

        terminal->csi_cursor_move_left(m_terminalCursorIndex-1);
        terminal->write(m_termrecvdata.c_str());
        terminal->csi_cursor_move_left(m_termrecvdata.size() - m_terminalCursorIndex);
      }

      __i_dvc_ctrl.wait(1);
    }

    // terminal->write_ro(RODT_ATTR("\n[termdbg : "));
    // terminal->write(m_termrecvdata.c_str());
    // terminal->write_ro(RODT_ATTR(":"));
    // terminal->write(m_termrecvdata.size());
    // terminal->write_ro(RODT_ATTR(":"));
    // terminal->write((int32_t)inseq);
    // terminal->write_ro(RODT_ATTR(":"));
    // terminal->write((int32_t)m_terminalCursorIndex);
    // terminal->write_ro(RODT_ATTR("]\r\n"));

    // check if line ending is entered
    if( 
      // true 
      inseq != CMD_TERM_INSEQ_ENTER &&
      inseq != CMD_TERM_INSEQ_CTRL_C &&
      inseq != CMD_TERM_INSEQ_CTRL_Z &&
      inseq != CMD_TERM_INSEQ_ESC &&
      inseq != CMD_TERM_INSEQ_UP_ARROW &&
      inseq != CMD_TERM_INSEQ_DOWN_ARROW &&
      inseq != CMD_TERM_INSEQ_TAB
    ){
      m_cmdHistoryIndex = -1;
      m_cmdAutoCompleteIndex = -1;
      return CMD_RESULT_INCOMPLETE;
    }

    // check if user trying to exit
    if(
      inseq == CMD_TERM_INSEQ_CTRL_C ||
      inseq == CMD_TERM_INSEQ_CTRL_Z
    ){
      // clear stored string
      m_termrecvdata.clear();
      m_termrecvdata = "";
      m_terminalCursorIndex = 0;
    }

    cmd_result_t result = executeCommand(&m_termrecvdata, inseq);

    // flush stored string
    if( result == CMD_RESULT_TERMINAL_HOLD_BUFFER ){

    }else{

      m_termrecvdata.clear();
      m_termrecvdata = "";
      m_terminalCursorIndex = 0;
    }

    // flush terminal if command has been processed
    if( CMD_RESULT_NOT_FOUND != result && CMD_RESULT_MAX != result ){
      terminal->flush();
    }

    return result;
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
  if( nullptr == m_terminal ){
    return CMD_RESULT_TERMINAL_ERR;
  }

  cmd_result_t res = CMD_RESULT_NOT_FOUND;
  int16_t waitingCmdIndex = isCommandWaitingForUserInput();
  bool is_executing_lastcommand = (waitingCmdIndex != -1);

  // process the known inseq 
  // no command is waiting for user input
  if(!is_executing_lastcommand){

    if( inseq == CMD_TERM_INSEQ_UP_ARROW || inseq == CMD_TERM_INSEQ_DOWN_ARROW ){
      #ifdef ENABLE_STORAGE_SERVICE
      // fetch command from history
      pdiutil::string cmdExec;

      if( inseq == CMD_TERM_INSEQ_UP_ARROW ){
        // up arrow
        m_cmdHistoryIndex++;
      }else if( inseq == CMD_TERM_INSEQ_DOWN_ARROW ){
        // down arrow
        m_cmdHistoryIndex--;
      }

      // check for the limit
      pdiutil::vector<uint32_t> historyEntries;
      int16_t indexlimit = __i_instance.getFileSystemInstance().getLineNumbersInFile(m_termhistoryfile.c_str(), historyEntries) - 1;
      m_cmdHistoryIndex = m_cmdHistoryIndex > indexlimit ? indexlimit : m_cmdHistoryIndex;

      if( getCommandExecutedFromHistory(cmdExec, m_cmdHistoryIndex) ){

        // clear current line
        m_terminal->csi_cursor_move_left(m_termrecvdata.size());
        m_terminal->csi_erase_in_line(0);

        // copy command to terminal buffer
        m_termrecvdata = cmdExec;
        m_terminalCursorIndex = m_termrecvdata.size();
        
        m_terminal->write(m_termrecvdata.c_str());
      }else{
        // reset history index as no command found
        m_cmdHistoryIndex = -1;
      }
      #endif

      return CMD_RESULT_TERMINAL_HOLD_BUFFER;
    }else{

      // auto complete command
      if( inseq == CMD_TERM_INSEQ_TAB && m_termrecvdata.size() > 0 ){

        static int16_t prevCmdSize = m_termrecvdata.size();
        static int16_t prevArgSize = 0;
        pdiutil::string _cmdtosearch = m_termrecvdata;
        pdiutil::vector<pdiutil::string> matchedCmds;
        pdiutil::string::size_type argStartIndex = m_termrecvdata.find(CMD_OPTION_SEPERATOR_SPACE);

        if( m_cmdAutoCompleteIndex == -1 ){
          prevCmdSize = m_termrecvdata.size();
        }

        if( argStartIndex != pdiutil::string::npos ){
          prevCmdSize = argStartIndex;
        }

        if( prevCmdSize != m_termrecvdata.size() ){
          _cmdtosearch = m_termrecvdata.substr(0, prevCmdSize);
        }

        for (int16_t i = 0; i < m_cmdlist.size(); i++){

          if(nullptr != m_cmdlist[i] && m_cmdlist[i]->isValidCommand((char*)_cmdtosearch.c_str(), true)){

            matchedCmds.push_back(pdiutil::string(m_cmdlist[i]->m_cmd));
          }
        }

        if( matchedCmds.size() > 0 ){

          if( argStartIndex == pdiutil::string::npos ){

            m_cmdAutoCompleteIndex = (m_cmdAutoCompleteIndex + 1) % (matchedCmds.size());

            // clear current line
            m_terminal->csi_cursor_move_left(m_termrecvdata.size());
            m_terminal->csi_erase_in_line(0);

            // copy command to terminal buffer
            m_termrecvdata = matchedCmds[m_cmdAutoCompleteIndex];
            m_terminalCursorIndex = m_termrecvdata.size();
            
            m_terminal->write(m_termrecvdata.c_str());
          }else{

            #ifdef ENABLE_STORAGE_SERVICE
            pdiutil::string _matchcmd = m_termrecvdata.substr(0, argStartIndex);
            pdiutil::string _argtosearch = m_termrecvdata.substr(argStartIndex);
            _argtosearch.erase(0, _argtosearch.find_first_not_of(' '));
            if( m_cmdAutoCompleteIndex == -1 ){
              prevArgSize = _argtosearch.size();
            }
            _argtosearch = _argtosearch.substr(0, prevArgSize);
            pdiutil::vector<file_info_t> itemlist;
            int resultCode = __i_fs.getDirFileList(__i_fs.getPWD().c_str(), itemlist, _argtosearch.c_str());
            
            if(resultCode >= 0 && itemlist.size() > 0){

              m_cmdAutoCompleteIndex = (m_cmdAutoCompleteIndex + 1) % (itemlist.size());

              // clear current line
              m_terminal->csi_cursor_move_left(m_termrecvdata.size());
              m_terminal->csi_erase_in_line(0);

              // copy command to terminal buffer
              m_termrecvdata = _matchcmd + CMD_OPTION_SEPERATOR_SPACE + pdiutil::string(itemlist[m_cmdAutoCompleteIndex].name);
              m_terminalCursorIndex = m_termrecvdata.size();
              
              m_terminal->write(m_termrecvdata.c_str());

              for (file_info_t item : itemlist) {
                // deallocates memory for items
                delete[] item.name;
              }
              itemlist.clear();
            }else{

              m_cmdAutoCompleteIndex = -1;
            }
            #else
            m_cmdAutoCompleteIndex = -1;  
            #endif
          }

          return CMD_RESULT_TERMINAL_HOLD_BUFFER;
        }else{

          m_cmdAutoCompleteIndex = -1;
        }
      }else{

        m_cmdAutoCompleteIndex = -1;
      }
      
      // reset index as we are in middle of command execution
      m_cmdHistoryIndex = -1;
    }
  }else{
    // reset index as we are in middle of command execution
    m_cmdHistoryIndex = -1;
    m_cmdAutoCompleteIndex = -1;
  }


  if( nullptr != cmd && cmd->size() ){

    // first check whether any last command is incomplete and waiting for user input
    if(is_executing_lastcommand){

      res = m_cmdlist[waitingCmdIndex]->executeCommand((char*)cmd->c_str(), cmd->size(), true, inseq);
    }

    for (int16_t i = 0; !is_executing_lastcommand && i < m_cmdlist.size(); i++){

      if(nullptr != m_cmdlist[i] && m_cmdlist[i]->isValidCommand((char*)cmd->c_str())){

        res = m_cmdlist[i]->executeCommand((char*)cmd->c_str(), cmd->size());

        // if( CMD_RESULT_OK == res ){
          break;
        // }
      }
    }

    // store command in history
    #ifdef ENABLE_STORAGE_SERVICE
    if( __i_instance.getFileSystemInstance().isFileExist(m_termhistoryfile.c_str()) == false ){
      __i_instance.getFileSystemInstance().createFile(m_termhistoryfile.c_str(), "", 0);
    }

    if( !is_executing_lastcommand && 
        cmd->size() < 1024
      ){

        // keep only last CMD_TERMINAL_HISTORY_MAX_LINES lines in history
        pdiutil::vector<uint32_t> newlineindices;
        __i_instance.getFileSystemInstance().findInFile(m_termhistoryfile.c_str(), "\r\n", newlineindices, [&](void){
          // yield function to avoid watchdog reset
          __i_dvc_ctrl.yield();
        });
        if(newlineindices.size() > CMD_TERMINAL_HISTORY_MAX_LINES){
          uint32_t remove_upto_index = newlineindices[newlineindices.size() - CMD_TERMINAL_HISTORY_MAX_LINES - 1];

          const char *tempdir = __i_instance.getFileSystemInstance().getTempDirectory();
          pdiutil::string tempFilePath = pdiutil::string(tempdir) + __i_instance.getFileSystemInstance().basename(m_termhistoryfile.c_str());
          if(__i_instance.getFileSystemInstance().isFileExist(tempFilePath.c_str())){
              __i_instance.getFileSystemInstance().deleteFile(tempFilePath.c_str());
          }
          
          int iStatus = __i_instance.getFileSystemInstance().readFile(m_termhistoryfile.c_str(), 100, [&](char* data, uint32_t size)->bool{
            __i_instance.getFileSystemInstance().writeFile(tempFilePath.c_str(), data, size, true);
            return true;
          }, remove_upto_index+1);

          if( iStatus >= 0 ){
            __i_instance.getFileSystemInstance().deleteFile(m_termhistoryfile.c_str());
            __i_instance.getFileSystemInstance().moveFile(tempFilePath.c_str(), m_termhistoryfile.c_str());
          }
        }

        // append command to history file
        __i_instance.getFileSystemInstance().writeFile(m_termhistoryfile.c_str(), (char*)cmd->c_str(), cmd->size(), true);
        __i_instance.getFileSystemInstance().writeFile(m_termhistoryfile.c_str(), (char*)"\r\n", 2, true);
    }
    #endif

    // if command is incomplete then we are in continue execution mode
    if (CMD_RESULT_INCOMPLETE == res){
      is_executing_lastcommand = true;
    }
  }else{

    res = CMD_RESULT_MAX;

    // check if user trying to exit
    if(
      inseq == CMD_TERM_INSEQ_CTRL_C ||
      inseq == CMD_TERM_INSEQ_CTRL_Z
    ){
      res = CMD_RESULT_TERMINAL_ABORTED;
    }

    // check if any command is waiting for user input
    // if any command is waiting for user input then we are in continue execution mode
    if(is_executing_lastcommand){

      res = CMD_RESULT_INCOMPLETE;

      /* Perform terminal input actions if any */
      if( inseq > CMD_TERM_INSEQ_NONE && inseq < CMD_TERM_INSEQ_MAX ){
        res = m_cmdlist[waitingCmdIndex]->executeCommand((char*)cmd->c_str(), cmd->size(), true, inseq);
      }
    }
  }

  #ifdef ENABLE_AUTH_SERVICE
  bool isWaitingForUserAuth = false;
  if(!__auth_service.getAuthorized()){
    cmd_t *logincmd = __cmd_service.getCommandByName(CMD_NAME_LOGIN);
    if( nullptr != logincmd && logincmd->isWaitingForOption(CMD_OPTION_NAME_U) ){
      isWaitingForUserAuth = true;
    }
  }
  #endif

  if( 
    !is_executing_lastcommand || 
    CMD_RESULT_OK == res || 
    CMD_RESULT_ABORTED == res ||
    CMD_RESULT_FAILED == res
    #ifdef ENABLE_AUTH_SERVICE
    || (isWaitingForUserAuth && res == CMD_RESULT_WRONG_CREDENTIAL)
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

    cmd_t *logincmd = __cmd_service.getCommandByName(CMD_NAME_LOGIN);
    
    if( __auth_service.getAuthorized() ){
      
      m_terminal->write(__auth_service.getUsername());
		  m_terminal->write_ro(RODT_ATTR("@"));
      m_terminal->write(__i_dvc_ctrl.getDeviceId());
      #ifdef ENABLE_STORAGE_SERVICE
		  m_terminal->write_ro(RODT_ATTR(":("));
      m_terminal->write(__i_fs.getPWD().c_str());
		  m_terminal->write_ro(RODT_ATTR("): "));
      #else
		  m_terminal->write_ro(RODT_ATTR(": "));
      #endif

      if( nullptr != logincmd && logincmd->isWaitingForOption() ){
        logincmd->setWaitingForOption(nullptr);
      }
    }else{

      m_terminal->write(CMD_NAME_LOGIN);
		  m_terminal->write_ro(RODT_ATTR(": "));

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

/**
 * @brief Use terminal for command line interaction
 * @param terminal iTerminalInterface* terminal
 */
void CommandLineServiceProvider::useTerminal(iTerminalInterface *terminal)
{
  setTerminal(terminal);

  // set terminal for all commands
  for (int16_t i = 0; i < m_cmdlist.size(); i++){

    if(nullptr != m_cmdlist[i]){
      m_cmdlist[i]->SetTerminal(terminal);
    }
  }

  if( nullptr != terminal ){
    // start interaction
    startInteraction();
  }
}

/**
 * @brief Get command executed from history by index
 * @param cmdExec pdiutil::string &cmdExec
 * @param index int16_t index
 * @return true if command found otherwise false
 */
bool CommandLineServiceProvider::getCommandExecutedFromHistory(pdiutil::string &cmdExec, int16_t index){

  #ifdef ENABLE_STORAGE_SERVICE
  if( index >= 0 && __i_instance.getFileSystemInstance().isFileExist(m_termhistoryfile.c_str()) ){

    int32_t linenumber = (index + 1) * -1;
    int iStatus = __i_instance.getFileSystemInstance().readLineInFile(m_termhistoryfile.c_str(), linenumber, cmdExec, [&](void){
      // yield function to avoid watchdog reset
      __i_dvc_ctrl.yield();
    });

    if( iStatus >= 0 ){
      return true;
    }
  }
  #endif

  return false;
}

/**
 * @brief Check if any command is waiting for user input
 * @return index if any command is executing and waiting otherwise -1
 */
int16_t CommandLineServiceProvider::isCommandWaitingForUserInput(){
  for (int16_t i = 0; i < m_cmdlist.size(); i++){

    if(nullptr != m_cmdlist[i] && m_cmdlist[i]->isWaitingForOption()){
      return i;
    }
  }
  return -1;
}

CommandLineServiceProvider __cmd_service;

#endif