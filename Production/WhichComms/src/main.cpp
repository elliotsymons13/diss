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
    // lora_impl.setFrequency(433E6); // TODO change frequency to 433 MHz for testing with other devices

}
int counter = 0;
void loop() {
    // put your main code here, to run repeatedly:

    Serial.print("Sending packet: ");
    Serial.println(counter);

    // send packet
    LoRa.beginPacket();
    uint8_t message[] = "hello ";
    LoRa.write(message, sizeof(message) - 1); // account for null terminator in size
    char counter_str[4]; // max uint8_t is 255, so 3 bytes needed for string rep, plus one for null terminator
    size_t chars_used = sprintf(counter_str, "%d", counter);
    LoRa.write((const uint8_t*)&counter_str, chars_used);
    LoRa.endPacket();

    counter++;

    delay(5000);
}
