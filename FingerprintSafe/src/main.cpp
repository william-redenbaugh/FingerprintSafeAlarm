#include <Arduino.h>
#include "OS/OSThreadKernel.h"
#include "HAL/MPU6050/mpu6050_imu.h"
#include "fingerprint_module.h"


void setup() {
  // Make sure the first thing we do is unlock the machine
  pinMode(PB1, OUTPUT); 
  digitalWrite(PB1, LOW);

  // put your setup code here, to run once:
  os_init();  
  
  start_fingerprint_module();
  _os_yield();  
}

void loop() {}