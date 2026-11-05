/********************************** Exec Command *******************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 18th July 2026
******************************************************************************/
#ifndef _EXEC_COMMAND_H_
#define _EXEC_COMMAND_H_

#include "CommandCommon.h"

#if defined(ENABLE_PROGRAM_EXEC)

#include <interface/pdi/modules/exec/iProgramLoaderInterface.h>
#include <interface/pdi/threading/iExecution.h>

struct ExecCommand : public CommandBase {

	ExecCommand(){
		Clear();
		SetCommand(CMD_NAME_EXEC);
		setArgs(RODT_ATTR(""), 1);
		setAcceptArgsOptions(true);
	}

	/**
	 * Put the command in the registry under its name.
	 */
	static void RegisterCommand(){
		CommandBase::RegisterCommand(CMD_NAME_EXEC, [](void *arg)->void *{
			return pdiutil::safe_new<ExecCommand>();
		});
	}

	/**
	 * The one line the help listing and every argument error print.
	 */
	const char* getUsage() const override {
		return RODT_ATTR("exec <file>  load a program image and run it in background");
	}

#ifdef ENABLE_AUTH_SERVICE
	/**
	 * Running arbitrary code is privileged, so a session must be logged in.
	 */
	bool needauth() override { return true; }
#endif

	/**
	 * Report a failure and the detail that caused it, which every message shares.
	 */
	pdi_err_t fail(const char *_msg, const char *_detail = nullptr){
		m_terminal->putln();
		m_terminal->write_ro(_msg);
		if( nullptr != _detail ) m_terminal->write(_detail);
		return CMD_ERROR_FAILED;
	}

	/**
	 * Read the image, hand it to the port's loader and run it on its own task.
	 */
	pdi_err_t execute(cmd_term_inseq_t terminputaction){

#ifdef ENABLE_AUTH_SERVICE
		if( needauth() && !__auth_service.getAuthorized() ){
			return CMD_ERROR_PERM;
		}
#endif

		if( nullptr == m_terminal ) return CMD_ERROR_NOTTY;

		iProgramLoaderInterface *loader = getProgramLoader();
		if( nullptr == loader ){
			return fail(RODT_ATTR("This device cannot load programs"));
		}

		CommandOption *cmdoptn = &m_options[0];
		pdiutil::string progfile = resolveArgPath(cmdoptn);

		if( progfile.empty() ){
			m_terminal->putln();
			m_terminal->write_ro(RODT_ATTR("Usage: "));
			m_terminal->write_ro(getUsage());
			return CMD_ERROR_INVAL;
		}

		if( !__i_fs.isFileExist(progfile.c_str()) ){
			return fail(RODT_ATTR("No such file: "), progfile.c_str());
		}

		int64_t fsize = __i_fs.getFileSize(progfile.c_str());
		if( fsize <= 0 ){
			return fail(RODT_ATTR("Empty or unreadable file: "), progfile.c_str());
		}

		uint8_t *image = (uint8_t *)loader->allocImage((uint32_t)fsize);
		if( nullptr == image ){
			m_terminal->putln();
			m_terminal->write_ro(RODT_ATTR("Out of memory: need "));
			m_terminal->write((uint32_t)fsize);
			m_terminal->write_ro(RODT_ATTR(" bytes, free heap "));
			m_terminal->write((uint32_t)__i_dvc_ctrl.get_free_heap());
			return CMD_ERROR_FAILED;
		}

		uint32_t pos = 0;
		int rd = __i_fs.readFile(progfile.c_str(), 512, [&](char* data, uint32_t size)->bool{
			if( pos + size > (uint32_t)fsize ) size = (uint32_t)fsize - pos;
			memcpy(image + pos, data, size);
			pos += size;
			return pos < (uint32_t)fsize;
		});

		if( rd < 0 || pos != (uint32_t)fsize ){
			loader->freeImage(image);
			return fail(RODT_ATTR("Failed to read program file"));
		}

		if( !loader->isImageValid(image, (uint32_t)fsize) ){
			loader->freeImage(image);
			return fail(RODT_ATTR("Not a loadable program: "), progfile.c_str());
		}

		int32_t err = 0;
		program_t program = loader->load(image, (uint32_t)fsize, err);
		loader->freeImage(image);

		if( nullptr == program ){
			m_terminal->putln();
			m_terminal->write_ro(RODT_ATTR("Failed to load program, err: "));
			m_terminal->write((int32_t)err);
			return CMD_ERROR_FAILED;
		}

		pdiutil::task_id_t pid = __task_scheduler.register_task([loader, program]() {
			loader->run(program);
		});

		if( pid < 0 ){
			loader->unload(program);
			return fail(RODT_ATTR("Failed to create task"));
		}

		int sret = __task_scheduler.scheduleUnderExecSched(&__i_preemptive_scheduler, pid,
														   TASK_MODE_PREEMPTIVE, loader->stackSize());
		if( sret != 0 ){
			__task_scheduler.remove_task(pid);
			loader->unload(program);
			m_terminal->putln();
			m_terminal->write_ro(RODT_ATTR("Failed to start task, err: "));
			m_terminal->write((int32_t)sret);
			return CMD_ERROR_FAILED;
		}

		task_t *t = __task_scheduler.get_task(pid);
		if( nullptr != t ){
			t->m_stoppable = false;
			t->m_finalizer = [loader, program](void *) {
				loader->unload(program);
			};
		}
		__task_scheduler.setTaskName(pid, RODT_ATTR("exec"));

		m_terminal->putln();
		m_terminal->write_ro(RODT_ATTR("Started : pid "));
		m_terminal->write((int32_t)pid);
		return PDI_OK;
	}
};

#endif

#endif
