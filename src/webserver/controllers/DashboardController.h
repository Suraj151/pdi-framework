/*************************** Dashboard Controller *****************************
This file is part of the Ewings Esp Stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st June 2019
******************************************************************************/

#ifndef _EW_SERVER_DASHBOARD_CONTROLLER_
#define _EW_SERVER_DASHBOARD_CONTROLLER_

#include "Controller.h"
#include <webserver/pages/Dashboard.h>

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
				this->m_route_handler->register_route( EW_SERVER_DASHBOARD_ROUTE, [&]() { this->handleDashboardRoute(); }, AUTH_MIDDLEWARE );
	      		this->m_route_handler->register_route( EW_SERVER_DASHBOARD_MONITOR_ROUTE, [&]() { this->handleDashboardMonitor(); } );
			}
		}

		/**
		 * handle dashboard monitor calls. it provides live dashborad parameters.
		 */
		void handleDashboardMonitor(void)
		{
			LogI("Handling dashboard monitor route\n");

			if (nullptr == this->m_web_resource || nullptr == this->m_web_resource->m_wifi || nullptr == this->m_route_handler)
			{
				return;
			}

			struct station_info *stat_info = wifi_softap_get_station_info();
			int n = 1;

			std::string _response = "{\"nm\":\"";
			_response += this->m_web_resource->m_wifi->SSID();
			_response += "\",\"ip\":\"";
			_response += this->m_web_resource->m_wifi->localIP().toString().c_str();
			_response += "\",\"rs\":\"";
			_response += std::to_string(this->m_web_resource->m_wifi->RSSI());
			_response += "\",\"mc\":\"";
			_response += this->m_web_resource->m_wifi->macAddress();
			_response += "\",\"st\":";
			_response += std::to_string(this->m_web_resource->m_wifi->status());
			_response += ",\"nt\":";
			_response += std::to_string(__status_wifi.internet_available);
			_response += ",\"nwt\":";
			_response += std::to_string(__i_ntp.get_ntp_time());
			_response += ",\"dl\":\"";

			while (nullptr != stat_info)
			{
				char macStr[30];
				memset(macStr, 0, 30);
				sprintf(
					macStr,
					"%02X:%02X:%02X:%02X:%02X:%02X",
					stat_info->bssid[0], stat_info->bssid[1], stat_info->bssid[2], stat_info->bssid[3], stat_info->bssid[4], stat_info->bssid[5]);
				_response += "<tr><td>";
				_response += std::string(macStr);
				_response += "</td><td>";
				_response += std::to_string((uint8_t)stat_info->ip.addr);
				_response += ".";
				_response += std::to_string((uint8_t)(stat_info->ip.addr >> 8));
				_response += ".";
				_response += std::to_string((uint8_t)(stat_info->ip.addr >> 16));
				_response += ".";
				_response += std::to_string((uint8_t)(stat_info->ip.addr >> 24));
				_response += "</td></tr>";
				stat_info = STAILQ_NEXT(stat_info, next);
			}
			_response += "\",\"r\":";
			_response += std::to_string(!this->m_route_handler->has_active_session());
			_response += "}";

			this->m_web_resource->m_server->sendHeader("Cache-Control", "no-cache");
			this->m_web_resource->m_server->send(HTTP_OK, EW_HTML_CONTENT, _response.c_str());
		}

		/**
		 * handle dashboard page route. it build & send dashboard html to client.
		 */
		void handleDashboardRoute(void)
		{
			LogI("Handling dashboard route\n");

			bool _is_posted = false;

			char *_page = new char[EW_HTML_MAX_SIZE];
			memset(_page, 0, EW_HTML_MAX_SIZE);

			strcat_P(_page, EW_SERVER_HEADER_HTML);
			strcat_P(_page, EW_SERVER_DASHBOARD_PAGE);
			strcat_P(_page, EW_SERVER_FOOTER_WITH_DASHBOARD_MONITOR_HTML);

			this->m_web_resource->m_server->send(HTTP_OK, EW_HTML_CONTENT, _page);
			delete[] _page;
		}
};

#endif
