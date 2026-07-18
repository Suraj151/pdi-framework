/******************************** Grep Command ********************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 28th May 2026
******************************************************************************/

#ifndef _GREP_FS_COMMAND_H_
#define _GREP_FS_COMMAND_H_

#include "CommandCommon.h"
#include <utility/RegexMatch.h>

#ifdef ENABLE_STORAGE_SERVICE

struct GrepFSCommand : public CommandBase {

	GrepFSCommand(){
		Clear();
		SetCommand(CMD_NAME_GREP);
		setAcceptArgsOptions(true);
		setCmdOptionSeparator(CMD_OPTION_SEPERATOR_SPACE);
	}

	static void RegisterCommand(){
		CommandBase::RegisterCommand(CMD_NAME_GREP, [](void *arg)->void *{
			return new GrepFSCommand();
		});
	}

#ifdef ENABLE_AUTH_SERVICE
	bool needauth() override { return true; }
#endif

	void grepFile(const char *filepath, int filepath_len, const char *pattern){

		int64_t fs = __i_fs.getFileSize(filepath);
		if(fs <= 0) return;

		uint64_t offset = 0;
		int32_t lineNo = 0;
		pdiutil::string linedata;
		while(offset < (uint64_t)fs){
			linedata.clear();
			int bytes = __i_fs.readFile(filepath, 200, [&](char *data, uint32_t size) -> bool {
				linedata += pdiutil::string(data, size);
				return true;
			}, offset, "\n");
			if(bytes < 0) break;

			if(!linedata.empty() && linedata.back() == '\r'){
				linedata.pop_back();
			}

			int matchPos = regex_match(pattern, linedata.c_str(), (int)linedata.size());
			if(matchPos >= 0){
				m_terminal->write(filepath, filepath_len);
				m_terminal->write(':');
				m_terminal->write((int32_t)(lineNo + 1));
				m_terminal->write(':');
				m_terminal->write((int32_t)matchPos);
				m_terminal->write(':');
				m_terminal->write(linedata.c_str());
				m_terminal->putln();
			}

			offset += (uint64_t)bytes + 1;
			lineNo++;
			__i_dvc_ctrl.yield();
		}
	}

	void grepDir(const char *dirpath, int dirpath_len, const char *pattern){

		pdiutil::vector<file_info_t> items;
		int rc = __i_fs.getDirFileList(dirpath, items);

		if(rc >= 0){
			for(file_info_t &item : items){
				if(nullptr == item.name) continue;

				int namelen = (int)strlen(item.name);
				char *childpath = new char[dirpath_len + namelen + 2]();

				if(nullptr != childpath){
					memcpy(childpath, dirpath, dirpath_len);
					__i_fs.appendFileSeparator(childpath);
					strncat(childpath, item.name, namelen);
					int childpath_len = (int)strlen(childpath);

					if(item.type == FILE_TYPE_DIR){
						grepDir(childpath, childpath_len, pattern);
					}else{
						grepFile(childpath, childpath_len, pattern);
					}

					delete[] childpath;
				}
			}
		}

		for(file_info_t &item : items){
			if(item.name) delete[] item.name;
		}
		items.clear();
	}

	cmd_result_t execute(cmd_term_inseq_t terminputaction){

#ifdef ENABLE_AUTH_SERVICE
		if( needauth() && !__auth_service.getAuthorized()){
			return CMD_RESULT_NEED_AUTH;
		}
#endif

		cmd_result_t result = CMD_RESULT_OK;

		if(nullptr != m_terminal){
			CommandOption *patternoptn = &m_options[0];
			CommandOption *fileoptn = &m_options[1];

			bool isPatternProvided = (nullptr != patternoptn && nullptr != patternoptn->optionval && patternoptn->optionvalsize > 0);
			bool isFileProvided = (nullptr != fileoptn && nullptr != fileoptn->optionval && fileoptn->optionvalsize > 0);

			if(isPatternProvided && isFileProvided){

				char *pattern = new char[patternoptn->optionvalsize + 1]();
				char *path = new char[fileoptn->optionvalsize + SessionManager::getPWD().size() + 2]();

				if(nullptr != pattern && nullptr != path){
					memcpy(pattern, patternoptn->optionval, patternoptn->optionvalsize);

					memcpy(path, SessionManager::getPWD().c_str(), SessionManager::getPWD().size());
					__i_fs.appendFileSeparator(path);
					strncat(path, fileoptn->optionval, fileoptn->optionvalsize);
					int path_len = (int)strlen(path);

					m_terminal->putln();

					if(__i_fs.isDirectory(path)){
						grepDir(path, path_len, pattern);
					}else if(__i_fs.isFileExist(path)){
						grepFile(path, path_len, pattern);
					}else{
						result = CMD_RESULT_FAILED;
						m_terminal->write_ro(RODT_ATTR("Not found : "));
						m_terminal->write(path);
					}
				}

				if(nullptr != path) delete[] path;
				if(nullptr != pattern) delete[] pattern;
			}else{
				result = CMD_RESULT_ARGS_MISSING;
			}
		}

		return result;
	}
};

#endif

#endif
