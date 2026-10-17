/***************************** Shell Parser ***********************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 26th Aug 2026
******************************************************************************/

#include "ShellParser.h"
#include <utility/DataTypeConversions.h>
#include <utility/StringOperations.h>
#include <service_provider/session/Environment.h>
#ifdef ENABLE_SCRIPT_RUNNER
#include <service_provider/cmd/ScriptRunner.h>
#endif

#if defined(ENABLE_CMD_SERVICE) && defined(ENABLE_STORAGE_SERVICE)

namespace {

  // Where an operator starts, so a word knows where to end.
  bool isOperatorStart(char c) {
    return '|' == c || '&' == c || ';' == c || '<' == c || '>' == c;
  }

  /**
   * A value as a whole decimal number, with an optional sign; an empty value is
   * zero, and anything else is not a number.
   */
  bool wholeNumber(const char *s, int16_t len, int32_t &value) {

    value = 0;
    if (0 == len) {
      return true;
    }

    int16_t i = 0;
    bool negative = false;
    if ('-' == s[0] || '+' == s[0]) {
      negative = ('-' == s[0]);
      i = 1;
    }
    if (i >= len) {
      return false;
    }

    int64_t total = 0;
    for (; i < len; i++) {
      if (!__is_digit(s[i])) {
        return false;
      }
      total = total * 10 + (s[i] - '0');
      if (total > 2147483648LL) {
        return false;
      }
    }

    if (!negative && total > 2147483647LL) {
      return false;
    }
    value = (int32_t)(negative ? -total : total);
    return true;
  }

  /**
   * Works out an integer expression of + - * / %, unary signs, parentheses,
   * numbers and variable names, refusing anything it cannot read.
   */
  struct Arith {

    Arith(const char *s, int16_t len) : m_s(s), m_len(len), m_pos(0), m_depth(0), m_ok(true) {}

    void blanks() {
      while (m_pos < m_len && __is_blank(m_s[m_pos])) m_pos++;
    }

    int32_t expr() {
      int32_t value = term();
      while (m_ok) {
        blanks();
        if (m_pos >= m_len || ('+' != m_s[m_pos] && '-' != m_s[m_pos])) break;
        char op = m_s[m_pos++];
        int32_t rhs = term();
        value = (int32_t)(('+' == op) ? ((uint32_t)value + (uint32_t)rhs) : ((uint32_t)value - (uint32_t)rhs));
      }
      return value;
    }

    int32_t term() {
      int32_t value = factor();
      while (m_ok) {
        blanks();
        if (m_pos >= m_len || ('*' != m_s[m_pos] && '/' != m_s[m_pos] && '%' != m_s[m_pos])) break;
        char op = m_s[m_pos++];
        int32_t rhs = factor();
        if ('*' == op) {
          value = (int32_t)((uint32_t)value * (uint32_t)rhs);
        } else if (0 == rhs || (INT32_MIN == value && -1 == rhs)) {
          m_ok = false;
        } else {
          value = ('/' == op) ? (value / rhs) : (value % rhs);
        }
      }
      return value;
    }

    int32_t factor() {
      blanks();
      if (m_pos >= m_len) {
        m_ok = false;
        return 0;
      }

      char c = m_s[m_pos];

      if ('-' == c || '+' == c) {
        m_pos++;
        int32_t value = factor();
        return ('-' == c) ? (int32_t)(0u - (uint32_t)value) : value;
      }

      if ('(' == c) {
        if (++m_depth > SHELL_ARITH_DEPTH) {
          m_ok = false;
          return 0;
        }
        m_pos++;
        int32_t value = expr();
        blanks();
        if (m_pos >= m_len || ')' != m_s[m_pos]) {
          m_ok = false;
          return 0;
        }
        m_pos++;
        m_depth--;
        return value;
      }

      int16_t start = m_pos;
      int32_t value = 0;

      if (__is_digit(c)) {
        while (m_pos < m_len && __is_digit(m_s[m_pos])) m_pos++;
        if (!wholeNumber(m_s + start, m_pos - start, value)) m_ok = false;
        return value;
      }

      if (__is_alpha(c) || '_' == c) {
        while (m_pos < m_len && (__is_alnum(m_s[m_pos]) || '_' == m_s[m_pos])) m_pos++;
        pdiutil::string name(m_s + start, m_pos - start);
        pdiutil::string text;
        Environment::get(name.c_str(), text);
        if (!wholeNumber(text.c_str(), (int16_t)text.size(), value)) m_ok = false;
        return value;
      }

      m_ok = false;
      return 0;
    }

    const char *m_s;
    int16_t m_len;
    int16_t m_pos;
    uint8_t m_depth;
    bool m_ok;
  };
}

/**
 * Splits a line into tokens, honouring quotes and backslash escapes.
 * False when a quote is never closed.
 */
bool ShellParser::tokenize(const char *line, int16_t len, pdiutil::vector<Token> &tokens) {

  tokens.clear();

  if (nullptr == line || len <= 0) {
    return false;
  }

  int16_t i = 0;

  while (i < len) {

    while (i < len && __is_blank(line[i])) i++;
    if (i >= len) break;

    Token token;
    token.m_start = i;

    // operators first, so a word never starts on one
    if (isOperatorStart(line[i])) {

      char c = line[i];
      bool doubled = ((i + 1) < len) && (line[i + 1] == c);

      if ('|' == c) {
        token.m_type = doubled ? TOKEN_OR_IF : TOKEN_PIPE;
      } else if ('&' == c) {
        // a lone '&' is not an operator here, so it stays part of a word
        if (!doubled) {
          token.m_type = TOKEN_WORD;
        } else {
          token.m_type = TOKEN_AND_IF;
        }
      } else if (';' == c) {
        token.m_type = TOKEN_SEMI;
      } else if ('>' == c) {
        token.m_type = doubled ? TOKEN_DGT : TOKEN_GT;
      } else {
        token.m_type = TOKEN_LT;
      }

      if (TOKEN_WORD != token.m_type) {
        token.m_len = (doubled && (TOKEN_SEMI != token.m_type)) ? 2 : 1;
        tokens.push_back(token);
        i += token.m_len;
        continue;
      }
    }

    // a word runs to the next blank or operator, with quotes and escapes
    // holding those characters inside it
    while (i < len) {

      char c = line[i];

      if (__is_blank(c)) break;

      if ('\\' == c) {
        i += ((i + 1) < len) ? 2 : 1;
        continue;
      }

      if ('\'' == c || '"' == c) {

        char quote = c;
        i++;

        while (i < len && line[i] != quote) {
          if ('"' == quote && '\\' == line[i] && (i + 1) < len) {
            i += 2;
            continue;
          }
          i++;
        }

        if (i >= len) {
          tokens.clear();
          return false;
        }

        i++;
        continue;
      }

      if ('$' == c && (i + 1) < len && '(' == line[i + 1]) {
        int16_t end = substitutionEnd(line, len, i);
        if (end < 0) {
          tokens.clear();
          return false;
        }
        i = end + 1;
        continue;
      }

      if ('$' == c && (i + 1) < len && '{' == line[i + 1]) {
        int16_t end = braceEnd(line, len, i);
        i = (end < 0) ? i + 1 : end + 1;
        continue;
      }

      // a lone '&' belongs to the word; a doubled one ends it
      if ('&' == c) {
        if ((i + 1) < len && '&' == line[i + 1]) break;
        i++;
        continue;
      }

      if (isOperatorStart(c)) break;

      i++;
    }

    token.m_type = TOKEN_WORD;
    token.m_len = i - token.m_start;

    if (token.m_len > 0) {
      tokens.push_back(token);
    }
  }

  return tokens.size() > 0;
}

/**
 * Whether a span holds anything the expander would rewrite.
 */
bool ShellParser::expandable(const char *src, int16_t len) {

  for (int16_t i = 0; i < len; i++) {
    if ('$' == src[i]) {
      return true;
    }
  }

  return false;
}

/**
 * Where the ')' closing the $( at open sits, or -1 when nothing closes it.
 */
int16_t ShellParser::substitutionEnd(const char *src, int16_t len, int16_t open) {

  int16_t depth = 1;
  int16_t i = open + 2;

  while (i < len) {

    char c = src[i];

    if ('\\' == c) {
      i += 2;
      continue;
    }

    if ('\'' == c || '"' == c) {
      char quote = c;
      i++;
      while (i < len && src[i] != quote) {
        if ('"' == quote && '\\' == src[i]) i++;
        i++;
      }
      i++;
      continue;
    }

    if ('(' == c) {
      depth++;
    } else if (')' == c) {
      depth--;
      if (0 == depth) {
        return i;
      }
    }

    i++;
  }

  return -1;
}

/**
 * Where the '}' closing the ${ at open sits, or -1 when nothing closes it.
 */
int16_t ShellParser::braceEnd(const char *src, int16_t len, int16_t open) {

  int16_t depth = 1;
  int16_t i = open + 2;

  while (i < len) {

    char c = src[i];

    if ('\\' == c) {
      i += 2;
      continue;
    }

    if ('$' == c && (i + 1) < len && '(' == src[i + 1]) {
      int16_t end = substitutionEnd(src, len, i);
      if (end < 0) {
        return -1;
      }
      i = end + 1;
      continue;
    }

    if ('$' == c && (i + 1) < len && '{' == src[i + 1]) {
      depth++;
      i += 2;
      continue;
    }

    if ('}' == c) {
      depth--;
      if (0 == depth) {
        return i;
      }
    }

    i++;
  }

  return -1;
}

/**
 * Every $(...) a span holds outside single quotes, each as the run from its
 * '$' to its ')', in the order they appear.
 */
void ShellParser::substitutions(const char *src, int16_t len, pdiutil::vector<Token> &spans) {

  bool insingle = false;
  bool indouble = false;

  for (int16_t i = 0; i < len; i++) {

    char c = src[i];

    if ('\\' == c && !insingle) {
      i++;
      continue;
    }

    if ('\'' == c && !indouble) {
      insingle = !insingle;
      continue;
    }

    if ('"' == c && !insingle) {
      indouble = !indouble;
      continue;
    }

    if ('$' == c && !insingle && (i + 2) < len && '(' == src[i + 1] && '(' == src[i + 2]) {
      i += 2;
      continue;
    }

    if ('$' == c && !insingle && (i + 1) < len && '(' == src[i + 1]) {
      int16_t end = substitutionEnd(src, len, i);
      if (end < 0) {
        return;
      }
      Token span;
      span.m_start = i;
      span.m_len = end - i + 1;
      spans.push_back(span);
      i = end;
    }
  }
}

/**
 * Copies a span with expansions applied. A single quoted run is literal, so
 * nothing inside it expands; a double quoted one still does.
 */
void ShellParser::expand(const char *src, int16_t len, pdi_err_t lastexit, pdiutil::string &out,
                         bool unquote, const Captures *captures) {

  char number[12] = {0};
  bool rendered = false;
  bool insingle = false;
  bool indouble = false;

  for (int16_t i = 0; i < len; i++) {

    char c = src[i];

    if ('\\' == c && !insingle && (i + 1) < len) {
      if (!unquote) out += c;
      out += src[i + 1];
      i++;
      continue;
    }

    if ('\'' == c && !indouble) {
      insingle = !insingle;
      if (!unquote) out += c;
      continue;
    }

    if ('"' == c && !insingle) {
      indouble = !indouble;
      if (!unquote) out += c;
      continue;
    }

    if ('$' == c && !insingle && (i + 2) < len && '(' == src[i + 1] && '(' == src[i + 2]) {

      int16_t end = substitutionEnd(src, len, i);
      if (end > i + 3 && ')' == src[end - 1]) {

        pdiutil::string text;
        expand(src + i + 3, end - 1 - (i + 3), lastexit, text, true, captures);

        Arith arith(text.c_str(), (int16_t)text.size());
        int32_t value = arith.expr();
        arith.blanks();

        if (arith.m_ok && arith.m_pos == arith.m_len) {
          char digits[12] = {0};
          Int32ToString(value, digits, sizeof(digits) - 1);
          out += digits;
        } else {
          out.append(src + i, (pdiutil::string::size_type)(end - i + 1));
        }
        i = end;
        continue;
      }
    }

    if ('$' == c && !insingle && (i + 1) < len && '(' == src[i + 1]) {

      int16_t end = substitutionEnd(src, len, i);
      if (end < 0) {
        out += c;
        continue;
      }

      const Capture *found = nullptr;
      for (uint16_t k = 0; nullptr != captures && k < captures->size(); k++) {
        if ((*captures)[k].m_at == src + i) {
          found = &(*captures)[k];
          break;
        }
      }

      if (nullptr != found) {
        out += found->m_value;
      } else {
        out.append(src + i, (pdiutil::string::size_type)(end - i + 1));
      }
      i = end;
      continue;
    }

    if ('$' == c && !insingle && (i + 1) < len && '?' == src[i + 1]) {

      if (!rendered) {
        Int32ToString((int32_t)lastexit, number, sizeof(number) - 1);
        rendered = true;
      }

      out += number;
      i++;
      continue;
    }

    if ('$' == c && !insingle && (i + 1) < len &&
        (('1' <= src[i + 1] && '9' >= src[i + 1]) || '#' == src[i + 1] || '@' == src[i + 1])) {

      const pdiutil::vector<pdiutil::string> *args = nullptr;
#ifdef ENABLE_SCRIPT_RUNNER
      args = ScriptRunner::arguments();
#endif
      uint32_t count = (nullptr != args) ? (uint32_t)args->size() : 0;
      char which = src[i + 1];

      if ('#' == which) {
        char digits[12] = {0};
        Int32ToString((int32_t)count, digits, sizeof(digits) - 1);
        out += digits;
      } else if ('@' == which) {
        for (uint32_t k = 0; k < count; k++) {
          if (k > 0) out += ' ';
          out += (*args)[k];
        }
      } else if ((uint32_t)(which - '1') < count) {
        out += (*args)[which - '1'];
      }

      i++;
      continue;
    }

    if ('$' == c && !insingle && (i + 1) < len && '{' == src[i + 1]) {

      int16_t end = braceEnd(src, len, i);
      if (end < 0) {
        out += c;
        continue;
      }

      const char *inner = src + i + 2;
      int16_t innerlen = end - (i + 2);
      bool length = (innerlen > 0 && '#' == inner[0]);
      int16_t namestart = length ? 1 : 0;
      int16_t nameend = namestart;
      while (nameend < innerlen && (__is_alnum(inner[nameend]) || '_' == inner[nameend])) nameend++;

      bool named = (nameend > namestart) && !__is_digit(inner[namestart]);
      bool fallback = !length && (nameend + 1) < innerlen && ':' == inner[nameend] && '-' == inner[nameend + 1];

      if (!named || (length && nameend != innerlen) || (!length && !fallback && nameend != innerlen)) {
        out.append(src + i, (pdiutil::string::size_type)(end - i + 1));
        i = end;
        continue;
      }

      pdiutil::string name(inner + namestart, nameend - namestart);
      pdiutil::string value;
      Environment::get(name.c_str(), value);

      if (length) {
        char digits[12] = {0};
        Int32ToString((int32_t)value.size(), digits, sizeof(digits) - 1);
        out += digits;
      } else if (fallback && value.empty()) {
        expand(inner + nameend + 2, innerlen - (nameend + 2), lastexit, out, unquote, captures);
      } else {
        out += value;
      }

      i = end;
      continue;
    }

    // a name after $ is a variable, and one the environment does not carry
    // expands to nothing the way a shell leaves an unset name empty
    if ('$' == c && !insingle && (i + 1) < len &&
        (__is_alpha(src[i + 1]) || '_' == src[i + 1])) {

      int16_t nameend = i + 1;
      while (nameend < len && (__is_alnum(src[nameend]) || '_' == src[nameend])) nameend++;

      pdiutil::string name(src + i + 1, nameend - (i + 1));
      pdiutil::string value;
      Environment::get(name.c_str(), value);

      out += value;
      i = nameend - 1;
      continue;
    }

    out += c;
  }
}

/**
 * Text of one command without its redirections, expansions applied.
 */
bool ShellParser::Line::commandText(const char *line, int16_t index, pdiutil::string &out,
                                    pdi_err_t lastexit, const Captures *captures) const {

  out.clear();

  if (nullptr == line || index < 0 || index >= (int16_t)m_commands.size()) {
    return false;
  }

  const Command &command = m_commands[index];
  if (command.m_word_count <= 0) {
    return false;
  }

  int16_t first = m_words[command.m_word_start].m_start;
  const Token &lastword = m_words[command.m_word_start + command.m_word_count - 1];
  int16_t last = lastword.m_start + lastword.m_len;

  // nothing of the command's own redirections falls between its first and last
  // word in the common case, so the line can be handed over as it was typed
  bool contiguous = true;
  for (int16_t r = 0; r < command.m_redirect_count; r++) {
    const Redirect &redirect = m_redirects[command.m_redirect_start + r];
    if (redirect.m_start > first && redirect.m_start < last) {
      contiguous = false;
      break;
    }
  }

  if (contiguous && !expandable(line + first, last - first)) {
    out.append(line + first, (pdiutil::string::size_type)(last - first));
    return true;
  }

  if (contiguous) {
    expand(line + first, last - first, lastexit, out, false, captures);
    return true;
  }

  for (int16_t w = 0; w < command.m_word_count; w++) {
    const Token &word = m_words[command.m_word_start + w];
    if (w > 0) out += ' ';
    expand(line + word.m_start, word.m_len, lastexit, out, false, captures);
  }

  return true;
}

/**
 * Redirection of the given kind on one command, if it has one.
 */
const ShellParser::Redirect *ShellParser::Line::findRedirect(int16_t command, redirect_op_t op) const {

  if (command < 0 || command >= (int16_t)m_commands.size()) {
    return nullptr;
  }

  const Command &c = m_commands[command];

  for (int16_t r = 0; r < c.m_redirect_count; r++) {
    const Redirect &redirect = m_redirects[c.m_redirect_start + r];
    if (redirect.m_op == op) {
      return &redirect;
    }
  }

  return nullptr;
}

/**
 * Reads a whole line into pipelines, commands and redirections.
 */
ShellParser::Line ShellParser::parse(const char *line, int16_t len) {

  Line out;

  pdiutil::vector<Token> tokens;
  if (!tokenize(line, len, tokens)) {
    out.m_malformed = true;
    return out;
  }

  uint16_t at = 0;
  join_t join = JOIN_FIRST;

  while (at < tokens.size()) {

    Pipeline pipeline;
    pipeline.m_join = join;
    pipeline.m_command_start = (int16_t)out.m_commands.size();
    pipeline.m_command_count = 0;

    if (TOKEN_WORD == tokens[at].m_type && 1 == tokens[at].m_len && '!' == line[tokens[at].m_start]) {
      pipeline.m_negate = true;
      at++;
    }

    // a pipeline is one or more commands joined by '|'
    while (true) {

      Command command;
      command.m_word_start = (int16_t)out.m_words.size();
      command.m_redirect_start = (int16_t)out.m_redirects.size();
      command.m_word_count = 0;
      command.m_redirect_count = 0;

      while (at < tokens.size()) {

        const Token &token = tokens[at];

        if (TOKEN_WORD == token.m_type) {
          out.m_words.push_back(token);
          command.m_word_count++;
          at++;
          continue;
        }

        if (TOKEN_LT == token.m_type || TOKEN_GT == token.m_type || TOKEN_DGT == token.m_type) {

          // a redirection needs a path, and only one of each kind per command
          if ((at + 1) >= tokens.size() || TOKEN_WORD != tokens[at + 1].m_type) {
            out.m_malformed = true;
            return out;
          }

          Redirect redirect;
          redirect.m_op = (TOKEN_LT == token.m_type) ? REDIRECT_IN
                        : (TOKEN_DGT == token.m_type) ? REDIRECT_APPEND : REDIRECT_OUT;
          redirect.m_start = tokens[at + 1].m_start;
          redirect.m_len = tokens[at + 1].m_len;

          for (int16_t r = 0; r < command.m_redirect_count; r++) {
            const Redirect &seen = out.m_redirects[command.m_redirect_start + r];
            bool bothOut = (REDIRECT_IN != seen.m_op) && (REDIRECT_IN != redirect.m_op);
            if (seen.m_op == redirect.m_op || bothOut) {
              out.m_malformed = true;
              return out;
            }
          }

          out.m_redirects.push_back(redirect);
          command.m_redirect_count++;
          at += 2;
          continue;
        }

        break;
      }

      if (0 == command.m_word_count) {
        out.m_malformed = true;
        return out;
      }

      out.m_commands.push_back(command);
      pipeline.m_command_count++;

      if (at < tokens.size() && TOKEN_PIPE == tokens[at].m_type) {
        at++;
        continue;
      }

      break;
    }

    out.m_pipelines.push_back(pipeline);

    if (at >= tokens.size()) {
      break;
    }

    if (TOKEN_SEMI == tokens[at].m_type) {
      join = JOIN_ALWAYS;
    } else if (TOKEN_AND_IF == tokens[at].m_type) {
      join = JOIN_ON_SUCCESS;
    } else if (TOKEN_OR_IF == tokens[at].m_type) {
      join = JOIN_ON_FAILURE;
    } else {
      out.m_malformed = true;
      return out;
    }

    at++;

    // a trailing ';' ends the line, anything else is left dangling
    if (at >= tokens.size()) {
      if (JOIN_ALWAYS != join) {
        out.m_malformed = true;
      }
      return out;
    }
  }

  if (0 == out.m_pipelines.size()) {
    out.m_malformed = true;
  }

  return out;
}

/**
 * Whether a pipeline runs, given what the one before it answered.
 */
bool ShellParser::shouldRun(join_t join, pdi_err_t previous) {

  if (JOIN_ON_SUCCESS == join) {
    return PDI_OK == previous;
  }

  if (JOIN_ON_FAILURE == join) {
    return PDI_OK != previous;
  }

  return true;
}

#endif
