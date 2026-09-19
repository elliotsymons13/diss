#include <WhichCommsCommon.h>


void await_serial() {
    Serial.begin(115200); // TODO when I introduce M5.begin(), may need to deconflict ref https://forum.arduino.cc/t/m5stack-serial-begin-9600-breaker-serial/977695/7
    #ifdef DEBUG_MODE
    while (!Serial.available()) {
        // wait for Serial to become active, and any input to be made e.g. random key press in debug monitor
        delay(100);
    }   
    #endif // DEBUG_MODE
    delay(500);
    Serial.println("Serial connected. ");

    // Serial.printf("Last reset reason: %d\n", esp_reset_reason());
}

void block_forever() {
    Serial.println("BLOCKING forever due to unrecoverable error. ");
    while (true) {
        delay(1000);
        Serial.print(".");\
    }
}

bool setup_common_lora868(LoRa868* lora) {

    if (!lora->pre_config()) {
        Serial.println("Failed to pre-configure LoRa868. ");
        return false;
    }
    if (!lora->begin_duplex_mode()) {
        Serial.println("Failed to initialize LoRa868 in duplex mode. ");
        return false;
    }
    if (!lora->post_config()) {
        Serial.println("Failed to post-configure LoRa868. ");
        return false;
    }
    
    M5_SX127X lora_impl = lora->unwrap(); // TODO rm test
    // lora_impl.setFrequency(433E6); 
    return true;
}
