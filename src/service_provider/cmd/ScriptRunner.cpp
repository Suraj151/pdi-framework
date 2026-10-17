/**************************** Script Runner ***********************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 5th Sep 2026
******************************************************************************/

#include "ScriptRunner.h"

#ifdef ENABLE_SCRIPT_RUNNER

#include "CommandLineServiceProvider.h"
#include "ShellParser.h"
#include <service_provider/session/SessionManager.h>
#include <service_provider/session/Environment.h>
#include <utility/StringOperations.h>
#include <interface/pdi/impl/modules/terminal/NullTerminal.h>
#include <utility/DataTypeConversions.h>

#ifdef ENABLE_AUTH_SERVICE
#include <service_provider/user/UserStoreService.h>
#endif

/**
 * What the line opens or closes, and the command left after the keyword for
 * the ones that carry a condition.
 */
ScriptRunner::keyword_t ScriptRunner::keyword(const pdiutil::string &line, pdiutil::string &rest)
{
  rest.clear();

  pdiutil::string::size_type end = 0;
  while( end < line.size() && !__is_blank(line[end]) ) end++;

  pdiutil::string word = line.substr(0, end);

  keyword_t found = KEYWORD_NONE;

  switch( word.empty() ? '\0' : word[0] ){

    case 'i': {
      pdiutil::string kwif = CHARPTR_WRAP(SCRIPT_KEYWORD_IF);
      if( word == kwif ) found = KEYWORD_IF;
      break;
    }
    case 'e': {
      pdiutil::string kwelse = CHARPTR_WRAP(SCRIPT_KEYWORD_ELSE);
      if( word == kwelse ){ found = KEYWORD_ELSE; break; }
      pdiutil::string kwelif = CHARPTR_WRAP(SCRIPT_KEYWORD_ELIF);
      if( word == kwelif ){ found = KEYWORD_ELIF; break; }
      pdiutil::string kwexit = CHARPTR_WRAP(SCRIPT_KEYWORD_EXIT);
      if( word == kwexit ) found = KEYWORD_EXIT;
      break;
    }
    case 'b': {
      pdiutil::string kwbreak = CHARPTR_WRAP(SCRIPT_KEYWORD_BREAK);
      if( word == kwbreak ) found = KEYWORD_BREAK;
      break;
    }
    case 'c': {
      pdiutil::string kwcontinue = CHARPTR_WRAP(SCRIPT_KEYWORD_CONTINUE);
      if( word == kwcontinue ) found = KEYWORD_CONTINUE;
      break;
    }
    case 'f': {
      pdiutil::string kwfi = CHARPTR_WRAP(SCRIPT_KEYWORD_FI);
      if( word == kwfi ){ found = KEYWORD_FI; break; }
      pdiutil::string kwfor = CHARPTR_WRAP(SCRIPT_KEYWORD_FOR);
      if( word == kwfor ) found = KEYWORD_FOR;
      break;
    }
    case 'w': {
      pdiutil::string kwwhile = CHARPTR_WRAP(SCRIPT_KEYWORD_WHILE);
      if( word == kwwhile ) found = KEYWORD_WHILE;
      break;
    }
    case 'd': {
      pdiutil::string kwdone = CHARPTR_WRAP(SCRIPT_KEYWORD_DONE);
      if( word == kwdone ) found = KEYWORD_DONE;
      break;
    }
    default:
      break;
  }

  if( KEYWORD_NONE == found ){
    return found;
  }

  while( end < line.size() && __is_blank(line[end]) ) end++;
  if( end < line.size() ){
    rest = line.substr(end);
  }

  return found;
}

/**
 * Whether the innermost open block is passing its lines over rather than
 * running them.
 */
bool ScriptRunner::skipping(script_frame_t &frame)
{
  for( uint32_t i = 0; i < frame.m_blocks.size(); i++ ){
    if( frame.m_blocks[i].m_skipping ) return true;
  }
  return false;
}

/**
 * The innermost open while or for, or -1 when the line is in no loop.
 */
int16_t ScriptRunner::innermostLoop(script_frame_t &frame)
{
  for( int16_t i = (int16_t)frame.m_blocks.size() - 1; i >= 0; i-- ){
    if( SCRIPT_BLOCK_IF != frame.m_blocks[i].m_kind ) return i;
  }
  return -1;
}

/**
 * Goes back to the head of the innermost block's loop for its next pass, or
 * fails when the loop has used every pass it is allowed.
 */
bool ScriptRunner::loopBack(script_frame_t &frame)
{
  if( frame.m_blocks.back().m_passes + 1 >= SCRIPT_LOOP_MAX ){
    return false;
  }

  frame.m_line = frame.m_blocks.back().m_head;
  frame.m_loop_passes = frame.m_blocks.back().m_passes + 1;
  frame.m_blocks.pop_back();
  return true;
}

/**
 * Reads a for line into the name it binds and the word its pass takes, with
 * how many words the list holds so the caller knows when it is spent.
 */
bool ScriptRunner::loopWord(const pdiutil::string &rest, uint32_t index, pdiutil::string &name,
                            pdiutil::string &value, uint32_t &count)
{
  name.clear();
  value.clear();
  count = 0;

  pdiutil::vector<ShellParser::Token> tokens;

  if( !ShellParser::tokenize(rest.c_str(), (int16_t)rest.size(), tokens) || tokens.size() < 2 ){
    return false;
  }

  if( ShellParser::TOKEN_WORD != tokens[0].m_type || ShellParser::TOKEN_WORD != tokens[1].m_type ){
    return false;
  }

  name = rest.substr(tokens[0].m_start, tokens[0].m_len);

  pdiutil::string joiner = rest.substr(tokens[1].m_start, tokens[1].m_len);
  pdiutil::string kwin = CHARPTR_WRAP(SCRIPT_KEYWORD_IN);

  if( joiner != kwin ){
    return false;
  }

  if( !Environment::isValidName(name.c_str()) || Environment::isDerived(name.c_str()) ){
    return false;
  }

  pdi_err_t lastexit = SessionManager::getLastExit();

  for( uint32_t i = 2; i < tokens.size(); i++ ){

    if( ShellParser::TOKEN_WORD != tokens[i].m_type ){
      return false;
    }

    const char *span = rest.c_str() + tokens[i].m_start;
    int16_t len = tokens[i].m_len;

    bool quoted = false;

    for( int16_t c = 0; c < len; c++ ){
      if( '\'' == span[c] || '"' == span[c] ) quoted = true;
    }

    pdiutil::string text;
    ShellParser::expand(span, len, lastexit, text, true);

    if( quoted ){

      if( index == count ) value = text;
      count++;
      continue;
    }

    pdiutil::string::size_type at = 0;

    while( at < text.size() ){

      while( at < text.size() && __is_blank(text[at]) ) at++;
      if( at >= text.size() ) break;

      pdiutil::string::size_type end = at;
      while( end < text.size() && !__is_blank(text[end]) ) end++;

      if( index == count ) value = text.substr(at, end - at);
      count++;
      at = end;
    }
  }

  return true;
}

/**
 * Opens a for block on the frame, binding the name to the word this pass takes
 * and marking the block spent when the list has no word left for it.
 */
pdi_err_t ScriptRunner::openForBlock(script_frame_t &frame, const pdiutil::string &rest)
{
  pdiutil::string name;
  pdiutil::string value;
  uint32_t count = 0;

  uint32_t index = frame.m_loop_passes;
  frame.m_loop_passes = 0;

  if( frame.m_blocks.size() >= SCRIPT_BLOCK_MAX || !loopWord(rest, index, name, value, count) ){
    return CMD_ERROR_FAILED;
  }

  script_block_t block;
  block.m_kind = SCRIPT_BLOCK_FOR;
  block.m_head = frame.m_line - 1;
  block.m_passes = index;
  block.m_taken = true;

  block.m_skipping = (index >= count);

  if( !block.m_skipping ){
    Environment::set(name.c_str(), value.c_str());
  }

  frame.m_blocks.push_back(block);

  return PDI_OK;
}

/**
 * Runs lines from the innermost open script until the stack is back down to
 * the given depth or one of them stops for input.
 */
pdi_err_t ScriptRunner::drain(uint32_t floor)
{
  session_t *session = SessionManager::current();

  if( nullptr == session ){
    return CMD_ERROR_FAILED;
  }

  pdi_err_t last = SessionManager::getLastExit();

  while( session->m_scripts.size() > floor ){

    __i_dvc_ctrl.yield();

    SessionManager::setCurrent(session);

    pdiutil::string statement;
    bool runnable = false;

    {
      script_frame_t &frame = session->m_scripts.back();

      pdiutil::string line;
      int rc = __i_fs.readLineInFile(frame.m_path.c_str(), frame.m_line, line, nullptr, [](void){
        __i_dvc_ctrl.yield();
      });

      if( rc < 0 ){

        bool ended = (PDI_ERR_NOT_FOUND == rc);

        bool unclosed = frame.m_blocks.size() > 0;

        session->m_scripts.pop_back();

        if( !ended || unclosed ){
          return CMD_ERROR_FAILED;
        }

        continue;
      }

      frame.m_line++;

      pdiutil::string::size_type start = 0;
      while( start < line.size() && __is_blank(line[start]) ) start++;

      if( start < line.size() && SCRIPT_COMMENT_CHAR != line[start] ){
        statement = line.substr(start);
        runnable = true;
      }
    }

    if( !runnable ){
      continue;
    }

    pdiutil::string rest;
    keyword_t word = keyword(statement, rest);
    bool branch = false;

    {
      script_frame_t &frame = session->m_scripts.back();

      if( skipping(frame) ){

        if( KEYWORD_IF == word || KEYWORD_WHILE == word || KEYWORD_FOR == word ){

          if( frame.m_blocks.size() >= SCRIPT_BLOCK_MAX ){
            session->m_scripts.clear();
            return CMD_ERROR_FAILED;
          }

          script_block_t block;

          if( KEYWORD_IF == word ) block.m_kind = SCRIPT_BLOCK_IF;
          else if( KEYWORD_WHILE == word ) block.m_kind = SCRIPT_BLOCK_WHILE;
          else block.m_kind = SCRIPT_BLOCK_FOR;

          block.m_taken = true;
          block.m_skipping = true;
          frame.m_blocks.push_back(block);

        }else if( KEYWORD_FI == word || KEYWORD_DONE == word ){

          if( 0 == frame.m_blocks.size() ||
              (KEYWORD_FI == word) != (SCRIPT_BLOCK_IF == frame.m_blocks.back().m_kind) ){
            session->m_scripts.clear();
            return CMD_ERROR_FAILED;
          }

          if( frame.m_blocks.back().m_continue ){
            if( !loopBack(frame) ){
              session->m_scripts.clear();
              return CMD_ERROR_FAILED;
            }
          }else{
            frame.m_blocks.pop_back();
          }

        }else if( KEYWORD_ELIF == word ){

          if( 0 == frame.m_blocks.size() ||
              SCRIPT_BLOCK_IF != frame.m_blocks.back().m_kind ){
            session->m_scripts.clear();
            return CMD_ERROR_FAILED;
          }

          script_block_t &block = frame.m_blocks.back();

          if( block.m_skipping && !block.m_taken ){
            if( rest.empty() ){
              session->m_scripts.clear();
              return CMD_ERROR_FAILED;
            }
            statement = rest;
            branch = true;
          }

        }else if( KEYWORD_ELSE == word ){

          if( 0 == frame.m_blocks.size() ||
              SCRIPT_BLOCK_IF != frame.m_blocks.back().m_kind ){
            session->m_scripts.clear();
            return CMD_ERROR_FAILED;
          }

          script_block_t &block = frame.m_blocks.back();

          if( block.m_skipping && !block.m_taken ){
            block.m_skipping = false;
            block.m_taken = true;
          }
        }

        if( !branch ){
          continue;
        }
      }

      if( KEYWORD_ELIF == word && !branch ){

        if( 0 == frame.m_blocks.size() ||
            SCRIPT_BLOCK_IF != frame.m_blocks.back().m_kind ){
          session->m_scripts.clear();
          return CMD_ERROR_FAILED;
        }

        frame.m_blocks.back().m_skipping = true;
        continue;
      }

      if( KEYWORD_BREAK == word || KEYWORD_CONTINUE == word ){

        int16_t loop = innermostLoop(frame);
        if( loop < 0 ){
          session->m_scripts.clear();
          return CMD_ERROR_FAILED;
        }

        frame.m_blocks[loop].m_skipping = true;
        frame.m_blocks[loop].m_taken = true;
        frame.m_blocks[loop].m_continue = (KEYWORD_CONTINUE == word);
        continue;
      }

      if( KEYWORD_EXIT == word ){

        if( !rest.empty() ){
          int32_t status = 0;
          pdiutil::string::size_type digits = 0;
          bool number = !rest.empty();
          for( pdiutil::string::size_type d = 0; d < rest.size(); d++ ){
            if( 0 == d && '-' == rest[d] ) continue;
            if( !__is_digit(rest[d]) ){ number = false; break; }
            digits++;
          }
          if( !number || 0 == digits ){
            session->m_scripts.clear();
            return CMD_ERROR_FAILED;
          }
          status = StringToInt32(rest.c_str());
          last = (pdi_err_t)status;
        }

        SessionManager::setLastExit(last);
        session->m_scripts.pop_back();
        continue;
      }

      if( KEYWORD_FI == word || KEYWORD_ELSE == word ){

        if( 0 == frame.m_blocks.size() ||
            SCRIPT_BLOCK_IF != frame.m_blocks.back().m_kind ){
          session->m_scripts.clear();
          return CMD_ERROR_FAILED;
        }

        if( KEYWORD_FI == word ){
          frame.m_blocks.pop_back();
        }else{
          frame.m_blocks.back().m_skipping = frame.m_blocks.back().m_taken;
        }

        continue;
      }

      if( KEYWORD_DONE == word ){

        if( 0 == frame.m_blocks.size() ||
            SCRIPT_BLOCK_IF == frame.m_blocks.back().m_kind ){
          session->m_scripts.clear();
          return CMD_ERROR_FAILED;
        }

        if( !loopBack(frame) ){
          session->m_scripts.clear();
          return CMD_ERROR_FAILED;
        }

        continue;
      }

      if( KEYWORD_FOR == word ){

        if( PDI_OK != openForBlock(frame, rest) ){
          session->m_scripts.clear();
          return CMD_ERROR_FAILED;
        }

        continue;
      }

      if( KEYWORD_IF == word || KEYWORD_WHILE == word ){

        if( frame.m_blocks.size() >= SCRIPT_BLOCK_MAX || rest.empty() ){
          session->m_scripts.clear();
          return CMD_ERROR_FAILED;
        }

        script_block_t block;
        block.m_kind = (KEYWORD_IF == word) ? SCRIPT_BLOCK_IF : SCRIPT_BLOCK_WHILE;
        block.m_head = frame.m_line - 1;

        block.m_passes = frame.m_loop_passes;
        frame.m_loop_passes = 0;

        frame.m_blocks.push_back(block);

        statement = rest;
      }
    }

    bool named_noent = false;
    last = __cmd_service.runLine(statement, named_noent);
    SessionManager::setLastExit(last);

    if( branch ){
      script_block_t &block = session->m_scripts.back().m_blocks.back();
      block.m_taken = (PDI_OK == last);
      block.m_skipping = (PDI_OK != last);
    }

    if( KEYWORD_IF == word || KEYWORD_WHILE == word ){

      script_frame_t &frame = session->m_scripts.back();
      script_block_t &block = frame.m_blocks.back();

      block.m_taken = (PDI_OK == last);
      block.m_skipping = (PDI_OK != last);

      if( SCRIPT_BLOCK_WHILE == block.m_kind && block.m_skipping ){
        block.m_taken = true;
      }
    }

    int16_t waiting = __cmd_service.getCommandWaitingForUserInput();

    if( waiting >= 0 ){

      if( session->m_script_exitonprompt ){

        if( nullptr != __cmd_service.m_cmdlist[waiting] ){
          __cmd_service.m_cmdlist[waiting]->executeTermInputAction(CMD_TERM_INSEQ_CTRL_C);
        }

        session->m_scripts.clear();
        return CMD_ERROR_CANCELED;
      }

      return CMD_ERROR_AGAIN;
    }

    __cmd_service.reapFinishedCommands();
  }

  return last;
}

/**
 * Runs the file in the session that asked for it, so a cd or an export the
 * script performs is still in force once it returns.
 */
pdi_err_t ScriptRunner::run(const char *path, bool exitonprompt,
                            const pdiutil::vector<pdiutil::string> *args)
{
  session_t *session = SessionManager::current();

  if( nullptr == session || nullptr == path ){
    return CMD_ERROR_FAILED;
  }

  if( session->m_scripts.size() >= SCRIPT_MAX_DEPTH ){
    return CMD_ERROR_FAILED;
  }

  if( !__i_fs.isFileExist(path) ){
    return CMD_ERROR_FAILED;
  }

  uint32_t floor = (uint32_t)session->m_scripts.size();

  if( 0 == floor ){
    session->m_script_exitonprompt = exitonprompt;
  }

  script_frame_t frame;
  frame.m_path = path;
  frame.m_line = 0;
  if( nullptr != args ){
    frame.m_args = *args;
  }
  session->m_scripts.push_back(frame);

  return drain(floor);
}

/**
 * The arguments of the innermost script that was given any, or none outside
 * a script; a source with no arguments sees its caller's.
 */
const pdiutil::vector<pdiutil::string> *ScriptRunner::arguments()
{
  session_t *session = SessionManager::current();

  if( nullptr == session ) return nullptr;

  for( uint32_t i = session->m_scripts.size(); i > 0; i-- ){
    if( !session->m_scripts[i - 1].m_args.empty() ){
      return &session->m_scripts[i - 1].m_args;
    }
  }

  return nullptr;
}

/**
 * Carries on with the script the session stopped in, once whatever asked
 * for input has been answered.
 */
pdi_err_t ScriptRunner::resume()
{
  return drain(0);
}

/**
 * Whether the session has a script it has not finished.
 */
bool ScriptRunner::pending()
{
  session_t *session = SessionManager::current();
  return (nullptr != session) && (session->m_scripts.size() > 0);
}

/**
 * Abandons whatever the session was running, for a login ending or an
 * operator interrupting.
 */
void ScriptRunner::clearSession()
{
  session_t *session = SessionManager::current();

  if( nullptr == session ) return;

  session->m_scripts.clear();
  session->m_script_exitonprompt = true;
}

/**
 * The identity a script's header asks to run under, false when it names one
 * that does not exist and the script must therefore not run at all.
 */
bool ScriptRunner::declaredUid(const char *path, uint16_t &uid, bool &named)
{
  named = false;
#ifdef ENABLE_AUTH_SERVICE
  uid = USER_STORE_ROOT_UID;
#else
  uid = 0;
#endif

  pdiutil::string directive = CHARPTR_WRAP(SCRIPT_UID_DIRECTIVE);

  for( int32_t n = 0; ; n++ ){

    __i_dvc_ctrl.yield();

    pdiutil::string line;

    if( __i_fs.readLineInFile(path, n, line, nullptr, [](void){
          __i_dvc_ctrl.yield();
        }) < 0 ){
      break;
    }

    pdiutil::string::size_type start = 0;
    while( start < line.size() && __is_blank(line[start]) ) start++;

    if( start >= line.size() ){
      continue;
    }

    if( SCRIPT_COMMENT_CHAR != line[start] ){
      break;
    }

    start++;
    while( start < line.size() && __is_blank(line[start]) ) start++;

    if( line.compare(start, directive.size(), directive) != 0 ){
      continue;
    }

    pdiutil::string::size_type at = start + directive.size();
    while( at < line.size() && __is_blank(line[at]) ) at++;

    if( at >= line.size() || !__is_digit(line[at]) ){
      continue;
    }

    pdiutil::string value = line.substr(at);
    uid = (uint16_t)StringToUint32(value.c_str(), (uint8_t)value.size());
    named = true;
    break;
  }

  if( !named ){
    return true;
  }

#ifdef ENABLE_AUTH_SERVICE
  user_record_t rec;
  return __user_store_service.findUserByUid(uid, rec);
#else
  return false;
#endif
}

/**
 * Runs a script nobody is watching under the identity it names, and does
 * nothing at all when it names one the user store cannot resolve.
 */
/**
 * Opens a session on the given terminal under the identity named, refusing
 * when that terminal already holds one so nothing can borrow a live session.
 */
session_t *ScriptRunner::beginDetached(uint16_t uid, iTerminalInterface *terminal)
{
  if( nullptr == terminal || nullptr != SessionManager::findByTerminal(terminal) ){
    return nullptr;
  }

  session_t *session = SessionManager::attach(terminal);

  if( nullptr == session ){
    return nullptr;
  }

  session->m_state = SESSION_STATE_INTERACTIVE;

#ifdef ENABLE_AUTH_SERVICE
  user_record_t rec;
  bool found = __user_store_service.findUserByUid(uid, rec);

  session->m_isAuthorized = true;
  session->m_uid = uid;
  session->m_gid = found ? rec.m_gid : (uint16_t)USER_STORE_ROOT_GID;

  if( found ){
    session->m_username = rec.m_username;
  }

  if( found && !rec.m_home.empty() ){
    SessionManager::setPWD(rec.m_home.c_str());
  }else{
    SessionManager::setPWD(__i_fs.getRootDirectory());
  }
#else
  (void)uid;
  SessionManager::setPWD(__i_fs.getRootDirectory());
#endif

  return session;
}

/**
 * Gives the terminal back once detached work is done, so the console can be
 * logged into again.
 */
void ScriptRunner::endDetached(iTerminalInterface *terminal)
{
  SessionManager::detach(terminal);
  SessionManager::setCurrent(nullptr);
}

pdi_err_t ScriptRunner::runScheduledScript(const char *path)
{
  if( nullptr == path || !__i_fs.isFileExist(path) ){
    return PDI_OK;
  }

#ifdef ENABLE_AUTH_SERVICE
  uint16_t uid = USER_STORE_ROOT_UID;
#else
  uint16_t uid = 0;
#endif
  bool named = false;

  if( !declaredUid(path, uid, named) ){
    return CMD_ERROR_PERM;
  }

  iTerminalInterface *terminal = __cmd_service.m_terminal;
  session_t *session = beginDetached(uid, terminal);

  if( nullptr == session ){
    return CMD_ERROR_FAILED;
  }

  pdi_err_t res = run(path, true);

  endDetached(terminal);

  return res;
}

/**
 * Runs one command line with nobody watching, so a scheduled job runs as the
 * identity it was given rather than as whoever last used the console.
 */
pdi_err_t ScriptRunner::runDetachedLine(const char *line, uint16_t uid)
{
  if( nullptr == line || '\0' == line[0] ){
    return PDI_OK;
  }

  iTerminalInterface *terminal = &__i_null_terminal;
  session_t *session = beginDetached(uid, terminal);

  if( nullptr == session ){
    return CMD_ERROR_FAILED;
  }

  pdiutil::string statement = line;
  bool named_noent = false;

  pdi_err_t res = __cmd_service.runLine(statement, named_noent);

  int16_t waiting = __cmd_service.getCommandWaitingForUserInput();

  if( waiting >= 0 ){

    if( nullptr != __cmd_service.m_cmdlist[waiting] ){
      __cmd_service.m_cmdlist[waiting]->executeTermInputAction(CMD_TERM_INSEQ_CTRL_C);
    }

    res = CMD_ERROR_CANCELED;
  }

  __cmd_service.reapFinishedCommands();
  endDetached(terminal);

  return res;
}

#endif
