
#include <WhichCommsCommon.h>
#include "M5Unified.h"

LoRa868 lora;
RF433 rf433;

void setup() {
    M5.begin();

    // setup_common_lora868(&lora);
    
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

    await_serial();
}

void loop() {

    uint8_t buffer[256] = {0}; // arbitrary length, but library will handle any buffer overflow internally
    size_t rxd_size = 0;
    Serial.print("SLAVE Awaiting LoRa or RF433 packet. ");
    while (true) {

        // LoRa868 test case 
        // memset(buffer, 0, sizeof(buffer)); 
        // rxd_size = 0; 
        // switch (lora.receiveOne((uint8_t*)buffer, sizeof(buffer), &rxd_size, 500)) { // TODO 500 lora vs 500 rf433
        //     case err::none: // packet rx'd successfully 
        //         Serial.printf("Packet received (%d bytes)\r\n", rxd_size);
        //         break;
        //     case err::timeout_expired:
        //         Serial.println("Nothing received within 5s timeout. Could do other things now, but this example will just loop and try again. "); // nothing rx'd
        //         break;
        // }

        // RF433 test case handling
        memset(buffer, 0, sizeof(buffer)); 
        rxd_size = 0; 
        switch (rf433.receiveOne((uint8_t*)buffer, sizeof(buffer), &rxd_size, 5000)) {
            case err::none: // packet rx'd successfully 
                Serial.printf("RF433 Packet received (%d bytes)\r\n", rxd_size);
                Serial.print("Payload: ");
                for (size_t i = 0; i < rxd_size; ++i) {
                    Serial.printf("%02X ", buffer[i]);
                }
                Serial.println();
                break;
            case err::timeout_expired:
                Serial.println("Nothing RF433 received within timeout. "); 
                break;
            case err::buffer_overflow:
                Serial.println("Buffer overflow occurred while receiving RF433 packet.");
                break;
            case err::waiting:
                // TODO Could do other things now, but this example will just loop and try again.
                break;
            default:
                Serial.println("Unsupported error occurred while receiving RF433 packet.");
                break;
        }
        delay(10);
        M5.update();
    }
}
