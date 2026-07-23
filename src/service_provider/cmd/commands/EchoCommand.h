/********************************* Echo Command *******************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 23rd July 2026
******************************************************************************/

#ifndef _ECHO_COMMAND_H_
#define _ECHO_COMMAND_H_

#include "CommandCommon.h"

/**
 * echo command
 *
 * Prints its argument, or with a '>' redirection writes it to a file:
 *   echo hello
 *   echo 1 > /sys/class/gpio/5/value
 */
struct EchoCommand : public CommandBase {

	/* Constructor */
	EchoCommand(){
		Clear();
		SetCommand(CMD_NAME_ECHO);
		setAcceptArgsOptions(true);
	}

	/**
     * @brief Register the command.
     */
    static void RegisterCommand(){
		CommandBase::RegisterCommand(CMD_NAME_ECHO, [](void *arg)->void *{
			return new EchoCommand();
		});
	}

	const char* getUsage() const override {
		return RODT_ATTR("echo <text> [> <file>]  print text or write it to a file");
	}

#ifdef ENABLE_AUTH_SERVICE
	/* override the necesity of required permission */
	bool needauth() override { return true; }
#endif

	/* execute command with provided options */
	cmd_result_t execute(cmd_term_inseq_t terminputaction){

#ifdef ENABLE_AUTH_SERVICE
		// return in case authentication needed and not authorized yet
		if( needauth() && !__auth_service.getAuthorized()){
			return CMD_RESULT_NEED_AUTH;
		}
#endif

		if( nullptr == m_terminal ){
			return CMD_RESULT_TERMINAL_ERR;
		}

		CommandOption *cmdoptn = &m_options[0];
		// absent positional arg — parser leaves size -1; print a blank line
		if( nullptr == cmdoptn->optionval || 0 >= cmdoptn->optionvalsize ){
			m_terminal->writeln();
			return CMD_RESULT_OK;
		}

		const char* blob = cmdoptn->optionval;
		// the arg buffer is null-terminated and trailing spaces are trimmed,
		// so clamp the measured span to the real string length
		int16_t bloblen = 0;
		while( bloblen < cmdoptn->optionvalsize && blob[bloblen] != '\0' ) bloblen++;

		// locate a redirection operator '>'
		int16_t gt = -1;
		for( int16_t i = 0; i < bloblen; i++ ){
			if( blob[i] == '>' ){ gt = i; break; }
		}

#ifdef ENABLE_STORAGE_SERVICE
		if( gt >= 0 ){
			// left of '>' is the text, right is the target path
			int16_t textend = gt;
			while( textend > 0 && blob[textend-1] == ' ' ) textend--;
			int16_t pathstart = gt + 1;
			while( pathstart < bloblen && blob[pathstart] == ' ' ) pathstart++;

			pdiutil::string filepath = resolveArgPathStr(blob + pathstart, bloblen - pathstart);
			if( filepath.empty() ){
				return CMD_RESULT_ARGS_ERROR;
			}

			int iStatus = __i_fs.writeFile(filepath.c_str(), blob, textend, false);
			if( iStatus < 0 ){
				m_terminal->putln();
				m_terminal->write_ro(RODT_ATTR("Failed : "));
				m_terminal->write(filepath.c_str());
				m_terminal->write_ro(RODT_ATTR(" : "));
				m_terminal->write((int32_t)iStatus);
				return CMD_RESULT_FAILED;
			}
			return CMD_RESULT_OK;
		}
#endif

		// no redirection — print the text
		m_terminal->putln();
		m_terminal->write(blob, bloblen);
		m_terminal->writeln();
		return CMD_RESULT_OK;
	}
};

#endif
