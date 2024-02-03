/******************************** MQTT File ***********************************
This file is part of the Ewings Esp Stack. It is written with the reference
of https://github.com/tuanpmt/esp_mqtt


This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st June 2019
******************************************************************************/
#include <config/Config.h>

#if defined(ENABLE_MQTT_SERVICE)

#include "Mqtt.h"

#define MQTT_SEND_TIMEOUT           5
#define MQTT_READ_TIMEOUT           10

#ifndef QUEUE_BUFFER_SIZE
#define QUEUE_BUFFER_SIZE           2048
#endif

void mqttConnectedCb( uint32_t *args ){

    #ifdef EW_SERIAL_LOG
    Logln(F("MQTT: connected cb"));
    #endif
    MQTTClient* _client = reinterpret_cast<MQTTClient*>(args);
    if( nullptr != _client ){
      _client->m_mqttClient.mqtt_connected = true;
    }
}

void mqttDisconnectedCb( uint32_t *args ){

    #ifdef EW_SERIAL_LOG
    Logln(F("MQTT: disconnect cb"));
    #endif
    MQTTClient* _client = reinterpret_cast<MQTTClient*>(args);

    if( nullptr != _client ){
      _client->m_mqttClient.mqtt_connected = false;
      _client->clear_all_subscribed_topics();
    }
}

void mqttPublishedCb( uint32_t *args ){

    #ifdef EW_SERIAL_LOG
    Logln(F("MQTT: published cb"));
    #endif
}

void mqttSubscribedCb( uint32_t *args ){

    #ifdef EW_SERIAL_LOG
    Logln(F("MQTT: subscribed cb"));
    #endif
}

void mqttUnsubscribedCb( uint32_t *args ){

    #ifdef EW_SERIAL_LOG
    Logln(F("MQTT: unsubscribed cb"));
    #endif
}

void mqttDataCb( uint32_t *args, const char *topic, uint32_t topic_len, const char *data, uint32_t data_len){

    char *topicBuf = new char[topic_len+1], *dataBuf = new char[data_len+1];

    if( nullptr != topicBuf ){

      memcpy(topicBuf, topic, topic_len);
      topicBuf[topic_len] = 0;
    }
    if( nullptr != dataBuf ){

      memcpy(dataBuf, data, data_len);
      dataBuf[data_len] = 0;
    }

    if( nullptr != topicBuf && nullptr != dataBuf ){

      #ifdef EW_SERIAL_LOG
      Logln(F("MQTT: data cb"));
      Serial.printf("MQTT: Receive topic: %s, data: %s \r\n", topicBuf, dataBuf);
      #endif
    }

    if( nullptr != topicBuf ){

      delete[] topicBuf;
    }
    if( nullptr != dataBuf ){

      delete[] dataBuf;
    }
}


bool MQTTClient::is_mqtt_connected(){

  if(
    this->m_mqttClient.connState == WIFI_CONNECTING_ERROR ||
    this->m_mqttClient.connState == MQTT_DELETING ||
    this->m_mqttClient.connState == MQTT_DISCONNECTED
  ) {
    return false;
  }
  return ( this->connected() && this->m_mqttClient.mqtt_connected );
}

bool MQTTClient::is_topic_subscribed( char *_topic ){

  for ( uint16_t i = 0; i < this->m_mqttClient.subscribed_topics.size(); i++) {

    if( nullptr != this->m_mqttClient.subscribed_topics[i].topic ){
      if( __are_str_equals( this->m_mqttClient.subscribed_topics[i].topic, _topic, strlen( this->m_mqttClient.subscribed_topics[i].topic ) ) ){
        return true;
      }
    }
  }
  return false;
}

void MQTTClient::clear_all_subscribed_topics(){

  for ( uint16_t i = 0; i < this->m_mqttClient.subscribed_topics.size(); i++) {

    if( nullptr != this->m_mqttClient.subscribed_topics[i].topic ){
      delete[] this->m_mqttClient.subscribed_topics[i].topic;
      this->m_mqttClient.subscribed_topics[i].topic = nullptr;
    }
  }
  this->m_mqttClient.subscribed_topics.clear();
}

void MQTTClient::add_to_subscribed_topics( char *_topic, uint8_t _qos ){

  int _len = strlen(_topic);
  mqtt_subscribed_topics_t _subscribe_topic;
  _subscribe_topic.topic = new char[_len + 1];
  if( nullptr != _subscribe_topic.topic ){

    memset( _subscribe_topic.topic, 0, _len+1 );
    strcpy( _subscribe_topic.topic, _topic);
    _subscribe_topic.topic[_len] = 0;
    _subscribe_topic.qos = _qos;

    this->m_mqttClient.subscribed_topics.push_back( _subscribe_topic );
  }
}

bool MQTTClient::remove_from_subscribed_topics( char *_topic ){

  for ( uint16_t i = 0; i < this->m_mqttClient.subscribed_topics.size(); i++) {

    if( nullptr != this->m_mqttClient.subscribed_topics[i].topic ){

      if( __are_str_equals( this->m_mqttClient.subscribed_topics[i].topic, _topic, strlen( this->m_mqttClient.subscribed_topics[i].topic ) ) ){

        delete []this->m_mqttClient.subscribed_topics[i].topic;
        this->m_mqttClient.subscribed_topics[i].topic = nullptr;
        this->m_mqttClient.subscribed_topics.erase( this->m_mqttClient.subscribed_topics.begin() + i );
        return true;
      }
    }
  }
  return false;
}


void MQTTClient::deliver_publish(  uint8_t *message, int length ){

    mqtt_event_data_t event_data;

    event_data.topic_length = length;
    event_data.topic = (char*)mqtt_get_publish_topic(message, &event_data.topic_length);
    event_data.data_length = length;
    event_data.data = (char*)mqtt_get_publish_data(message, &event_data.data_length);

    if ( nullptr != this->m_dataCb && nullptr != event_data.topic && nullptr != event_data.data ){

      this->m_dataCb( this->m_mqttDataCallbackArgs, event_data.topic, event_data.topic_length, event_data.data, event_data.data_length);
    }else{

      mqttDataCb( this->m_mqttDataCallbackArgs, event_data.topic, event_data.topic_length, event_data.data, event_data.data_length);
    }
}

// void mqtt_wificlient_delete( MQTT_Client *m_mqttClient ){
//
//     if ( m_mqttClient->m_wifi_client != nullptr) {
//         #ifdef EW_SERIAL_LOG
//         Logln(F("MQTT: deleting wifi Client"));
//         #endif
//         m_mqttClient->m_wifi_client->stopAll();
//         delete m_mqttClient->m_wifi_client;
//         m_mqttClient->m_wifi_client = nullptr;
//     }
// }

void MQTTClient::mqtt_client_recv(){

    if( nullptr == this->m_mqttClient.mqtt_state.in_buffer ){
      return;
    }

    int len = this->readFullPacket( this->m_mqttClient.mqtt_state.in_buffer, this->m_mqttClient.mqtt_state.in_buffer_length, MQTT_READ_TIMEOUT*10 );
    #ifdef EW_SERIAL_LOG
    Log(F("MQTT: recieved packet size :"));Logln(len);
    Log(F("MQTT: recieved packets :"));
    for (int i = 0; i < len; i++) {
      Log_format( this->m_mqttClient.mqtt_state.in_buffer[i], HEX ); Log(" ");
      __i_dvc_ctrl.wait(0);
    }
    Logln();
    #endif
    __i_dvc_ctrl.wait(0);

    if ( len < MQTT_BUF_SIZE && len > 0 ) {

        uint8_t msg_type = mqtt_get_type(this->m_mqttClient.mqtt_state.in_buffer);
        uint8_t msg_qos = mqtt_get_qos(this->m_mqttClient.mqtt_state.in_buffer);
        uint16_t msg_id = mqtt_get_id(this->m_mqttClient.mqtt_state.in_buffer, this->m_mqttClient.mqtt_state.in_buffer_length);

        if( !( MQTT_MSG_TYPE_PUBLISH == msg_type && 0 == msg_qos ) ){

          this->m_mqttClient.keepAliveTick = 0;
          this->m_mqttClient.readTimeout = 0;
          this->m_mqttClient.sendTimeout = MQTT_SEND_TIMEOUT;
        }

        #ifdef EW_SERIAL_LOG
        Log(F("MQTT: recieved msg type :"));Logln(msg_type);
        Log(F("MQTT: recieved msg qos :"));Logln(msg_qos);
        #endif

        switch ( this->m_mqttClient.connState ) {

            case MQTT_CONNECT_SENT:

                if ( MQTT_MSG_TYPE_CONNACK == msg_type  ) {

                    if ( MQTT_MSG_TYPE_CONNECT != this->m_mqttClient.mqtt_state.pending_msg_type ) {

                        #ifdef EW_SERIAL_LOG
                        Logln(F("MQTT: Invalid packet recieved"));
                        #endif
                        this->m_mqttClient.connState = MQTT_HOST_RECONNECT_REQ;
                    } else {

                        #ifdef EW_SERIAL_LOG
                        Logln(F("MQTT: CONNACK recieved"));
                        #endif
                        this->m_mqttClient.connState = MQTT_DATA;
                        if ( nullptr != this->m_connectedCb ){
                          this->m_connectedCb( (uint32_t*)this );
                        }
                    }

                }
                break;
            case MQTT_DISCONNECT_SENT:
                //ignore
                if( nullptr != this->m_disconnectedCb ){
                  this->m_disconnectedCb( (uint32_t*)this );
                }
                this->m_mqttClient.connState = MQTT_DISCONNECTED;
                break;
            case MQTT_DATA_SENT:
            case MQTT_PING_SENT:
            case MQTT_DATA:
            case MQTT_KEEPALIVE_REQ:

                this->m_mqttClient.mqtt_state.message_length_read = len;
                this->m_mqttClient.mqtt_state.message_length = mqtt_get_total_length( this->m_mqttClient.mqtt_state.in_buffer, this->m_mqttClient.mqtt_state.message_length_read );
                this->m_mqttClient.connState = MQTT_DATA;

                switch (msg_type){

                    case MQTT_MSG_TYPE_SUBACK:

                        if ( MQTT_MSG_TYPE_SUBSCRIBE == this->m_mqttClient.mqtt_state.pending_msg_type && this->m_mqttClient.mqtt_state.pending_msg_id == msg_id ){

                          #ifdef EW_SERIAL_LOG
                          Logln(F("MQTT: Subscribe successful"));
                          #endif
                        }
                        if ( nullptr != this->m_subscribedCb ){
                          this->m_subscribedCb( (uint32_t*)this );
                        }
                        break;

                    case MQTT_MSG_TYPE_UNSUBACK:

                        if ( MQTT_MSG_TYPE_UNSUBSCRIBE == this->m_mqttClient.mqtt_state.pending_msg_type && this->m_mqttClient.mqtt_state.pending_msg_id == msg_id ){
                          #ifdef EW_SERIAL_LOG
                          Logln(F("MQTT: UnSubscribe successful"));
                          #endif
                        }
                        if ( nullptr != this->m_unsubscribedCb ){
                          this->m_unsubscribedCb( (uint32_t*)this );
                        }
                        break;

                    case MQTT_MSG_TYPE_PUBLISH:

                        if ( 1 == msg_qos ){

                          this->m_mqttClient.mqtt_state.outbound_message = mqtt_msg_puback( &this->m_mqttClient.mqtt_state.mqtt_connection, msg_id );
                        } else if ( 2 == msg_qos ){

                          this->m_mqttClient.mqtt_state.outbound_message = mqtt_msg_pubrec( &this->m_mqttClient.mqtt_state.mqtt_connection, msg_id );
                        }
                        if ( 1 == msg_qos || 2 == msg_qos ) {

                            #ifdef EW_SERIAL_LOG
                            Serial.printf("MQTT: Queue response QoS: %d\r\n", msg_qos);
                            #endif
                            if ( QUEUE_Puts(&this->m_mqttClient.msgQueue, this->m_mqttClient.mqtt_state.outbound_message->data, this->m_mqttClient.mqtt_state.outbound_message->length) == -1) {
                              #ifdef EW_SERIAL_LOG
                              Logln(F("MQTT: Queue full"));
                              #endif
                            }
                        }

                        this->deliver_publish( this->m_mqttClient.mqtt_state.in_buffer, this->m_mqttClient.mqtt_state.message_length_read );
                        break;

                    case MQTT_MSG_TYPE_PUBACK:

                        if ( MQTT_MSG_TYPE_PUBLISH == this->m_mqttClient.mqtt_state.pending_msg_type && this->m_mqttClient.mqtt_state.pending_msg_id == msg_id) {

                            #ifdef EW_SERIAL_LOG
                            Logln(F("MQTT: received MQTT_MSG_TYPE_PUBACK, finish QoS1 publish"));
                            #endif
                            if ( nullptr != this->m_publishedCb ){
                              this->m_publishedCb( (uint32_t*)this );
                            }
                        }

                        break;

                    case MQTT_MSG_TYPE_PUBREC:

                        this->m_mqttClient.mqtt_state.outbound_message = mqtt_msg_pubrel(&this->m_mqttClient.mqtt_state.mqtt_connection, msg_id);

                        if (QUEUE_Puts(&this->m_mqttClient.msgQueue, this->m_mqttClient.mqtt_state.outbound_message->data, this->m_mqttClient.mqtt_state.outbound_message->length) == -1) {
                            #ifdef EW_SERIAL_LOG
                            Logln(F("MQTT: Queue full"));
                            #endif
                        }
                        break;

                    case MQTT_MSG_TYPE_PUBREL:

                        this->m_mqttClient.mqtt_state.outbound_message = mqtt_msg_pubcomp(&this->m_mqttClient.mqtt_state.mqtt_connection, msg_id);

                        if (QUEUE_Puts(&this->m_mqttClient.msgQueue, this->m_mqttClient.mqtt_state.outbound_message->data, this->m_mqttClient.mqtt_state.outbound_message->length) == -1) {
                            #ifdef EW_SERIAL_LOG
                            Logln(F("MQTT: Queue full"));
                            #endif
                        }
                        break;

                    case MQTT_MSG_TYPE_PUBCOMP:

                        if ( MQTT_MSG_TYPE_PUBLISH == this->m_mqttClient.mqtt_state.pending_msg_type && this->m_mqttClient.mqtt_state.pending_msg_id == msg_id) {

                            #ifdef EW_SERIAL_LOG
                            Logln(F("MQTT: received MQTT_MSG_TYPE_PUBCOMP, finish QoS2 publish"));
                            #endif
                            if ( nullptr != this->m_publishedCb ){
                              this->m_publishedCb( (uint32_t*)this );
                            }
                        }
                        break;

                    case MQTT_MSG_TYPE_PINGREQ:

                        this->m_mqttClient.mqtt_state.outbound_message = mqtt_msg_pingresp(&this->m_mqttClient.mqtt_state.mqtt_connection);

                        if (QUEUE_Puts(&this->m_mqttClient.msgQueue, this->m_mqttClient.mqtt_state.outbound_message->data, this->m_mqttClient.mqtt_state.outbound_message->length) == -1) {

                            #ifdef EW_SERIAL_LOG
                            Logln(F("MQTT: Queue full"));
                            #endif
                        }
                        break;

                    case MQTT_MSG_TYPE_PINGRESP:
                        #ifdef EW_SERIAL_LOG
                        Logln(F("MQTT: MQTT_MSG_TYPE_PINGRESP recieved"));
                        #endif
                        break;
                    default :
                        #ifdef EW_SERIAL_LOG
                        Logln(F("MQTT: Uknown msg type received."));
                        #endif
                        break;
                }
                break;
            default :
                #ifdef EW_SERIAL_LOG
                Logln(F("MQTT: Uknown msg type received."));
                #endif
                break;
        }
    } else if( 0 == len ){
      #ifdef EW_SERIAL_LOG
      Logln(F("MQTT: No response received yet"));
      #endif
    }else {
      #ifdef EW_SERIAL_LOG
      Logln(F("MQTT: response too long"));
      #endif
    }
    // if( this->m_mqttClient.connState == MQTT_DATA ) this->MQTT_Task();
}

void MQTTClient::mqtt_client_connect(){

    mqtt_msg_init(&this->m_mqttClient.mqtt_state.mqtt_connection, this->m_mqttClient.mqtt_state.out_buffer, this->m_mqttClient.mqtt_state.out_buffer_length);
    this->m_mqttClient.mqtt_state.outbound_message = mqtt_msg_connect(&this->m_mqttClient.mqtt_state.mqtt_connection, this->m_mqttClient.mqtt_state.connect_info);
    this->m_mqttClient.mqtt_state.pending_msg_type = mqtt_get_type(this->m_mqttClient.mqtt_state.outbound_message->data);
    this->m_mqttClient.mqtt_state.pending_msg_id = mqtt_get_id(this->m_mqttClient.mqtt_state.outbound_message->data, this->m_mqttClient.mqtt_state.outbound_message->length);

    this->m_mqttClient.sendTimeout = MQTT_SEND_TIMEOUT;
    #ifdef EW_SERIAL_LOG
    Serial.printf("MQTT: Sending, type: %d, id: %04X\r\n", this->m_mqttClient.mqtt_state.pending_msg_type, this->m_mqttClient.mqtt_state.pending_msg_id);
    #endif
    bool result = sendPacket( this->m_mqttClient.mqtt_state.outbound_message->data, this->m_mqttClient.mqtt_state.outbound_message->length );

    if( result ){

      #ifdef EW_SERIAL_LOG
      Logln(F("MQTT: connect packet sent"));
      #endif
      this->m_mqttClient.connState = MQTT_CONNECT_SENT;
      this->m_mqttClient.readTimeout = 0;
    }else{

      #ifdef EW_SERIAL_LOG
      Logln(F("MQTT: connect packet send failed"));
      #endif
      this->m_mqttClient.connState = MQTT_CONNECT_FAILED;
      this->m_mqttClient.host_connect_tick = 0;
      this->disconnectServer();
    }
    this->m_mqttClient.mqtt_state.outbound_message = nullptr;
    // this->MQTT_Task();
}

void MQTTClient::mqtt_client_disconnect(){

    this->m_mqttClient.mqtt_state.outbound_message = mqtt_msg_disconnect(&this->m_mqttClient.mqtt_state.mqtt_connection);
    this->m_mqttClient.mqtt_state.pending_msg_type = mqtt_get_type(this->m_mqttClient.mqtt_state.outbound_message->data);
    this->m_mqttClient.mqtt_state.pending_msg_id = mqtt_get_id(this->m_mqttClient.mqtt_state.outbound_message->data, this->m_mqttClient.mqtt_state.outbound_message->length);

    this->m_mqttClient.sendTimeout = MQTT_SEND_TIMEOUT;
    #ifdef EW_SERIAL_LOG
    Serial.printf("MQTT: Sending, type: %d, id: %04X\r\n", this->m_mqttClient.mqtt_state.pending_msg_type, this->m_mqttClient.mqtt_state.pending_msg_id);
    #endif
    bool result = sendPacket( this->m_mqttClient.mqtt_state.outbound_message->data, this->m_mqttClient.mqtt_state.outbound_message->length );

    if( result ){

      #ifdef EW_SERIAL_LOG
      Logln(F("MQTT: disconnect packet sent"));
      #endif
      this->m_mqttClient.connState = MQTT_DISCONNECT_SENT;
      this->m_mqttClient.readTimeout = 0;
    }else{

      #ifdef EW_SERIAL_LOG
      Logln(F("MQTT: disconnect packet send failed"));
      #endif
      this->m_mqttClient.connState = MQTT_DISCONNECT_FAILED;
      this->m_mqttClient.host_connect_tick = 0;
    }
    this->disconnectServer();
    this->m_mqttClient.mqtt_state.outbound_message = nullptr;
}

void MQTTClient::mqtt_send_keepalive(){

    #ifdef EW_SERIAL_LOG
    Serial.printf("\r\nMQTT: Send keepalive packet to %s:%d!\r\n", this->m_host, this->m_port);
    #endif
    this->m_mqttClient.mqtt_state.outbound_message = mqtt_msg_pingreq(&this->m_mqttClient.mqtt_state.mqtt_connection);
    // this->m_mqttClient.mqtt_state.pending_msg_type = MQTT_MSG_TYPE_PINGREQ;
    this->m_mqttClient.mqtt_state.pending_msg_type = mqtt_get_type(this->m_mqttClient.mqtt_state.outbound_message->data);
    this->m_mqttClient.mqtt_state.pending_msg_id = mqtt_get_id(this->m_mqttClient.mqtt_state.outbound_message->data, this->m_mqttClient.mqtt_state.outbound_message->length);


    this->m_mqttClient.sendTimeout = MQTT_SEND_TIMEOUT;
    #ifdef EW_SERIAL_LOG
    Serial.printf("MQTT: Sending, type: %d, id: %04X\r\n", this->m_mqttClient.mqtt_state.pending_msg_type, this->m_mqttClient.mqtt_state.pending_msg_id);
    #endif
    bool result = sendPacket( this->m_mqttClient.mqtt_state.outbound_message->data, this->m_mqttClient.mqtt_state.outbound_message->length );

    if(result) {
        this->m_mqttClient.keepAliveTick = 0;
        this->m_mqttClient.readTimeout = 0;
        this->m_mqttClient.connState = MQTT_PING_SENT;
    }else {
        this->m_mqttClient.host_connect_tick = 0;
        this->m_mqttClient.connState = MQTT_PING_FAILED;
        this->disconnectServer();
    }
    // this->MQTT_Task();
    this->m_mqttClient.mqtt_state.outbound_message = nullptr;
}

void MQTTClient::MQTT_Task(){

    #ifdef EW_SERIAL_LOG
    Log(F("MQTT: Task ")); Logln(this->m_mqttClient.connState);
    #endif

    uint8_t dataBuffer[MQTT_BUF_SIZE];  uint16_t dataLen;
    memset( dataBuffer, 0, MQTT_BUF_SIZE );

    switch (this->m_mqttClient.connState) {

      default:
          break;
      case MQTT_HOST_RECONNECT_REQ:
      case MQTT_HOST_RECONNECT:
          this->disconnectServer();
          this->Connect();
          break;
      case MQTT_CONNECT_SENT:
      case MQTT_DISCONNECT_SENT:
      case MQTT_DATA_SENT:
      case MQTT_PING_SENT:
          this->mqtt_client_recv();
          break;
      case WIFI_CONNECTING_ERROR:
          if( nullptr != this->m_wifi && this->m_wifi->isConnected() ){
            this->m_mqttClient.connState = MQTT_HOST_RECONNECT_REQ;
          }
          break;
      case MQTT_HOST_CONNECTING:
          if( nullptr != this->m_wifi && !this->m_wifi->isConnected() ){
            #ifdef EW_SERIAL_LOG
            Logln( F("MQTT: WiFi not connected. waiting..") );
            #endif
            this->m_mqttClient.host_connect_tick = 0;
            this->m_mqttClient.connState = WIFI_CONNECTING_ERROR;
          }else{

            if( this->connected() ){
              #ifdef EW_SERIAL_LOG
              Serial.printf("MQTT: Connected to broker %s:%d\r\n", this->m_host, this->m_port);
              #endif
              this->mqtt_client_connect();
            }
          }
          break;
      case MQTT_DISCONNECT_REQ:
          this->mqtt_client_disconnect();
          break;
      case MQTT_KEEPALIVE_REQ:
          this->mqtt_send_keepalive();
          break;
      case MQTT_DATA:
          // if ( QUEUE_IsEmpty(&this->m_mqttClient.msgQueue) || this->m_mqttClient.sendTimeout != 0 ) {
          if ( QUEUE_IsEmpty(&this->m_mqttClient.msgQueue) ) {
              this->mqtt_client_recv();
              break;
          }
          if ( QUEUE_Gets(&this->m_mqttClient.msgQueue, dataBuffer, &dataLen, MQTT_BUF_SIZE) == 0 ) {

              #ifdef EW_SERIAL_LOG
              Log(F("MQTT: getting qued packets :"));
              for (int i = 0; i < dataLen; i++) {
                // Log_format( dataBuffer[i], HEX ); Log(" ");
                Log( (char)dataBuffer[i] ); Log(" ");
                __i_dvc_ctrl.wait(0);
              }
              Logln();
              #endif

              uint8_t msg_qos = mqtt_get_qos(dataBuffer);
              uint8_t msg_type = mqtt_get_type(dataBuffer);

              this->m_mqttClient.mqtt_state.pending_msg_type = msg_type;
              this->m_mqttClient.mqtt_state.pending_msg_id = mqtt_get_id(dataBuffer, dataLen);

              this->m_mqttClient.sendTimeout = MQTT_SEND_TIMEOUT;
              bool result = this->sendPacket( dataBuffer, dataLen );

              if( result ){

                #ifdef EW_SERIAL_LOG
                Log(F("MQTT: data packet sent of id: "));Logln(this->m_mqttClient.mqtt_state.pending_msg_id);
                #endif
                this->m_mqttClient.connState = (MQTT_MSG_TYPE_PUBLISH==msg_type&&0==msg_qos)?MQTT_DATA:MQTT_DATA_SENT;
                this->m_mqttClient.readTimeout = 0;
              }else{

                #ifdef EW_SERIAL_LOG
                Logln(F("MQTT: data packet send failed"));
                #endif
                this->m_mqttClient.connState = MQTT_DATA_FAILED;
                this->m_mqttClient.host_connect_tick = 0;
                this->disconnectServer();
              }

              this->m_mqttClient.mqtt_state.outbound_message = nullptr;
          }
          break;
    }
    __i_dvc_ctrl.wait(0); // yield purpose
}

void MQTTClient::mqtt_client_delete(){

    // mqtt_wificlient_delete(m_mqttClient);

    // if (this->m_mqttClient.user_data != nullptr) {
    //   delete this->m_mqttClient.user_data;
    //   this->m_mqttClient.user_data = nullptr;
    // }
    if( nullptr != this->m_host ) {
      delete[] this->m_host;
      this->m_host = nullptr;
    }

    if( nullptr != this->m_mqttClient.connect_info.client_id ) {
      delete[] this->m_mqttClient.connect_info.client_id;
      this->m_mqttClient.connect_info.client_id = nullptr;
    }

    if( nullptr != this->m_mqttClient.connect_info.username ) {
      delete[] this->m_mqttClient.connect_info.username;
      this->m_mqttClient.connect_info.username = nullptr;
    }

    if( nullptr != this->m_mqttClient.connect_info.password ) {
      delete[] this->m_mqttClient.connect_info.password;
      this->m_mqttClient.connect_info.password = nullptr;
    }

    if( nullptr != this->m_mqttClient.connect_info.will_topic ) {
      delete[] this->m_mqttClient.connect_info.will_topic;
      this->m_mqttClient.connect_info.will_topic = nullptr;
    }

    if( nullptr != this->m_mqttClient.connect_info.will_message ) {
      delete[] this->m_mqttClient.connect_info.will_message;
      this->m_mqttClient.connect_info.will_message = nullptr;
    }

    if( nullptr != this->m_mqttClient.mqtt_state.in_buffer ) {
      delete[] this->m_mqttClient.mqtt_state.in_buffer;
      this->m_mqttClient.mqtt_state.in_buffer = nullptr;
    }

    if( nullptr != this->m_mqttClient.mqtt_state.out_buffer ) {
      delete[] this->m_mqttClient.mqtt_state.out_buffer;
      this->m_mqttClient.mqtt_state.out_buffer = nullptr;
    }

    if( nullptr != this->m_mqttClient.msgQueue.buf ) {
      delete[] this->m_mqttClient.msgQueue.buf;
      this->m_mqttClient.msgQueue.buf = nullptr;
    }

    if( nullptr != this->m_wifi_client ) {
      delete this->m_wifi_client;
      this->m_wifi_client = nullptr;
    }

    this->clear_all_subscribed_topics();
}

bool MQTTClient::Subscribe( char *topic, uint8_t qos){

    uint8_t dataBuffer[MQTT_BUF_SIZE];  uint16_t dataLen;
    memset( dataBuffer, 0, MQTT_BUF_SIZE );

    if( !this->connected() ){
      return false;
    }

    this->m_mqttClient.mqtt_state.outbound_message = mqtt_msg_subscribe( &this->m_mqttClient.mqtt_state.mqtt_connection,
                                          topic, qos,
                                          &this->m_mqttClient.mqtt_state.pending_msg_id);

    #ifdef EW_SERIAL_LOG
    Serial.printf("MQTT: queue subscribe, topic \"%s\", id: %d\r\n", topic, this->m_mqttClient.mqtt_state.pending_msg_id );
    #endif
    // bool result = sendPacket( this->m_mqttClient.mqtt_state.outbound_message->data, this->m_mqttClient.mqtt_state.outbound_message->length );

    while (QUEUE_Puts(&this->m_mqttClient.msgQueue, this->m_mqttClient.mqtt_state.outbound_message->data, this->m_mqttClient.mqtt_state.outbound_message->length) == -1) {
      #ifdef EW_SERIAL_LOG
      Logln(F("MQTT: Queue full"));
      #endif
        if (QUEUE_Gets(&this->m_mqttClient.msgQueue, dataBuffer, &dataLen, MQTT_BUF_SIZE) == -1) {
          #ifdef EW_SERIAL_LOG
          Logln(F("MQTT: Serious buffer error"));
          #endif
          return false;
        }
    }

    if( !this->is_topic_subscribed( topic ) ){
      this->add_to_subscribed_topics( topic, qos );
    }
    this->m_mqttClient.sendTimeout = MQTT_SEND_TIMEOUT;
    return true;
}

bool MQTTClient::UnSubscribe( char *topic ) {

    uint8_t dataBuffer[MQTT_BUF_SIZE];  uint16_t dataLen;
    memset( dataBuffer, 0, MQTT_BUF_SIZE );

    this->m_mqttClient.mqtt_state.outbound_message = mqtt_msg_unsubscribe(&this->m_mqttClient.mqtt_state.mqtt_connection,
                                          topic,
                                          &this->m_mqttClient.mqtt_state.pending_msg_id);

    #ifdef EW_SERIAL_LOG
    Serial.printf("MQTT: queue un-subscribe, topic \"%s\", id: %d\r\n", topic, this->m_mqttClient.mqtt_state.pending_msg_id);
    #endif

    while (QUEUE_Puts(&this->m_mqttClient.msgQueue, this->m_mqttClient.mqtt_state.outbound_message->data, this->m_mqttClient.mqtt_state.outbound_message->length) == -1) {
        #ifdef EW_SERIAL_LOG
        Logln(F("MQTT: Queue full"));
        #endif
        if (QUEUE_Gets(&this->m_mqttClient.msgQueue, dataBuffer, &dataLen, MQTT_BUF_SIZE) == -1) {
            #ifdef EW_SERIAL_LOG
            Logln(F("MQTT: Serious buffer error"));
            #endif
            return false;
        }
    }

    this->remove_from_subscribed_topics( topic );
    this->m_mqttClient.sendTimeout = MQTT_SEND_TIMEOUT;
    return true;
}

bool MQTTClient::Publish( const char *topic, const char *data, int data_length, int qos, int retain ){

    uint8_t dataBuffer[MQTT_BUF_SIZE];  uint16_t dataLen;
    memset( dataBuffer, 0, MQTT_BUF_SIZE );

    if( !this->connected() ){
      return false;
    }

    this->m_mqttClient.mqtt_state.outbound_message = mqtt_msg_publish(&this->m_mqttClient.mqtt_state.mqtt_connection,
                                          topic, data, data_length,
                                          qos, retain,
                                          &this->m_mqttClient.mqtt_state.pending_msg_id);
    if ( 0 == this->m_mqttClient.mqtt_state.outbound_message->length ) {
        #ifdef EW_SERIAL_LOG
        Logln(F("MQTT: Queuing publish failed"));
        #endif
        return false;
    }
    #ifdef EW_SERIAL_LOG
    Serial.printf("MQTT: queuing publish, length: %d, queue size(%d/%d)\r\n", this->m_mqttClient.mqtt_state.outbound_message->length, this->m_mqttClient.msgQueue.rb.fill_cnt, this->m_mqttClient.msgQueue.rb.size);
    #endif
    while (QUEUE_Puts(&this->m_mqttClient.msgQueue, this->m_mqttClient.mqtt_state.outbound_message->data, this->m_mqttClient.mqtt_state.outbound_message->length) == -1) {
        #ifdef EW_SERIAL_LOG
        Logln(F("MQTT: Queue full"));
        #endif
        if (QUEUE_Gets(&this->m_mqttClient.msgQueue, dataBuffer, &dataLen, MQTT_BUF_SIZE) == -1) {
            #ifdef EW_SERIAL_LOG
            Logln(F("MQTT: Serious buffer error"));
            #endif
            return false;
        }
    }
    this->m_mqttClient.sendTimeout = MQTT_SEND_TIMEOUT;
    return true;
}

MQTTClient::MQTTClient():
  m_mqttDataCallbackArgs(reinterpret_cast<uint32_t*>(this)),
  m_host(nullptr),
  m_port(0),
  m_security(0),
  m_wifi_client(nullptr),
  m_wifi(nullptr),
  m_connectedCb(nullptr),
  m_disconnectedCb(nullptr),
  m_publishedCb(nullptr),
  m_subscribedCb(nullptr),
  m_unsubscribedCb(nullptr),
  m_timeoutCb(nullptr),
  m_dataCb(nullptr)
{

}

MQTTClient::~MQTTClient(){

  this->mqtt_client_delete();
}

bool MQTTClient::begin( iWiFiInterface *_wifi, mqtt_general_config_table *_mqtt_general_configs, mqtt_lwt_config_table *_mqtt_lwt_configs ){

  if( nullptr == _wifi || nullptr == _mqtt_general_configs || nullptr == _mqtt_lwt_configs ){
    return false;
  }

  this->m_wifi = _wifi;

  if( strlen(_mqtt_general_configs->host) > 5 && _mqtt_general_configs->port > 0 ){

    this->InitConnection( _mqtt_general_configs->host, _mqtt_general_configs->port, _mqtt_general_configs->security );
  }else{
    return false;
  }

  if( strlen(_mqtt_general_configs->client_id) > 0 && _mqtt_general_configs->keepalive > 5 ){

    this->InitClient(
      _mqtt_general_configs->client_id,
      _mqtt_general_configs->username,
      _mqtt_general_configs->password,
      _mqtt_general_configs->keepalive,
      _mqtt_general_configs->clean_session
    );
  }else{
    this->mqtt_client_delete();
    return false;
  }

  if( strlen(_mqtt_lwt_configs->will_topic) > 0 ){

    this->InitLWT(
      _mqtt_lwt_configs->will_topic,
      _mqtt_lwt_configs->will_message,
      _mqtt_lwt_configs->will_qos < MQTT_MAX_QOS_LEVEL ?_mqtt_lwt_configs->will_qos:MQTT_MAX_QOS_LEVEL,
      _mqtt_lwt_configs->will_retain
    );
  }
  #ifdef EW_SERIAL_LOG
  Logln( F("MQTT: Connecting") );
  // Log( F("client_id: ") );	Logln( _mqtt_general_configs->client_id );
  // Log( F("password: ") );	Logln( _mqtt_general_configs->password );
  // Log( F("username: ") );	Logln( _mqtt_general_configs->username );
  #endif
  this->OnConnected( mqttConnectedCb );
  this->OnDisconnected( mqttDisconnectedCb );
  this->OnPublished( mqttPublishedCb );
  this->OnSubscribed( mqttSubscribedCb );
  this->OnUnsubscribed( mqttUnsubscribedCb );
  // this->OnData( mqttDataCb );

  this->Connect();
  return true;
}

void MQTTClient::InitConnection( char *host, int port, uint8_t security ){

    #ifdef EW_SERIAL_LOG
    Logln(F("MQTT: InitConnection"));
    #endif

    int _host_len = strlen(host);
    memset( &this->m_mqttClient, 0, sizeof(MQTT_Client) );
    this->m_host = new char[_host_len + 1];

    if( nullptr != this->m_host ){

      memset( this->m_host, 0, _host_len + 1 );
      strcpy( this->m_host, host );
      this->m_host[_host_len] = 0;
    }

    this->m_port = port;
    this->m_security = security;
}

void MQTTClient::InitClient( char *client_id, char *client_user, char *client_pass, uint32_t keepAliveTime, uint8_t cleanSession ){

    uint32_t temp;
    int _len = 0;

    #ifdef EW_SERIAL_LOG
    Logln(F("MQTT: InitClient"));
    #endif

    memset(&this->m_mqttClient.connect_info, 0, sizeof(mqtt_connect_info_t));

    if ( nullptr != client_id ){

        _len = strlen(client_id);
        this->m_mqttClient.connect_info.client_id = new char[_len + 1];
        if ( nullptr != this->m_mqttClient.connect_info.client_id ){

            memset( this->m_mqttClient.connect_info.client_id, 0, _len + 1 );
            strcpy( this->m_mqttClient.connect_info.client_id, client_id);
            this->m_mqttClient.connect_info.client_id[_len] = 0;
        }
    }

    if ( nullptr != client_user ){

        _len = strlen(client_user);
        this->m_mqttClient.connect_info.username = new char[_len + 1];
        if ( nullptr != this->m_mqttClient.connect_info.username ){

            memset( this->m_mqttClient.connect_info.username, 0, _len + 1 );
            strcpy( this->m_mqttClient.connect_info.username, client_user);
            this->m_mqttClient.connect_info.username[_len] = 0;
        }
    }

    if ( nullptr != client_pass ){

        _len = strlen(client_pass);
        this->m_mqttClient.connect_info.password = new char[_len + 1];
        if ( nullptr != this->m_mqttClient.connect_info.password ){

            memset( this->m_mqttClient.connect_info.password, 0, _len + 1 );
            strcpy( this->m_mqttClient.connect_info.password, client_pass);
            this->m_mqttClient.connect_info.password[_len] = 0;
        }
    }


    this->m_mqttClient.connect_info.keepalive = keepAliveTime > 5 ? keepAliveTime : MQTT_DEFAULT_KEEPALIVE;
    this->m_mqttClient.connect_info.clean_session = cleanSession;

    this->m_mqttClient.mqtt_state.in_buffer = new uint8_t[MQTT_BUF_SIZE];
    if ( nullptr != this->m_mqttClient.mqtt_state.in_buffer ){

        memset( this->m_mqttClient.mqtt_state.in_buffer, 0, MQTT_BUF_SIZE );
        this->m_mqttClient.mqtt_state.in_buffer_length = MQTT_BUF_SIZE;
    }
    this->m_mqttClient.mqtt_state.out_buffer =  new uint8_t[MQTT_BUF_SIZE];
    if ( nullptr != this->m_mqttClient.mqtt_state.out_buffer ){

        memset( this->m_mqttClient.mqtt_state.out_buffer, 0, MQTT_BUF_SIZE );
        this->m_mqttClient.mqtt_state.out_buffer_length = MQTT_BUF_SIZE;
    }
    this->m_mqttClient.mqtt_state.connect_info = &this->m_mqttClient.connect_info;

    mqtt_msg_init( &this->m_mqttClient.mqtt_state.mqtt_connection, this->m_mqttClient.mqtt_state.out_buffer, this->m_mqttClient.mqtt_state.out_buffer_length );

    QUEUE_Init(&this->m_mqttClient.msgQueue, QUEUE_BUFFER_SIZE);

    // this->MQTT_Task();
}

void MQTTClient::InitLWT( char *will_topic, char *will_msg, uint8_t will_qos, uint8_t will_retain ){

    int _len = 0;
    #ifdef EW_SERIAL_LOG
    Logln(F("MQTT: InitLWT"));
    #endif

    if ( nullptr != will_topic ){

      _len = strlen(will_topic);
      this->m_mqttClient.connect_info.will_topic = new char[_len + 1];
      if ( nullptr != this->m_mqttClient.connect_info.will_topic ){

          memset( this->m_mqttClient.connect_info.will_topic, 0, _len + 1 );
          strcpy(this->m_mqttClient.connect_info.will_topic, will_topic);
          this->m_mqttClient.connect_info.will_topic[_len] = 0;
      }
    }

    if ( nullptr != will_topic ){

      _len = strlen(will_msg);
      this->m_mqttClient.connect_info.will_message = new char[_len + 1];
      if ( nullptr != this->m_mqttClient.connect_info.will_message ){

          memset( this->m_mqttClient.connect_info.will_message, 0, _len + 1 );
          strcpy(this->m_mqttClient.connect_info.will_message, will_msg);
          this->m_mqttClient.connect_info.will_message[_len] = 0;
      }
    }

    this->m_mqttClient.connect_info.will_qos = will_qos;
    this->m_mqttClient.connect_info.will_retain = will_retain;
}


void MQTTClient::mqtt_timer(){

    #ifdef EW_SERIAL_LOG
    Log( F("MQTT: mqtt timer : ") ); Log( this->connected() );
    Log( F("\t mqtt state : ") ); Logln( this->m_mqttClient.connState );

    Log( F("MQTT: subscribed topics(QoS) : ") );
    for ( uint16_t i = 0; i < this->m_mqttClient.subscribed_topics.size(); i++) {

      Log( this->m_mqttClient.subscribed_topics[i].topic );
      Log( F("(") );
      Log( this->m_mqttClient.subscribed_topics[i].qos );
      Log( F("), ") );
    }
    Logln();
    #endif
    // if( this->m_mqttClient.connState == MQTT_DELETING ) return;

    if ( MQTT_DATA == this->m_mqttClient.connState ) {

        this->m_mqttClient.keepAliveTick ++;
        int _keep_alive = this->m_mqttClient.mqtt_state.connect_info->keepalive * 0.85;
        if ( this->m_mqttClient.keepAliveTick > _keep_alive ) {
            this->m_mqttClient.connState = MQTT_KEEPALIVE_REQ;
        }
        this->MQTT_Task();
    } else if (
      MQTT_CONNECT_SENT == this->m_mqttClient.connState ||
      MQTT_DISCONNECT_SENT == this->m_mqttClient.connState ||
      MQTT_PING_SENT == this->m_mqttClient.connState ||
      MQTT_DATA_SENT == this->m_mqttClient.connState
    ) {

        this->m_mqttClient.readTimeout ++;
        if (this->m_mqttClient.readTimeout > MQTT_READ_TIMEOUT) {
            this->m_mqttClient.readTimeout = 0;
            this->m_mqttClient.connState = MQTT_HOST_RECONNECT_REQ;
            if ( nullptr != this->m_timeoutCb ){
              this->m_timeoutCb( (uint32_t*)this );
            }
        }
        this->MQTT_Task();
    } else if (
      MQTT_HOST_RECONNECT_REQ == this->m_mqttClient.connState ||
      WIFI_CONNECTING_ERROR == this->m_mqttClient.connState
    ) {

        this->MQTT_Task();
    } else if (
      MQTT_HOST_CONNECTING == this->m_mqttClient.connState ||
      MQTT_CONNECT_FAILED == this->m_mqttClient.connState ||
      MQTT_DISCONNECT_FAILED == this->m_mqttClient.connState ||
      MQTT_PING_FAILED == this->m_mqttClient.connState ||
      MQTT_DATA_FAILED == this->m_mqttClient.connState
    ) {

      this->m_mqttClient.host_connect_tick ++;
      if (this->m_mqttClient.host_connect_tick > MQTT_HOST_CONNECT_TIMEOUT) {
          this->m_mqttClient.host_connect_tick = 0;
          #ifdef EW_SERIAL_LOG
          Logln( F("MQTT: host connect error. trying reconnect") );
          #endif
          this->m_mqttClient.connState = MQTT_HOST_RECONNECT;
          this->MQTT_Task();
      }
    }else{

      //probably this->m_mqttClient.connState == MQTT_DELETING
    }

    if ( this->m_mqttClient.sendTimeout > 0 ){
      this->m_mqttClient.sendTimeout --;
    }
    __i_dvc_ctrl.wait(0); // yield purpose
}

void MQTTClient::Connect(){

    if ( nullptr == this->m_wifi_client ) {

      this->m_wifi_client = new WiFiClientInterface;
    }
    this->m_mqttClient.keepAliveTick = 0;
    this->m_mqttClient.host_connect_tick = 0;

    this->m_mqttClient.connState = MQTT_HOST_CONNECTING;

    this->connectServer();
    this->MQTT_Task();
}

void MQTTClient::Disconnect(){

    this->m_mqttClient.connState = MQTT_DISCONNECT_REQ;
    this->MQTT_Task();
}

void MQTTClient::DeleteClient(){

    this->m_mqttClient.connState = MQTT_DELETING;
    this->disconnectServer();
    this->mqtt_client_delete();
    // this->MQTT_Task();
}

void MQTTClient::OnConnected( MqttCallback connectedCb ){

    this->m_connectedCb = connectedCb;
}

void MQTTClient::OnDisconnected( MqttCallback disconnectedCb ){

    this->m_disconnectedCb = disconnectedCb;
}

void MQTTClient::OnData( MqttDataCallback dataCb ){

    this->m_dataCb = dataCb;
}

void MQTTClient::OnPublished( MqttCallback publishedCb ){

    this->m_publishedCb = publishedCb;
}

void MQTTClient::OnSubscribed( MqttCallback subscribedCb ){

    this->m_subscribedCb = subscribedCb;
}

void MQTTClient::OnUnsubscribed( MqttCallback unsubscribedCb ){

    this->m_unsubscribedCb = unsubscribedCb;
}

void MQTTClient::OnTimeout( MqttCallback timeoutCb ){

    this->m_timeoutCb = timeoutCb;
}



/*   WiFi client support functions */


bool MQTTClient::connectServer( ) {

  if( nullptr == this->m_wifi_client ){
    return false;
  }
  #ifdef EW_SERIAL_LOG
  Serial.printf("MQTT: Connecting to %s:%d\r\n", this->m_host, this->m_port);
  #endif
  this->m_wifi_client->setTimeout(3000);
  int result = this->m_wifi_client->connect(this->m_host, this->m_port);
  #ifdef EW_SERIAL_LOG
  Log(F("MQTT: Connect result -: ")); Logln(result);
  #endif
  return ( 0 != result );
}

bool MQTTClient::disconnectServer() {

  if( this->m_disconnectedCb ) {
    this->m_disconnectedCb( (uint32_t*)this );
  }
  return this->connected() ? this->m_wifi_client->stop(500) : true;
}

bool MQTTClient::connected() {

  // bool _is_live = this->m_wifi_client->connected();
  // if ( !_is_live ) {
  //   this->m_wifi_client->stop();
  // }
  // return _is_live;
  return ( nullptr != this->m_wifi_client ) ? this->m_wifi_client->connected() : false;
}

uint16_t MQTTClient::readFullPacket( uint8_t *buffer, uint16_t maxsize, uint16_t timeout ) {

  uint8_t *pbuff = buffer;
  uint16_t rlen;

  memset( buffer, 0, maxsize );
  // read the packet type:
  rlen = this->readPacket( pbuff, 1, timeout );
  if (rlen != 1) return 0;

  // #ifdef EW_SERIAL_LOG
  // Log(F("MQTT: Packet Type:\t")); Logln((char*)pbuff);
  // #endif

  pbuff++;

  uint32_t value = 0;
  uint32_t multiplier = 1;
  uint8_t encodedByte;

  do {
    rlen = this->readPacket( pbuff, 1, timeout );
    if (rlen != 1) return 0;
    encodedByte = pbuff[0]; // save the last read val
    pbuff++; // get ready for reading the next byte
    uint32_t intermediate = encodedByte & 0x7F;
    intermediate *= multiplier;
    value += intermediate;
    multiplier *= 128;
    if (multiplier > (128UL*128UL*128UL)) {
      #ifdef EW_SERIAL_LOG
      Log(F("Malformed packet len"));
      #endif
      return 0;
    }
  } while (encodedByte & 0x80);

  // #ifdef EW_SERIAL_LOG
  // Log(F("MQTT: Packet Length:\t"));Logln(value);
  // #endif

  if (value > (maxsize - (pbuff-buffer) - 1)) {
    #ifdef EW_SERIAL_LOG
    Logln(F("MQTT: Packet too big for buffer"));
    #endif
    rlen = this->readPacket( pbuff, (maxsize - (pbuff-buffer) - 1), timeout );
  } else {
    rlen = this->readPacket( pbuff, value, timeout );
  }
  return ((pbuff - buffer)+rlen);
}

uint16_t MQTTClient::readPacket( uint8_t *buffer, uint16_t maxlen, int16_t timeout ) {

  uint16_t len = 0;
  int16_t t = timeout;

  while ( this->connected() && ( timeout >= 0 ) ) {

    while ( this->m_wifi_client->available() ) {

      char c = this->m_wifi_client->read();
      timeout = t;  // reset the timeout
      buffer[len] = c;
      len++;

      if( maxlen == 0 ){
        return 0;
      }

      if( len == maxlen ){
        // #ifdef EW_SERIAL_LOG
        // Log(F("MQTT: Read Data ")); Logln((char*)buffer);
        // #endif
        return len;
      }
    }
    timeout -= MQTT_CLIENT_READINTERVAL_MS;
    __i_dvc_ctrl.wait(MQTT_CLIENT_READINTERVAL_MS);
  }
  return len;
}

bool MQTTClient::sendPacket( uint8_t *buffer, uint16_t len ) {
  uint16_t ret = 0;

  while (len > 0) {
    if ( this->connected() ) {
      // send 250 bytes at most at a time, can adjust this later based on Client

      // uint16_t sendlen = len > 250 ? 250 : len;
      uint16_t sendlen = len;
      //Serial.print("Sending: "); Serial.println(sendlen);
      ret = this->m_wifi_client->write( buffer, sendlen);
      #ifdef EW_SERIAL_LOG
      // Log(F("MQTT: sending packets : "));
      // for (int i = 0; i < len; i++) {
      //   Log((char)buffer[i]);
      //   Log(F(" "));
      // }
      // Logln();
      Log(F("MQTT: send packet return ")); Logln(ret);
      #endif
      len -= ret;

      if (ret != sendlen) {
        #ifdef EW_SERIAL_LOG
        Logln(F("MQTT: send packet - failed to send"));
        #endif
	      return false;
      }
    } else {
      #ifdef EW_SERIAL_LOG
      Logln(F("MQTT: send packet - connection failed"));
      #endif
      return false;
    }
  }
  return true;
}

#endif
