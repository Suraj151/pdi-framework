/******************************** MQTT File ***********************************
This file is part of the Ewings Esp Stack. It is written with the reference
of https://github.com/tuanpmt/esp_mqtt


This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st June 2019
******************************************************************************/
#ifndef EW_MQTT_CLIENT_SERVICE_H
#define EW_MQTT_CLIENT_SERVICE_H

#include <interface/pdi.h>
#include "Mqtt_msg.h"

typedef enum {
	MQTT_PUBLISH_RECV,
	WIFI_CONNECTING_ERROR,
	MQTT_HOST_CONNECTING,
	MQTT_HOST_CONNECTED,
	MQTT_CONNECT_SENT,
	MQTT_CONNECT_FAILED,
	MQTT_DISCONNECT_REQ,
	MQTT_DISCONNECT_SENT,
	MQTT_DISCONNECT_FAILED,
	MQTT_DISCONNECTED,
	MQTT_DATA,
	MQTT_DATA_SENT,
	MQTT_DATA_FAILED,
	MQTT_KEEPALIVE_REQ,
	MQTT_PING_SENT,
	MQTT_PING_FAILED,
	MQTT_DELETING,
	MQTT_HOST_RECONNECT_REQ,
	MQTT_HOST_RECONNECT,
} tConnState;

#ifndef MQTT_DEFAULT_KEEPALIVE
#define MQTT_DEFAULT_KEEPALIVE  60     /*second*/
#endif

#ifndef MQTT_HOST_CONNECT_TIMEOUT
#define MQTT_HOST_CONNECT_TIMEOUT  5    /*second*/
#endif

#ifndef MQTT_DEFAULT_PORT
#define MQTT_DEFAULT_PORT       1883
#endif

#ifndef MQTT_MAX_QOS_LEVEL
#define MQTT_MAX_QOS_LEVEL      2
#endif

#define MQTT_BUF_SIZE           640
#define MQTT_CLIENT_READINTERVAL_MS	10

typedef struct {

  uint8_t type;
  char* topic;
  char* data;
  uint16_t topic_length;
  uint16_t data_length;
  uint16_t data_offset;
} mqtt_event_data_t;

typedef struct {

  mqtt_connect_info_t* connect_info;
  uint8_t* in_buffer;
  uint8_t* out_buffer;
  int in_buffer_length;
  int out_buffer_length;
  uint16_t message_length;
  uint16_t message_length_read;
  mqtt_message_t* outbound_message;
  mqtt_connection_t mqtt_connection;
  uint16_t pending_msg_id;
  int pending_msg_type;
  int pending_publish_qos;
} mqtt_state_t;

typedef void (*MqttCallback)(uint32_t *args);
typedef void (*MqttDataCallback)(uint32_t *args, const char* topic, uint32_t topic_len, const char *data, uint32_t lengh);

typedef struct  {
	ip_addr_t ip;
	mqtt_state_t  mqtt_state;
	mqtt_connect_info_t connect_info;
	uint32_t keepAliveTick;
	uint32_t sendTimeout;
	uint32_t readTimeout;
	tConnState connState;
  QUEUE msgQueue;
	uint16_t host_connect_tick;
	std::vector<mqtt_subscribed_topics_t> subscribed_topics;
	bool mqtt_connected;
} MQTT_Client;

void mqttConnectedCb(uint32_t *args);
void mqttDisconnectedCb(uint32_t *args);
void mqttPublishedCb(uint32_t *args);
void mqttSubscribedCb(uint32_t *args);
void mqttUnsubscribedCb(uint32_t *args);
void mqttDataCb(uint32_t *args, const char* topic, uint32_t topic_len, const char *data, uint32_t data_len);

class MQTTClient{

	public:

		MQTTClient();
		~MQTTClient();

		bool begin( iWiFiInterface *_wifi, mqtt_general_config_table *_mqtt_general_configs, mqtt_lwt_config_table *_mqtt_lwt_configs );
		void InitConnection( char* host, int port=MQTT_DEFAULT_PORT, uint8_t security=0 );
		void InitClient( char *client_id, char *client_user, char *client_pass, uint32_t keepAliveTime=MQTT_DEFAULT_KEEPALIVE, uint8_t cleanSession=1 );
		void InitLWT( char *will_topic, char *will_msg, uint8_t will_qos, uint8_t will_retain );

		void OnConnected( MqttCallback connectedCb );
		void OnDisconnected( MqttCallback disconnectedCb );
		void OnPublished( MqttCallback publishedCb );
		void OnSubscribed( MqttCallback subscribedCb );
		void OnUnsubscribed( MqttCallback unsubscribedCb );
		void OnTimeout( MqttCallback timeoutCb );
		void OnData( MqttDataCallback dataCb );

		bool Subscribe( char* topic, uint8_t qos );
		bool UnSubscribe( char* topic );
		void Connect( void );
		void Disconnect( void );
		bool Publish( const char *topic, const char *data, int data_length, int qos, int retain );
		void DeleteClient( void );

		void mqtt_timer( void );
		void MQTT_Task( void );

		bool is_mqtt_connected( void );
		bool is_topic_subscribed( char *_topic );
		void clear_all_subscribed_topics( void );
		void add_to_subscribed_topics( char *_topic, uint8_t _qos );
		bool remove_from_subscribed_topics( char *_topic );

		MQTT_Client 				m_mqttClient;
		uint32_t 						*m_mqttDataCallbackArgs;

	protected:

		char								*m_host;
		int 								m_port;
		uint8_t 						m_security;

		iWiFiClientInterface	*m_wifi_client;
		iWiFiInterface				*m_wifi;

		MqttCallback 				m_connectedCb;
		MqttCallback 				m_disconnectedCb;
		MqttCallback 				m_publishedCb;
		MqttCallback 				m_subscribedCb;
		MqttCallback 				m_unsubscribedCb;
		MqttCallback 				m_timeoutCb;
		MqttDataCallback 		m_dataCb;

		bool connectServer( void );
		bool disconnectServer( void );
		bool connected( void );
		bool sendPacket( uint8_t *buffer, uint16_t len );
		uint16_t readPacket( uint8_t *buffer, uint16_t maxlen, int16_t timeout );
		uint16_t readFullPacket( uint8_t *buffer, uint16_t maxsize, uint16_t timeout );

		void mqtt_client_delete( void );
		void mqtt_send_keepalive( void );
		void mqtt_client_connect( void );
		void mqtt_client_disconnect( void );
		// void mqtt_wificlient_delete( void );
		void mqtt_client_recv( void );
		void deliver_publish( uint8_t* message, int length );

};

#endif
