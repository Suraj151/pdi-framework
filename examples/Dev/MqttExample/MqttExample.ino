/* Demo example about how to use mqtt service provider
 */

#include <PdiStack.h>

// mqtt grneral configuration
#define MQTT_HOST             "test.mosquitto.org" // using mosquitto test server for example
#define MQTT_PORT             1883
#define MQTT_CLIENT_ID        "client_[mac]"
#define MQTT_USERNAME         ""
#define MQTT_PASSWORD         ""
#define MQTT_KEEP_ALIVE       30

// mqtt publish / subscribe configuration
#define MQTT_PUBLISH_TOPIC    "pidstack_[mac]"
#define MQTT_SUBSCRIBE_TOPIC  "pidstack_[mac]"
#define MQTT_PUBLISH_FREQ     5   // publish after every 5 second
#define MQTT_PUBLISH_QOS      0
#define MQTT_SUBSCRIBE_QOS    0

// mqtt lwt configuration
#define MQTT_WILL_TOPIC       "disconnect"
#define MQTT_WILL_MESSAGE     "[mac] is disconnected" // [mac] will get replaced internally with actual mac id
#define MQTT_WILL_QOS         0

#if defined(ENABLE_MQTT_SERVICE)


// mqtt service will call this function whenever it initiate publish process to set user data for publish in payload
// sending demo data in json format
void publish_callback( char* _payload, uint16_t _length ){

  memset( _payload, 0, _length );

  String _data_to_publish = "";
  _data_to_publish += "{\"device_id\":[mac], \"value\":";
  _data_to_publish += random(0, 100);
  _data_to_publish += "}";

  _data_to_publish.toCharArray( _payload, _length );
}


// mqtt service will call this function whenever it receive data on subscribed topic
void subscribe_callback( uint32_t *args, const char* topic, uint32_t topic_len, const char *data, uint32_t data_len ){

  char *topicBuf = pdiutil::safe_new_array<char>(topic_len+1), *dataBuf = pdiutil::safe_new_array<char>(data_len+1);

  if( nullptr == topicBuf || nullptr == dataBuf ){
    pdiutil::safe_delete_array(topicBuf);
    pdiutil::safe_delete_array(dataBuf);
    return;
  }

  memcpy(topicBuf, topic, topic_len);
  topicBuf[topic_len] = 0;

  memcpy(dataBuf, data, data_len);
  dataBuf[data_len] = 0;

  Serial.println(F("\n\nMQTT: user data callback"));
  Serial.printf("MQTT: user Receive topic: %s, data: %s \n\n", topicBuf, dataBuf);

  pdiutil::safe_delete_array(topicBuf); pdiutil::safe_delete_array(dataBuf);
}

// copy text into a fixed size config field.
// clear it first or a shorter new value leaves the tail of the old one behind,
// and never copy past the end of the field
void set_config_field( char *_field, uint16_t _fieldsize, const char *_value ){

  memset( _field, 0, _fieldsize );
  memcpy( _field, _value, pdistd::min( (size_t)(_fieldsize - 1), strlen(_value) ) );
}

// each table below is taken in its own function on purpose.
// a config table is a few hundred bytes, so holding two or three of them on the
// same stack at once is enough to overflow it on a small board
void configure_mqtt_general(){

  mqtt_general_config_table _mqtt_general_configs;
  __database_service.get_mqtt_general_config_table(&_mqtt_general_configs);

  set_config_field( _mqtt_general_configs.host, MQTT_HOST_BUF_SIZE, MQTT_HOST );
  set_config_field( _mqtt_general_configs.client_id, MQTT_CLIENT_ID_BUF_SIZE, MQTT_CLIENT_ID );
  set_config_field( _mqtt_general_configs.username, MQTT_USERNAME_BUF_SIZE, MQTT_USERNAME );
  set_config_field( _mqtt_general_configs.password, MQTT_PASSWORD_BUF_SIZE, MQTT_PASSWORD );
  _mqtt_general_configs.port = MQTT_PORT;
  _mqtt_general_configs.keepalive = MQTT_KEEP_ALIVE;

  __database_service.set_mqtt_general_config_table( &_mqtt_general_configs );
}

// by default 2 publish and subscribe topics are suported you can change it in mqtt configuration file
void configure_mqtt_pubsub(){

  mqtt_pubsub_config_table _mqtt_pubsub_configs;
  __database_service.get_mqtt_pubsub_config_table(&_mqtt_pubsub_configs);

  set_config_field( _mqtt_pubsub_configs.publish_topics[0].topic, MQTT_TOPIC_BUF_SIZE, MQTT_PUBLISH_TOPIC );
  _mqtt_pubsub_configs.publish_topics[0].qos = MQTT_PUBLISH_QOS;
  set_config_field( _mqtt_pubsub_configs.subscribe_topics[0].topic, MQTT_TOPIC_BUF_SIZE, MQTT_SUBSCRIBE_TOPIC );
  _mqtt_pubsub_configs.subscribe_topics[0].qos = MQTT_SUBSCRIBE_QOS;
  _mqtt_pubsub_configs.publish_frequency = MQTT_PUBLISH_FREQ;

  __database_service.set_mqtt_pubsub_config_table( &_mqtt_pubsub_configs );
}

void configure_mqtt_lwt(){

  mqtt_lwt_config_table _mqtt_lwt_configs;
  __database_service.get_mqtt_lwt_config_table(&_mqtt_lwt_configs);

  set_config_field( _mqtt_lwt_configs.will_topic, MQTT_TOPIC_BUF_SIZE, MQTT_WILL_TOPIC );
  set_config_field( _mqtt_lwt_configs.will_message, MQTT_WILL_MSG_BUF_SIZE, MQTT_WILL_MESSAGE );
  _mqtt_lwt_configs.will_qos = MQTT_WILL_QOS;

  __database_service.set_mqtt_lwt_config_table( &_mqtt_lwt_configs );
}

void configure_mqtt(){

  configure_mqtt_general();
  configure_mqtt_pubsub();
  configure_mqtt_lwt();

  // set publish subscribe callbacks
  __mqtt_service.setMqttPublishDataCallback( publish_callback );
  __mqtt_service.setMqttSubscribeDataCallback( subscribe_callback );

  // start mqtt service with new configuration immediate after this call. e.g. here after 10 ms
  __task_scheduler.setTimeout( [&]() { __mqtt_service.handleMqttConfigChange(); }, 10, millis() );
}


#else
  #error "Mqtt portal service is disabled ( in devices/DeviceConfig.h of framework library ). please enable(uncomment ENABLE_MQTT_SERVICE) it for this example"
#endif

void setup() {
  PdiStack.initialize();

  // call it only after framework initialization
  configure_mqtt();
}

void loop() {

  PdiStack.serve();
}
