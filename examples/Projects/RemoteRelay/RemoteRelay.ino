/* Remote relay : one output switched from the shell, the portal and MQTT
 *
 * This example puts several features together the way a real device would. One
 * pin drives a relay (or an LED to try it), and it can be switched three ways :
 *   - the `relay` shell command, over serial, telnet and ssh : relay on, relay off
 *   - a /relay page on the web portal, behind the login, with on and off buttons
 *   - an MQTT topic, when a broker is set in MQTT_HOST : publish "on" or "off" to it
 * Whichever way it is switched, the state is kept, so the relay comes back the
 * way it was after a restart.
 *
 * The page changes the relay only on a POST from its own form, which carries the
 * session's csrf token; the framework refuses a POST without it. A link or a GET
 * that switched the relay would let any other web page switch it through a
 * logged in browser, so never change state on a GET.
 *
 * Built from the single feature examples : Dev/GpioControl, Dev/AddingCommand,
 * Dev/AddingController and Dev/MqttExample.
 */

#include <PdiStack.h>

#if defined(ENABLE_GPIO_SERVICE) && defined(ENABLE_STORAGE_SERVICE) && defined(ENABLE_CMD_SERVICE) && defined(ENABLE_HTTP_SERVER)

#define RELAY_PIN_DIR     "/sys/class/gpio/4"        /* framework pin 4 : D4, the on-board LED, on a NodeMCU */
#define MODE_DIGITAL_WRITE "1"                       /* the mode number `echo 1 > .../mode` takes */
#define RELAY_ROUTE       "/relay"

#define MQTT_HOST         ""                         /* your broker, empty to leave MQTT as it is */
#define MQTT_PORT         1883
#define MQTT_SECURE       0                          /* 1 for a broker that takes TLS (mqtts), usually port 8883 */
#define MQTT_CLIENT_ID    "pdistack_[mac]"           /* must not be empty : no client id, no connection */
#define MQTT_KEEP_ALIVE   30                         /* seconds; the client starts only with more than 5 */
#define MQTT_TOPIC        "pdistack_[mac]/relay"     /* [mac] is replaced with the board's mac */

#define CMD_NAME_RELAY    "relay"

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
 * whether the relay is on now
 */
bool relay_is_on(){

	return read_number( RELAY_PIN_DIR "/value" ) > 0;
}

/**
 * switch the relay. The pin's value is written only when it changes : each write
 * is also saved to flash, which is what brings the state back after a restart
 */
void set_relay( bool _on ){

	if( relay_is_on() == _on ){
		return;
	}
	const char *value = _on ? "1" : "0";
	__i_fs.writeFile( RELAY_PIN_DIR "/value", value, 1 );
}

/**
 * make the pin a digital output, unless it already is one
 */
void prepare_pin(){

	if( read_number(RELAY_PIN_DIR "/mode") != (int32_t)StringToUint32(MODE_DIGITAL_WRITE) ){
		__i_fs.writeFile( RELAY_PIN_DIR "/mode", MODE_DIGITAL_WRITE, strlen(MODE_DIGITAL_WRITE) );
	}
}

/**
 * `relay` prints the state, `relay on` and `relay off` switch it
 */
struct RelayCommand : public CommandBase {

	RelayCommand(){
		Clear();
		SetCommand(CMD_NAME_RELAY);
		setArgs(RODT_ATTR(""), 1);
		setAcceptArgsOptions(true);
		setCmdOptionSeparator(CMD_OPTION_SEPERATOR_SPACE);
	}

	static void RegisterCommand(){
		CommandBase::RegisterCommand(CMD_NAME_RELAY, [](void *arg)->void *{
			return pdiutil::safe_new<RelayCommand>();
		});
	}

	const char* getUsage() const override {
		return RODT_ATTR("relay [on|off]  show the relay, or switch it");
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

		CommandOption *word = &m_options[0];
		bool given = nullptr != word->optionval && 0 < word->optionvalsize;
		if( given ){
			if( 2 == word->optionvalsize && 0 == strncmp(word->optionval, "on", 2) ){
				set_relay( true );
			}else if( 3 == word->optionvalsize && 0 == strncmp(word->optionval, "off", 3) ){
				set_relay( false );
			}else{
				m_terminal->putln();
				m_terminal->writeln_ro( RODT_ATTR("relay takes on or off") );
				return CMD_ERROR_INVAL;
			}
		}

		m_terminal->putln();
		m_terminal->writeln_ro( relay_is_on() ? RODT_ATTR("relay is on") : RODT_ATTR("relay is off") );
		return PDI_OK;
	}
};

/**
 * the /relay page : the state and two buttons, each a small form that posts back
 * here with the csrf token.
 *
 * A page is built in a PAGE_HTML_MAX_SIZE buffer, and the framework's header and
 * footer alone take most of it, so never build a whole page in one buffer : send
 * it in chunks, flushing the buffer with CONTINUE_SEND_IN_CHUNK after each piece,
 * the way the framework's own controllers do. Any piece then only has to fit on
 * its own.
 */
class RelayController : public Controller {

	public:

		RelayController():Controller("relay"){}

		void boot( void ){
			this->m_route_handler->register_route( RELAY_ROUTE, [&]() { this->handleRelayRoute(); }, AUTH_MIDDLEWARE );
		}

		void add_button( char *_page, const char *_state, const char *_label ){

			strcat( _page, "<form method='post' action='" RELAY_ROUTE "' style='display:inline'>" );
			concat_csrf_input_html_tag( _page );
			strcat( _page, "<input type='hidden' name='state' value='" );
			strcat( _page, _state );
			strcat( _page, "'><button>" );
			strcat( _page, _label );
			strcat( _page, "</button></form> " );
			CONTINUE_SEND_IN_CHUNK( _page );
		}

		void handleRelayRoute( void ){

			// a POST reaches here only with a valid csrf token, the framework checks it first
			if( this->m_web_resource->m_server->isPostRequest() && this->m_web_resource->m_server->hasArg("state") ){
				set_relay( this->m_web_resource->m_server->arg("state") == "on" );
			}

			char *_page = pdiutil::safe_new_array<char>( PAGE_HTML_MAX_SIZE );
			if( nullptr == _page ){
				this->m_web_resource->m_server->send( HTTP_RESP_SERVICE_UNAVAILABLE );
				return;
			}

			BEGIN_SEND_IN_CHUNK( HTTP_RESP_OK, MIME_TYPE_TEXT_HTML, _page );

			concat_header_html( _page );

			strcat( _page, "<div style='text-align:center;margin:2em'><h2>Relay is " );
			strcat( _page, relay_is_on() ? "on" : "off" );
			strcat( _page, "</h2>" );
			CONTINUE_SEND_IN_CHUNK( _page );

			add_button( _page, "on", "Turn on" );
			add_button( _page, "off", "Turn off" );

			strcat( _page, "</div>" );
			strcat_ro( _page, WEB_SERVER_FOOTER_HTML );
			CONTINUE_SEND_IN_CHUNK( _page );

			END_SENDING_CHUNK();
			pdiutil::safe_delete_array( _page );
		}
};

/**
 * this should be defined before framework initialization
 */
RelayController relay_controller;

#if defined(ENABLE_MQTT_SERVICE)

/**
 * a message on the subscribed topic : "on" or "off" switches the relay
 */
void relay_message( uint32_t *args, const char *topic, uint32_t topic_len, const char *data, uint32_t data_len ){

	if( 2 == data_len && 0 == strncmp(data, "on", 2) ){
		set_relay( true );
	}else if( 3 == data_len && 0 == strncmp(data, "off", 3) ){
		set_relay( false );
	}
}

/**
 * copy text into a fixed size config field, clearing it first
 */
void set_field( char *_field, uint16_t _fieldsize, const char *_value ){

	memset( _field, 0, _fieldsize );
	memcpy( _field, _value, pdistd::min( (size_t)(_fieldsize - 1), strlen(_value) ) );
}

/**
 * point mqtt at the broker and subscribe to the topic, each table in its own
 * function so two are never on the stack at once
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
	if( 0 == strncmp(pubsub.subscribe_topics[0].topic, MQTT_TOPIC, MQTT_TOPIC_BUF_SIZE) ){
		return;
	}
	set_field( pubsub.subscribe_topics[0].topic, MQTT_TOPIC_BUF_SIZE, MQTT_TOPIC );
	__database_service.set_mqtt_pubsub_config_table( &pubsub );
}

void configure_mqtt(){

	if( 0 == strlen(MQTT_HOST) ){
		return;
	}
	configure_mqtt_general();
	configure_mqtt_pubsub();
	__mqtt_service.setMqttSubscribeDataCallback( relay_message );
	__task_scheduler.setTimeout( [&]() { __mqtt_service.handleMqttConfigChange(); }, 10, millis() );
}

#endif

#else
  #error "this example needs the GPIO, storage, command and HTTP server services ( ENABLE_GPIO_SERVICE, ENABLE_STORAGE_SERVICE, ENABLE_CMD_SERVICE, ENABLE_HTTP_SERVER in devices/DeviceConfig.h )"
#endif


void setup() {

	PdiStack.initialize();

	prepare_pin();
	RelayCommand::RegisterCommand();

	#if defined(ENABLE_MQTT_SERVICE)
	configure_mqtt();
	#endif
}

void loop() {
	PdiStack.serve();
}
