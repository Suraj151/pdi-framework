/******************************** Df Command **********************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 28th May 2026
******************************************************************************/

#ifndef _DF_FS_COMMAND_H_
#define _DF_FS_COMMAND_H_

#include "CommandCommon.h"
#include <helpers/ProcHelper.h>

#ifdef ENABLE_STORAGE_SERVICE

/**
 * df command — one line per mounted VFS backend with total/used/free bytes.
 */
struct DfFSCommand : public CommandBase {

	DfFSCommand(){
		Clear();
		SetCommand(CMD_NAME_DF);
		setArgs(RODT_ATTR("h"), 0);
		setAcceptArgsOptions(true);
		setCmdOptionSeparator(CMD_OPTION_SEPERATOR_SPACE);
	}

	static void RegisterCommand(){
		CommandBase::RegisterCommand(CMD_NAME_DF, [](void *arg)->void *{
			return pdiutil::safe_new<DfFSCommand>();
		});
	}

	const char* getUsage() const override {
		return RODT_ATTR("df [-h]  list mounted filesystems with total/used/free bytes (-h human sizes)");
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
			return CMD_ERROR_FAILED;
		}

		bool human = optionGiven(&m_options[0]);

		pdiutil::vector<pdiutil::string> cells;
		addCell(cells, RODT_ATTR("MOUNT"));
		addCell(cells, RODT_ATTR("NAME"));
		addCell(cells, RODT_ATTR("TOTAL"));
		addCell(cells, RODT_ATTR("USED"));
		addCell(cells, RODT_ATTR("FREE"));

		bool whole = ( DF_COLUMNS == cells.size() );

#ifdef ENABLE_PROCFS
		pdiutil::string name, prefix;

		readProcLines(PROC_MOUNT_PREFIX "/mounts", [&](pdiutil::string &line) -> bool {
			if( !__line_field(line, 0, name) || !__line_field(line, 1, prefix) ) return true;
			whole = addUsage(cells, prefix.c_str(), name.c_str(), __i_fs.findMountForPath(prefix.c_str()), human) && whole;
			return true;
		});
#else
		for( uint8_t i = 0; i < __i_fs.getMountCount(); i++ ){
			const vfs_mount_t *m = __i_fs.getMount(i);
			if( nullptr == m ) continue;
			whole = addUsage(cells, m->m_prefix, m->m_name, m, human) && whole;
		}
#endif

		uint32_t widths[DF_COLUMNS] = {0, 0, 0, 0, 0};
		for( uint32_t i = 0; i < cells.size(); i++ ){
			widths[i % DF_COLUMNS] = pdistd::max(widths[i % DF_COLUMNS], (uint32_t)cells[i].size());
		}

		m_terminal->writeln();
		for( uint32_t i = 0; i + DF_COLUMNS <= cells.size(); i += DF_COLUMNS ){
			for( uint8_t c = 0; c < DF_COLUMNS - 1; c++ ){
				m_terminal->write_column(cells[i + c].c_str(), widths[c], c >= 2);
			}
			const pdiutil::string &last = cells[i + DF_COLUMNS - 1];
			for( uint32_t k = (uint32_t)last.size(); k < widths[DF_COLUMNS - 1]; k++ ){
				m_terminal->write(' ');
			}
			m_terminal->write(last.c_str());
			m_terminal->putln();
		}

		if( !whole ){
			writeFailure(RODT_ATTR(CMD_MSG_FAILED), CMD_NAME_DF, (int32_t)PDI_ERR_NO_MEM);
			return CMD_ERROR_FAILED;
		}
		return PDI_OK;
	}

private:

	/**
	 * Adds one cell of the table.
	 */
	static void addCell(pdiutil::vector<pdiutil::string> &cells, const char *ro){
		cells.push_back(pdiutil::string(CHARPTR_WRAP_RO(ro)));
	}

	/**
	 * Adds one mount's row, asking the backend behind it what it holds; the row
	 * goes in whole or not at all, and false when the heap could not hold it.
	 */
	static bool addUsage(pdiutil::vector<pdiutil::string> &cells, const char *prefix, const char *name,
						 const vfs_mount_t *mount, bool human){

		if( nullptr == mount || nullptr == mount->m_backend ) return true;

		uint64_t sizes[3] = { mount->m_backend->getTotalSize(), mount->m_backend->getUsedSize(), mount->m_backend->getFreeSize() };
		uint32_t before = cells.size();
		cells.push_back(pdiutil::string(prefix));
		cells.push_back(pdiutil::string(name));
		for( uint8_t k = 0; k < 3; k++ ){
			char buf[24];
			if( human ){
				BytesToSizeString(sizes[k], true, buf, sizeof(buf));
			}else{
				Uint64ToString(sizes[k], buf, sizeof(buf) - 1, 0);
			}
			cells.push_back(pdiutil::string(buf));
		}
		if( cells.size() != before + DF_COLUMNS ){
			cells.resize(before);
			return false;
		}
		return true;
	}

	static constexpr uint8_t DF_COLUMNS = 5;
};

#endif

#endif
