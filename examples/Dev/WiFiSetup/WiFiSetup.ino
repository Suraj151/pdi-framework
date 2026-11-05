/* Joining your WiFi from the sketch
 *
 * This example puts the board on your own network without the portal or the
 * shell. It writes the station name and password into the WiFi settings once,
 * restarts the WiFi service so it takes them, and then prints the address the
 * router gave it, so you know where to ssh or browse to.
 *
 * The settings are kept, so later boots join on their own. They are only
 * written when they differ from what is stored, which spares the flash.
 * The same thing from the shell is: net connsta,<ssid>,<password>
 */

#include <PdiStack.h>

#define MY_WIFI_SSID      "your-ssid"		/* your router's network name */
#define MY_WIFI_PASSWORD  "your-password"	/* empty for an open network */

#if defined(ENABLE_WIFI_SERVICE)

/**
 * write the station settings when they differ from the stored ones, and
 * restart the WiFi service so it joins with them
 */
void join_my_wifi(){

	wifi_config_table wifi;
	__database_service.get_wifi_config_table(&wifi);

	bool same = (0 == strncmp(wifi.sta_ssid, MY_WIFI_SSID, WIFI_CONFIGS_BUF_SIZE)) &&
				(0 == strncmp(wifi.sta_password, MY_WIFI_PASSWORD, WIFI_CONFIGS_BUF_SIZE)) &&
				wifi.sta_enable;
	if( same ){
		return;
	}

	memset(wifi.sta_ssid, 0, WIFI_CONFIGS_BUF_SIZE);
	strncpy(wifi.sta_ssid, MY_WIFI_SSID, WIFI_CONFIGS_BUF_SIZE - 1);
	memset(wifi.sta_password, 0, WIFI_CONFIGS_BUF_SIZE);
	strncpy(wifi.sta_password, MY_WIFI_PASSWORD, WIFI_CONFIGS_BUF_SIZE - 1);
	wifi.sta_enable = true;

	__database_service.set_wifi_config_table(&wifi);
	__wifi_service.restartService(&__i_wifi);
}

/**
 * print the station address once the board has one, otherwise look again
 * in a second
 */
void print_address_when_joined(){

	ipaddress_t ip = __i_wifi.localIP();
	if( !ip.isSet() ){
		__task_scheduler.setTimeout( print_address_when_joined, MILLISECOND_DURATION_1000, millis() );
		return;
	}

	iTerminalInterface *terminal = __i_dvc_ctrl.getTerminal();
	if( nullptr != terminal ){
		terminal->with_timestamp()->write_ro(RODT_ATTR(" Joined " MY_WIFI_SSID ", address "));
		terminal->writeln(((pdiutil::string)ip).c_str());
	}
}

#endif

void setup() {

	PdiStack.initialize();

	#if defined(ENABLE_WIFI_SERVICE)
	join_my_wifi();

	// look for the address a second from now, and every second after until the router has given one
	__task_scheduler.setTimeout( print_address_when_joined, MILLISECOND_DURATION_1000, millis() );
	#else
	#error "this example needs the WiFi service, ENABLE_WIFI_SERVICE"
	#endif
}

void loop() {
	PdiStack.serve();
}
