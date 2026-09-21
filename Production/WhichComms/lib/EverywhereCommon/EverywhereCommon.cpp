#include "EverywhereCommon.h"

// log bytes if in trace only
void log_bytes(uint8_t* buff, size_t len) {
    if (GLOBAL_LOG_LEVEL != DebugLogLevel::LVL_TRACE) return;
    // guard against logging too many bytes e.g. due to signed integers being passed to unsigned len field
    if (len > 100) {
        LOG_WARN("Attempted to log too many bytes, ignoring", len);
        return;
    }

    for (size_t i = 0; i < len; i++) {
        Serial.printf("%02X ", buff[i]);
    }
    Serial.println();
}

bool equal(uint8_t* buff_a, uint8_t* buff_b, size_t len) {
    for (size_t i = 0; i < len; i++) {
        if (buff_a[i] != buff_b[i]) {
            return false;
        }
    }
    return true;
}