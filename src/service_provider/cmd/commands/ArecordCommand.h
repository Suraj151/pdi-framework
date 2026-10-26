/****************************** Arecord Command *******************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Records from the first capture device bound into a wav file for as many seconds
as asked, or until interrupted.

Author          : Suraj I.
created Date    : 19th Oct 2026
******************************************************************************/
#ifndef _ARECORD_COMMAND_H_
#define _ARECORD_COMMAND_H_

#include "CommandCommon.h"

#if defined(ENABLE_I2S_BUS) && defined(ENABLE_STORAGE_SERVICE)

#include <service_provider/transport/SerialServiceProvider.h>
#include <interface/pdi/modules/driver/iSoundDriverInterface.h>

struct ArecordCommand : public CommandBase {

	ArecordCommand() : m_taskid(-1), m_file(-1), m_buf(nullptr), m_target(0), m_done(0), m_synced(0),
	                   m_error(0), m_rate(0), m_bits(0), m_channels(0) {
		Clear();
		SetCommand(CMD_NAME_ARECORD);
		setAcceptArgsOptions(true);
		setCmdOptionSeparator(CMD_OPTION_SEPERATOR_SPACE);
	}

	/**
	 * Closes a recording still open, so the file keeps a header that matches it.
	 */
	~ArecordCommand(){
		finish(nullptr, false);
	}

	static void RegisterCommand(){
		CommandBase::RegisterCommand(CMD_NAME_ARECORD, [](void *arg)->void *{
			return pdiutil::safe_new<ArecordCommand>();
		});
	}

	const char* getUsage() const override {
		return RODT_ATTR("arecord -d <seconds> <file>  record the capture device into a wav file; Ctrl+C to stop");
	}

#ifdef ENABLE_AUTH_SERVICE
	bool needauth() override { return true; }
#endif

	/**
	 * Interrupting a recording keeps what was taken.
	 */
	bool stopRunningInBackground() override {
		finish(nullptr, true);
		return CommandBase::stopRunningInBackground();
	}

	pdi_err_t execute(cmd_term_inseq_t terminputaction){

#ifdef ENABLE_AUTH_SERVICE
		if( needauth() && !__auth_service.getAuthorized() ){
			return CMD_ERROR_PERM;
		}
#endif

		if( nullptr == m_terminal ){
			return CMD_ERROR_NOTTY;
		}

		CommandOption *flag = &m_options[0];
		CommandOption *secs = &m_options[1];
		CommandOption *file = &m_options[2];

		if( nullptr == flag->optionval || 2 != flag->optionvalsize || 0 != strncmp(flag->optionval, "-d", 2) ||
		    nullptr == secs->optionval || 0 >= secs->optionvalsize ||
		    nullptr == file->optionval || 0 >= file->optionvalsize ){
			return CMD_ERROR_ARGS_MISSING;
		}

		uint32_t seconds = StringToUint32(secs->optionval, secs->optionvalsize);
		if( 0 == seconds ){
			return CMD_ERROR_INVAL;
		}

		iSoundDriverInterface *snd = captureDevice();
		if( nullptr == snd ){
			m_terminal->putln();
			m_terminal->writeln_ro(RODT_ATTR("no capture device"));
			return CMD_ERROR_FAILED;
		}

		m_rate = snd->rate();
		m_bits = snd->bits();
		m_channels = snd->channels();

		uint64_t target = (uint64_t)seconds * m_rate * (m_bits / 8) * m_channels;
		if( 0 == target || target > (uint64_t)(0xFFFFFFFFUL - SOUND_WAV_HEADER_SIZE) ){
			return CMD_ERROR_INVAL;
		}

		finish(nullptr, false);
		m_target = (uint32_t)target;
		m_done = 0;
		m_synced = 0;
		m_error = 0;
		m_path = resolveArgPath(file);

		uint64_t available = __i_fs.getFreeSize(m_path.c_str());
		if( __i_fs.isFileExist(m_path.c_str()) ){
			int64_t existing = __i_fs.getFileSize(m_path.c_str());
			available += (existing > 0) ? (uint64_t)existing : 0;
		}
		uint64_t needed = (uint64_t)SOUND_WAV_HEADER_SIZE + m_target + SOUND_CAPTURE_SPACE_RESERVE;
		if( needed > available ){
			m_terminal->putln();
			m_terminal->write_ro(RODT_ATTR("not enough space: needs "));
			m_terminal->write((int32_t)needed);
			m_terminal->write_ro(RODT_ATTR(" bytes, "));
			m_terminal->write((int32_t)available);
			m_terminal->writeln_ro(RODT_ATTR(" free"));
			return CMD_ERROR_FAILED;
		}

		m_file = __i_fs.openFile(m_path.c_str(), FILE_OPEN_WRITE | FILE_OPEN_CREATE | FILE_OPEN_TRUNCATE);
		if( m_file < 0 ){
			m_terminal->putln();
			m_terminal->write_ro(RODT_ATTR("cannot open "));
			m_terminal->writeln(m_path.c_str());
			m_file = -1;
			return CMD_ERROR_FAILED;
		}

		m_buf = pdiutil::safe_new_array<uint8_t>(SOUND_CAPTURE_CHUNK);
		if( nullptr == m_buf || !writeHeader(m_target) ){
			finish(nullptr, false);
			__i_fs.deleteFile(m_path.c_str());
			return CMD_ERROR_FAILED;
		}

		m_taskid = __task_scheduler.register_task( [&]() {
			runAsOwner(this, [&](){
				capture();
			});
		}, SOUND_CAPTURE_INTERVAL_MS, DEFAULT_TASK_PRIORITY, __i_dvc_ctrl.millis_now(), -1, CMD_NAME_ARECORD );

		if( m_taskid < 0 ){
			finish(nullptr, false);
			__i_fs.deleteFile(m_path.c_str());
			return CMD_ERROR_FAILED;
		}

		m_terminal->putln();
		m_terminal->write_ro(RODT_ATTR("recording "));
		m_terminal->writeln(m_path.c_str());
		m_runinbackground = true;
		return PDI_OK;
	}

private:

	int32_t m_taskid;
	pdi_fhandle_t m_file;
	uint8_t *m_buf;
	uint32_t m_target;
	uint32_t m_done;
	uint32_t m_synced;
	int32_t m_error;
	uint32_t m_rate;
	uint8_t m_bits;
	uint8_t m_channels;
	pdiutil::string m_path;

	/**
	 * The first capture device bound, looked up afresh each time since a scan
	 * may let it go.
	 */
	static iSoundDriverInterface *captureDevice(){
		DeviceManager *mgr = __serial_service.devices();
		iDriverInterface *drv = (nullptr == mgr) ? nullptr : mgr->bound(DRIVER_CLASS_SOUND_CAPTURE);
		return static_cast<iSoundDriverInterface *>(drv);
	}

	/**
	 * One pass: takes what the device holds, up to what is still wanted, and
	 * ends the recording once it has it all.
	 */
	void capture(){

		iSoundDriverInterface *snd = captureDevice();
		if( nullptr == snd ){
			finish(RODT_ATTR("capture device gone"), true);
			return;
		}

		uint32_t want = pdistd::min((uint32_t)SOUND_CAPTURE_CHUNK, m_target - m_done);
		int32_t got = snd->readNode(m_buf, want);
		if( got < 0 ){
			m_error = got;
			finish(RODT_ATTR("capture device failed"), true);
			return;
		}

		if( got > 0 ){
			int written = __i_fs.writeFileHandle(m_file, (const char *)m_buf, (uint32_t)got);
			if( written != got ){
				m_error = (written < 0) ? written : PDI_ERR_FAILURE;
				finish(RODT_ATTR("write failed"), true);
				return;
			}
			m_done += (uint32_t)written;

			if( m_done - m_synced >= SOUND_CAPTURE_SYNC_BYTES ){
				pdi_err_t synced = __i_fs.syncFile(m_file);
				if( synced < 0 ){
					m_error = synced;
					finish(RODT_ATTR("write failed"), true);
					return;
				}
				m_synced = m_done;
			}
		}

		if( m_done >= m_target ){
			finish(nullptr, true);
		}
	}

	/**
	 * Writes a value least significant byte first.
	 */
	static void putLe(uint8_t *at, uint32_t value, uint8_t bytes){
		for( uint8_t i = 0; i < bytes; i++ ){
			at[i] = (uint8_t)(value >> (8 * i));
		}
	}

	/**
	 * Writes the riff header for a data size at the start of the file, leaving
	 * the handle at its end.
	 */
	bool writeHeader(uint32_t datasize){

		uint8_t hdr[SOUND_WAV_HEADER_SIZE];
		uint16_t align = (uint16_t)(m_channels * (m_bits / 8));
		pdiutil::string riff = CHARPTR_WRAP(SOUND_WAV_RIFF), wave = CHARPTR_WRAP(SOUND_WAV_WAVE);
		pdiutil::string fmt = CHARPTR_WRAP(SOUND_WAV_FMT), data = CHARPTR_WRAP(SOUND_WAV_DATA);

		memcpy(hdr, riff.c_str(), 4);
		putLe(hdr + 4, SOUND_WAV_HEADER_SIZE - 8 + datasize, 4);
		memcpy(hdr + 8, wave.c_str(), 4);
		memcpy(hdr + 12, fmt.c_str(), 4);
		putLe(hdr + 16, 16, 4);
		putLe(hdr + 20, 1, 2);
		putLe(hdr + 22, m_channels, 2);
		putLe(hdr + 24, m_rate, 4);
		putLe(hdr + 28, m_rate * align, 4);
		putLe(hdr + 32, align, 2);
		putLe(hdr + 34, m_bits, 2);
		memcpy(hdr + 36, data.c_str(), 4);
		putLe(hdr + 40, datasize, 4);

		if( __i_fs.seekFile(m_file, 0, FILE_SEEK_SET) < 0 ||
		    SOUND_WAV_HEADER_SIZE != __i_fs.writeFileHandle(m_file, (const char *)hdr, SOUND_WAV_HEADER_SIZE) ){
			return false;
		}
		return __i_fs.seekFile(m_file, 0, FILE_SEEK_END) >= 0;
	}

	/**
	 * Stops sampling, fixes the header to the samples kept, closes the file and, when
	 * asked, says why it stopped.
	 */
	void finish(const char *reason, bool report){

		if( m_taskid >= 0 ){
			__task_scheduler.remove_task(m_taskid);
			m_taskid = -1;
		}

		if( m_file >= 0 ){

			__i_fs.syncFile(m_file);
			bool headed = (m_done == m_target) || writeHeader(m_done);
			pdi_err_t closed = __i_fs.closeFile(m_file);
			m_file = -1;

			int64_t size = __i_fs.getFileSize(m_path.c_str());
			int64_t kept = (size > SOUND_WAV_HEADER_SIZE) ? size - SOUND_WAV_HEADER_SIZE : 0;
			if( 0 == kept ){
				__i_fs.deleteFile(m_path.c_str());
			}

			if( report && nullptr != m_terminal ){
				m_terminal->putln();
				if( nullptr != reason ){
					m_terminal->write_ro(reason);
					if( 0 != m_error ){
						m_terminal->write_ro(RODT_ATTR(" : "));
						m_terminal->write((int32_t)m_error);
					}
					m_terminal->putln();
				}
				if( closed < 0 || !headed ){
					m_terminal->write_ro(RODT_ATTR("could not finish the header : "));
					m_terminal->writeln((int32_t)((closed < 0) ? closed : PDI_ERR_FAILURE));
				}
				if( 0 == kept ){
					m_terminal->writeln_ro(RODT_ATTR("nothing was saved"));
				}else{
					m_terminal->write_ro(RODT_ATTR("recorded "));
					m_terminal->write((int32_t)kept);
					m_terminal->write_ro(RODT_ATTR(" bytes to "));
					m_terminal->writeln(m_path.c_str());
				}
				m_terminal->commit();
			}
		}

		pdiutil::safe_delete_array(m_buf);
		m_runinbackground = false;
	}
};

#endif

#endif
