

#include <WhichCommsCommon.h>
#include "M5Unified.h"

LoRa868 lora; 
RF433 rf433;

#define RX_WAIT_TIME 700 // HACK must be greater than rx wait on slave side
#define RX_BUFF_SIZE 256

void setup() {
    M5.begin();
    await_serial(); // must await serial before setup, so failures can be logged

    if (!setup_common_lora868(&lora)) {
        Serial.println("Critical error. BLOCKING. ");
        block_forever();
    }
    
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

}

void loop() {


    Serial.println("MASTER entering main loop ");
    while (true) {

        // TODO func: make message sent the exact same for both bearers
        // TODO func: send then wait for response
        // TODO func: add timer
        // TODO loop over bearers agnostically 

        // RTT trigger message
        const size_t tx_data_len = 6;
        const uint8_t tx_data[tx_data_len] = {0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x06};
        
        // LoRa868 send 
        Serial.println("SENDING LoRa868");
        lora.send(tx_data, tx_data_len);
        Serial.println("SENT LoRa868");

        // LoRa868 receive response
        Serial.println("WAITING LoRa868 response");
        uint8_t rx_buffer[RX_BUFF_SIZE] = {0};
        size_t rxd_size = 0;
        switch(lora.receiveOne((uint8_t*)rx_buffer, sizeof(rx_buffer), &rxd_size, RX_WAIT_TIME)) {
            case err::none:
                Serial.println("LoRa868 packet received successfully.");
                Serial.printf("LoRa868 received response, length %d\r\n", rxd_size);
                Serial.print("LoRa868 response payload: ");
                log_bytes(rx_buffer, rxd_size);
                if (rxd_size == tx_data_len) {
                    if (equal(rx_buffer, (uint8_t*)tx_data, rxd_size)) {
                        Serial.println("LoRa868 response matches sent data.");
                    } else {
                        Serial.println("LoRa868 response does NOT match sent data.");
                    }
                } else {
                    Serial.printf("LoRa868 response length (%d) does NOT match sent data length (%d). Ignoring. ", rxd_size, tx_data_len);
                }
                break;
            case err::timeout_expired:
                Serial.println("LoRa868 receive timed out.");
                break;
            case err::buffer_overflow:
                Serial.println("LoRa868 buffer overflow occurred.");
                break;
            case err::waiting:
                Serial.println("LoRa868 waiting for packet.");
                break;
            default:
                Serial.println("LoRa868 unknown error occurred.");
                break;
        }
       
        
        // RF433
        // Serial.println("SENDING RF433");
        // rf433.send(tx_data, tx_data_len);
        // Serial.println("SENT RF433");

        // TODO RF433 receive response
        
        delay(345); // TODO explicitly not 500 like any of my timeouts, to make timing distinct
        M5.update();
    }
}
