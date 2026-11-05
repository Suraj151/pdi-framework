/******************************** Find Command ********************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st Nov 2026
******************************************************************************/

#ifndef _FIND_COMMAND_H_
#define _FIND_COMMAND_H_

#include "CommandCommon.h"

#ifdef ENABLE_STORAGE_SERVICE

struct FindCommand : public CommandBase {

	FindCommand(){
		Clear();
		SetCommand(CMD_NAME_FIND);
		setAcceptArgsOptions(true);
		setCmdOptionSeparator(CMD_OPTION_SEPERATOR_SPACE);
	}

	static void RegisterCommand(){
		CommandBase::RegisterCommand(CMD_NAME_FIND, [](void *arg)->void *{
			return pdiutil::safe_new<FindCommand>();
		});
	}

	const char* getUsage() const override {
		return RODT_ATTR("find [<path>] [-name <glob>] [-type f|d]  list a tree, filtered by name and type");
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

		pdiutil::string typed;
		bool given = false;
		pdiutil::string pattern;
		bool named = false;
		char wanted = 0;

		pdiutil::string pname = CHARPTR_WRAP(CMD_FIND_PRED_NAME);
		pdiutil::string ptype = CHARPTR_WRAP(CMD_FIND_PRED_TYPE);

		for( uint8_t i = 0; i < CMD_OPTION_MAX; i++ ){

			CommandOption *opt = &m_options[i];
			if( !optionGiven(opt) ){
				break;
			}
			pdiutil::string token(opt->optionval, (pdiutil::string::size_type)opt->optionvalsize);

			if( token == pname || token == ptype ){
				CommandOption *arg = ( i + 1 < CMD_OPTION_MAX ) ? &m_options[i + 1] : nullptr;
				if( !optionGiven(arg) ){
					m_terminal->putln();
					m_terminal->write_ro(RODT_ATTR("missing argument to "));
					m_terminal->write(token.c_str());
					return CMD_ERROR_ARGS_MISSING;
				}
				if( token == pname ){
					pattern.assign(arg->optionval, (pdiutil::string::size_type)arg->optionvalsize);
					named = true;
				}else if( 1 == arg->optionvalsize && ('f' == arg->optionval[0] || 'd' == arg->optionval[0]) ){
					wanted = arg->optionval[0];
				}else{
					m_terminal->putln();
					m_terminal->write_ro(RODT_ATTR("unknown type : "));
					m_terminal->write(arg->optionval, (uint32_t)arg->optionvalsize);
					return CMD_ERROR_INVAL;
				}
				i++;
			}else if( !token.empty() && '-' == token[0] ){
				m_terminal->putln();
				m_terminal->write_ro(RODT_ATTR("unknown predicate : "));
				m_terminal->write(token.c_str());
				return CMD_ERROR_OPT;
			}else if( 0 == i ){
				typed = token;
				given = true;
			}else{
				m_terminal->putln();
				m_terminal->write_ro(RODT_ATTR("paths must precede the expression : "));
				m_terminal->write(token.c_str());
				return CMD_ERROR_INVAL;
			}
		}

		pdiutil::string start = given ? resolveArgPathStr(typed.c_str(), (int16_t)typed.size()) : SessionManager::getPWD();
		if( !given ){
			typed = ".";
		}

		m_terminal->putln();

		if( !__i_fs.isDirectory(start.c_str()) && !__i_fs.isFileExist(start.c_str()) ){
			m_terminal->write_ro(RODT_ATTR(CMD_MSG_NOT_FOUND));
			writeName(typed.c_str());
			return CMD_ERROR_FAILED;
		}

		pdi_err_t result = PDI_OK;
		bool broken = false;
		pdiutil::vector<pdiutil::string> paths;
		pdiutil::vector<pdiutil::string> shown;
		pdiutil::vector<uint8_t> dirs;

		paths.push_back(start);
		shown.push_back(typed);
		dirs.push_back(__i_fs.isDirectory(start.c_str()) ? 1 : 0);

		while( !paths.empty() && !commandStopped(m_terminal) ){

			pdiutil::string path = paths.back();
			pdiutil::string name = shown.back();
			bool isdir = ( 0 != dirs.back() );
			paths.pop_back();
			shown.pop_back();
			dirs.pop_back();

			if( matches(name, isdir, named, pattern, wanted) ){
				if( broken ){
					m_terminal->putln();
					broken = false;
				}
				m_terminal->writeln(name.c_str());
			}

			if( !isdir ){
				continue;
			}

			pdiutil::vector<file_info_t> items;
			int rc = __i_fs.getDirFileList(path.c_str(), items);
			if( rc < 0 ){
				if( broken ){
					m_terminal->putln();
				}
				writeFailure(RODT_ATTR(CMD_MSG_FAILED), name.c_str(), (int32_t)rc);
				broken = true;
				result = CMD_ERROR_FAILED;
			}

			for( int32_t j = (int32_t)items.size() - 1; j >= 0; j-- ){
				const char *entry = items[j].m_name;
				if( nullptr != entry && 0 != strcmp(entry, ".") && 0 != strcmp(entry, "..") ){
					pdiutil::string childpath = path;
					__i_fs.appendFileSeparator(childpath);
					childpath += entry;
					pdiutil::string childshown = name;
					__i_fs.appendFileSeparator(childshown);
					childshown += entry;
					paths.push_back(childpath);
					shown.push_back(childshown);
					dirs.push_back(FILE_TYPE_DIR == items[j].m_type ? 1 : 0);
				}
			}

			for( uint16_t j = 0; j < items.size(); j++ ){
				pdiutil::safe_delete_array(items[j].m_name);
			}

			__i_dvc_ctrl.yield();
		}

		return stoppedResult(result);
	}

private:

	/**
	 * Whether an entry passes the expression: its last name part against the
	 * glob, and its kind against the type asked for.
	 */
	bool matches(const pdiutil::string &shown, bool isdir, bool named, const pdiutil::string &pattern, char wanted){
		if( 'f' == wanted && isdir ) return false;
		if( 'd' == wanted && !isdir ) return false;
		if( !named ) return true;

		pdiutil::string::size_type end = shown.size();
		while( end > 1 && FILE_SEPARATOR[0] == shown[end - 1] ) end--;
		pdiutil::string::size_type begin = end;
		while( begin > 0 && FILE_SEPARATOR[0] != shown[begin - 1] ) begin--;
		if( begin == end ){
			begin = 0;
		}

		return __glob_match(pattern.c_str(), (uint32_t)pattern.size(), shown.c_str() + begin, (uint32_t)(end - begin));
	}
};

#endif

#endif
