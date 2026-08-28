
#include <WhichCommsCommon.h>

LoRa868 lora;

void setup() {
    setup_common(&lora);
}

int counter = 0;
void loop() {
    // put your main code here, to run repeatedly:
    uint8_t buffer[256]; // HACK len arb
    Serial.print("SLAVE Awaiting packet: ");
    while (true) {
        if (!lora.receiveOne((uint8_t*)buffer, sizeof(buffer), 5000)) {
            Serial.println("Nothing received within 5s timeout. Could do other things now, but this example will just loop and try again. "); // nothing rx'd
        }
        delay(1000);
    }
}
