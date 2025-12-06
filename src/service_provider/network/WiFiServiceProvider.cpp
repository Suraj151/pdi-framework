/******************************* WiFi service *********************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st June 2019
******************************************************************************/
#include <config/Config.h>

#if defined(ENABLE_WIFI_SERVICE)

#include "WiFiServiceProvider.h"

__status_wifi_t __status_wifi = {
  false, false, 0, {0}
};

/**
 * WiFiServiceProvider constructor.
 */
WiFiServiceProvider::WiFiServiceProvider():
  m_wifi_connection_timeout(WIFI_STATION_CONNECT_ATTEMPT_TIMEOUT),
  m_wifi(nullptr),
  ServiceProvider(SERVICE_WIFI, RODT_ATTR("WiFi"))
{
  memset(m_temp_mac, 0, 6);
}

/**
 * WiFiServiceProvider destructor
 */
WiFiServiceProvider::~WiFiServiceProvider(){
  this->m_wifi = nullptr;
}

/**
 * Init wifi functionality
 */
bool WiFiServiceProvider::initService( void *arg ){

  if ( nullptr == arg ) {
    return false;
  }

  this->m_wifi = reinterpret_cast<iWiFiInterface*>(arg);
  wifi_config_table _wifi_credentials;
  __database_service.get_wifi_config_table( &_wifi_credentials );

  this->m_wifi->init();
  this->m_wifi->setAutoReconnect(false);
  this->configure_wifi_station( &_wifi_credentials );
  this->configure_wifi_access_point( &_wifi_credentials );

  // routine to check wifi and internet connectivity
  m_service_routine_task_id = __task_scheduler.updateInterval( m_service_routine_task_id, [&]() {
    this->handleWiFiConnectivity();
    #ifdef ENABLE_DYNAMIC_SUBNETTING
    this->reconfigure_wifi_access_point();
    #endif
    this->handleInternetConnectivity();
  }, WIFI_CONNECTIVITY_CHECK_DURATION, 0, __i_dvc_ctrl.millis_now() );

  // __task_scheduler.setInterval( [&]() {
  //   this->handleInternetConnectivity();
  // }, INTERNET_CONNECTIVITY_CHECK_DURATION, __i_dvc_ctrl.millis_now() );
  _ClearObject(&_wifi_credentials);

  // disconnect on factory reset event
  __utl_event.add_event_listener(EVENT_FACTORY_RESET, [&](void *e){
    this->m_wifi->disconnect(true);
  });

  // set AP MAC address which can be customized and recognizable easily
  uint8_t sta_mac[6];
	this->m_wifi->macAddress(sta_mac);
  sta_mac[0] +=2;
  this->m_wifi->setSoftAPmacAddress(sta_mac);

  return ServiceProvider::initService(arg);
}

/**
 * stop wifi service
 */
bool WiFiServiceProvider::stopService(){
  if( nullptr != this->m_wifi ){
    this->m_wifi->disconnect(true);
    this->m_wifi->softAPdisconnect(true);
  }
  return ServiceProvider::stopService();
}

/**
 * handle internet availability by ping function
 */
void WiFiServiceProvider::handleInternetConnectivity(){

  if ( nullptr == this->m_wifi ) {
    return;
  }

  if( !this->m_wifi->localIP().isSet() || !this->m_wifi->isConnected() ){

    memset( &__status_wifi, 0, sizeof(__status_wifi_t) );
    __status_wifi.last_internet_millis = __i_dvc_ctrl.millis_now();
  }else{

    bool ping_ret = __i_ping.ping();
    bool ping_resp = __i_ping.isHostRespondingToPing();
    bool ping_success = ping_ret && ping_resp;

    if( ping_success ){

      __status_wifi.last_internet_millis = __i_dvc_ctrl.millis_now();

      if( !__status_wifi.internet_available ){

        __utl_event.execute_event(EVENT_WIFI_INTERNET_UP);
        LogI("WiFi Internet Up\n");
        __task_scheduler.setTimeout( [&]() { this->m_wifi->enableNAPT(true); }, NAPT_INIT_DURATION_AFTER_WIFI_CONNECT, __i_dvc_ctrl.millis_now() );
      }  
    }else{

      if( __status_wifi.internet_available ){

        __utl_event.execute_event(EVENT_WIFI_INTERNET_DOWN);
        LogI("WiFi Internet Down\n");
        __task_scheduler.setTimeout( [&]() { this->m_wifi->enableNAPT(false); }, MILLISECOND_DURATION_1000, __i_dvc_ctrl.millis_now() );
      }  
    }

    __status_wifi.internet_available = ping_success;

    #ifdef ENABLE_INTERNET_BASED_CONNECTIONS
    if( !__status_wifi.internet_available && (__i_dvc_ctrl.millis_now()-__status_wifi.last_internet_millis) >= SWITCHING_DURATION_FOR_NO_INTERNET_CONNECTION ){

      memcpy( __status_wifi.ignore_bssid, this->m_wifi->BSSID(), 6 );
      this->m_wifi->disconnect(false);
      __status_wifi.last_internet_millis = __i_dvc_ctrl.millis_now();

      #ifndef IGNORE_FREE_RELAY_CONNECTIONS
      __task_scheduler.setTimeout( [&]() {
        this->m_wifi->scanNetworksAsync( [&](int _scanCount) {
          this->scan_aps_and_configure_wifi_station_async(_scanCount);
        }, false);
      }, 500, __i_dvc_ctrl.millis_now() );
      #endif

      __task_scheduler.setTimeout( [&]() {
        LogI("\nHandle station reconnecting...\n");
        memset( __status_wifi.ignore_bssid, 0, 6 );
        if( !this->m_wifi->localIP().isSet() || !this->m_wifi->isConnected() ){
          wifi_config_table _wifi_credentials;
          __database_service.get_wifi_config_table( &_wifi_credentials );
          this->configure_wifi_station( &_wifi_credentials );
          _ClearObject(&_wifi_credentials);
        }
      }, 2*INTERNET_CONNECTIVITY_CHECK_DURATION, __i_dvc_ctrl.millis_now() );
    }
    #endif
  }

  LogFmtI("\nHandeling internet connectivity : %d : %d\n", (int)__status_wifi.internet_available, 
  (__i_dvc_ctrl.millis_now()-__status_wifi.last_internet_millis) );
}

/**
 * return station subnet ip address
 *
 * @return  uint32_t
 */
uint32_t WiFiServiceProvider::getStationSubnetIP(void){

  uint32_t _subnet_ip4 = 0;
  if( nullptr != this->m_wifi && this->m_wifi->isConnected() && this->m_wifi->localIP().isSet() ){

    _subnet_ip4 = (uint32_t)this->m_wifi->subnetMask()&(uint32_t)this->m_wifi->localIP();
  }
  return _subnet_ip4;
}

/**
 * return station broadcast ip address
 *
 * @return  uint32_t
 */
uint32_t WiFiServiceProvider::getStationBroadcastIP(void){

  uint32_t _broadcast_ip4 = this->getStationSubnetIP();
  if( _broadcast_ip4 && nullptr != this->m_wifi ){

    uint32_t _no_of_ips = ~(uint32_t)this->m_wifi->subnetMask();
    _broadcast_ip4 += (_no_of_ips - 1);
  }
  return _broadcast_ip4;
}

/**
 * configure and start wifi station functionality
 *
 * @param   wifi_config_table* _wifi_credentials
 * @return  bool
 */
bool WiFiServiceProvider::configure_wifi_station( wifi_config_table* _wifi_credentials, uint8_t* mac ){

  if( nullptr == this->m_wifi || nullptr == _wifi_credentials ){
    return false;
  }

  LogFmtI("\nWiFi Connecing To %s", _wifi_credentials->sta_ssid);

  if (nullptr != mac)
  {
    LogFmtI(" : %x%x%x%x%x%x",
            mac[0], mac[1], mac[2],
            mac[3], mac[4], mac[5]);
  }

  ipaddress_t local_IP(
    _wifi_credentials->sta_local_ip[0],_wifi_credentials->sta_local_ip[1],_wifi_credentials->sta_local_ip[2],_wifi_credentials->sta_local_ip[3]
  );
  ipaddress_t gateway(
    _wifi_credentials->sta_gateway[0],_wifi_credentials->sta_gateway[1],_wifi_credentials->sta_gateway[2],_wifi_credentials->sta_gateway[3]
  );
  ipaddress_t subnet(
    _wifi_credentials->sta_subnet[0],_wifi_credentials->sta_subnet[1],_wifi_credentials->sta_subnet[2],_wifi_credentials->sta_subnet[3]
  );

  this->m_wifi->enableSTA(true);
  this->m_wifi->config( local_IP, gateway, subnet );
  this->m_wifi->begin(_wifi_credentials->sta_ssid, _wifi_credentials->sta_password, 0, mac);

  uint8_t _wait = 1;
  while ( ! this->m_wifi->isConnected() ) {

    __i_dvc_ctrl.wait(999);
    if( _wait%7 == 0 ){
      LogI("\ntrying reconnect");
      this->m_wifi->reconnect();
    }
    if( _wait++ > this->m_wifi_connection_timeout ){
      break;
    }
    LogI(".");
  }

  LogI("\n");

  wifi_status_t stat = this->m_wifi->status();
  if( CONN_STATUS_CONNECTED == stat ){
    LogFmtI("WiFi Connected to %s\n", _wifi_credentials->sta_ssid);
    LogFmtI("IP address: %s\n", ((pdiutil::string)this->m_wifi->localIP()).c_str());
    // this->m_wifi->setAutoConnect(true);
    // this->m_wifi->setAutoReconnect(true);
    return true;
  }else if( CONN_STATUS_NOT_AVAILABLE == stat ){
    LogFmtW("%s Not Found/reachable. Make sure it's availability.\n", _wifi_credentials->sta_ssid);
  }else if( CONN_STATUS_CONNECTION_FAILED == stat || CONN_STATUS_CONFIG_ERROR == stat ){
    LogFmtW("%s is available but not connecting. Please check password.\n", _wifi_credentials->sta_ssid);
  }else{
    LogW("WiFi Not Connecting. Will try later soon..\n");
  }
  return false;
}

#ifdef ENABLE_DYNAMIC_SUBNETTING
/**
 * reconfigure and start wifi access point functionality
 *
 */
void WiFiServiceProvider::reconfigure_wifi_access_point( void ){

  if( nullptr == this->m_wifi ){
    return;
  }

  LogI("Handeling reconfigure WiFi AP.\n");

  wifi_config_table _wifi_credentials;
  __database_service.get_wifi_config_table( &_wifi_credentials );
  bool _ap_change = false;

  if( !this->m_wifi->localIP().isSet() || !this->m_wifi->isConnected() ){

    // if( __are_arrays_equal( (char*)_wifi_credentials.ap_local_ip, (char*)DEFAULT_AP_LOCAL_IP, 4 ) ){
    if(
      _wifi_credentials.ap_local_ip[0] != DEFAULT_AP_LOCAL_IP[0] ||
      _wifi_credentials.ap_local_ip[1] != DEFAULT_AP_LOCAL_IP[1] ||
      _wifi_credentials.ap_local_ip[2] != DEFAULT_AP_LOCAL_IP[2] ||
      _wifi_credentials.ap_local_ip[3] != DEFAULT_AP_LOCAL_IP[3]
    ){

      memcpy( _wifi_credentials.ap_local_ip, DEFAULT_AP_LOCAL_IP, 4);
      memcpy( _wifi_credentials.ap_gateway, DEFAULT_AP_GATEWAY, 4);

      __database_service.set_wifi_config_table(&_wifi_credentials);
      _ap_change = true;
    }
  }else{

    ipaddress_t gateway_IP = this->m_wifi->gatewayIP();
    ipaddress_t sta_subnet_ip = ipaddress_t(this->getStationSubnetIP());

    if(
      (( gateway_IP[3] - sta_subnet_ip[3] ) == 1 &&
      ( sta_subnet_ip[0] == DEFAULT_AP_GATEWAY[0] ) &&
      ( sta_subnet_ip[1] == DEFAULT_AP_GATEWAY[1] )) &&
      (( _wifi_credentials.ap_gateway[2] - sta_subnet_ip[2] ) != 1 ||
      ( _wifi_credentials.ap_gateway[3] != sta_subnet_ip[3]+1 ))
    ){

      _wifi_credentials.ap_local_ip[0] = sta_subnet_ip[0];
      _wifi_credentials.ap_local_ip[1] = sta_subnet_ip[1];
      _wifi_credentials.ap_local_ip[2] = (sta_subnet_ip[2]+1) < 255 ? (sta_subnet_ip[2]+1) : DEFAULT_AP_LOCAL_IP[2];
      _wifi_credentials.ap_local_ip[3] = sta_subnet_ip[3]+1;

      memcpy( _wifi_credentials.ap_gateway, _wifi_credentials.ap_local_ip, 4 );

      __database_service.set_wifi_config_table(&_wifi_credentials);
      _ap_change = true;
    }
  }

  if( _ap_change ){
    LogI("reconfiguring....\n");

    this->m_wifi->softAPdisconnect(false);
    // this->m_wifi->enableAP(false);

    __task_scheduler.setTimeout( [&](){
      wifi_config_table __wifi_credentials;
      __database_service.get_wifi_config_table(&__wifi_credentials);
      this->configure_wifi_access_point(&__wifi_credentials);
      _ClearObject(&__wifi_credentials);
    }, 1, __i_dvc_ctrl.millis_now());
  }
  _ClearObject(&_wifi_credentials);
}
#endif

/**
 * configure and start wifi access point functionality
 *
 * @param   wifi_config_table* _wifi_credentials
 * @return  bool
 */
bool WiFiServiceProvider::configure_wifi_access_point( wifi_config_table* _wifi_credentials ){

  if( nullptr == this->m_wifi || nullptr == _wifi_credentials ){
    return false;
  }

  LogFmtI("Configuring WiFi access point %s..\n", _wifi_credentials->ap_ssid);

  ipaddress_t local_IP(
    _wifi_credentials->ap_local_ip[0],_wifi_credentials->ap_local_ip[1],_wifi_credentials->ap_local_ip[2],_wifi_credentials->ap_local_ip[3]
  );
  ipaddress_t gateway(
    _wifi_credentials->ap_gateway[0],_wifi_credentials->ap_gateway[1],_wifi_credentials->ap_gateway[2],_wifi_credentials->ap_gateway[3]
  );
  ipaddress_t subnet(
    _wifi_credentials->ap_subnet[0],_wifi_credentials->ap_subnet[1],_wifi_credentials->ap_subnet[2],_wifi_credentials->ap_subnet[3]
  );

  this->m_wifi->enableAP(true);

  if( this->m_wifi->softAPConfig( local_IP, gateway, subnet ) &&
    this->m_wifi->softAP( _wifi_credentials->ap_ssid, _wifi_credentials->ap_password, 1, 0, 8 )
  ){
    LogFmtI("AP IP address: %s\n", ((pdiutil::string)this->m_wifi->softAPIP()).c_str());
    return true;
  }else{
    LogE("Configuring WiFi access point failed!\n");
    return false;
  }
}

#ifndef IGNORE_FREE_RELAY_CONNECTIONS
/**
 * scan connected stations then configure and start wifi station functionality
 *
 * @param   int _scanCount
 */
void WiFiServiceProvider::scan_aps_and_configure_wifi_station_async( int _scanCount ){

  LogFmtI("Scanning AP's and configuring stations. stations count is %d\n", _scanCount);

  wifi_config_table _wifi_credentials;
  __database_service.get_wifi_config_table(&_wifi_credentials);
  if( this->m_wifi->get_bssid_within_scanned_nw_ignoring_connected_stations( _wifi_credentials.sta_ssid, this->m_temp_mac, __status_wifi.ignore_bssid, _scanCount ) ) {
    __task_scheduler.setTimeout([&](){
      wifi_config_table __wifi_credentials;
      __database_service.get_wifi_config_table(&__wifi_credentials);
      this->configure_wifi_station( &__wifi_credentials, this->m_temp_mac );
      _ClearObject(&__wifi_credentials);
    }, 1, __i_dvc_ctrl.millis_now());
  }
  _ClearObject(&_wifi_credentials);
}
#endif

/**
 * check wifi connectivity after each wifi activity cycle. try to reconnect if failed
 */
void WiFiServiceProvider::handleWiFiConnectivity(){

  if( nullptr == this->m_wifi ){
    return;
  }

  LogI("\nHandeling WiFi Connectivity\n");

  if( !this->m_wifi->localIP().isSet() || !this->m_wifi->isConnected() ){

    #ifdef IGNORE_FREE_RELAY_CONNECTIONS
    this->m_wifi->reconnect();
    #else

    pdiutil::vector<wifi_station_info_t> stations;
    this->m_wifi->getApsConnectedStations(stations);

    LogFmtI("Handeling WiFi Reconnect Manually : %s : Connected Stations Count : %d\n", ((pdiutil::string)this->m_wifi->softAPIP()).c_str(), stations.size());

    if( stations.size() > 0 ){
      this->m_wifi->scanNetworksAsync( [&](int _scanCount) {
        this->scan_aps_and_configure_wifi_station_async(_scanCount);
      }, false);
    }else{
      this->m_wifi->reconnect();
    }
    #endif
    __status_wifi.wifi_connected = false;
  }else{
    __status_wifi.wifi_connected = true;
    LogFmtI("IP address: %s : %s : %s\n", 
    ((pdiutil::string)this->m_wifi->gatewayIP()).c_str(), 
    ((pdiutil::string)this->m_wifi->localIP()).c_str(), 
    ((pdiutil::string)this->m_wifi->softAPIP()).c_str());
  }
}

/**
 * print wifi configs to terminal
 */
void WiFiServiceProvider::printConfigToTerminal(iTerminalInterface *terminal)
{
  if( nullptr != terminal ){

    wifi_config_table _table;
    __database_service.get_wifi_config_table(&_table);
    char _ip[20];

    terminal->writeln();
    terminal->writeln_ro(RODT_ATTR("Station Configs :"));
    terminal->write(_table.sta_ssid); terminal->write_ro(RODT_ATTR("\t"));
    terminal->write(_table.sta_password); terminal->write_ro(RODT_ATTR("\t"));

    __int_ip_to_str( _ip, _table.sta_local_ip, 20 ); terminal->write(_ip); terminal->write_ro(RODT_ATTR("\t"));
    __int_ip_to_str( _ip, _table.sta_gateway, 20 ); terminal->write(_ip); terminal->write_ro(RODT_ATTR("\t"));
    __int_ip_to_str( _ip, _table.sta_subnet, 20 ); terminal->write(_ip); terminal->write_ro(RODT_ATTR("\t\n"));

    terminal->writeln();
    terminal->writeln_ro(RODT_ATTR("Access Configs :"));
    terminal->write(_table.ap_ssid); terminal->write_ro(RODT_ATTR("\t"));
    terminal->write(_table.ap_password); terminal->write_ro(RODT_ATTR("\t"));

    __int_ip_to_str( _ip, _table.ap_local_ip, 20 ); terminal->write(_ip); terminal->write_ro(RODT_ATTR("\t"));
    __int_ip_to_str( _ip, _table.ap_gateway, 20 ); terminal->write(_ip); terminal->write_ro(RODT_ATTR("\t"));
    __int_ip_to_str( _ip, _table.ap_subnet, 20 ); terminal->writeln(_ip);

    terminal->writeln();
    terminal->write_ro(RODT_ATTR("STA MAC :"));
    terminal->writeln(this->m_wifi->macAddress().c_str());

    terminal->writeln();
    terminal->write_ro(RODT_ATTR("AP MAC :"));
    terminal->writeln(this->m_wifi->softAPmacAddress().c_str());
  }
}

/**
 * print service status to terminal
 */
void WiFiServiceProvider::printStatusToTerminal(iTerminalInterface *terminal){

  if( nullptr != terminal ){

    pdiutil::string stname = NOT_APPLICABLE;
    pdiutil::string apname = NOT_APPLICABLE;

    terminal->writeln();

    wifi_config_table _table;
    __database_service.get_wifi_config_table(&_table);
    apname = _table.ap_ssid;
    stname = _table.sta_ssid;

    if( __status_wifi.wifi_connected ){
      
      stname = this->m_wifi->SSID();

      terminal->writeln_ro((RODT_ATTR("station : ")));

      terminal->write_ro((RODT_ATTR("         ")));
      terminal->write_ro((RODT_ATTR("ssid - ")));
      terminal->writeln(stname.c_str());

      terminal->write_ro((RODT_ATTR("         ")));
      terminal->write_ro((RODT_ATTR("ip - ")));
      terminal->writeln(((pdiutil::string)this->m_wifi->localIP()).c_str());

      terminal->write_ro((RODT_ATTR("         ")));
      terminal->write_ro((RODT_ATTR("gateway - ")));
      terminal->writeln(((pdiutil::string)this->m_wifi->gatewayIP()).c_str());

      terminal->write_ro((RODT_ATTR("         ")));
      terminal->write_ro((RODT_ATTR("bssid - ")));
      uint8_t *bssid = this->m_wifi->BSSID();
      if(nullptr != bssid){
        char macstr[36] = {0};
        snprintf(macstr, sizeof(macstr), "%02X:%02X:%02X:%02X:%02X:%02X", bssid[0], bssid[1], bssid[2], bssid[3], bssid[4], bssid[5]);
        terminal->writeln(macstr);
      }else{
        terminal->putln();
      }

      terminal->write_ro((RODT_ATTR("         ")));
      terminal->write_ro((RODT_ATTR("rssi - ")));
  		terminal->writeln((int32_t)this->m_wifi->RSSI());

      terminal->write_ro((RODT_ATTR("         ")));
      terminal->write_ro((RODT_ATTR("netstatus - ")));
  		terminal->writeln((int32_t)__status_wifi.internet_available);

    }else{

      terminal->write_ro((RODT_ATTR("station : failing to connect \"")));
      terminal->write(stname.c_str());
      terminal->writeln_ro((RODT_ATTR("\", will retry soon. make sure it's availability OR reconfigure.")));
    }

    terminal->putln();

    terminal->writeln_ro((RODT_ATTR("accespoint : ")));
    
    terminal->write_ro((RODT_ATTR("         ")));
    terminal->write_ro((RODT_ATTR("ssid - ")));
    terminal->writeln(apname.c_str());
    
    terminal->write_ro((RODT_ATTR("         ")));
    terminal->write_ro((RODT_ATTR("ip - ")));
    terminal->writeln(((pdiutil::string)this->m_wifi->softAPIP()).c_str());

    terminal->write_ro((RODT_ATTR("         ")));
    terminal->write_ro((RODT_ATTR("bssid - ")));
    terminal->writeln(this->m_wifi->softAPmacAddress().c_str());
  }
}


WiFiServiceProvider __wifi_service;

#endif