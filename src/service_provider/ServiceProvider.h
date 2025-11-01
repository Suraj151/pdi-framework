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
#ifdef ENABLE_HTTP_SERVER
  SERVICE_HTTP_SERVER,
#endif
#ifdef ENABLE_TELNET_SERVICE
  SERVICE_TELNET,
#endif
#ifdef ENABLE_SSH_SERVICE
  SERVICE_SSH,
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
    ServiceProvider(service_t st, const char *_svc_name) : m_service_t(st), m_service_name(_svc_name), m_service_routine_task_id(-1) {
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
    virtual bool initService(void *arg = nullptr){
      if(nullptr != m_terminal){
        m_terminal->with_timestamp()->write_ro(RODT_ATTR(" Starting "));
        m_terminal->write_ro(m_service_name);
        m_terminal->writeln_ro(RODT_ATTR(" Service"));
      }
      return true;
    }

    /**
     * stop service
     */
    virtual bool stopService(){
      __task_scheduler.clearInterval( m_service_routine_task_id );
      m_service_routine_task_id = -1;
      if(nullptr != m_terminal){
        m_terminal->with_timestamp()->write_ro(RODT_ATTR(" Stopping "));
        m_terminal->write_ro(m_service_name);
        m_terminal->writeln_ro(RODT_ATTR(" Service"));
      }
      return true;
    }

    /**
     * Get service instance
     */
    static ServiceProvider* getService(service_t st){
      return m_services[st];
    }

    /**
     * Set terminal interface
     * @param terminal Pointer to the terminal interface.
     */
    static void setTerminal(iTerminalInterface *terminal){
      m_terminal = terminal;
    }

    /**
     * Get terminal interface
     * @return Pointer to the terminal interface.
     */
    static iTerminalInterface* getTerminal(){
      return m_terminal;
    }
    
    /**
     * print service config to terminal
     * @param terminal Pointer to the terminal interface.
     */
    virtual void printConfigToTerminal(iTerminalInterface *terminal){
    }

    /**
     * print service status to terminal
     * @param terminal Pointer to the terminal interface.
     */
    virtual void printStatusToTerminal(iTerminalInterface *terminal){
    }

    /**
     * @array ServiceProvider* m_services
     */
    static ServiceProvider *m_services[SERVICE_MAX]; 

    /**
     * @var const char* m_service_name
     * @brief Name of the service.
     */
    const char *m_service_name;

  protected:

    /**
     * @var service_t m_service_t
     * @brief Type of the service.
     */
    service_t m_service_t;

    /**
     * @var iTerminalInterface* m_terminal
     * @brief Pointer to the terminal interface for output.
     */
    static iTerminalInterface *m_terminal;

    /**
     * @var int32_t m_service_task_id
     * @brief Task ID associated with the service.
     */
    int32_t m_service_routine_task_id;
};

#endif
