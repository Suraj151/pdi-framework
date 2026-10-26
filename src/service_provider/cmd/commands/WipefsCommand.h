/******************************** Wipefs Command ******************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Lists the signatures a disk or partition carries, the way linux wipefs does:
the offset and the name of each filesystem and partition table signature.
wipefs -a <node> erases them, one line per signature with the bytes it held,
leaving the disk blank with no partition table.

Author          : Suraj I.
created Date    : 23rd Oct 2026
******************************************************************************/
#ifndef _WIPEFS_COMMAND_H_
#define _WIPEFS_COMMAND_H_

#include "CommandCommon.h"

#if defined(ENABLE_DRIVER_CORE) && defined(ENABLE_STORAGE_SERVICE) && defined(ENABLE_DEVFS)

#include <interface/pdi/impl/modules/storage/DiskTools.h>

struct WipefsCommand : public CommandBase {

	WipefsCommand(){
		Clear();
		SetCommand(CMD_NAME_WIPEFS);
		setAcceptArgsOptions(true);
		setCmdOptionSeparator(CMD_OPTION_SEPERATOR_SPACE);
	}

	static void RegisterCommand(){
		CommandBase::RegisterCommand(CMD_NAME_WIPEFS, [](void *arg)->void *{
			return pdiutil::safe_new<WipefsCommand>();
		});
	}

	const char* getUsage() const override {
		return RODT_ATTR("wipefs [-a] <node>  list the signatures a disk carries, -a erases them");
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
			return CMD_ERROR_NOTTY;
		}

		bool erase = false;
		pdiutil::string device;
		for( uint8_t i = 0; i < CMD_OPTION_MAX; i++ ){
			const CommandOption *option = &m_options[i];
			if( nullptr == option->optionval || option->optionvalsize <= 0 ){
				break;
			}
			if( 2 == option->optionvalsize && 0 == strncmp(option->optionval, "-a", 2) ){
				erase = true;
			}else if( device.empty() ){
				device = resolveArgPath(option);
			}else{
				return CMD_ERROR_INVAL;
			}
		}
		if( device.empty() ){
			return CMD_ERROR_ARGS_MISSING;
		}

		uint32_t first = 0, span = 0;
		iBlockDriverInterface *disk = DiskTools::open(device.c_str(), first, span);
		pdi_err_t refused = PDI_OK;
		if( nullptr == disk ){
			refused = PDI_ERR_NOT_FOUND;
		}
		else if( !SessionManager::isRoot() ){
			refused = PDI_ERR_PERM;
		}
		else if( erase && DiskTools::inUse(disk) ){
			refused = PDI_ERR_BUSY;
		}
		if( PDI_OK != refused ){
			return reportFailure(device, refused);
		}

		m_terminal->putln();
		bool header = false;
		int16_t result = DiskTools::wipe(disk, first, span, erase, [&](const block_signature_t &sig, const uint8_t *bytes){
			uint32_t offset = sig.m_block * SD_BLOCK_SIZE + sig.m_offset;
			if( erase ){
				writeErased(device, sig, offset, bytes);
				return;
			}
			if( !header ){
				m_terminal->write_pad_ro(RODT_ATTR("DEVICE"), 6, COL_DEVICE);
				m_terminal->write_pad_ro(RODT_ATTR("OFFSET"), 6, COL_OFFSET);
				m_terminal->writeln_ro(RODT_ATTR("TYPE"));
				header = true;
			}
			char hex[9];
			Uint32ToHexString(offset, hex, sizeof(hex));
			pdiutil::string shown = CHARPTR_WRAP("0x");
			shown += hex;
			m_terminal->write_pad(DevFs::nodeOf(device.c_str()), COL_DEVICE);
			m_terminal->write_pad(shown.c_str(), COL_OFFSET);
			m_terminal->writeln(sig.m_name);
		});

		if( result < 0 ){
			return reportFailure(device, (pdi_err_t)result);
		}
		return PDI_OK;
	}

private:

	/**
	 * One line for an erased signature: its size, where it sat and what it held.
	 */
	void writeErased(const pdiutil::string &device, const block_signature_t &sig, uint32_t offset, const uint8_t *bytes){
		char hex[9];
		Uint32ToHexString(offset, hex, sizeof(hex), false, 8);
		m_terminal->write(device.c_str());
		m_terminal->write_ro(RODT_ATTR(": "));
		m_terminal->write((int32_t)sig.m_size);
		m_terminal->write_ro(RODT_ATTR(" bytes were erased at offset 0x"));
		m_terminal->write(hex);
		m_terminal->write_ro(RODT_ATTR(" ("));
		m_terminal->write(sig.m_name);
		m_terminal->write_ro(RODT_ATTR("): "));
		m_terminal->writeln(BytesToMacString(bytes, sig.m_size, ' ', false).c_str());
	}

	/**
	 * Says which node failed and with what code.
	 */
	pdi_err_t reportFailure(const pdiutil::string &device, pdi_err_t code){
		m_terminal->putln();
		writeFailure(RODT_ATTR("wipefs: "), device.c_str(), (int32_t)code);
		m_terminal->putln();
		return CMD_ERROR_FAILED;
	}

	static constexpr uint8_t COL_DEVICE = 14;
	static constexpr uint8_t COL_OFFSET = 10;
};

#endif

#endif
