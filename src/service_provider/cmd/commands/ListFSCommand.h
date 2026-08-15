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
		return RODT_ATTR("ls [<dir>]  list files and directories (defaults to current dir)");
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

			// Optional positional arg: resolve against PWD; missing → PWD itself.
			CommandOption *cmdoptn = &m_options[0];
			pdiutil::string target = resolveArgPath(cmdoptn);
			if( target.empty() ){
				target = SessionManager::getPWD();
			}

			pdiutil::vector<file_info_t> itemlist;
			int resultCode = __i_fs.getDirFileList(target.c_str(), itemlist);
			
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

			for (file_info_t item : itemlist) {
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
				// deallocates memory for items
				pdiutil::safe_delete_array(item.m_name);
			}

			itemlist.clear();
		}

		return result;
	}
};
#endif


#endif
