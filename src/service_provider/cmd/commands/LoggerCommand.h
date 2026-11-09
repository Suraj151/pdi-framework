/******************************* Logger Command *******************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Writes a line into the system log, tagged with the
user who wrote it: the words given, or each line of the input when none are.
-p names the priority, alone or as facility.priority, and decides the log file
it lands in; notice is the default. -- ends the options.

Author          : Suraj I.
created Date    : 7th Nov 2026
******************************************************************************/
#ifndef _LOGGER_COMMAND_H_
#define _LOGGER_COMMAND_H_

#include "CommandCommon.h"

#ifdef ENABLE_SYSLOG_SERVICE

struct LoggerCommand : public CommandBase {

	LoggerCommand(){
		Clear();
		SetCommand(CMD_NAME_LOGGER);
		setAcceptArgsOptions(true);
	}

	static void RegisterCommand(){
		CommandBase::RegisterCommand(CMD_NAME_LOGGER, [](void *arg)->void *{
			return pdiutil::safe_new<LoggerCommand>();
		});
	}

	const char* getUsage() const override {
		return RODT_ATTR("logger [-p <priority>] [--] [message]  write a line to the system log, or each input line");
	}

#ifdef ENABLE_AUTH_SERVICE
	bool needauth() override { return true; }
#endif

	pdi_err_t execute(cmd_term_inseq_t terminputaction){

#ifdef ENABLE_AUTH_SERVICE
		if( needauth() && !__auth_service.getAuthorized() ){
			return CMD_ERROR_PERM;
		}
#endif

		if( nullptr == m_terminal ){
			return CMD_ERROR_NOTTY;
		}

		const CommandOption *line = &m_options[0];
		const char *text = optionGiven(line) ? line->optionval : nullptr;
		int16_t size = 0;
		while( nullptr != text && size < line->optionvalsize && '\0' != text[size] ) size++;

		logger_type_t priority = SUCCESS_LOG;
		bool given = (nullptr != text);

		while( given && size > 0 && '-' == text[0] ){
			int16_t word = 0;
			while( word < size && ' ' != text[word] ) word++;
			if( 2 == word && '-' == text[1] ){
				skipWord(text, size, word);
				break;
			}
			if( word < 2 || 'p' != text[1] ){
				return refuseArg(RODT_ATTR(CMD_MSG_INVALID_OPTION), text, word, CMD_ERROR_OPT);
			}
			if( 2 == word ){
				skipWord(text, size, word);
				word = 0;
				while( word < size && ' ' != text[word] ) word++;
				if( 0 == word ){
					m_terminal->putln();
					m_terminal->write_ro(RODT_ATTR("logger: -p needs a priority"));
					return CMD_ERROR_ARGS_MISSING;
				}
				if( !takePriority(text, word, priority) ){
					return CMD_ERROR_INVAL;
				}
			}else if( !takePriority(text + 2, word - 2, priority) ){
				return CMD_ERROR_INVAL;
			}
			skipWord(text, size, word);
		}

		pdiutil::string tag;
#ifdef ENABLE_AUTH_SERVICE
		const char *user = __auth_service.getUsername();
		if( nullptr != user ) tag = user;
#endif
		if( tag.empty() ){
			tag = CHARPTR_WRAP(CMD_NAME_LOGGER);
		}

		if( given && size > 0 ){
			pdiutil::string message(text, (size_t)size);
			write(priority, tag, message);
			return PDI_OK;
		}

#ifdef ENABLE_STORAGE_SERVICE
		if( isInputRedirected() ){
			int rc = readCommandLines(pdiutil::string(), m_terminal, [&](const pdiutil::string &entry)->bool{
				write(priority, tag, entry);
				return true;
			});
			return (rc < 0) ? (pdi_err_t)CMD_ERROR_FAILED : stoppedResult(PDI_OK);
		}
#endif

		if( given ){
			write(priority, tag, pdiutil::string());
			return PDI_OK;
		}
		return CMD_ERROR_ARGS_MISSING;
	}

private:

	/**
	 * Steps past one word and the spaces after it.
	 */
	void skipWord(const char *&text, int16_t &size, int16_t word){
		text += word;
		size -= word;
		while( size > 0 && ' ' == text[0] ){
			text++;
			size--;
		}
	}

	/**
	 * The log a priority name sends a line to, the name taken alone or after
	 * a facility and a dot; an unknown one is refused, naming the word.
	 */
	bool takePriority(const char *word, int16_t size, logger_type_t &out){
		const char *name = word;
		int16_t namesize = size;
		for( int16_t i = size - 1; i >= 0; i-- ){
			if( '.' == word[i] ){
				name = word + i + 1;
				namesize = size - i - 1;
				break;
			}
		}
		const char *names[] = {
			RODT_ATTR(SYSLOG_PRIORITY_EMERG), RODT_ATTR(SYSLOG_PRIORITY_ALERT),
			RODT_ATTR(SYSLOG_PRIORITY_CRIT), RODT_ATTR(SYSLOG_PRIORITY_ERR),
			RODT_ATTR(SYSLOG_PRIORITY_WARNING), RODT_ATTR(SYSLOG_PRIORITY_NOTICE),
			RODT_ATTR(SYSLOG_PRIORITY_INFO), RODT_ATTR(SYSLOG_PRIORITY_DEBUG)
		};
		for( uint8_t i = 0; i < sizeof(names) / sizeof(names[0]); i++ ){
			if( namesize > 0 && (size_t)namesize == strlen_ro(names[i]) && 0 == strncmp_ro(name, names[i], namesize) ){
				out = (i < 4) ? ERROR_LOG : (4 == i) ? WARNING_LOG : (5 == i) ? SUCCESS_LOG : INFO_LOG;
				return true;
			}
		}
		refuseArg(RODT_ATTR("logger: unknown priority : "), word, size, CMD_ERROR_INVAL);
		return false;
	}

	/**
	 * One line into the log, cut to what a log line holds.
	 */
	void write(logger_type_t priority, const pdiutil::string &tag, const pdiutil::string &message){
		size_t room = SYSLOG_LINE_MAX - 4;
		room = (tag.size() < room) ? room - tag.size() : 0;
		pdiutil::string body = (message.size() > room) ? message.substr(0, room) : message;
		__log_manager.syslog(priority, RODT_ATTR("%s: %s\n"), tag.c_str(), body.c_str());
	}
};

#endif

#endif
