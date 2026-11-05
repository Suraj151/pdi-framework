/******************************** Sort Command ********************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st Nov 2026
******************************************************************************/

#ifndef _SORT_COMMAND_H_
#define _SORT_COMMAND_H_

#include "CommandCommon.h"

#ifdef ENABLE_STORAGE_SERVICE

struct SortCommand : public CommandBase {

	SortCommand(){
		Clear();
		SetCommand(CMD_NAME_SORT);
		setArgs(RODT_ATTR("rnu"), 1);
		setAcceptArgsOptions(true);
		setCmdOptionSeparator(CMD_OPTION_SEPERATOR_SPACE);
	}

	static void RegisterCommand(){
		CommandBase::RegisterCommand(CMD_NAME_SORT, [](void *arg)->void *{
			return pdiutil::safe_new<SortCommand>();
		});
	}

	const char* getUsage() const override {
		return RODT_ATTR("sort [-r] [-n] [-u] [<file>]  sort lines (-r reverse, -n numeric, -u unique)");
	}

#ifdef ENABLE_AUTH_SERVICE
	bool needauth() override { return true; }
#endif

	pdi_err_t execute(cmd_term_inseq_t terminputaction){

#ifdef ENABLE_AUTH_SERVICE
		if( needauth() && !__auth_service.getAuthorized()){
			return CMD_ERROR_PERM;
		}
#endif

		if( nullptr == m_terminal ){
			return PDI_OK;
		}

		m_reverse = false;
		m_numeric = false;
		m_unique = false;
		pdiutil::string typed;
		bool given = false;

		for( uint8_t i = 0; i < CMD_OPTION_MAX; i++ ){

			CommandOption *opt = &m_options[i];
			if( !optionGiven(opt) ){
				break;
			}

			if( opt->optionvalsize > 1 && '-' == opt->optionval[0] ){
				for( int16_t c = 1; c < opt->optionvalsize; c++ ){
					char f = opt->optionval[c];
					m_reverse = m_reverse || ( 'r' == f );
					m_numeric = m_numeric || ( 'n' == f );
					m_unique = m_unique || ( 'u' == f );
				}
			}else{
				typed.assign(opt->optionval, (pdiutil::string::size_type)opt->optionvalsize);
				given = true;
			}
		}

		pdiutil::string path = given ? resolveArgPathStr(typed.c_str(), (int16_t)typed.size()) : pdiutil::string();
		if( !given && !isInputRedirected() ){
			return CMD_ERROR_ARGS_MISSING;
		}
		if( given && path.empty() ){
			m_terminal->putln();
			m_terminal->write_ro(RODT_ATTR(CMD_MSG_NOT_FOUND));
			writeName(nullptr);
			return CMD_ERROR_FAILED;
		}

		bool refused = false;
		pdiutil::string text;
		pdiutil::vector<uint32_t> starts;
		m_text = &text;

		int rc = readCommandLines(path, m_terminal, [&](const pdiutil::string &line)->bool{
			uint32_t at = (uint32_t)text.size();
			uint32_t count = (uint32_t)starts.size();
			text += line;
			text += '\0';
			starts.push_back(at);
			if( text.size() != at + line.size() + 1 || starts.size() != count + 1 ){
				refused = true;
				return false;
			}
			return true;
		});

		m_terminal->putln();

		pdi_err_t result = PDI_OK;
		if( rc < 0 ){
			writeFailure(RODT_ATTR(CMD_MSG_FAILED), typed.c_str(), (int32_t)rc);
			result = CMD_ERROR_FAILED;
		}else if( refused ){
			m_terminal->write_ro(RODT_ATTR(CMD_MSG_OUT_OF_MEMORY));
			result = CMD_ERROR_FAILED;
		}else if( !commandStopped(m_terminal) ){
			_HeapSort(starts.data(), (uint32_t)starts.size(), [this](uint32_t a, uint32_t b){ return before(a, b); }, [](){ __i_dvc_ctrl.yield(); });
			for( uint32_t i = 0; i < starts.size() && !commandStopped(m_terminal); i++ ){
				if( m_unique && i > 0 && 0 == compareKeys(starts[i - 1], starts[i]) ){
					continue;
				}
				m_terminal->writeln(text.c_str() + starts[i]);
				if( 0 == (i & 31) ){
					__i_dvc_ctrl.yield();
				}
			}
		}

		m_text = nullptr;
		return stoppedResult(result);
	}

private:

	/**
	 * Orders two lines by the sort key alone: their numbers for -n, their bytes
	 * otherwise, reversed for -r.
	 */
	int8_t compareKeys(uint32_t a, uint32_t b){
		const char *la = m_text->c_str() + a;
		const char *lb = m_text->c_str() + b;
		int8_t order = 0;
		if( m_numeric ){
			double na = StringToDouble(la);
			double nb = StringToDouble(lb);
			order = ( na < nb ) ? -1 : ( na > nb ) ? 1 : 0;
		}else{
			int c = strcmp(la, lb);
			order = ( c < 0 ) ? -1 : ( c > 0 ) ? 1 : 0;
		}
		return m_reverse ? (int8_t)-order : order;
	}

	/**
	 * Orders two lines completely: the key, then the whole line unless -u asked
	 * for equal keys to stay in input order, then the input order.
	 */
	bool before(uint32_t a, uint32_t b){
		int8_t order = compareKeys(a, b);
		if( 0 == order && m_numeric && !m_unique ){
			int c = strcmp(m_text->c_str() + a, m_text->c_str() + b);
			order = ( c < 0 ) ? -1 : ( c > 0 ) ? 1 : 0;
			if( m_reverse ) order = (int8_t)-order;
		}
		if( 0 != order ){
			return order < 0;
		}
		return a < b;
	}

	pdiutil::string *m_text = nullptr;
	bool m_reverse = false;
	bool m_numeric = false;
	bool m_unique = false;
};

#endif

#endif
