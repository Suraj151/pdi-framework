/**************************** pdi stack *********************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st June 2019
******************************************************************************/
#include "PdiStack.h"
#include <utility/EventUtil.h>

/**
 * @brief Constructor for the PDIStack class.
 *
 * Initializes the PDIStack object and sets up utility interfaces and task scheduler limits.
 * If WiFi service is enabled, it initializes the client and server interfaces.
 */
PDIStack::PDIStack()
#ifdef ENABLE_WIFI_SERVICE
  :
  m_client(&__i_wifi_client),
  m_server(&__i_http_server)
#endif
{
  __utl_event.begin(&__i_dvc_ctrl);
  __task_scheduler.setUtilityInterface(&__i_dvc_ctrl);
  __task_scheduler.setMaxTasksLimit(MAX_SCHEDULABLE_TASKS);
}

/**
 * @brief Destructor for the PDIStack class.
 *
 * Cleans up resources by setting client and server pointers to nullptr if WiFi service is enabled.
 */
PDIStack::~PDIStack(){
#ifdef ENABLE_WIFI_SERVICE
  this->m_client = nullptr;
  this->m_server = nullptr;
#endif
}

/**
 * @brief Initializes all required features, services, and actions.
 *
 * This method initializes various services such as database, WiFi, HTTP server, OTA, GPIO, MQTT,
 * email, device IoT, authentication, and command-line services based on the enabled preprocessor directives.
 */
void PDIStack::initialize(){

  LOGBEGIN;
  // LogFmtI("\n________________________\n\nInitializing PDI Stack\nRelease : %s\nConfig  : %s\n________________________\n", RELEASE, CONFIG_VERSION);

  __i_dvc_ctrl.initDeviceSpecificFeatures();

  // Get terminal
  iTerminalInterface *terminal = __i_dvc_ctrl.getTerminal();
  if (nullptr != terminal) {
    terminal->writeln();
    terminal->writeln();
		terminal->writeln_ro(RODT_ATTR("Starting PDIStack !"));
		terminal->write_ro(RODT_ATTR("Release : "));
    terminal->writeln(RELEASE);
		terminal->write_ro(RODT_ATTR("Config : "));
    terminal->writeln(CONFIG_VERSION);
    terminal->writeln();
  }

  // Set the terminal interface for the service providers
  ServiceProvider::setTerminal(terminal);

  __database_service.initService();

  #ifdef ENABLE_SERIAL_SERVICE
  __serial_service.initService();
  #endif

  #ifdef ENABLE_WIFI_SERVICE
  __wifi_service.initService( &__i_wifi );
  #endif

  #ifdef ENABLE_OTA_SERVICE
  __ota_service.initService( this->m_client );
  #endif
  
  #ifdef ENABLE_GPIO_SERVICE
  __gpio_service.initService( 
    #ifdef ENABLE_HTTP_CLIENT
    this->m_client 
    #endif
    );
  #endif
  
  #ifdef ENABLE_MQTT_SERVICE
  __mqtt_service.initService( this->m_client );
  #endif

  #ifdef ENABLE_EMAIL_SERVICE
  __email_service.initService( this->m_client );
  #endif

  __factory_reset.initService();

  #ifdef ENABLE_DEVICE_IOT
  __device_iot_service.initService( this->m_client );
  #endif

  #ifdef ENABLE_AUTH_SERVICE
  __auth_service.initService();
  #endif

  #ifdef ENABLE_STORAGE_SERVICE
  __i_fs.init();
  #endif

  #ifdef ENABLE_HTTP_SERVER
  __web_server.initService( this->m_server );
  #endif

  #ifdef ENABLE_TELNET_SERVICE
  uint16_t telnet_port = 23; // Default Telnet port
  __telnet_service.initService(&telnet_port);
  #endif

  #ifdef ENABLE_SSH_SERVICE
  uint16_t ssh_port = 22; // Default SSH port
  __sshserver_service.initService(&ssh_port);
  #endif

  #ifdef ENABLE_CMD_SERVICE
  __cmd_service.initService();
  CommandLineServiceProvider::startInteraction();
  #endif
}

/**
 * @brief Handles internal actions, client requests, and auto operations.
 *
 * This method serves the PDI stack by handling HTTP server clients, executing scheduled tasks,
 * and yielding control to the device controller.
 */
void PDIStack::serve(){

  #ifdef ENABLE_HTTP_SERVER
  __web_server.handle_clients();
  #endif
  __task_scheduler.handle_tasks();
  __i_dvc_ctrl.yield();
  __i_dvc_ctrl.handleEvents();
}

/**
 * @brief Prints logs at defined intervals.
 *
 * If the network service is enabled, this method logs the validity of the NTP time and the current NTP time.
 */
void PDIStack::handleLogPrints(){

  #ifdef ENABLE_NETWORK_SERVICE
  LogFmtI("\nNTP Validity : %d\n", __i_ntp.is_valid_ntptime());
  LogFmtI("NTP Time : %d\n", (int)__i_ntp.get_ntp_time());
  #endif
}

/**
 * @brief Global instance of the PDIStack class.
 *
 * This instance is used to manage and serve the PDI stack throughout the application.
 */
PDIStack PdiStack;
