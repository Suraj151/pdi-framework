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
#define PING_CMD_BUSY_WAIT_MS    MILLISECOND_DURATION_10000

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
			return pdiutil::safe_new<PingCommand>();
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
	pdi_err_t execute(cmd_term_inseq_t terminputaction){

#ifdef ENABLE_AUTH_SERVICE
		// return in case authentication needed and not authorized yet
		if( needauth() && !__auth_service.getAuthorized() ){
			return CMD_ERROR_PERM;
		}
#endif

		if( nullptr == m_terminal ){
			return CMD_ERROR_NOTTY;
		}

		CommandOption *a0 = &m_options[0];
		CommandOption *a1 = &m_options[1];
		if( nullptr == a0->optionval || a0->optionvalsize <= 0 ){
			return CMD_ERROR_ARGS_MISSING;
		}

		pdiutil::string host(a0->optionval, a0->optionvalsize);

		uint16_t count = PING_CMD_DEFAULT_COUNT;
		if( nullptr != a1->optionval && a1->optionvalsize > 0 ){
			count = (uint16_t)StringToUint16(a1->optionval, a1->optionvalsize);
		}
		if( count == 0 ) count = 1;
		if( count > PING_CMD_MAX_COUNT ) count = PING_CMD_MAX_COUNT;

		m_terminal->putln();

		uint32_t waited = 0;
		while( __i_ping.isPingBusy() && waited < PING_CMD_BUSY_WAIT_MS ){
			if( 0 == waited ){
				m_terminal->write_ro(RODT_ATTR("waiting for another ping to finish"));
			}
			__i_dvc_ctrl.wait(50);
			__i_dvc_ctrl.yield();
			waited += 50;
			if( 0 == (waited % MILLISECOND_DURATION_1000) ){
				m_terminal->write_ro(RODT_ATTR("."));
				m_terminal->commit();
			}
		}

		if( waited > 0 ){
			m_terminal->putln();
		}

		if( __i_ping.isPingBusy() ){
			m_terminal->writeln_ro(RODT_ATTR("ping: another ping is in progress, try again"));
			return CMD_ERROR_FAILED;
		}

		ipaddress_t ip;
		if( !NameResolver::resolve(host.c_str(), ip) ){
			m_terminal->write_ro(RODT_ATTR("ping: cannot resolve host: "));
			m_terminal->writeln(host.c_str());
			return PDI_OK;
		}

		pdiutil::string ipstr = ip;
		m_terminal->write_ro(RODT_ATTR("PING "));
		m_terminal->write(host.c_str());
		m_terminal->write_ro(RODT_ATTR(" ("));
		m_terminal->write(ipstr.c_str());
		m_terminal->write_ro(RODT_ATTR(") x"));
		m_terminal->writeln((int32_t)count);

		static ping_pkt_t s_packet;
		static volatile bool s_received = false;
		s_received = false;
		CallBackVoidPointerArgFn on_packet = [](void *arg){
			ping_pkt_t *p = (ping_pkt_t *)arg;
			if( nullptr == p ) return;
			s_packet = *p;
			s_received = true;
		};

		if( !__i_ping.ping(ip, count, on_packet) ){
			m_terminal->writeln_ro(RODT_ATTR("ping: could not start"));
			return CMD_ERROR_FAILED;
		}

		uint32_t start = __i_dvc_ctrl.millis_now();
		uint32_t budget = (uint32_t)count * PING_CMD_PER_PACKET_MS + 1000;
		while( !__i_ping.isPingComplete() && (__i_dvc_ctrl.millis_now() - start) < budget ){
			writeReceived(s_packet, s_received);
			__i_dvc_ctrl.wait(50);
			__i_dvc_ctrl.yield();
			m_terminal->commit();
		}
		writeReceived(s_packet, s_received);

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

		return PDI_OK;
	}

private:

	/**
	 * Prints the reply or timeout the port reported, if one is waiting, in the
	 * order it came, and marks it printed.
	 */
	void writeReceived(const ping_pkt_t &packet, volatile bool &received){
		if( !received ){
			return;
		}
		ping_pkt_t p = packet;
		received = false;
		m_terminal->write_ro(RODT_ATTR("seq="));
		m_terminal->write((int32_t)p.m_seqno);
		if( p.m_replied ){
			m_terminal->write_ro(RODT_ATTR(" time="));
			m_terminal->write((int32_t)p.m_rtt_ms);
			m_terminal->writeln_ro(RODT_ATTR(" ms"));
		}else{
			m_terminal->writeln_ro(RODT_ATTR(" timeout"));
		}
	}
};

#endif

#endif
