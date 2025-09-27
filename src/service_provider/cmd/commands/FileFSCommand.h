/*************************** File Operation Command ***************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st June 2019
******************************************************************************/

#ifndef _FILE_OPERATION_FS_COMMAND_H_
#define _FILE_OPERATION_FS_COMMAND_H_

#include "CommandCommon.h"

#ifdef ENABLE_STORAGE_SERVICE
/**
 * file read operation command
 * 
 * e.g. if we want to read file in the system then we can execute command as below
 * frd <filename>
 * 
 */
struct FileReadCommand : public CommandBase {

	/* Constructor */
	FileReadCommand(){
		Clear();
		SetCommand(CMD_NAME_FILE_READ);
		setAcceptArgsOptions(true);
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
			// Get first option which must be the filename to read
			CommandOption *cmdoptn = &m_options[0];
			if( nullptr != cmdoptn && nullptr != cmdoptn->optionval && cmdoptn->optionvalsize > 0 ){
				char *filename = new char[cmdoptn->optionvalsize+__i_fs.getPWD().size()+2]();
				if( nullptr != filename ){
					memset(filename, 0, cmdoptn->optionvalsize+__i_fs.getPWD().size()+2);
					memcpy(filename, __i_fs.getPWD().c_str(), __i_fs.getPWD().size());
					__i_fs.appendFileSeparator(filename);
					strncat(filename, cmdoptn->optionval, cmdoptn->optionvalsize);

					m_terminal->putln();
					int iStatus = __i_fs.readFile(filename, 10, [&](char* data, uint32_t size)->bool{
						m_terminal->write(data, size);
						// return true to continue reading
						return true;
					});

					if (iStatus < 0) {
						result = CMD_RESULT_FAILED;
						m_terminal->putln();
						m_terminal->write_ro(RODT_ATTR("Failed : "));
						m_terminal->write(filename);
						m_terminal->write_ro(RODT_ATTR(" : "));
						m_terminal->write((int32_t)iStatus);
					}
					delete[] filename;	
				}
			}
		}

		return result;
	}
};


/**
 * file write operation command
 * 
 * e.g. if we want to write on a file in the system then we can execute command as below
 * fwr <filename>
 * 
 * value can be given in subsequent enter i.e. first give only filename with command then
 * will get notification from where we can enter the data for file
 */
struct FileWriteCommand : public CommandBase {

	/* Constructor */
	FileWriteCommand(){
		Clear();
		SetCommand(CMD_NAME_FILE_WRITE);
		AddOption(CMD_OPTION_NAME_F);
		AddOption(CMD_OPTION_NAME_V);
		setAcceptArgsOptions(true);
	}

	// Local variables used in operations
	bool m_fscmd_esc_attempted = false;

#ifdef ENABLE_AUTH_SERVICE
	/* override the necesity of required permission */
	bool needauth() override { return true; }
#endif

    /**
     * @brief Executes the terminal input action.
     * @param terminputaction The terminal input action to execute.
     * @return The result of the command execution.
     */
    cmd_result_t executeTermInputAction(cmd_term_inseq_t terminputaction) override{

		CommandOption *filenamecmdoptn = RetrieveOption(CMD_OPTION_NAME_F);	
		bool isFilenameProvided = ( nullptr != filenamecmdoptn && nullptr != filenamecmdoptn->optionval && filenamecmdoptn->optionvalsize );
		m_fscmd_esc_attempted = false;

        if( terminputaction == CMD_TERM_INSEQ_ESC ){

			if( nullptr != m_terminal ){
				m_terminal->putln();
				m_terminal->write_ro(RODT_ATTR("write/append[!w] cancel[!c]: "));
			}

			m_fscmd_esc_attempted = true;
			return m_result;

		}else if( m_result == CMD_RESULT_INCOMPLETE && terminputaction == CMD_TERM_INSEQ_ENTER ){

			cmd_result_t result = CMD_RESULT_OK;
			if(isFilenameProvided){
				result = updateFile(filenamecmdoptn, "\r\n", 2);

				if( nullptr != m_terminal ){
					m_terminal->putln();
				}
			}

			return m_result;
		}

		return CommandBase::executeTermInputAction(terminputaction);
    }

	/* execute command with provided options */
	cmd_result_t execute(cmd_term_inseq_t terminputaction){

#ifdef ENABLE_AUTH_SERVICE
		// return in case authentication needed and not authorized yet
		if( needauth() && !__auth_service.getAuthorized()){
			return CMD_RESULT_NEED_AUTH;
		}
#endif

		cmd_result_t result = CMD_RESULT_OK;

		CommandOption *filenamecmdoptn = RetrieveOption(CMD_OPTION_NAME_F);	
		CommandOption *valuecmdoptn = RetrieveOption(CMD_OPTION_NAME_V);	

		bool isFilenameProvided = ( nullptr != filenamecmdoptn && nullptr != filenamecmdoptn->optionval && filenamecmdoptn->optionvalsize );
		bool isFileValueProvided = ( nullptr != valuecmdoptn && nullptr != valuecmdoptn->optionval && valuecmdoptn->optionvalsize );

		if( isFilenameProvided ){
			if( !isFileValueProvided ){
				holdOptionValue(CMD_OPTION_NAME_F);
			}
		}else{
			setWaitingForOption(CMD_OPTION_NAME_F);
			if( nullptr != m_terminal ){
				m_terminal->putln();
				m_terminal->write_ro(RODT_ATTR("Enter Filename : "));
			}
			return CMD_RESULT_INCOMPLETE;
		}

		if( !isFileValueProvided ){
			setWaitingForOption(CMD_OPTION_NAME_V);
			
			if( nullptr != m_terminal && m_iterations == 0){
				// clear the display
				m_terminal->csi_erase_display();
				// m_terminal->putln();
				// m_terminal->write_ro(RODT_ATTR("Enter data ( quit[!q] ) : "));
			}
			return CMD_RESULT_INCOMPLETE;
		}

		if( isFilenameProvided && isFileValueProvided ){
			result = updateFile(filenamecmdoptn, valuecmdoptn->optionval, valuecmdoptn->optionvalsize);
		}else{
			result = CMD_RESULT_ARGS_ERROR;
		}

		return result;
	}

	/* Update file with provided value */
	cmd_result_t updateFile( CommandOption *filenamecmdoptn, const char* data, int16_t len){

		cmd_result_t result = CMD_RESULT_OK;
		char *filename = new char[filenamecmdoptn->optionvalsize+__i_fs.getPWD().size()+2]();
		
		if( nullptr != filename ){

			memset(filename, 0, filenamecmdoptn->optionvalsize+__i_fs.getPWD().size()+2);
			memcpy(filename, __i_fs.getPWD().c_str(), __i_fs.getPWD().size());
			__i_fs.appendFileSeparator(filename);
			strncat(filename, filenamecmdoptn->optionval, filenamecmdoptn->optionvalsize);

			int iStatus = 0;
			int findWriteQuit = -1;
			int findCancelQuit = -1;

			// check if user has opted esc options
			if(m_fscmd_esc_attempted){
				findWriteQuit = __strstr((char*)data, "!w", len);
				findCancelQuit = __strstr((char*)data, "!c", len);
			}

			// Create temp file of existing file if exist
			pdiutil::string tempfile = filename;
			tempfile += ".tmp";
			const char* tempfilename = tempfile.c_str();
			if( __i_fs.isFileExist(filename) && !__i_fs.isFileExist(tempfilename) ){
				iStatus = __i_fs.copyFile(filename, tempfilename);
			}

			if (findWriteQuit < 0 && findCancelQuit < 0) {

				iStatus = __i_fs.writeFile(tempfilename, data, len, true);

				// on success
				if (iStatus >= 0) {
					setWaitingForOption(CMD_OPTION_NAME_V);
					result = CMD_RESULT_INCOMPLETE;
				}
			} else if(0 <= findWriteQuit){

				if(__i_fs.isFileExist(filename)){

					iStatus = __i_fs.deleteFile(filename);
				}
				iStatus = __i_fs.rename(tempfilename, filename);
			} else if(0 <= findCancelQuit){

				if(__i_fs.isFileExist(tempfilename)){

					iStatus = __i_fs.deleteFile(tempfilename);
				}
			}

			// clear the local buffer data if found quite
			if (0 <= findWriteQuit || 0 <= findCancelQuit) {
				result = CMD_RESULT_OK;
			}

			if (iStatus < 0) {
				result = CMD_RESULT_FAILED;
				m_terminal->putln();
				m_terminal->write_ro(RODT_ATTR("Failed : "));
				m_terminal->write(filename);
				m_terminal->write_ro(RODT_ATTR(" : "));
				m_terminal->write((int32_t)iStatus);
			}

			delete[] filename;	
		}

		return result;
	}
};

#endif

#endif
