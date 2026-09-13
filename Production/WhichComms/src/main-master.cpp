

#include <WhichCommsCommon.h>
#include "M5Unified.h"

#ifdef USE_LORA // HACK better compile flags
LoRa868 lora;
#else 
RF433 rf433;
#endif

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
    Serial.print("MASTER LoRa868 Sending packet: ");
    Serial.println(counter);

    // send packet
    char message[7] = "hello ";
    char counter_str[4]; // max uint8_t is 255, so 3 bytes needed for string rep, plus one for null terminator
    size_t chars_used = sprintf(counter_str, "%d", counter);

    uint8_t msg_len = 7 + chars_used;
    char full_message[msg_len];
    snprintf(full_message, sizeof(full_message), "hello %s", counter_str);
    Serial.printf("About to send  '%s' with length %d\n", full_message, msg_len);
    lora.send((const uint8_t*)full_message, msg_len-1); // -1 to exclude null terminator

    counter++;

    delay(5000);
#else
    const size_t user_data_len = 6;
    const size_t packet_size = 1 + user_data_len; // 1 byte for length + user data
    const uint8_t data[1+user_data_len] = {packet_size, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00,};
    Serial.print("MASTER RF433 Sending packet: ");
    uint32_t send_count = 0;
    const uint8_t retries = 1;
    while (true) {
        uint32_t old_count = send_count;
        send_count = send_count + retries;
        Serial.printf("SENDING %d packets %d - %d \r\n", retries, old_count+1, send_count);
        rf433.send(data, packet_size);
        delay(345);
    }
#endif // USE_LORA
}
