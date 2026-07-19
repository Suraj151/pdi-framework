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
  SERVICE_USER_STORE,
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
 * Upper bound on tasks a single service can own. Services with more periodic
 * or one-shot tasks than this will lose lifecycle visibility for the overflow
 * (they still run, but srvc stop/start/status won't see them). Raise if a
 * service needs more — cost is 2 bytes/slot per service instance.
 */
#define MAX_SERVICE_TASKS 6

/**
 * ServiceProvider class
 */
class ServiceProvider{

  public:

    /**
     * ServiceProvider constructor.
     */
    ServiceProvider(service_t st, const char *_svc_name) : m_service_name(_svc_name), m_service_t(st), m_service_routine_task_id(-1), m_service_task_count(0) {
      m_services[st] = this;
      for (uint8_t i = 0; i < MAX_SERVICE_TASKS; i++) {
        m_service_task_ids[i] = -1;
      }
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
     * stop service — cleans every tracked scheduler task the service owns.
     * Services that need to release additional resources (connections, buffers,
     * hardware) should override, call the base impl, then do their own teardown.
     */
    virtual bool stopService(){
      for (uint8_t i = 0; i < m_service_task_count; i++) {
        if (m_service_task_ids[i] >= 0) {
          __task_scheduler.clearInterval(m_service_task_ids[i]);
          m_service_task_ids[i] = -1;
        }
      }
      m_service_task_count = 0;
      m_service_routine_task_id = -1;
      if(nullptr != m_terminal){
        m_terminal->with_timestamp()->write_ro(RODT_ATTR(" Stopping "));
        m_terminal->write_ro(m_service_name);
        m_terminal->writeln_ro(RODT_ATTR(" Service"));
      }
      return true;
    }

    /**
     * Thin wrappers around __task_scheduler.setInterval / setTimeout /
     * updateInterval that auto-supply the task name (m_service_name), auto-tag
     * owner=0 (kernel), and add the returned id to this service's tracked list.
     *
     * Signatures mirror TaskScheduler 1:1 with the trailing `name`/`owner` args
     * dropped — every other argument (including now_millis, priority,
     * last_millis, max_attempts) stays explicit so callers keep the same
     * scheduling control they had with the raw API.
     */
    pdiutil::task_id_t serviceSetInterval(CallBackVoidArgFn _fn, pdiutil::millis_t _duration, pdiutil::millis_t _now_millis, pdiutil::task_priority_t _priority = DEFAULT_TASK_PRIORITY){
      pdiutil::task_id_t id = __task_scheduler.setInterval(_fn, _duration, _now_millis, _priority, m_service_name, 0);
      if (id > 0) trackServiceTask(id);
      return id;
    }

    pdiutil::task_id_t serviceSetTimeout(CallBackVoidArgFn _fn, pdiutil::millis_t _duration, pdiutil::millis_t _now_millis, pdiutil::task_priority_t _priority = DEFAULT_TASK_PRIORITY){
      pdiutil::task_id_t id = __task_scheduler.setTimeout(_fn, _duration, _now_millis, _priority, m_service_name, 0);
      if (id > 0) trackServiceTask(id);
      return id;
    }

    pdiutil::task_id_t serviceUpdateInterval(pdiutil::task_id_t _existing_id, CallBackVoidArgFn _fn, pdiutil::millis_t _duration, pdiutil::task_priority_t _priority = DEFAULT_TASK_PRIORITY, pdiutil::millis_t _last_millis = 0, pdiutil::attempts_t _max_attempts = -1){
      pdiutil::task_id_t id = __task_scheduler.updateInterval(_existing_id, _fn, _duration, _priority, _last_millis, _max_attempts, m_service_name, 0);
      if (id > 0 && !isServiceTaskTracked(id)) trackServiceTask(id);
      return id;
    }

    /**
     * Add an existing task id to this service's tracked list. Returns false
     * if already tracked or if the array is full.
     */
    bool trackServiceTask(pdiutil::task_id_t _id){
      if (_id < 0) return false;
      if (m_service_task_count >= MAX_SERVICE_TASKS) return false;
      for (uint8_t i = 0; i < m_service_task_count; i++) {
        if (m_service_task_ids[i] == _id) return false; // already tracked
      }
      m_service_task_ids[m_service_task_count++] = _id;
      // Backward-compat: keep the historical "primary" slot pointing at the
      // first task for services that inspect m_service_routine_task_id directly.
      if (m_service_routine_task_id < 0) m_service_routine_task_id = _id;
      return true;
    }

    /**
     * Remove a task id from this service's tracked list. Does NOT stop the
     * task itself — pair with clearInterval if you also want it cancelled.
     */
    bool untrackServiceTask(pdiutil::task_id_t _id){
      for (uint8_t i = 0; i < m_service_task_count; i++) {
        if (m_service_task_ids[i] == _id) {
          for (uint8_t j = i + 1; j < m_service_task_count; j++) {
            m_service_task_ids[j - 1] = m_service_task_ids[j];
          }
          m_service_task_count--;
          m_service_task_ids[m_service_task_count] = -1;
          if (m_service_routine_task_id == _id) {
            m_service_routine_task_id = (m_service_task_count > 0) ? m_service_task_ids[0] : -1;
          }
          return true;
        }
      }
      return false;
    }

    bool isServiceTaskTracked(pdiutil::task_id_t _id){
      for (uint8_t i = 0; i < m_service_task_count; i++) {
        if (m_service_task_ids[i] == _id) return true;
      }
      return false;
    }

    /**
     * Deliver a signal to every tracked task belonging to this service.
     * @return Number of tasks the signal was queued on.
     */
    uint16_t signalAllServiceTasks(signal_t _sig){
      uint16_t hits = 0;
      for (uint8_t i = 0; i < m_service_task_count; i++) {
        if (m_service_task_ids[i] >= 0 && __task_scheduler.sendSignal(m_service_task_ids[i], _sig)) {
          hits++;
        }
      }
      return hits;
    }

    /**
     * Count tracked tasks bucketed by lifecycle state. Used by `srvc list/status`
     * to render a service's aggregate state.
     */
    void countServiceTasks(uint16_t &_running, uint16_t &_stopped, uint16_t &_zombie){
      _running = 0; _stopped = 0; _zombie = 0;
      for (uint8_t i = 0; i < m_service_task_count; i++) {
        task_t *t = __task_scheduler.get_task(m_service_task_ids[i]);
        if (nullptr == t) { _zombie++; continue; } // reaped out from under us
        switch (t->m_state) {
          case TASK_STATE_STOPPED: _stopped++; break;
          case TASK_STATE_ZOMBIE:  _zombie++;  break;
          default:                 _running++; break;
        }
      }
    }

    uint8_t getServiceTaskCount() const { return m_service_task_count; }
    pdiutil::task_id_t getServiceTaskId(uint8_t idx) const {
      return (idx < m_service_task_count) ? m_service_task_ids[idx] : -1;
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
     * @var pdiutil::task_id_t m_service_task_id
     * @brief Primary routine task ID (mirrors m_service_task_ids[0] once
     *        anything is tracked). Kept for backward compat with services that
     *        read this field directly.
     */
    pdiutil::task_id_t m_service_routine_task_id;

    /**
     * @var pdiutil::task_id_t m_service_task_ids[MAX_SERVICE_TASKS]
     * @brief Fixed-size list of scheduler task ids this service owns. Populated
     *        by trackServiceTask (called automatically by the serviceSet* wrappers).
     *        Slots hold -1 when empty. Overflow beyond MAX_SERVICE_TASKS is
     *        silently dropped — raise the constant if a service needs more.
     */
    pdiutil::task_id_t m_service_task_ids[MAX_SERVICE_TASKS];

    /**
     * @var uint8_t m_service_task_count
     * @brief Number of populated slots in m_service_task_ids.
     */
    uint8_t m_service_task_count;
};

#endif
