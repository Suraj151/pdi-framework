/****************************** Mqtt service **********************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st June 2019
******************************************************************************/

#include <config/Config.h>

#if defined(ENABLE_MQTT_SERVICE)

#include "MqttServiceProvider.h"

/**
 * MqttServiceProvider constructor.
 */
MqttServiceProvider::MqttServiceProvider():
  m_mqtt_timer_cb_id(0),
  m_mqtt_publish_cb_id(0),
  m_mqtt_subscribe_cb_id(0),
  m_mqtt_payload(nullptr),
  m_mqtt_publish_data_cb(nullptr),
  m_mqtt_subscribe_data_cb(nullptr),
  m_client(nullptr),
  ServiceProvider(SERVICE_MQTT)
{
}

/**
 * MqttServiceProvider destructor
 */
MqttServiceProvider::~MqttServiceProvider(){
  this->stop();
  if( nullptr != this->m_mqtt_payload ){
    delete[] this->m_mqtt_payload;
    this->m_mqtt_payload = nullptr;
  }
  this->m_mqtt_publish_data_cb = nullptr;
  this->m_mqtt_subscribe_data_cb = nullptr;
}

/**
 * start mqtt service. initialize it with mqtt configs at database
 */
void MqttServiceProvider::begin( iClientInterface* _client ){

  m_client = _client;
  this->m_mqtt_payload = new char[ MQTT_PAYLOAD_BUF_SIZE ];
  if( nullptr != this->m_mqtt_payload ){
    memset( this->m_mqtt_payload, 0, MQTT_PAYLOAD_BUF_SIZE );
  }
  this->m_mqtt_client.m_mqttDataCallbackArgs = reinterpret_cast<uint32_t*>(this);
  this->m_mqtt_client.OnData( MqttServiceProvider::handleMqttDataCb );

  this->handleMqttConfigChange();
}

/**
 * check and do mqtt publish on given configs.
 */
void MqttServiceProvider::handleMqttPublish(){

  LogI("MQTT: handling mqtt publish interval\n");
  if( !this->m_mqtt_client.is_mqtt_connected() ) return;

  mqtt_pubsub_config_table _mqtt_pubsub_configs;
  __database_service.get_mqtt_pubsub_config_table(&_mqtt_pubsub_configs);

  for (uint8_t i = 0; i < MQTT_MAX_PUBLISH_TOPIC; i++) {

    __find_and_replace( _mqtt_pubsub_configs.publish_topics[i].topic, "[mac]", __i_dvc_ctrl.getDeviceMac().c_str(), 2 );
    LogFmtI("MQTT: publishing on topic : %s\n", _mqtt_pubsub_configs.publish_topics[i].topic);
    if( nullptr != this->m_mqtt_payload && strlen(_mqtt_pubsub_configs.publish_topics[i].topic) > 0 ){

      #ifdef ENABLE_MQTT_DEFAULT_PAYLOAD

        pdiutil::string *_payload = new pdiutil::string();

        if( nullptr != _payload ){

          #ifdef ENABLE_GPIO_SERVICE

            __gpio_service.appendGpioJsonPayload( *_payload );
          #else

            *_payload += "Hello from PDI Client : ";
            *_payload += __i_dvc_ctrl.getDeviceId();
          #endif

          memset( this->m_mqtt_payload, 0, MQTT_PAYLOAD_BUF_SIZE );
          // _payload->toCharArray( this->m_mqtt_payload, MQTT_PAYLOAD_BUF_SIZE );
          strncpy(this->m_mqtt_payload, _payload->c_str(), _payload->size());

          delete _payload;
        }

      #else

      #endif

      if( nullptr != this->m_mqtt_publish_data_cb ){
        this->m_mqtt_publish_data_cb( this->m_mqtt_payload, MQTT_PAYLOAD_BUF_SIZE );
      }
      __find_and_replace( this->m_mqtt_payload, "[mac]", __i_dvc_ctrl.getDeviceMac().c_str(), 2 );

      this->m_mqtt_client.Publish(
        _mqtt_pubsub_configs.publish_topics[i].topic,
        this->m_mqtt_payload,
        strlen( this->m_mqtt_payload ),
        _mqtt_pubsub_configs.publish_topics[i].qos < MQTT_MAX_QOS_LEVEL ?
        _mqtt_pubsub_configs.publish_topics[i].qos : MQTT_MAX_QOS_LEVEL,
        _mqtt_pubsub_configs.publish_topics[i].retain
      );
    }
  }
}

/**
 * handle mqtt subscribe cycle. suscribe to topics in pubsub configs
 */
void MqttServiceProvider::handleMqttSubScribe(){

  LogI("MQTT: handling mqtt subscribe interval\n");
  if( !this->m_mqtt_client.is_mqtt_connected() ) return;

  mqtt_pubsub_config_table _mqtt_pubsub_configs;
  __database_service.get_mqtt_pubsub_config_table(&_mqtt_pubsub_configs);

  for (uint8_t i = 0; i < MQTT_MAX_SUBSCRIBE_TOPIC; i++) {

    __find_and_replace( _mqtt_pubsub_configs.subscribe_topics[i].topic, "[mac]", __i_dvc_ctrl.getDeviceMac().c_str(), 2 );

    if( strlen(_mqtt_pubsub_configs.subscribe_topics[i].topic) > 0 && !this->m_mqtt_client.is_topic_subscribed(_mqtt_pubsub_configs.subscribe_topics[i].topic) ){

      this->m_mqtt_client.Subscribe(

        _mqtt_pubsub_configs.subscribe_topics[i].topic,
        _mqtt_pubsub_configs.subscribe_topics[i].qos < MQTT_MAX_QOS_LEVEL ?
        _mqtt_pubsub_configs.subscribe_topics[i].qos : MQTT_MAX_QOS_LEVEL
      );
    }
  }
}

/**
 * stop mqtt client
 *
 */
void MqttServiceProvider::stop(){

  this->m_mqtt_client.DeleteClient();
  __task_scheduler.clearInterval( this->m_mqtt_timer_cb_id );
  this->m_mqtt_timer_cb_id = 0;
}

/**
 * handle restart of mqtt services on config change from autherised client
 *
 * @param   int _mqtt_config_type
 */
void MqttServiceProvider::handleMqttConfigChange( int _mqtt_config_type ){


  mqtt_general_config_table _mqtt_general_configs;
  mqtt_pubsub_config_table _mqtt_pubsub_configs;
  __database_service.get_mqtt_general_config_table(&_mqtt_general_configs);
  __database_service.get_mqtt_pubsub_config_table(&_mqtt_pubsub_configs);

  if( MQTT_GENERAL_CONFIG == _mqtt_config_type || MQTT_LWT_CONFIG == _mqtt_config_type ){

    this->m_mqtt_client.DeleteClient();
    int _stat = __task_scheduler.setTimeout( [&]() {

      mqtt_general_config_table _mqtt_general_configs;
      mqtt_lwt_config_table _mqtt_lwt_configs;
      __database_service.get_mqtt_general_config_table(&_mqtt_general_configs);
      __database_service.get_mqtt_lwt_config_table(&_mqtt_lwt_configs);

      __find_and_replace( _mqtt_general_configs.username, "[mac]", __i_dvc_ctrl.getDeviceMac().c_str(), 2 );
      __find_and_replace( _mqtt_general_configs.client_id, "[mac]", __i_dvc_ctrl.getDeviceMac().c_str(), 2 );
      __find_and_replace( _mqtt_lwt_configs.will_message, "[mac]", __i_dvc_ctrl.getDeviceMac().c_str(), 2 );

      if( this->m_mqtt_client.begin( m_client, &_mqtt_general_configs, &_mqtt_lwt_configs ) ){
        this->m_mqtt_timer_cb_id = __task_scheduler.updateInterval(
          this->m_mqtt_timer_cb_id,
          [&]() { this->m_mqtt_client.mqtt_timer(); },
          MILLISECOND_DURATION_1000
        );
      }else{
        __task_scheduler.clearInterval( this->m_mqtt_timer_cb_id );
        this->m_mqtt_timer_cb_id = 0;
      }
    }, MQTT_INITIALIZE_DURATION, __i_dvc_ctrl.millis_now() );
  }else if( MQTT_PUBSUB_CONFIG == _mqtt_config_type ){

    for ( uint16_t i = 0; i < this->m_mqtt_client.m_mqttClient.subscribed_topics.size(); i++) {

      bool _found = false;
      for (uint8_t j = 0; j < MQTT_MAX_SUBSCRIBE_TOPIC; j++) {

        if( __are_str_equals(
          _mqtt_pubsub_configs.subscribe_topics[j].topic,
          this->m_mqtt_client.m_mqttClient.subscribed_topics[i].topic,
          strlen( _mqtt_pubsub_configs.subscribe_topics[j].topic )
        ) ) _found = true;

      }
      if( !_found )
        this->m_mqtt_client.UnSubscribe( this->m_mqtt_client.m_mqttClient.subscribed_topics[i].topic );
    }

  }else{

  }

  if( _mqtt_pubsub_configs.publish_frequency > 0 ){
    this->m_mqtt_publish_cb_id = __task_scheduler.updateInterval(
      this->m_mqtt_publish_cb_id,
      [&]() { this->handleMqttPublish(); },
      _mqtt_pubsub_configs.publish_frequency*MILLISECOND_DURATION_1000
    );
  }else{
    __task_scheduler.clearInterval( this->m_mqtt_publish_cb_id );
    this->m_mqtt_publish_cb_id = 0;
  }

  if( _mqtt_general_configs.keepalive > 0 ){
    this->m_mqtt_subscribe_cb_id = __task_scheduler.updateInterval(
      this->m_mqtt_subscribe_cb_id,
      [&]() { this->handleMqttSubScribe(); },
      (_mqtt_general_configs.keepalive/2)*MILLISECOND_DURATION_1000,
      DEFAULT_TASK_PRIORITY, ( __i_dvc_ctrl.millis_now() + MQTT_INITIALIZE_DURATION )
    );
  }else{
    __task_scheduler.clearInterval( this->m_mqtt_subscribe_cb_id );
    this->m_mqtt_subscribe_cb_id = 0;
  }

}

/**
 * set publish data callback. used to set data before send
 *
 * @param   MqttPublishDataCallback _cb
 */
void MqttServiceProvider::setMqttPublishDataCallback( MqttPublishDataCallback _cb ){

  this->m_mqtt_publish_data_cb = _cb;
}

/**
 * set subscribe data callback. used to get data before received
 *
 * @param   MqttSubscribeDataCallback _cb
 */
void MqttServiceProvider::setMqttSubscribeDataCallback( MqttSubscribeDataCallback _cb ){

  this->m_mqtt_subscribe_data_cb = _cb;
}

/**
 * handle subscribed topics data
 *
 */
void MqttServiceProvider::handleMqttDataCb( uint32_t *args, const char* topic, uint32_t topic_len, const char *data, uint32_t data_len ){

    if( nullptr != __mqtt_service.m_mqtt_subscribe_data_cb ) __mqtt_service.m_mqtt_subscribe_data_cb(
      args, topic, topic_len, data, data_len
    );

    char *topicBuf = new char[topic_len+1], *dataBuf = new char[data_len+1];

    if( nullptr != topicBuf ){
      memcpy(topicBuf, topic, topic_len);
      topicBuf[topic_len] = 0;
      delete[] topicBuf;
    }

    if( nullptr != dataBuf ){
      memcpy(dataBuf, data, data_len);
      dataBuf[data_len] = 0;

      #if defined( ENABLE_MQTT_DEFAULT_PAYLOAD ) && defined( ENABLE_GPIO_SERVICE )
      __gpio_service.applyGpioJsonPayload( dataBuf, data_len );
      #endif

      delete[] dataBuf;
    }
}

/**
 * print Mqtt configs to terminal
 */
void MqttServiceProvider::printConfigToTerminal(iTerminalInterface *terminal)
{
  if( nullptr != terminal ){

    mqtt_general_config_table _mqtt_general_configs;
    mqtt_lwt_config_table _mqtt_lwt_configs;
    mqtt_pubsub_config_table _mqtt_pubsub_configs;
    __database_service.get_mqtt_general_config_table(&_mqtt_general_configs);
    __database_service.get_mqtt_lwt_config_table(&_mqtt_lwt_configs);
    __database_service.get_mqtt_pubsub_config_table(&_mqtt_pubsub_configs);

    terminal->write_ro(RODT_ATTR("\nMqtt General Configs :\n"));
    terminal->write(_mqtt_general_configs.host); terminal->write(RODT_ATTR("\t"));
    terminal->write(_mqtt_general_configs.port); terminal->write(RODT_ATTR("\t"));
    terminal->write(_mqtt_general_configs.security); terminal->write(RODT_ATTR("\t"));
    terminal->write(_mqtt_general_configs.client_id); terminal->write(RODT_ATTR("\t"));
    terminal->write(_mqtt_general_configs.username); terminal->write(RODT_ATTR("\t"));
    terminal->write(_mqtt_general_configs.password); terminal->write(RODT_ATTR("\t"));
    terminal->write(_mqtt_general_configs.keepalive); terminal->write(RODT_ATTR("\t"));
    terminal->write(_mqtt_general_configs.clean_session); terminal->write(RODT_ATTR("\n"));


    terminal->write_ro(RODT_ATTR("\nMqtt Lwt Configs :\n"));
    terminal->write(_mqtt_lwt_configs.will_topic); terminal->write(RODT_ATTR("\t"));
    terminal->write(_mqtt_lwt_configs.will_message); terminal->write(RODT_ATTR("\t"));
    terminal->write(_mqtt_lwt_configs.will_qos); terminal->write(RODT_ATTR("\t"));
    terminal->write(_mqtt_lwt_configs.will_retain); terminal->write(RODT_ATTR("\n"));

    terminal->write_ro(RODT_ATTR("\nMqtt Pub Configs :\n"));
    for (uint8_t i = 0; i < MQTT_MAX_PUBLISH_TOPIC; i++) {

      if( strlen(_mqtt_pubsub_configs.publish_topics[i].topic) > 0 ){

        terminal->write(_mqtt_pubsub_configs.publish_topics[i].topic); terminal->write(RODT_ATTR("\t"));
        terminal->write((int32_t)_mqtt_pubsub_configs.publish_topics[i].qos); terminal->write(RODT_ATTR("\t"));
        terminal->write((int32_t)_mqtt_pubsub_configs.publish_topics[i].retain); terminal->write(RODT_ATTR("\n"));
      }
    }

    terminal->write_ro(RODT_ATTR("\nMqtt Sub Configs :\n"));
    for (uint8_t i = 0; i < MQTT_MAX_SUBSCRIBE_TOPIC; i++) {

      if( strlen(_mqtt_pubsub_configs.subscribe_topics[i].topic) > 0 ){

        terminal->write(_mqtt_pubsub_configs.subscribe_topics[i].topic); terminal->write(RODT_ATTR("\t"));
        terminal->write((int32_t)_mqtt_pubsub_configs.subscribe_topics[i].qos); terminal->write(RODT_ATTR("\n"));
      }
    }
  }
}


MqttServiceProvider __mqtt_service;

#endif
