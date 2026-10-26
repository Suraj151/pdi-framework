/**************************** pdi stack *********************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st June 2019
******************************************************************************/
#include "PdiStack.h"
#include <utility/EventUtil.h>
#include <service_provider/session/SessionManager.h>
#ifdef ENABLE_SCRIPT_RUNNER
#include <service_provider/cmd/ScriptRunner.h>
#endif

#if defined(ENABLE_STORAGE_SERVICE) && defined(ENABLE_PROCFS)
#include <interface/pdi/impl/modules/storage/ProcFs.h>
#endif

#if defined(ENABLE_STORAGE_SERVICE) && defined(ENABLE_SYSFS)
#include <interface/pdi/impl/modules/storage/SysFs.h>
#endif

#if defined(ENABLE_STORAGE_SERVICE) && defined(ENABLE_DEVFS)
#include <interface/pdi/impl/modules/storage/DevFs.h>
#endif

#if defined(ENABLE_STORAGE_SERVICE) && defined(ENABLE_TMPFS)
#include <interface/pdi/impl/modules/storage/TmpFs.h>
#endif

#ifdef ENABLE_NETWORK_SERVICE
#if defined(ENABLE_NETWORK_SERVICE) && defined(ENABLE_WIFI_SERVICE)
#include <interface/pdi/impl/modules/netif/WiFiNetif.h>
#endif
#include <service_provider/network/NameResolver.h>
#endif

/**
 * @brief Constructor for the PDIStack class.
 *
 * Initializes the PDIStack object and sets up utility interfaces and task scheduler limits.
 * If WiFi service is enabled, it initializes the client and server interfaces.
 */
PDIStack::PDIStack()
{
}

/**
 * @brief Destructor for the PDIStack class.
 *
 * Cleans up resources by setting client and server pointers to nullptr if WiFi service is enabled.
 */
PDIStack::~PDIStack(){
#ifdef ENABLE_NETWORK_SERVICE
  __i_instance.releaseSharedTcpClientInstance();
#endif
#ifdef ENABLE_TLS_SERVICE
  __i_instance.releaseSharedTlsClientInstance();
#endif
}

/**
 * @brief Initializes all required features, services, and actions.
 *
 * This method initializes various services such as database, WiFi, HTTP server, OTA, GPIO, MQTT,
 * email, device IoT, authentication, and command-line services based on the enabled preprocessor directives.
 */
void PDIStack::initialize(){

  pdiutil::enable_heap_check();

  __utl_event.begin(&__i_dvc_ctrl);
  __task_scheduler.setUtilityInterface(&__i_dvc_ctrl);
  __task_scheduler.setMaxTasksLimit(MAX_SCHEDULABLE_TASKS);
  __task_scheduler.setSystemHooks(SessionManager::enterSystem, SessionManager::leaveSystem);

  __log_manager.init(__i_dvc_ctrl.getTerminal());
  // LogI("\n________________________\n\nInitializing PDI Stack\nRelease : %s\nConfig  : %s\n________________________\n", RELEASE, CONFIG_VERSION);

  #ifdef ENABLE_STORAGE_SERVICE
  __i_fs.mount(FILE_SEPARATOR, &__i_rootfs, "rootfs", VFS_TYPE_LITTLEFS);
  #ifdef ENABLE_PROCFS
  __i_fs.mount(PROC_MOUNT_PREFIX, &__i_procfs, "procfs", VFS_TYPE_PROCFS);
  #endif
  #ifdef ENABLE_SYSFS
  __i_fs.mount(SYS_MOUNT_PREFIX, &__i_sysfs, "sysfs", VFS_TYPE_SYSFS);
  #endif
  #ifdef ENABLE_DEVFS
  __i_fs.mount(DEV_MOUNT_PREFIX, &__i_devfs, "devfs", VFS_TYPE_DEVFS);
  #endif
  #ifdef ENABLE_TMPFS
  __i_fs.mount(TMP_MOUNT_PREFIX, &__i_tmpfs, "tmpfs", VFS_TYPE_TMPFS);
  #endif
  __i_fs.init();
  #endif

  __i_dvc_ctrl.initDeviceSpecificFeatures();

  // Get terminal
  iTerminalInterface *terminal = __i_dvc_ctrl.getTerminal();
  if (nullptr != terminal) {
    terminal->writeln();
    terminal->writeln();
		terminal->with_timestamp()->writeln_ro(RODT_ATTR(" Starting PDIStack !"));
		terminal->with_timestamp()->write_ro(RODT_ATTR(" Release: "));
    terminal->write(RELEASE);
		terminal->write_ro(RODT_ATTR(", config "));
    terminal->writeln(CONFIG_VERSION);

    device_info_t info;
    __i_dvc_ctrl.getDeviceInfo(info);

    if (!info.m_model.empty()) {
      terminal->with_timestamp()->write_ro(RODT_ATTR(" Machine: "));
      terminal->write(info.m_model.c_str());
      if (info.m_cores > 0) {
        terminal->write_ro(RODT_ATTR(", "));
        terminal->write((uint32_t)info.m_cores);
        terminal->write_ro(RODT_ATTR(" core(s)"));
      }
      if (info.m_cpu_freq_mhz > 0) {
        terminal->write_ro(RODT_ATTR(" @ "));
        terminal->write(info.m_cpu_freq_mhz);
        terminal->write_ro(RODT_ATTR(" MHz"));
      }
      terminal->writeln();
    }

    if (!info.m_platform_version.empty()) {
      terminal->with_timestamp()->write_ro(RODT_ATTR(" Platform: "));
      terminal->writeln(info.m_platform_version.c_str());
    }

    if (info.m_flash_size > 0) {
      terminal->with_timestamp()->write_ro(RODT_ATTR(" Flash: "));
      terminal->write(info.m_flash_size);
      terminal->writeln_ro(RODT_ATTR(" bytes"));
    }

    terminal->with_timestamp()->write_ro(RODT_ATTR(" Memory: "));
    terminal->write(__i_dvc_ctrl.get_free_heap());
    terminal->write_ro(RODT_ATTR(" free, largest block "));
    terminal->writeln(__i_dvc_ctrl.get_max_free_block());

    terminal->with_timestamp()->write_ro(RODT_ATTR(" Device: id "));
    terminal->write(__i_dvc_ctrl.getDeviceId());
    pdiutil::string mac = __i_dvc_ctrl.getDeviceMac();
    if (!mac.empty()) {
      terminal->write_ro(RODT_ATTR(", mac "));
      terminal->write(mac.c_str());
    }
    terminal->writeln();

    if (!info.m_restart_reason.empty()) {
      terminal->with_timestamp()->write_ro(RODT_ATTR(" Restart reason: "));
      terminal->writeln(info.m_restart_reason.c_str());
    }
  }

  // Set the terminal interface for the service providers
  ServiceProvider::setTerminal(terminal);
  #ifdef ENABLE_CMD_SERVICE
  SessionManager::attach(terminal);
  #endif

  // what each service is meant to do is persisted, so read it before any start
  for (uint8_t i = 0; i < SERVICE_MAX; i++) {
    ServiceProvider *svc = ServiceProvider::getService((service_t)i);
    if (nullptr != svc) svc->loadServiceEnabled();
  }

  __time_service.startService();

  // start the syslog sink first so subsequent services' SysLog lines are persisted
  #ifdef ENABLE_SYSLOG_SERVICE
  __syslog_service.startService();
  #endif

  __database_service.startService();

  #ifdef ENABLE_SERIAL_SERVICE
  __serial_service.startService();
  #endif

  #ifdef ENABLE_WIFI_SERVICE
  __wifi_service.startService();
  #endif

  #ifdef ENABLE_OTA_SERVICE
  __ota_service.startService();
  #endif

  #ifdef ENABLE_GPIO_SERVICE
  __gpio_service.startService();
  #endif

  #ifdef ENABLE_MQTT_SERVICE
  __mqtt_service.startService();
  #endif

  #ifdef ENABLE_EMAIL_SERVICE
  __email_service.startService();
  #endif

  __factory_reset.startService();

  #ifdef ENABLE_DEVICE_IOT
  __device_iot_service.startService();
  #endif

  #ifdef ENABLE_AUTH_SERVICE
  __auth_service.startService();
  #endif

  #if defined(ENABLE_AUTH_SERVICE) && defined(ENABLE_STORAGE_SERVICE)
  __user_store_service.startService();
  #endif

  #ifdef ENABLE_NETWORK_SERVICE
  NameResolver::ensureHostsFile();
  #endif

  #ifdef ENABLE_MDNS_SERVICE
  __mdns_service.startService();
  #endif

  #ifdef ENABLE_HTTP_SERVER
  __web_server.startService();
  #endif

  #ifdef ENABLE_TELNET_SERVICE
  __telnet_service.startService();
  #endif

  #ifdef ENABLE_SSH_SERVICE
  __sshserver_service.startService();
  #endif

  #ifdef ENABLE_CMD_SERVICE
  __cmd_service.startService();
  #ifdef ENABLE_SCRIPT_RUNNER
  {
    pdiutil::string rclocal = CHARPTR_WRAP(RC_LOCAL_FILE_PATH);
    ScriptRunner::runScheduledScript(rclocal.c_str());
  }
  #endif
  #ifdef ENABLE_CRON_SERVICE
  __cron_service.startService();
  #endif
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
  // Handle web server clients
  __web_server.handle_clients();
  #endif

  // Run inline taskscheduler in loop
  __task_scheduler.run();

  // Yield
  __i_dvc_ctrl.yield();

  // Handle device pending events
  __i_dvc_ctrl.handleEvents();

  #ifdef ENABLE_CONTEXTUAL_EXECUTION
  __i_cooperative_scheduler.tick_from_loop();
  #endif
}

/**
 * @brief Global instance of the PDIStack class.
 *
 * This instance is used to manage and serve the PDI stack throughout the application.
 */
PDIStack PdiStack;
