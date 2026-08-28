

#include <WhichCommsCommon.h>

LoRa868 lora;

void setup() {
    setup_common(&lora);
}

int counter = 0;
void loop() {
    // put your main code here, to run repeatedly:

    Serial.print("MASTER Sending packet: ");
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
