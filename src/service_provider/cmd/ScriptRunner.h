/**************************** Script Runner ***********************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Reads a file of shell lines and runs each one through the shell that a typed
line goes through, so a script has the whole grammar without a grammar of its
own. Blank lines and comments are the only things it reads for itself.

Where it has stopped is held by the session rather than by the call stack, so
a line that asks for input can hand the terminal back and be picked up again
once the answer arrives.

Author          : Suraj I.
created Date    : 5th Sep 2026
******************************************************************************/
#ifndef _SCRIPT_RUNNER_H_
#define _SCRIPT_RUNNER_H_

#include <config/Config.h>
#include <utility/DataTypeDef.h>

#ifdef ENABLE_SCRIPT_RUNNER

class ScriptRunner {

public:

  /**
   * Runs the file in the session that asked for it, so a cd or an export the
   * script performs is still in force once it returns.
   */
  static pdi_err_t run(const char *path, bool exitonprompt = true,
                       const pdiutil::vector<pdiutil::string> *args = nullptr);

  /**
   * The arguments of the innermost script that was given any, or none outside
   * a script; a source with no arguments sees its caller's.
   */
  static const pdiutil::vector<pdiutil::string> *arguments();

  /**
   * Carries on with the script the session stopped in, once whatever asked
   * for input has been answered.
   */
  static pdi_err_t resume();

  /**
   * Whether the session has a script it has not finished.
   */
  static bool pending();

  /**
   * Abandons whatever the session was running, for a login ending or an
   * operator interrupting.
   */
  static void clearSession();

  /**
   * Runs a script nobody is watching under the identity it names, and does
   * nothing at all when it names one the user store cannot resolve.
   */
  static pdi_err_t runScheduledScript(const char *path);

  /**
   * The identity a script's header asks to run under, false when it names one
   * that does not exist and the script must therefore not run at all.
   */
  static bool declaredUid(const char *path, uint16_t &uid, bool &named);

  /**
   * Runs one command line with nobody watching, so a scheduled job runs as the
   * identity it was given rather than as whoever last used the console.
   */
  static pdi_err_t runDetachedLine(const char *line, uint16_t uid);

private:

  /**
   * Opens a session on the given terminal under the identity named, refusing
   * when that terminal already holds one so nothing can borrow a live session.
   */
  static session_t *beginDetached(uint16_t uid, iTerminalInterface *terminal);

  /**
   * Gives the terminal back once detached work is done, so the console can be
   * logged into again.
   */
  static void endDetached(iTerminalInterface *terminal);

  enum keyword_t : uint8_t {
    KEYWORD_NONE = 0,
    KEYWORD_IF,
    KEYWORD_ELSE,
    KEYWORD_FI,
    KEYWORD_WHILE,
    KEYWORD_DONE,
    KEYWORD_FOR,
    KEYWORD_ELIF,
    KEYWORD_BREAK,
    KEYWORD_CONTINUE,
    KEYWORD_EXIT
  };

  /**
   * The innermost open while or for, or -1 when the line is in no loop.
   */
  static int16_t innermostLoop(script_frame_t &frame);

  /**
   * Goes back to the head of the innermost block's loop for its next pass, or
   * fails when the loop has used every pass it is allowed.
   */
  static bool loopBack(script_frame_t &frame);

  /**
   * Runs lines from the innermost open script until the stack is back down to
   * the given depth or one of them stops for input.
   */
  static pdi_err_t drain(uint32_t floor);

  /**
   * What the line opens or closes, and the command left after the keyword for
   * the ones that carry a condition.
   */
  static keyword_t keyword(const pdiutil::string &line, pdiutil::string &rest);

  /**
   * Whether the innermost open block is passing its lines over rather than
   * running them.
   */
  static bool skipping(script_frame_t &frame);

  /**
   * Reads a for line into the name it binds and the word its pass takes, with
   * how many words the list holds so the caller knows when it is spent.
   */
  static bool loopWord(const pdiutil::string &rest, uint32_t index, pdiutil::string &name,
                       pdiutil::string &value, uint32_t &count);

  /**
   * Opens a for block on the frame, binding the name to the word this pass
   * takes and marking the block spent when the list has no word left for it.
   */
  static pdi_err_t openForBlock(script_frame_t &frame, const pdiutil::string &rest);

};

#endif

#endif
