/*************************** Device IOT Sensor ********************************
This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st June 2019
******************************************************************************/

#ifndef _DEVICE_IOT_SENSOR_H_
#define _DEVICE_IOT_SENSOR_H_

#include <PdiStack.h>

#ifndef ENABLE_DEVICE_IOT
  #error "device iot service is disabled ( in devices/DeviceConfig.h of ewings esp framework library ). please enable(uncomment ENABLE_DEVICE_IOT) it for this example"
#endif

/**
 * DeviceIotSensor class
 */
class DeviceIotSensor : public iDeviceIotInterface {

  public:

    /**
     * DeviceIotSensor constructor.
     */
    DeviceIotSensor();
    /**
		 * DeviceIotSensor destructor
		 */
    ~DeviceIotSensor();

    void init( void );
    void sampleHook( void );
    void dataHook( pdiutil::string &_payload );
    void resetSampleHook( void );

  protected:
    /**
		 * @var	sensor variables
		 */
    double    m_sensor_samples[SENSOR_DATA_SAMPLES_PER_PUBLISH_MAX_BUFF];
 		uint16_t  m_sensor_sample_index;
 		double    m_sensor_sample_value;
};

#endif
