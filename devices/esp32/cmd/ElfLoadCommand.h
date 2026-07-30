/******************************** ElfLoad Command ******************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 18th July 2026
******************************************************************************/
#ifndef _ELF_LOAD_COMMAND_H_
#define _ELF_LOAD_COMMAND_H_

#include <service_provider/cmd/commands/CommandCommon.h>

#if defined(ENABLE_PROGRAM_EXEC)

#include "../elf_loader/elf_loader.h"
#include "../threading/Preemptive.h"

#define CMD_NAME_ELFLOAD		    "elfload"
#define ELF_EXEC_STACK_SIZE		    (8 * 1024)

/**
 * elfload command
 *
 * Loads a relocatable ELF from the filesystem into RAM and starts it as a
 * background preemptive task. The command returns immediately; the program
 * runs until it exits on its own or is stopped with `kill`. STOP/CONT do not
 * apply to it.
 *   elfload /apps/hello
 */
struct ElfLoadCommand : public CommandBase {

	ElfLoadCommand(){
		Clear();
		SetCommand(CMD_NAME_ELFLOAD);
		setAcceptArgsOptions(true);
	}

	static void RegisterCommand(){
		CommandBase::RegisterCommand(CMD_NAME_ELFLOAD, [](void *arg)->void *{
			return new ElfLoadCommand();
		});
	}

	const char* getUsage() const override {
		return RODT_ATTR("elfload  <elf file path> to load and run in background");
	}

#ifdef ENABLE_AUTH_SERVICE
	bool needauth() override { return true; }
#endif

	cmd_result_t execute(cmd_term_inseq_t terminputaction){

#ifdef ENABLE_AUTH_SERVICE
		if( needauth() && !__auth_service.getAuthorized() ){
			return CMD_RESULT_NEED_AUTH;
		}
#endif

		if( nullptr == m_terminal ) return CMD_RESULT_TERMINAL_ERR;

		CommandOption *cmdoptn = &m_options[0];
		pdiutil::string elffile = resolveArgPath(cmdoptn);

		if( elffile.empty() ){
			m_terminal->putln();
			m_terminal->write_ro(RODT_ATTR("Usage: "));
			m_terminal->write_ro(getUsage());
			return CMD_RESULT_ARGS_ERROR;
		}

		if( !__i_fs.isFileExist(elffile.c_str()) ){
			m_terminal->putln();
			m_terminal->write_ro(RODT_ATTR("No such file: "));
			m_terminal->write(elffile.c_str());
			return CMD_RESULT_FAILED;
		}

		int64_t fsize = __i_fs.getFileSize(elffile.c_str());
		if( fsize <= 0 ){
			m_terminal->putln();
			m_terminal->write_ro(RODT_ATTR("Empty or unreadable file: "));
			m_terminal->write(elffile.c_str());
			return CMD_RESULT_FAILED;
		}

		uint8_t *payload = (uint8_t *)esp_elf_malloc((uint32_t)fsize, false);
		if( nullptr == payload ){
			m_terminal->putln();
			m_terminal->write_ro(RODT_ATTR("Out of memory: need "));
			m_terminal->write((uint32_t)fsize);
			m_terminal->write_ro(RODT_ATTR(" bytes, free heap "));
			m_terminal->write((uint32_t)__i_dvc_ctrl.get_free_heap());
			return CMD_RESULT_FAILED;
		}

		uint32_t pos = 0;
		int rd = __i_fs.readFile(elffile.c_str(), 512, [&](char* data, uint32_t size)->bool{
			if( pos + size > (uint32_t)fsize ) size = (uint32_t)fsize - pos;
			memcpy(payload + pos, data, size);
			pos += size;
			return pos < (uint32_t)fsize;
		});

		if( rd < 0 || pos != (uint32_t)fsize ){
			esp_elf_free(payload);
			m_terminal->putln();
			m_terminal->write_ro(RODT_ATTR("Failed to read ELF file"));
			return CMD_RESULT_FAILED;
		}

		if( fsize < 4 || payload[0] != 0x7F || payload[1] != 'E' ||
			payload[2] != 'L' || payload[3] != 'F' ){
			esp_elf_free(payload);
			m_terminal->putln();
			m_terminal->write_ro(RODT_ATTR("Not an ELF file: "));
			m_terminal->write(elffile.c_str());
			return CMD_RESULT_FAILED;
		}

		esp_elf_t *elf = (esp_elf_t *)esp_elf_malloc(sizeof(esp_elf_t), false);
		if( nullptr == elf ){
			esp_elf_free(payload);
			m_terminal->putln();
			m_terminal->write_ro(RODT_ATTR("Out of memory for ELF object"));
			return CMD_RESULT_FAILED;
		}
		memset(elf, 0, sizeof(esp_elf_t));

		int ret = esp_elf_init(elf);
		if( ret < 0 ){
			esp_elf_free(elf);
			esp_elf_free(payload);
			m_terminal->putln();
			m_terminal->write_ro(RODT_ATTR("Failed to init ELF object, err: "));
			m_terminal->write((int32_t)ret);
			return CMD_RESULT_FAILED;
		}

		ret = esp_elf_relocate(elf, payload);
		esp_elf_free(payload);
		if( ret < 0 ){
			esp_elf_deinit(elf);
			esp_elf_free(elf);
			m_terminal->putln();
			m_terminal->write_ro(RODT_ATTR("Failed to relocate ELF, err: "));
			m_terminal->write((int32_t)ret);
			return CMD_RESULT_FAILED;
		}

		pdiutil::task_id_t pid = __task_scheduler.register_task([elf]() {
			esp_elf_request(elf, 0, 0, nullptr);
		});

		if( pid < 0 ){
			esp_elf_deinit(elf);
			esp_elf_free(elf);
			m_terminal->putln();
			m_terminal->write_ro(RODT_ATTR("Failed to create task"));
			return CMD_RESULT_FAILED;
		}

		int sret = __task_scheduler.scheduleUnderExecSched(&__i_preemptive_scheduler, pid, TASK_MODE_PREEMPTIVE, ELF_EXEC_STACK_SIZE);
		if( sret != 0 ){
			__task_scheduler.remove_task(pid);
			esp_elf_deinit(elf);
			esp_elf_free(elf);
			m_terminal->putln();
			m_terminal->write_ro(RODT_ATTR("Failed to start task, err: "));
			m_terminal->write((int32_t)sret);
			return CMD_RESULT_FAILED;
		}

		task_t *t = __task_scheduler.get_task(pid);
		if( nullptr != t ){
			t->m_stoppable = false;
			t->m_finalizer = [elf]() {
				esp_elf_deinit(elf);
				esp_elf_free(elf);
			};
		}
		__task_scheduler.setTaskName(pid, RODT_ATTR("elfexec"));

		m_terminal->putln();
		m_terminal->write_ro(RODT_ATTR("Started : pid "));
		m_terminal->write((int32_t)pid);
		return CMD_RESULT_OK;
	}
};

#endif

#endif
