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
	}

	/**
     * @brief Register the command.
     */
    static void RegisterCommand(){
		CommandBase::RegisterCommand(CMD_NAME_LS, [](void *arg)->void *{
			return new ListFSCommand();
		});
	}

	const char* getUsage() const override {
		return RODT_ATTR("ls  list files and directories in the current directory");
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

		cmd_result_t result = CMD_RESULT_OK;

		if(nullptr != m_terminal){

			pdiutil::vector<file_info_t> itemlist;
			int resultCode = __i_fs.getDirFileList(SessionManager::getPWD().c_str(), itemlist);
			
			if(resultCode < 0){
				result = CMD_RESULT_FAILED;
			}

			// Print the directory and file list
			m_terminal->putln();

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

#if defined(ENABLE_AUTH_SERVICE) && defined(ENABLE_STORAGE_SERVICE)
			// Per-ls uid->name cache so we don't rescan /etc/passwd per row.
			static constexpr uint8_t NAME_CACHE_MAX = 4;
			uint16_t cachedUid[NAME_CACHE_MAX];
			pdiutil::string cachedName[NAME_CACHE_MAX];
			uint8_t cachedCount = 0;
#endif

			for (file_info_t item : itemlist) {
				if( item.m_type == FILE_TYPE_DIR ){
					m_terminal->write_ro(RODT_ATTR("D "));
				}else {
					m_terminal->write_ro(RODT_ATTR("F "));
				}

				// Permissions as 4 octal digits (chmod style), e.g. 0644 / 0755.
				char permbuf[6];
				permbuf[0] = '0' + ((item.m_perms >> 9) & 7);
				permbuf[1] = '0' + ((item.m_perms >> 6) & 7);
				permbuf[2] = '0' + ((item.m_perms >> 3) & 7);
				permbuf[3] = '0' + (item.m_perms & 7);
				permbuf[4] = ' ';
				permbuf[5] = '\0';
				m_terminal->write(permbuf);

				char fallbackId[8];
#if defined(ENABLE_AUTH_SERVICE) && defined(ENABLE_STORAGE_SERVICE)
				const char *unameStr = nullptr;
				for (uint8_t ci = 0; ci < cachedCount; ci++) {
					if (cachedUid[ci] == item.m_uid) {
						unameStr = cachedName[ci].c_str();
						break;
					}
				}
				user_record_t urec;
				if (nullptr == unameStr && __user_store_service.findUserByUid(item.m_uid, urec)) {
					if (cachedCount < NAME_CACHE_MAX) {
						cachedUid[cachedCount] = item.m_uid;
						cachedName[cachedCount] = urec.m_username;
						unameStr = cachedName[cachedCount].c_str();
						cachedCount++;
					} else {
						unameStr = urec.m_username.c_str();
					}
				}
				if (nullptr == unameStr) {
					Uint32ToString((uint32_t)item.m_uid, fallbackId, sizeof(fallbackId) - 1, 0);
					unameStr = fallbackId;
				}
				m_terminal->write_pad(unameStr, 10);

				const char *gnameStr = nullptr;
				if (item.m_gid == item.m_uid) {
					// gid == uid convention: reuse the resolved user name.
					gnameStr = unameStr;
				} else {
					for (uint8_t ci = 0; ci < cachedCount; ci++) {
						if (cachedUid[ci] == item.m_gid) {
							gnameStr = cachedName[ci].c_str();
							break;
						}
					}
					user_record_t grec;
					if (nullptr == gnameStr && __user_store_service.findUserByUid(item.m_gid, grec)) {
						gnameStr = grec.m_username.c_str();
					}
					if (nullptr == gnameStr) {
						Uint32ToString((uint32_t)item.m_gid, fallbackId, sizeof(fallbackId) - 1, 0);
						gnameStr = fallbackId;
					}
				}
				m_terminal->write_pad(gnameStr, 10);
#else
				Uint32ToString((uint32_t)item.m_uid, fallbackId, sizeof(fallbackId) - 1, 0);
				m_terminal->write_pad(fallbackId, 6);
				Uint32ToString((uint32_t)item.m_gid, fallbackId, sizeof(fallbackId) - 1, 0);
				m_terminal->write_pad(fallbackId, 6);
#endif

				char tsbuf[16];

				// // Ctime column, ls-style: shift UTC by TZ then pick fmt by year.
				// uint32_t ctimeLocal = item.m_ctime ? item.m_ctime + (uint32_t)TZ_SEC : 0;
				// char cYear[5];
				// EpochToDateTimeString(ctimeLocal, cYear, sizeof(cYear), "%Y");
				// const char* cfmt = __are_arrays_equal(cYear, nowYear, 4)
				// 	? "%b %d %H:%M" : "%b %d  %Y";
				// EpochToDateTimeString(ctimeLocal, tsbuf, sizeof(tsbuf), cfmt);
				// m_terminal->write(tsbuf);
				// m_terminal->write(' ');

				// Mtime column, ls-style: shift UTC by TZ then pick fmt by year.
				uint32_t mtimeLocal = item.m_mtime ? item.m_mtime + (uint32_t)TZ_SEC : 0;
				char mYear[5];
				EpochToDateTimeString(mtimeLocal, mYear, sizeof(mYear), "%Y");
				const char* mfmt = __are_arrays_equal(mYear, nowYear, 4)
					? "%b %d %H:%M" : "%b %d  %Y";
				EpochToDateTimeString(mtimeLocal, tsbuf, sizeof(tsbuf), mfmt);
				m_terminal->write(tsbuf);
				m_terminal->write(' ');

				char sizebuf[12];
				Uint32ToString((uint32_t)item.m_size, sizebuf, sizeof(sizebuf) - 1, 10);
				m_terminal->write(sizebuf);
				m_terminal->write(' ');

				m_terminal->writeln(item.m_name);
				// deallocates memory for items
				delete[] item.m_name;
			}

			itemlist.clear();
		}

		return result;
	}
};
#endif


#endif
