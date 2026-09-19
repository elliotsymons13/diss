

#include <WhichCommsCommon.h>
#include "M5Unified.h"

LoRa868 lora; 
RF433 rf433;

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

int counter = 0;
void loop() {


    while (true) {
        Serial.print("MASTER sending RTT for both LoRa868 and RF433. ");

        // TODO func: make message sent the exact same for both bearers
        // TODO func: send then wait for response
        // TODO func: add timer
        
        // LoRa868
        char message[7] = "hello ";
        char counter_str[4]; // max uint8_t is 255, so 3 bytes needed for string rep, plus one for null terminator
        size_t chars_used = sprintf(counter_str, "%d", counter);
        uint8_t msg_len = 7 + chars_used;
        char full_message[msg_len];
        snprintf(full_message, sizeof(full_message), "hello %s", counter_str);
        Serial.printf("SENDING LoRa868 #%d  '%s' with length %d\n", counter, full_message, msg_len);
        lora.send((const uint8_t*)full_message, msg_len-1); // -1 to exclude null terminator
        
        // RF433
        const size_t user_data_len = 6;
        const uint8_t data[user_data_len] = {0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x06};
        Serial.printf("SENDING RF433 #%d  \r\n", counter);
        rf433.send(data, user_data_len);
        
        counter++;
        delay(345); // TODO explicitly not 500 like any of my timeouts, to make timing distinct
        M5.update();
    }
}
