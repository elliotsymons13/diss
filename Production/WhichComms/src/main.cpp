#include <Arduino.h>
// #include <M5CoreS3.h>
// #include "M5Unified.h"

// #include <OneComms.h> //included incirectly by LoRa868.h etc. but is needed here so should specify explitly 
#include <LoRa868.h>
LoRa868 lora;

void await_serial() {
    Serial.begin(115200);
    while (!Serial) {
        // wait for Serial to become active
        delay(100);
    }   
    delay(500);
    Serial.println("Serial connected. ");

}

void setup() {
    await_serial();

    lora.pre_config();
    lora.begin();
    lora.post_config();

    M5_SX127X lora_impl = lora.unwrap();
    // lora_impl.setFrequency(433E6); 

}
int counter = 0;
void loop() {
    // put your main code here, to run repeatedly:

    Serial.print("Sending packet: ");
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
}
