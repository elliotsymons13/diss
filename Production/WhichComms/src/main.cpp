#include <Arduino.h>
// #include <M5CoreS3.h>
// #include "M5Unified.h"

// #include <OneComms.h>
#include <LoRa868.h>


// put function declarations here:
int myFunction(int, int);

void setup() {
  // put your setup code here, to run once:
  int result = myFunction(2, 3);

  LoRa868 lora;
  lora.begin();
}

void loop() {
  // put your main code here, to run repeatedly:
}

// put function definitions here:
int myFunction(int x, int y) {
  return x + y;
}