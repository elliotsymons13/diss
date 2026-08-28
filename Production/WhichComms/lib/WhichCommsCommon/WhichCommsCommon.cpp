#include <WhichCommsCommon.h>


void await_serial() {
    Serial.begin(115200); // TODO when I introduce M5.begin(), will need to deconflict ref https://forum.arduino.cc/t/m5stack-serial-begin-9600-breaker-serial/977695/7
    #ifdef DEBUG_MODE
    while (!Serial.available()) {
        // wait for Serial to become active, and any input to be made e.g. random key press in debug monitor
        delay(100);
    }   
    #endif // DEBUG_MODE
    delay(500);
    Serial.println("Serial connected. ");
    Serial.printf("Last reset reason: %d\n", esp_reset_reason());

}

void setup_common(LoRa868* lora) {
    await_serial();

    lora->pre_config();
    lora->begin();
    lora->post_config();

    M5_SX127X lora_impl = lora->unwrap();
    // lora_impl.setFrequency(433E6); 
}