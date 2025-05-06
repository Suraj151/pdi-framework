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
				#ifdef ENABLE_WIFI_SERVICE
	      		this->m_route_handler->register_route( WEB_SERVER_DASHBOARD_MONITOR_ROUTE, [&]() { this->handleDashboardMonitor(); } );
				#endif
			}
		}

#ifdef ENABLE_WIFI_SERVICE
		/**
		 * handle dashboard monitor calls. it provides live dashborad parameters.
		 */
		void handleDashboardMonitor(void)
		{
			LogI("Handling dashboard monitor route\n");

			if (nullptr == this->m_web_resource || nullptr == this->m_web_resource->m_server || nullptr == this->m_route_handler)
			{
				return;
			}

			pdiutil::vector<wifi_station_info_t> stations;
			__i_wifi.getApsConnectedStations(stations);

			pdiutil::string _response = "{\"nm\":\"";
			_response += __i_wifi.SSID();
			_response += "\",\"ip\":\"";
			_response += (pdiutil::string)__i_wifi.localIP();
			_response += "\",\"rs\":\"";
			_response += pdiutil::to_string(__i_wifi.RSSI());
			_response += "\",\"mc\":\"";
			_response += __i_wifi.macAddress();
			_response += "\",\"st\":";
			_response += pdiutil::to_string((int32_t)__i_wifi.status());
			_response += ",\"nt\":";
			_response += pdiutil::to_string(__status_wifi.internet_available);
			_response += ",\"nwt\":";
			_response += pdiutil::to_string(__i_ntp.get_ntp_time());
			_response += ",\"dl\":\"";

			for (uint32_t sta_indx=0; sta_indx < stations.size(); sta_indx++)
			{
				char macStr[30];
				memset(macStr, 0, 30);
				sprintf(
					macStr,
					"%02X:%02X:%02X:%02X:%02X:%02X",
					stations[sta_indx].bssid[0], stations[sta_indx].bssid[1], stations[sta_indx].bssid[2], stations[sta_indx].bssid[3], stations[sta_indx].bssid[4], stations[sta_indx].bssid[5]);
				_response += "<tr><td>";
				_response += pdiutil::string(macStr);
				_response += "</td><td>";
				_response += pdiutil::to_string((uint8_t)stations[sta_indx].ip4);
				_response += ".";
				_response += pdiutil::to_string((uint8_t)(stations[sta_indx].ip4 >> 8));
				_response += ".";
				_response += pdiutil::to_string((uint8_t)(stations[sta_indx].ip4 >> 16));
				_response += ".";
				_response += pdiutil::to_string((uint8_t)(stations[sta_indx].ip4 >> 24));
				_response += "</td></tr>";
			}
			_response += "\",\"r\":";
			_response += pdiutil::to_string(!this->m_route_handler->has_active_session());
			_response += "}";

			this->m_web_resource->m_server->sendHeader("Cache-Control", "no-cache");
			this->m_web_resource->m_server->send(HTTP_OK, TEXT_HTML_CONTENT, _response.c_str());
		}
#endif		

		/**
		 * handle dashboard page route. it build & send dashboard html to client.
		 */
		void handleDashboardRoute(void)
		{
			LogI("Handling dashboard route\n");

			bool _is_posted = false;

			char *_page = new char[PAGE_HTML_MAX_SIZE];
			memset(_page, 0, PAGE_HTML_MAX_SIZE);

			strcat_ro(_page, WEB_SERVER_HEADER_HTML);
			strcat_ro(_page, WEB_SERVER_DASHBOARD_PAGE);
			strcat_ro(_page, WEB_SERVER_FOOTER_WITH_DASHBOARD_MONITOR_HTML);

			this->m_web_resource->m_server->send(HTTP_OK, TEXT_HTML_CONTENT, _page);
			delete[] _page;
		}
};

#endif
