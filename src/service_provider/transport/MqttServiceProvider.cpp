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
#include <helpers/FeatureConfigFiles.h>

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
  ServiceProvider(SERVICE_MQTT, RODT_ATTR("MQTT"))
{
}

/**
 * MqttServiceProvider destructor
 */
MqttServiceProvider::~MqttServiceProvider(){
  this->stop();
  if( nullptr != this->m_mqtt_payload ){
    pdiutil::safe_delete_array(this->m_mqtt_payload);
    this->m_mqtt_payload = nullptr;
  }
  this->m_mqtt_publish_data_cb = nullptr;
  this->m_mqtt_subscribe_data_cb = nullptr;
}

/**
 * start mqtt service. initialize it with mqtt configs at database
 */
bool MqttServiceProvider::initService( void *arg ){

  // m_client = reinterpret_cast<iClientInterface*>(arg);
  this->m_mqtt_payload = pdiutil::safe_new_array<char>( MQTT_PAYLOAD_BUF_SIZE );

  if ( nullptr == this->m_mqtt_payload ) {
    return false;
  }

  this->m_mqtt_client.m_mqttDataCallbackArgs = reinterpret_cast<uint32_t*>(this);
  this->m_mqtt_client.OnData( MqttServiceProvider::handleMqttDataCb );

#ifdef ENABLE_MQTT_CONFIG_FILE
  syncMqttConfigFile();
#endif

  this->handleMqttConfigChange();

  iTerminalInterface *line = serviceBootLine();
  if( nullptr != line ){
    line->writeln_ro(RODT_ATTR("client ready"));
  }

  return ServiceProvider::initService(arg);
}

/**
 * check and do mqtt publish on given configs.
 */
void MqttServiceProvider::handleMqttPublish(bool sync){

  LogI("MQTT: handling mqtt publish interval, %d\n", (int)sync);
  if( !this->m_mqtt_client.is_mqtt_connected() ) return;

  mqtt_pubsub_config_table _mqtt_pubsub_configs;
  __database_service.get_mqtt_pubsub_config_table(&_mqtt_pubsub_configs);

  pdiutil::string mac_placeholder = CHARPTR_WRAP("[mac]");

  for (uint8_t i = 0; i < MQTT_MAX_PUBLISH_TOPIC; i++) {

    __find_and_replace( _mqtt_pubsub_configs.publish_topics[i].topic, mac_placeholder.c_str(), __i_dvc_ctrl.getDeviceMac().c_str(), 2, MQTT_TOPIC_BUF_SIZE );
    if( nullptr != this->m_mqtt_payload && strlen(_mqtt_pubsub_configs.publish_topics[i].topic) > 0 ){

      LogI("MQTT: publishing on topic : %s\n", _mqtt_pubsub_configs.publish_topics[i].topic);

      #ifdef ENABLE_MQTT_DEFAULT_PAYLOAD

        pdiutil::string *_payload = pdiutil::safe_new<pdiutil::string>();

        if( nullptr != _payload ){

          #ifdef ENABLE_GPIO_SERVICE

            __gpio_service.appendGpioJsonPayload( *_payload );
          #else

            *_payload += "Hello from PDI Client : ";
            *_payload += __i_dvc_ctrl.getDeviceId();
          #endif

          setMqttPayload(_payload->c_str(), _payload->size());

          pdiutil::safe_delete(_payload);
        }

      #else

      #endif

      if( nullptr != this->m_mqtt_publish_data_cb ){
        this->m_mqtt_publish_data_cb( this->m_mqtt_payload, MQTT_PAYLOAD_BUF_SIZE );
      }
      __find_and_replace( this->m_mqtt_payload, mac_placeholder.c_str(), __i_dvc_ctrl.getDeviceMac().c_str(), 2, MQTT_PAYLOAD_BUF_SIZE );

      bool ret = this->m_mqtt_client.Publish(
        _mqtt_pubsub_configs.publish_topics[i].topic,
        this->m_mqtt_payload,
        strlen( this->m_mqtt_payload ),
        _mqtt_pubsub_configs.publish_topics[i].qos < MQTT_MAX_QOS_LEVEL ?
        _mqtt_pubsub_configs.publish_topics[i].qos : MQTT_MAX_QOS_LEVEL,
        _mqtt_pubsub_configs.publish_topics[i].retain
      );

      if( ret && sync ){
        this->m_mqtt_client.MQTT_Task();
      }
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

  pdiutil::string mac_placeholder = CHARPTR_WRAP("[mac]");

  for (uint8_t i = 0; i < MQTT_MAX_SUBSCRIBE_TOPIC; i++) {

    __find_and_replace( _mqtt_pubsub_configs.subscribe_topics[i].topic, mac_placeholder.c_str(), __i_dvc_ctrl.getDeviceMac().c_str(), 2, MQTT_TOPIC_BUF_SIZE );

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
 * Copy what the next publish should carry into the buffer this service owns,
 * refusing while it holds none and when the text does not fit.
 */
bool MqttServiceProvider::setMqttPayload(const char *data, uint16_t len){

  if( nullptr == this->m_mqtt_payload ){
    return false;
  }

  memset( this->m_mqtt_payload, 0, MQTT_PAYLOAD_BUF_SIZE );

  if( nullptr == data || (uint32_t)len + 1 >= MQTT_PAYLOAD_BUF_SIZE ){
    pdiutil::string _toobig = CHARPTR_WRAP("mqtt data is too big to fit in buffer !");
    strncpy( this->m_mqtt_payload, _toobig.c_str(), MQTT_PAYLOAD_BUF_SIZE - 1 );
    return false;
  }

  memcpy( this->m_mqtt_payload, data, len );
  return true;
}

/**
 * Forget the task ids, which name tasks the stop has already dropped.
 */
void MqttServiceProvider::resetServiceState(){

  this->m_mqtt_timer_cb_id = 0;
  this->m_mqtt_publish_cb_id = 0;
  this->m_mqtt_subscribe_cb_id = 0;
}

/**
 * Drops the broker connection and releases the client and payload buffer a
 * run allocated, so a stopped mqtt service holds nothing.
 */
bool MqttServiceProvider::stopService(){

  this->stop();

  pdiutil::safe_delete(this->m_client);
  pdiutil::safe_delete_array(this->m_mqtt_payload);

  return ServiceProvider::stopService();
}

/**
 * Build the transport the configured scheme asks for, keeping the one already
 * held when it is of the right kind.
 */
bool MqttServiceProvider::prepareClientForConfig( mqtt_general_config_table *_general ){

  bool _wantsecure = nullptr != _general && IsSecureScheme(
    _general->security ? PROTO_SCHEME_MQTTS : PROTO_SCHEME_MQTT );

  if( nullptr != this->m_client && this->m_client->isSecure() == _wantsecure ){
    return true;
  }

  pdiutil::safe_delete(this->m_client);

#ifdef ENABLE_TLS_SERVICE
  if( _wantsecure ){

    iTlsClientInterface *_tls = __i_instance.getNewTlsClientInstance();

    if( nullptr != _tls ){

      pdiutil::string _cabundle = CHARPTR_WRAP(TLS_DEFAULT_OUTBOUND_CA_BUNDLE_PATH);

      if( __i_instance.getFileSystemInstance().isFileExist(_cabundle.c_str()) &&
          _tls->setCertificateAuthorityPath(_cabundle.c_str()) ){
        _tls->setVerifyPeer(true);
      }else{
        _tls->setVerifyPeer(false);
        SysLogW("MQTT: no trust anchors at %s, broker identity is unverified\n", _cabundle.c_str());
      }

      _tls->setSNIHostname(_general->host);
    }

    this->m_client = _tls;
  }else
#endif
  {
#ifndef ENABLE_TLS_SERVICE
    if( _wantsecure ){
      SysLogW("MQTT: secure connection asked for but this build carries no tls, connecting in the clear\n");
    }
#endif
    this->m_client = __i_instance.getNewTcpClientInstance();
  }

  return nullptr != this->m_client;
}

/**
 * handle restart of mqtt services on config change from autherised client
 *
 * @param   int _mqtt_config_type
 */
void MqttServiceProvider::handleMqttConfigChange( int _mqtt_config_type ){


  mqtt_general_config_table *_mqtt_general_configs = pdiutil::safe_new<mqtt_general_config_table>();
  mqtt_pubsub_config_table *_mqtt_pubsub_configs = pdiutil::safe_new<mqtt_pubsub_config_table>();

  if( nullptr == _mqtt_general_configs || nullptr == _mqtt_pubsub_configs ){
    pdiutil::safe_delete(_mqtt_general_configs);
    pdiutil::safe_delete(_mqtt_pubsub_configs);
    return;
  }

  __database_service.get_mqtt_general_config_table(_mqtt_general_configs);
  __database_service.get_mqtt_pubsub_config_table(_mqtt_pubsub_configs);

  if( MQTT_GENERAL_CONFIG == _mqtt_config_type || MQTT_LWT_CONFIG == _mqtt_config_type ){

    this->m_mqtt_client.DeleteClient();
    int _stat = this->serviceSetTimeout( [&]() {

      mqtt_general_config_table *_general = pdiutil::safe_new<mqtt_general_config_table>();
      mqtt_lwt_config_table *_lwt = pdiutil::safe_new<mqtt_lwt_config_table>();

      if( nullptr == _general || nullptr == _lwt ){
        pdiutil::safe_delete(_general);
        pdiutil::safe_delete(_lwt);
        return;
      }

      __database_service.get_mqtt_general_config_table(_general);
      __database_service.get_mqtt_lwt_config_table(_lwt);

      pdiutil::string mac_placeholder = CHARPTR_WRAP("[mac]");
      __find_and_replace( _general->username, mac_placeholder.c_str(), __i_dvc_ctrl.getDeviceMac().c_str(), 2, MQTT_USERNAME_BUF_SIZE );
      __find_and_replace( _general->client_id, mac_placeholder.c_str(), __i_dvc_ctrl.getDeviceMac().c_str(), 2, MQTT_CLIENT_ID_BUF_SIZE );
      __find_and_replace( _lwt->will_message, mac_placeholder.c_str(), __i_dvc_ctrl.getDeviceMac().c_str(), 2, MQTT_WILL_MSG_BUF_SIZE );

      bool _began = this->prepareClientForConfig( _general ) &&
                    this->m_mqtt_client.begin( m_client, _general, _lwt );

      pdiutil::safe_delete(_general);
      pdiutil::safe_delete(_lwt);

      if( _began ){
        this->m_mqtt_timer_cb_id = this->serviceUpdateInterval(
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

    // UnSubscribe removes the topic from the list being walked, so step down
    // through it or the entry shifted into this slot is never examined
    for ( int32_t i = (int32_t)this->m_mqtt_client.m_mqttClient.subscribed_topics.size() - 1; i >= 0; i--) {

      bool _found = false;
      for (uint8_t j = 0; j < MQTT_MAX_SUBSCRIBE_TOPIC; j++) {

        if( __are_str_equals(
          _mqtt_pubsub_configs->subscribe_topics[j].topic,
          this->m_mqtt_client.m_mqttClient.subscribed_topics[i].topic,
          strlen( _mqtt_pubsub_configs->subscribe_topics[j].topic )
        ) ) _found = true;

      }
      if( !_found )
        this->m_mqtt_client.UnSubscribe( this->m_mqtt_client.m_mqttClient.subscribed_topics[i].topic );
    }

  }else{

  }

  if( _mqtt_pubsub_configs->publish_frequency > 0 ){
    this->m_mqtt_publish_cb_id = this->serviceUpdateInterval(
      this->m_mqtt_publish_cb_id,
      [&]() { this->handleMqttPublish(); },
      _mqtt_pubsub_configs->publish_frequency*MILLISECOND_DURATION_1000
    );
  }else{
    __task_scheduler.clearInterval( this->m_mqtt_publish_cb_id );
    this->m_mqtt_publish_cb_id = 0;
  }

  if( _mqtt_general_configs->keepalive > 0 ){
    this->m_mqtt_subscribe_cb_id = this->serviceUpdateInterval(
      this->m_mqtt_subscribe_cb_id,
      [&]() { this->handleMqttSubScribe(); },
      (_mqtt_general_configs->keepalive/2)*MILLISECOND_DURATION_1000,
      DEFAULT_TASK_PRIORITY, ( __i_dvc_ctrl.millis_now() + MQTT_INITIALIZE_DURATION )
    );
  }else{
    __task_scheduler.clearInterval( this->m_mqtt_subscribe_cb_id );
    this->m_mqtt_subscribe_cb_id = 0;
  }

  pdiutil::safe_delete(_mqtt_general_configs);
  pdiutil::safe_delete(_mqtt_pubsub_configs);
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

    char *topicBuf = pdiutil::safe_new_array<char>(topic_len+1), *dataBuf = pdiutil::safe_new_array<char>(data_len+1);

    if( nullptr != topicBuf ){
      memcpy(topicBuf, topic, topic_len);
      topicBuf[topic_len] = 0;
      pdiutil::safe_delete_array(topicBuf);
    }

    if( nullptr != dataBuf ){
      memcpy(dataBuf, data, data_len);
      dataBuf[data_len] = 0;

      #if defined( ENABLE_MQTT_DEFAULT_PAYLOAD ) && defined( ENABLE_GPIO_SERVICE )
      __gpio_service.applyGpioJsonPayload( dataBuf, data_len );
      #endif

      pdiutil::safe_delete_array(dataBuf);
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

    terminal->putln(); terminal->writeln_ro(RODT_ATTR("Mqtt General Configs :"));
    terminal->write(_mqtt_general_configs.host); terminal->write_ro(RODT_ATTR("\t"));
    terminal->write((int32_t)_mqtt_general_configs.port); terminal->write_ro(RODT_ATTR("\t"));
    terminal->write((int32_t)_mqtt_general_configs.security); terminal->write_ro(RODT_ATTR("\t"));
    terminal->write(_mqtt_general_configs.client_id); terminal->write_ro(RODT_ATTR("\t"));
    terminal->write(_mqtt_general_configs.username); terminal->write_ro(RODT_ATTR("\t"));
    terminal->write(_mqtt_general_configs.password); terminal->write_ro(RODT_ATTR("\t"));
    terminal->write((int32_t)_mqtt_general_configs.keepalive); terminal->write_ro(RODT_ATTR("\t"));
    terminal->write((int32_t)_mqtt_general_configs.clean_session); terminal->putln();


    terminal->putln(); terminal->writeln_ro(RODT_ATTR("Mqtt Lwt Configs :"));
    terminal->write(_mqtt_lwt_configs.will_topic); terminal->write_ro(RODT_ATTR("\t"));
    terminal->write(_mqtt_lwt_configs.will_message); terminal->write_ro(RODT_ATTR("\t"));
    terminal->write((int32_t)_mqtt_lwt_configs.will_qos); terminal->write_ro(RODT_ATTR("\t"));
    terminal->write((int32_t)_mqtt_lwt_configs.will_retain); terminal->putln();

    terminal->putln(); terminal->writeln_ro(RODT_ATTR("Mqtt Pub Configs :"));
    for (uint8_t i = 0; i < MQTT_MAX_PUBLISH_TOPIC; i++) {

      if( strlen(_mqtt_pubsub_configs.publish_topics[i].topic) > 0 ){

        terminal->write(_mqtt_pubsub_configs.publish_topics[i].topic); terminal->write_ro(RODT_ATTR("\t"));
        terminal->write((int32_t)_mqtt_pubsub_configs.publish_topics[i].qos); terminal->write_ro(RODT_ATTR("\t"));
        terminal->write((int32_t)_mqtt_pubsub_configs.publish_topics[i].retain); terminal->putln();
      }
    }

    terminal->putln(); terminal->writeln_ro(RODT_ATTR("Mqtt Sub Configs :"));
    for (uint8_t i = 0; i < MQTT_MAX_SUBSCRIBE_TOPIC; i++) {

      if( strlen(_mqtt_pubsub_configs.subscribe_topics[i].topic) > 0 ){

        terminal->write(_mqtt_pubsub_configs.subscribe_topics[i].topic); terminal->write_ro(RODT_ATTR("\t"));
        terminal->write((int32_t)_mqtt_pubsub_configs.subscribe_topics[i].qos); terminal->putln();
      }
    }
  }
}


MqttServiceProvider __mqtt_service;

#endif
