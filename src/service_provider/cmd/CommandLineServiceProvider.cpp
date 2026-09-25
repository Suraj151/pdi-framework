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
#include "ShellCompletion.h"
#include <service_provider/session/SessionManager.h>
#include <service_provider/session/Environment.h>
#ifdef ENABLE_SCRIPT_RUNNER
#include "ScriptRunner.h"
#endif
#ifdef ENABLE_STORAGE_SERVICE
#include <service_provider/session/FileWriteStream.h>
#include <service_provider/session/FileReadStream.h>
#include <service_provider/session/PipeStream.h>
#include <utility/SafeAlloc.h>
#endif


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

  // FileEditCommand *fileWritecmd = new FileEditCommand();
  // m_cmdlist.push_back(fileWritecmd);
  FileEditCommand::RegisterCommand();

  HexdumpCommand::RegisterCommand();
  DfFSCommand::RegisterCommand();
  DatabaseCommand::RegisterCommand();
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
  SSHKeygenCommand::RegisterCommand();
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
#if defined(ENABLE_HTTP_CLIENT) && defined(ENABLE_STORAGE_SERVICE)
  WgetCommand::RegisterCommand();
#endif
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
  EnvCommand::RegisterCommand();
  ExportCommand::RegisterCommand();
  UnsetCommand::RegisterCommand();
  TestCommand::RegisterCommand();
  #ifdef ENABLE_SCRIPT_RUNNER
  SourceCommand::RegisterCommand();
#ifdef ENABLE_CRON_SERVICE
  CrontabCommand::RegisterCommand();
#endif
  #endif
  DateCommand::RegisterCommand();
  TimedatectlCommand::RegisterCommand();

  #ifdef ENABLE_PROGRAM_EXEC
  ExecCommand::RegisterCommand();
  #endif

  CommandBase::SetCommandExecutionInterface(this);
}

/**
 * CommandLineServiceProvider destructor
 */
CommandLineServiceProvider::~CommandLineServiceProvider()
{
  for (auto cmd : m_cmdlist) {
    pdiutil::safe_delete(cmd);
  }
  m_cmdlist.clear();
}

/**
 * Initialize cmd service 
 *
 */
bool CommandLineServiceProvider::initService(void *arg)
{

#ifdef ENABLE_STORAGE_SERVICE
  Environment::ensureBaseFile();
#endif

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
pdi_err_t CommandLineServiceProvider::processTerminalInput(iTerminalInterface *terminal)
{
    if( nullptr == terminal ) return CMD_ERROR_NOTTY;

    session_t *session = SessionManager::findByTerminal(terminal);
    if( nullptr == session ) return CMD_ERROR_NOTTY;

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

    bool ownRender = false;
    for (int16_t i = 0; !ownRender && i < m_cmdlist.size(); i++){
      if( nullptr != m_cmdlist[i] && m_cmdlist[i]->m_owner == session &&
          m_cmdlist[i]->isWaitingForOption() && m_cmdlist[i]->managesLineRender() ){
        ownRender = true;
      }
    }
    
    while (terminal->available())
    {
      char c = terminal->read();

      bool iseol = ('\n' == c || '\r' == c);

      if (iseol && 0 != session->m_lastEol && c != session->m_lastEol) {
        session->m_lastEol = 0;
        continue;
      }

      session->m_lastEol = iseol ? c : 0;

      // break command on line ending
      if (iseol) {

        terminal->write(c);  // echo
        inseq = CMD_TERM_INSEQ_ENTER;
        break;
      }else if (c == '\b' || c == 0x7F) {
        // backspace or delete char

        // remove cursor position char from string
        inseq = c == '\b' ? CMD_TERM_INSEQ_BACKSPACE_CHAR : CMD_TERM_INSEQ_DELETE_CHAR;
        if (session->m_linebuf.size() > 0 && session->m_cursor > 0) {

          session->m_linebuf.erase(session->m_cursor - 1, 1);
          session->m_cursor--;

          if(!ownRender){
            // putty can visualize 0x7F but not every terminal like linux.
            // so sending backspace irespective of incoming char
            terminal->write('\b');  // echo

            // Redraw the tail from the new cursor position.
            uint16_t tailSize = session->m_linebuf.size() - session->m_cursor;
            terminal->csi_erase_in_line(0);
            if(tailSize > 0){
              terminal->write((char*)(session->m_linebuf.c_str() + session->m_cursor), tailSize);
              terminal->csi_cursor_move_left(tailSize);
            }
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

                    if(!ownRender){
                      // Redraw only the tail from current cursor position.
                      uint16_t tailSize = session->m_linebuf.size() - session->m_cursor;
                      terminal->csi_erase_in_line(0);
                      if(tailSize > 0){
                        terminal->write((char*)(session->m_linebuf.c_str() + session->m_cursor), tailSize);
                        terminal->csi_cursor_move_left(tailSize);
                      }
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

            if( shouldEcho && !ownRender ){
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
      }else if( (uint8_t)c >= 0x20 && 0xFF != (uint8_t)c ){

        //terminal->write(c);  // echo

        session->m_linebuf.insert(session->m_cursor, 1, c);
        session->m_cursor++;

        if(!dontecho && !ownRender){
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
    if( !ownRender && ( (
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
    ) )){
#ifdef ENABLE_STORAGE_SERVICE      
      session->m_historyIdx = -1;
#endif
      return CMD_ERROR_AGAIN;
    }

    if(
      inseq == CMD_TERM_INSEQ_CTRL_C ||
      inseq == CMD_TERM_INSEQ_CTRL_Z
    ){
      session->m_linebuf.clear();
      session->m_cursor = 0;
    }

    pdi_err_t result = executeCommand(&session->m_linebuf, inseq);

    // A waiting command (e.g. the fedit line editor) can manage the line
    // buffer itself; don't wipe its preloaded content between inputs.
    bool preserveLineBuf = false;
    int16_t activeWaitingIdx = getCommandWaitingForUserInput();
    if( activeWaitingIdx != -1 && nullptr != m_cmdlist[activeWaitingIdx] &&
        m_cmdlist[activeWaitingIdx]->preservesLineBuffer() ){
      preserveLineBuf = true;
    }

    // flush stored string
    if( result == CMD_ERROR_HOLD_BUFFER || preserveLineBuf ){

    }else{

      session->m_linebuf.clear();
      session->m_cursor = 0;
    }

    // flush terminal if command has been processed
    if( CMD_ERROR_NOENT != result && CMD_ERROR_UNSET != result ){
      terminal->flush();
    }

    return result;
}

/**
 * Releases every command that has finished, leaving the ones still waiting
 * for input, running in background, or executing on the stack.
 */
void CommandLineServiceProvider::reapFinishedCommands()
{
  for (int16_t i = static_cast<int16_t>(m_cmdlist.size()) - 1; i >= 0; i--){

    if(nullptr != m_cmdlist[i] && !m_cmdlist[i]->isWaitingForOption() && !m_cmdlist[i]->isRunningInBackground() &&
       !m_cmdlist[i]->isExecuting()){

      pdiutil::safe_delete(m_cmdlist[i]);
      m_cmdlist.erase(m_cmdlist.begin() + i);
    }
  }
}

/**
 * Parses one line and dispatches it, without the history, cleanup and
 * prompt that an interactively typed line carries around it.
 */
pdi_err_t CommandLineServiceProvider::runLine(pdiutil::string &line, bool &named_noent)
{
  pdi_err_t res = CMD_ERROR_NOENT;

  #if defined(ENABLE_STORAGE_SERVICE)
  ShellParser::Line parsed = ShellParser::parse(line.c_str(), (int16_t)line.size());

  if( parsed.m_malformed ){

    m_terminal->writeln();
    m_terminal->writeln_ro(RODT_ATTR("syntax error"));
    res = CMD_ERROR_INVAL;
  }else if( parsed.isPlain() && !ShellParser::expandable(line.c_str(), (int16_t)line.size()) ){

    // one command, nothing to wire up and nothing to expand, so run it
    // where it lies rather than copying the line
    cmd_t* cmd_to_exec = getCommandToExecute(line.c_str());

    res = ( nullptr != cmd_to_exec ) ?
          cmd_to_exec->executeCommand((char*)line.c_str(), line.size()) :
          (pdi_err_t)CMD_ERROR_NOENT;
  }else{

    for( uint16_t p = 0; p < parsed.m_pipelines.size(); p++ ){

      if( !ShellParser::shouldRun(parsed.m_pipelines[p].m_join, res) ){
        continue;
      }

      res = runPipeline(line.c_str(), parsed, p);

      // a command that stopped for input owns the terminal, so what
      // follows on the line cannot run behind it
      if( CMD_ERROR_AGAIN == res || CMD_ERROR_HOLD_BUFFER == res ){
        break;
      }

      // $? is the status of the last command that finished, so a segment
      // later on the same line has to see this one
      SessionManager::setLastExit(res);
    }

    named_noent = true;
  }
  #else
  cmd_t* cmd_to_exec = getCommandToExecute(line.c_str());

  if(nullptr != cmd_to_exec){

    res = cmd_to_exec->executeCommand((char*)line.c_str(), line.size());
  }
  #endif

  return res;
}

/**
 * execute command provided or in list available
 *
 * @param pdiutil::string* cmd
 * @param cmd_term_inseq_t inseq
 * @return pdi_err_t command result status
 */
pdi_err_t CommandLineServiceProvider::executeCommand(pdiutil::string *cmd, cmd_term_inseq_t inseq)
{
  if( nullptr == m_terminal ){
    return CMD_ERROR_NOTTY;
  }

  session_t *session = SessionManager::current();
  if( nullptr == session ) return CMD_ERROR_NOTTY;

  pdi_err_t res = CMD_ERROR_NOENT;
  int16_t waitingCmdIndex = getCommandWaitingForUserInput();
  bool is_executing_lastcommand = (waitingCmdIndex != -1);

  // the pipeline path names the segment it could not find, so the whole line
  // is not named a second time on its behalf
  bool reported_noent = false;

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

      return CMD_ERROR_HOLD_BUFFER;
    }else{

      // auto complete command
      if( inseq == CMD_TERM_INSEQ_TAB ){

        shell_complete_result_t completed =
          ShellCompletion::complete(session->m_linebuf, session->m_cursor, m_terminal);

        // a listing leaves the line scrolled away, so it is drawn again under
        // a fresh prompt with the cursor put back where it was
        if( SHELL_COMPLETE_LISTED == completed ){

          startInteraction();
          m_terminal->write(session->m_linebuf.c_str());

          int16_t back = (int16_t)session->m_linebuf.size() - session->m_cursor;
          if( back > 0 ){
            m_terminal->csi_cursor_move_left(back);
          }
        }

        return CMD_ERROR_HOLD_BUFFER;
      }
      
#ifdef ENABLE_STORAGE_SERVICE      // reset index as we are in middle of command execution
      session->m_historyIdx = -1;
#endif
    }
  }else{
#ifdef ENABLE_STORAGE_SERVICE    // reset index as we are in middle of command execution
    session->m_historyIdx = -1;
#endif
  }


  if( nullptr != cmd && cmd->size() ){

    // Snapshot the typed command now: `cmd` aliases session->m_linebuf, and an
    // interactive command (e.g. the fedit editor) may overwrite that buffer
    // during execute() — history must record what was entered, not that.
    pdiutil::string originalCmd = *cmd;

    // first check whether any last command is incomplete and waiting for user input
    if(is_executing_lastcommand){

      res = m_cmdlist[waitingCmdIndex]->executeCommand((char*)cmd->c_str(), cmd->size(), true, inseq);
    }else{

      res = runLine(*cmd, reported_noent);
    }

    // for (int16_t i = 0; !is_executing_lastcommand && i < m_cmdlist.size(); i++){

    //   if(nullptr != m_cmdlist[i] && m_cmdlist[i]->isValidCommand((char*)cmd->c_str())){

    //     res = m_cmdlist[i]->executeCommand((char*)cmd->c_str(), cmd->size());

    //     // if( PDI_OK == res ){
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
        originalCmd.size() < 1024
      ){

        // keep only last CMD_TERMINAL_HISTORY_MAX_LINES lines in history
        pdiutil::vector<uint32_t> newlineindices;
        __i_instance.getFileSystemInstance().findInFile(m_termhistoryfile.c_str(), "\r\n", &newlineindices, -1, 1, 0, [&](void){
          // yield function to avoid watchdog reset
          __i_dvc_ctrl.yield();
        });
        if(newlineindices.size() > CMD_TERMINAL_HISTORY_MAX_LINES){
          uint32_t keeplines = CMD_TERMINAL_HISTORY_MAX_LINES - CMD_TERMINAL_HISTORY_ROTATE_SLACK;
          uint32_t remove_upto_index = newlineindices[newlineindices.size() - keeplines - 1];

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
        originalCmd += "\r\n";
        __i_instance.getFileSystemInstance().writeFile(m_termhistoryfile.c_str(), (char*)originalCmd.c_str(), originalCmd.size(), true);
    }
    #endif

    // if command is incomplete then we are in continue execution mode
    if (CMD_ERROR_AGAIN == res){
      is_executing_lastcommand = true;
    }
  }else{

    res = CMD_ERROR_UNSET;

    // Ctrl+C / Ctrl+Z at an idle prompt aborts anything running in the
    // background but MUST NOT drop the session. Transport layers close the
    // channel only on CMD_ERROR_INTR (logout / EOF).
    if(
      inseq == CMD_TERM_INSEQ_CTRL_C ||
      inseq == CMD_TERM_INSEQ_CTRL_Z
    ){
      res = CMD_ERROR_CANCELED;
    }

    // check if any command is waiting for user input
    // if any command is waiting for user input then we are in continue execution mode
    if(is_executing_lastcommand){

      res = CMD_ERROR_AGAIN;

      /* Perform terminal input actions if any */
      if( inseq > CMD_TERM_INSEQ_NONE && inseq < CMD_TERM_INSEQ_MAX ){
        res = m_cmdlist[waitingCmdIndex]->executeCommand((char*)cmd->c_str(), cmd->size(), true, inseq);
      }
    }
  }

  // if command aborted then stop every background-running command owned by
  // the current session
  if( CMD_ERROR_CANCELED == res || CMD_ERROR_INTR == res ){
    session_t *cur = SessionManager::current();
    for (int16_t i = 0; i < m_cmdlist.size(); i++){
      if( nullptr != m_cmdlist[i] && m_cmdlist[i]->m_owner == cur && m_cmdlist[i]->isRunningInBackground() ){
        m_cmdlist[i]->stopRunningInBackground();
      }
    }
    #ifdef ENABLE_SCRIPT_RUNNER
    ScriptRunner::clearSession();
    #endif
  }

  #ifdef ENABLE_SCRIPT_RUNNER
  // whatever the script stopped for has been answered, so it carries on before
  // the prompt is drawn and before this call reaps what it leaves behind
  if( ScriptRunner::pending() && getCommandWaitingForUserInput() < 0 ){
    res = ScriptRunner::resume();
  }
  #endif

  #ifdef ENABLE_AUTH_SERVICE
  bool isWaitingForUserAuth = false;
  if(!__auth_service.getAuthorized()){
    cmd_t *logincmd = __cmd_service.getActiveCommandByName(CMD_NAME_LOGIN);
    if( nullptr != logincmd && logincmd->isWaitingForOption(CMD_OPTION_NAME_U) ){
      isWaitingForUserAuth = true;
    }
  }
  #endif

  if( CMD_ERROR_NOENT == res && !reported_noent && nullptr != cmd && cmd->size() ){

    reportUnknownCommand(cmd->c_str(), (uint16_t)cmd->size());
  }

  if(
    !is_executing_lastcommand ||
    PDI_OK == res ||
    CMD_ERROR_CANCELED == res ||
    CMD_ERROR_FAILED == res
    #ifdef ENABLE_AUTH_SERVICE
    || (isWaitingForUserAuth && res == CMD_ERROR_ACCES)
    #endif
    || (
      CMD_ERROR_AGAIN != res &&
      CMD_ERROR_HOLD_BUFFER != res &&
      CMD_ERROR_INTR != res
    )
  ){
    // start new interaction
    startInteraction();
  }

  // Clean up executed commands.
  reapFinishedCommands();

  // the line is answered, so this is what the session last exited with
  SessionManager::setLastExit(res);

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
bool CommandLineServiceProvider::useTerminal(iTerminalInterface *terminal)
{
  setTerminal(terminal);

  if( nullptr != terminal ){
    session_t *s = SessionManager::attach(terminal);
    if( nullptr == s ){
      SysLogW("CMD: no free session for this terminal\n");
      terminal->writeln();
      terminal->writeln_ro(RODT_ATTR("no session available, try again later"));
      return false;
    }
    for (int16_t i = 0; i < m_cmdlist.size(); i++){
      if(nullptr != m_cmdlist[i] && m_cmdlist[i]->m_owner == s){
        m_cmdlist[i]->SetTerminal(terminal);
      }
    }
    startInteraction();
    return true;
  }

  SessionManager::detachCurrent();
  return false;
}

/**
 * @brief Drop every command belonging to a session that is going away.
 *
 * A command left waiting for input is skipped by the cleanup that runs after
 * each execution, so without this it would outlive its session. Session slots
 * are reused, and the next terminal handed the same slot would inherit the
 * unfinished prompt along with whatever it was still holding.
 *
 * @param session session_t* session being torn down
 */
void CommandLineServiceProvider::releaseSession(session_t *session)
{
  if( nullptr == session ) return;

  for (int16_t i = static_cast<int16_t>(m_cmdlist.size()) - 1; i >= 0; i--){

    if(nullptr != m_cmdlist[i] && m_cmdlist[i]->m_owner == session && !m_cmdlist[i]->isExecuting()){

      pdiutil::safe_delete(m_cmdlist[i]);
      m_cmdlist.erase(m_cmdlist.begin() + i);
    }
  }
}

bool CommandLineServiceProvider::isSessionBusy(session_t *session)
{
  if( nullptr == session ) return false;

  for (int16_t i = 0; i < static_cast<int16_t>(m_cmdlist.size()); i++){

    cmd_t *cmd = m_cmdlist[i];
    if(nullptr != cmd && cmd->m_owner == session &&
       (cmd->isRunningInBackground() || cmd->isWaitingForOption())){
      return true;
    }
  }
  return false;
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
  return PDI_ERR_FAILURE;
}

#ifdef ENABLE_STORAGE_SERVICE
/**
 * Points the session output descriptor at the redirect target, resolved
 * against the working directory. False when the file will not open.
 */
bool CommandLineServiceProvider::openRedirect(const pdiutil::string &target, bool append){

  pdiutil::string path = resolveArgPathStr(target.c_str(), (int16_t)target.size());

  if( path.size() == 0 ){
    return false;
  }

  FileWriteStream *sink = pdiutil::safe_new<FileWriteStream>(path.c_str(), append);

  if( nullptr == sink || !sink->isValid() ){
    pdiutil::safe_delete(sink);
    return false;
  }

  if( !SessionManager::setFd(PDI_FD_STDOUT, sink, true) ){
    pdiutil::safe_delete(sink);
    return false;
  }

  return true;
}

/**
 * Points the session input descriptor at the named file, resolved against the
 * working directory. False when the file will not open.
 */
bool CommandLineServiceProvider::openSource(const pdiutil::string &source){

  pdiutil::string path = resolveArgPathStr(source.c_str(), (int16_t)source.size());

  if( path.size() == 0 ){
    return false;
  }

  FileReadStream *src = pdiutil::safe_new<FileReadStream>(path.c_str());

  if( nullptr == src || !src->isValid() ){
    pdiutil::safe_delete(src);
    return false;
  }

  if( !SessionManager::setFd(PDI_FD_STDIN, src, true) ){
    pdiutil::safe_delete(src);
    return false;
  }

  return true;
}

/**
 * Runs every command of one pipeline in turn, carrying each one's output into
 * the next through a pipe and honouring the redirections each one carries.
 */
pdi_err_t CommandLineServiceProvider::runPipeline(const char *line, const ShellParser::Line &parsed, uint16_t index){

  if( index >= parsed.m_pipelines.size() ){
    return CMD_ERROR_INVAL;
  }

  const ShellParser::Pipeline &pipeline = parsed.m_pipelines[index];

  pdi_err_t res = CMD_ERROR_UNSET;
  PipeStream *carry = nullptr;
  pdiutil::string outpath;
  int16_t first = pipeline.m_command_start;
  int16_t last = (int16_t)(first + pipeline.m_command_count - 1);

  for( int16_t c = first; c <= last; c++ ){

    SessionManager::closeFd(PDI_FD_STDIN);
    SessionManager::closeFd(PDI_FD_STDOUT);

    // input comes from the pipe when there is one, otherwise from whatever
    // this command names for itself
    const ShellParser::Redirect *in = parsed.findRedirect(c, ShellParser::REDIRECT_IN);

    if( nullptr != carry ){
      SessionManager::setFd(PDI_FD_STDIN, carry, false);
    }else if( nullptr != in ){

      pdiutil::string inpath;
      ShellParser::expand(line + in->m_start, in->m_len, SessionManager::getLastExit(), inpath, true);

      if( !openSource(inpath) ){

        m_terminal->writeln();
        m_terminal->write_ro(RODT_ATTR("cannot open "));
        m_terminal->writeln(inpath.c_str());
        SessionManager::resetStdio();
        return CMD_ERROR_INVAL;
      }
    }

    const ShellParser::Redirect *out = parsed.findRedirect(c, ShellParser::REDIRECT_OUT);
    bool append = false;

    if( nullptr == out ){
      out = parsed.findRedirect(c, ShellParser::REDIRECT_APPEND);
      append = ( nullptr != out );
    }

    PipeStream *outpipe = nullptr;
    bool redirected = false;

    if( c < last ){

      outpipe = pdiutil::safe_new<PipeStream>();

      if( nullptr == outpipe || !outpipe->isValid() ){
        pdiutil::safe_delete(outpipe);
        pdiutil::safe_delete(carry);
        m_terminal->writeln();
        m_terminal->writeln_ro(RODT_ATTR("cannot open pipe"));
        SessionManager::resetStdio();
        return CMD_ERROR_FAILED;
      }

      // the pipe still carries to the next command, but a redirect this
      // command names for itself is applied after it and takes the output
      if( nullptr == out ){
        SessionManager::setFd(PDI_FD_STDOUT, outpipe, false);
      }
    }

    if( nullptr != out ){

      outpath.clear();
      ShellParser::expand(line + out->m_start, out->m_len, SessionManager::getLastExit(), outpath, true);

      if( !openRedirect(outpath, append) ){

        pdiutil::safe_delete(outpipe);
        pdiutil::safe_delete(carry);
        m_terminal->writeln();
        m_terminal->write_ro(RODT_ATTR("cannot open "));
        m_terminal->writeln(outpath.c_str());
        SessionManager::resetStdio();
        return CMD_ERROR_INVAL;
      }

      redirected = true;
    }

    pdiutil::string text;
    if( !parsed.commandText(line, c, text, SessionManager::getLastExit()) ){
      pdiutil::safe_delete(carry);
      SessionManager::resetStdio();
      return CMD_ERROR_INVAL;
    }

    cmd_t* cmd_to_exec = getCommandToExecute(text.c_str());

    if( nullptr == cmd_to_exec ){
      reportUnknownCommand(text.c_str(), (uint16_t)text.size());
    }

    res = ( nullptr != cmd_to_exec ) ?
          cmd_to_exec->executeCommand((char*)text.c_str(), text.size()) :
          (pdi_err_t)CMD_ERROR_NOENT;

    // storage says whether it took the write only when the last block is
    // committed, so the target is closed here rather than left to the reset,
    // which has nowhere to report a refusal
    if( redirected ){

      iTerminalInterface *sink = SessionManager::getFd(PDI_FD_STDOUT);

      if( nullptr != sink && sink->disconnect() < 0 ){

        m_terminal->writeln();
        m_terminal->write_ro(RODT_ATTR("cannot write "));
        m_terminal->writeln(outpath.c_str());
        res = CMD_ERROR_FAILED;
      }
    }

    pdiutil::safe_delete(carry);
    carry = outpipe;

    // a command that stopped early leaves the rest of the pipeline nothing
    // worth running
    if( CMD_ERROR_AGAIN == res ){
      break;
    }
  }

  pdiutil::safe_delete(carry);

  SessionManager::resetStdio();
  return res;
}
#endif

/**
 * @brief Mark command to be execute and add it in cmdlist for further operations
 */
/**
 * @brief Report a command name the registry does not hold.
 */
void CommandLineServiceProvider::reportUnknownCommand(const char *text, uint16_t len){

  if( nullptr == text || 0 == len ){
    return;
  }

  uint16_t namelen = 0;
  while( namelen < len && text[namelen] != ' ' ) namelen++;

  m_terminal->writeln();
  m_terminal->write(text, namelen);
  m_terminal->writeln_ro(RODT_ATTR(": command not found"));
}

cmd_t* CommandLineServiceProvider::getCommandToExecute(const char *cmdname){

  cmd_t* cmd_to_exec = CommandBase::GetCommand(cmdname);

  if(nullptr != cmd_to_exec){

    session_t *owner = SessionManager::current();
    SessionStdio *io = SessionManager::stdioFor(owner);

    cmd_to_exec->SetTerminal(nullptr != io ? (iTerminalInterface *)io : m_terminal);
    cmd_to_exec->m_owner = owner;
    m_cmdlist.push_back(cmd_to_exec);
  }

  return cmd_to_exec;
}

CommandLineServiceProvider __cmd_service;

#endif