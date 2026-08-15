/*************************** Dashboard Controller *****************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st June 2019
******************************************************************************/

#ifndef _WEB_SERVER_DASHBOARD_CONTROLLER_
#define _WEB_SERVER_DASHBOARD_CONTROLLER_

#include "Controller.h"
#include <webserver/pages/Dashboard.h>
#include <utility/TaskScheduler.h>

#ifdef ENABLE_AUTH_SERVICE
#include <service_provider/session/SessionManager.h>
#endif

#ifdef ENABLE_GPIO_SERVICE
#include <service_provider/device/GpioServiceProvider.h>
#endif

/**
 * @define DASHBOARD_MAX_TASK_ROWS
 * @brief Number of tasks carried in one monitor response.
 */
#define DASHBOARD_MAX_TASK_ROWS 4

/**
 * @define DASHBOARD_JSON_RESERVE
 * @brief Payload buffer reserved up front so the string never grows in steps.
 */
#define DASHBOARD_JSON_RESERVE 1024

/**
 * DashboardController class
 */
class DashboardController : public Controller {

	public:

		/**
		 * DashboardController constructor
		 */
		DashboardController():Controller("dashboard"){
		}

		/**
		 * DashboardController destructor
		 */
		~DashboardController(){
		}

		/**
		 * register dashboard controller
		 *
		 */
		void boot( void ){

			if( nullptr != this->m_route_handler ){
				this->m_route_handler->register_route( WEB_SERVER_DASHBOARD_ROUTE, [&]() { this->handleDashboardRoute(); }, AUTH_MIDDLEWARE );
	      		this->m_route_handler->register_route( WEB_SERVER_DASHBOARD_MONITOR_ROUTE, [&]() { this->handleDashboardMonitor(); }, API_MIDDLEWARE );
			}
		}

		/**
		 * append the radio state and the network time.
		 */
		void appendNetworkJson(pdiutil::string &_response)
		{
			_response += CHARPTR_WRAP("\"w\":{\"c\":");

#ifdef ENABLE_WIFI_SERVICE
			_response += pdiutil::to_string(__i_wifi.isConnected() ? 1 : 0);
			_response += CHARPTR_WRAP(",\"nm\":\"");
			_response += __i_wifi.SSID();
			_response += CHARPTR_WRAP("\",\"ip\":\"");
			_response += (pdiutil::string)__i_wifi.localIP();
			_response += CHARPTR_WRAP("\",\"mc\":\"");
			_response += __i_wifi.macAddress();
			_response += CHARPTR_WRAP("\",\"rs\":");
			_response += pdiutil::to_string(__i_wifi.RSSI());
			_response += CHARPTR_WRAP(",\"nt\":");
			_response += pdiutil::to_string(__status_wifi.internet_available ? 1 : 0);
#else
			_response += CHARPTR_WRAP("0,\"nm\":\"\",\"ip\":\"\",\"mc\":\"\",\"rs\":0,\"nt\":0");
#endif

			_response += CHARPTR_WRAP("},\"nwt\":");
			_response += pdiutil::to_string(__i_ntp.get_ntp_time());
		}

		/**
		 * append the root filesystem totals.
		 */
		void appendStorageJson(pdiutil::string &_response)
		{
#ifdef ENABLE_STORAGE_SERVICE
			_response += CHARPTR_WRAP(",\"fs\":{\"t\":");
			_response += pdiutil::to_string((int32_t)__i_fs.getTotalSize());
			_response += CHARPTR_WRAP(",\"u\":");
			_response += pdiutil::to_string((int32_t)__i_fs.getUsedSize());
			_response += "}";
#else
			_response += CHARPTR_WRAP(",\"fs\":0");
#endif
		}

		/**
		 * append the heap figures and the busiest tasks.
		 *
		 * Tasks are ranked by lifetime cpu share so the rows carry the ones worth
		 * watching rather than the first few registered.
		 */
		void appendTasksJson(pdiutil::string &_response)
		{
			uint32_t _now = (uint32_t)__i_dvc_ctrl.millis_now();

			_response += CHARPTR_WRAP(",\"up\":");
			_response += pdiutil::to_string((int32_t)(_now / 1000));
			_response += CHARPTR_WRAP(",\"hp\":");
			_response += pdiutil::to_string((int32_t)__i_dvc_ctrl.get_free_heap());
			_response += CHARPTR_WRAP(",\"hb\":");
			_response += pdiutil::to_string((int32_t)__i_dvc_ctrl.get_max_free_block());
			_response += CHARPTR_WRAP(",\"tc\":");
			_response += pdiutil::to_string((int32_t)__task_scheduler.getTaskCount());
			_response += CHARPTR_WRAP(",\"ps\":[");

			uint16_t _picked[DASHBOARD_MAX_TASK_ROWS];
			uint8_t _pickedcount = 0;

			for (uint16_t _slot = 0; _slot < __task_scheduler.getTaskSlots(); _slot++)
			{
				task_t *_task = __task_scheduler.getTaskByIndex(_slot);
				if (nullptr == _task) continue;

				uint32_t _share = taskCpuShare(_task, _now);
				uint8_t _at = _pickedcount;

				while (_at > 0)
				{
					task_t *_ranked = __task_scheduler.getTaskByIndex(_picked[_at - 1]);
					if (nullptr != _ranked && taskCpuShare(_ranked, _now) >= _share) break;
					if (_at < DASHBOARD_MAX_TASK_ROWS) _picked[_at] = _picked[_at - 1];
					_at--;
				}

				if (_at < DASHBOARD_MAX_TASK_ROWS)
				{
					_picked[_at] = _slot;
					if (_pickedcount < DASHBOARD_MAX_TASK_ROWS) _pickedcount++;
				}
			}

			bool _first = true;

			for (uint8_t _row = 0; _row < _pickedcount; _row++)
			{
				task_t *_task = __task_scheduler.getTaskByIndex(_picked[_row]);
				if (nullptr == _task) continue;

				if (!_first) _response += ",";
				_first = false;

				_response += CHARPTR_WRAP("[");
				_response += pdiutil::to_string((int32_t)_task->m_task_id);
				_response += CHARPTR_WRAP(",\"");

				// the task name is a read only pointer, so it has to be pulled out
				// of program space before it can join the payload
				if (nullptr != _task->m_name)
				{
					_response += CHARPTR_WRAP_RO(_task->m_name);
				}
				else
				{
					_response += "-";
				}

				_response += CHARPTR_WRAP("\",\"");
				_response += taskStateLetter(_task->m_state);
				_response += CHARPTR_WRAP("\",\"");
				appendCpuPercent(_response, taskCpuShare(_task, _now));
				_response += CHARPTR_WRAP("\"]");
			}

			_response += "]";
		}

		/**
		 * append every authenticated session with the terminal it arrived on.
		 */
		void appendSessionsJson(pdiutil::string &_response)
		{
			_response += CHARPTR_WRAP(",\"se\":[");

#ifdef ENABLE_AUTH_SERVICE
			uint32_t _now = (uint32_t)__i_dvc_ctrl.millis_now();
			bool _first = true;

			for (uint8_t _idx = 0; _idx < SessionManager::maxSessions(); _idx++)
			{
				session_t *_session = SessionManager::getByIndex(_idx);
				if (nullptr == _session || SESSION_STATE_FREE == _session->m_state) continue;
				if (!_session->m_isAuthorized) continue;

				appendSessionRow(_response, _first, _session->m_username.c_str(),
					terminalName(nullptr != _session->m_terminal ? _session->m_terminal->get_terminal_type() : TERMINAL_TYPE_MAX),
					_now - _session->m_loginAt, _now - _session->m_lastActivityAt);
			}

			for (uint8_t _idx = 0; _idx < WebSessionManager::maxSessions(); _idx++)
			{
				web_session_t *_session = __web_session_manager.getByIndex(_idx);
				if (nullptr == _session) continue;

				appendSessionRow(_response, _first, _session->m_username.c_str(), "web",
					_now - _session->m_loginAt, _now - _session->m_lastActivityAt);
			}
#endif

			_response += "]";
		}

		/**
		 * append the pins that are not switched off, and the connected stations.
		 */
		void appendGpioJson(pdiutil::string &_response)
		{
			_response += CHARPTR_WRAP(",\"gm\":");

#ifdef ENABLE_GPIO_SERVICE
			_response += pdiutil::to_string(ANALOG_GPIO_RESOLUTION);
#else
			_response += "0";
#endif

			_response += CHARPTR_WRAP(",\"gp\":[");

#ifdef ENABLE_GPIO_SERVICE
			bool _first = true;

			for (uint8_t _pin = 0; _pin < MAX_GPIO_PINS; _pin++)
			{
				if (OFF == __gpio_service.m_gpio_config_copy.gpio_mode[_pin]) continue;
				if (__i_dvc_ctrl.isExceptionalGpio(_pin)) continue;

				bool _analog = (_pin >= MAX_DIGITAL_GPIO_PINS);

				if (!_first) _response += ",";
				_first = false;

				_response += CHARPTR_WRAP("[\"");
				_response += _analog ? "A" : "D";
				_response += pdiutil::to_string(_analog ? (_pin - MAX_DIGITAL_GPIO_PINS) : _pin);
				_response += CHARPTR_WRAP("\",");
				_response += pdiutil::to_string((int32_t)__gpio_service.m_gpio_config_copy.gpio_mode[_pin]);
				_response += ",";
				_response += pdiutil::to_string((int32_t)__gpio_service.m_gpio_config_copy.gpio_readings[_pin]);
				_response += ",";
				_response += _analog ? "1" : "0";
				_response += "]";
			}
#endif

			_response += CHARPTR_WRAP("],\"dv\":[");

#ifdef ENABLE_WIFI_SERVICE
			pdiutil::vector<wifi_station_info_t> _stations;
			__i_wifi.getApsConnectedStations(_stations);

			for (uint32_t _idx = 0; _idx < _stations.size(); _idx++)
			{
				char _macstr[20];
				memset(_macstr, 0, sizeof(_macstr));
				__sprintf(_macstr, "%02X:%02X:%02X:%02X:%02X:%02X",
					_stations[_idx].bssid[0], _stations[_idx].bssid[1], _stations[_idx].bssid[2],
					_stations[_idx].bssid[3], _stations[_idx].bssid[4], _stations[_idx].bssid[5]);

				if (_idx > 0) _response += ",";
				_response += CHARPTR_WRAP("[\"");
				_response += _macstr;
				_response += CHARPTR_WRAP("\",\"");
				_response += pdiutil::to_string((uint8_t)_stations[_idx].ip4);
				_response += ".";
				_response += pdiutil::to_string((uint8_t)(_stations[_idx].ip4 >> 8));
				_response += ".";
				_response += pdiutil::to_string((uint8_t)(_stations[_idx].ip4 >> 16));
				_response += ".";
				_response += pdiutil::to_string((uint8_t)(_stations[_idx].ip4 >> 24));
				_response += CHARPTR_WRAP("\"]");
			}
#endif

			_response += "]";
		}

		/**
		 * handle dashboard monitor calls. it provides live dashboard parameters.
		 */
		void handleDashboardMonitor(void)
		{
			if (nullptr == this->m_web_resource || nullptr == this->m_web_resource->m_server || nullptr == this->m_route_handler)
			{
				return;
			}

			pdiutil::string *_response = pdiutil::safe_new<pdiutil::string>();
			if (nullptr == _response) return;

			_response->reserve(DASHBOARD_JSON_RESERVE);
			*_response = "{";

			this->appendNetworkJson(*_response);
			this->appendStorageJson(*_response);
			this->appendTasksJson(*_response);
			this->appendSessionsJson(*_response);
			this->appendGpioJson(*_response);

			*_response += "}";

			this->m_web_resource->m_server->addHeader(CHARPTR_WRAP_RO(HTTP_HEADER_KEY_CACHE_CONTROL), CHARPTR_WRAP_RO(HTTP_HEADER_VALUE_NO_CACHE));
			this->m_web_resource->m_server->send(HTTP_RESP_OK, MIME_TYPE_APPLICATION_JSON, _response->c_str());

			pdiutil::safe_delete(_response);
		}

		/**
		 * handle dashboard page route. it build & send dashboard html to client.
		 */
		void handleDashboardRoute(void)
		{
			LogI("Handling Dashboard route\n");

			char *_page = pdiutil::safe_new_array<char>(PAGE_HTML_MAX_SIZE);
			if (nullptr == _page) return;

			BEGIN_SEND_IN_CHUNK(HTTP_RESP_OK, MIME_TYPE_TEXT_HTML, _page);
			concat_header_html( _page, true );

			strcat_ro(_page, WEB_SERVER_DASHBOARD_STYLE);
			CONTINUE_SEND_IN_CHUNK(_page);
			strcat_ro(_page, WEB_SERVER_DASHBOARD_HEAD);
			CONTINUE_SEND_IN_CHUNK(_page);
			strcat_ro(_page, WEB_SERVER_DASHBOARD_STORAGE);
			CONTINUE_SEND_IN_CHUNK(_page);
			strcat_ro(_page, WEB_SERVER_DASHBOARD_TASKS);
			CONTINUE_SEND_IN_CHUNK(_page);
			strcat_ro(_page, WEB_SERVER_DASHBOARD_SESSIONS);
			CONTINUE_SEND_IN_CHUNK(_page);
			strcat_ro(_page, WEB_SERVER_DASHBOARD_NETWORK);
			CONTINUE_SEND_IN_CHUNK(_page);
			strcat_ro(_page, WEB_SERVER_DASHBOARD_GPIO);
			CONTINUE_SEND_IN_CHUNK(_page);

			strcat_ro(_page, WEB_SERVER_DASHBOARD_SCRIPT1);
			CONTINUE_SEND_IN_CHUNK(_page);
			strcat_ro(_page, WEB_SERVER_DASHBOARD_SCRIPT2);
			CONTINUE_SEND_IN_CHUNK(_page);
			strcat_ro(_page, WEB_SERVER_DASHBOARD_SCRIPT3);
			CONTINUE_SEND_IN_CHUNK(_page);

			strcat_ro(_page, WEB_SERVER_FOOTER_HTML);
			CONTINUE_SEND_IN_CHUNK(_page);

			END_SENDING_CHUNK();

			pdiutil::safe_delete_array(_page);
		}

	private:

		/**
		 * lifetime cpu share of a task, scaled by 100 so it stays integral.
		 */
		uint32_t taskCpuShare(task_t *_task, uint32_t _now)
		{
			uint64_t _elapsed = (_now > _task->m_created_ms) ? (_now - _task->m_created_ms) : 1;
			uint64_t _share = (_task->m_total_exec_us * 10ULL) / _elapsed;
			return (_share > 99999ULL) ? 99999 : (uint32_t)_share;
		}

		/**
		 * append a cpu share as a two decimal percentage.
		 */
		void appendCpuPercent(pdiutil::string &_response, uint32_t _share)
		{
			_response += pdiutil::to_string((int32_t)(_share / 100));
			_response += ".";
			if ((_share % 100) < 10) _response += "0";
			_response += pdiutil::to_string((int32_t)(_share % 100));
		}

		/**
		 * ps style state letter of a task.
		 */
		const char *taskStateLetter(task_state_t _state)
		{
			switch (_state)
			{
				case TASK_STATE_RUNNING:  return "R";
				case TASK_STATE_SLEEPING: return "S";
				case TASK_STATE_STOPPED:  return "T";
				case TASK_STATE_ZOMBIE:   return "Z";
				default:                  return "r";
			}
		}

#ifdef ENABLE_AUTH_SERVICE
		/**
		 * short name of the transport a session arrived on.
		 */
		const char *terminalName(terminal_types_t _type)
		{
			switch (_type)
			{
				case TERMINAL_TYPE_SERIAL: return "serial";
				case TERMINAL_TYPE_TELNET: return "telnet";
				case TERMINAL_TYPE_SSH:    return "ssh";
				default:                   return "-";
			}
		}

		/**
		 * append one session row and advance the separator state.
		 */
		void appendSessionRow(pdiutil::string &_response, bool &_first, const char *_user, const char *_via, uint32_t _loginms, uint32_t _idlems)
		{
			if (!_first) _response += ",";
			_first = false;

			_response += CHARPTR_WRAP("[\"");
			_response += (nullptr != _user && 0 != _user[0]) ? _user : "-";
			_response += CHARPTR_WRAP("\",\"");
			_response += _via;
			_response += CHARPTR_WRAP("\",");
			_response += pdiutil::to_string((int32_t)(_loginms / 1000));
			_response += ",";
			_response += pdiutil::to_string((int32_t)(_idlems / 1000));
			_response += "]";
		}
#endif
};

#endif
