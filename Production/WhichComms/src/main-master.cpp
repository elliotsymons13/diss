

#include <WhichCommsCommon.h>
#include <WhichCommsMetrics.h>
// #include "M5Unified.h"
#include <M5GFX.h>
#include <M5CoreS3.h>
#include <UserInterface.h>

// user-configurable UI configuration constants
#define NUM_BUTTONS 3
#define NUM_STATUS_ROWS 1
#define NUM_BUTTON_ROWS 4

M5Canvas canvas;
M5GFX display;
UserInterface ui(display, canvas);

LoRa868 lora868; 
RF433 rf433;

#define RTT_INTERVAL 2000 // ms, 2 seconds
#define RX_WAIT_TIME 500
#define RX_BUFF_SIZE 256

bool setup_ui() {
	ui.configure(TEXT_SIZE, NUM_BUTTONS, NUM_BUTTON_ROWS, NUM_STATUS_ROWS);
	if (!ui.drawInitial()) {
		LOG_ERROR("UserInterface initial draw failed. ");
        return false;
	} else {
		LOG_TRACE("UserInterface initial draw succeeded. ");
	}

	ui.setStatusContent(0, "MASTER");
	ui.drawStatuses();
	ui.setButtonLabel(0, "SEND");
    ui.setButtonLabel(1, "<>");
    ui.setButtonLabel(2, "<>");
	ui.drawButtons();   
    return true;
}

// check if any button was newly pressed; if so, return true and set btn to buttonID
bool newButtonPress(UserInterface& ui, uint8_t& btn) {

	m5::Touch_Class::touch_detail_t touch = CoreS3.Touch.getDetail();
	
    if (touch.wasClicked()) {
        LOG_INFO("Touch was clicked. ");
    }
    if (touch.wasPressed()) {
        LOG_INFO("Touch was pressed. ");
    }

    // if (!touch.wasReleased()) {
	// 	return false;
	// }

	for (uint8_t i = 0; i < NUM_BUTTONS; i++) {
		if (ui.buttonWasPressed(i, touch.x, touch.y)) {
		LOG_TRACE("Button ", i, " was pressed. ");
		btn = i;
		return true;
		}
	}
	LOG_TRACE("A touch outside of any button area was detected. ");
	return false;

}

void setup() {
	// Initialise M5CoreS3
	auto cfg = M5.config();
	CoreS3.begin(cfg);
    
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

    if (!setup_ui()) {
        LOG_ERROR("Failed to setup UI. BLOCKING.");
        block_forever();
    }

}

uint8_t logs_written = 0;
uint8_t lastBtnPressed = -1;


bool packet_loss_test(WhichCommsMetrics& metrics, WhichCommsBearer& bearer) {
    const size_t tx_data_len = 6; // same as RF433
    const uint8_t tx_data[tx_data_len] = {0xFF, 0x00, 0xFF, 0x00, 0xFF, metrics.get_counter()};
           
    const uint8_t pings = 25;
    uint8_t acks = 0;
    for (uint8_t i = 0; i < pings; i++) {
        uint8_t rx_buffer[RX_BUFF_SIZE] = {0};
        size_t rxd_size = 0;
        int rtt = 0;

        // send ping and wait for response
        LOG_DEBUG("Sending LoRa868 ping number", i);
        lora868.send(tx_data, tx_data_len);
        if(metrics.complete_rtt((uint8_t*)tx_data, tx_data_len, rx_buffer, rxd_size, &rtt)) {
            LOG_INFO("Received LoRa868 response for ping number", i);
            acks++;
            if (rtt > 0) {
                LOG_INFO("RTT was", rtt, "microseconds.");
            }
        } else {
            LOG_INFO("Failed to receive LoRa868 response for ping number", i);
        }
    }
    LOG_INFO("LoRa868 packet loss rate:", (pings - acks) * 100.0f / pings, "%");
}

void loop() {

    WhichCommsMetrics lora868metrics;
    WhichCommsMetrics rf433metrics;
    int lastTimeSent433 = millis();
    int lastTimeSent868 = millis();

    // HACK illustrative packet loss metric, using RTT
    packet_loss_test(lora868metrics, lora868);
    while (true){} // HACK block to do above test only
    // FIXME should really factor about below main to a serials file, just so I don't have to keep messing around like this...
    
    LOG_DEBUG("MASTER entering main loop");
    while (true) {

        // TODO do this for RF433 also
        // DONE add counter on send, to ensure the response back is matched correctly THEN add timer
        // DONE send then wait for response
        // DONE add timer
        // DONE add logger
        // DONE make everything above into functions, with logger, tidy
        // TODO fix millis() and micros() overflows at all occurances ref https://roboticsbackend.com/arduino-millis-vs-micros/
        // TODO MUST add range test element e.g. for antenna testing talk in report. My RTT is a bit unfounded in science I am learning. it is mostly testing my code speed..
        // TODO add UI to trigger each test
        // TODO REPORT ONLY: add average rtt, but would need to account for lost packets somehow
        // TODO loop over bearers agnostically e.g. iterator/strategy pattern

        // Shared rx buffer for all bearers
        uint8_t rx_buffer[RX_BUFF_SIZE] = {0};
        size_t rxd_size = 0;

        // RTT trigger message
        const size_t tx_data_len = 6; // same as RF433
        const uint8_t lora868_tx_data[tx_data_len] = {0xFF, 0x00, 0xFF, 0x00, 0xFF, lora868metrics.get_counter()};
        
        // LoRa868 send, if sufficient time has passed
        if (millis() - lastTimeSent868 >= RTT_INTERVAL) {
            LOG_DEBUG("SENDING LoRa868 RTT ping with counter", lora868metrics.get_counter());
            lora868.send(lora868_tx_data, tx_data_len);
            lastTimeSent868 = millis();
            lora868metrics.start_rtt(); 
            LOG_DEBUG("SENT LoRa868 RTT ping with counter", lora868metrics.get_counter());
            lora868metrics.increment_counter();
        }

        // LoRa868 receive response
        LOG_DEBUG("WAITING for any LoRa868 RTT response (including out of sequence). ");
        switch(lora868.receiveOne((uint8_t*)rx_buffer, sizeof(rx_buffer), &rxd_size, RX_WAIT_TIME)) {
            case err::none:
                LOG_TRACE("LoRa868 packet received successfully.");
                LOG_TRACE("LoRa868 received response, length", rxd_size);
                LOG_TRACE("LoRa868 response payload:");
                log_bytes(rx_buffer, rxd_size);
                {
                    int rtt = 0;
                    if(lora868metrics.complete_rtt((uint8_t*)lora868_tx_data, tx_data_len, rx_buffer, rxd_size, &rtt)) {
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
            default:
                LOG_ERROR("LoRa868 unknown error occurred.");
                break;
        }
       
        // RF433 trigger message
        const uint8_t rf433_tx_data[tx_data_len] = {0xFF, 0x00, 0xFF, 0x00, 0xFF, rf433metrics.get_counter()};

        
        // RF433 send, if sufficient time has passed
        if (millis() - lastTimeSent433 >= RTT_INTERVAL) {
            LOG_DEBUG("SENDING RF433 with counter", rf433metrics.get_counter());
            rf433.send(rf433_tx_data, tx_data_len);
            lastTimeSent433 = millis();
            rf433metrics.start_rtt();
            LOG_DEBUG("SENT RF433 with counter", rf433metrics.get_counter());
            rf433metrics.increment_counter();
        }

        // RF433 receive response
        LOG_DEBUG("WAITING for any RF433 RTT response (including out of sequence). ");
        switch(rf433.receiveOne((uint8_t*)rx_buffer, sizeof(rx_buffer), &rxd_size, RX_WAIT_TIME)) {
            case err::none:
                LOG_TRACE("RF433 packet received successfully.");
                LOG_TRACE("RF433 received response, length", rxd_size);
                LOG_TRACE("RF433 response payload:");
                log_bytes(rx_buffer, rxd_size);
                {
                    int rtt = 0;
                    if(rf433metrics.complete_rtt((uint8_t*)rf433_tx_data, tx_data_len, rx_buffer, rxd_size, &rtt)) {
                        if (rtt > 0) {
                            LOG_INFO("RF433 RTT was", rtt, "microseconds.");
                        }
                    }
                }
                break;
            case err::timeout_expired:
                LOG_TRACE("RF433 receive timed out.");
                break;
            case err::buffer_overflow:
                LOG_WARN("RF433 buffer overflow occurred.");
                break;
            default:
                LOG_ERROR("RF433 unknown error occurred.");
                break;
        }

        bool newPress = newButtonPress(ui, lastBtnPressed);
        if (newPress) {
            ui.setLogContent(logs_written++, "Button " + String(lastBtnPressed) + " was pressed!\n");
        }
        
        ui.drawLogs();

        CoreS3.update();
    }
}
