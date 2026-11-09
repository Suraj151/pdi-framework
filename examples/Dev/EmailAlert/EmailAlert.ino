/* Sending an email when something happens
 *
 * This example illustrate configuring the mail server from the sketch and
 * sending a mail : once when the board has joined the network, and again
 * whenever a digital input pin goes high, for example a door switch or a button
 * wired from the pin to 3.3 V.
 *
 * The mail settings are written once and kept, the same ones the portal's email
 * page shows and lets you change. They are only written when they differ from
 * what is stored, which spares the flash.
 *
 * NOTE : the mail is sent over plain SMTP, with no TLS. Use a relay on your own
 * network or a provider's plain port; a server that only takes encrypted mail
 * will refuse it. The board logs in only when MAIL_USERNAME is set. A mail that is
 * not sent says why in the system log : cat /var/log/syslog.error A mail goes out
 * one step at a time while the board keeps running, and says when the server has
 * answered whether it took it; a mail is refused while another is still going
 * out, and an input that keeps changing sends at most one mail per ALERT_GAP_MS.
 */

#include <PdiStack.h>

#if defined(ENABLE_EMAIL_SERVICE) && defined(ENABLE_WIFI_SERVICE) && defined(ENABLE_GPIO_SERVICE) && defined(ENABLE_STORAGE_SERVICE)

#define MAIL_HOST         "192.168.1.10"            /* your SMTP server */
#define MAIL_PORT         25
#define MAIL_DOMAIN       "pdistack.local"          /* the name the board greets the server with */
#define MAIL_USERNAME     ""                        /* empty when the server takes mail without a login */
#define MAIL_PASSWORD     ""
#define MAIL_FROM         "board@example.com"
#define MAIL_FROM_NAME    "pdiStack"
#define MAIL_TO           "you@example.com"
#define MAIL_SUBJECT      "pdiStack alert"

#define ALERT_PIN_DIR     "/sys/class/gpio/5"       /* framework pin 5 is watched, see doc/services.md "Pin numbers" */
#define ALERT_GAP_MS      (60 * MILLISECOND_DURATION_1000)
#define RETRY_GAP_MS      (30 * MILLISECOND_DURATION_1000)  /* wait before trying the boot mail again */
#define MODE_DIGITAL_READ "2"                     /* the mode number `echo 2 > .../mode` takes */

static bool s_boot_mail_sent = false;
static uint32_t s_boot_mail_tried_at = 0;
static bool s_boot_mail_tried = false;
static int32_t s_last_level = -1;
static uint32_t s_last_alert_at = 0;
static bool s_alerted_once = false;

/**
 * copy text into a fixed size config field.
 * clear it first or a shorter new value leaves the tail of the old one behind,
 * and never copy past the end of the field
 */
bool set_field( char *_field, uint16_t _fieldsize, const char *_value ){

	if( 0 == strncmp(_field, _value, _fieldsize) ){
		return false;
	}
	memset( _field, 0, _fieldsize );
	memcpy( _field, _value, pdistd::min( (size_t)(_fieldsize - 1), strlen(_value) ) );
	return true;
}

/**
 * write the mail settings, only when one of them changed
 */
void configure_mail(){

	email_config_table mail;
	__database_service.get_email_config_table( &mail );

	bool changed = false;
	changed |= set_field( mail.mail_host, DEFAULT_MAIL_HOST_MAX_SIZE, MAIL_HOST );
	changed |= set_field( mail.sending_domain, DEFAULT_SENDING_DOMAIN_MAX_SIZE, MAIL_DOMAIN );
	changed |= set_field( mail.mail_username, DEFAULT_MAIL_USERNAME_MAX_SIZE, MAIL_USERNAME );
	changed |= set_field( mail.mail_password, DEFAULT_MAIL_PASSWORD_MAX_SIZE, MAIL_PASSWORD );
	changed |= set_field( mail.mail_from, DEFAULT_MAIL_FROM_MAX_SIZE, MAIL_FROM );
	changed |= set_field( mail.mail_from_name, DEFAULT_MAIL_FROM_NAME_MAX_SIZE, MAIL_FROM_NAME );
	changed |= set_field( mail.mail_to, DEFAULT_MAIL_TO_MAX_SIZE, MAIL_TO );
	changed |= set_field( mail.mail_subject, DEFAULT_MAIL_SUBJECT_MAX_SIZE, MAIL_SUBJECT );
	if( mail.mail_port != MAIL_PORT ){
		mail.mail_port = MAIL_PORT;
		changed = true;
	}

	if( changed ){
		__database_service.set_email_config_table( &mail );
	}
}

/**
 * print a line on the serial console
 */
void say( const char *_text ){

	iTerminalInterface *terminal = __i_dvc_ctrl.getTerminal();
	if( nullptr != terminal ){
		terminal->with_timestamp()->writeln( _text );
	}
}

/**
 * start one mail and say, once the server has answered, whether it took it.
 * a mail is refused at once while another is still going out, so a caller that
 * must get its mail out keeps _sent and tries again later until it is true
 */
bool send_alert( const char *_body, bool *_sent = nullptr ){

	bool taken = __email_service.sendMail( _body, [_sent](bool sent){
		if( nullptr != _sent ){
			*_sent = sent;
		}
		say( sent ? " mail sent" : " mail not sent, check the server settings and the network" );
	});
	if( !taken ){
		say( " mail not started, another one is still going out or the settings are incomplete" );
	}
	return taken;
}

/**
 * the level the GPIO service last read on the watched pin, -1 when unknown
 */
int32_t read_level(){

	char text[8] = {0};
	uint32_t got = 0;
	int status = __i_fs.readFile( ALERT_PIN_DIR "/value", sizeof(text) - 1, [&](char *data, uint32_t size)->bool{
		uint32_t take = pdistd::min( size, (uint32_t)(sizeof(text) - 1 - got) );
		memcpy( text + got, data, take );
		got += take;
		return got < sizeof(text) - 1;
	});
	return (status < 0 || 0 == got) ? -1 : (int32_t)StringToUint32( text );
}

/**
 * every second : send the boot mail once the board has an address, and a mail
 * when the pin goes from low to high, at most one per ALERT_GAP_MS
 */
void watch(){

	uint32_t now = (uint32_t)millis();

	// the boot mail is only done once the server has taken it
	if( !s_boot_mail_sent && __i_wifi.localIP().isSet() &&
		(!s_boot_mail_tried || (now - s_boot_mail_tried_at) >= RETRY_GAP_MS) ){
		s_boot_mail_tried = true;
		s_boot_mail_tried_at = now;
		send_alert( "the board has started and joined the network", &s_boot_mail_sent );
	}

	int32_t level = read_level();
	if( level < 0 ){
		return;
	}
	bool rose = (0 == s_last_level) && (level > 0);
	s_last_level = level;

	if( rose && (!s_alerted_once || (now - s_last_alert_at) >= ALERT_GAP_MS) ){
		s_alerted_once = true;
		s_last_alert_at = now;
		send_alert( "the alert pin went high" );
	}
}

/**
 * set the watched pin to a digital input, unless it already is one
 */
void watch_pin(){

	const char *mode_path = ALERT_PIN_DIR "/mode";
	char mode[4] = {0};
	__i_fs.readFile( mode_path, sizeof(mode) - 1, [&](char *data, uint32_t size)->bool{
		memcpy( mode, data, pdistd::min( size, (uint32_t)(sizeof(mode) - 1) ) );
		return false;
	});
	if( StringToUint32(MODE_DIGITAL_READ) != StringToUint32(mode) ){
		__i_fs.writeFile( mode_path, MODE_DIGITAL_READ, strlen(MODE_DIGITAL_READ) );
	}
}

#else
  #error "this example needs the email, WiFi, GPIO and storage services ( ENABLE_EMAIL_SERVICE, ENABLE_WIFI_SERVICE, ENABLE_GPIO_SERVICE, ENABLE_STORAGE_SERVICE in devices/DeviceConfig.h )"
#endif


void setup() {

	PdiStack.initialize();

	configure_mail();
	watch_pin();

	__task_scheduler.setInterval( watch, MILLISECOND_DURATION_1000, millis() );
}

void loop() {
	PdiStack.serve();
}
