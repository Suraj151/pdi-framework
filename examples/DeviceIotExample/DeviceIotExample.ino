/* stack initialize the services in setup and
 * should serve in loop continueously
 */

#include "DeviceIotSensor.h"

DeviceIotSensor sensor;

void setup() {
  PdiStack.initialize();
  __device_iot_service.initDeviceIotSensor(&sensor);
}

void loop() {
  PdiStack.serve();
}
