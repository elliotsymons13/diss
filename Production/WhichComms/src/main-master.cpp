

#include <WhichCommsCommon.h>
#include <WhichCommsMetrics.h>
#include "M5Unified.h"

LoRa868 lora; 
RF433 rf433;

#define RX_WAIT_TIME 700 // HACK must be greater than rx wait on slave side
#define RX_BUFF_SIZE 256

void setup() {
    M5.begin();
    LOG_SET_LEVEL(GLOBAL_LOG_LEVEL);
    await_serial(); // must await serial before setup, so failures can be logged

    if (!setup_common_lora868(&lora)) {
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

    WhichCommsMetrics lora868metrics;

    LOG_DEBUG("MASTER entering main loop");
    while (true) {

        // TODO do this for RF433 also
        // DONE add counter on send, to ensure the response back is matched correctly THEN add timer
        // DONE send then wait for response
        // DONE add timer
        // DONE add logger
        // TODO make everything above into functions, with logger, tidy
        // TODO fix millis() and micros() overflows at all occurances ref https://roboticsbackend.com/arduino-millis-vs-micros/
        // TODO MUST add range test element e.g. for antenna testing talk in report. My RTT is a bit unfounded in science I am learning. it is mostly testing my code speed..
        // TODO add UI to trigger each test
        // TODO REPORT ONLY: add average rtt, but would need to account for lost packets somehow
        // TODO loop over bearers agnostically 

        // RTT trigger message
        const size_t tx_data_len = 6;
        const uint8_t tx_data[tx_data_len] = {0xFF, 0x00, 0xFF, 0x00, 0xFF, lora868metrics.get_counter()};
        
        // LoRa868 send 
        LOG_DEBUG("SENDING LoRa868 RTT ping with counter", lora868metrics.get_counter());
        lora.send(tx_data, tx_data_len);
        lora868metrics.start_rtt();
        LOG_DEBUG("SENT LoRa868 RTT ping with counter", lora868metrics.get_counter());
        lora868metrics.increment_counter();

        // LoRa868 receive response
        LOG_DEBUG("WAITING for any LoRa868 RTT response (including out of sequence). ");
        uint8_t rx_buffer[RX_BUFF_SIZE] = {0};
        size_t rxd_size = 0;
        switch(lora.receiveOne((uint8_t*)rx_buffer, sizeof(rx_buffer), &rxd_size, RX_WAIT_TIME)) {
            case err::none:
                LOG_TRACE("LoRa868 packet received successfully.");
                LOG_TRACE("LoRa868 received response, length", rxd_size);
                LOG_TRACE("LoRa868 response payload:");
                log_bytes(rx_buffer, rxd_size);
                {
                    int rtt = 0;
                    if(lora868metrics.complete_rtt((uint8_t*)tx_data, tx_data_len, rx_buffer, rxd_size, &rtt)) {
                        if (rtt > 0) {
                            LOG_INFO("LoRa868 RTT was", rtt, "microseconds.");
                        }
                    }
                }
                break;
            case err::timeout_expired:
                LOG_TRACE("LoRa868 receive timed out.");
                break;
            case err::buffer_overflow:
                LOG_WARN("LoRa868 buffer overflow occurred.");
                break;
            case err::waiting:
                LOG_TRACE("LoRa868 waiting for packet.");
                break;
            default:
                LOG_ERROR("LoRa868 unknown error occurred.");
                break;
        }
       
        
        // RF433
        // LOG_DEBUG("SENDING RF433");
        // rf433.send(tx_data, tx_data_len);
        // LOG_DEBUG("SENT RF433");

        // TODO RF433 receive response
        
        delay(345); // TODO explicitly not 500 like any of my timeouts, to make timing distinct
        M5.update();
    }
}
