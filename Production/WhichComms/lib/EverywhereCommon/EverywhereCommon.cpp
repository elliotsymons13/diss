#include "EverywhereCommon.h"

void log_bytes(uint8_t* buff, size_t len) {
    for (size_t i = 0; i < len; i++) {
        Serial.printf("%02X ", buff[i]);
    }
    Serial.println();
}