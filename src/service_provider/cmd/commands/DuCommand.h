/********************************* Du Command *********************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st Nov 2026
******************************************************************************/

#ifndef _DU_COMMAND_H_
#define _DU_COMMAND_H_

#include "CommandCommon.h"

#ifdef ENABLE_STORAGE_SERVICE

struct DuCommand : public CommandBase {

	DuCommand(){
		Clear();
		SetCommand(CMD_NAME_DU);
		setArgs(RODT_ATTR("sbh"), 1);
		setAcceptArgsOptions(true);
		setCmdOptionSeparator(CMD_OPTION_SEPERATOR_SPACE);
	}

	static void RegisterCommand(){
		CommandBase::RegisterCommand(CMD_NAME_DU, [](void *arg)->void *{
			return pdiutil::safe_new<DuCommand>();
		});
	}

	const char* getUsage() const override {
		return RODT_ATTR("du [-s] [-b] [-h] [<path>]  space each directory takes on its disk, in KiB (-b bytes the files hold, -h human sizes, -s total only)");
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

		bool summary = false;
		bool bytes = false;
		bool human = false;
		pdiutil::string typed;
		bool given = false;

		for( uint8_t i = 0; i < CMD_OPTION_MAX; i++ ){

			CommandOption *opt = &m_options[i];
			if( !optionGiven(opt) ){
				break;
			}

			if( opt->optionvalsize > 1 && '-' == opt->optionval[0] ){
				for( int16_t c = 1; c < opt->optionvalsize; c++ ){
					summary = summary || ( 's' == opt->optionval[c] );
					bytes = bytes || ( 'b' == opt->optionval[c] );
					human = human || ( 'h' == opt->optionval[c] );
				}
			}else{
				typed.assign(opt->optionval, (pdiutil::string::size_type)opt->optionvalsize);
				given = true;
			}
		}

		pdiutil::string start = given ? resolveArgPathStr(typed.c_str(), (int16_t)typed.size()) : SessionManager::getPWD();
		if( !given ){
			typed = ".";
		}

		m_terminal->putln();

		if( !__i_fs.isDirectory(start.c_str()) ){
			if( !__i_fs.isFileExist(start.c_str()) ){
				m_terminal->write_ro(RODT_ATTR(CMD_MSG_NOT_FOUND));
				writeName(typed.c_str());
				return CMD_ERROR_FAILED;
			}
			int64_t size = __i_fs.getFileSize(start.c_str());
			uint64_t total = usage(size > 0 ? (uint64_t)size : 0, blockSizeOf(start), bytes);
			char text[24];
			shownSize(total, bytes, human, text, sizeof(text));
			printRow(text, (uint32_t)strlen(text), typed);
			return PDI_OK;
		}

		pdi_err_t result = PDI_OK;
		pdiutil::vector<pdiutil::string> paths;
		pdiutil::vector<pdiutil::string> shown;
		pdiutil::vector<int32_t> parents;
		pdiutil::vector<uint64_t> totals;
		pdiutil::vector<int32_t> pending;

		paths.push_back(start);
		shown.push_back(typed);
		parents.push_back(-1);
		totals.push_back(0);
		pending.push_back(0);

		pdiutil::vector<int32_t> order;

		while( !pending.empty() && !commandStopped(m_terminal) ){

			int32_t at = pending.back();
			pending.pop_back();
			order.push_back(at);

			pdiutil::vector<file_info_t> items;
			int rc = __i_fs.getDirFileList(paths[at].c_str(), items);
			uint32_t blocksize = blockSizeOf(paths[at]);
			if( rc < 0 ){
				writeFailure(RODT_ATTR(CMD_MSG_FAILED), shown[at].c_str(), (int32_t)rc);
				m_terminal->putln();
				result = CMD_ERROR_FAILED;
			}

			uint32_t firstchild = (uint32_t)paths.size();

			for( uint16_t j = 0; j < items.size(); j++ ){
				const char *entry = items[j].m_name;
				if( nullptr == entry || 0 == strcmp(entry, ".") || 0 == strcmp(entry, "..") ){
					continue;
				}
				if( FILE_TYPE_DIR != items[j].m_type ){
					totals[at] += usage(items[j].m_size, blocksize, bytes);
					continue;
				}
				pdiutil::string childpath = paths[at];
				__i_fs.appendFileSeparator(childpath);
				childpath += entry;
				pdiutil::string childshown = shown[at];
				__i_fs.appendFileSeparator(childshown);
				childshown += entry;
				paths.push_back(childpath);
				shown.push_back(childshown);
				parents.push_back(at);
				totals.push_back(0);
			}

			for( int32_t c = (int32_t)paths.size() - 1; c >= (int32_t)firstchild; c-- ){
				pending.push_back(c);
			}

			for( uint16_t j = 0; j < items.size(); j++ ){
				pdiutil::safe_delete_array(items[j].m_name);
			}

			__i_dvc_ctrl.yield();
		}

		for( int32_t i = (int32_t)order.size() - 1; i > 0; i-- ){
			int32_t node = order[i];
			totals[parents[node]] += totals[node];
		}

		uint32_t width = 1;
		for( uint32_t i = 0; i < order.size(); i++ ){
			if( summary && 0 != order[i] ){
				continue;
			}
			char text[24];
			shownSize(totals[order[i]], bytes, human, text, sizeof(text));
			width = pdistd::max(width, (uint32_t)strlen(text));
		}

		pdiutil::vector<int32_t> open;
		for( uint32_t i = 0; i < order.size() && !commandStopped(m_terminal); i++ ){
			int32_t node = order[i];
			while( !open.empty() && open.back() != parents[node] ){
				if( !summary ){
					writeTotal(totals[open.back()], bytes, human, width, shown[open.back()]);
				}
				open.pop_back();
			}
			open.push_back(node);
		}
		while( !open.empty() && !commandStopped(m_terminal) ){
			if( !summary || 0 == open.back() ){
				writeTotal(totals[open.back()], bytes, human, width, shown[open.back()]);
			}
			open.pop_back();
		}

		return stoppedResult(result);
	}

private:

	/**
	 * Writes one row: the size right-aligned to the width given, then the path.
	 */
	void printRow(const char *size, uint32_t width, const pdiutil::string &name){
		m_terminal->write_column(size, width, true);
		m_terminal->writeln(name.c_str());
	}

	/**
	 * Writes one directory's row, its size column as wide as the widest row.
	 */
	void writeTotal(uint64_t total, bool bytes, bool human, uint32_t width, const pdiutil::string &name){
		char text[24];
		shownSize(total, bytes, human, text, sizeof(text));
		printRow(text, width, name);
	}

	/**
	 * A total the way the row shows it: bytes for -b, a human size for -h, KiB
	 * otherwise.
	 */
	static void shownSize(uint64_t total, bool bytes, bool human, char *out, uint8_t outsize){
		if( human ){
			BytesToSizeString(total, true, out, outsize);
		}else{
			Uint64ToString(bytes ? total : (total + 1023) / 1024, out, outsize - 1, 0);
		}
	}

	/**
	 * The space a file takes: its bytes for -b, otherwise the blocks its disk
	 * stores it in, counted in whole KiB at least, so a directory never shows less
	 * than the rows below it; nothing on a disk whose files take no storage.
	 */
	static uint64_t usage(uint64_t size, uint32_t blocksize, bool bytes){
		if( bytes ){
			return size;
		}
		if( 0 == blocksize || 0 == size ){
			return 0;
		}
		uint64_t unit = (blocksize < 1024) ? 1024 : blocksize;
		return ((size + unit - 1) / unit) * unit;
	}

	/**
	 * The block size of the disk a path is on.
	 */
	static uint32_t blockSizeOf(const pdiutil::string &path){
		const vfs_mount_t *mount = __i_fs.findMountForPath(path.c_str());
		if( nullptr == mount || nullptr == mount->m_backend ){
			return __i_fs.getBlockSize();
		}
		return mount->m_backend->getBlockSize();
	}
};

#endif

#endif
