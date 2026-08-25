// adapted from example code: https://github.com/m5stack/M5-SX127x

#include <Arduino.h>
#include <SPI.h>
#include "M5_SX127X.h"

// Module Connect Pins Config
// for CoreS3
#define CS_PIN  6
#define IRQ_PIN 10
#define RST_PIN 5

// for CoreS3
#define LORA_MISO 35
#define LORA_MOSI 37
#define LORA_SCLK 36

// LoRa Parameters Config
// #define LORA_FREQ     433E6
#define LORA_FREQ     868E6
#define LORA_SF       12
#define LORA_BW       125E3
#define LORA_TX_POWER 17

int counter = 0;

void setup() {
    Serial.begin(115200);
    while (!Serial) {
		// wait for Serial to become active
		delay(100);
	}
	delay(500);
	Serial.println("Serial connected. ");

    Serial.println("LoRa Sender");
    SPI.begin(LORA_SCLK, LORA_MISO, LORA_MOSI, -1);  // SCK, MISO, MOSI, SS
    LoRa.setSPI(&SPI);
    LoRa.setPins(CS_PIN, RST_PIN, IRQ_PIN);  // set CS, reset, IRQ pin
    Serial.println("LoRa init start. ");
    while (!LoRa.begin(LORA_FREQ)) {
        Serial.println("LoRa init fail.");
        delay(1000);
    }
    Serial.println("LoRa init success. ");

    LoRa.setTxPower(LORA_TX_POWER);
    LoRa.setSignalBandwidth(LORA_BW);
    LoRa.setSpreadingFactor(LORA_SF);
}

void loop() {
    Serial.print("Sending packet: ");
    Serial.println(counter);

    // send packet
    LoRa.beginPacket();
    // LoRa.print("hello ");
    // LoRa.print(counter);
    uint8_t message[] = "hello ";
    LoRa.write(message, sizeof(message));
    LoRa.write((uint8_t*)&counter, sizeof(counter));
    LoRa.endPacket();

    counter++;

    delay(5000);
}
