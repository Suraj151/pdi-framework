/********************************** Dd Command ********************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Copies blocks from one file to another, at the block offsets given, the way
linux dd does: dd if=<file> of=<file> [bs=] [count=] [skip=] [seek=]. A
regular output file is written through a handle, replaced unless a seek is
given; a device node is written in place at the offset.

Author          : Suraj I.
created Date    : 21st Oct 2026
******************************************************************************/
#ifndef _DD_COMMAND_H_
#define _DD_COMMAND_H_

#include "CommandCommon.h"

#ifdef ENABLE_STORAGE_SERVICE

struct DdCommand : public CommandBase {

	DdCommand() : m_handle(PDI_ERR_NOT_SUPPORTED), m_written(0), m_error(0), m_buf(nullptr), m_filled(0){
		Clear();
		SetCommand(CMD_NAME_DD);
		setAcceptArgsOptions(true);
		setCmdOptionSeparator(CMD_OPTION_SEPERATOR_SPACE);
	}

	/**
	 * Lets go of the buffer and the output handle if a run left them.
	 */
	~DdCommand(){
		finish();
	}

	static void RegisterCommand(){
		CommandBase::RegisterCommand(CMD_NAME_DD, [](void *arg)->void *{
			return pdiutil::safe_new<DdCommand>();
		});
	}

	const char* getUsage() const override {
		return RODT_ATTR("dd if=<file> of=<file> [bs=512] [count=N] [skip=N] [seek=N]  copy blocks between files and devices");
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

		if( m_argsoverflow ){
			m_terminal->putln();
			m_terminal->writeln_ro(RODT_ATTR("too many arguments"));
			return CMD_ERROR_INVAL;
		}

		pdi_err_t parsed = parseOperands();
		if( PDI_OK != parsed ){
			return parsed;
		}

		if( !__i_fs.isFileExist(m_in.c_str()) ){
			reportFailure(m_in, PDI_ERR_NOT_FOUND);
			return CMD_ERROR_FAILED;
		}

		const vfs_mount_t *inmount = __i_fs.findMountForPath(m_in.c_str());
		bool stream = (nullptr != inmount && VFS_TYPE_DEVFS == inmount->m_type && 0 == __i_fs.getFileSize(m_in.c_str()));
		if( stream && !m_counted ){
			m_terminal->putln();
			m_terminal->write(m_in.c_str());
			m_terminal->writeln_ro(RODT_ATTR(" never ends, give count="));
			return CMD_ERROR_INVAL;
		}

		pdi_err_t opened = openOutput();
		if( PDI_OK != opened ){
			reportFailure(m_out, opened);
			return CMD_ERROR_FAILED;
		}

		m_buf = pdiutil::safe_new_array<uint8_t>(m_bs);
		if( nullptr == m_buf ){
			finish();
			reportFailure(m_out, PDI_ERR_NO_MEM);
			return CMD_ERROR_FAILED;
		}

		uint32_t fullin = 0, partin = 0, fullout = 0, partout = 0;
		bool limited = m_counted;
		uint64_t offset = stream ? 0 : (uint64_t)m_skip * m_bs;
		int got = 0;
		bool stopped = false;

		do{
			got = __i_fs.readFile(m_in.c_str(), m_bs, [&](char *data, uint32_t size)->bool{
				uint32_t taken = 0;
				while( taken < size ){
					if( limited && fullin >= m_count ){
						return false;
					}
					uint32_t n = pdistd::min(size - taken, m_bs - m_filled);
					memcpy(m_buf + m_filled, data + taken, n);
					m_filled += n;
					taken += n;
					if( m_filled < m_bs ){
						continue;
					}
					fullin++;
					if( !flushRecord() ){
						return false;
					}
					fullout++;
				}
				stopped = commandStopped(m_terminal);
				return !(limited && fullin >= m_count) && !stopped;
			}, offset);
			offset += stream ? 0 : (uint64_t)((got > 0) ? got : 0);
			__i_dvc_ctrl.yield();
		}while( got > 0 && 0 == m_error && !stopped && !(limited && fullin >= m_count) );

		if( got < 0 && 0 == m_error ){
			finish();
			reportFailure(m_in, (pdi_err_t)got);
			return CMD_ERROR_FAILED;
		}

		if( 0 == m_error && !stopped && m_filled > 0 && !(limited && fullin >= m_count) ){
			partin = 1;
			if( flushRecord() ){
				partout = 1;
			}
		}

		finish();

		if( 0 != m_error ){
			reportFailure(m_out, m_error);
		}

		m_terminal->putln();
		writeRecords(fullin, partin, RODT_ATTR(" records in"));
		writeRecords(fullout, partout, RODT_ATTR(" records out"));
		char bytes[24];
		Int64ToString((int64_t)m_written, bytes, sizeof(bytes), 0);
		m_terminal->write(bytes);
		m_terminal->writeln_ro(RODT_ATTR(" bytes copied"));

		return (0 != m_error) ? (pdi_err_t)CMD_ERROR_FAILED : stoppedResult(PDI_OK);
	}

private:

	pdiutil::string m_in;
	pdiutil::string m_out;
	uint32_t m_bs;
	uint32_t m_count;
	uint32_t m_skip;
	uint32_t m_seek;
	bool m_counted;
	pdi_fhandle_t m_handle;
	uint64_t m_written;
	pdi_err_t m_error;
	uint8_t *m_buf;
	uint32_t m_filled;

	/**
	 * A whole number of up to DD_NUMBER_DIGITS_MAX digits, false for anything else.
	 */
	static bool readNumber(const char *span, int16_t len, uint32_t &value){
		if( len <= 0 || len > DD_NUMBER_DIGITS_MAX ){
			return false;
		}
		return ParseUint32(span, (uint16_t)len, value);
	}

	/**
	 * Takes each key=value operand in: if and of are needed, bs is 1 to DD_BLOCK_MAX,
	 * count, skip and seek are numbers.
	 */
	pdi_err_t parseOperands(){

		m_in.clear();
		m_out.clear();
		m_bs = DD_BLOCK_DEFAULT;
		m_count = 0;
		m_skip = 0;
		m_seek = 0;
		m_counted = false;

		pdiutil::string inkey = CHARPTR_WRAP(DD_OPERAND_IF), outkey = CHARPTR_WRAP(DD_OPERAND_OF);
		pdiutil::string bskey = CHARPTR_WRAP(DD_OPERAND_BS), countkey = CHARPTR_WRAP(DD_OPERAND_COUNT);
		pdiutil::string skipkey = CHARPTR_WRAP(DD_OPERAND_SKIP), seekkey = CHARPTR_WRAP(DD_OPERAND_SEEK);

		for( uint8_t i = 0; i < CMD_OPTION_MAX; i++ ){

			const char *operand = m_options[i].optionval;
			int16_t len = m_options[i].optionvalsize;
			if( nullptr == operand || len <= 0 ){
				break;
			}

			int16_t eq = 0;
			while( eq < len && '=' != operand[eq] ) eq++;
			if( eq == 0 || eq >= len ){
				return badOperand(operand, len);
			}

			pdiutil::string key(operand, (pdiutil::string::size_type)eq);
			const char *value = operand + eq + 1;
			int16_t valuelen = (int16_t)(len - eq - 1);
			bool good = true;

			if( key == inkey ){
				m_in = resolveArgPathStr(value, valuelen);
				good = !m_in.empty();
			}else if( key == outkey ){
				m_out = resolveArgPathStr(value, valuelen);
				good = !m_out.empty();
			}else if( key == bskey ){
				good = readNumber(value, valuelen, m_bs) && m_bs > 0 && m_bs <= DD_BLOCK_MAX;
			}else if( key == countkey ){
				good = readNumber(value, valuelen, m_count);
				m_counted = good;
			}else if( key == skipkey ){
				good = readNumber(value, valuelen, m_skip);
			}else if( key == seekkey ){
				good = readNumber(value, valuelen, m_seek);
			}else{
				good = false;
			}

			if( !good ){
				return badOperand(operand, len);
			}
		}

		if( m_in.empty() || m_out.empty() ){
			m_terminal->putln();
			m_terminal->writeln_ro(RODT_ATTR("dd needs if=<file> and of=<file>"));
			return CMD_ERROR_ARGS_MISSING;
		}
		return PDI_OK;
	}

	/**
	 * Says which operand could not be taken.
	 */
	pdi_err_t badOperand(const char *operand, int16_t len){
		pdiutil::string shown(operand, (pdiutil::string::size_type)len);
		m_terminal->putln();
		m_terminal->write_ro(RODT_ATTR("dd: bad operand "));
		m_terminal->writeln(shown.c_str());
		return CMD_ERROR_INVAL;
	}

	/**
	 * Opens a regular output file through a handle at the seek offset, replaced with
	 * no seek; other nodes are written in place.
	 */
	pdi_err_t openOutput(){
		m_written = 0;
		m_error = 0;
		m_filled = 0;
		uint8_t flags = FILE_OPEN_WRITE | FILE_OPEN_CREATE | ((0 == m_seek) ? FILE_OPEN_TRUNCATE : 0);
		m_handle = __i_fs.openFile(m_out.c_str(), flags);
		if( PDI_ERR_NOT_SUPPORTED == m_handle ){
			return prepareInPlace();
		}
		if( m_handle < 0 ){
			pdi_err_t failed = (pdi_err_t)m_handle;
			m_handle = PDI_ERR_NOT_SUPPORTED;
			return failed;
		}
		int64_t at = __i_fs.seekFile(m_handle, (int64_t)m_seek * m_bs, FILE_SEEK_SET);
		return (at < 0) ? (pdi_err_t)at : PDI_OK;
	}

	/**
	 * Readies a handleless output: creates a missing file, empties it unless seeking,
	 * and leaves /dev, /sys and /proc as is.
	 */
	pdi_err_t prepareInPlace(){
		if( !__i_fs.isFileExist(m_out.c_str()) ){
			int made = __i_fs.writeFile(m_out.c_str(), "", 0, false);
			return (made < 0) ? (pdi_err_t)made : PDI_OK;
		}
		const vfs_mount_t *outmount = __i_fs.findMountForPath(m_out.c_str());
		bool node = ( nullptr != outmount && ( VFS_TYPE_DEVFS == outmount->m_type ||
		              VFS_TYPE_SYSFS == outmount->m_type || VFS_TYPE_PROCFS == outmount->m_type ) );
		if( 0 != m_seek || node ){
			return PDI_OK;
		}
		int emptied = __i_fs.writeFile(m_out.c_str(), "", 0, false);
		return (emptied < 0) ? (pdi_err_t)emptied : PDI_OK;
	}

	/**
	 * Writes the buffered record out and yields, false when the write failed.
	 */
	bool flushRecord(){
		int stored = 0;
		if( m_handle >= 0 ){
			stored = __i_fs.writeFileHandle(m_handle, (const char *)m_buf, m_filled);
		}else{
			stored = __i_fs.editFile(m_out.c_str(), (uint64_t)m_seek * m_bs + m_written, (const char *)m_buf, m_filled);
		}
		if( stored < 0 || (uint32_t)stored != m_filled ){
			m_error = (stored < 0) ? (pdi_err_t)stored : PDI_ERR_IO;
			return false;
		}
		m_written += m_filled;
		m_filled = 0;
		__i_dvc_ctrl.yield();
		return true;
	}

	/**
	 * Closes the output handle and frees the buffer.
	 */
	void finish(){
		if( m_handle >= 0 ){
			__i_fs.closeFile(m_handle);
		}
		m_handle = PDI_ERR_NOT_SUPPORTED;
		pdiutil::safe_delete_array(m_buf);
	}

	/**
	 * Says which file failed and with what code.
	 */
	void reportFailure(const pdiutil::string &path, pdi_err_t code){
		m_terminal->putln();
		writeFailure(RODT_ATTR("dd: "), path.c_str(), (int32_t)code);
		m_terminal->putln();
	}

	/**
	 * One summary line: whole records, a plus sign, part records.
	 */
	void writeRecords(uint32_t whole, uint32_t part, const char *label){
		if( nullptr == label ){
			return;
		}
		m_terminal->write((int32_t)whole);
		m_terminal->write("+");
		m_terminal->write((int32_t)part);
		m_terminal->writeln_ro(label);
	}
};

#endif

#endif
