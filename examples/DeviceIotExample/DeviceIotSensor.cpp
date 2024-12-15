/*************************** Device IOT Sensor ********************************
This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st June 2019
******************************************************************************/

#include "DeviceIotSensor.h"

/**
 * DeviceIotSensor constructor.
 */
DeviceIotSensor::DeviceIotSensor():
  m_sensor_sample_index(0),
  m_sensor_sample_value(0)
{
}

/**
 * DeviceIotSensor destructor
 */
DeviceIotSensor::~DeviceIotSensor(){
}

/**
 * initialize sensor
 */
void DeviceIotSensor::init(){

  this->resetSampleHook();
}

/**
 * hook to reset sample parameters
 */
void DeviceIotSensor::resetSampleHook(){

  this->m_sensor_sample_index = 0;
  this->m_sensor_sample_value = 0;
}

/**
 * hook to take samples from sensors.
 * periodically this method get called from internal service to collect samples.
 * here we can write sensor data collect logic
 */
void DeviceIotSensor::sampleHook(){

  this->m_sensor_samples[ this->m_sensor_sample_index++ ] = random(0, 100);
}

/**
 * hook to append sensor data to payload
 * this method get called from internal service while sending data to iot server
 * make sure that data should be in json format enclosed within curly brackets i.e. "{... data in json format ....}"
 */
void DeviceIotSensor::dataHook( pdiutil::string &_payload ){

  LogI("Gathering sensor data samples : ");

  float _total = 0;
  for (int i = 0; i < this->m_sensor_sample_index; i++) {
    _total += this->m_sensor_samples[i];
    LogFmtI("%f ", this->m_sensor_samples[i]);
  }
  this->m_sensor_sample_value = _total/this->m_sensor_sample_index;

  LogFmtI("\nAverage : %f\n", this->m_sensor_sample_value);

  this->m_sensor_sample_index = 0;

  _payload += "{\"sensor_type\":\"";
  _payload += DEVICE_IOT_SENSOR_TYPE;
  _payload += "\",\"count\":1,\"value\":[";
  _payload += this->m_sensor_sample_value;
  _payload += "]}";
}
