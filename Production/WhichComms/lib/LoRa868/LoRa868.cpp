#include "LoRa868.h"

bool LoRa868::pre_config() {
    SPI.begin(LORA_SCLK, LORA_MISO, LORA_MOSI, -1);  // SCK, MISO, MOSI, SS
    LoRa.setSPI(&SPI);
    LoRa.setPins(CS_PIN, RST_PIN, IRQ_PIN);  // set CS, reset, IRQ pin
    return true;
}

bool LoRa868::begin() { // TODO some design pattern to prevent these being called if Serial.begin not called yet?
    Serial.println("LoRa init start. ");
    uint8_t attempts = 0;
    while (!LoRa.begin(LORA_FREQ) && ++attempts < MAX_LORA_INIT_ATTEMPTS) {
        Serial.println("LoRa init fail.");
        delay(LORA_INIT_ATTEMPT_DELAY_MS);
    }
    if (attempts >= MAX_LORA_INIT_ATTEMPTS) {
        Serial.printf("LoRa init still failing after maximum attempts (%d).", MAX_LORA_INIT_ATTEMPTS);
        return false;
    }
    Serial.printf("LoRa init success on attempt %d. ", attempts);
    return true;
}

bool LoRa868::post_config() {
    LoRa.setTxPower(LORA_TX_POWER);
    LoRa.setSignalBandwidth(LORA_BW);
    LoRa.setSpreadingFactor(LORA_SF);
    return true;
}

M5_SX127X LoRa868::unwrap() {
    return LoRa;
}