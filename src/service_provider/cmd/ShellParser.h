/***************************** Shell Parser ***********************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Reads a typed line the way a shell reads one: a tokenizer that knows quoting and
escapes, then a grammar over those tokens.

    line     : and_or ( ';' and_or )* [';']
    and_or   : pipeline ( ('&&' | '||') pipeline )*
    pipeline : command ( '|' command )*
    command  : word+ redirect*
    redirect : ('<' | '>' | '>>') word

The result is held as flat tables joined by index ranges rather than nested
containers, so one line costs a handful of allocations and a new token type or
redirection form can be added without reshaping it.

Author          : Suraj I.
created Date    : 26th Aug 2026
******************************************************************************/
#ifndef _SHELL_PARSER_H_
#define _SHELL_PARSER_H_

#include <config/Config.h>
#include <utility/DataTypeDef.h>

#if defined(ENABLE_CMD_SERVICE) && defined(ENABLE_STORAGE_SERVICE)

class ShellParser {

public:

  // What the tokenizer found. Operators are the only non-word kinds today;
  // a new one (2>, >&, &) becomes a new value here and a case in the grammar.
  enum token_type_t : uint8_t {
    TOKEN_WORD = 0,
    TOKEN_PIPE,   ///< |
    TOKEN_AND_IF, ///< &&
    TOKEN_OR_IF,  ///< ||
    TOKEN_SEMI,   ///< ;
    TOKEN_LT,     ///< <
    TOKEN_GT,     ///< >
    TOKEN_DGT     ///< >>
  };

  // A span of the original line. Nothing is copied while parsing.
  struct Token {

    Token() : m_type(TOKEN_WORD), m_start(0), m_len(0) {}

    token_type_t m_type;
    int16_t m_start;
    int16_t m_len;
  };

  // What has to be true of the previous pipeline for this one to run.
  enum join_t : uint8_t {
    JOIN_FIRST = 0,  ///< nothing precedes it
    JOIN_ALWAYS,     ///< ';'
    JOIN_ON_SUCCESS, ///< '&&'
    JOIN_ON_FAILURE  ///< '||'
  };

  enum redirect_op_t : uint8_t {
    REDIRECT_IN = 0, ///< <
    REDIRECT_OUT,    ///< >
    REDIRECT_APPEND  ///< >>
  };

  // One redirection and the span of the path it names.
  struct Redirect {

    Redirect() : m_op(REDIRECT_IN), m_start(0), m_len(0) {}

    redirect_op_t m_op;
    int16_t m_start;
    int16_t m_len;
  };

  // One command: a run of words, and the redirections attached to it.
  struct Command {

    Command() : m_word_start(0), m_word_count(0), m_redirect_start(0), m_redirect_count(0) {}

    int16_t m_word_start;     ///< first index into Line::m_words
    int16_t m_word_count;
    int16_t m_redirect_start; ///< first index into Line::m_redirects
    int16_t m_redirect_count;
  };

  // One pipeline, and how it attaches to the pipeline before it.
  struct Pipeline {

    Pipeline() : m_command_start(0), m_command_count(0), m_join(JOIN_FIRST), m_negate(false) {}

    int16_t m_command_start; ///< first index into Line::m_commands
    int16_t m_command_count;
    join_t m_join;
    bool m_negate;           ///< a leading '!' turns its answer around
  };

  // What a $(...) answered, keyed by where its '$' sits in the line, so the
  // answer is put in as text after parsing and never read as a line itself.
  struct Capture {

    Capture() : m_at(nullptr) {}

    const char *m_at;
    pdiutil::string m_value;
  };

  typedef pdiutil::vector<Capture> Captures;

  // A whole line. The tables are flat and joined by index, so nothing nests.
  struct Line {

    Line() : m_malformed(false) {}

    /**
     * Whether the line needs descriptors at all.
     */
    bool isPlain() const {
      return 1 == m_pipelines.size() && !m_pipelines[0].m_negate && 1 == m_commands.size() &&
             0 == m_redirects.size();
    }

    /**
     * Text of one command without its redirections, expansions applied.
     */
    bool commandText(const char *line, int16_t index, pdiutil::string &out,
                     pdi_err_t lastexit = PDI_OK, const Captures *captures = nullptr) const;

    /**
     * Redirection of the given kind on one command, if it has one.
     */
    const Redirect *findRedirect(int16_t command, redirect_op_t op) const;

    pdiutil::vector<Pipeline> m_pipelines;
    pdiutil::vector<Command> m_commands;
    pdiutil::vector<Redirect> m_redirects;
    pdiutil::vector<Token> m_words;
    bool m_malformed;
  };

  /**
   * Splits a line into tokens, honouring quotes and backslash escapes.
   * False when a quote is never closed.
   */
  static bool tokenize(const char *line, int16_t len, pdiutil::vector<Token> &tokens);

  /**
   * Reads a whole line into pipelines, commands and redirections.
   */
  static Line parse(const char *line, int16_t len);

  /**
   * Copies a span with expansions applied. A single quoted run is literal, so
   * nothing inside it expands; a double quoted one still does. With unquote
   * the quoting is taken off what is produced, for a span the shell consumes
   * itself; a command's own text keeps it, because the command uses it to
   * bound an option value against its separator.
   */
  static void expand(const char *src, int16_t len, pdi_err_t lastexit, pdiutil::string &out,
                     bool unquote = false, const Captures *captures = nullptr);

  /**
   * Where the ')' closing the $( at open sits, or -1 when nothing closes it.
   */
  static int16_t substitutionEnd(const char *src, int16_t len, int16_t open);

  /**
   * Where the '}' closing the ${ at open sits, or -1 when nothing closes it.
   */
  static int16_t braceEnd(const char *src, int16_t len, int16_t open);

  /**
   * Every $(...) a span holds outside single quotes, each as the run from its
   * '$' to its ')', in the order they appear.
   */
  static void substitutions(const char *src, int16_t len, pdiutil::vector<Token> &spans);

  /**
   * Whether a span holds anything the expander would rewrite.
   */
  static bool expandable(const char *src, int16_t len);

  /**
   * Whether a pipeline runs, given what the one before it answered.
   */
  static bool shouldRun(join_t join, pdi_err_t previous);
};

#endif

#endif
