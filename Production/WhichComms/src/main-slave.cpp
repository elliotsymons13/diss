
#include <WhichCommsCommon.h>

// HACK flag for quick test
// #define USE_LORA

LoRa868 lora;
RF433 rf433;

void setup() {
#ifdef USE_LORA
    setup_common_lora868(&lora);
#else
    if (!setup_common_rf433((RF433*) &rf433)) {
        Serial.println("FATAL ERROR - STOPPING - Failed to setup RF433");
        while (true) {
            delay(1000);
        }
    }
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
    uint8_t buffer[256]; // HACK len arb
    Serial.print("SLAVE Awaiting RF433 packet (timeout): ");
    while (true) {
        size_t rxd_size = 0;
        switch (rf433.receiveOne((uint8_t*)buffer, sizeof(buffer), &rxd_size, 5000)) {
            case err::none: // packet rx'd successfully 
                Serial.printf("Packet received (%d bytes)\r\n", rxd_size);
                break;
            case err::timeout_expired:
                Serial.println("Nothing received within 5s timeout. Could do other things now, but this example will just loop and try again. "); // nothing rx'd
                break;
        }
        delay(1000); // HACK arb delay
    }
#endif // USE_LORA
}
