/******************************** Sfdisk Command ******************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Writes a disk's dos partition table from a script on its input, the way linux
sfdisk does: one "start, size, type, bootable" line per partition, any field
left empty for its default. sfdisk -l [disk] lists the table and
sfdisk --delete <disk> [n] empties one slot, or every slot.

Author          : Suraj I.
created Date    : 23rd Oct 2026
******************************************************************************/
#ifndef _SFDISK_COMMAND_H_
#define _SFDISK_COMMAND_H_

#include "CommandCommon.h"

#if defined(ENABLE_DRIVER_CORE) && defined(ENABLE_STORAGE_SERVICE) && defined(ENABLE_DEVFS)

#include <service_provider/transport/SerialServiceProvider.h>
#include <interface/pdi/impl/modules/storage/DiskTools.h>

struct SfdiskCommand : public CommandBase {

	SfdiskCommand(){
		Clear();
		SetCommand(CMD_NAME_SFDISK);
		setAcceptArgsOptions(true);
		setCmdOptionSeparator(CMD_OPTION_SEPERATOR_SPACE);
	}

	static void RegisterCommand(){
		CommandBase::RegisterCommand(CMD_NAME_SFDISK, [](void *arg)->void *{
			return pdiutil::safe_new<SfdiskCommand>();
		});
	}

	const char* getUsage() const override {
		return RODT_ATTR("sfdisk <disk> < script | -l [disk] | --delete <disk> [n]  write, list or delete partitions");
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

		const CommandOption *lead = &m_options[0];
		if( nullptr == lead->optionval || lead->optionvalsize <= 0 ){
			return CMD_ERROR_ARGS_MISSING;
		}

		m_terminal->putln();
		if( 2 == lead->optionvalsize && 0 == strncmp(lead->optionval, "-l", 2) ){
			return list(hasOption(1) ? resolveArgPath(&m_options[1]) : pdiutil::string());
		}
		pdiutil::string remover = CHARPTR_WRAP("--delete");
		if( remover.size() == (pdiutil::string::size_type)lead->optionvalsize && 0 == strncmp(lead->optionval, remover.c_str(), remover.size()) ){
			if( !hasOption(1) ){
				return CMD_ERROR_ARGS_MISSING;
			}
			return remove(resolveArgPath(&m_options[1]));
		}
		if( hasOption(1) ){
			return CMD_ERROR_INVAL;
		}
		return write(resolveArgPath(lead));
	}

private:

	bool hasOption(uint8_t i) const {
		return nullptr != m_options[i].optionval && m_options[i].optionvalsize > 0;
	}

	/**
	 * The whole disk the node names, ready for root to change; the refusing code
	 * otherwise, invalid for a partition.
	 */
	pdi_err_t openForChange(const pdiutil::string &device, iBlockDriverInterface *&disk){
		uint32_t first = 0, span = 0;
		disk = DiskTools::open(device.c_str(), first, span);
		if( nullptr == disk ){
			return PDI_ERR_NOT_FOUND;
		}
		if( !SessionManager::isRoot() ){
			return PDI_ERR_PERM;
		}
		if( 0 != first ){
			return PDI_ERR_INVALID_ARG;
		}
		return DiskTools::inUse(disk) ? PDI_ERR_BUSY : PDI_OK;
	}

	/**
	 * Reads the script from the input and writes it as the disk's table, only
	 * once every line of it has been taken.
	 */
	pdi_err_t write(const pdiutil::string &device){
		iBlockDriverInterface *disk = nullptr;
		pdi_err_t opened = openForChange(device, disk);
		if( PDI_OK != opened ){
			return reportFailure(device.c_str(), opened);
		}

		block_partition_t parts[BLOCK_PARTITIONS_MAX];
		uint8_t count = 0;
		uint16_t lineno = 0;
		pdi_err_t bad = PDI_OK;
		if( isInputRedirected() ){
			readCommandLines(pdiutil::string(), m_terminal, [&](const pdiutil::string &line)->bool{
				lineno++;
				const char *at = line.c_str();
				while( ' ' == *at || '\t' == *at ) at++;
				if( '\0' == *at || '#' == *at ){
					return true;
				}
				if( count >= BLOCK_PARTITIONS_MAX ){
					bad = PDI_ERR_FULL;
					return false;
				}
				uint32_t from = (0 == count) ? 0 : parts[count - 1].m_start + parts[count - 1].m_blocks;
				bad = blockScriptLine(at, disk->blocks(), from, parts[count]);
				count += (PDI_OK == bad) ? 1 : 0;
				return PDI_OK == bad;
			});
		}

		if( PDI_OK != bad ){
			m_terminal->write_ro(RODT_ATTR("sfdisk: line "));
			m_terminal->write((int32_t)lineno);
			m_terminal->write_ro(RODT_ATTR(" : "));
			m_terminal->write((int32_t)bad);
			m_terminal->putln();
			return CMD_ERROR_FAILED;
		}
		if( 0 == count ){
			m_terminal->writeln_ro(RODT_ATTR("sfdisk: no partitions given"));
			return CMD_ERROR_FAILED;
		}

		bool created = false;
		uint32_t diskid = 0;
		pdi_err_t written = DiskTools::writeTable(disk, parts, count, created, diskid);
		if( PDI_OK != written ){
			return reportFailure(device.c_str(), written);
		}

		if( created ){
			m_terminal->write_ro(RODT_ATTR("Created a new DOS disklabel with disk identifier 0x"));
			writeId(diskid);
			m_terminal->writeln(".");
		}
		for( uint8_t i = 0; i < count; i++ ){
			char size[16];
			BytesToSizeString((uint64_t)parts[i].m_blocks * SD_BLOCK_SIZE, false, size, sizeof(size));
			m_terminal->write(device.c_str());
			m_terminal->write("p");
			m_terminal->write((int32_t)(i + 1));
			m_terminal->write_ro(RODT_ATTR(": Created a new partition "));
			m_terminal->write((int32_t)(i + 1));
			m_terminal->write_ro(RODT_ATTR(" of type '"));
			m_terminal->write_ro(blockPartitionTypeName(parts[i].m_id));
			m_terminal->write_ro(RODT_ATTR("' and of size "));
			m_terminal->write(size);
			m_terminal->writeln(".");
		}
		m_terminal->writeln_ro(RODT_ATTR("The partition table has been altered."));
		return PDI_OK;
	}

	/**
	 * Empties the slot the third argument numbers, or every slot without one.
	 */
	pdi_err_t remove(const pdiutil::string &device){
		uint8_t number = 0;
		if( hasOption(2) ){
			const CommandOption *n = &m_options[2];
			if( 1 != n->optionvalsize || n->optionval[0] < '1' || n->optionval[0] > ('0' + BLOCK_PARTITIONS_MAX) ){
				return reportFailure(device.c_str(), PDI_ERR_INVALID_ARG);
			}
			number = (uint8_t)(n->optionval[0] - '0');
		}

		iBlockDriverInterface *disk = nullptr;
		pdi_err_t result = openForChange(device, disk);
		if( PDI_OK == result ){
			result = DiskTools::deletePartition(disk, number);
		}
		if( PDI_OK != result ){
			return reportFailure(device.c_str(), result);
		}
		m_terminal->writeln_ro(RODT_ATTR("The partition table has been altered."));
		return PDI_OK;
	}

	/**
	 * Lists the disk the node names, or every disk when none is named.
	 */
	pdi_err_t list(const pdiutil::string &device){
		if( !device.empty() ){
			uint32_t first = 0, span = 0;
			iBlockDriverInterface *disk = DiskTools::open(device.c_str(), first, span);
			if( nullptr == disk ){
				return reportFailure(device.c_str(), PDI_ERR_NOT_FOUND);
			}
			listDisk(device, disk, first, span);
			return PDI_OK;
		}

		DeviceManager *mgr = __serial_service.devices();
		pdiutil::string node;
		bool listed = false;
		for( uint16_t i = 0; nullptr != mgr && i < mgr->count(); i++ ){
			const driver_device_t *dev = mgr->at(i);
			iBlockDriverInterface *disk = iBlockDriverInterface::diskOf(dev);
			if( nullptr == disk || !disk->nodeName(node) ){
				continue;
			}
			if( listed ){
				m_terminal->putln();
			}
			pdiutil::string path = DevFs::pathOf(node.c_str());
			listDisk(path, disk, 0, disk->blocks());
			listed = true;
		}
		return PDI_OK;
	}

	/**
	 * The disk's size and, for a whole disk with a table, its label and the
	 * partitions it lists, the way fdisk -l prints them.
	 */
	void listDisk(const pdiutil::string &device, iBlockDriverInterface *disk, uint32_t first, uint32_t span){
		char text[24];
		uint64_t bytes = (uint64_t)span * SD_BLOCK_SIZE;
		BytesToSizeString(bytes, false, text, sizeof(text));
		m_terminal->write_ro(RODT_ATTR("Disk "));
		m_terminal->write(device.c_str());
		m_terminal->write_ro(RODT_ATTR(": "));
		m_terminal->write(text);
		m_terminal->write_ro(RODT_ATTR(", "));
		Int64ToString((int64_t)bytes, text, sizeof(text), 0);
		m_terminal->write(text);
		m_terminal->write_ro(RODT_ATTR(" bytes, "));
		Uint32ToString(span, text, sizeof(text));
		m_terminal->write(text);
		m_terminal->writeln_ro(RODT_ATTR(" sectors"));
		m_terminal->writeln_ro(RODT_ATTR("Units: sectors of 1 * 512 = 512 bytes"));

		uint8_t table = disk->tableType();
		if( 0 != first || BLOCK_TABLE_NONE == table ){
			return;
		}
		m_terminal->write_ro(RODT_ATTR("Disklabel type: "));
		m_terminal->writeln(blockTableName(table).c_str());
		uint32_t diskid = 0;
		if( BLOCK_TABLE_DOS == table && PDI_OK == DiskTools::diskId(disk, diskid) ){
			m_terminal->write_ro(RODT_ATTR("Disk identifier: 0x"));
			writeId(diskid);
			m_terminal->putln();
		}

		bool header = false;
		for( uint8_t n = 1; n <= disk->partitionCount(); n++ ){
			const block_partition_t *part = disk->partition(n);
			if( nullptr == part ){
				continue;
			}
			uint8_t width = (uint8_t)(device.size() + 3);
			if( !header ){
				m_terminal->putln();
				m_terminal->write_pad_ro(RODT_ATTR("Device"), 6, width);
				m_terminal->write_ro(RODT_ATTR("Boot"));
				m_terminal->write_pad_ro(RODT_ATTR("Start"), 5, COL_NUMBER, true);
				m_terminal->write_pad_ro(RODT_ATTR("End"), 3, COL_NUMBER, true);
				m_terminal->write_pad_ro(RODT_ATTR("Sectors"), 7, COL_NUMBER, true);
				m_terminal->write_pad_ro(RODT_ATTR("Size"), 4, COL_SIZE, true);
				m_terminal->write_pad_ro(RODT_ATTR("Id"), 2, COL_ID, true);
				m_terminal->writeln_ro(RODT_ATTR(" Type"));
				header = true;
			}
			pdiutil::string name = blockPartitionName(device.c_str(), n);
			m_terminal->write_pad(name.c_str(), width);
			m_terminal->write_pad(part->m_boot ? "*" : "", 4);
			writeNumber(part->m_start, COL_NUMBER);
			writeNumber(part->m_start + part->m_blocks - 1, COL_NUMBER);
			writeNumber(part->m_blocks, COL_NUMBER);
			BytesToSizeString((uint64_t)part->m_blocks * SD_BLOCK_SIZE, true, text, sizeof(text));
			m_terminal->write_pad(text, COL_SIZE, true);
			Uint32ToHexString(part->m_id, text, sizeof(text));
			m_terminal->write_pad(text, COL_ID, true);
			m_terminal->write(" ");
			m_terminal->writeln_ro(blockPartitionTypeName(part->m_id));
		}
	}

	void writeNumber(uint32_t value, uint8_t width){
		char digits[12];
		Uint32ToString(value, digits, sizeof(digits));
		m_terminal->write_pad(digits, width, true);
	}

	/**
	 * A disk identifier as eight hex digits.
	 */
	void writeId(uint32_t id){
		char hex[9];
		Uint32ToHexString(id, hex, sizeof(hex), false, 8);
		m_terminal->write(hex);
	}

	/**
	 * Says which node failed and with what code.
	 */
	pdi_err_t reportFailure(const char *device, pdi_err_t code){
		writeFailure(RODT_ATTR("sfdisk: "), device, (int32_t)code);
		m_terminal->putln();
		return CMD_ERROR_FAILED;
	}

	static constexpr uint8_t COL_NUMBER = 9;
	static constexpr uint8_t COL_SIZE = 6;
	static constexpr uint8_t COL_ID = 3;
};

#endif

#endif
