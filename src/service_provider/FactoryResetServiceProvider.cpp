/****************************** Reset Factory ********************************
This file is part of the Ewings Esp Stack.

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
  for (uint16_t i = 0; i < this->m_factory_reset_cbs.size(); i++)
  {
    if (nullptr != this->m_factory_reset_cbs[i]._cb)
    {
      this->m_factory_reset_cbs[i]._cb();
    }
  }
  __i_dvc_ctrl.wait(200);
  __i_dvc_ctrl.eraseConfig();
  __i_dvc_ctrl.resetDevice();
}

/**
 * this function register the callback tasks which needs to be done before factory reset.
 *
 * @param		CallBackVoidArgFn	_fn
 * @return	bool
 */
bool DeviceFactoryReset::run_while_factory_reset(CallBackVoidArgFn _fn)
{
  if (this->m_factory_reset_cbs.size() < MAX_FACTORY_RESET_CALLBACKS)
  {
    factory_reset_cb_ _new_factory_reset_cb;
    _new_factory_reset_cb._cb = _fn;
    this->m_factory_reset_cbs.push_back(_new_factory_reset_cb);
    return true;
  }
  return false;
}

/**
 * reset device to default configs on device factory requested
 */
void DeviceFactoryReset::check_device_factory_request()
{
  if( __i_dvc_ctrl.isDeviceFactoryRequested() )
  {
#ifdef EW_SERIAL_LOG
      Logln(F("requested factory reset !"));
#endif
      this->factory_reset();
  }
}

DeviceFactoryReset __factory_reset;
