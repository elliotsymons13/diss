#ifndef WHICHCOMMSCOMMON_H
#define WHICHCOMMSCOMMON_H

#include <esp_system.h>
#include <Arduino.h>
// #include <M5CoreS3.h>
// #include "M5Unified.h"

// #include <OneComms.h> //included incirectly by LoRa868.h etc. but is needed here so should specify explitly 
#include <LoRa868.h>

#define DEBUG_MODE 1 // comment out to remove wait for serial monitor intput on boot

void await_serial();
void setup_common(LoRa868* lora);

#endif // WHICHCOMMSCOMMON_H