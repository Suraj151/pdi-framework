/****************************** Reset Factory ********************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st June 2019
******************************************************************************/
#include "FactoryResetServiceProvider.h"

/**
 * DeviceFactoryReset constructor
 */
DeviceFactoryReset::DeviceFactoryReset() : m_flash_key_pressed(0)
{
}

/**
 * DeviceFactoryReset destructor
 */
DeviceFactoryReset::~DeviceFactoryReset()
{
}

/**
 * initialize the service
 */
bool DeviceFactoryReset::initService()
{
  __task_scheduler.setInterval([&](){ 
    this->check_device_factory_request(); 
  }, MILLISECOND_DURATION_1000, __i_dvc_ctrl.millis_now());
  return true;
}

/**
 * this function perform the factory reset operation.
 * it also performs the essential functionality registered as to be done
 * before device reset
 */
void DeviceFactoryReset::factory_reset()
{
  __utl_event.execute_event(EVENT_FACTORY_RESET);
  __i_dvc_ctrl.wait(200);
  __i_dvc_ctrl.eraseConfig();
  __i_dvc_ctrl.resetDevice();
}

/**
 * reset device to default configs on device factory requested
 */
void DeviceFactoryReset::check_device_factory_request()
{
  if( __i_dvc_ctrl.isDeviceFactoryRequested() )
  {
      LogI("requested factory reset !\n");
      this->factory_reset();
  }
}

DeviceFactoryReset __factory_reset;
