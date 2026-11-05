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
#include <interface/pdi/impl/modules/storage/FileLines.h>

#ifdef ENABLE_STORAGE_SERVICE

struct GrepFSCommand : public CommandBase {

	GrepFSCommand(){
		Clear();
		SetCommand(CMD_NAME_GREP);
		setArgs(RODT_ATTR(""), 2);
		setAcceptArgsOptions(true);
		setCmdOptionSeparator(CMD_OPTION_SEPERATOR_SPACE);
	}

	static void RegisterCommand(){
		CommandBase::RegisterCommand(CMD_NAME_GREP, [](void *arg)->void *{
			return pdiutil::safe_new<GrepFSCommand>();
		});
	}

	const char* getUsage() const override {
		return RODT_ATTR("grep <pattern> [<file_or_dir>]  regex search of a tree or the piped input; prints path:line:col:content");
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
		while(offset < (uint64_t)fs && !commandStopped(m_terminal)){
			bool cut = false;
			int bytes = FileLines::readLineAt(__i_fs, filepath, offset, linedata, &cut, [&]() -> bool {
				return !commandStopped(m_terminal);
			}, 200);
			if(bytes < 0) break;

			int matchPos = regex_match(pattern, linedata.c_str(), (int)linedata.size());
			if(matchPos >= 0){
				m_matched++;
				m_terminal->write(filepath, filepath_len);
				m_terminal->write(':');
				m_terminal->write((int32_t)(lineNo + 1));
				m_terminal->write(':');
				m_terminal->write((int32_t)matchPos);
				m_terminal->write(':');
				m_terminal->write(linedata.c_str());
				m_terminal->putln();
			}

			if(cut){
				m_terminal->write(filepath, filepath_len);
				m_terminal->write(':');
				reportCut(lineNo, linedata.size());
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
				if(nullptr == item.m_name || commandStopped(m_terminal)) continue;
				if(0 == strcmp(item.m_name, ".") || 0 == strcmp(item.m_name, "..")) continue;

				int namelen = (int)strlen(item.m_name);
				char *childpath = pdiutil::safe_new_array<char>(dirpath_len + namelen + 2);

				if(nullptr != childpath){
					memcpy(childpath, dirpath, dirpath_len);
					__i_fs.appendFileSeparator(childpath);
					strncat(childpath, item.m_name, namelen);
					int childpath_len = (int)strlen(childpath);

					if(item.m_type == FILE_TYPE_DIR){
						grepDir(childpath, childpath_len, pattern);
					}else{
						grepFile(childpath, childpath_len, pattern);
					}

					pdiutil::safe_delete_array(childpath);
				}
			}
		}

		for(file_info_t &item : items){
			pdiutil::safe_delete_array(item.m_name);
		}
		items.clear();
	}

	/**
	 * Matches the pattern against each line arriving on the input descriptor,
	 * which is where a pipeline stage finds its input.
	 */
	void grepStream(const char *pattern){

		pdiutil::string linedata;
		int32_t lineNo = 0;
		bool cut = false;
		char block[64];

		while(m_terminal->available() > 0 && !commandStopped(m_terminal)){

			int32_t got = m_terminal->read((uint8_t*)block, sizeof(block));
			if(got <= 0) break;

			for(int32_t i = 0; i < got; i++){

				if('\n' != block[i]){
					if(!cut){
						pdiutil::string::size_type held = linedata.size();
						linedata += block[i];
						cut = (linedata.size() == held);
					}
					continue;
				}

				if(!linedata.empty() && linedata.back() == '\r'){
					linedata.pop_back();
				}

				int matchPos = regex_match(pattern, linedata.c_str(), (int)linedata.size());
				if(matchPos >= 0){
					m_matched++;
					m_terminal->write((int32_t)(lineNo + 1));
					m_terminal->write(':');
					m_terminal->write((int32_t)matchPos);
					m_terminal->write(':');
					m_terminal->write(linedata.c_str());
					m_terminal->putln();
				}

				if(cut){
					reportCut(lineNo, linedata.size());
				}

				linedata.clear();
				cut = false;
				lineNo++;
			}
			__i_dvc_ctrl.yield();
		}

		if(!linedata.empty()){
			int matchPos = regex_match(pattern, linedata.c_str(), (int)linedata.size());
			if(matchPos >= 0){
				m_matched++;
				m_terminal->write((int32_t)(lineNo + 1));
				m_terminal->write(':');
				m_terminal->write((int32_t)matchPos);
				m_terminal->write(':');
				m_terminal->write(linedata.c_str());
				m_terminal->putln();
			}
		}

		if(cut){
			reportCut(lineNo, linedata.size());
		}
	}

	/**
	 * Says that a line was longer than the heap could hold, so it was matched
	 * on the part held rather than answered as no match in silence.
	 */
	void reportCut(int32_t lineNo, pdiutil::string::size_type held){
		m_terminal->write((int32_t)(lineNo + 1));
		m_terminal->write_ro(RODT_ATTR(": too long, matched on its first "));
		m_terminal->write((int32_t)held);
		m_terminal->writeln_ro(RODT_ATTR(" bytes"));
	}

	pdi_err_t execute(cmd_term_inseq_t terminputaction){

#ifdef ENABLE_AUTH_SERVICE
		if( needauth() && !__auth_service.getAuthorized()){
			return CMD_ERROR_PERM;
		}
#endif

		pdi_err_t result = PDI_OK;

		if(nullptr != m_terminal){
			CommandOption *patternoptn = &m_options[0];
			CommandOption *fileoptn = &m_options[1];

			bool isPatternProvided = optionGiven(patternoptn);
			bool isFileProvided = (nullptr != fileoptn && nullptr != fileoptn->optionval && fileoptn->optionvalsize > 0);

			m_matched = 0;
			if(isPatternProvided && (isFileProvided || isInputRedirected())){

				char *pattern = pdiutil::safe_new_array<char>(patternoptn->optionvalsize + 1);
				pdiutil::string filepath = isFileProvided ? resolveArgPath(fileoptn) : pdiutil::string();

				if(nullptr != pattern){
					memcpy(pattern, patternoptn->optionval, patternoptn->optionvalsize);
				}
				if(nullptr != pattern && !regex_valid(pattern)){
					result = refuseArg(RODT_ATTR("invalid pattern : "), patternoptn->optionval, patternoptn->optionvalsize, CMD_ERROR_INVAL);
				}else if(nullptr != pattern && (!filepath.empty() || !isFileProvided)){

					const char* path = filepath.c_str();
					int path_len = (int)filepath.size();

					m_terminal->putln();

					if(filepath.empty()){
						grepStream(pattern);
					}else if(__i_fs.isDirectory(path)){
						grepDir(path, path_len, pattern);
					}else if(__i_fs.isFileExist(path)){
						grepFile(path, path_len, pattern);
					}else{
						result = CMD_ERROR_FAILED;
						m_terminal->write_ro(RODT_ATTR(CMD_MSG_NOT_FOUND));
						writeName(path);
					}
				}

				pdiutil::safe_delete_array(pattern);
			}else{
				result = CMD_ERROR_ARGS_MISSING;
			}
		}

		if( PDI_OK == result && 0 == m_matched && !SessionManager::interrupted() ){
			result = CMD_RESULT_FALSE;
		}
		return stoppedResult(result);
	}

private:

	uint32_t m_matched = 0;
};

#endif

#endif
