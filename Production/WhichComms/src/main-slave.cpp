
#include <WhichCommsCommon.h>
#include "M5Unified.h"

// HACK flag for quick test
// #define USE_LORA

LoRa868 lora;
RF433 rf433;

void setup() {
    M5.begin();
#ifdef USE_LORA
    setup_common_lora868(&lora);
#else
    await_serial();

    rf433.pre_config();
    if (!rf433.begin_tx_only_mode()) {
        Serial.println("Failed to test initialize TX RF433. ");
        block_forever();
    }
    if (!rf433.begin_rx_only_mode()) {
        Serial.println("Failed to test initialize RX RF433. ");
        block_forever();
    }
    rf433.post_config();
#endif // USE_LORA
}

int counter = 0;
void loop() {

#ifdef USE_LORA
    // LoRa868 test case 
    uint8_t buffer[256]; // HACK len arb
    Serial.print("SLAVE Awaiting LoRa packet (timeout): ");
    while (true) {
        size_t rxd_size = 0;
        switch (lora.receiveOne((uint8_t*)buffer, sizeof(buffer), &rxd_size, 5000)) {
            case err::none: // packet rx'd successfully 
                Serial.printf("Packet received (%d bytes)\r\n", rxd_size);
                break;
            case err::timeout_expired:
                Serial.println("Nothing received within 5s timeout. Could do other things now, but this example will just loop and try again. "); // nothing rx'd
                break;
        }
        delay(1000); // HACK arb delay
    }
#else
    // RF433 test case
    uint8_t buffer[256]; // arbitrary length, but library will handle any buffer overflow internally
    Serial.print("SLAVE Awaiting RF433 packet (timeout): ");
    while (true) {
        size_t rxd_size = 0;
        switch (rf433.receiveOne((uint8_t*)buffer, sizeof(buffer), &rxd_size, 5000)) {
            case err::none: // packet rx'd successfully 
                Serial.printf("RF433 Packet received (%d bytes)\r\n", rxd_size);
                break;
            case err::timeout_expired:
                Serial.println("Nothing RF433 received within 5s timeout. "); 
                break;
            case err::buffer_overflow:
                Serial.println("Buffer overflow occurred while receiving RF433 packet.");
                break;
        }
        delay(10);
        M5.update();
        // TODO Could do other things now, but this example will just loop and try again.
    }
#endif // USE_LORA
}
