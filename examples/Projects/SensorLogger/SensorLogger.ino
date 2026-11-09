/* Sensor logger : a reading on a schedule, kept, shown and published
 *
 * This example puts several features together the way a real device would :
 *   - a task reads an analog input every SAMPLE_EVERY_MS,
 *   - each reading is added to a CSV file that never grows past LOG_MAX bytes,
 *   - a `sensor` shell command shows the latest reading and the range seen,
 *     over serial, telnet and ssh alike,
 *   - when a broker is set in MQTT_HOST, the latest reading is published as
 *     JSON every MQTT_PUBLISH_FREQ seconds.
 *
 * Wire a sensor with an analog output (a potentiometer is enough to try it) to
 * the board's first analog pin : A0 on an esp8266, GPIO 32 on an esp32 dev board.
 *
 * After it has run you can log in and look :
 *   sensor
 *   cat /data/sensor.csv
 *   sftp the CSV off the board, or open it on the portal's storage page
 *
 * Built from the single feature examples : Dev/GpioControl, Dev/FilesAndProc,
 * Dev/AddingCommand and Dev/MqttExample.
 */

#include <PdiStack.h>

#if defined(ENABLE_GPIO_SERVICE) && defined(ENABLE_STORAGE_SERVICE) && defined(ENABLE_CMD_SERVICE)

#define SENSOR_PIN        MAX_DIGITAL_GPIO_PINS      /* the first analog pin of this board */
#define MODE_ANALOG_READ  "5"                        /* the mode number `echo 5 > .../mode` takes */
#define SAMPLE_EVERY_MS   (10 * MILLISECOND_DURATION_1000)

#define LOG_DIR           "/data"
#define LOG_PATH          LOG_DIR "/sensor.csv"
#define LOG_OLD_PATH      LOG_DIR "/sensor.csv.1"
#define LOG_MAX           8192                       /* bytes before the log is rotated */
#define LOG_HEADER        "uptime_s,value" TERMINAL_NEW_LINE

#define MQTT_HOST         ""                         /* your broker, empty to leave MQTT as it is */
#define MQTT_PORT         1883
#define MQTT_SECURE       0                          /* 1 for a broker that takes TLS (mqtts), usually port 8883 */
#define MQTT_CLIENT_ID    "pdistack_[mac]"           /* must not be empty : no client id, no connection */
#define MQTT_KEEP_ALIVE   30                         /* seconds; the client starts only with more than 5 */
#define MQTT_TOPIC        "pdistack_[mac]/sensor"    /* [mac] is replaced with the board's mac */
#define MQTT_PUBLISH_FREQ 30

#define CMD_NAME_SENSOR   "sensor"

struct sensor_state {
	int32_t last = -1;
	int32_t low = -1;
	int32_t high = -1;
	uint32_t samples = 0;
};

static sensor_state s_sensor;

/**
 * the path of one leaf of the sensor pin
 */
pdiutil::string pin_path( const char *_leaf ){

	pdiutil::string path = "/sys/class/gpio/";
	path += pdiutil::to_string( (uint32_t)SENSOR_PIN );
	path += "/";
	path += _leaf;
	return path;
}

/**
 * the first few bytes of a file as a number, -1 when it cannot be read
 */
int32_t read_number( const char *_path ){

	char text[16] = {0};
	uint32_t got = 0;
	int status = __i_fs.readFile( _path, sizeof(text) - 1, [&](char *data, uint32_t size)->bool{
		uint32_t take = pdistd::min( size, (uint32_t)(sizeof(text) - 1 - got) );
		memcpy( text + got, data, take );
		got += take;
		return got < sizeof(text) - 1;
	});
	return (status < 0 || 0 == got) ? -1 : (int32_t)StringToUint32( text );
}

/**
 * set the pin to an analog input, unless it already is one
 */
void prepare_pin(){

	pdiutil::string mode = pin_path( "mode" );
	if( read_number(mode.c_str()) != (int32_t)StringToUint32(MODE_ANALOG_READ) ){
		__i_fs.writeFile( mode.c_str(), MODE_ANALOG_READ, strlen(MODE_ANALOG_READ) );
	}
}

/**
 * add one row to the CSV, starting a fresh file with its header when there is
 * none and rotating the full one to sensor.csv.1 first
 */
void log_reading( int32_t _value ){

	if( !__i_fs.isDirExist(LOG_DIR) ){
		__i_fs.createDirectory( LOG_DIR );
	}
	if( __i_fs.getFileSize(LOG_PATH) >= LOG_MAX ){
		__i_fs.deleteFile( LOG_OLD_PATH );
		__i_fs.rename( LOG_PATH, LOG_OLD_PATH );
	}
	if( !__i_fs.isFileExist(LOG_PATH) ){
		__i_fs.writeFile( LOG_PATH, LOG_HEADER, strlen(LOG_HEADER) );
	}

	pdiutil::string row = pdiutil::to_string( (uint32_t)(millis() / 1000) );
	row += ",";
	row += pdiutil::to_string( (int32_t)_value );
	row += TERMINAL_NEW_LINE;
	__i_fs.writeFile( LOG_PATH, row.c_str(), row.size(), true );
}

/**
 * take one reading : remember it, its range, and log it
 */
void sample(){

	int32_t value = read_number( pin_path("value").c_str() );
	if( value < 0 ){
		return;
	}
	s_sensor.last = value;
	s_sensor.low = (s_sensor.low < 0 || value < s_sensor.low) ? value : s_sensor.low;
	s_sensor.high = (value > s_sensor.high) ? value : s_sensor.high;
	s_sensor.samples++;
	log_reading( value );
}

/**
 * `sensor` prints the latest reading and the range seen since boot
 */
struct SensorCommand : public CommandBase {

	SensorCommand(){
		Clear();
		SetCommand(CMD_NAME_SENSOR);
		setArgs(RODT_ATTR(""), 0);
	}

	static void RegisterCommand(){
		CommandBase::RegisterCommand(CMD_NAME_SENSOR, [](void *arg)->void *{
			return pdiutil::safe_new<SensorCommand>();
		});
	}

	const char* getUsage() const override {
		return RODT_ATTR("sensor  latest reading, range since boot and where the log is");
	}

#ifdef ENABLE_AUTH_SERVICE
	bool needauth() override { return true; }
#endif

	pdi_err_t execute(cmd_term_inseq_t terminputaction){

#ifdef ENABLE_AUTH_SERVICE
		if( needauth() && !__auth_service.getAuthorized() ){
			return CMD_ERROR_PERM;
		}
#endif
		if( nullptr == m_terminal ){
			return CMD_ERROR_NOTTY;
		}

		m_terminal->putln();
		if( 0 == s_sensor.samples ){
			m_terminal->writeln_ro( RODT_ATTR("no reading yet") );
			return PDI_OK;
		}
		m_terminal->write_ro( RODT_ATTR("latest  ") );  m_terminal->writeln( s_sensor.last );
		m_terminal->write_ro( RODT_ATTR("lowest  ") );  m_terminal->writeln( s_sensor.low );
		m_terminal->write_ro( RODT_ATTR("highest ") );  m_terminal->writeln( s_sensor.high );
		m_terminal->write_ro( RODT_ATTR("samples ") );  m_terminal->writeln( (int32_t)s_sensor.samples );
		m_terminal->writeln_ro( RODT_ATTR("log     " LOG_PATH) );
		return PDI_OK;
	}
};

#if defined(ENABLE_MQTT_SERVICE)

/**
 * the mqtt service asks for the payload each time it publishes
 */
void publish_reading( char *_payload, uint16_t _length ){

	memset( _payload, 0, _length );
	pdiutil::string json = "{\"device\":\"[mac]\",\"value\":";
	json += pdiutil::to_string( (int32_t)s_sensor.last );
	json += ",\"samples\":";
	json += pdiutil::to_string( (uint32_t)s_sensor.samples );
	json += "}";
	memcpy( _payload, json.c_str(), pdistd::min( (size_t)(_length - 1), json.size() ) );
}

/**
 * copy text into a fixed size config field, clearing it first
 */
void set_field( char *_field, uint16_t _fieldsize, const char *_value ){

	memset( _field, 0, _fieldsize );
	memcpy( _field, _value, pdistd::min( (size_t)(_fieldsize - 1), strlen(_value) ) );
}

/**
 * point mqtt at the broker and the topic, each table in its own function so two
 * are never on the stack at once
 */
void configure_mqtt_general(){

	mqtt_general_config_table general;
	__database_service.get_mqtt_general_config_table( &general );
	if( 0 == strncmp(general.host, MQTT_HOST, MQTT_HOST_BUF_SIZE) && general.port == MQTT_PORT &&
		general.security == MQTT_SECURE && general.keepalive == MQTT_KEEP_ALIVE &&
		0 == strncmp(general.client_id, MQTT_CLIENT_ID, MQTT_CLIENT_ID_BUF_SIZE) ){
		return;
	}
	// set every field the connection needs : a board keeps whatever it had stored
	// before, and the client does not start at all without a client id and a
	// keepalive above 5 s
	set_field( general.host, MQTT_HOST_BUF_SIZE, MQTT_HOST );
	set_field( general.client_id, MQTT_CLIENT_ID_BUF_SIZE, MQTT_CLIENT_ID );
	general.port = MQTT_PORT;
	general.security = MQTT_SECURE;
	general.keepalive = MQTT_KEEP_ALIVE;
	__database_service.set_mqtt_general_config_table( &general );
}

void configure_mqtt_pubsub(){

	mqtt_pubsub_config_table pubsub;
	__database_service.get_mqtt_pubsub_config_table( &pubsub );
	if( 0 == strncmp(pubsub.publish_topics[0].topic, MQTT_TOPIC, MQTT_TOPIC_BUF_SIZE) &&
		pubsub.publish_frequency == MQTT_PUBLISH_FREQ ){
		return;
	}
	set_field( pubsub.publish_topics[0].topic, MQTT_TOPIC_BUF_SIZE, MQTT_TOPIC );
	pubsub.publish_frequency = MQTT_PUBLISH_FREQ;
	__database_service.set_mqtt_pubsub_config_table( &pubsub );
}

void configure_mqtt(){

	if( 0 == strlen(MQTT_HOST) ){
		return;
	}
	configure_mqtt_general();
	configure_mqtt_pubsub();
	__mqtt_service.setMqttPublishDataCallback( publish_reading );
	__task_scheduler.setTimeout( [&]() { __mqtt_service.handleMqttConfigChange(); }, 10, millis() );
}

#endif

#else
  #error "this example needs the GPIO, storage and command services ( ENABLE_GPIO_SERVICE, ENABLE_STORAGE_SERVICE, ENABLE_CMD_SERVICE in devices/DeviceConfig.h )"
#endif


void setup() {

	PdiStack.initialize();

	prepare_pin();
	SensorCommand::RegisterCommand();

	#if defined(ENABLE_MQTT_SERVICE)
	configure_mqtt();
	#endif

	__task_scheduler.setInterval( sample, SAMPLE_EVERY_MS, millis() );
}

void loop() {
	PdiStack.serve();
}
