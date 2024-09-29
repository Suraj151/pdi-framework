/********************************* ESPNOW  ************************************
This file is part of the Ewings Esp Stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st June 2019
******************************************************************************/
#ifndef _ESPNOW_H_
#define _ESPNOW_H_

#include "EspnowConfig.h"
#include "../WiFiInterface.h"

typedef struct
{
  uint8_t mesh_level;
  // struct global_config global_config;
  // uint8_t ssid[WIFI_CONFIGS_BUF_SIZE];
  // uint8_t pswd[WIFI_CONFIGS_BUF_SIZE];
  esp_now_device_t device_table[ESP_NOW_DEVICE_TABLE_MAX_SIZE];
  uint8_t device_count;
} esp_now_payload_t;

// esp_now_peer_t esp_now_peers[ESP_NOW_MAX_PEER];

void add_device_to_table(esp_now_device_t *_device);
int is_device_exist_in_table(esp_now_device_t *_device);
void remove_device_from_table(esp_now_device_t *_device);

void esp_now_encrypt_payload(uint8_t *payload, uint8_t len);
void esp_now_decrypt_payload(uint8_t *payload, uint8_t len);
void esp_now_recv_cb(uint8_t *macaddr, uint8_t *data, uint8_t len);
void esp_now_send_cb(uint8_t *macaddr, uint8_t status);

/**
 * Espnow class
 */
class Espnow
{

public:
  /**
   * Espnow constructor.
   */
  Espnow();
  /**
   * Espnow destructor.
   */
  ~Espnow();

  void begin(iWiFiInterface *_wifi);
  void handlePeers(void);
  void scanPeers(void);
  void printPeers(void);

  void registerCallbacks(void);
  void unregisterCallbacks(void);

  bool isPeerExist(uint8_t *mac_addr);
  bool isApPeer(uint8_t *mac_addr);
  bool setPeerRole(uint8_t *mac_addr, uint8_t role = ESP_NOW_ROLE_COMBO);
  bool addPeer(uint8_t *mac_addr, uint8_t role = ESP_NOW_ROLE_COMBO, uint8_t channel = ESP_NOW_CHANNEL, uint8_t *key = (uint8_t *)ESP_NOW_KEY, uint8_t key_len = ESP_NOW_KEY_LENGTH);
  bool addInPeers(uint8_t *mac_addr, uint8_t role = ESP_NOW_ROLE_COMBO, uint8_t channel = ESP_NOW_CHANNEL);
  void freePeers(void);

  bool sendToPeer(uint8_t *mac_addr, uint8_t *packet, uint8_t len);
  void receiveFromPeers(void);
  bool broadcastToPeers(uint8_t *packet, uint8_t len);
  bool broadcastToAll(uint8_t *packet, uint8_t len);
  void broadcastConfigData(void);

  void closeAll(void);
  bool deletePeer(uint8_t _peer_index);
  void flushPeersToDefaults(void);
  void setPeerToDefaults(uint8_t _peer_index);

protected:
  /**
   * @var	iWiFiInterface*  m_wifi
   */
  iWiFiInterface *m_wifi;
};

extern Espnow __espnow;

#endif
