/********************************* Ping Command *******************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 23rd July 2026
******************************************************************************/

#ifndef _PING_COMMAND_H_
#define _PING_COMMAND_H_

#include "CommandCommon.h"

#ifdef ENABLE_NETWORK_SERVICE

#include <service_provider/network/NameResolver.h>
#include <interface/pdi/middlewares/iPingInterface.h>
#include <utility/DataTypeConversions.h>

#define PING_CMD_DEFAULT_COUNT   4
#define PING_CMD_MAX_COUNT       10
#define PING_CMD_PER_PACKET_MS   1500

/**
 * ping command
 *
 * ICMP echo a host, resolving it via NameResolver (IP / /etc/hosts / DNS):
 *   ping example.com
 *   ping 8.8.8.8 3
 */
struct PingCommand : public CommandBase {

	/* Constructor */
	PingCommand(){
		Clear();
		SetCommand(CMD_NAME_PING);
		setAcceptArgsOptions(true);
		setCmdOptionSeparator(CMD_OPTION_SEPERATOR_SPACE);
	}

	/**
     * @brief Register the command.
     */
    static void RegisterCommand(){
		CommandBase::RegisterCommand(CMD_NAME_PING, [](void *arg)->void *{
			return new PingCommand();
		});
	}

	const char* getUsage() const override {
		return RODT_ATTR("ping <host> [count]  ICMP echo a host (default 4, max 10)");
	}

#ifdef ENABLE_AUTH_SERVICE
	/* override the necesity of required permission */
	bool needauth() override { return true; }
#endif

	/* execute command with provided options */
	cmd_result_t execute(cmd_term_inseq_t terminputaction){

#ifdef ENABLE_AUTH_SERVICE
		// return in case authentication needed and not authorized yet
		if( needauth() && !__auth_service.getAuthorized() ){
			return CMD_RESULT_NEED_AUTH;
		}
#endif

		if( nullptr == m_terminal ){
			return CMD_RESULT_TERMINAL_ERR;
		}

		CommandOption *a0 = &m_options[0];
		CommandOption *a1 = &m_options[1];
		if( nullptr == a0->optionval || a0->optionvalsize <= 0 ){
			return CMD_RESULT_ARGS_MISSING;
		}

		pdiutil::string host(a0->optionval, a0->optionvalsize);

		uint16_t count = PING_CMD_DEFAULT_COUNT;
		if( nullptr != a1->optionval && a1->optionvalsize > 0 ){
			count = (uint16_t)StringToUint16(a1->optionval, a1->optionvalsize);
		}
		if( count == 0 ) count = 1;
		if( count > PING_CMD_MAX_COUNT ) count = PING_CMD_MAX_COUNT;

		m_terminal->putln();
		ipaddress_t ip;
		if( !NameResolver::resolve(host.c_str(), ip) ){
			m_terminal->write_ro(RODT_ATTR("ping: cannot resolve host: "));
			m_terminal->writeln(host.c_str());
			return CMD_RESULT_OK;
		}

		pdiutil::string ipstr = ip;
		m_terminal->write_ro(RODT_ATTR("PING "));
		m_terminal->write(host.c_str());
		m_terminal->write_ro(RODT_ATTR(" ("));
		m_terminal->write(ipstr.c_str());
		m_terminal->write_ro(RODT_ATTR(") x"));
		m_terminal->writeln((int32_t)count);

		// print each reply/timeout live as the port reports it, so the shell
		// doesn't look frozen during the run
		iTerminalInterface *term = m_terminal;
		CallBackVoidPointerArgFn on_packet = [term](void *arg){
			ping_pkt_t *p = (ping_pkt_t *)arg;
			if( nullptr == p ) return;
			term->write_ro(RODT_ATTR("seq="));
			term->write((int32_t)p->m_seqno);
			if( p->m_replied ){
				term->write_ro(RODT_ATTR(" time="));
				term->write((int32_t)p->m_rtt_ms);
				term->writeln_ro(RODT_ATTR(" ms"));
			}else{
				term->writeln_ro(RODT_ATTR(" timeout"));
			}
		};

		if( !__i_ping.ping(ip, count, on_packet) ){
			m_terminal->writeln_ro(RODT_ATTR("ping: could not start"));
			return CMD_RESULT_FAILED;
		}

		uint32_t start = __i_dvc_ctrl.millis_now();
		uint32_t budget = (uint32_t)count * PING_CMD_PER_PACKET_MS + 1000;
		while( !__i_ping.isPingComplete() && (__i_dvc_ctrl.millis_now() - start) < budget ){
			__i_dvc_ctrl.wait(50);
			__i_dvc_ctrl.yield();
		}

		const ping_stats_t &st = __i_ping.getPingStats();
		uint16_t tx = st.m_transmitted;
		uint16_t rx = st.m_received;
		uint16_t loss = (tx > 0) ? (uint16_t)(((uint32_t)(tx - rx) * 100) / tx) : 100;

		m_terminal->putln();
		m_terminal->write((int32_t)tx);
		m_terminal->write_ro(RODT_ATTR(" transmitted, "));
		m_terminal->write((int32_t)rx);
		m_terminal->write_ro(RODT_ATTR(" received, "));
		m_terminal->write((int32_t)loss);
		m_terminal->writeln_ro(RODT_ATTR("% loss"));

		if( rx > 0 ){
			m_terminal->write_ro(RODT_ATTR("rtt min/avg/max = "));
			m_terminal->write((int32_t)st.m_min_ms);
			m_terminal->write_ro(RODT_ATTR("/"));
			m_terminal->write((int32_t)st.m_avg_ms);
			m_terminal->write_ro(RODT_ATTR("/"));
			m_terminal->write((int32_t)st.m_max_ms);
			m_terminal->writeln_ro(RODT_ATTR(" ms"));
		}

		return CMD_RESULT_OK;
	}
};

#endif

#endif
