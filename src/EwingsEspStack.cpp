/**************************** Ewings Esp Stack *********************************
This file is part of the Ewings Esp Stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st June 2019
******************************************************************************/
#include "EwingsEspStack.h"

/**
 * EwingsEspStack constructor.
 */
EwingsEspStack::EwingsEspStack():
  m_wifi(&__i_wifi),
  m_wifi_client(&__i_wifi_client)
{
  __task_scheduler.setUtilityInterface(&__i_dvc_ctrl);
  __task_scheduler.setMaxTasksLimit(MAX_SCHEDULABLE_TASKS);
}

/**
 * EwingsEspStack destructor.
 */
EwingsEspStack::~EwingsEspStack(){
  this->m_wifi = nullptr;
  this->m_wifi_client = nullptr;
}

/**
 * initialize all required features/services/actions.
 */
void EwingsEspStack::initialize(){

  LOGBEGIN;
  LogI("Initializing...\n");

  __database_service.init_default_database();
  #ifdef AUTO_FACTORY_RESET_ON_INVALID_CONFIGS
  __task_scheduler.setInterval( [&]() {
    if ( !__i_db.isValidConfigs() ){
      LogE("\n\nFound invalid configs.. starting factory reset..!\n\n");
      __factory_reset.factory_reset();
    }
  }, MILLISECOND_DURATION_5000, __i_dvc_ctrl.millis_now() );

  #endif
  
  __event_service.begin();
  #if defined( ENABLE_NAPT_FEATURE ) || defined( ENABLE_NAPT_FEATURE_LWIP_V2 )
  __event_service.add_event_listener( EVENT_WIFI_STA_CONNECTED, [&](void*sta_connected) {
    __task_scheduler.setTimeout( [&]() { this->enable_napt_service(); }, NAPT_INIT_DURATION_AFTER_WIFI_CONNECT, __i_dvc_ctrl.millis_now() );
  } );
  #endif

  __wifi_service.begin( this->m_wifi );
  __ping_service.init_ping( this->m_wifi );
  #ifdef ENABLE_EWING_HTTP_SERVER
  __web_server.start_server( this->m_wifi );
  #endif
  __ota_service.begin_ota( this->m_wifi_client );
  #ifdef ENABLE_GPIO_SERVICE
  __gpio_service.begin( this->m_wifi_client );
  #endif
  #ifdef ENABLE_MQTT_SERVICE
  __mqtt_service.begin();
  #endif

  #ifdef ENABLE_EMAIL_SERVICE
  __email_service.begin( this->m_wifi_client );
  #endif

  __task_scheduler.setInterval( this->handleLogPrints, MILLISECOND_DURATION_5000, __i_dvc_ctrl.millis_now() );

  __factory_reset.initService();
  __factory_reset.run_while_factory_reset( [&]() { __database_service.clear_default_tables(); this->m_wifi->disconnect(true); } );

  #ifdef ENABLE_ESP_NOW
  __espnow_service.beginEspNow( this->m_wifi );
  __task_scheduler.setInterval( [&]() { __espnow_service.handlePeers(); }, ESP_NOW_HANDLE_DURATION, __i_dvc_ctrl.millis_now() );
  #endif

  #ifdef ENABLE_DEVICE_IOT
  __device_iot_service.init( this->m_wifi_client );
  #endif

  #ifdef ENABLE_EXCEPTION_NOTIFIER
  beginCrashHandler();
  #endif
}

#if defined( ENABLE_NAPT_FEATURE )
/**
 * enable napt feature
 */
void EwingsEspStack::enable_napt_service(){

  // Initialize the NAT feature
  ip_napt_init(IP_NAPT_MAX, IP_PORTMAP_MAX);
  // Enable NAT on the AP interface
  ip_napt_enable_no(1, 1);
  // Set the DNS server for clients of the AP to the one we also use for the STA interface
  dhcps_set_DNS(this->m_wifi->dnsIP());
  LogFmtS("NAPT(lwip %d) initialization done\n", (int)LWIP_VERSION_MAJOR);
}
#elif defined( ENABLE_NAPT_FEATURE_LWIP_V2 )
/**
 * enable napt feature
 */
void EwingsEspStack::enable_napt_service(){

  // Initialize the NAPT feature
  err_t ret = ip_napt_init(IP_NAPT_MAX, IP_PORTMAP_MAX);
  if (ret == ERR_OK) {
    // Enable NAT on the AP interface
    ret = ip_napt_enable_no(SOFTAP_IF, 1);
    if (ret == ERR_OK) {
      LogFmtS("NAPT(lwip %d) initialization done\n", (int)LWIP_VERSION_MAJOR);
      // Set the DNS server for clients of the AP to the one we also use for the STA interface
      getNonOSDhcpServer().setDns(this->m_wifi->dnsIP(0));
      //dhcpSoftAP.dhcps_set_dns(1, this->m_wifi->dnsIP(1));
    }
  }
  if (ret != ERR_OK) {
    LogE("NAPT initialization failed\n");
  }
}
#endif

/**
 * serve each internal action, client request, auto operations
 */
void EwingsEspStack::serve(){

  #ifdef ENABLE_EWING_HTTP_SERVER
  __web_server.handle_clients();
  #endif
  __task_scheduler.handle_tasks();
  __i_dvc_ctrl.yield();
}

/**
 * prints log as per defined duration
 */
void EwingsEspStack::handleLogPrints(){

  __wifi_service.printWiFiConfigLogs();
  __ota_service.printOtaConfigLogs();
  #ifdef ENABLE_GPIO_SERVICE
  __gpio_service.printGpioConfigLogs();
  #endif
  #ifdef ENABLE_MQTT_SERVICE
  __mqtt_service.printMqttConfigLogs();
  #endif
  #ifdef ENABLE_EMAIL_SERVICE
  __email_service.printEmailConfigLogs();
  #endif
  #ifdef ENABLE_DEVICE_IOT
  __device_iot_service.printDeviceIotConfigLogs();
  #endif
  __task_scheduler.printTaskSchedulerLogs();
  LogFmtI("\nNTP Validity : %d\n", __nw_time_service.is_valid_ntptime());
  LogFmtI("NTP Time : %d\n", (int)__nw_time_service.get_ntp_time());
}

EwingsEspStack EwStack;
