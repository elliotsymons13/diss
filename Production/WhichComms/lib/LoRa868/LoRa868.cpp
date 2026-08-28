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
    while (!LoRa.begin(LORA_FREQ) & attempts++ <= MAX_LORA_INIT_ATTEMPTS) {
        Serial.println("LoRa init fail.");
        delay(LORA_INIT_ATTEMPT_DELAY_MS);
    }
    if (attempts >= MAX_LORA_INIT_ATTEMPTS) {
        Serial.printf("LoRa init still failing after maximum attempts (%d).", MAX_LORA_INIT_ATTEMPTS);
        return false;
    }
    Serial.printf("LoRa init success on attempt %d. \n", attempts);
    return true;
}

bool LoRa868::post_config() {
    LoRa.setTxPower(LORA_TX_POWER);
    LoRa.setSignalBandwidth(LORA_BW);
    LoRa.setSpreadingFactor(LORA_SF);
    return true;
}

bool LoRa868::send(const uint8_t* data, size_t size) {
    LoRa.beginPacket();
    LoRa.write(data, size);
    LoRa.endPacket();
    return true;
}

bool _tryRx(uint8_t* buffer, size_t size) {
    // try to parse packet
    if (!LoRa.parsePacket()) {
        return false;
    }
    // received a packet
    Serial.print("Received packet '");

    // read packet
    unsigned int next_byte_pos = 0;
    while (LoRa.available()) {
        char c = (char)LoRa.read();
        if (next_byte_pos < size) {
            buffer[next_byte_pos++] = c;
        } else {
            return false; // buffer too small for packet 
            Serial.println("Buffer too small for packet. Packet truncated.");
            // TODO return codes
        }
        Serial.print(c);
    }

    // print RSSI of packet
    Serial.print("' with RSSI ");
    Serial.println(LoRa.packetRssi());
    return true; // packet received
}

// TODO try porting impl to OneComms.cpp, as common
bool LoRa868::receiveOne(uint8_t* buffer, size_t size) {
    return receiveOne(buffer, size, 0);
}

bool LoRa868::receiveOne(uint8_t* buffer, size_t size, uint32_t timeout_ms) {
    // TODO error codes -1 for buffer too small, -2 for timeout

    // START HERE
    // TODO actual impl, including 0 triggered blocking case, and timeout case

    // blocking case (no timeout)
    if (timeout_ms == 0) {
        // TODO implement timeout
        while (!_tryRx(buffer, size)) {
            // wait for packet
            delay(RX_WAIT_INTERVAL_MS);
        }
        return true;
    }
    
    // timout case 
    unsigned long startMillis = millis();
    while (millis() - startMillis < timeout_ms) {
        if (_tryRx(buffer, size)) {
            return true;
        }
        delay(RX_WAIT_INTERVAL_MS);
    }
    Serial.println("Timeout waiting for packet.");
    return false;
}


M5_SX127X LoRa868::unwrap() {
    return LoRa;
}