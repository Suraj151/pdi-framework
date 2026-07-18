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

			m_terminal->write_ro(RODT_ATTR("Used : "));
			m_terminal->write((int64_t)__i_fs.getUsedSize());
			m_terminal->write_ro(RODT_ATTR(", Free : "));
			m_terminal->writeln((int64_t)__i_fs.getFreeSize());
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
				if( item.type == FILE_TYPE_DIR ){
					m_terminal->write_ro(RODT_ATTR("D "));
				}else {
					m_terminal->write_ro(RODT_ATTR("F "));
				}

				// Permissions as 4 octal digits (chmod style), e.g. 0644 / 0755.
				char permbuf[6];
				permbuf[0] = '0' + ((item.perms >> 9) & 7);
				permbuf[1] = '0' + ((item.perms >> 6) & 7);
				permbuf[2] = '0' + ((item.perms >> 3) & 7);
				permbuf[3] = '0' + (item.perms & 7);
				permbuf[4] = ' ';
				permbuf[5] = '\0';
				m_terminal->write(permbuf);

				char tsbuf[16];

				// // Ctime column, ls-style: shift UTC by TZ then pick fmt by year.
				// uint32_t ctimeLocal = item.ctime ? item.ctime + (uint32_t)TZ_SEC : 0;
				// char cYear[5];
				// EpochToDateTimeString(ctimeLocal, cYear, sizeof(cYear), "%Y");
				// const char* cfmt = __are_arrays_equal(cYear, nowYear, 4)
				// 	? "%b %d %H:%M" : "%b %d  %Y";
				// EpochToDateTimeString(ctimeLocal, tsbuf, sizeof(tsbuf), cfmt);
				// m_terminal->write(tsbuf);
				// m_terminal->write(' ');

				// Mtime column, ls-style: shift UTC by TZ then pick fmt by year.
				uint32_t mtimeLocal = item.mtime ? item.mtime + (uint32_t)TZ_SEC : 0;
				char mYear[5];
				EpochToDateTimeString(mtimeLocal, mYear, sizeof(mYear), "%Y");
				const char* mfmt = __are_arrays_equal(mYear, nowYear, 4)
					? "%b %d %H:%M" : "%b %d  %Y";
				EpochToDateTimeString(mtimeLocal, tsbuf, sizeof(tsbuf), mfmt);
				m_terminal->write(tsbuf);
				m_terminal->write(' ');

				char sizebuf[12];
				Uint32ToString((uint32_t)item.size, sizebuf, sizeof(sizebuf) - 1, 10);
				m_terminal->write(sizebuf);
				m_terminal->write(' ');

				m_terminal->writeln(item.name);
				// deallocates memory for items
				delete[] item.name;
			}

			itemlist.clear();
		}

		return result;
	}
};
#endif


#endif
