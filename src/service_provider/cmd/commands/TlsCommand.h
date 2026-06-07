/*********************************** TLS Command *****************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 7th June 2026
******************************************************************************/

#ifndef _TLS_COMMAND_H_
#define _TLS_COMMAND_H_

#include "CommandCommon.h"

#ifdef ENABLE_TLS_CERT_GENERATION

#include <interface/pdi.h>
#include <config/TlsConfig.h>

/**
 * TLS command
 *
 * e.g. tls q={TlsCommandQuery enum number},t={algo: 0=EC, 1=RSA},
 *          l={key bits / curve size},n={subject CN or DNS},i={IPv4 dotted}
 */
struct TlsCommand : public CommandBase {

    /* TLS Query options */
    enum TlsCommandQuery {
        TLS_COMMAND_QUERY_MIN = 0,
        TLS_COMMAND_QUERY_CERTGEN,
        TLS_COMMAND_QUERY_MAX
    };

    /* Constructor */
    TlsCommand() {
        Clear();
        SetCommand(CMD_NAME_TLS);
        AddOption(CMD_OPTION_NAME_Q);
        AddOption(CMD_OPTION_NAME_T);
        AddOption(CMD_OPTION_NAME_L);
        AddOption(CMD_OPTION_NAME_N);
        AddOption(CMD_OPTION_NAME_I);
    }

    /**
     * @brief Register the command.
     */
    static void RegisterCommand() {
        CommandBase::RegisterCommand(CMD_NAME_TLS, [](void* arg) -> void* {
            return new TlsCommand();
        });
    }

#ifdef ENABLE_AUTH_SERVICE
    /* override the necesity of required permission */
    bool needauth() override { return true; }
#endif

    /* execute command with provided options */
    cmd_result_t execute(cmd_term_inseq_t terminputaction) {

#ifdef ENABLE_AUTH_SERVICE
        if (needauth() && !__auth_service.getAuthorized()) {
            return CMD_RESULT_NEED_AUTH;
        }
#endif

        cmd_result_t result = CMD_RESULT_OK;
        TlsCommandQuery tlsq = TLS_COMMAND_QUERY_MAX;
        CommandOption* cmdoptn = nullptr;

        cmdoptn = RetrieveOption(CMD_OPTION_NAME_Q);
        if (nullptr != cmdoptn) {
            tlsq = (TlsCommandQuery)StringToUint16(cmdoptn->optionval, cmdoptn->optionvalsize);
        }

        if (nullptr != m_terminal &&
            tlsq > TLS_COMMAND_QUERY_MIN && tlsq < TLS_COMMAND_QUERY_MAX) {

            if (tlsq == TLS_COMMAND_QUERY_CERTGEN) {

                TlsCertProvisioner::CertParams params;

                cmdoptn = RetrieveOption(CMD_OPTION_NAME_T);
                if (nullptr != cmdoptn) {
                    uint16_t algo = StringToUint16(cmdoptn->optionval, cmdoptn->optionvalsize);
                    params.algo = (algo == 1) ? TlsCertProvisioner::KEY_ALGO_RSA
                                              : TlsCertProvisioner::KEY_ALGO_EC;
                }

                cmdoptn = RetrieveOption(CMD_OPTION_NAME_L);
                if (nullptr != cmdoptn) {
                    params.keySize = (int)StringToUint16(cmdoptn->optionval, cmdoptn->optionvalsize);
                }

                char nameBuf[64]; memset(nameBuf, 0, sizeof(nameBuf));
                cmdoptn = RetrieveOption(CMD_OPTION_NAME_N);
                if (nullptr != cmdoptn && cmdoptn->optionvalsize > 0) {
                    uint16_t n = cmdoptn->optionvalsize;
                    if (n >= sizeof(nameBuf)) n = sizeof(nameBuf) - 1;
                    memcpy(nameBuf, cmdoptn->optionval, n);
                    nameBuf[n] = 0;
                    params.dns_name = nameBuf;
                    params.subjectCn = nameBuf;
                }

                cmdoptn = RetrieveOption(CMD_OPTION_NAME_I);
                if (nullptr != cmdoptn && cmdoptn->optionvalsize > 0) {
                    uint16_t n = cmdoptn->optionvalsize;
                    char ipStr[20]; memset(ipStr, 0, sizeof(ipStr));
                    if (n >= sizeof(ipStr)) n = sizeof(ipStr) - 1;
                    memcpy(ipStr, cmdoptn->optionval, n);
                    ipStr[n] = 0;

                    ipaddress_t ip(ipStr);
                    params.ip_v4 = (uint32_t)ip;
                }

                if (params.ip_v4 == 0 &&
                    (params.dns_name == nullptr || params.dns_name[0] == 0)) {
                    result = CMD_RESULT_FAILED;
                    m_terminal->putln();
                    m_terminal->writeln_ro(RODT_ATTR("Provide -n <name> and/or -i <ip>."));
                    return result;
                }

                bool ok = TlsCertProvisioner::generateCert(
                    TLS_DEFAULT_SERVER_CERT_PATH,
                    TLS_DEFAULT_SERVER_KEY_PATH,
                    params);

                m_terminal->putln();
                if (ok) {
                    m_terminal->writeln_ro(RODT_ATTR("TLS cert generated successfully."));
                } else {
                    result = CMD_RESULT_FAILED;
                    m_terminal->writeln_ro(RODT_ATTR("Failed to generate TLS cert."));
                }
            }
        }

        return result;
    }
};

#endif // ENABLE_TLS_CERT_GENERATION

#endif
