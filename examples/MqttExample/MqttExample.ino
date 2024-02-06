/* Demo example about how to use mqtt service provider
 */

#include <EwingsEspStack.h>

// mqtt grneral configuration
#define MQTT_HOST             "---mqtt host---"
#define MQTT_PORT             1883
#define MQTT_CLIENT_ID        "---client id---"
#define MQTT_USERNAME         ""
#define MQTT_PASSWORD         ""
#define MQTT_KEEP_ALIVE       60

// mqtt publish / subscribe configuration
#define MQTT_PUBLISH_TOPIC    "test_publish"
#define MQTT_SUBSCRIBE_TOPIC  "test_subscribe"
#define MQTT_PUBLISH_FREQ     5   // publish after every 5 second
#define MQTT_PUBLISH_QOS      0
#define MQTT_SUBSCRIBE_QOS    0

// mqtt lwt configuration
#define MQTT_WILL_TOPIC       "disconnect"
#define MQTT_WILL_MESSAGE     "[mac] is disconnected" // [mac] will get replaced internally with actual mac id
#define MQTT_WILL_QOS         0

#if defined(ENABLE_EWING_HTTP_SERVER)


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

  char *topicBuf = new char[topic_len+1], *dataBuf = new char[data_len+1];

  memcpy(topicBuf, topic, topic_len);
  topicBuf[topic_len] = 0;

  memcpy(dataBuf, data, data_len);
  dataBuf[data_len] = 0;

  Logln(F("\n\nMQTT: user data callback"));
  Serial.printf("MQTT: user Receive topic: %s, data: %s \n\n", topicBuf, dataBuf);

  delete[] topicBuf; delete[] dataBuf;
}

void configure_mqtt(){

  // take mqtt tables from database
  mqtt_general_config_table _mqtt_general_configs;
  mqtt_lwt_config_table _mqtt_lwt_configs;
  mqtt_pubsub_config_table _mqtt_pubsub_configs;
  __database_service.get_mqtt_general_config_table(&_mqtt_general_configs);
  __database_service.get_mqtt_lwt_config_table(&_mqtt_lwt_configs);
  __database_service.get_mqtt_pubsub_config_table(&_mqtt_pubsub_configs);

  // copy general configs in mqtt general table
  memcpy( _mqtt_general_configs.host, MQTT_HOST, strlen( MQTT_HOST ) );
  _mqtt_general_configs.port = MQTT_PORT;
  memcpy( _mqtt_general_configs.client_id, MQTT_CLIENT_ID, strlen( MQTT_CLIENT_ID ) );
  memcpy( _mqtt_general_configs.username, MQTT_USERNAME, strlen( MQTT_USERNAME ) );
  memcpy( _mqtt_general_configs.password, MQTT_PASSWORD, strlen( MQTT_PASSWORD ) );
  _mqtt_general_configs.keepalive = MQTT_KEEP_ALIVE;

  // copy publish / subscribe configs in mqtt pubsub table
  // by default 2 publish and subscribe topics are suported you can change it in mqtt configuration file
  memcpy( _mqtt_pubsub_configs.publish_topics[0].topic, MQTT_PUBLISH_TOPIC, strlen( MQTT_PUBLISH_TOPIC ) );
  _mqtt_pubsub_configs.publish_topics[0].qos = MQTT_PUBLISH_QOS;
  memcpy( _mqtt_pubsub_configs.subscribe_topics[0].topic, MQTT_SUBSCRIBE_TOPIC, strlen( MQTT_SUBSCRIBE_TOPIC ) );
  _mqtt_pubsub_configs.subscribe_topics[0].qos = MQTT_SUBSCRIBE_QOS;
  _mqtt_pubsub_configs.publish_frequency = MQTT_PUBLISH_FREQ;

  // copy lwt configs in mqtt lwt table
  memcpy( _mqtt_lwt_configs.will_topic, MQTT_WILL_TOPIC, strlen( MQTT_WILL_TOPIC ) );
  memcpy( _mqtt_lwt_configs.will_message, MQTT_WILL_MESSAGE, strlen( MQTT_WILL_MESSAGE ) );
  _mqtt_lwt_configs.will_qos = MQTT_WILL_QOS;

  // set config tables back in database
  __database_service.set_mqtt_general_config_table( &_mqtt_general_configs );
  __database_service.set_mqtt_lwt_config_table( &_mqtt_lwt_configs );
  __database_service.set_mqtt_pubsub_config_table( &_mqtt_pubsub_configs );

  // set publish subscribe callbacks
  __mqtt_service.setMqttPublishDataCallback( publish_callback );
  __mqtt_service.setMqttSubscribeDataCallback( subscribe_callback );

  // start mqtt service with new configuration immediate after this call. e.g. here after 10 ms
  __task_scheduler.setTimeout( [&]() { __mqtt_service.handleMqttConfigChange(); }, 10, millis() );
}


#else
  #error "Mqtt service is disabled ( in config/Common.h of framework library ). please enable(uncomment) it for this example"
#endif

void setup() {
  EwStack.initialize();

  // call it only after framework initialization
  configure_mqtt();
}

void loop() {

  EwStack.serve();
}
