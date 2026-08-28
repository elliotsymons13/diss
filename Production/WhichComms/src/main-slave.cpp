
#include <WhichCommsCommon.h>

LoRa868 lora;

void setup() {
    setup_common(&lora);
}

int counter = 0;
void loop() {
    // put your main code here, to run repeatedly:
    uint8_t buffer[256]; // HACK len arb
    Serial.print("SLAVE Awaiting packet (timeout): ");
    while (true) {
        switch (lora.receiveOne((uint8_t*)buffer, sizeof(buffer), 5000)) {
            case err::none:
                Serial.println("Packet received. "); // packet rx'd
                break;
            case err::timeout_expired:
                Serial.println("Nothing received within 5s timeout. Could do other things now, but this example will just loop and try again. "); // nothing rx'd
                break;
        }
        delay(1000);
    }
}
