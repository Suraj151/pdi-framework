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
 * PDIStack constructor.
 */
PDIStack::PDIStack():
  m_wifi(&__i_wifi),
  m_client(&__i_wifi_client)
{
  __utl_event.begin(&__i_dvc_ctrl);
  __task_scheduler.setUtilityInterface(&__i_dvc_ctrl);
  __task_scheduler.setMaxTasksLimit(MAX_SCHEDULABLE_TASKS);
}

/**
 * PDIStack destructor.
 */
PDIStack::~PDIStack(){
  this->m_wifi = nullptr;
  this->m_client = nullptr;
}

/**
 * initialize all required features/services/actions.
 */
void PDIStack::initialize(){

  LOGBEGIN;
  LogI("Initializing...\n");

  __database_service.init_default_database();
  
  __i_dvc_ctrl.initDeviceSpecificFeatures();

  __wifi_service.begin( this->m_wifi );

  #ifdef ENABLE_HTTP_SERVER
  __web_server.start_server( this->m_wifi );
  #endif
  __ota_service.begin_ota( this->m_client );
  #ifdef ENABLE_GPIO_SERVICE
  __gpio_service.begin( this->m_client );
  #endif
  #ifdef ENABLE_MQTT_SERVICE
  __mqtt_service.begin( this->m_client );
  #endif

  #ifdef ENABLE_EMAIL_SERVICE
  __email_service.begin( this->m_client );
  #endif

  __task_scheduler.setInterval( this->handleLogPrints, MILLISECOND_DURATION_5000, __i_dvc_ctrl.millis_now() );

  __factory_reset.initService();

  #ifdef ENABLE_DEVICE_IOT
  __device_iot_service.init( this->m_client );
  #endif
}

/**
 * serve each internal action, client request, auto operations
 */
void PDIStack::serve(){

  #ifdef ENABLE_HTTP_SERVER
  __web_server.handle_clients();
  #endif
  __task_scheduler.handle_tasks();
  __i_dvc_ctrl.yield();
}

/**
 * prints log as per defined duration
 */
void PDIStack::handleLogPrints(){

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
  LogFmtI("\nNTP Validity : %d\n", __i_ntp.is_valid_ntptime());
  LogFmtI("NTP Time : %d\n", (int)__i_ntp.get_ntp_time());
}

PDIStack PdiStack;
