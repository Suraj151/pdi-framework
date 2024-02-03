/******************************* ESPNOW service ********************************
This file is part of the Ewings Esp Stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st June 2019
******************************************************************************/
#include <config/Config.h>

#if defined(ENABLE_ESP_NOW)

#include "ESPNOWServiceProvider.h"

esp_now_peer_t esp_now_peers[ESP_NOW_MAX_PEER];
std::vector<esp_now_device_t> esp_now_device_table;

/**
 * ESPNOWServiceProvider constructor.
 */
ESPNOWServiceProvider::ESPNOWServiceProvider():
  m_wifi(nullptr)
{
  // this->flushPeersToDefaults();
}

/**
 * ESPNOWServiceProvider destructor.
 */
ESPNOWServiceProvider::~ESPNOWServiceProvider(){

  this->closeAll();
  this->m_wifi = nullptr;
}

void add_device_to_table(esp_now_device_t *_device){

  // #ifdef EW_SERIAL_LOG
  // Plain_Logln(F("espnow: adding device in table"));
  // #endif

  int _index = is_device_exist_in_table( _device );

  if( _index < 0 ){

    esp_now_device_t new_device;
    memcpy( &new_device, _device, sizeof(esp_now_device_t) );
    esp_now_device_table.push_back(new_device);
  }else{

    esp_now_device_table[_index].mesh_level = _device->mesh_level;
  }
}

int is_device_exist_in_table(esp_now_device_t *_device){

  // #ifdef EW_SERIAL_LOG
  // Plain_Logln(F("espnow: device exist"));
  // #endif

  for ( int i = 0; i < esp_now_device_table.size(); i++) {

    if( __are_arrays_equal( (char*)esp_now_device_table[i].mac, (char*)_device->mac, 6 ) ){

      return i;
    }
  }
  return -1;
}

void remove_device_from_table(esp_now_device_t *_device){

  #ifdef EW_SERIAL_LOG
  Plain_Logln(F("espnow: remove device in table"));
  #endif

  for ( uint16_t i = 0; i < esp_now_device_table.size(); i++) {

    if( __are_arrays_equal( (char*)esp_now_device_table[i].mac, (char*)_device->mac, 6 ) ){

      esp_now_device_table.erase( esp_now_device_table.begin() + i );
    }
  }
}



void esp_now_encrypt_payload(uint8_t *payload, uint8_t len){

  #ifdef EW_SERIAL_LOG
  Plain_Logln(F("espnow: encrypting payload"));
  #endif

  for (uint8_t i = 0; i < len; i++) {

    payload[i] = payload[i] + 1;
  }

}

void esp_now_decrypt_payload(uint8_t *payload, uint8_t len){

  #ifdef EW_SERIAL_LOG
  Plain_Logln(F("espnow: decrypting payload"));
  #endif

  for (uint8_t i = 0; i < len; i++) {

    payload[i] = payload[i] - 1;
  }

}

void esp_now_send_cb(uint8_t *macaddr, uint8_t status){

  #ifdef EW_SERIAL_LOG
  Plain_Log(F("\nespnow: in send cb : "));
  Plain_Log_format(macaddr[0],HEX);    Plain_Log_format(macaddr[1],HEX);
  Plain_Log_format(macaddr[2],HEX);    Plain_Log_format(macaddr[3],HEX);
  Plain_Log_format(macaddr[4],HEX);    Plain_Log_format(macaddr[5],HEX);
  Plain_Log(F(" : ")); Plain_Logln(status);
  #endif

  for (uint8_t i = 0; i < ESP_NOW_MAX_PEER; i++) {

    if( esp_now_peers[i].state == ESP_NOW_STATE_SENT &&
     __are_arrays_equal( (char*)esp_now_peers[i].mac, (char*)macaddr, 6 )
    ){

      if( status == 0 ){
        esp_now_peers[i].state = ESP_NOW_STATE_SENT_SUCCEED;
      }else{
        esp_now_peers[i].state = ESP_NOW_STATE_SENT_FAILED;
      }
      break;
    }
  }

}

void esp_now_recv_cb(uint8_t *macaddr, uint8_t *data, uint8_t len){

  esp_now_decrypt_payload(data,len);

  #ifdef EW_SERIAL_LOG
  Plain_Log(F("\nespnow: in recv cb : "));
  Plain_Log_format(macaddr[0],HEX);    Plain_Log_format(macaddr[1],HEX);
  Plain_Log_format(macaddr[2],HEX);    Plain_Log_format(macaddr[3],HEX);
  Plain_Log_format(macaddr[4],HEX);    Plain_Log_format(macaddr[5],HEX);
  Plain_Log(F(" : "));
  Plain_Log(len);

  // esp_now_payload_t* _payload = (esp_now_payload_t*)data;
  // Plain_Log(F("\nmesh : ")); Plain_Logln(_payload->mesh_level);
  // Plain_Log(F("config version : ")); Plain_Logln(_payload->global_config.config_version);
  // Plain_Log(F("current year : ")); Plain_Logln(_payload->global_config.current_year);
  // uint32_t fimr_version = 0;
  // memcpy(&fimr_version, &_payload->global_config.firmware_version, 4);
  // Plain_Log(F("firmware version : ")); Plain_Logln(fimr_version);
  // Plain_Log(F("ssid : ")); Plain_Logln((char*)_payload->ssid);
  // Plain_Log(F("pswd : ")); Plain_Logln((char*)_payload->pswd);

  Plain_Log(F(" : "));
  for (uint8_t i = 0; i < len; i++) {
    Plain_Log((char)data[i]);
  }
  Plain_Logln();
  #endif

  esp_now_device_t new_device;
  esp_now_payload_t* _payload = (esp_now_payload_t*)data;
  if( _payload->device_count < ESP_NOW_DEVICE_TABLE_MAX_SIZE ){
    for (uint8_t i = 0; i < _payload->device_count ; i++) {
      add_device_to_table( &_payload->device_table[i] );
    }
  }
  memcpy(&new_device.mac, macaddr, 6);
  new_device.mesh_level = _payload->mesh_level;
  add_device_to_table( &new_device );

  bool _found = false;
   for (uint8_t i = 0; i < ESP_NOW_MAX_PEER; i++) {

     if( __are_arrays_equal( (char*)esp_now_peers[i].mac, (char*)macaddr, 6 ) ){

       esp_now_peers[i].state = ESP_NOW_STATE_DATA_AVAILABLE;
       esp_now_peers[i].last_receive = __i_dvc_ctrl.millis_now();
       memset(esp_now_peers[i].buffer, 0, ESP_NOW_MAX_BUFF_SIZE);
       memcpy( esp_now_peers[i].buffer, data, len<ESP_NOW_MAX_BUFF_SIZE?len:ESP_NOW_MAX_BUFF_SIZE );
       esp_now_peers[i].data_length = len;
       _found = true;
       break;
     }
   }

   if( !_found ){

     for (uint8_t i = 0; i < ESP_NOW_MAX_PEER; i++) {

         if( esp_now_peers[i].state == ESP_NOW_STATE_EMPTY ){

           esp_now_peers[i].state = ESP_NOW_STATE_RECV_AVAILABLE;
           esp_now_peers[i].last_receive = __i_dvc_ctrl.millis_now();
           memcpy( esp_now_peers[i].mac, macaddr, 6 );
           esp_now_peers[i].buffer = new uint8_t[ESP_NOW_MAX_BUFF_SIZE];
           memset(esp_now_peers[i].buffer, 0, ESP_NOW_MAX_BUFF_SIZE);
           memcpy( esp_now_peers[i].buffer, data, len<ESP_NOW_MAX_BUFF_SIZE?len:ESP_NOW_MAX_BUFF_SIZE );
           esp_now_peers[i].data_length = len;
           break;
         }
     }
   }

}

void ESPNOWServiceProvider::registerCallbacks(void) {

  #ifdef EW_SERIAL_LOG
  Logln(F("espnow: registering callbacks"));
  #endif
  esp_now_register_send_cb(esp_now_send_cb);
  esp_now_register_recv_cb(esp_now_recv_cb);
}

void ESPNOWServiceProvider::unregisterCallbacks(void) {

  #ifdef EW_SERIAL_LOG
  Logln(F("espnow: unregistering callbacks"));
  #endif
  esp_now_unregister_send_cb();
  esp_now_unregister_recv_cb();
}

void ESPNOWServiceProvider::beginEspNow( iWiFiInterface* _wifi ){

  #ifdef EW_SERIAL_LOG
  Logln(F("espnow: begin"));
  #endif
  this->m_wifi= _wifi;

  this->flushPeersToDefaults();

  if( esp_now_init()==0 ){

    #ifdef EW_SERIAL_LOG
    Logln(F("espnow: init done"));
    #endif
    this->registerCallbacks();
    esp_now_set_kok((uint8_t*)ESP_NOW_KEY, ESP_NOW_KEY_LENGTH);
    esp_now_set_self_role(ESP_NOW_ROLE_COMBO);
  }else{

    this->closeAll();
    #ifdef EW_SERIAL_LOG
    Logln(F("espnow: init failed !"));
    #endif
  }
}

void ESPNOWServiceProvider::scanPeers(void) {

  if( nullptr == this->m_wifi ) return;

  uint8_t number_client= wifi_softap_get_station_num(); // Count of stations which are connected to ESP8266 soft-AP
  struct station_info * stat_info = wifi_softap_get_station_info();

  #ifdef ENABLE_NAPT_FEATURE
  struct ip_addr *IPaddress;
  #else
  struct ip4_addr *IPaddress;
  #endif
  IPAddress address;

  #ifdef EW_SERIAL_LOG
  Log(F("espnow: scanning.. client found = "));
  Logln(number_client);
  #endif

  int i=1;
  while (stat_info != NULL) {

    IPaddress = &stat_info->ip;
    address = IPaddress->addr;

    #ifdef EW_SERIAL_LOG
    Log(F("espnow: station : ")); Log(i); Log(F(" : "));
    Log((address)); Log(F(" : "));
    Log_format(stat_info->bssid[0],HEX);    Log_format(stat_info->bssid[1],HEX);
    Log_format(stat_info->bssid[2],HEX);    Log_format(stat_info->bssid[3],HEX);
    Log_format(stat_info->bssid[4],HEX);    Logln_format(stat_info->bssid[5],HEX);
    #endif

    uint8_t mac[6];
    memcpy(mac, stat_info->bssid, 6);
    mac[0] +=2;

    if( !this->isPeerExist(mac) ){

      this->addPeer(mac, ESP_NOW_ROLE_COMBO, (uint8_t)this->m_wifi->channel());
      // this->addPeer(mac, ESP_NOW_ROLE_COMBO, ESP_NOW_CHANNEL);
    }

    stat_info = STAILQ_NEXT(stat_info, next);
    i++;
  }

  if( this->m_wifi->isConnected() && this->m_wifi->localIP().isSet() ){

    uint8_t mac[6];
    memcpy(mac, this->m_wifi->BSSID(), 6);

    #ifdef EW_SERIAL_LOG
    Log(F("espnow: ap : "));
    Log_format(mac[0],HEX);    Log_format(mac[1],HEX);
    Log_format(mac[2],HEX);    Log_format(mac[3],HEX);
    Log_format(mac[4],HEX);    Logln_format(mac[5],HEX);
    #endif

    if( !this->isPeerExist(mac) ){

      this->addPeer(mac, ESP_NOW_ROLE_COMBO, (uint8_t)this->m_wifi->channel());
      // this->addPeer(mac, ESP_NOW_ROLE_COMBO, ESP_NOW_CHANNEL);
    }
  }

}

void ESPNOWServiceProvider::handlePeers(void) {

  if( nullptr == this->m_wifi ) return;

  #ifdef EW_SERIAL_LOG
  Log(F("\nespnow: handeling peers : "));
  Logln(this->m_wifi->channel());
  #endif
  this->freePeers();
  this->receiveFromPeers();
  this->scanPeers();
  this->printPeers();
  this->broadcastConfigData();
  // this->broadcastToPeers((uint8_t*)"Hello",5);
}

void ESPNOWServiceProvider::broadcastConfigData(void){

  esp_now_payload_t* payload = new esp_now_payload_t;

  if( nullptr != this->m_wifi && nullptr != payload ){

    memset( (void*)payload, 0, sizeof(esp_now_payload_t));

    WiFiMode_t _wifi_mode = (WiFiMode_t)this->m_wifi->getMode();
    if( WIFI_AP == _wifi_mode || WIFI_AP_STA == _wifi_mode ){

      payload->mesh_level = this->m_wifi->softAPIP()[2];
    }

    global_config_table _global_configs;
    wifi_config_table _wifi_configs;
    __database_service.get_global_config_table(&_global_configs);
    __database_service.get_wifi_config_table(&_wifi_configs);
    // memcpy( reinterpret_cast<global_config>(payload->global_config), reinterpret_cast<global_config>(_global_configs), sizeof(global_config_table) );
    memcpy( &payload->global_config, &_global_configs, sizeof(global_config_table) );
    memcpy( &payload->ssid, &_wifi_configs.sta_ssid, WIFI_CONFIGS_BUF_SIZE );
    memcpy( &payload->pswd, &_wifi_configs.sta_password, WIFI_CONFIGS_BUF_SIZE );
    // memcpy( &payload->device_table, &esp_now_device_table, esp_now_device_table.size()*sizeof(esp_now_device_t) );
    for (int i=0; i<ESP_NOW_DEVICE_TABLE_MAX_SIZE; i++){
      memset( &payload->device_table[i], 0, sizeof(esp_now_device_t) );
    }
    for (int i=0; i<esp_now_device_table.size(); i++){
      memcpy( &payload->device_table[i], &esp_now_device_table[i], sizeof(esp_now_device_t) );
    }
        // payload->device_table.push_back(esp_now_device_table[i]);
    payload->device_count = esp_now_device_table.size();

    esp_now_encrypt_payload((uint8_t*)payload,sizeof(esp_now_payload_t));
    this->broadcastToAll((uint8_t*)payload,sizeof(esp_now_payload_t));
    delete payload;
  }
}

void ESPNOWServiceProvider::freePeers(void) {

  for (uint8_t i = 0; i < ESP_NOW_MAX_PEER; i++) {

    if( esp_now_peers[i].state == ESP_NOW_STATE_SENT_FAILED ){
      this->deletePeer(i);
    }
  }
}

bool ESPNOWServiceProvider::isApPeer(uint8_t *mac_addr) {

  return ( nullptr != this->m_wifi ) &&
  this->m_wifi->isConnected() && this->m_wifi->localIP().isSet() && __are_arrays_equal( (char*)this->m_wifi->BSSID(), (char*)mac_addr, 6 );
}

void ESPNOWServiceProvider::printPeers(void) {

  #ifdef EW_SERIAL_LOG

  Logln(F("espnow: peer list"));
  for (uint8_t i = 0; i < ESP_NOW_MAX_PEER; i++) {

    if( esp_now_peers[i].state != ESP_NOW_STATE_EMPTY ){
      Log(i); Log(F(" : "));
      Log_format(esp_now_peers[i].mac[0],HEX);    Log_format(esp_now_peers[i].mac[1],HEX);
      Log_format(esp_now_peers[i].mac[2],HEX);    Log_format(esp_now_peers[i].mac[3],HEX);
      Log_format(esp_now_peers[i].mac[4],HEX);    Log_format(esp_now_peers[i].mac[5],HEX);   Log(F(" : "));
      Log(esp_now_peers[i].role); Log(F(" : "));
      Log(esp_now_peers[i].channel); Log(F(" : "));
      Log(esp_now_peers[i].state); Log(F(" : "));
      Log(esp_now_peers[i].last_receive); Log(F(" : "));
      for (uint8_t j = 0; j < esp_now_peers[i].data_length; j++) {
        Log((char)esp_now_peers[i].buffer[j]);
      }
      Logln();
    }
  }

  Logln(F("espnow: device list"));
  for ( uint16_t i = 0; i < esp_now_device_table.size(); i++) {

    Log(i); Log(F(" : "));
    Log(esp_now_device_table[i].mesh_level); Log(F(" : "));
    Log_format(esp_now_device_table[i].mac[0],HEX);    Log_format(esp_now_device_table[i].mac[1],HEX);
    Log_format(esp_now_device_table[i].mac[2],HEX);    Log_format(esp_now_device_table[i].mac[3],HEX);
    Log_format(esp_now_device_table[i].mac[4],HEX);    Log_format(esp_now_device_table[i].mac[5],HEX);
    Logln();
  }

  #endif
}

void ESPNOWServiceProvider::receiveFromPeers(void) {

  if( nullptr == this->m_wifi ) return;

  for (uint8_t i = 0; i < ESP_NOW_MAX_PEER; i++) {

    if( ESP_NOW_STATE_DATA_AVAILABLE == esp_now_peers[i].state || ESP_NOW_STATE_RECV_AVAILABLE == esp_now_peers[i].state ){

      #ifdef EW_SERIAL_LOG
      Log(F("espnow: recv from "));
      Log_format(esp_now_peers[i].mac[0],HEX);    Log_format(esp_now_peers[i].mac[1],HEX);
      Log_format(esp_now_peers[i].mac[2],HEX);    Log_format(esp_now_peers[i].mac[3],HEX);
      Log_format(esp_now_peers[i].mac[4],HEX);    Log_format(esp_now_peers[i].mac[5],HEX);
      Log(F(" : "));
      for (uint8_t j = 0; j < esp_now_peers[i].data_length; j++) {
        Log((char)esp_now_peers[i].buffer[j]);
      }
      Logln();
      #endif

      esp_now_peers[i].state=ESP_NOW_STATE_INIT;
      // memset(esp_now_peers[i].buffer, 0, ESP_NOW_MAX_BUFF_SIZE);
      if( ESP_NOW_STATE_RECV_AVAILABLE == esp_now_peers[i].state && !this->isPeerExist(esp_now_peers[i].mac) ){

        this->addPeer(esp_now_peers[i].mac, ESP_NOW_ROLE_COMBO, (uint8_t)this->m_wifi->channel());
      }
    }
  }
}

bool ESPNOWServiceProvider::sendToPeer(uint8_t *mac_addr, uint8_t *packet, uint8_t len) {

  uint8_t no_of_devices, no_of_encrypted_devices;
  esp_now_get_cnt_info(&no_of_devices,&no_of_encrypted_devices);
  if( 0 == no_of_devices ){
    return false;
  }

  #ifdef EW_SERIAL_LOG
  Log(F("espnow: sending to "));
  Log_format(mac_addr[0],HEX);    Log_format(mac_addr[1],HEX);
  Log_format(mac_addr[2],HEX);    Log_format(mac_addr[3],HEX);
  Log_format(mac_addr[4],HEX);    Log_format(mac_addr[5],HEX);
  Log(F(" : "));
  Logln((char*)packet);
  #endif

  int result =  esp_now_send(mac_addr, packet, len<ESP_NOW_MAX_BUFF_SIZE?len:ESP_NOW_MAX_BUFF_SIZE);
  if( 0 == result ){

    for (uint8_t i = 0; i < ESP_NOW_MAX_PEER; i++) {

      if( __are_arrays_equal( (char*)esp_now_peers[i].mac, (char*)mac_addr, 6 ) ){

        esp_now_peers[i].state=ESP_NOW_STATE_SENT;
        break;
      }
    }
    return true;
  }else{
    return false;
  }

}

bool ESPNOWServiceProvider::broadcastToPeers(uint8_t *packet, uint8_t len) {

  uint8_t no_of_devices, no_of_encrypted_devices;
  esp_now_get_cnt_info(&no_of_devices,&no_of_encrypted_devices);
  if( 0 == no_of_devices ){
    return false;
  }

  #ifdef EW_SERIAL_LOG
  Log(F("espnow: broadcasting to peer : "));
  Logln((char*)packet);
  #endif

  int result = esp_now_send(NULL, packet, len<ESP_NOW_MAX_BUFF_SIZE?len:ESP_NOW_MAX_BUFF_SIZE);
  if( 0 == result ){

    for (uint8_t i = 0; i < ESP_NOW_MAX_PEER; i++) {

        if( esp_now_peers[i].state != ESP_NOW_STATE_EMPTY && esp_now_peers[i].state != ESP_NOW_STATE_SENT_FAILED )
        esp_now_peers[i].state=ESP_NOW_STATE_SENT;
    }
    return true;
  }else{
    return false;
  }
}

bool ESPNOWServiceProvider::broadcastToAll(uint8_t *packet, uint8_t len) {

  #ifdef EW_SERIAL_LOG
  Log(F("espnow: broadcasting to all : "));
  Logln((char*)packet);
  #endif

  uint8_t broadcast_address[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

  int result = this->sendToPeer(broadcast_address, packet, len<ESP_NOW_MAX_BUFF_SIZE?len:ESP_NOW_MAX_BUFF_SIZE);
  if( 0 == result ){
    return true;
  }else{
    return false;
  }
}

bool ESPNOWServiceProvider::setPeerRole(uint8_t *mac_addr, uint8_t role) {

  #ifdef EW_SERIAL_LOG
  Log(F("espnow: settting role to "));
  Log_format(mac_addr[0],HEX);    Log_format(mac_addr[1],HEX);
  Log_format(mac_addr[2],HEX);    Log_format(mac_addr[3],HEX);
  Log_format(mac_addr[4],HEX);    Log_format(mac_addr[5],HEX);
  Log(F(" : "));
  Logln(role);
  #endif

  int result = esp_now_set_peer_role(mac_addr, role);
  if( 0 == result ){

    for (uint8_t i = 0; i < ESP_NOW_MAX_PEER; i++) {

      if( esp_now_peers[i].state != ESP_NOW_STATE_EMPTY &&
        __are_arrays_equal( (char*)esp_now_peers[i].mac, (char*)mac_addr, 6 )
      ){

        esp_now_peers[i].role=(esp_now_role)role;
        break;
      }
    }
    return true;
  }else{
    return false;
  }
}

bool ESPNOWServiceProvider::isPeerExist(uint8_t *mac_addr) {

  #ifdef EW_SERIAL_LOG
  Log(F("espnow: isExist : "));
  Log_format(mac_addr[0],HEX);    Log_format(mac_addr[1],HEX);
  Log_format(mac_addr[2],HEX);    Log_format(mac_addr[3],HEX);
  Log_format(mac_addr[4],HEX);    Log_format(mac_addr[5],HEX);
  Logln();
  #endif

  int exist = esp_now_is_peer_exist(mac_addr);
  if( exist>0 ){

    for (uint8_t i = 0; i < ESP_NOW_MAX_PEER; i++) {

        if( __are_arrays_equal( (char*)esp_now_peers[i].mac, (char*)mac_addr, 6 )  ){

          return true;
        }
    }
    #ifdef EW_SERIAL_LOG
    Logln(F("espnow: not exist adding"));
    #endif
    return this->addInPeers(mac_addr, esp_now_get_peer_role(mac_addr), esp_now_get_peer_channel(mac_addr));
  }else{
    return false;
  }
}

bool ESPNOWServiceProvider::addPeer(uint8_t *mac_addr, uint8_t role, uint8_t channel, uint8_t *key, uint8_t key_len) {

  uint8_t no_of_devices, no_of_encrypted_devices;
  esp_now_get_cnt_info(&no_of_devices,&no_of_encrypted_devices);
  if( no_of_devices >= ESP_NOW_MAX_PEER ) return false;

  #ifdef EW_SERIAL_LOG
  Log(F("espnow: adding peer : "));
  Log_format(mac_addr[0],HEX);    Log_format(mac_addr[1],HEX);
  Log_format(mac_addr[2],HEX);    Log_format(mac_addr[3],HEX);
  Log_format(mac_addr[4],HEX);    Log_format(mac_addr[5],HEX);
  Log(F(" : "));
  Logln(role);
  #endif

  int add = esp_now_add_peer(mac_addr, role, channel, key, key_len);
  if( add==0 ){

    return this->addInPeers(mac_addr, role, channel);
  }else{
    return false;
  }
}

bool ESPNOWServiceProvider::addInPeers(uint8_t *mac_addr, uint8_t role, uint8_t channel) {

    for (uint8_t i = 0; i < ESP_NOW_MAX_PEER; i++) {

        if( __are_arrays_equal( (char*)esp_now_peers[i].mac, (char*)mac_addr, 6 )  ){
          return true;
        }
    }

    for (uint8_t i = 0; i < ESP_NOW_MAX_PEER; i++) {

        if( esp_now_peers[i].state == ESP_NOW_STATE_EMPTY ){

          memcpy( esp_now_peers[i].mac, mac_addr, 6 );
          esp_now_peers[i].state=ESP_NOW_STATE_INIT;
          esp_now_peers[i].role=(esp_now_role)role;
          esp_now_peers[i].channel=channel;
          esp_now_peers[i].buffer = new uint8_t[ESP_NOW_MAX_BUFF_SIZE];
          memset(esp_now_peers[i].buffer, 0, ESP_NOW_MAX_BUFF_SIZE);
          esp_now_peers[i].data_length = 0;
          esp_now_peers[i].last_receive = __i_dvc_ctrl.millis_now();

          return true;
        }
    } return false;
}

void ESPNOWServiceProvider::closeAll(void) {

  #ifdef EW_SERIAL_LOG
  Logln(F("espnow: closing all"));
  #endif

    for (uint8_t i = 0; i < ESP_NOW_MAX_PEER; i++) {

      this->deletePeer(i);
    }
    this->unregisterCallbacks();
    esp_now_deinit();
}


bool ESPNOWServiceProvider::deletePeer(uint8_t _peer_index) {

  #ifdef EW_SERIAL_LOG
  Logln(F("espnow: deleting peer"));
  #endif

  if ( _peer_index < ESP_NOW_MAX_PEER &&
    esp_now_peers[_peer_index].state != ESP_NOW_STATE_EMPTY &&
    this->isPeerExist(esp_now_peers[_peer_index].mac)
  ) {


    int del = esp_now_del_peer(esp_now_peers[_peer_index].mac);
    if( del==0 ){

      // delete[] esp_now_peers[_peer_index].buffer;
      this->setPeerToDefaults(_peer_index);
    }else{
      return false;
    }
  } return true;
}

void ESPNOWServiceProvider::flushPeersToDefaults(void) {

  for (uint8_t i = 0; i < ESP_NOW_MAX_PEER; i++) {

    this->setPeerToDefaults(i);
  }
}

void ESPNOWServiceProvider::setPeerToDefaults(uint8_t _peer_index) {

  if ( _peer_index < ESP_NOW_MAX_PEER && nullptr != this->m_wifi ) {

      memset((char*)esp_now_peers[_peer_index].mac,0,6);
      esp_now_peers[_peer_index].role=ESP_NOW_ROLE_COMBO;
      esp_now_peers[_peer_index].channel=(uint8_t)this->m_wifi->channel();
      // esp_now_peers[_peer_index].channel=ESP_NOW_CHANNEL;
      esp_now_peers[_peer_index].state=ESP_NOW_STATE_EMPTY;

      if( esp_now_peers[_peer_index].buffer ) delete[] esp_now_peers[_peer_index].buffer;
      esp_now_peers[_peer_index].buffer=NULL;
      esp_now_peers[_peer_index].data_length = 0;
      esp_now_peers[_peer_index].last_receive = 0;
  }
}

ESPNOWServiceProvider __espnow_service;

#endif
