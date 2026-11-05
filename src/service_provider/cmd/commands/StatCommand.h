/******************************** Stat Command ********************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st Nov 2026
******************************************************************************/

#ifndef _STAT_COMMAND_H_
#define _STAT_COMMAND_H_

#include "CommandCommon.h"
#include <utility/DataTypeConversions.h>
#if defined(ENABLE_AUTH_SERVICE) && defined(ENABLE_STORAGE_SERVICE)
#include <service_provider/user/UserStoreService.h>
#endif

#ifdef ENABLE_STORAGE_SERVICE

struct StatCommand : public CommandBase {

	StatCommand(){
		Clear();
		SetCommand(CMD_NAME_STAT);
		setArgs(RODT_ATTR(""), 255);
		setAcceptArgsOptions(true);
		setCmdOptionSeparator(CMD_OPTION_SEPERATOR_SPACE);
	}

	static void RegisterCommand(){
		CommandBase::RegisterCommand(CMD_NAME_STAT, [](void *arg)->void *{
			return pdiutil::safe_new<StatCommand>();
		});
	}

	const char* getUsage() const override {
		return RODT_ATTR("stat <path> [<path>...]  size, type, mode, owner and times of each path");
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

		if( !optionGiven(&m_options[0]) ){
			return CMD_ERROR_ARGS_MISSING;
		}

		pdi_err_t result = PDI_OK;
		bool broken = false;
		m_terminal->putln();

		for( uint8_t i = 0; i < CMD_OPTION_MAX && !commandStopped(m_terminal); i++ ){

			CommandOption *opt = &m_options[i];
			if( !optionGiven(opt) ){
				break;
			}

			pdiutil::string typed(opt->optionval, (pdiutil::string::size_type)opt->optionvalsize);
			pdiutil::string path = resolveArgPath(opt);

			file_info_t meta;
			memset(&meta, 0, sizeof(meta));
			pdi_err_t got = __i_fs.getFileMeta(path.c_str(), meta);
			if( broken ){
				m_terminal->putln();
				broken = false;
			}
			if( 0 != got ){
				writeFailure(RODT_ATTR(CMD_MSG_FAILED), typed.c_str(), (int32_t)got);
				broken = true;
				result = CMD_ERROR_FAILED;
				continue;
			}
			printMeta(typed, meta);
		}

		return stoppedResult(result);
	}

private:

	/**
	 * The words stat shows for a type.
	 */
	const char *typeName(file_type_t type){
		switch( type ){
			case FILE_TYPE_DIR: return RODT_ATTR("directory");
			case FILE_TYPE_LINK: return RODT_ATTR("symbolic link");
			case FILE_TYPE_CHR: return RODT_ATTR("character special file");
			case FILE_TYPE_BLK: return RODT_ATTR("block special file");
			default: return RODT_ATTR("regular file");
		}
	}

	/**
	 * Writes the stat block of one path: name, size and type, mode with owner and
	 * group, then the modify and birth times.
	 */
	void printMeta(const pdiutil::string &typed, const file_info_t &meta){
		bool isdir = ( FILE_TYPE_DIR == meta.m_type );

		m_terminal->write_ro(RODT_ATTR("  File: "));
		m_terminal->writeln(typed.c_str());

		char num[24];
		Uint64ToString(isdir ? 0 : meta.m_size, num, sizeof(num) - 1, 0);
		m_terminal->write_ro(RODT_ATTR("  Size: "));
		m_terminal->write_column(num, 12);
		m_terminal->writeln_ro(typeName(meta.m_type));

		char mode[6];
		uint16_t perms = meta.m_perms & 07777;
		mode[0] = (char)('0' + ((perms >> 9) & 7));
		mode[1] = (char)('0' + ((perms >> 6) & 7));
		mode[2] = (char)('0' + ((perms >> 3) & 7));
		mode[3] = (char)('0' + (perms & 7));
		mode[4] = '\0';
		char permbuf[12];
		FilePermsToString(meta.m_perms, meta.m_type, permbuf);
		permbuf[10] = '\0';

		m_terminal->write_ro(RODT_ATTR("Access: ("));
		m_terminal->write(mode);
		m_terminal->write('/');
		m_terminal->write(permbuf);
		m_terminal->write_ro(RODT_ATTR(")  Uid: ("));
		Uint32ToString((uint32_t)meta.m_uid, num, sizeof(num) - 1, 0);
		m_terminal->write(num);
		m_terminal->write('/');

		pdiutil::string owner, group;
#if defined(ENABLE_AUTH_SERVICE)
		__user_store_service.resolveOwnerNames(meta.m_uid, meta.m_gid, owner, group);
#endif
		m_terminal->write(owner.c_str());
		m_terminal->write_ro(RODT_ATTR(")  Gid: ("));
		Uint32ToString((uint32_t)meta.m_gid, num, sizeof(num) - 1, 0);
		m_terminal->write(num);
		m_terminal->write('/');
		m_terminal->write(group.c_str());
		m_terminal->writeln(')');

		m_terminal->write_ro(RODT_ATTR("Modify: "));
		printTime(meta.m_mtime);
		m_terminal->write_ro(RODT_ATTR(" Birth: "));
		printTime(meta.m_ctime);
	}

	/**
	 * Writes an epoch as local date and time, or a dash when it is not known.
	 */
	void printTime(uint32_t epoch){
		if( 0 == epoch ){
			m_terminal->writeln('-');
			return;
		}
		char when[24];
		EpochToDateTimeString(epoch + (uint32_t)TZ_SEC, when, sizeof(when), "%Y-%m-%d %H:%M:%S");
		m_terminal->writeln(when);
	}
};

#endif

#endif
