
#include <WhichCommsCommon.h>
#include "M5Unified.h"

LoRa868 lora868;
RF433 rf433;

#define RX_WAIT_TIME 50
#define RX_BUFF_SIZE 256

void setup() {
    M5.begin();
    LOG_SET_LEVEL(GLOBAL_LOG_LEVEL);
    await_serial(); // must await serial before setup, so failures can be logged

    if (!setup_common_lora868(&lora868)) {
        LOG_ERROR("Critical error. BLOCKING.");
        block_forever();
    }
    
    rf433.pre_config();
    if (!rf433.begin_tx_only_mode()) {
        LOG_ERROR("Failed to test initialize TX RF433.");
        block_forever();
    }
    if (!rf433.begin_rx_only_mode()) {
        LOG_ERROR("Failed to test initialize RX RF433.");
        block_forever();
    }
    rf433.post_config();

}

void loop() {

    uint8_t buffer[RX_BUFF_SIZE] = {0}; // arbitrary length, but library will handle any buffer overflow internally
    size_t rxd_size = 0;
    LOG_INFO("SLAVE Awaiting LoRa or RF433 packet.");
    while (true) {

        // LoRa868 test case 
        memset(buffer, 0, sizeof(buffer)); 
        rxd_size = 0; 
        switch (lora868.receiveOne((uint8_t*)buffer, sizeof(buffer), &rxd_size, RX_WAIT_TIME)) {
            case err::none: // packet rx'd successfully 
                LOG_DEBUG("Packet received", rxd_size, "bytes, echoing back");
                // TODO switch on packet contents e.g. RTT response
                LOG_TRACE("LoRa868 trigger+response payload:");
                lora868.send(buffer, rxd_size); // echo back the received packet
                break;
            case err::timeout_expired:
                LOG_TRACE("Nothing received within timeout. Could do other things now, but this example will just loop and try again."); // nothing rx'd
                break;
            case err::buffer_overflow:
                LOG_WARN("Buffer overflow occurred while receiving LoRa868 packet.");
                break;
            default:
                LOG_ERROR("Unsupported error occurred while receiving LoRa868 packet.");
                break;
        }

        // RF433 test case handling
        memset(buffer, 0, sizeof(buffer)); 
        rxd_size = 0; 
        switch (rf433.receiveOne((uint8_t*)buffer, sizeof(buffer), &rxd_size, RX_WAIT_TIME)) {
            case err::none: // packet rx'd successfully 
                LOG_DEBUG("RF433 Packet received", rxd_size, "bytes, echoing back");
                // TODO switch on packet contents e.g. RTT response
                LOG_TRACE("RF433 trigger payload:");
                log_bytes(buffer, rxd_size);
                rf433.send(buffer, rxd_size); // echo back the received packet
                break;
            case err::timeout_expired:
                LOG_TRACE("Nothing RF433 received within timeout."); 
                break;
            case err::buffer_overflow:
                LOG_WARN("Buffer overflow occurred while receiving RF433 packet.");
                break;
            default:
                LOG_ERROR("Unsupported error occurred while receiving RF433 packet.");
                break;
        }
        M5.update();
    }
}
