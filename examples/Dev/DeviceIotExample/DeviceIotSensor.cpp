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

  #if defined( ENABLE_GPIO_SERVICE )
    // we can read gpio values as sensor data here
    // GPIO service is already handling gpio operation per seconds
    // So not doing anything here
  #else
    this->m_sensor_samples[ this->m_sensor_sample_index++ ] = random(0, 100);
  #endif
}

/**
 * hook to append sensor data to payload
 * this method get called from internal service while sending data to iot server
 * make sure that data should be in json format enclosed within curly brackets i.e. "{... data in json format ....}"
 */
void DeviceIotSensor::dataHook( pdiutil::string &_payload ){

  LogI("Gathering sensor data samples : ");

  #if defined( ENABLE_GPIO_SERVICE )

    LogI("\n");
    pdiutil::vector<pdiutil::string> concatenated_v = __device_iot_service.m_server_configurable_interface_read;
    concatenated_v.insert(concatenated_v.end(), __device_iot_service.m_server_configurable_interface_write.begin(), __device_iot_service.m_server_configurable_interface_write.end());

    pdiutil::string iface_payload = "";
    __gpio_service.appendGpioJsonPayload( iface_payload, false, &concatenated_v );

    char gpiotembuff[ iface_payload.size() + 1 ];
    memset(gpiotembuff, 0, iface_payload.size() + 1 );

    bool _json_result = __get_from_json( (char*)iface_payload.c_str(), (char*)GPIO_PAYLOAD_DATA_KEY, gpiotembuff, iface_payload.size() + 1 );

    if(_json_result){

      _payload += gpiotembuff;

      #if defined(ENABLE_SERIAL_SERVICE)
      
      iface_payload = "";
      __serial_service.appendSerialJsonPayload( iface_payload, &concatenated_v );
      char serialtembuff[ iface_payload.size() + 1 ];
      memset(serialtembuff, 0, iface_payload.size() + 1 );

      _json_result = __get_from_json( (char*)iface_payload.c_str(), (char*)SERIAL_PAYLOAD_DATA_KEY, serialtembuff, iface_payload.size() + 1 );

      if(_json_result && strlen(serialtembuff) > 2){

        _payload.pop_back(); // remove last curly close bracket to continue to add payload
        serialtembuff[0] = ',';   // Replace first curly bracket with comma to continue payload
        _payload += serialtembuff;
      }
      #endif
    }
  #else
    float _total = 0;
    for (int i = 0; i < this->m_sensor_sample_index; i++) {
      _total += this->m_sensor_samples[i];
      LogFmtI("%f ", this->m_sensor_samples[i]);
    }

    this->m_sensor_sample_value = _total/this->m_sensor_sample_index;
    this->m_sensor_sample_index = 0;
    char tembuff[25];  memset(tembuff, 0, 25);  sprintf(tembuff, "%f", this->m_sensor_sample_value);

    LogFmtI("\nAverage : %f\n", this->m_sensor_sample_value);

    _payload += "{\"count\":1,\"value\":[";
    _payload += tembuff;
    _payload += "]}";
  #endif
}
