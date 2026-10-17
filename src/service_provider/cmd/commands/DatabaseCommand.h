/****************************** Db Command ************************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 22nd Aug 2026
******************************************************************************/

#ifndef _DATABASE_COMMAND_H_
#define _DATABASE_COMMAND_H_

#include "CommandCommon.h"
#include <database/core/DbLayout.h>
#include <service_provider/database/DatabaseServiceProvider.h>

/**
 * db command — look at the config database and manage the defaults behind it.
 *
 * e.g.
 *   db status     which medium is live, how much of it the records take
 *   db list       one line per record, sealed ones marked
 *   db verify     read every record past its checksum or tag
 *   db save       take what is running now as the defaults to fall back to
 *   db restore    put those defaults back in use
 */
struct DatabaseCommand : public CommandBase {

	DatabaseCommand(){
		Clear();
		SetCommand(CMD_NAME_DB);
		setAcceptArgsOptions(true);
		setCmdOptionSeparator(CMD_OPTION_SEPERATOR_SPACE);
	}

	static void RegisterCommand(){
		CommandBase::RegisterCommand(CMD_NAME_DB, [](void *arg)->void *{
			return pdiutil::safe_new<DatabaseCommand>();
		});
	}

	const char* getUsage() const override {
		return RODT_ATTR("db  status|list|verify|save|restore  inspect the config database");
	}

#ifdef ENABLE_AUTH_SERVICE
	bool needauth() override { return true; }
#endif

	pdi_err_t execute(cmd_term_inseq_t terminputaction){

#ifdef ENABLE_AUTH_SERVICE
		if( needauth() && !__auth_service.getAuthorized() ){
			return CMD_ERROR_PERM;
		}
#endif

		if( nullptr == m_terminal ){
			return CMD_ERROR_FAILED;
		}

		CommandOption *verbOpt = &m_options[0];
		if( nullptr == verbOpt || nullptr == verbOpt->optionval || 0 == verbOpt->optionvalsize ){
			return CMD_ERROR_ARGS_MISSING;
		}

		if( matches(verbOpt, RODT_ATTR("status"), 6) ){
			printStatus();
			return PDI_OK;
		}

		if( matches(verbOpt, RODT_ATTR("list"), 4) ){
			printList();
			return PDI_OK;
		}

		if( matches(verbOpt, RODT_ATTR("verify"), 6) ){
			return verifyAll();
		}

		if( matches(verbOpt, RODT_ATTR("save"), 4) ){
			bool ok = __database_service.save_defaults();
			m_terminal->putln();
			m_terminal->writeln_ro(ok ? RODT_ATTR("defaults saved") : RODT_ATTR("no defaults medium to save to"));
			return ok ? PDI_OK : CMD_ERROR_FAILED;
		}

		if( matches(verbOpt, RODT_ATTR("restore"), 7) ){
			bool ok = __database_service.restore_defaults();
			if( ok ){
				__database_service.write_config_files();
			}
			m_terminal->putln();
			m_terminal->writeln_ro(ok ? RODT_ATTR("defaults restored") : RODT_ATTR("no defaults medium to restore from"));
			return ok ? PDI_OK : CMD_ERROR_FAILED;
		}

		return CMD_ERROR_OPT;
	}

private:

	// verb literals live in RO/PROGMEM, strncmp_ro reads them safely there.
	// length gate first so a same length false prefix cannot match.
	bool matches(CommandOption *opt, const char *verb, uint16_t len){
		return opt->optionvalsize == len && 0 == strncmp_ro(opt->optionval, verb, len);
	}

	void printStatus(){

		m_terminal->writeln();
		m_terminal->write_ro(RODT_ATTR("medium   : "));
		m_terminal->writeln_ro(__database_service.is_storage_tier() ? RODT_ATTR(DB_STORE_FILE) : RODT_ATTR("eeprom"));

		m_terminal->write_ro(RODT_ATTR("mounted  : "));
		m_terminal->writeln_ro(__db_layout.is_mounted() ? RODT_ATTR("yes") : RODT_ATTR("no"));

		m_terminal->write_ro(RODT_ATTR("records  : "));
		m_terminal->write((int32_t)__db_layout.entry_count());
		m_terminal->putln();

		m_terminal->write_ro(RODT_ATTR("used     : "));
		m_terminal->write((int32_t)__db_layout.used_bytes());
		m_terminal->putln();

		m_terminal->write_ro(RODT_ATTR("free     : "));
		m_terminal->write((int32_t)__db_layout.free_bytes());
		m_terminal->putln();

		m_terminal->write_ro(RODT_ATTR("firmware : "));
		m_terminal->write((int32_t)__db_layout.firmware_version());
		m_terminal->putln();
	}

	void printList(){

		m_terminal->writeln();
		m_terminal->write_pad_ro(RODT_ATTR("ID"),    2, COL_ID);
		m_terminal->write_pad_ro(RODT_ATTR("VER"),   3, COL_VER);
		m_terminal->write_pad_ro(RODT_ATTR("SLOT"),  4, COL_NUM);
		m_terminal->write_pad_ro(RODT_ATTR("BYTES"), 5, COL_NUM);
		m_terminal->writeln_ro(RODT_ATTR("STATE"));

		char buf[16];
		for( uint8_t i = 0; i < __db_layout.entry_count(); i++ ){

			const db_dir_entry_t *entry = __db_layout.entry_at(i);
			if( nullptr == entry ) continue;

			Int32ToString((int32_t)entry->m_id, buf, sizeof(buf), 0);
			m_terminal->write_pad(buf, COL_ID);
			Int32ToString((int32_t)entry->m_version, buf, sizeof(buf), 0);
			m_terminal->write_pad(buf, COL_VER);
			Int32ToString((int32_t)entry->m_cap, buf, sizeof(buf), 0);
			m_terminal->write_pad(buf, COL_NUM);
			Int32ToString((int32_t)entry->m_len, buf, sizeof(buf), 0);
			m_terminal->write_pad(buf, COL_NUM);

			if( 0 == entry->m_len ){
				m_terminal->write_ro(RODT_ATTR("default"));
			}else if( entry->m_flags & DB_ENTRY_FLAG_SEALED ){
				m_terminal->write_ro(RODT_ATTR("sealed"));
			}else{
				m_terminal->write_ro(RODT_ATTR("stored"));
			}
			m_terminal->putln();
		}
	}

	/**
	 * run every record past its checksum or its tag. Nothing is copied out, so
	 * this needs no room to hold a record and never sees a credential.
	 */
	pdi_err_t verifyAll(){

		uint8_t bad = 0;

		m_terminal->writeln();

		for( uint8_t i = 0; i < __db_layout.entry_count(); i++ ){

			const db_dir_entry_t *entry = __db_layout.entry_at(i);
			if( nullptr == entry ) continue;

			if( PDI_OK != __db_layout.verify_record(entry->m_id) ){
				bad++;
				m_terminal->write_ro(RODT_ATTR("record "));
				m_terminal->write((int32_t)entry->m_id);
				m_terminal->writeln_ro(RODT_ATTR(" failed"));
			}
		}

		if( 0 == bad ){
			m_terminal->writeln_ro(RODT_ATTR("every record verified"));
			return PDI_OK;
		}

		return CMD_ERROR_FAILED;
	}

	static constexpr uint8_t COL_ID  = 4;
	static constexpr uint8_t COL_VER = 5;
	static constexpr uint8_t COL_NUM = 8;
};

#endif
