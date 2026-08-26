// adapted from example code: https://github.com/m5stack/M5-SX127x
#ifndef LORA868_H
#define LORA868_H

#include <Arduino.h>
#include <SPI.h>
#include <OneComms.h>
#include "M5_SX127X.h"

// Module Connect Pins Config
// for CoreS3
#define CS_PIN  6
#define IRQ_PIN 10
#define RST_PIN 5

// for CoreS3
#define LORA_MISO 35
#define LORA_MOSI 37
#define LORA_SCLK 36

// LoRa Parameters Config
// #define LORA_FREQ     433E6
#define LORA_FREQ     868E6
#define LORA_SF       12
#define LORA_BW       125E3
#define LORA_TX_POWER 17

#define MAX_LORA_INIT_ATTEMPTS 5
#define LORA_INIT_ATTEMPT_DELAY_MS 1000

class LoRa868 : public OneComms {
    // no specifics in header beyond OneComms
    public:
        virtual bool pre_config() override;
        virtual bool begin() override;
        virtual bool post_config() override;
        M5_SX127X unwrap();
};

#endif // LORA868_H