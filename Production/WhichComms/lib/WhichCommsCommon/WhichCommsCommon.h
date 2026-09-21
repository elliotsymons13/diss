#ifndef WHICHCOMMSCOMMON_H
#define WHICHCOMMSCOMMON_H

#include <esp_system.h>
#include <Arduino.h>
// #include <M5CoreS3.h>
// #include "M5Unified.h"

#include <EverywhereCommon.h>
// #include <OneComms.h> //included incirectly by LoRa868.h etc. but is needed here so should specify explitly 
#include <LoRa868.h>
#include <RF433.h>

#define DEBUG_MODE // comment out to remove wait for serial monitor intput on boot


void await_serial();
void block_forever();
bool setup_common_lora868(LoRa868* lora868);

#endif // WHICHCOMMSCOMMON_H