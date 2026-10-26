/*************************** List File System Command *************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st June 2019
******************************************************************************/

#ifndef _LIST_FILE_SYSTEM_COMMAND_H_
#define _LIST_FILE_SYSTEM_COMMAND_H_

#include "CommandCommon.h"
#include <utility/DataTypeConversions.h>
#include <interface/pdi/middlewares/iNtpInterface.h>
#if defined(ENABLE_AUTH_SERVICE) && defined(ENABLE_STORAGE_SERVICE)
#include <service_provider/user/UserStoreService.h>
#endif

#ifdef ENABLE_STORAGE_SERVICE
/**
 * list file system command
 * 
 * e.g. if we want to list the files & directories in the current directory, we can execute command as below
 * ls
 */
struct ListFSCommand : public CommandBase {

	/* Constructor */
	ListFSCommand(){
		Clear();
		SetCommand(CMD_NAME_LS);
		setAcceptArgsOptions(true);
		setCmdOptionSeparator(CMD_OPTION_SEPERATOR_SPACE);
	}

	/**
     * @brief Register the command.
     */
    static void RegisterCommand(){
		CommandBase::RegisterCommand(CMD_NAME_LS, [](void *arg)->void *{
			return pdiutil::safe_new<ListFSCommand>();
		});
	}

	const char* getUsage() const override {
		return RODT_ATTR("ls [<path>]  list a directory, or one file (defaults to current dir)");
	}

#ifdef ENABLE_AUTH_SERVICE
	/* override the necesity of required permission */
	bool needauth() override { return true; }
#endif

	/* execute command with provided options */
	pdi_err_t execute(cmd_term_inseq_t terminputaction){

#ifdef ENABLE_AUTH_SERVICE
		// return in case authentication needed and not authorized yet
		if( needauth() && !__auth_service.getAuthorized()){
			return CMD_ERROR_PERM;
		}
#endif

		pdi_err_t result = PDI_OK;

		if(nullptr != m_terminal){

			// Optional positional arg: resolve against PWD; missing → PWD itself.
			CommandOption *cmdoptn = &m_options[0];
			pdiutil::string target = resolveArgPath(cmdoptn);
			if( target.empty() ){
				target = SessionManager::getPWD();
			}

			pdiutil::vector<file_info_t> itemlist;
			int resultCode = 0;
			if( __i_fs.isFileExist(target.c_str()) && !__i_fs.isDirectory(target.c_str()) ){
				bool typed = ( nullptr != cmdoptn->optionval && cmdoptn->optionvalsize > 0 );
				pdiutil::string shown = typed ? pdiutil::string(cmdoptn->optionval, cmdoptn->optionvalsize) : target;
				resultCode = addEntry(target.c_str(), shown.c_str(), itemlist);
			}else{
				resultCode = __i_fs.getDirFileList(target.c_str(), itemlist);
				if( resultCode >= 0 ){
					addDotEntries(target, itemlist);
				}
			}

			m_terminal->putln();

			if(resultCode < 0){
				result = CMD_ERROR_FAILED;
				writeFailure(RODT_ATTR("Failed : "), target.c_str(), (int32_t)resultCode);
			}

			// m_terminal->write_ro(RODT_ATTR("Used : "));
			// m_terminal->write((int64_t)__i_fs.getUsedSize());
			// m_terminal->write_ro(RODT_ATTR(", Free : "));
			// m_terminal->writeln((int64_t)__i_fs.getFreeSize());
			m_terminal->putln();

			// Current local year, computed once, used to pick between
			// "%b %d %H:%M" (same year) and "%b %d  %Y" (different year) —
			// matches GNU coreutils' `ls -l` display policy.
			uint32_t nowLocal = __i_ntp.is_valid_ntptime()
				? (uint32_t)__i_ntp.get_ntp_time() + (uint32_t)TZ_SEC
				: 0;
			char nowYear[5];
			EpochToDateTimeString(nowLocal, nowYear, sizeof(nowYear), "%Y");

			for (file_info_t item : itemlist) {
				printRow(item, nowYear);
				// deallocates memory for items
				pdiutil::safe_delete_array(item.m_name);
			}

			itemlist.clear();
		}

		return result;
	}

private:

	/**
	 * Adds one listing row for the path under the name given; 0, or the error that
	 * kept it out.
	 */
	int addEntry(const char *path, const char *name, pdiutil::vector<file_info_t> &itemlist){
		file_info_t meta;
		memset(&meta, 0, sizeof(meta));
		pdi_err_t got = __i_fs.getFileMeta(path, meta);
		if( 0 != got ){
			return got;
		}
		uint32_t namelen = (uint32_t)strlen(name);
		meta.m_name = pdiutil::safe_new_array<char>(namelen + 1);
		if( nullptr == meta.m_name ){
			return PDI_ERR_NO_MEM;
		}
		memcpy(meta.m_name, name, namelen + 1);
		itemlist.push_back(meta);
		return 0;
	}

	/**
	 * Puts the . and .. rows at the head of a directory's listing when its filesystem
	 * did not give them.
	 */
	void addDotEntries(const pdiutil::string &dir, pdiutil::vector<file_info_t> &itemlist){
		for( uint16_t i = 0; i < itemlist.size(); i++ ){
			if( nullptr != itemlist[i].m_name && 0 == strcmp(itemlist[i].m_name, ".") ){
				return;
			}
		}

		size_t cut = dir.size();
		while( cut > 1 && FILE_SEPARATOR[0] == dir[cut - 1] ) cut--;
		while( cut > 0 && FILE_SEPARATOR[0] != dir[cut - 1] ) cut--;
		pdiutil::string parent = (cut > 1) ? dir.substr(0, cut - 1) : pdiutil::string(FILE_SEPARATOR);

		pdiutil::vector<file_info_t> dots;
		if( 0 == addEntry(dir.c_str(), ".", dots) && 0 == addEntry(parent.c_str(), "..", dots) ){
			itemlist.insert(itemlist.begin(), dots.begin(), dots.end());
			return;
		}
		for( uint16_t i = 0; i < dots.size(); i++ ){
			pdiutil::safe_delete_array(dots[i].m_name);
		}
	}

	/**
	 * Writes one row the way ls -l does: mode, owner, group, size, time, name.
	 */
	void printRow(const file_info_t &item, const char *nowYear){
		char permbuf[12];
		FilePermsToString(item.m_perms, item.m_type == FILE_TYPE_DIR, permbuf);
		permbuf[10] = ' ';
		permbuf[11] = '\0';
		m_terminal->write(permbuf);

#if defined(ENABLE_AUTH_SERVICE) && defined(ENABLE_STORAGE_SERVICE)
		pdiutil::string unameStr, gnameStr;
		__user_store_service.resolveOwnerNames(item.m_uid, item.m_gid, unameStr, gnameStr);
		m_terminal->write_pad(unameStr.c_str(), 10);
		m_terminal->write_pad(gnameStr.c_str(), 10);
#else
		char fallbackId[8];
		Uint32ToString((uint32_t)item.m_uid, fallbackId, sizeof(fallbackId) - 1, 0);
		m_terminal->write_pad(fallbackId, 6);
		Uint32ToString((uint32_t)item.m_gid, fallbackId, sizeof(fallbackId) - 1, 0);
		m_terminal->write_pad(fallbackId, 6);
#endif

		char sizebuf[12];
		Uint32ToString((uint32_t)item.m_size, sizebuf, sizeof(sizebuf) - 1, 10);
		m_terminal->write_pad(sizebuf, 10);
		m_terminal->write(' ');

		char tsbuf[16];

		// Mtime column, ls-style: shift UTC by TZ then pick fmt by year.
		uint32_t mtimeLocal = item.m_mtime ? item.m_mtime + (uint32_t)TZ_SEC : 0;
		char mYear[5];
		EpochToDateTimeString(mtimeLocal, mYear, sizeof(mYear), "%Y");
		const char* mfmt = __are_arrays_equal(mYear, nowYear, 4)
			? "%b %d %H:%M" : "%b %d  %Y";
		EpochToDateTimeString(mtimeLocal, tsbuf, sizeof(tsbuf), mfmt);
		m_terminal->write(tsbuf);
		m_terminal->write(' ');

		m_terminal->writeln(item.m_name);
	}
};
#endif


#endif
