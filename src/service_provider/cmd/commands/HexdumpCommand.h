/******************************** Hexdump Command *****************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 28th May 2026
******************************************************************************/

#ifndef _HEXDUMP_COMMAND_H_
#define _HEXDUMP_COMMAND_H_

#include "CommandCommon.h"

#ifdef ENABLE_STORAGE_SERVICE

struct HexdumpCommand : public CommandBase {

	HexdumpCommand(){
		Clear();
		SetCommand(CMD_NAME_HEXDUMP);
		setAcceptArgsOptions(true);
	}

	static void RegisterCommand(){
		CommandBase::RegisterCommand(CMD_NAME_HEXDUMP, [](void *arg)->void *{
			return new HexdumpCommand();
		});
	}

	const char* getUsage() const override {
		return RODT_ATTR("hexdump <file>  print offset / 16 hex bytes / ASCII");
	}

#ifdef ENABLE_AUTH_SERVICE
	bool needauth() override { return true; }
#endif

	static char nibbleHex(uint8_t n){
		return (n < 10) ? ('0' + n) : ('a' + (n - 10));
	}

	cmd_result_t execute(cmd_term_inseq_t terminputaction){

#ifdef ENABLE_AUTH_SERVICE
		if( needauth() && !__auth_service.getAuthorized()){
			return CMD_RESULT_NEED_AUTH;
		}
#endif

		cmd_result_t result = CMD_RESULT_OK;

		if(nullptr != m_terminal){
			CommandOption *cmdoptn = &m_options[0];
			pdiutil::string filename = resolveArgPath(cmdoptn);
			if( !filename.empty() ){
				{
					uint32_t offset = 0;
					uint8_t rowfilled = 0;
					char asciibuf[16];

					m_terminal->putln();

					int iStatus = __i_fs.readFile(filename.c_str(), 16, [&](char *data, uint32_t size)->bool{

						char prefix[10];
						char hexpair[3];

						for (uint32_t i = 0; i < size; i++){

							if(rowfilled == 0){
								for (uint8_t s = 0; s < 8; s++){
									prefix[s] = nibbleHex((offset >> ((7 - s) * 4)) & 0x0F);
								}
								prefix[8] = ' ';
								prefix[9] = ' ';
								m_terminal->write(prefix, 10);
							}

							uint8_t b = (uint8_t)data[i];
							hexpair[0] = nibbleHex(b >> 4);
							hexpair[1] = nibbleHex(b & 0x0F);
							hexpair[2] = ' ';
							m_terminal->write(hexpair, 3);

							asciibuf[rowfilled++] = (b >= 0x20 && b < 0x7F) ? (char)b : '.';
							offset++;

							if(rowfilled == 16){
								m_terminal->write_ro(RODT_ATTR(" |"));
								m_terminal->write(asciibuf, 16);
								m_terminal->write_ro(RODT_ATTR("|"));
								m_terminal->putln();
								rowfilled = 0;
							}
						}
						return true;
					});

					if(rowfilled > 0){
						for (uint8_t i = rowfilled; i < 16; i++){
							m_terminal->write_ro(RODT_ATTR("   "));
						}
						m_terminal->write_ro(RODT_ATTR(" |"));
						m_terminal->write(asciibuf, rowfilled);
						m_terminal->write_ro(RODT_ATTR("|"));
						m_terminal->putln();
					}

					if(iStatus < 0){
						result = CMD_RESULT_FAILED;
						m_terminal->write_ro(RODT_ATTR("Failed : "));
						m_terminal->write(filename.c_str());
						m_terminal->write_ro(RODT_ATTR(" : "));
						m_terminal->write((int32_t)iStatus);
					}
				}
			}else{
				result = CMD_RESULT_ARGS_MISSING;
			}
		}

		return result;
	}
};

#endif

#endif
