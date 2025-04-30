/***************************** service provider *******************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st June 2019
******************************************************************************/

#ifndef _SERVICE_PROVIDER_H_
#define _SERVICE_PROVIDER_H_


#include <interface/pdi.h>
#include <config/Config.h>
#include <utility/Utility.h>


/**
* services type
*/
typedef enum services{

#ifdef ENABLE_AUTH_SERVICE
  SERVICE_AUTH,
#endif
#ifdef ENABLE_CMD_SERVICE
  SERVICE_CMD,
#endif
  SERVICE_DATABASE,
#ifdef ENABLE_GPIO_SERVICE
  SERVICE_GPIO,
#endif
  SERVICE_FACTORY,
#ifdef ENABLE_OTA_SERVICE
  SERVICE_OTA,
#endif
#ifdef ENABLE_EMAIL_SERVICE
  SERVICE_EMAIL,
#endif
#ifdef ENABLE_DEVICE_IOT
  SERVICE_DVCIOT,
#endif
#ifdef ENABLE_WIFI_SERVICE
  SERVICE_WIFI,
#endif
#ifdef ENABLE_MQTT_SERVICE
  SERVICE_MQTT,
#endif
#ifdef ENABLE_SERIAL_SERVICE
  SERVICE_SERIAL,
#endif
  SERVICE_MAX
} service_t;


/**
 * ServiceProvider class
 */
class ServiceProvider{

  public:

    /**
     * ServiceProvider constructor.
     */
    ServiceProvider(service_t st) : m_service_t(st) {
      m_services[st] = this;
    }

    /**
		 * ServiceProvider destructor
		 */
    virtual ~ServiceProvider(){
    }

    /**
     * init service
     */
    virtual bool initService(){
      return true;
    }

    /**
     * Get service instance
     */
    static ServiceProvider* getService(service_t st){
      return m_services[st];
    }

    /**
     * print service config to terminal
     */
    virtual void printConfigToTerminal(iTerminalInterface *terminal){
    }

    /**
     * print service status to terminal
     */
    virtual void printStatusToTerminal(iTerminalInterface *terminal){
    }

    /**
     * @array ServiceProvider* m_services
     */
    static ServiceProvider *m_services[SERVICE_MAX]; 

  protected:

    /**
     * print service config to terminal
     */
    service_t m_service_t;
};

#endif
