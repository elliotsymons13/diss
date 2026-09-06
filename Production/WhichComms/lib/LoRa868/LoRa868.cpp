#include "LoRa868.h"

bool LoRa868::pre_config() {
    SPI.begin(LORA_SCLK, LORA_MISO, LORA_MOSI, -1);  // SCK, MISO, MOSI, SS
    LoRa.setSPI(&SPI);
    LoRa.setPins(CS_PIN, RST_PIN, IRQ_PIN);  // set CS, reset, IRQ pin
    return true;
}

bool LoRa868::begin_duplex_mode() { // TODO some design pattern to prevent these being called if Serial.begin not called yet?
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

bool LoRa868::begin_rx_only_mode() {
    Serial.println("LoRa868 bearer functions in duplex mode, and does not support RX-only mode. Invoking begin_duplex_mode instead. ");
    return this->begin_duplex_mode();
}

bool LoRa868::begin_tx_only_mode() {
    Serial.println("LoRa868 bearer functions in duplex mode, and does not support TX-only mode. Invoking begin_duplex_mode instead. ");
    return this->begin_duplex_mode();
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

bool _tryRx(uint8_t* buffer, size_t buffer_size, size_t* rxd_size) {
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
        if (next_byte_pos < buffer_size) {
            buffer[next_byte_pos++] = c;
        } else {
            Serial.println("Buffer too small for packet. Packet truncated.");
            *rxd_size = next_byte_pos;
            return false; // buffer too small for packet 
            // TODO return codes
        }
        Serial.print(c);
    }

    // print RSSI of packet
    Serial.print("' with RSSI ");
    Serial.println(LoRa.packetRssi());

    // TODO populate buffer & size
    *rxd_size = next_byte_pos;
    return true; // packet received
}

// TODO try porting impl to OneComms.cpp, as common
err LoRa868::receiveOne(uint8_t* buffer, size_t buffer_size, size_t* rxd_size) {
    return receiveOne(buffer, buffer_size, rxd_size, 0);
}

err LoRa868::receiveOne(uint8_t* buffer, size_t buffer_size, size_t* rxd_size, uint32_t timeout_ms) {

    // blocking case (no timeout)
    if (timeout_ms == 0) {
        while (!_tryRx(buffer, buffer_size, rxd_size)) {
            // wait for packet
            delay(RX_WAIT_INTERVAL_MS);
        }
        return err::none;
    }
    
    // timout case 
    unsigned long startMillis = millis();
    while (millis() - startMillis < timeout_ms) {
        if (_tryRx(buffer, buffer_size, rxd_size)) {
            return err::none;
        }
        delay(RX_WAIT_INTERVAL_MS);
    }
    return err::timeout_expired;
}


M5_SX127X LoRa868::unwrap() {
    return LoRa;
}