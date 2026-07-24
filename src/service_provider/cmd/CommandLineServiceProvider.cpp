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
#include <service_provider/session/SessionManager.h>


/**
 * CommandLineServiceProvider constructor
 */
CommandLineServiceProvider::CommandLineServiceProvider() :
  ServiceProvider(SERVICE_CMD, RODT_ATTR("CMD"))
{
  // Registers commands
  #ifdef ENABLE_AUTH_SERVICE
  LoginCommand::RegisterCommand();
  LogoutCommand::RegisterCommand();
  WhoAmICommand::RegisterCommand();
  IdCommand::RegisterCommand();
  WhoCommand::RegisterCommand();
  SuCommand::RegisterCommand();
  GroupsCommand::RegisterCommand();
  #ifdef ENABLE_STORAGE_SERVICE
  PasswdCommand::RegisterCommand();
  UseraddCommand::RegisterCommand();
  UserdelCommand::RegisterCommand();
  #endif
  #endif

  // ServiceCommand *svccmd = new ServiceCommand();
  // m_cmdlist.push_back(svccmd);
  ServiceCommand::RegisterCommand();

  #ifdef ENABLE_STORAGE_SERVICE
  // ListFSCommand *listfscmd = new ListFSCommand();
  // m_cmdlist.push_back(listfscmd);
  ListFSCommand::RegisterCommand();

  // ChangeDirFSCommand *chdircmd = new ChangeDirFSCommand();
  // m_cmdlist.push_back(chdircmd);
  ChangeDirFSCommand::RegisterCommand();

  // PWDFSCommand *pwdfscmd = new PWDFSCommand();
  // m_cmdlist.push_back(pwdfscmd);
  PWDFSCommand::RegisterCommand();

  MkdirCommand::RegisterCommand();

  TouchCommand::RegisterCommand();

  ChmodCommand::RegisterCommand();
  #ifdef ENABLE_AUTH_SERVICE
  ChownCommand::RegisterCommand();
  #endif
  UmaskCommand::RegisterCommand();

  // RemoveFSCommand *removefdcmd = new RemoveFSCommand();
  // m_cmdlist.push_back(removefdcmd);
  RemoveFSCommand::RegisterCommand();

  // MoveFSCommand *movefscmd = new MoveFSCommand();
  // m_cmdlist.push_back(movefscmd);
  MoveFSCommand::RegisterCommand();

  // CopyFSCommand *copyfscmd = new CopyFSCommand();
  // m_cmdlist.push_back(copyfscmd);
  CopyFSCommand::RegisterCommand();

  // FileReadCommand *fileReadcmd = new FileReadCommand();
  // m_cmdlist.push_back(fileReadcmd);
  FileReadCommand::RegisterCommand();

  // FileWriteCommand *fileWritecmd = new FileWriteCommand();
  // m_cmdlist.push_back(fileWritecmd);
  FileWriteCommand::RegisterCommand();

  HexdumpCommand::RegisterCommand();
  DfFSCommand::RegisterCommand();
  MountCommand::RegisterCommand();
  WcFSCommand::RegisterCommand();
  HeadFSCommand::RegisterCommand();
  TailFSCommand::RegisterCommand();
  GrepFSCommand::RegisterCommand();

  m_termhistoryfile = CHARPTR_WRAP_RO(CMD_TERMINAL_HISTORY_STATIC_FILEPATH);
  #endif

  // ClearScreenCommand *clearscreencmd = new ClearScreenCommand();
  // m_cmdlist.push_back(clearscreencmd);
  ClearScreenCommand::RegisterCommand();

  PsCommand::RegisterCommand();
  TopCommand::RegisterCommand();
  KillCommand::RegisterCommand();
  PkillCommand::RegisterCommand();
  KillallCommand::RegisterCommand();
  ReniceCommand::RegisterCommand();

  #ifdef ENABLE_SSH_SERVICE
  // SSHCommand *sshcmd = new SSHCommand();
  // m_cmdlist.push_back(sshcmd);
  SSHCommand::RegisterCommand();
  #endif

  #ifdef ENABLE_TLS_CERT_GENERATION
  TlsCommand::RegisterCommand();
  #endif

  // RebootCommand *rebootcmd = new RebootCommand();
  // m_cmdlist.push_back(rebootcmd);
  RebootCommand::RegisterCommand();

  #ifdef ENABLE_NETWORK_SERVICE
  // NetworkCommand *networkcmd = new NetworkCommand();
  // m_cmdlist.push_back(networkcmd);
  NetworkCommand::RegisterCommand();
  HostCommand::RegisterCommand();
  PingCommand::RegisterCommand();
  #endif

  // WatchCommand *watchcmd = new WatchCommand();
  // m_cmdlist.push_back(watchcmd);
  WatchCommand::RegisterCommand();

  #ifdef ENABLE_DEVICE_IOT
  // DeviceIotCommand *deviceiotcmd = new DeviceIotCommand();
  // m_cmdlist.push_back(deviceiotcmd);
  DeviceIotCommand::RegisterCommand();
  #endif

  HelpCommand::RegisterCommand();
  UptimeCommand::RegisterCommand();
  EchoCommand::RegisterCommand();
  DateCommand::RegisterCommand();
  TimedatectlCommand::RegisterCommand();

  CommandBase::SetCommandExecutionInterface(this);
}

/**
 * CommandLineServiceProvider destructor
 */
CommandLineServiceProvider::~CommandLineServiceProvider()
{
  for (auto cmd : m_cmdlist) {
    if(cmd) delete cmd;
  }
  m_cmdlist.clear();
}

/**
 * Initialize cmd service 
 *
 */
bool CommandLineServiceProvider::initService(void *arg)
{

  if( nullptr != m_terminal ){

    // for (int16_t i = 0; i < m_cmdlist.size(); i++){

    //   if( nullptr != m_cmdlist[i] ){

    //     // Set default terminal at start
    //     m_cmdlist[i]->SetTerminal(m_terminal);
    //   }
    // }

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

    session_t *session = SessionManager::findByTerminal(terminal);
    if( nullptr == session ) return CMD_RESULT_TERMINAL_ERR;

    SessionManager::setCurrent(session);
    setTerminal(terminal);
    session->m_lastActivityAt = (uint32_t)__i_dvc_ctrl.millis_now();

    cmd_term_inseq_t inseq = CMD_TERM_INSEQ_NONE;
    bool dontecho = false;
    for (int16_t i = 0; !dontecho && i < m_cmdlist.size(); i++){
      if( nullptr != m_cmdlist[i] && m_cmdlist[i]->m_owner == session &&
          m_cmdlist[i]->isWaitingForOption() && m_cmdlist[i]->wantsMaskedInput() ){
        dontecho = true;
      }
    }
    
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
        if (session->m_linebuf.size() > 0 && session->m_cursor > 0) {

          // putty can visualize 0x7F but not every terminal like linux.
          // so sending backspace irespective of incoming char
          terminal->write('\b');  // echo

          session->m_linebuf.erase(session->m_cursor - 1, 1);
          session->m_cursor--;

          // Redraw the tail from the new cursor position.
          uint16_t tailSize = session->m_linebuf.size() - session->m_cursor;
          terminal->csi_erase_in_line(0);
          if(tailSize > 0){
            terminal->write((char*)(session->m_linebuf.c_str() + session->m_cursor), tailSize);
            terminal->csi_cursor_move_left(tailSize);
          }
        }
      }else if (c == '\t') {
        // tab
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
              if( session->m_cursor < session->m_linebuf.size() ){
                session->m_cursor++;
              }else{
                shouldEcho = false; // skip echo as of now
              }
            }else if (escseq[2] == 'D') {
              // left arrow
              inseq = CMD_TERM_INSEQ_LEFT_ARROW;
              if( session->m_cursor > 0 ){
                session->m_cursor--;
              }else{
                shouldEcho = false; // skip echo as of now
              }
            }else if (escseq[2] == 'H') {
              // home
              inseq = CMD_TERM_INSEQ_HOME;
              session->m_cursor = 0;
            }else if (escseq[2] == 'F') {
              // end
              inseq = CMD_TERM_INSEQ_END;
              session->m_cursor = session->m_linebuf.size();
            }else if (terminal->available()){

              escseq[seqlen++] = terminal->read();

              // check for vt sequence
              if( escseq[3] == '~' ){

                if (escseq[2] == '1' || escseq[2] == '7') {
                  // home
                  inseq = CMD_TERM_INSEQ_HOME;
                  session->m_cursor = 0;
                }else if (escseq[2] == '3') {
                  // delete
                  inseq = CMD_TERM_INSEQ_DELETE;

                  // remove char at cursor position
                  if (session->m_linebuf.size() > 0 && session->m_cursor < session->m_linebuf.size()) {

                    session->m_linebuf.erase(session->m_cursor, 1);

                    // Redraw only the tail from current cursor position.
                    uint16_t tailSize = session->m_linebuf.size() - session->m_cursor;
                    terminal->csi_erase_in_line(0);
                    if(tailSize > 0){
                      terminal->write((char*)(session->m_linebuf.c_str() + session->m_cursor), tailSize);
                      terminal->csi_cursor_move_left(tailSize);
                    }

                    shouldEcho = false; // skip echo as of now
                  }
                }else if (escseq[2] == '4' || escseq[2] == '8') {
                  // end
                  inseq = CMD_TERM_INSEQ_END;
                  session->m_cursor = session->m_linebuf.size();
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
            //   session->m_linebuf += escseq[i];
            // }
          }
        }
      }else{

        //terminal->write(c);  // echo

        session->m_linebuf.insert(session->m_cursor, 1, c);
        session->m_cursor++;

        if(!dontecho){
          // Echo only the newly inserted char plus any tail to its right,
          // then walk the cursor back so it sits after the new char. For
          // typing at end-of-line this collapses to a single byte written.
          uint16_t tailRemaining = session->m_linebuf.size() - session->m_cursor;
          uint16_t writeSize = tailRemaining + 1;
          terminal->write((char*)(session->m_linebuf.c_str() + session->m_cursor - 1), writeSize);
          if(tailRemaining > 0){
            terminal->csi_cursor_move_left(tailRemaining);
          }
        }
      }

      __i_dvc_ctrl.wait(0.1);
    }

    // terminal->write_ro(RODT_ATTR("\n[termdbg : "));
    // terminal->write(session->m_linebuf.c_str());
    // terminal->write_ro(RODT_ATTR(":"));
    // terminal->write(session->m_linebuf.size());
    // terminal->write_ro(RODT_ATTR(":"));
    // terminal->write((int32_t)inseq);
    // terminal->write_ro(RODT_ATTR(":"));
    // terminal->write((int32_t)session->m_cursor);
    // terminal->write_ro(RODT_ATTR("]\r\n"));

    // check if line ending is entered
    if( ( 
        dontecho && 
        inseq != CMD_TERM_INSEQ_ENTER &&
        inseq != CMD_TERM_INSEQ_CTRL_C &&
        inseq != CMD_TERM_INSEQ_CTRL_Z 
      ) || (
        inseq != CMD_TERM_INSEQ_ENTER &&
        inseq != CMD_TERM_INSEQ_CTRL_C &&
        inseq != CMD_TERM_INSEQ_CTRL_Z &&
        inseq != CMD_TERM_INSEQ_ESC &&
        inseq != CMD_TERM_INSEQ_UP_ARROW &&
        inseq != CMD_TERM_INSEQ_DOWN_ARROW &&
        inseq != CMD_TERM_INSEQ_TAB
    )){
#ifdef ENABLE_STORAGE_SERVICE      
      session->m_historyIdx = -1;
#endif
      session->m_autoCompleteIdx = -1;
      return CMD_RESULT_INCOMPLETE;
    }

    if(
      inseq == CMD_TERM_INSEQ_CTRL_C ||
      inseq == CMD_TERM_INSEQ_CTRL_Z
    ){
      session->m_linebuf.clear();
      session->m_cursor = 0;
    }

    cmd_result_t result = executeCommand(&session->m_linebuf, inseq);

    // flush stored string
    if( result == CMD_RESULT_TERMINAL_HOLD_BUFFER ){

    }else{

      session->m_linebuf.clear();
      session->m_cursor = 0;
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

  session_t *session = SessionManager::current();
  if( nullptr == session ) return CMD_RESULT_TERMINAL_ERR;

  cmd_result_t res = CMD_RESULT_NOT_FOUND;
  int16_t waitingCmdIndex = getCommandWaitingForUserInput();
  bool is_executing_lastcommand = (waitingCmdIndex != -1);

  // process the known inseq 
  // no command is waiting for user input
  if(!is_executing_lastcommand){

    if( inseq == CMD_TERM_INSEQ_UP_ARROW || inseq == CMD_TERM_INSEQ_DOWN_ARROW ){
      #ifdef ENABLE_STORAGE_SERVICE
      // fetch command from history
      pdiutil::string cmdExec;

      if( session->m_historyIdx == -1 ){
        session->m_prevHistorySize = session->m_linebuf.size();
        session->m_origTypedPrefix = session->m_linebuf;
      }

      pdiutil::string _patterntosearch = session->m_origTypedPrefix;

      if( inseq == CMD_TERM_INSEQ_UP_ARROW ){
        // up arrow
        session->m_historyIdx++;
      }else if( inseq == CMD_TERM_INSEQ_DOWN_ARROW ){
        // down arrow
        session->m_historyIdx--;
      }

      // check for the limit
      pdiutil::vector<uint32_t> historyEntries;
      int16_t indexlimit = __i_instance.getFileSystemInstance().getLineNumbersInFile(m_termhistoryfile.c_str(), historyEntries) - 1;
      session->m_historyIdx = session->m_historyIdx > indexlimit ? indexlimit : 
      session->m_historyIdx < 0 ? 0 : session->m_historyIdx;

      if( getCommandExecutedFromHistory(cmdExec, session->m_historyIdx, _patterntosearch.size() > 0 ? _patterntosearch.c_str() : nullptr) ){

        // clear current line
        m_terminal->csi_cursor_move_left(session->m_linebuf.size());
        m_terminal->csi_erase_in_line(0);

        // copy command to terminal buffer
        session->m_linebuf = cmdExec;
        session->m_cursor = session->m_linebuf.size();
        
        m_terminal->write(session->m_linebuf.c_str());
      }else{
        // reset history index as no command found
        session->m_historyIdx = session->m_historyIdx > 0 ? session->m_historyIdx - 1 : -1;
      }
      #endif

      return CMD_RESULT_TERMINAL_HOLD_BUFFER;
    }else{

      // auto complete command
      if( inseq == CMD_TERM_INSEQ_TAB && session->m_linebuf.size() > 0 ){

        pdiutil::string _cmdtosearch = session->m_linebuf;
        pdiutil::vector<pdiutil::string> matchedCmds;
        pdiutil::string::size_type argStartIndex = session->m_linebuf.find(CMD_OPTION_SEPERATOR_SPACE);

        if( session->m_autoCompleteIdx == -1 ){
          session->m_prevCmdSize = session->m_linebuf.size();
        }

        if( argStartIndex != pdiutil::string::npos ){
          session->m_prevCmdSize = argStartIndex;
        }

        if( session->m_prevCmdSize != (int16_t)session->m_linebuf.size() ){
          _cmdtosearch = session->m_linebuf.substr(0, session->m_prevCmdSize);
        }

        for (int16_t i = 0; i < CommandBase::m_cmd_registry.size(); i++){

          if(CommandBase::isCommandMatch(CommandBase::m_cmd_registry[i].cmdname, _cmdtosearch.c_str(), true)){

            matchedCmds.push_back(pdiutil::string(CommandBase::m_cmd_registry[i].cmdname));
          }
        }

        if( matchedCmds.size() > 0 ){

          if( argStartIndex == pdiutil::string::npos ){

            session->m_autoCompleteIdx = (session->m_autoCompleteIdx + 1) % (matchedCmds.size());

            // clear current line
            m_terminal->csi_cursor_move_left(session->m_linebuf.size());
            m_terminal->csi_erase_in_line(0);

            // copy command to terminal buffer
            session->m_linebuf = matchedCmds[session->m_autoCompleteIdx];
            session->m_cursor = session->m_linebuf.size();
            
            m_terminal->write(session->m_linebuf.c_str());
          }else{

            #ifdef ENABLE_STORAGE_SERVICE
            pdiutil::string _matchcmd = session->m_linebuf.substr(0, argStartIndex);
            pdiutil::string _argtosearch = session->m_linebuf.substr(argStartIndex);
            _argtosearch.erase(0, _argtosearch.find_first_not_of(' '));
            if( session->m_autoCompleteIdx == -1 ){
              session->m_prevArgSize = _argtosearch.size();
            }
            _argtosearch = _argtosearch.substr(0, session->m_prevArgSize);
            pdiutil::vector<file_info_t> itemlist;
            int resultCode = __i_fs.getDirFileList(SessionManager::getPWD().c_str(), itemlist, _argtosearch.c_str());

            if(resultCode >= 0 && itemlist.size() > 0){

              session->m_autoCompleteIdx = (session->m_autoCompleteIdx + 1) % (itemlist.size());

              // clear current line
              m_terminal->csi_cursor_move_left(session->m_linebuf.size());
              m_terminal->csi_erase_in_line(0);

              // copy command to terminal buffer
              session->m_linebuf = _matchcmd + CMD_OPTION_SEPERATOR_SPACE + pdiutil::string(itemlist[session->m_autoCompleteIdx].m_name);
              session->m_cursor = session->m_linebuf.size();

              m_terminal->write(session->m_linebuf.c_str());
            }else{

              session->m_autoCompleteIdx = -1;
            }

            for (file_info_t &item : itemlist) {
              if(item.m_name) delete[] item.m_name;
            }
            itemlist.clear();
            #else
            session->m_autoCompleteIdx = -1;  
            #endif
          }

          return CMD_RESULT_TERMINAL_HOLD_BUFFER;
        }else{

          session->m_autoCompleteIdx = -1;
        }
      }else{

        session->m_autoCompleteIdx = -1;
      }
      
#ifdef ENABLE_STORAGE_SERVICE      // reset index as we are in middle of command execution
      session->m_historyIdx = -1;
#endif
    }
  }else{
#ifdef ENABLE_STORAGE_SERVICE    // reset index as we are in middle of command execution
    session->m_historyIdx = -1;
#endif
    session->m_autoCompleteIdx = -1;
  }


  if( nullptr != cmd && cmd->size() ){

    // first check whether any last command is incomplete and waiting for user input
    if(is_executing_lastcommand){

      res = m_cmdlist[waitingCmdIndex]->executeCommand((char*)cmd->c_str(), cmd->size(), true, inseq);
    }else{

      cmd_t* cmd_to_exec = getCommandToExecute(cmd->c_str());

      if(nullptr != cmd_to_exec){

        res = cmd_to_exec->executeCommand((char*)cmd->c_str(), cmd->size());
      }
    }

    // for (int16_t i = 0; !is_executing_lastcommand && i < m_cmdlist.size(); i++){

    //   if(nullptr != m_cmdlist[i] && m_cmdlist[i]->isValidCommand((char*)cmd->c_str())){

    //     res = m_cmdlist[i]->executeCommand((char*)cmd->c_str(), cmd->size());

    //     // if( CMD_RESULT_OK == res ){
    //       break;
    //     // }
    //   }
    // }

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
        __i_instance.getFileSystemInstance().findInFile(m_termhistoryfile.c_str(), "\r\n", &newlineindices, -1, 1, 0, [&](void){
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

          // Snapshot original ctime so it survives the temp+rename rotation.
          uint32_t origCtime = 0;
          __i_instance.getFileSystemInstance().getFileAttr(m_termhistoryfile.c_str(), FILE_ATTR_CTIME, &origCtime, sizeof(origCtime));

          int iStatus = __i_instance.getFileSystemInstance().readFile(m_termhistoryfile.c_str(), 100, [&](char* data, uint32_t size)->bool{
            __i_instance.getFileSystemInstance().writeFile(tempFilePath.c_str(), data, size, true);
            return true;
          }, remove_upto_index+1);

          if( iStatus >= 0 ){
            __i_instance.getFileSystemInstance().deleteFile(m_termhistoryfile.c_str());
            __i_instance.getFileSystemInstance().moveFile(tempFilePath.c_str(), m_termhistoryfile.c_str());
            if( origCtime ){
              __i_instance.getFileSystemInstance().setFileAttr(m_termhistoryfile.c_str(), FILE_ATTR_CTIME, &origCtime, sizeof(origCtime));
            }
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

    // Ctrl+C / Ctrl+Z at an idle prompt aborts anything running in the
    // background but MUST NOT drop the session. Transport layers close the
    // channel only on CMD_RESULT_TERMINAL_ABORTED (logout / EOF).
    if(
      inseq == CMD_TERM_INSEQ_CTRL_C ||
      inseq == CMD_TERM_INSEQ_CTRL_Z
    ){
      res = CMD_RESULT_ABORTED;
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

  // if command aborted then stop every background-running command owned by
  // the current session
  if( CMD_RESULT_ABORTED == res || CMD_RESULT_TERMINAL_ABORTED == res ){
    session_t *cur = SessionManager::current();
    for (int16_t i = 0; i < m_cmdlist.size(); i++){
      if( nullptr != m_cmdlist[i] && m_cmdlist[i]->m_owner == cur && m_cmdlist[i]->isRunningInBackground() ){
        m_cmdlist[i]->stopRunningInBackground();
      }
    }
  }

  #ifdef ENABLE_AUTH_SERVICE
  bool isWaitingForUserAuth = false;
  if(!__auth_service.getAuthorized()){
    cmd_t *logincmd = __cmd_service.getActiveCommandByName(CMD_NAME_LOGIN);
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

  // Clean up executed commands.
  for (int16_t i = static_cast<int16_t>(m_cmdlist.size()) - 1; i >= 0; i--){

    if(nullptr != m_cmdlist[i] && !m_cmdlist[i]->isWaitingForOption() && !m_cmdlist[i]->isRunningInBackground()){

      delete m_cmdlist[i];
      m_cmdlist.erase(m_cmdlist.begin() + i);
    }
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

    cmd_t *logincmd = __cmd_service.getActiveCommandByName(CMD_NAME_LOGIN); 
    
    if(nullptr == logincmd){
      logincmd = __cmd_service.getCommandToExecute(CMD_NAME_LOGIN);
    }
    
    if( __auth_service.getAuthorized() ){
      
      m_terminal->write(__auth_service.getUsername());
		  m_terminal->write_ro(RODT_ATTR("@"));
      m_terminal->write(__i_dvc_ctrl.getDeviceId());
      #ifdef ENABLE_STORAGE_SERVICE
		  m_terminal->write_ro(RODT_ATTR(":("));
      m_terminal->write(SessionManager::getPWD().c_str());
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
 * Return the active command by provided name tag 
 *
 */
cmd_t *CommandLineServiceProvider::getActiveCommandByName(const char *_cmd)
{
  session_t *cur = SessionManager::current();
  for (int16_t i = 0; i < m_cmdlist.size(); i++){

    if(nullptr != m_cmdlist[i] && m_cmdlist[i]->m_owner == cur && m_cmdlist[i]->isValidCommand(_cmd)){

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

  if( nullptr != terminal ){
    session_t *s = SessionManager::attach(terminal);
    for (int16_t i = 0; i < m_cmdlist.size(); i++){
      if(nullptr != m_cmdlist[i] && m_cmdlist[i]->m_owner == s){
        m_cmdlist[i]->SetTerminal(terminal);
      }
    }
    startInteraction();
  }else{
    SessionManager::detachCurrent();
  }
}

/**
 * @brief Get command executed from history by index
 * @param cmdExec pdiutil::string &cmdExec
 * @param index int16_t index
 * @return true if command found otherwise false
 */
bool CommandLineServiceProvider::getCommandExecutedFromHistory(pdiutil::string &cmdExec, int16_t index, const char* pattern){

  #ifdef ENABLE_STORAGE_SERVICE
  if( index >= 0 && __i_instance.getFileSystemInstance().isFileExist(m_termhistoryfile.c_str()) ){

    int32_t linenumber = (index + 1) * -1;
    int iStatus = __i_instance.getFileSystemInstance().readLineInFile(m_termhistoryfile.c_str(), linenumber, cmdExec, pattern, [&](void){
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
int16_t CommandLineServiceProvider::getCommandWaitingForUserInput(){
  session_t *cur = SessionManager::current();
  for (int16_t i = 0; i < m_cmdlist.size(); i++){

    if(nullptr != m_cmdlist[i] && m_cmdlist[i]->m_owner == cur && m_cmdlist[i]->isWaitingForOption()){
      return i;
    }
  }
  return -1;
}

/**
 * @brief Mark command to be execute and add it in cmdlist for further operations
 */
cmd_t* CommandLineServiceProvider::getCommandToExecute(const char *cmdname){

  cmd_t* cmd_to_exec = CommandBase::GetCommand(cmdname);

  if(nullptr != cmd_to_exec){

    cmd_to_exec->SetTerminal(m_terminal);
    cmd_to_exec->m_owner = SessionManager::current();
    m_cmdlist.push_back(cmd_to_exec);
  }

  return cmd_to_exec;
}

CommandLineServiceProvider __cmd_service;

#endif