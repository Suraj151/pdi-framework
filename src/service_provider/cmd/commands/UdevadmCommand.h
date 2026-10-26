/****************************** Udevadm Command *******************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Lists the devices the serial service found and the drivers bound to them, asks
it to look again, or watches devices come and go until interrupted.

Author          : Suraj I.
created Date    : 19th Oct 2026
******************************************************************************/
#ifndef _UDEVADM_COMMAND_H_
#define _UDEVADM_COMMAND_H_

#include "CommandCommon.h"

#ifdef ENABLE_DRIVER_CORE

#include <service_provider/transport/SerialServiceProvider.h>

struct UdevadmCommand : public CommandBase {

	UdevadmCommand() : m_added_listener(EVENT_LISTENER_ID_INVALID), m_removed_listener(EVENT_LISTENER_ID_INVALID) {
		Clear();
		SetCommand(CMD_NAME_UDEVADM);
		setAcceptArgsOptions(true);
		setCmdOptionSeparator(CMD_OPTION_SEPERATOR_SPACE);
	}

	/**
	 * Stops watching before the command goes, so no event reaches it after.
	 */
	~UdevadmCommand(){
		stopWatching();
	}

	static void RegisterCommand(){
		CommandBase::RegisterCommand(CMD_NAME_UDEVADM, [](void *arg)->void *{
			return pdiutil::safe_new<UdevadmCommand>();
		});
	}

	const char* getUsage() const override {
		return RODT_ATTR("udevadm info|trigger|monitor  list devices, look again, or watch them come and go");
	}

#ifdef ENABLE_AUTH_SERVICE
	bool needauth() override { return true; }
#endif

	/**
	 * Interrupting a monitor stops it watching.
	 */
	bool stopRunningInBackground() override {
		stopWatching();
		m_runinbackground = false;
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

		CommandOption *verb = &m_options[0];
		if( nullptr == verb->optionval || 0 >= verb->optionvalsize ){
			return CMD_ERROR_ARGS_MISSING;
		}

		DeviceManager *mgr = __serial_service.devices();
		if( nullptr == mgr ){
			m_terminal->putln();
			m_terminal->writeln_ro(RODT_ATTR("no device manager running"));
			return CMD_ERROR_FAILED;
		}

		pdiutil::string action(verb->optionval, verb->optionvalsize);
		pdiutil::string info = CHARPTR_WRAP(UDEVADM_VERB_INFO);
		pdiutil::string trigger = CHARPTR_WRAP(UDEVADM_VERB_TRIGGER);
		pdiutil::string monitor = CHARPTR_WRAP(UDEVADM_VERB_MONITOR);

		if( action == info ){
			printDevices(mgr);
			return PDI_OK;
		}

		if( action == trigger ){
			mgr->trigger();
			return PDI_OK;
		}

		if( action == monitor ){
			return startWatching();
		}

		return CMD_ERROR_OPT;
	}

private:

	static constexpr uint8_t COL_BUS = 7;
	static constexpr uint8_t COL_ID = 10;
	static constexpr uint8_t COL_DRIVER = 18;

	int16_t m_added_listener;
	int16_t m_removed_listener;

	/**
	 * One row per known device: its interface, its name, its driver and where
	 * it stands.
	 */
	void printDevices(DeviceManager *mgr){

		m_terminal->putln();
		m_terminal->write_pad_ro(RODT_ATTR("BUS"), 3, COL_BUS);
		m_terminal->write_pad_ro(RODT_ATTR("ID"), 2, COL_ID);
		m_terminal->write_pad_ro(RODT_ATTR("DRIVER"), 6, COL_DRIVER);
		m_terminal->writeln_ro(RODT_ATTR("STATE"));

		pdiutil::string bus;
		pdiutil::string id;
		pdiutil::string none = "-";

		for( uint16_t i = 0; i < mgr->count(); i++ ){
			const driver_device_t *dev = mgr->at(i);
			DeviceManager::busName(dev->m_bus, bus);
			DeviceManager::deviceId(*dev, id);
			const pdiutil::string &driver = dev->m_driver.empty() ? none : dev->m_driver;

			m_terminal->write_pad(bus.c_str(), COL_BUS);
			m_terminal->write_pad(id.c_str(), COL_ID);
			m_terminal->write_pad(driver.c_str(), COL_DRIVER);
			pdiutil::string state = driverStateName(dev->m_state);
			if( dev->m_unscanned ){
				state += CHARPTR_WRAP(DEVICE_STATE_NOTE_MANUAL);
			}
			m_terminal->writeln(state.c_str());
		}
	}

	/**
	 * Starts printing each device that comes or goes, and keeps the command
	 * running until it is interrupted.
	 */
	pdi_err_t startWatching(){

		stopWatching();

		m_added_listener = __utl_event.add_event_listener(EVENT_DEVICE_ADDED, [&](void *e){
			runAsOwner(this, [&](){
				printEvent(true, e);
			});
		});

		m_removed_listener = __utl_event.add_event_listener(EVENT_DEVICE_REMOVED, [&](void *e){
			runAsOwner(this, [&](){
				printEvent(false, e);
			});
		});

		if( EVENT_LISTENER_ID_INVALID == m_added_listener || EVENT_LISTENER_ID_INVALID == m_removed_listener ){
			stopWatching();
			return CMD_ERROR_FAILED;
		}

		m_terminal->putln();
		m_terminal->writeln_ro(RODT_ATTR("monitor will print the received events"));
		m_runinbackground = true;
		return PDI_OK;
	}

	/**
	 * Stops printing device events.
	 */
	void stopWatching(){

		if( EVENT_LISTENER_ID_INVALID != m_added_listener ){
			__utl_event.remove_event_listener(m_added_listener);
			m_added_listener = EVENT_LISTENER_ID_INVALID;
		}

		if( EVENT_LISTENER_ID_INVALID != m_removed_listener ){
			__utl_event.remove_event_listener(m_removed_listener);
			m_removed_listener = EVENT_LISTENER_ID_INVALID;
		}
	}

	/**
	 * One line for a device that came or went: what happened, its interface,
	 * its name and, when it came, its driver.
	 */
	void printEvent(bool added, void *e){

		if( nullptr == m_terminal || nullptr == e ){
			return;
		}

		const driver_device_t *dev = reinterpret_cast<const driver_device_t *>(e);
		pdiutil::string bus;
		pdiutil::string id;
		DeviceManager::busName(dev->m_bus, bus);
		DeviceManager::deviceId(*dev, id);

		pdiutil::string line = added ? CHARPTR_WRAP(UDEVADM_EVENT_ADD) : CHARPTR_WRAP(UDEVADM_EVENT_REMOVE);
		line += bus;
		line += ' ';
		line += id;
		if( added && !dev->m_driver.empty() ){
			line += ' ';
			line += dev->m_driver;
		}

		m_terminal->writeln(line.c_str());
		m_terminal->commit();
	}
};

#endif

#endif
